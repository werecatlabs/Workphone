#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Systems/ComponentSystem.hpp>
#include <Workphone/Interface/Scene/IComponent.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone, ComponentSystem, IComponentSystem );

    ComponentSystem::ComponentSystem() = default;

    ComponentSystem::~ComponentSystem() = default;

    void ComponentSystem::load( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        applicationManager->addObjectListener( this );

        reserve( 12 );
        m_dirtyComponents.reserve( 12 );
    }

    void ComponentSystem::unload( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        applicationManager->removeObjectListener( this );
    }

    u32 ComponentSystem::addComponent( SmartPtr<IComponent> component )
    {
        ScopedLock lock( this );

        WP_ASSERT( isValid() );

        const auto retries = 10;
        for( size_t i = 0; i < retries; ++i )
        {
            for( u32 i = m_lastFreeSlot.load() + 1; i < getSize(); ++i )
            {
                if( isFreeSlot( i ) )
                {
                    setLoadingState( i, LoadingState::Allocated );

                    WP_ASSERT( getObject( i ) == nullptr );
                    setObject( i, component );

                    //auto componentState = m_data[i];
                    //component->addState( componentState );
                    //WP_ASSERT( !component->getStates().empty() );

                    m_lastFreeSlot = i;
                    return i;
                }
            }

            for( u32 i = 0; i < m_lastFreeSlot; ++i )
            {
                if( isFreeSlot( i ) )
                {
                    setLoadingState( i, LoadingState::Allocated );

                    WP_ASSERT( getObject( i ) == nullptr );
                    setObject( i, component );

                    //auto componentState = m_data[i];
                    //component->addState( componentState );
                    //WP_ASSERT( !component->getStates().empty() );

                    m_lastFreeSlot = i;
                    return i;
                }
            }

            WP_ASSERT( isValid() );

            // grow the arrays
            auto growSize = getGrowSize();
            const auto currentSize = getSize();
            reserve( ( currentSize + growSize ) * 2 );

            WP_ASSERT( isValid() );
        }

        WP_ASSERT( isValid() );
        return std::numeric_limits<u32>::max();
    }

    void ComponentSystem::removeComponent( u32 id )
    {
        ScopedLock lock( this );

        WP_ASSERT( isValid() );
        if( id >= getSize() )
            return;

        setLoadingState( id, LoadingState::Unallocated );
        setObject( id, nullptr );
    }

    void ComponentSystem::removeComponent( SmartPtr<IComponent> component )
    {
        ScopedLock lock( this );

        if( !component )
            return;
        auto handle = component->getHandle();
        if( !handle )
            return;
        auto id = handle->getInstanceId();
        if( id >= getSize() || getObject( id ) != component.get() )
            return;

        WP_ASSERT( isValid() );
        if( id >= getSize() )
            return;

        setLoadingState( id, LoadingState::Unallocated );
        setObject( id, nullptr );
    }

    void ComponentSystem::reserve( size_t size )
    {
        WP_ASSERT( isValid() );

        ScopedLock lock( this );

        auto currentSize = getSize();
        if( currentSize != size )
        {
            m_components.resize( size );
            m_loadingStates.resize( size );

            for( size_t i = currentSize; i < size; ++i )
            {
                m_loadingStates[i] = LoadingState::Unallocated;
            }

            reserveData( size );

            setSize( size );
        }
    }

    size_t ComponentSystem::getSize() const
    {
        return m_size;
    }

    void ComponentSystem::setSize( size_t size )
    {
        m_size = size;
    }

    u32 ComponentSystem::getGrowSize() const
    {
        return m_growSize;
    }

    void ComponentSystem::setGrowSize( u32 growSize )
    {
        m_growSize = growSize;
    }

    void ComponentSystem::reserveData( size_t size )
    {
    }

    bool ComponentSystem::isFreeSlot( u32 slot )
    {
        const AtomicValue<LoadingState> unallocatedState = LoadingState::Unallocated;
        const auto &loadingState = getLoadingState( slot );

        if( loadingState == unallocatedState )
        {
            return true;
        }

        return false;
    }

    const AtomicValue<LoadingState> &ComponentSystem::getLoadingState( u32 id ) const
    {
        WP_ASSERT( id < getSize() );
        return m_loadingStates[id];
    }

    void ComponentSystem::setLoadingState( u32 id, LoadingState state )
    {
        WP_ASSERT( id < getSize() );
        m_loadingStates[id] = state;
    }

    void ComponentSystem::setObject( u32 index, SmartPtr<IComponent> component )
    {
        WP_ASSERT( index < getSize() );
        m_components[index] = component.get();
    }

    SmartPtr<IComponent> ComponentSystem::getObject( u32 index ) const
    {
        WP_ASSERT( index < getSize() );
        return m_components[index];
    }

    void ComponentSystem::setComponents( const Array<IComponent *> &components )
    {
        m_components = { components.begin(), components.end() };
    }

    Array<IComponent *> ComponentSystem::getComponents() const
    {
        return m_components.snapshot();
    }

    void ComponentSystem::setDirty( bool dirty )
    {
        m_dirty = dirty;
    }

    bool ComponentSystem::isDirty() const
    {
        return m_dirty;
    }

    void ComponentSystem::makeDirty()
    {
        m_dirty = true;
    }

    void ComponentSystem::addDirtyComponent( SmartPtr<IComponent> component )
    {
        ScopedLock lock( this );

        auto it = std::find( m_dirtyComponents.begin(), m_dirtyComponents.end(), component.get() );
        if( it == m_dirtyComponents.end() )
        {
            m_dirtyComponents.push_back( component.get() );
        }

        makeDirty();
    }

    void ComponentSystem::removeDirtyComponent( SmartPtr<IComponent> component )
    {
        ScopedLock lock( this );

        m_dirtyComponents.erase(
            std::remove( m_dirtyComponents.begin(), m_dirtyComponents.end(), component.get() ),
            m_dirtyComponents.end() );
    }

    Parameter ComponentSystem::handleEvent( EventType eventType, hash_type eventValue,
                                            const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                            SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        return {};
    }

    void ComponentSystem::setDirtyComponents( const Array<IComponent *> &dirtyComponents )
    {
        m_dirtyComponents = { dirtyComponents.begin(), dirtyComponents.end() };
    }

    Array<IComponent *> ComponentSystem::getDirtyComponents() const
    {
        return m_dirtyComponents.snapshot();
    }

    void ComponentSystem::lock()
    {
        m_mutex.lock();
    }

    bool ComponentSystem::try_lock()
    {
        return m_mutex.try_lock();
    }

    void ComponentSystem::unlock()
    {
        m_mutex.unlock();
    }

}  // namespace workphone::scene
