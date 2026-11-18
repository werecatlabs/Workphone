#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Systems/MeshImposterSystem.hpp>
#include <Workphone/Scene/Components/MeshImposter.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, MeshImposterSystem, ComponentSystem );

    struct MeshImposterSystem::Storage::Page : SourceBudgetObject
    {
        std::array<Views, pageSize> views;
        std::array<SmartPtr<render::IMaterial>, pageSize> materials;
        std::array<Vector3F, pageSize> centres{};
        std::array<s32, pageSize> selectedViews;
        std::array<u64, pageSize> selectionChanges{};
        std::array<u64, pageSize> generations{};
        std::array<u8, pageSize> active{};
        std::array<u32, pageSize> nextFree{};

        Page()
        {
            selectedViews.fill( -1 );
            for( auto &frames : views )
            {
                frames = Views( Allocator<MeshImposterView>( sourceBudget ) );
            }
        }
    };

    MeshImposterSystem::MeshImposterSystem() = default;
    MeshImposterSystem::~MeshImposterSystem() = default;
    MeshImposterSystem::Storage::Storage() = default;
    MeshImposterSystem::Storage::~Storage() = default;

    TaskId MeshImposterSystem::getUpdateTask() const
    {
        return TaskId::LOD;
    }

    void MeshImposterSystem::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
        {
            return;
        }
        IComponentSystem::setLoadingState( LoadingState::Loading );
        ComponentSystem::load( data );
        IComponentSystem::setLoadingState( LoadingState::Loaded );
    }

    void MeshImposterSystem::unload( SmartPtr<ISharedObject> data )
    {
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;
        ScopedLock managerLock( manager );
        if( ISharedObject::getLoadingState() == LoadingState::Unloaded )
        {
            return;
        }
        IComponentSystem::setLoadingState( LoadingState::Unloading );
        Array<time_interval> retiredUpdates{ Allocator<time_interval>( sourceBudget ) };
        {
            ScopedLock lock( this );
            for( auto &component : m_components )
            {
                if( component && component->getComponentSystemPtr() == this )
                {
                    component->setComponentSystem( nullptr );
                    component->setComponentSystemData( nullptr );
                }
            }
            retiredUpdates.swap( m_nextUpdates );
            m_lastUpdateTime = 0;
        }
        ComponentSystem::unload( data );
        IComponentSystem::setLoadingState( LoadingState::Unloaded );
    }

    void MeshImposterSystem::reserveData( size_t size )
    {
        m_nextUpdates.resize( size, 0.0 );
    }

    u32 MeshImposterSystem::addComponent( SmartPtr<IComponent> component )
    {
        if( !component || !component->isDerived<MeshImposter>() )
        {
            return invalidIndex;
        }
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;
        ScopedLock managerLock( manager );
        ScopedLock lock( this );
        if( auto owner = component->getComponentSystem(); owner && owner.get() != this )
        {
            throw std::logic_error( "Mesh imposter is already registered with another system" );
        }
        for( u32 slot = 0; slot < getSize(); ++slot )
        {
            if( getObject( slot ) == component )
            {
                return slot;
            }
        }
        const auto slot = ComponentSystem::addComponent( component );
        if( slot != invalidIndex )
        {
            m_nextUpdates[slot] = 0;
            component->setComponentSystem( this );
        }
        return slot;
    }

    void MeshImposterSystem::removeComponent( SmartPtr<IComponent> component )
    {
        if( !component )
        {
            return;
        }
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;
        ScopedLock managerLock( manager );
        ScopedLock lock( this );
        // Manual system registration need not share the component factory's ID.
        for( u32 slot = 0; slot < getSize(); ++slot )
        {
            if( getObject( slot ) == component )
            {
                removeComponent( slot );
                return;
            }
        }
    }

    void MeshImposterSystem::removeComponent( u32 id )
    {
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app ? app->getGameManager() : nullptr;
        ScopedLock managerLock( manager );
        ScopedLock lock( this );
        if( id >= getSize() )
        {
            return;
        }
        auto component = getObject( id );
        if( !component )
        {
            return;
        }
        if( component->getComponentSystemPtr() == this )
        {
            component->setComponentSystem( nullptr );
            component->setComponentSystemData( nullptr );
        }
        m_nextUpdates[id] = 0;
        ComponentSystem::removeComponent( id );
        m_lastFreeSlot = std::min( m_lastFreeSlot.load(), id );
    }

    void MeshImposterSystem::update()
    {
        if( Thread::getCurrentTask() != TaskId::LOD || !isLoaded() )
        {
            return;
        }

        auto app = core::IApplicationManager::instancePtr();
        auto manager = app->getGameManager();
        auto timer = app->getTimer();

        if( !manager || !timer )
        {
            return;
        }

        const auto now = timer->getTime();

        ScopedLock lock( this );
        if( now < m_lastUpdateTime )
        {
            std::fill( m_nextUpdates.begin(), m_nextUpdates.end(), 0.0 );
        }

        m_lastUpdateTime = now;
        for( u32 slot = 0; slot < getSize(); ++slot )
        {
            auto component = getObject( slot );
            if( !component || !component->isLoaded() || !component->isEnabled() ||
                now < m_nextUpdates[slot] )
            {
                continue;
            }

            auto actor = component->getActor();
            if( !actor || !actor->isEnabledInScene() )
            {
                continue;
            }

            m_nextUpdates[slot] = now + ( 1.0 / 3.0 );
            
            // Respect the enclosing LOD group's owner, including explicit view
            // overrides supplied by applications using a raw render camera.
            for( auto owner = actor; owner; owner = owner->getParent() )
            {
                auto group = owner->getComponent<LODGroup>();
                auto system =
                    group ? dynamic_pointer_cast<LODSystem>( group->getComponentSystem() ) : nullptr;
                if( !system )
                {
                    continue;
                }

                LODSystem::View view;
                if( system->getView( view ) )
                {
                    static_pointer_cast<MeshImposter>( component )->updateForView( view );
                }

                break;
            }
        }
    }

    MeshImposterSystem::Storage &MeshImposterSystem::instance()
    {
        static Storage storage;
        return storage;
    }

    MeshImposterSystem::Handle MeshImposterSystem::Storage::create()
    {
        std::lock_guard<std::recursive_mutex> lock( m_storageMutex );
        if( m_freeHead == invalidIndex )
        {
            if( m_pages.size() >= invalidIndex / pageSize )
            {
                throw std::length_error( "Mesh imposter slots exhausted" );
            }
            const auto base = static_cast<u32>( m_pages.size() ) * pageSize;
            auto page = std::make_unique<Page>();
            for( u32 i = 0; i < pageSize; ++i )
            {
                page->nextFree[i] = i + 1 < pageSize ? base + i + 1 : invalidIndex;
            }
            m_pages.push_back( std::move( page ) );
            m_freeHead = base;
        }
        const auto index = m_freeHead;
        auto &page = *m_pages[index / pageSize];
        const auto slot = index % pageSize;
        m_freeHead = page.nextFree[slot];
        page.active[slot] = 1;
        ++m_activeCount;
        return { index, page.generations[slot] };
    }

    bool MeshImposterSystem::Storage::containsUnlocked( Handle handle ) const
    {
        if( handle.index / pageSize >= m_pages.size() )
        {
            return false;
        }
        const auto &page = *m_pages[handle.index / pageSize];
        const auto slot = handle.index % pageSize;
        return page.active[slot] && page.generations[slot] == handle.generation;
    }

    void MeshImposterSystem::Storage::destroy( Handle handle )
    {
        // Material teardown can enter graphics. Retire resource owners after
        // releasing storage, preserving the publication lock order.
        Views retiredViews{ Allocator<MeshImposterView>( sourceBudget ) };
        SmartPtr<render::IMaterial> retiredMaterial;
        std::lock_guard<std::recursive_mutex> lock( m_storageMutex );
        if( !containsUnlocked( handle ) )
        {
            return;
        }
        auto &page = *m_pages[handle.index / pageSize];
        const auto slot = handle.index % pageSize;
        page.views[slot].swap( retiredViews );
        retiredMaterial = page.materials[slot];
        page.materials[slot] = nullptr;
        page.centres[slot] = Vector3F::zero();
        page.selectedViews[slot] = -1;
        page.selectionChanges[slot] = 0;
        page.active[slot] = 0;
        ++page.generations[slot];
        page.nextFree[slot] = m_freeHead;
        m_freeHead = handle.index;
        --m_activeCount;
    }

    MeshImposterSystem::Storage::Access MeshImposterSystem::Storage::access( Handle handle )
    {
        std::unique_lock<std::recursive_mutex> lock( m_storageMutex );
        if( !containsUnlocked( handle ) )
        {
            throw std::logic_error( "Stale mesh imposter storage handle" );
        }
        auto &page = *m_pages[handle.index / pageSize];
        const auto slot = handle.index % pageSize;
        return { std::move( lock ),  page.views[slot],         page.materials[slot],
                 page.centres[slot], page.selectedViews[slot], page.selectionChanges[slot] };
    }

    bool MeshImposterSystem::Storage::contains( Handle handle ) const
    {
        std::lock_guard<std::recursive_mutex> lock( m_storageMutex );
        return containsUnlocked( handle );
    }

    size_t MeshImposterSystem::Storage::getActiveCount() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_storageMutex );
        return m_activeCount;
    }

    size_t MeshImposterSystem::Storage::getSlotCount() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_storageMutex );
        return m_pages.size() * pageSize;
    }
}  // namespace workphone::scene
