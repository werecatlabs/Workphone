#ifndef StateMessageBase_h__
#define StateMessageBase_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>

namespace workphone
{

    /**
     * @file StateMessage.hpp
     * @brief Concrete implementation of the IStateMessage interface.
     *
     * StateMessage is the default message object used by the engine's state
     * system to carry typed payloads between state contexts and listeners.
     * Each instance stores a hash-based type identifier, an optional sender
     * reference and an optional state context reference. A send counter is
     * provided to track how many times the message has been queued or
     * dispatched.
     *
     * Thread-safety:
     * - The sender and state context are stored in atomic weak pointers
     *   (AtomicSmartPtr). Accessors return either a raw non-owning pointer
     *   or a SmartPtr that may be null if the target object was destroyed.
     * - The send counter is an atomic_u32 and may be safely incremented or
     *   decremented from multiple threads.
     */
    class WPCore_API StateMessage : public IStateMessage
    {
    public:
        /**
         * @brief Message type indicating an object should be set or replaced.
         *
         * Receivers should treat the message payload as a request to set or
         * replace an object reference. The concrete hash value is defined in
         * the corresponding implementation file.
         */
        static const hash_type SET_OBJECT;

        /**
         * @brief Message type indicating a mesh resource should be set.
         *
         * Payload or associated data should reference mesh content or a
         * handle to mesh resources.
         */
        static const hash_type SET_MESH;

        /**
         * @brief Message type indicating a cubemap resource should be set.
         */
        static const hash_type SET_CUBEMAP;

        /**
         * @brief Message type indicating textures or material resources should
         * be set.
         */
        static const hash_type SET_TEXTURES;

        /**
         * @brief Default constructs an empty message.
         *
         * The message is initialized with type 0 and no sender or state
         * context attached.
         */
        StateMessage();

        /**
         * @brief Virtual destructor.
         *
         * Releases any internal resources. External object ownership follows
         * SmartPtr/AtomicSmartPtr semantics and is not assumed by this class.
         */
        ~StateMessage() override;

        /**
         * @brief Unload or clear a payload object associated with the message.
         *
         * Concrete implementations may use this to explicitly release or
         * clear payloads referenced by the message (for example decrementing
         * reference counts or resetting internal pointers).
         *
         * @param data Smart pointer to the object to be unloaded/cleared.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Get the message type identifier.
         *
         * @return The hash_type value that identifies this message's type.
         */
        hash_type getType() const override;

        /**
         * @brief Set the message type identifier.
         *
         * @param type Hash value representing the message type.
         */
        void setType( hash_type type ) override;

        /**
         * @brief Get a raw, non-owning pointer to the sender object.
         *
         * This returns the raw pointer held by the internal AtomicSmartPtr
         * and does not extend the sender's lifetime. The pointer may be
         * nullptr if the sender has already been destroyed.
         *
         * @return Raw ISharedObject pointer or nullptr.
         */
        ISharedObject *getSenderPtr() const override;

        /**
         * @brief Get an owning SmartPtr to the sender object.
         *
         * Attempts to obtain a strong SmartPtr from the internal
         * AtomicSmartPtr. The returned SmartPtr may be null if the sender
         * no longer exists.
         *
         * @return SmartPtr<ISharedObject> strong reference or null.
         */
        SmartPtr<ISharedObject> getSender() const override;

        /**
         * @brief Set the sender for this message.
         *
         * Only a weak/atomic reference is stored internally. Passing a null
         * SmartPtr clears the sender reference.
         *
         * @param object SmartPtr<ISharedObject> sender object (strong ptr).
         */
        void setSender( SmartPtr<ISharedObject> object ) override;

        /**
         * @brief Get a raw, non-owning pointer to the associated state
         * context.
         *
         * The returned pointer does not extend the context's lifetime and may
         * be nullptr if the context was destroyed.
         *
         * @return Raw IStateContext pointer or nullptr.
         */
        IStateContext *getStateContextPtr() const override;

        /**
         * @brief Get an owning SmartPtr to the associated state context.
         *
         * Attempts to obtain a strong SmartPtr from the internal
         * AtomicSmartPtr. The returned SmartPtr may be null if the context
         * no longer exists.
         *
         * @return SmartPtr<IStateContext> strong reference or null.
         */
        SmartPtr<IStateContext> getStateContext() const override;

        /**
         * @brief Set the state context associated with this message.
         *
         * Only a weak/atomic reference is stored internally. Passing a null
         * SmartPtr clears the context reference.
         *
         * @param object SmartPtr<IStateContext> state context (strong ptr).
         */
        void setStateContext( SmartPtr<IStateContext> object ) override;

        /**
         * @brief Atomically increment the send counter.
         *
         * Indicates the message has been queued or dispatched one additional
         * time.
         */
        void addSendCount() override;

        /**
         * @brief Atomically decrement the send counter.
         *
         * Indicates one fewer active/queued send. Behavior when the counter
         * is already zero is implementation-defined.
         */
        void removeSendCount() override;

        /**
         * @brief Retrieve the current send counter value.
         *
         * @return Number of times this message has been sent/queued.
         */
        u32 getSendCount() const override;

        /**
         * @brief Explicitly set the send counter value.
         *
         * @param sendCount New value for the send counter.
         */
        void setSendCount( u32 sendCount ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Message type identifier (hash).
         *
         * Default is 0 (no type). Use setType() and getType() to modify and
         * query this value.
         */
        hash_type m_type = 0;

        /**
         * @brief Atomic counter tracking how many times this message has been
         * queued or dispatched.
         */
        atomic_u32 m_sendCount = 0;

        /**
         * @brief Weak/atomic reference to the message sender.
         *
         * Stored as an AtomicSmartPtr to avoid unintentionally extending the
         * sender's lifetime. Call getSender() to obtain a strong SmartPtr if
         * an owning reference is required.
         */
        AtomicSmartPtr<ISharedObject> m_sender;

        /**
         * @brief Weak/atomic reference to the state context that produced or
         *        should handle the message.
         *
         * Use getStateContext() to obtain a strong SmartPtr if a persistent
         * reference is needed.
         */
        AtomicSmartPtr<IStateContext> m_stateContext;
    };

    inline ISharedObject *StateMessage::getSenderPtr() const
    {
        return m_sender.get();
    }

    inline IStateContext *StateMessage::getStateContextPtr() const
    {
        return m_stateContext.get();
    }

}  // namespace workphone

#endif  // StateMessageBase_h__
