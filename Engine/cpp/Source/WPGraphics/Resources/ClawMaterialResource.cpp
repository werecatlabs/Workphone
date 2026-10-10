#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/Resources/ClawMaterialResource.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <WPGraphics/ClawMaterialTechnique.hpp>
#include <WPGraphics/ClawMaterialPass.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <thread>
#include <limits>

namespace workphone::render
{
    struct ClawMaterialResource::Candidate::Data
    {
        SmartPtr<ClawMaterial> material;
        SmartPtr<ClawTexture> texture;
        std::shared_ptr<const resource::RuntimeResource> resource;
        CatalogResourceAdapter::Request request;
        std::shared_ptr<const int> publisher;
        u64 ticket = 0;
        Microsoft::WRL::ComPtr<ID3D11Device> device;
    };

    struct ClawMaterialResource::Data
    {
        SmartPtr<AssetDatabaseManager> catalog;
        CatalogResourceAdapter adapter;
        SmartPtr<ClawRendererDX11> renderer;
        String target;
        bool packagedBuild;
        const std::thread::id owner = std::this_thread::get_id();
        std::shared_ptr<const int> publisher = std::make_shared<const int>( 0 );
        u64 ticket = 0;
        std::shared_ptr<const Candidate> visible;

        Data( SmartPtr<AssetDatabaseManager> catalog,
              std::shared_ptr<resource::IResourceSystem> resources, const String &root,
              const Array<CatalogResourceAdapter::TypeMapping> &mappings,
              SmartPtr<ClawRendererDX11> renderer, const String &target, bool packagedBuild ) :
            catalog( catalog ), adapter( catalog, resources, root, mappings ),
            renderer( renderer ), target( target ), packagedBuild( packagedBuild )
        {
        }

        bool checkOwner( String &error ) const
        {
            if( std::this_thread::get_id() == owner )
                return true;
            error = "Cooked material operations require the constructing render thread.";
            return false;
        }

        wp_renderer_dx11 *activeRenderer() const
        {
            auto application = core::IApplicationManager::instancePtr();
            auto graphics = application ? application->getGraphicsSystemPtr() : nullptr;
            if( !renderer || !renderer->isLoaded() || !graphics ||
                graphics->getRenderer().get() != renderer.get() )
                return nullptr;
            return wp_renderer_get_dx11( renderer->getNativeRenderer() );
        }

