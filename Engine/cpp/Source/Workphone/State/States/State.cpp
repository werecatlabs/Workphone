#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/System/DebugUtil.hpp>

namespace workphone
{
    const String State::timeStr = String( "time" );

    WP_CLASS_REGISTER_DERIVED( workphone, State, IState );

    State::State() = default;

    State::State( u32 poolTypeId ) : IState( poolTypeId )
    {
    }

    State::~State() = default;

    void State::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            m_data = nullptr;
            // State contexts can outlive their owners until StateManager teardown. In that
            // case the weak owner is already dangling, so releasing its bookkeeping would
            // dereference freed memory.
            m_owner.forceReset();
            m_stateContext = nullptr;

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto State::getTime() const -> time_interval
    {
        return m_updateTime;
    }

    void State::setTime( time_interval time )
    {
        m_updateTime = time;
    }

    auto State::isDirty() const -> bool
    {
        return m_dirtyTime > m_updateTime;
    }

    void State::setDirty( bool dirty )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "State::setDirty: failed to get application manager instance." );
            return;
        }

        if( !applicationManager->isRunning() )
        {
            return;
        }

        if( applicationManager->getQuit() )
        {
            return;
        }

        auto timer = applicationManager->getTimerPtr();
        WP_ASSERT( timer );

        if( !timer )
        {
            return;
        }

        if( !timer->isLoaded() )
        {
            return;
        }

        if( dirty )
        {
            m_dirtyTime = timer->now();

            if( auto stateContext = getStateContext() )
            {
                auto stateManager = applicationManager->getStateManagerPtr();
                if( stateManager )
                {
                    auto task = stateContext->getTaskId();
                    stateManager->addDirty( stateContext, task );
                }
            }
        }
        else
        {
            if( isDirty() )
            {
                m_updateTime = timer->now();
            }
        }
    }

    auto State::getStateContext() const -> SmartPtr<IStateContext>
    {
        return m_stateContext;
    }

    void State::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        m_stateContext = stateContext;
    }

    auto State::getProperties() const -> SmartPtr<Properties>
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto properties = factoryManager->make_ptr<Properties>();

        auto time = static_cast<f32>( m_updateTime );
        properties->setProperty( State::timeStr, time );

        return properties;
    }

    void State::setProperties( SmartPtr<Properties> properties )
    {
        auto time = 0.f;
        properties->getPropertyValue( State::timeStr, time );
        m_updateTime = time;
    }

    auto State::getOwner() const -> SmartPtr<ISharedObject>
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void State::setOwner( SmartPtr<ISharedObject> owner )
    {
#if !WP_FINAL
        String debugStr = getDebugStr() + DebugUtil::getStackTrace();
        setDebugStr( debugStr );
#endif

        m_owner = owner;
    }

    auto State::clone() const -> SmartPtr<IState>
    {
        WP_ASSERT( false );  // derived clone not implemented
        auto state = workphone::make_ptr<State>();
        return state;
    }

    void State::assign( SmartPtr<IState> state )
    {
    }

    SmartPtr<ISharedObject> State::getData() const
    {
        return m_data;
    }

    void State::setData( SmartPtr<ISharedObject> data )
    {
        m_data = data;
    }

    void State::makeClone( SmartPtr<State> state ) const
    {
        if( state )
        {
            state->m_owner = m_owner;
            state->m_stateContext = m_stateContext;
            state->m_updateTime = m_updateTime;
        }
    }

    void State::addSendCount()
    {
        ++m_sendCount;
    }

    void State::removeSendCount()
    {
        --m_sendCount;
    }

    u32 State::getSendCount() const
    {
        return m_sendCount;
    }

    void State::setSendCount( u32 sendCount )
    {
        m_sendCount = sendCount;
    }

}  // namespace workphone
