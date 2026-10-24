#pragma once

#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/ClawTextureManager.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Graphics/TextureMipGenerator.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <stdexcept>

namespace claw_texture_retirement_budget_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using Microsoft::WRL::ComPtr;

    inline void require( bool value, const char *message )
    { if( !value ) throw std::runtime_error( message ); }
    struct Capacity
    {
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        static constexpr auto source = size_t( StreamMemoryClass::SourceCpu );
        static constexpr auto gpu = size_t( StreamMemoryClass::Gpu );
        u64 original = scheduler->stats().limits[source];
        u64 used() const { return scheduler->stats().used[source]; }
        u64 gpuUsed() const { return scheduler->stats().used[gpu]; }
        void set( u64 value )
        {
            String error;
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, value, error ),
                     "texture retirement Source limit applies" );
        }
        void restore() { set( original ); }
        ~Capacity() { restore(); }
    };

    struct LoadedApplication
    {
        SmartPtr<core::IApplicationManager> application = core::IApplicationManager::instance();
        LoadingState state;
        u8 flags;
        SmartPtr<IFactoryManager> factories;
        LoadedApplication()
        {
            require( application != nullptr, "texture retirement requires the initialized application" );
            state = application->getLoadingState();
            flags = application->getObjectFlags();
            factories = application->getFactoryManager();
            application->setObjectFlags( static_cast<u8>( flags &
                ~( OBJECT_FLAG_TRIGGER_EVENTS | OBJECT_FLAG_LOADING_EVENTS ) ) );
            application->setLoadingState( LoadingState::Loaded );
            application->setObjectFlags( flags );
            // This fixture exercises optional notification admission itself;
            // it does not require an EventJob factory or asynchronous dispatch.
            application->setFactoryManager(nullptr);
        }
        ~LoadedApplication()
        {
            application->setFactoryManager(factories);
            application->setObjectFlags( static_cast<u8>( flags &
                ~( OBJECT_FLAG_TRIGGER_EVENTS | OBJECT_FLAG_LOADING_EVENTS ) ) );
            application->setLoadingState(state);
            application->setObjectFlags(flags);
        }
    };

    class Manager : public ClawTextureManager
    {
    public:
        void retain( const SmartPtr<ITexture> &texture ) { m_textures.push_back(texture); }
        size_t count() const { return m_textures.size(); }
        size_t capacity() const { return m_textures.capacity(); }
    };
    class ThrowingTexture : public ClawTexture
    {
    public:
        Manager *manager = nullptr;
        bool detached = false;
        void unload( SmartPtr<ISharedObject> ) override
        {
            detached = manager && manager->count() == 0 && !manager->getByName("callback-texture");
            throw std::bad_alloc();
        }
    };

    inline void detachBeforeCallbacks()
    {
        Capacity budget;
        Manager manager;
        auto texture = make_ptr<ThrowingTexture>();
        texture->manager = &manager;
        texture->setName("callback-texture");
        manager.retain(texture);
        const auto before = budget.used();
        budget.set(before);
        bool threw = false;
        try { manager.destroyResource(texture); }
        catch( const std::bad_alloc & ) { threw = true; }
        require( threw && texture->detached && manager.count() == 0 &&
                 !manager.getByName("callback-texture") && budget.used() == before,
                 "manager destruction detaches its owner before a reentrant throwing unload callback" );
        texture = nullptr;
        const auto capacity = manager.capacity();
        const auto retained = budget.used();
        manager.unload(nullptr);
        require( manager.capacity() == 0 && budget.used() == retained -
                    sourceBudgetStorageBytes(capacity * sizeof(SmartPtr<ITexture>), alignof(SmartPtr<ITexture>)),
                 "full-cap manager teardown physically frees its retained empty inventory" );
    }

    inline void enableNotifications( ClawTexture &texture )
    {
        texture.setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        texture.setObjectFlag( OBJECT_FLAG_LOADING_EVENTS, true );
    }
    inline void warm( Manager &manager )
    {
        auto texture = dynamic_pointer_cast<ClawTexture>(manager.create("retirement-warm"));
        Array<u8> pixels(16,255);
        String error;
        require( texture && texture->copyByteData(pixels,{2,2},error),
                 "texture retirement cache metadata warm-up succeeds" );
        manager.destroyResource(texture);
        texture = nullptr;
    }

    inline void ordinaryPixels()
    {
        Capacity budget;
        Manager manager;
        warm(manager);
        const auto baseline = budget.used();
        auto texture = dynamic_pointer_cast<ClawTexture>(manager.create("retirement-pixels"));
        Array<u8> pixels(16,255);
        pixels[1] = 37;
        String error;
        require( texture && texture->copyByteData(pixels,{2,2},error),
                 "headless ordinary texture pixels are admitted" );
        texture->setLoadingState(LoadingState::Loaded);
        const auto beforeSnapshot = budget.used();
        auto snapshot = texture->getCpuBaseMip(error);
        const auto snapshotBytes = budget.used() - beforeSnapshot;
        require( snapshot && snapshotBytes >= pixels.size() && snapshot->bgra[1] == 37,
                 "detached CPU snapshot owns an admitted copy of the original pixels" );
        auto alias = texture;
        enableNotifications(*texture);
        const auto resident = budget.used();
        budget.set(resident);
        const auto epoch = sourceBudgetDenialCount();
        manager.destroyResource(texture);
        require( sourceBudgetDenialCount() > epoch && texture->getLoadingState() == LoadingState::Unloaded &&
                 manager.count() == 0 && !manager.getByName("retirement-pixels") &&
                 budget.used() == resident - (pixels.size() + 4096) && snapshot->bgra[1] == 37,
                 "full-cap notification denial cannot strand ordinary pixels or a ghost manager candidate" );
        texture = nullptr;
        alias = nullptr;
        require( budget.used() == baseline + snapshotBytes && snapshot->width == 2 && snapshot->bgra[1] == 37,
                 "escaped CPU snapshot remains charged after the final texture wrapper dies" );
        budget.set(budget.used());
        snapshot.reset();
        require( budget.used() == baseline, "the last CPU snapshot release retires its own admitted storage" );
        manager.unload(nullptr);
    }

    template<class Fixture> inline void retainedCookedOwners( Fixture &fixture )
    {
        Capacity budget;
        Manager manager;
        warm(manager);
        const auto baseline = budget.used();
        const auto gpuBaseline = budget.gpuUsed();
        auto texture = dynamic_pointer_cast<ClawTexture>(manager.create("retirement-cooked"));
        Array<TextureMipLevel> levels(1);
        levels[0].width = levels[0].height = 2;
        levels[0].bgra.resize(16,211);
        TextureMipSettings settings;
        settings.filter = TextureMipFilter::None;
        auto native = wp_renderer_get_dx11(fixture.renderer->getNativeRenderer());
        String error;
        require( texture && texture->uploadCookedMips(levels,settings,native,error),
                 "retained cooked texture fixture admits real CPU and GPU mip owners" );
        ComPtr<ID3D11ShaderResourceView> view =
            static_cast<ID3D11ShaderResourceView *>(texture->getNativeTextureView(native));
        ComPtr<ID3D11Resource> resource;
        require( view != nullptr, "cooked texture exposes its admitted native view" );
        view->GetResource(resource.GetAddressOf());
        const auto beforeSnapshot = budget.used();
        auto snapshot = texture->getCpuBaseMip(error);
        const auto snapshotBytes = budget.used() - beforeSnapshot;
        require( snapshot && budget.gpuUsed() == gpuBaseline + 16,
                 "retained native mip resources charge exactly the sixteen GPU pixel bytes" );
        auto alias = texture;
        enableNotifications(*texture);
        const auto resident = budget.used();
        budget.set(resident);
        const auto epoch = sourceBudgetDenialCount();
        manager.destroyResource(texture);
        require( sourceBudgetDenialCount() >= epoch + 2 &&
                 texture->getLoadingState() == LoadingState::Unloaded && manager.count() == 0 &&
                 !manager.getByName("retirement-cooked") && budget.used() == resident &&
                 budget.gpuUsed() == gpuBaseline + 16,
                 "both full-cap loading transitions can be denied while retained cooked CPU and native GPU owners survive" );
        texture = nullptr;
        require( budget.used() == resident, "a retained wrapper alias keeps authored cooked CPU storage charged" );
        alias = nullptr;
        require( budget.used() == baseline + snapshotBytes && snapshot->bgra[0] == 211 &&
                 budget.gpuUsed() == gpuBaseline + 16,
                 "final wrapper retirement leaves only the escaped CPU snapshot and physical native GPU owners" );
        view.Reset();
        require( budget.gpuUsed() == gpuBaseline + 16,
                 "a native resource reference retains GPU admission after the final view release" );
        resource.Reset();
        require( budget.gpuUsed() == gpuBaseline,
                 "the final native resource release retires its physical GPU reservation" );
        budget.set(budget.used());
        snapshot.reset();
        require( budget.used() == baseline,
                 "retained cooked CPU snapshot admission lasts exactly through its final physical owner" );
        manager.unload(nullptr);
    }

    template<class Fixture> inline void run( Fixture &fixture )
    {
        LoadedApplication application;
        detachBeforeCallbacks();
        ordinaryPixels();
        retainedCookedOwners(fixture);
    }
}
