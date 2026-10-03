#ifndef IStateQueue_h__
#define IStateQueue_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>

namespace workphone
{

    /**
     * @brief Interface for a thread-safe queue that stores and manages state messages.
     *
     * This class provides an interface for queuing, retrieving, and managing state messages
     * in a concurrent environment. It allows for associating a task ID with the queue,
     * clearing the queue, and retrieving messages in bulk, with or without clearing the queue.
     *
     * @note Implementations must ensure thread safety for all operations.
     */
    class WPCore_API IStateQueue : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor for safe polymorphic destruction.
         */
        ~IStateQueue() override;

        /**
         * @brief Queues a state message for later processing.
         *
         * @param message The message to be added to the queue. Must not be null.
         *
         * @note Thread-safe. The message will be processed by the task associated with this queue.
         */
        virtual void queueMessage( const SmartPtr<IStateMessage> &message ) = 0;

        /**
         * @brief Removes all messages from the queue.
         *
         * After calling this method, the queue will be empty.
         *
         * @note Thread-safe.
         */
        virtual void clear() = 0;

        /**
         * @brief Gets the task ID responsible for processing the queued messages.
         *
         * @return The task ID associated with this queue.
         */
        virtual u32 getTaskId() const = 0;

        /**
         * @brief Sets the task ID responsible for processing the queued messages.
         *
         * @param taskId The new task ID to associate with this queue.
         */
        virtual void setTaskId( u32 taskId ) = 0;

        /**
         * @brief Checks if the message queue is currently empty.
         *
         * @return True if the queue contains no messages, false otherwise.
         */
        virtual bool isEmpty() const = 0;

        /**
         * @brief Retrieves all messages currently in the queue without clearing them.
         *
         * @return A shared pointer to a concurrent array containing all queued messages.
         *         The returned array reflects the state of the queue at the time of the call.
         *
         * @note The queue remains unchanged after this call.
         */
        virtual Array<SmartPtr<IStateMessage>> getMessages() const = 0;

        /**
         * @brief Retrieves all messages currently in the queue and clears the queue.
         *
         * @return A shared pointer to a concurrent array containing all queued messages.
         *         After this call, the queue will be empty.
         */
        virtual Array<SmartPtr<IStateMessage>> getMessagesAndClear() = 0;

        // 'c' style linked list of states for message queueing. This is used to avoid dynamic memory
        // allocation when queuing messages.
        AtomicRawPtr<IStateQueue> m_next;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IStateQueue_h__
