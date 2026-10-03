#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/StateQueue.hpp>
#include <Workphone/System/DebugUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateQueue, IStateQueue );

    StateQueue::StateQueue()
    {
#if WP_ENABLE_MEMORY_TRACKER
#    if WP_ARCH_TYPE == WP_ARCHITECTURE_64
#        if WP_ENABLE_TRACE
        const auto stack = DebugUtil::getStackTrace();
        setDebugStr( stack );
#        endif
#    endif
#endif
    }

    StateQueue::~StateQueue() = default;

    void StateQueue::load( SmartPtr<ISharedObject> data )
    {
        (void)data;
        setLoadingState( LoadingState::Loading );
        setLoadingState( LoadingState::Loaded );
    }

    void StateQueue::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto messageQueue = getMessagesAndClear();
            for( auto &message : messageQueue )
            {
                if( message )
                {
                    message->unload( nullptr );
                }
                else
                {
                    WP_LOG_WARNING( "StateQueue::unload: null message in queue; skipping." );
                }
            }

            while( !m_messageQueue.empty() )
            {
                SmartPtr<IStateMessage> message;
                if( m_messageQueue.try_pop( message ) )
                {
                    if( message )
                    {
                        message->unload( nullptr );
                    }
                    else
                    {
                        WP_LOG_WARNING( "StateQueue::clear: null message popped from queue; skipping." );
                    }
                }
            }

            m_messageQueue.clear();

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void StateQueue::update()
    {
        try
        {
            auto owner = m_owner.load();
            if( !owner )
            {
                WP_LOG_WARNING( "StateQueue::update: no owner set; cannot dispatch messages." );
                return;
            }

            auto messageQueue = getMessagesAndClear();
            for( auto &message : messageQueue )
            {
                if( !message )
                {
                    WP_LOG_WARNING( "StateQueue::update: null message in queue; skipping." );
                    continue;
                }

                try
                {
                    owner->sendMessage( message );
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void StateQueue::queueMessage( const SmartPtr<IStateMessage> &message )
    {
        if( !isLoaded() )
        {
            return;
        }

        if( !message )
        {
            WP_LOG_WARNING( "StateQueue::queueMessage: null message supplied; ignoring." );
            return;
        }

        m_messageQueue.push( message );
    }

    void StateQueue::clear()
    {
        while( !m_messageQueue.empty() )
        {
            SmartPtr<IStateMessage> message;
            if( m_messageQueue.try_pop( message ) )
            {
                if( message )
                {
                    message->unload( nullptr );
                }
                else
                {
                    WP_LOG_WARNING( "StateQueue::clear: null message popped from queue; skipping." );
                }
            }
        }

        m_messageQueue.clear();
    }

    auto StateQueue::getTaskId() const -> u32
    {
        return m_taskId;
    }

    void StateQueue::setTaskId( u32 taskId )
    {
        m_taskId = taskId;
    }

    auto StateQueue::isEmpty() const -> bool
    {
        return m_messageQueue.empty();
    }

    auto StateQueue::getOwner() const -> SmartPtr<IStateContext>
    {
        return m_owner;
    }

    void StateQueue::setOwner( SmartPtr<IStateContext> owner )
    {
        m_owner = owner;
    }

    auto StateQueue::getMessages() const -> Array<SmartPtr<IStateMessage>>
    {
        return {};
    }

    auto StateQueue::getMessagesAndClear() -> Array<SmartPtr<IStateMessage>>
    {
        if( !m_messageQueue.empty() )
        {
            auto messages = Array<SmartPtr<IStateMessage>>();
            messages.reserve( 128 );

            SmartPtr<IStateMessage> message;
            while( m_messageQueue.try_pop( message ) )
            {
                if( message )
                {
                    messages.push_back( message );
                }
                else
                {
                    WP_LOG_WARNING(
                        "StateQueue::getMessagesAndClear: null message popped from queue; "
                        "skipping." );
                }
            }

            return messages;
        }

        return {};
    }

}  // namespace workphone