        ID3D11Device *device() const
        {
            auto native = activeRenderer();
            return native ? static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) )
                          : nullptr;
        }
    };

    ClawMaterialResource::Candidate::Candidate( std::shared_ptr<Data> data ) :
        m_data( std::move( data ) )
    {
    }
    ClawMaterialResource::Candidate::~Candidate() = default;
    SmartPtr<ClawMaterial> ClawMaterialResource::Candidate::material() const
    {
        return m_data->material;
    }
    SmartPtr<ClawTexture> ClawMaterialResource::Candidate::texture() const
    {
        return m_data->texture;
    }
    std::shared_ptr<const resource::RuntimeResource>
    ClawMaterialResource::Candidate::resource() const
    {
        return m_data->resource;
    }

    ClawMaterialResource::ClawMaterialResource(
        SmartPtr<AssetDatabaseManager> catalog,
        std::shared_ptr<resource::IResourceSystem> resources, const String &sourceRoot,
        const Array<CatalogResourceAdapter::TypeMapping> &mappings,
        SmartPtr<ClawRendererDX11> renderer, const String &target, bool packagedBuild ) :
        m_data( std::make_unique<Data>( catalog, resources, sourceRoot, mappings, renderer, target,
                                       packagedBuild ) )
    {
    }
    ClawMaterialResource::~ClawMaterialResource() = default;

    std::shared_ptr<const ClawMaterialResource::Candidate> ClawMaterialResource::stage(
        const String &uuid, String &error, bool force )
    {
        error.clear();
        if( !m_data->checkOwner( error ) )
            return {};
        if( m_data->ticket == std::numeric_limits<u64>::max() )
        {
            error = "Cooked material request sequence exhausted; recreate the owner.";
            return {};
        }
        const auto ticket = ++m_data->ticket;
        try
        {
            auto candidate = std::make_shared<Candidate::Data>();
            candidate->publisher = m_data->publisher;
            candidate->ticket = ticket;
            candidate->device = m_data->device(); // Pins identity through device recreation.
            if( !candidate->device )
            {
                error = "Cooked material requires the active DX11 renderer.";
                return {};
            }
            if( !m_data->adapter.resolve( uuid, candidate->request, error ) )
                return {};
            if( candidate->request.resourceId.type() != resource::ResourceTypeID( "matres" ) )
            {
                error = "Cooked material requires a matres catalog entry.";
                return {};
            }
            resource::CompilationOptions options;
            options.force = force;
            options.packagedBuild = m_data->packagedBuild;
            const auto report = m_data->adapter.compile( candidate->request, options );
            if( !report.succeeded() )
            {
                error = "Cooked material compilation failed.";
                for( const auto &message : report.messages )
                    error += " " + message;
                return {};
            }
            candidate->resource = m_data->adapter.load( candidate->request, error );
            if( !candidate->resource )
                return {};
            // Compilation and loading are separate service operations. Another
            // caller may recook this ID without changing the catalog generation.
            if( candidate->resource->header.resourceId != report.resourceId ||
                candidate->resource->header.sourceHash != report.sourceHash ||
                candidate->resource->header.payloadHash != report.outputHash )
            {
                error = "Cooked material generation changed between compilation and loading.";
                return {};
            }
            CookedMaterialData material;
            if( !decodeCookedMaterial( *candidate->resource, material, error ) )
                return {};
            if( material.target != m_data->target ||
                material.packagedBuild != m_data->packagedBuild )
            {
                error = "Cooked material target/build mode does not match its consumer.";
                return {};
            }
            CookedTextureData texture;
            if( !decodeCookedTexture( *candidate->resource->dependencies.at( 0 ), texture, error ) )
                return {};
            if( candidate->device.Get() != m_data->device() )
            {
                error = "DX11 device changed while staging the cooked material.";
                return {};
            }
            auto application = core::IApplicationManager::instancePtr();
            if( !application || !application->getStateManager() || !application->getFactoryManager() ||
                !application->getTimer() )
            {
                error = "Cooked material requires application state, factory and timer services.";
                return {};
            }
            candidate->texture = make_ptr<ClawTexture>();
            if( !candidate->texture->uploadCookedMips( texture.levels, texture.mipSettings,
                                                       m_data->activeRenderer(), error ) )
                return {};
            candidate->material = make_ptr<ClawMaterial>();
            auto technique = make_ptr<ClawMaterialTechnique>();
            technique->setMaterial( candidate->material );
            auto pass = technique->createPass();
            candidate->material->addTechnique( technique );
            candidate->material->setDiffuse( material.baseColour );
            // This format uses the metallic/roughness model. The legacy pass
            // default is white specular, which suppresses dielectric diffuse.
            candidate->material->setSpecular( ColourF( .04f, .04f, .04f, 1.0f ) );
            candidate->material->setMetalness( material.metalness );
            candidate->material->setRoughness( material.roughness );
            pass->setTexture( candidate->texture, 0 );
            if( !pass->getStateContext() || !candidate->material->getNativeMaterial() ||
                candidate->material->getTexture( 0 ) != candidate->texture )
            {
                error = "Cooked material state/texture installation failed.";
                return {};
            }
            return std::shared_ptr<const Candidate>( new Candidate( std::move( candidate ) ) );
        }
        catch( const std::exception &e )
        {
            error = String( "Cooked material staging failed: " ) + e.what();
            return {};
        }
    }

    bool ClawMaterialResource::publish( std::shared_ptr<const Candidate> candidate, String &error )
    {
        error.clear();
        if( !m_data->checkOwner( error ) )
            return false;
        if( !candidate || candidate->m_data->publisher != m_data->publisher ||
            candidate->m_data->ticket != m_data->ticket )
        {
            error = "Cooked material candidate belongs to an obsolete or different request.";
            return false;
        }
        const auto device = m_data->device();
        if( !device || candidate->m_data->device.Get() != device ||
            FAILED( device->GetDeviceRemovedReason() ) )
        {
            error = "Cooked material candidate belongs to an unavailable or replaced DX11 device.";
            return false;
        }
        // Compile, decode, GPU creation and material construction all finish before
        // this lock. The final catalog check and noexcept pointer swap are indivisible
        // with catalog mutations. The retired handle is released AFTER unlocking.
        auto catalog = m_data->catalog;
        if( !catalog )
        {
            error = "Cooked material catalog is unavailable.";
            return false;
        }
        // Adapter validation may query a compiler's outputs. Keep that extensible
        // code outside the catalog lock; recheck only the catalog token inside.
        try
        {
            if( !m_data->adapter.isCurrent( candidate->m_data->request, error ) )
                return false;
        }
        catch( const std::exception &e )
        {
            error = String( "Cooked material publication validation failed: " ) + e.what();
            return false;
        }
        if( candidate->m_data->publisher != m_data->publisher ||
            candidate->m_data->ticket != m_data->ticket || device != m_data->device() ||
            FAILED( device->GetDeviceRemovedReason() ) )
        {
            error = "Cooked material request/device changed during publication validation.";
            return false;
        }
        {
            ScopedLock lock( catalog.get() );
            if( !catalog->isEntryCurrent( candidate->m_data->request.entry ) )
            {
                error = "Cooked material catalog generation changed before publication.";
                return false;
            }
            m_data->visible.swap( candidate );
        }
        return true;
    }

    bool ClawMaterialResource::reload( const String &uuid, String &error, bool force )
    {
        auto candidate = stage( uuid, error, force );
        return candidate && publish( std::move( candidate ), error );
    }

    std::shared_ptr<const ClawMaterialResource::Candidate> ClawMaterialResource::current() const
    {
        // A retained old handle remains valid for its original device, but callers
        // must reacquire current() when drawing after a renderer lifecycle change.
        if( std::this_thread::get_id() != m_data->owner || !m_data->visible )
            return {};
        const auto device = m_data->device();
        if( !device || m_data->visible->m_data->device.Get() != device ||
            FAILED( device->GetDeviceRemovedReason() ) )
            return {};
        return m_data->visible;
    }

    bool ClawMaterialResource::unload( String &error )
    {
        error.clear();
        if( !m_data->checkOwner( error ) )
            return false;
        if( m_data->ticket != std::numeric_limits<u64>::max() )
            ++m_data->ticket;
        m_data->publisher = std::make_shared<const int>( 0 );
        m_data->visible.reset();
        return true;
    }
}
