#ifndef StateQueueStandard_h__
#define StateQueueStandard_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IStateQueue.hpp>
#include <Workphone/Core/ConcurrentDeque.hpp>

namespace workphone
{

    /** Standard implementation of IStateQueue. */
    class WPCore_API StateQueue : public IStateQueue
    {
    public:
        /** Constructor */
        StateQueue();

        /** Destructor */
        ~StateQueue() override;

        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc IStateQueue::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc IStateQueue::update */
        void update() override;

        /** @copydoc IStateQueue::queueMessage */
        void queueMessage( const SmartPtr<IStateMessage> &message ) override;

        /** @copydoc IStateQueue::clear */
        void clear() override;

        /** @copydoc IStateQueue::getTaskId */
        u32 getTaskId() const override;

        /** @copydoc IStateQueue::setTaskId */
        void setTaskId( u32 taskId ) override;

        /** @copydoc IStateQueue::isEmpty */
        bool isEmpty() const override;

        /** @copydoc IStateQueue::getOwner */
        SmartPtr<IStateContext> getOwner() const;

        /** @copydoc IStateQueue::setOwner */
        void setOwner( SmartPtr<IStateContext> owner );

        /** @copydoc IStateQueue::getMessages */
        Array<SmartPtr<IStateMessage>> getMessages() const override;

        /** @copydoc IStateQueue::getMessagesAndClear */
        Array<SmartPtr<IStateMessage>> getMessagesAndClear() override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// The message queue.
        ConcurrentQueue<SmartPtr<IStateMessage>> m_messageQueue;

        /// The message queue.
        ConcurrentDeque<SmartPtr<IState>> m_stateQueue;

        /// The state object that receives messages.
        AtomicSmartPtr<IStateContext> m_owner;

        /// The id this queue dispatches to.
        atomic_s32 m_taskId;
    };
}  // namespace workphone

#endif  // StateQueueStandard_h__
