#ifndef _IStateMessage_H_
#define _IStateMessage_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief Interface representing a state message used by the state system.
     *
     * IStateMessage encapsulates information that is passed between state contexts
     * and listeners. Implementations store a message type (a hash value), an
     * optional sender object, and an associated state context. The interface
     * also exposes a send counter which can be used to track how many times the
     * message has been queued or dispatched.
     */
    class WPCore_API IStateMessage : public ISharedObject
    {
    public:
        /// Key for the left coordinate/value in messages that carry layout data.
        static const hash_type STATE_MESSAGE_LEFT;
        /// Key for the top coordinate/value in messages that carry layout data.
        static const hash_type STATE_MESSAGE_TOP;
        /// Key for the width value in messages that carry layout data.
        static const hash_type STATE_MESSAGE_WIDTH;
        /// Key for the height value in messages that carry layout data.
        static const hash_type STATE_MESSAGE_HEIGHT;
        /// Key indicating the metrics mode (for example pixels vs. relative units).
        static const hash_type STATE_MESSAGE_METRICSMODE;
        /// Key for horizontal alignment information.
        static const hash_type STATE_MESSAGE_ALIGN_HORIZONTAL;
        /// Key for vertical alignment information.
        static const hash_type STATE_MESSAGE_ALIGN_VERTICAL;
        /// Key for textual content carried by the message.
        static const hash_type STATE_MESSAGE_TEXT;

        /**
         * @brief Virtual destructor.
         *
         * Ensures derived implementations are destroyed correctly when referenced
         * through an IStateMessage pointer.
         */
        ~IStateMessage() override;

        /**
         * @brief Get the message type identifier.
         *
         * The type is represented as a hash_type and is used to distinguish
         * different message kinds. Callers can use this value to cast or
         * interpret message payloads appropriately.
         *
         * @return The hash-based type identifier for this message.
         */
        virtual hash_type getType() const = 0;

        /**
         * @brief Set the message type identifier.
         *
         * @param type Hash-based identifier representing the message type.
         */
        virtual void setType( hash_type type ) = 0;

        /**
         * @brief Get a raw pointer to the sender object.
         *
         * Returns a non-owning raw pointer to the object that created or
         * dispatched this message. The lifetime of the returned pointer is
         * managed externally; callers should not delete it.
         *
         * @return Raw pointer to the sender, or nullptr if none is set.
         */
        virtual ISharedObject *getSenderPtr() const = 0;

        /**
         * @brief Get a smart pointer to the sender object.
         *
         * Returns an owning SmartPtr to the sender so callers can extend the
         * sender's lifetime safely while they inspect or use it.
         *
         * @return SmartPtr wrapping the sender object, may be null.
         */
        virtual SmartPtr<ISharedObject> getSender() const = 0;

        /**
         * @brief Set the sender of this message.
         *
         * @param object SmartPtr to the sender object. Passing a null
         * SmartPtr clears the sender.
         */
        virtual void setSender( SmartPtr<ISharedObject> object ) = 0;

        /**
         * @brief Get a raw pointer to the associated state context.
         *
         * The returned pointer is non-owning and its lifetime is managed by
         * the context owner. Use getStateContext() to obtain an owning
         * SmartPtr if you need to extend the lifetime.
         *
         * @return Raw pointer to the IStateContext, or nullptr if none set.
         */
        virtual IStateContext *getStateContextPtr() const = 0;

        /**
         * @brief Get an owning SmartPtr to the associated state context.
         *
         * @return SmartPtr wrapping the IStateContext, may be null.
         */
        virtual SmartPtr<IStateContext> getStateContext() const = 0;

        /**
         * @brief Set the associated state context for this message.
         *
         * @param object SmartPtr to the state context. Passing a null
         * SmartPtr clears the association.
         */
        virtual void setStateContext( SmartPtr<IStateContext> object ) = 0;

        /**
         * @brief Increment the internal send counter.
         *
         * Implementations should atomically increment any internal counter
         * that tracks how many times this message has been queued or sent.
         */
        virtual void addSendCount() = 0;

        /**
         * @brief Decrement the internal send counter.
         *
         * Implementations should atomically decrement the counter. Behavior
         * when the counter is already zero is implementation-defined.
         */
        virtual void removeSendCount() = 0;

        /**
         * @brief Get the current send counter value.
         *
         * @return The number of times this message has been sent/queued.
         */
        virtual u32 getSendCount() const = 0;

        /**
         * @brief Explicitly set the send counter value.
         *
         * @param sendCount New value for the send counter.
         */
        virtual void setSendCount( u32 sendCount ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif
