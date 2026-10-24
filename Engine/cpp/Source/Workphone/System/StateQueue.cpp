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
        { ScopedLock lock(&m_messageMutex); m_acceptMessages = true; }
        setLoadingState( LoadingState::Loaded );
    }

    void StateQueue::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            { ScopedLock lock(&m_messageMutex); m_acceptMessages = false; }
            auto messageQueue = getMessagesAndClear();
            // setLoadingState commits the state before publishing a new event.
            // Denying that optional publication must not strand accepted work.
            try { setLoadingState( LoadingState::Unloading ); }
            catch(const std::bad_alloc &) {}
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

            try { setLoadingState( LoadingState::Unloaded ); }
            catch(const std::bad_alloc &) {}
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

        ScopedLock lock(&m_messageMutex);
        if(m_acceptMessages) m_messageQueue.push_back( message );
    }

    void StateQueue::clear()
    {
        auto messages = getMessagesAndClear();
        for(auto &message : messages)
            if(message) message->unload(nullptr);
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
        ScopedLock lock(&m_messageMutex);
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
        ScopedLock lock(&m_messageMutex);
        return m_messageQueue;
    }

    auto StateQueue::getMessagesAndClear() -> Array<SmartPtr<IStateMessage>>
    {
        auto messages = makeSourceBudgetSnapshot<SmartPtr<IStateMessage>>();
        { ScopedLock lock(&m_messageMutex); messages.swap(m_messageQueue); }
        return messages;
    }

}  // namespace workphone
