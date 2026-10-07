#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/FSMManager.hpp>
#include <Workphone/System/FSM.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/System/IFSMListener.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, FSMManager, IFSMManager );

    atomic_s32 FSMManager::m_idExt = 0;

    FSMManager::FSMManager() = default;

    FSMManager::~FSMManager() = default;

    auto FSMManager::createFSM() -> SmartPtr<IFSM>
    {
        ScopedLock lock( &m_mutex );

        WP_ASSERT( isValid() );
        WP_ASSERT( isLoaded() );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto fsm = factoryManager->make_ptr<FSM>();
        WP_ASSERT( fsm );

        fsm->setFsmManager( this );

        auto handle = fsm->getHandle();
        WP_ASSERT( handle );

        auto id = createNewId();
        while( id >= getSize() )
        {
            auto growSize = getGrowSize();
            auto size = getSize() + growSize;
            resize( size );
        }

        WP_ASSERT( id < m_fsms.size() );

        handle->setInstanceId( id );

        m_fsms[id] = fsm;

        fsm->load( nullptr );

        return fsm;
    }

    void FSMManager::destroyFSM( SmartPtr<IFSM> fsm )
    {
        ScopedLock lock( &m_mutex );

        WP_ASSERT( fsm );
        WP_ASSERT( isValid() );
        WP_ASSERT( isLoaded() );

        fsm->unload( nullptr );

        fsm->setFsmManager( nullptr );

        auto it = std::find( m_fsms.begin(), m_fsms.end(), fsm );
        if( it != m_fsms.end() )
        {
            auto handle = fsm->getHandle();
            WP_ASSERT( handle );

            auto id = handle->getInstanceId();
            m_fsms[id] = nullptr;
        }
    }

    Array<SmartPtr<IFSM>> FSMManager::getFsms() const
    {
        return m_fsms.snapshot();
    }

    u32 FSMManager::getNumFsms() const
    {
        return (u32)m_fsms.size();
    }

    auto FSMManager::createNewId() -> u32
    {
        WP_ASSERT( isValid() );
        WP_ASSERT( isLoaded() );

        m_idCount = m_idExt++;
        return m_idCount;
    }

    void FSMManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto growSize = getGrowSize();
            auto size = getSize() + growSize;
            resize( size );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FSMManager::resize( size_t size )
    {
        ScopedLock lock( &m_mutex );

        auto currentSize = getSize();
        if( currentSize != size )
        {
            m_fsms.resize( size );

            m_previousStates.resize( size );
            m_currentStates.resize( size );
            m_newStates.resize( size );
            m_listeners.resize( size );

            m_stateChangeTimes.resize( size );
            m_stateTimes.resize( size );
            m_flags.resize( size );
            m_ready.resize( size );

            // Resize new member arrays
            m_autoChangeState.resize( size );
            m_allowStateChange.resize( size );
            m_stateChangeComplete.resize( size );
            m_autoTriggerEnterStateComplete.resize( size );
            m_stateTicks.resize( size );
            m_listenerPriority.resize( size );

            setSize( size );
        }
    }

    void FSMManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            for( auto &fsm : m_fsms )
            {
                if( fsm )
                {
                    fsm->unload( nullptr );
                }
            }

            for( auto listeners : m_listeners )
            {
                for( auto listener : listeners )
                {
                    if( listener )
                    {
                        listener->unload( nullptr );
                    }
                }
            }

            m_listeners.clear();
            m_fsms.clear();

            // Clear member arrays
            m_stateChangeTimes.clear();
            m_stateTimes.clear();
            m_flags.clear();
            m_ready.clear();

            m_autoChangeState.clear();
            m_allowStateChange.clear();
            m_stateChangeComplete.clear();
            m_autoTriggerEnterStateComplete.clear();
            m_stateTicks.clear();
            m_listenerPriority.clear();

            m_previousStates.clear();
            m_currentStates.clear();
            m_newStates.clear();

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FSMManager::changeState()
    {
        auto size = getSize();
        for( size_t i = 0; i < size; ++i )
        {
            if( m_fsms[i] != nullptr )
            {
                const auto &currentState = m_currentStates[i];
                const auto &newState = m_newStates[i];

                if( currentState != newState )
                {
                    auto listeners = getListeners( static_cast<u32>( i ) );

                    if( !listeners.empty() )
                    {
                        for( auto &listener : listeners )
                        {
                            if( listener )
                            {
                                auto leaveReturn =
                                    listener->handleEvent( currentState, FSMEvent::Leave );
                                if( leaveReturn != FSMReturnType::Ok )
                                {
                                    continue;
                                }

                                auto enterReturn = listener->handleEvent( newState, FSMEvent::Enter );
                                if( enterReturn != FSMReturnType::Ok )
                                {
                                    continue;
                                }

                                setPreviousState( static_cast<u32>( i ), (s32)currentState );
                                setCurrentState( static_cast<u32>( i ), (s32)newState );

                                const auto result =
                                    listener->handleEvent( newState, FSMEvent::Complete );
                                if( result != FSMReturnType::Ok )
                                {
                                    auto stateStr = StringUtil::toString( newState );
                                    WP_LOG( "State change complete not ok: " + stateStr );
                                }
                            }
                        }
                    }
                    else
                    {
                        setCurrentState( static_cast<u32>( i ), (s32)newState );
                    }

                    setStateChangeComplete( static_cast<u32>( i ), true );

                    auto applicationManager = core::IApplicationManager::instance();
                    auto timer = applicationManager->getTimer();
                    auto t = timer->getTime();
                    setStateChangeTime( static_cast<u32>( i ), t );
                }
            }
        }
    }

    void FSMManager::changeState( u32 i )
    {
        if( m_fsms[i] != nullptr )
        {
            auto currentState = m_currentStates[i];
            auto newState = m_newStates[i];

            if( currentState != newState )
            {
                auto listeners = getListeners( i );

                if( !listeners.empty() )
                {
                    for( auto listener : listeners )
                    {
                        if( listener )
                        {
                            listener->handleEvent( currentState, FSMEvent::Leave );
                        }

                        setCurrentState( i, newState );

                        if( listener )
                        {
                            listener->handleEvent( newState, FSMEvent::Enter );
                        }

                        if( listener )
                        {
                            const auto result = listener->handleEvent( newState, FSMEvent::Complete );
                            if( result != FSMReturnType::Ok )
                            {
                                auto stateStr = StringUtil::toString( newState );
                                WP_LOG( "State change complete not ok: " + stateStr );
                            }
                        }
                    }
                }
                else
                {
                    setCurrentState( i, newState );
                }

                setStateChangeComplete( i, true );

                auto applicationManager = core::IApplicationManager::instance();

                if( auto timer = applicationManager->getTimer() )
                {
                    auto t = timer->getTime();
                    setStateChangeTime( i, t );
                }
            }
        }
    }

    void FSMManager::queueDirtyFSM( SmartPtr<IFSM> fsm )
    {
        m_dirtyQueue.push( fsm );
    }

    void FSMManager::update()
    {
#if 1
        if( !m_dirtyQueue.empty() )
        {
            Array<SmartPtr<IFSM>> fsms;
            fsms.reserve( m_dirtyQueue.size() );

            SmartPtr<IFSM> fsm;
            while( m_dirtyQueue.try_pop( fsm ) )
            {
                if( fsm )
                {
                    fsms.push_back( fsm );
                }
            }

            // sort by priority
            std::sort( fsms.begin(), fsms.end(),
                       [this]( const SmartPtr<IFSM> &a, const SmartPtr<IFSM> &b ) -> bool {
                           auto aPriority = a->getPriority();
                           auto bPriority = b->getPriority();
                           return aPriority > bPriority;
                       } );

            for( auto &fsm : fsms )
            {
                if( fsm )
                {
                    fsm->update();
                }
            }
        }
#else
        RecursiveMutex::ScopedLock lock( m_mutex );

        WP_ASSERT( isLoaded() );

        auto size = getSize();

        auto applicationManager = core::IApplicationManager::instance();
        auto &timer = applicationManager->getTimer();
        if( timer )
        {
            auto dt = timer->getDeltaTime();

            for( size_t i = 0; i < size; ++i )
            {
                if( m_fsms[i] != nullptr )
                {
                    addStateTime( static_cast<u32>( i ), dt );
                }
            }
        }

        changeState();
#endif
    }

    auto FSMManager::getStateChangeTime( u32 id ) const -> f64
    {
        WP_ASSERT( id < m_stateChangeTimes.size() );
        return m_stateChangeTimes[id];
    }

    void FSMManager::setStateChangeTime( u32 id, const f64 &stateChangeTime )
    {
        WP_ASSERT( id < m_stateChangeTimes.size() );
        m_stateChangeTimes[id] = stateChangeTime;
    }

    auto FSMManager::getPreviousState( u32 id ) const -> u8
    {
        if( id < m_previousStates.size() )
        {
            return m_previousStates[id];
        }

        return 0;
    }

    void FSMManager::setPreviousState( u32 id, s32 state )
    {
        if( id < m_previousStates.size() )
        {
            m_previousStates[id] = state;
        }
    }

    auto FSMManager::getCurrentState( u32 id ) const -> u8
    {
        if( id < m_currentStates.size() )
        {
            return m_currentStates[id];
        }

        return 0;
    }

    void FSMManager::setCurrentState( u32 id, s32 state )
    {
        if( id < m_currentStates.size() )
        {
            m_currentStates[id] = state;
        }
    }

    auto FSMManager::getNewState( u32 id ) const -> u8
    {
        if( id < m_newStates.size() )
        {
            return m_newStates[id];
        }

        return 0;
    }

    void FSMManager::setNewState( u32 id, s32 state, bool changeNow )
    {
        if( id < m_newStates.size() )
        {
            m_newStates[id] = state;
        }

        if( changeNow )
        {
            changeState( id );
        }
        else if( getAutoChangeState( id ) )
        {
            changeState( id );
        }
    }

    void FSMManager::stateOverride( u32 id, s32 state )
    {
        // Directly set all state values without triggering listeners
        setPreviousState( id, getCurrentState( id ) );
        setCurrentState( id, state );

        if( id < m_newStates.size() )
        {
            m_newStates[id] = state;
        }

        // Reset state time since we're forcing a new state
        setStateTime( id, 0 );

        auto applicationManager = core::IApplicationManager::instance();
        if( auto timer = applicationManager->getTimer() )
        {
            auto t = timer->getTime();
            setStateChangeTime( id, t );
        }
    }

    auto FSMManager::isPending( u32 id ) const -> bool
    {
        auto currentState = getCurrentState( id );
        auto newState = getNewState( id );
        return currentState != newState;
    }

    auto FSMManager::isStateChangeComplete( u32 id ) const -> bool
    {
        if( id < m_stateChangeComplete.size() )
        {
            return m_stateChangeComplete[id];
        }

        return false;
    }

    void FSMManager::setStateChangeComplete( u32 id, bool value )
    {
        if( id < m_stateChangeComplete.size() )
        {
            m_stateChangeComplete[id] = value;
        }
    }

    void FSMManager::addListener( u32 id, SmartPtr<IFSMListener> listener )
    {
        m_listeners[id].reserve( 1 );
        m_listeners[id].push_back( listener );
    }

    void FSMManager::removeListener( u32 id, SmartPtr<IFSMListener> listener )
    {
        auto listeners = getListeners( id );

        auto it = std::find( listeners.begin(), listeners.end(), listener );
        if( it != listeners.end() )
        {
            auto listener = *it;
            if( listener )
            {
                listener->unload( nullptr );
            }

            listeners.erase( it );
            setListeners( id, listeners );
        }
    }

    void FSMManager::removeListeners( u32 id )
    {
        auto listeners = getListeners( id );

        for( auto &listener : listeners )
        {
            if( listener )
            {
                listener->unload( nullptr );
            }
        }

        listeners.clear();
        setListeners( id, listeners );
    }

    auto FSMManager::getAutoChangeState( u32 id ) const -> bool
    {
        if( id < m_autoChangeState.size() )
        {
            return m_autoChangeState[id];
        }
        return false;
    }

    void FSMManager::setAutoChangeState( u32 id, bool value )
    {
        if( id < m_autoChangeState.size() )
        {
            m_autoChangeState[id] = value;
        }
    }

    auto FSMManager::getAllowStateChange( u32 id ) const -> bool
    {
        if( id < m_allowStateChange.size() )
        {
            return m_allowStateChange[id];
        }
        return true;  // Default to allowing state changes
    }

    void FSMManager::setAllowStateChange( u32 id, bool value )
    {
        if( id < m_allowStateChange.size() )
        {
            m_allowStateChange[id] = value;
        }
    }

    auto FSMManager::isReady( u32 id ) const -> bool
    {
        WP_ASSERT( id < m_ready.size() );
        return m_ready[id];
    }

    void FSMManager::setReady( u32 id, bool ready )
    {
        WP_ASSERT( id < m_ready.size() );
        m_ready[id] = ready;
    }

    auto FSMManager::getAutoTriggerEnterStateComplete( u32 id ) const -> bool
    {
        if( id < m_autoTriggerEnterStateComplete.size() )
        {
            return m_autoTriggerEnterStateComplete[id];
        }
        return false;
    }

    void FSMManager::setAutoTriggerEnterStateComplete( u32 id, bool value )
    {
        if( id < m_autoTriggerEnterStateComplete.size() )
        {
            m_autoTriggerEnterStateComplete[id] = value;
        }
    }

    auto FSMManager::getStateTicks( u32 id, s32 task ) const -> s32
    {
        // For task-specific ticks, you might want to extend this with a 2D array
        // For now, delegate to the primary tick counter
        return getStateTicks( id );
    }

    auto FSMManager::getStateTicks( u32 id ) const -> s32
    {
        if( id < m_stateTicks.size() )
        {
            return m_stateTicks[id];
        }
        return 0;
    }

    void FSMManager::setStateTicks( u32 id, s32 ticks )
    {
        if( id < m_stateTicks.size() )
        {
            m_stateTicks[id] = ticks;
        }
    }

    auto FSMManager::getListenerPriority( u32 id ) -> u32
    {
        if( id < m_listenerPriority.size() )
        {
            return m_listenerPriority[id];
        }
        return 0;
    }

    void FSMManager::setListenerPriority( u32 id, u32 priority )
    {
        if( id < m_listenerPriority.size() )
        {
            m_listenerPriority[id] = priority;
        }
    }

    auto FSMManager::getFlagsPtr( u32 id ) const -> u32 *
    {
        WP_ASSERT( id < m_flags.size() );
        return const_cast<u32 *>( &m_flags[id] );
    }

    auto FSMManager::getFlags( u32 id ) -> u32
    {
        WP_ASSERT( id < m_flags.size() );
        return m_flags[id];
    }

    void FSMManager::setFlags( u32 id, u32 flags )
    {
        WP_ASSERT( id < m_flags.size() );
        m_flags[id] = flags;
    }

    void FSMManager::setListeners( u32 id, const Array<SmartPtr<IFSMListener>> &listeners )
    {
        if( id < m_listeners.size() )
        {
            m_listeners[id] = { listeners.begin(), listeners.end() };
        }
    }

    auto FSMManager::getListeners( u32 id ) const -> Array<SmartPtr<IFSMListener>>
    {
        if( id < m_listeners.size() )
        {
            return m_listeners[id].snapshot();
        }

        return {};
    }

    auto FSMManager::getStateTime( u32 id ) const -> time_interval
    {
        WP_ASSERT( id < m_stateTimes.size() );
        return m_stateTimes[id];
    }

    void FSMManager::setStateTime( u32 id, time_interval stateTime )
    {
        WP_ASSERT( id < m_stateTimes.size() );
        WP_ASSERT( Math<time_interval>::isFinite( stateTime ) );
        m_stateTimes[id] = stateTime;
    }

    void FSMManager::addStateTime( u32 id, time_interval stateTime )
    {
        if( id < m_stateTimes.size() )
        {
            m_stateTimes[id] += stateTime;
        }
    }

    auto FSMManager::isValid() const -> bool
    {
        auto loadingState = getLoadingState();
        switch( loadingState )
        {
        case LoadingState::Unloaded:
        {
            auto growSize = getGrowSize();
            auto size = getSize();

            if( growSize == 0 && size == 0 )
            {
                return true;
            }
        }
        break;
        case LoadingState::Loaded:
        {
            auto growSize = getGrowSize();
            auto size = getSize();

            if( growSize > 0 && size > 0 )
            {
                return true;
            }
        }
        break;
        default:
        {
        }
        };

        return true;
    }

    auto FSMManager::getSize() const -> size_t
    {
        return m_size;
    }

    void FSMManager::setSize( size_t size )
    {
        m_size = size;
    }

    auto FSMManager::getGrowSize() const -> size_t
    {
        return m_growSize;
    }

    void FSMManager::setGrowSize( size_t growSize )
    {
        m_growSize = growSize;
    }

    auto FSMManager::getPreviousStates() const -> Array<atomic_u8>
    {
        return m_previousStates.snapshot();
    }

    void FSMManager::setPreviousStates( const Array<atomic_u8> &previousStates )
    {
        m_previousStates = { previousStates.begin(), previousStates.end() };
    }

    auto FSMManager::getCurrentStates() const -> Array<atomic_u8>
    {
        return m_currentStates.snapshot();
    }

    void FSMManager::setCurrentStates( const Array<atomic_u8> &currentStates )
    {
        m_currentStates = { currentStates.begin(), currentStates.end() };
    }

    auto FSMManager::getNewStates() const -> Array<atomic_u8>
    {
        return m_newStates.snapshot();
    }

    void FSMManager::setNewStates( const Array<atomic_u8> &newStates )
    {
        m_newStates = { newStates.begin(), newStates.end() };
    }
}  // namespace workphone
