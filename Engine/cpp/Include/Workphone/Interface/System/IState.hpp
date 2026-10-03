#ifndef __IState_h__
#define __IState_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @class IState
     * @brief Interface for a state in the state context pattern.
     *
     * This interface defines the contract for state objects that can be managed by a state context.
     * Implementations of this interface can specify custom behavior, manage ownership, and handle
     * associated data. The interface supports cloning, assignment, and dirty-state tracking, and
     * provides access to owner and context objects.
     *
     * @see IStateContext
     * @see ISharedObject
     */
    class WPCore_API IState : public ISharedObject
    {
    public:
        IState();
        IState( u32 poolTypeId );

        /**
         * @brief Virtual destructor for safe polymorphic destruction.
         */
        ~IState() override;

        /**
         * @brief Gets the time associated with this state.
         * @return The time interval representing this state's time.
         */
        virtual time_interval getTime() const = 0;

        /**
         * @brief Sets the time associated with this state.
         * @param time The new time interval for this state.
         */
        virtual void setTime( time_interval time ) = 0;

        /**
         * @brief Checks if the state is marked as dirty (requiring an update).
         * @return True if the state is dirty and needs updating, false otherwise.
         */
        virtual bool isDirty() const = 0;

        /**
         * @brief Sets the dirty flag for this state.
         * @param dirty True to mark the state as dirty (needs update), false to mark as clean.
         */
        virtual void setDirty( bool dirty ) = 0;

        /**
         * @brief Creates a deep copy of this state object.
         * @return A SmartPtr to a new IState instance that is a clone of this one.
         */
        virtual SmartPtr<IState> clone() const = 0;

        /**
         * @brief Assigns the values from another state object to this one.
         * @param state The source state to copy values from.
         */
        virtual void assign( SmartPtr<IState> state ) = 0;

        /**
         * @brief Gets the owner of this state object.
         * @return A pointer to the owner as an ISharedObject.
         */
        ISharedObject *getOwnerPtr() const;

        /**
         * @brief Gets the owner of this state object.
         * @return A SmartPtr to the owner as an ISharedObject.
         */
        virtual SmartPtr<ISharedObject> getOwner() const = 0;

        /**
         * @brief Sets the owner of this state object.
         * @param owner A SmartPtr to the new owner as an ISharedObject.
         */
        virtual void setOwner( SmartPtr<ISharedObject> owner ) = 0;

        /**
         * @brief Gets the state context that owns this state (non-const version).
         *
         * @return A reference to a SmartPtr for the owning IStateContext.
         */
        virtual SmartPtr<IStateContext> getStateContext() const = 0;

        /**
         * @brief Gets the state context that owns this state (const version).
         * @return A pointer for the owning IStateContext.
         */
        IStateContext *getStateContextPtr() const;

        /**
         * @brief Sets the state context that owns this state.
         * @param stateContext A SmartPtr to the new owning IStateContext.
         */
        virtual void setStateContext( SmartPtr<IStateContext> stateContext ) = 0;

        /**
         * @brief Gets the data associated with this state (non-const version).
         * @return A reference to a SmartPtr for the associated ISharedObject data.
         */
        virtual SmartPtr<ISharedObject> getData() const = 0;

        /**
         * @brief Gets the data associated with this state (const version).
         * @return A pointer for the associated ISharedObject data.
         */
        ISharedObject *getDataPtr() const;

        /**
         * @brief Sets the data associated with this state.
         * @param data A SmartPtr to the new associated ISharedObject data.
         */
        virtual void setData( SmartPtr<ISharedObject> data ) = 0;

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

        /**
         * @brief Gets the associated data as a specific type.
         * This template method attempts to cast the associated data to the specified type.
         * @tparam T The type to cast the data to.
         * @return A pointer to the data as type T, or nullptr if the cast fails.
         */
        template <class T>
        T *getDataByType() const;

        // 'c' style linked list of states for message queueing. This is used to avoid dynamic memory
        // allocation when queuing messages.
        AtomicRawPtr<IState> m_next;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// The owner of state.
        AtomicWeakPtr<ISharedObject> m_owner;

        /// The owner state context object.
        AtomicSmartPtr<IStateContext> m_stateContext;

        /// The state data.
        AtomicSmartPtr<ISharedObject> m_data;
    };

    template <class T>
    T *IState::getDataByType() const
    {
        auto p = getData();
        return static_cast<T *>( p.get() );
    }

    WPForceInline IStateContext *IState::getStateContextPtr() const
    {
        return m_stateContext.get();
    }

    WPForceInline ISharedObject *IState::getDataPtr() const
    {
        return m_data.get();
    }

    WPForceInline ISharedObject *IState::getOwnerPtr() const
    {
        return m_owner.get();
    }

}  // namespace workphone

#endif  // IState_h__
