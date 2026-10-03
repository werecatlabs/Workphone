#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/FSM.hpp>
#include <Workphone/System/FSMManager.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/System/IFSMListener.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, FSM, IFSM );

    FSM::FSM() = default;

    FSM::~FSM() = default;

    void FSM::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fsmManager = getFsmManager();
            WP_ASSERT( fsmManager );

            auto handle = getHandle();
            auto id = handle->getInstanceId();
            m_flags = fsmManager->getFlagsPtr( id );

            *m_flags =
                autoChangeStateFlag | allowStateChangeFlag | isStateChangeCompleteFlag | isReadyFlag;

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FSM::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto fsmManager = getFsmManager();
                if( !fsmManager )
                {
                    WP_LOG_ERROR( "FSMManager is not available during FSM unload." );
                    setLoadingState( LoadingState::Unloaded );
                    return;
                }

                const auto handle = getHandle();
                WP_ASSERT( handle );

                const auto id = handle->getInstanceId();

                fsmManager->removeListeners( id );
                setFsmManager( nullptr );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FSM::update()
    {
        auto autoChangeState = getAutoChangeState();
        if( autoChangeState )
        {
            updateState();
        }
    }

    void FSM::updateState()
    {
        if( auto fsmManager = getFsmManager() )
        {
            const auto handle = getHandle();
            WP_ASSERT( handle );

            const auto id = handle->getInstanceId();

            fsmManager->changeState( id );
        }
    }

    u32 FSM::getFlags() const
    {
        return *m_flags;
    }

    void FSM::setFlags( u32 flags )
    {
        *m_flags = flags;
    }

    auto FSM::getFsmManager() const -> SmartPtr<IFSMManager>
    {
        const auto fsmManager = m_fsmManager.load();
        return fsmManager.lock();
    }

    void FSM::setFsmManager( SmartPtr<IFSMManager> fsmManager )
    {
        m_fsmManager = fsmManager;
    }

    auto FSM::getStateTime() const -> time_interval
    {
        auto fsmManager = getFsmManager();
        WP_ASSERT( fsmManager );

        const auto handle = getHandle();
        WP_ASSERT( handle );

        const auto id = handle->getInstanceId();

        return fsmManager->getStateTime( id );
    }

    void FSM::setStateTime( time_interval stateTime )
    {
        auto fsmManager = getFsmManager();
        WP_ASSERT( fsmManager );

        const auto handle = getHandle();
        WP_ASSERT( handle );

        const auto id = handle->getInstanceId();

        return fsmManager->setStateTime( id, stateTime );
    }

    auto FSM::getStateTimeElapsed() const -> time_interval
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto timer = applicationManager->getTimer();

        return timer->getTime() - getStateTime();
    }

    auto FSM::getPreviousState() const -> u8
    {
        auto fsmManager = getFsmManager();
        WP_ASSERT( fsmManager );

        const auto handle = getHandle();
        WP_ASSERT( handle );

        const auto id = handle->getInstanceId();

        return fsmManager->getPreviousState( id );
    }

    auto FSM::getCurrentState() const -> u8
    {
        const auto fsmManager = getFsmManager();
        if( fsmManager )
        {
            const auto handle = getHandle();
            WP_ASSERT( handle );

            const auto id = handle->getInstanceId();
            return fsmManager->getCurrentState( id );
        }

        return 0;
    }

    auto FSM::getNewState() const -> u8
    {
        if( const auto fsmManager = getFsmManager() )
        {
            const auto handle = getHandle();
            WP_ASSERT( handle );

            const auto id = handle->getInstanceId();
            return fsmManager->getNewState( id );
        }

        return 0;
    }

    void FSM::setNewState( s32 value, bool changeNow /*= false*/ )
    {
        if( auto fsmManager = getFsmManager() )
        {
            const auto handle = getHandle();
            WP_ASSERT( handle );

            const auto id = handle->getInstanceId();
            fsmManager->setNewState( id, value, changeNow );

            if( !changeNow )
            {
                fsmManager->queueDirtyFSM( this );
            }
        }
    }

    void FSM::stateOverride( s32 state )
    {
    }

    auto FSM::isPending() const -> bool
    {
        return BitUtil::getFlagValue( *m_flags, IFSM::isPendingFlag );
    }

    void FSM::triggerStateChangeComplete()
    {
        BitUtil::setFlagValue( *m_flags, IFSM::isPendingFlag, false );
        BitUtil::setFlagValue( *m_flags, IFSM::isStateChangeCompleteFlag, true );

        auto listeners = getListeners();
        for( auto listener : listeners )
        {
            try
            {
                if( listener )
                {
                    const auto state = getCurrentState();
                    const auto result = listener->handleEvent( state, FSMEvent::Complete );
                    if( result == FSMReturnType::Ok )
                    {
                        auto stateStr = StringUtil::toString( state );
                        WP_LOG( "State change complete ok: " + stateStr );
                    }
                    else
                    {
                        auto stateStr = StringUtil::toString( state );
                        WP_LOG( "State change complete not ok: " + stateStr );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
    }

    auto FSM::isStateChangeComplete() const -> bool
    {
        return BitUtil::getFlagValue( *m_flags, IFSM::isStateChangeCompleteFlag );
    }

    void FSM::setStateChangeComplete( bool stateChangeComplete )
    {
        BitUtil::setFlagValue( *m_flags, IFSM::isStateChangeCompleteFlag, stateChangeComplete );
    }

    void FSM::addListener( SmartPtr<IFSMListener> listener )
    {
        if( listener )
        {
            listener->setFSM( this );
        }

        auto fsmManager = getFsmManager();
        WP_ASSERT( fsmManager );

        auto handle = getHandle();
        WP_ASSERT( handle );

        auto id = handle->getInstanceId();
        WP_ASSERT( id < std::numeric_limits<u32>::max() );

        fsmManager->addListener( id, listener );
    }

    void FSM::removeListener( SmartPtr<IFSMListener> listener )
    {
        auto fsmManager = getFsmManager();
        WP_ASSERT( fsmManager );

        auto handle = getHandle();
        WP_ASSERT( handle );

        if( handle )
        {
            auto id = handle->getInstanceId();
            WP_ASSERT( id < std::numeric_limits<u32>::max() );

            fsmManager->removeListener( id, listener );
        }
    }

    Array<SmartPtr<IFSMListener>> FSM::getListeners() const
    {
        auto fsmManager = (FSMManager *)getFsmManagerPtr();
        WP_ASSERT( fsmManager );
        auto handle = getHandle();
        WP_ASSERT( handle );
        auto id = handle->getInstanceId();

        return fsmManager->getListeners( id );
    }

    auto FSM::getAutoChangeState() const -> bool
    {
        return BitUtil::getFlagValue( *m_flags, IFSM::autoChangeStateFlag );
    }

    void FSM::setAutoChangeState( bool autoChangeState )
    {
        BitUtil::setFlagValue( *m_flags, IFSM::autoChangeStateFlag, autoChangeState );
    }

    auto FSM::getAllowStateChange() const -> bool
    {
        return BitUtil::getFlagValue( *m_flags, IFSM::allowStateChangeFlag );
    }

    void FSM::setAllowStateChange( bool allowStateChange )
    {
        BitUtil::setFlagValue( *m_flags, IFSM::allowStateChangeFlag, allowStateChange );
    }

    auto FSM::getAutoTriggerEnterStateComplete() const -> bool
    {
        return false;
    }

    void FSM::setAutoTriggerEnterStateComplete( bool autoTriggerEnterStateComplete )
    {
    }

    auto FSM::getStateTicks( TaskId task ) const -> s32
    {
        return m_stateTicks;
    }

    auto FSM::getStateTicks() const -> s32
    {
        return m_stateTicks;
    }

    void FSM::setStateTicks( s32 ticks )
    {
        m_stateTicks = ticks;
    }

    s32 FSM::getPriority() const
    {
        return m_eventListenerPriority;
    }

    void FSM::setPriority( s32 priority )
    {
        m_eventListenerPriority = priority;
    }

}  // namespace workphone
