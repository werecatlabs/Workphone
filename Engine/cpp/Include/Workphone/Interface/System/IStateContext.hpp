#ifndef _IStateObject_H_
#define _IStateObject_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/SafePtr.hpp>
#include <Workphone/Memory/SafeReadPtr.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Thread/Thread.hpp>

namespace workphone
{

    /**
     * @class IStateContext
     * @brief Interface for managing an object's internal state, state listeners, and event listeners.
     *
     * This interface provides a context for managing the state of an object, including adding, removing,
     * and querying states, handling state and event listeners, managing messages, and supporting state
     * invalidation and event triggering. It is designed to facilitate state management in a concurrent
     * and extensible manner.
     *
     * @ingroup StateManagement
     */
    class WPCore_API IStateContext : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor for safe cleanup of derived classes.
         */
        ~IStateContext() override;

        /**
         * @brief Set the owner of this context.
         * @param owner SmartPtr to the object that owns this context. May be null.
         */
        virtual void setOwner( SmartPtr<ISharedObject> owner ) = 0;

        /**
         * @brief Retrieve the owner of this context.
         * @return SmartPtr to the owner object or null if none assigned.
         */
        virtual SmartPtr<ISharedObject> getOwner() const = 0;

        /**
         * @brief Retrieve the owner of this context as a raw pointer (no dereference).
         *
         * Returns the raw owner pointer without locking the weak reference or
         * touching the pointed-to object. The returned pointer may be dangling
         * if the owner has already been destroyed.
         *
         * Useful for owner identity comparisons during teardown when the owner
         * may already be gone and constructing a SmartPtr would crash.
         *
         * @return Raw owner pointer, or nullptr if no owner.
         */
        virtual ISharedObject *getOwnerPtr() const = 0;

        /**
         * @brief Query whether this context is marked dirty (requires update).
         * @return true if dirty, false otherwise.
         */
        virtual bool isDirty() const = 0;

        /**
         * @brief Mark or clear the dirty flag for this context.
         * @param dirty true to mark dirty, false to clear.
         * @param cascade When true, propagate the dirty flag to related objects (default: true).
         */
        virtual void setDirty( bool dirty, bool cascade = true ) = 0;

        /**
         * @brief Queue a state message for processing on a specific task.
         * @param taskId Identifier of the task that should process the message.
         * @param message Message to enqueue for later handling.
         */
        virtual void addMessage( TaskId taskId, SmartPtr<IStateMessage> message ) = 0;

        /**
         * @brief Register a state listener that will receive state change notifications.
         * @param stateListener Listener to register.
         */
        virtual void addStateListener( SmartPtr<IStateListener> stateListener ) = 0;

        /**
         * @brief Unregister a previously registered state listener.
         * @param stateListener Listener to remove.
         * @return true if the listener was removed, false if it was not found.
         */
        virtual bool removeStateListener( SmartPtr<IStateListener> stateListener ) = 0;

        /**
         * @brief Get the collection of registered state listeners.
         * @return Array of SmartPtr<IStateListener> representing current listeners.
         */
        virtual Array<SmartPtr<IStateListener>> getStateListeners() const = 0;

        /**
         * @brief Register an event listener for this context.
         * @param eventListener Listener to register.
         */
        virtual void addEventListener( SmartPtr<IEventListener> eventListener ) = 0;

        /**
         * @brief Remove a registered event listener.
         * @param eventListener Listener to remove.
         * @return true if removed, false otherwise.
         */
        virtual bool removeEventListener( SmartPtr<IEventListener> eventListener ) = 0;

        /**
         * @brief Get the list of event listeners currently registered.
         * @return Array of SmartPtr<IEventListener>.
         */
        virtual Array<SmartPtr<IEventListener>> getEventListeners() const = 0;

        /**
         * @brief Add a state object to this context.
         * @param state State object to manage.
         */
        virtual void addState( SmartPtr<IState> state ) = 0;

        /**
         * @brief Remove a state object from this context.
         * @param state State object to remove.
         */
        virtual void removeState( SmartPtr<IState> state ) = 0;

        /**
         * @brief Remove all states that match the provided hash identifier.
         * @param id Identifier to match for removal.
         */
        virtual void removeStatesById( hash_type id ) = 0;

        /**
         * @brief Remove all state objects and clear listener registrations.
         *
         * Implementations should ensure this leaves the context in a clean,
         * default state.
         */
        virtual void clear() = 0;

        /**
         * @brief Find a state by its identifier.
         * @param id Hash identifier of the state to find.
         * @return SmartPtr<IState> pointing to the state or null if not found.
         */
        virtual SmartPtr<IState> getStateById( hash_type id ) const = 0;

        /**
         * @brief Find a state by its identifier and expected type.
         * @param id Hash identifier of the state to find.
         * @param type Expected type identifier for the state.
         * @return SmartPtr<IState> if a matching state exists, null otherwise.
         */
        virtual SmartPtr<IState> getStateById( hash_type id, u32 type ) const = 0;

        /**
         * @brief Find a state by its runtime type identifier.
         * @param typeId Runtime type id to search for.
         * @return SmartPtr<IState> or null if none matches.
         */
        virtual SmartPtr<IState> getStateByTypeId( u32 typeId ) const = 0;

        /**
         * @brief Get a raw pointer to the state data for a given state id and type info.
         * @param id State identifier to search for.
         * @param typeinfo Runtime type information of the expected state data.
         * @return void* Pointer to the state data or null if not found or type mismatch.
         */
        virtual void *getStateDataPtrById( hash_type id, hash_type typeinfo ) const = 0;

        /**
         * @brief Retrieve all states currently managed by this context.
         * @return Array of SmartPtr<IState> containing all state objects.
         */
        virtual Array<SmartPtr<IState>> getStates() const = 0;

        /**
         * @brief Send a state message to all registered state listeners immediately.
         * @param message Message to broadcast to listeners.
         */
        virtual void sendMessage( SmartPtr<IStateMessage> message ) = 0;

        /**
         * @brief Get the task id used to schedule state updates for this context.
         * @return TaskId used for state updates.
         */
        virtual TaskId getTaskId() const = 0;

        /**
         * @brief Set the task id used when scheduling state updates.
         * @param task TaskId to associate with this context's updates.
         */
        virtual void setTaskId( TaskId task ) = 0;

        /**
         * @brief Mark all contained states as invalid/dirty so they will be recomputed.
         */
        virtual void invalidateState() = 0;

        /**
         * @brief Invalidate the first state data found derived from type T and mark it dirty.
         * @tparam T Concrete state data type to search for.
         * @param dirty true to mark the state dirty (default: true).
         * @return SafePtr<T> to the invalidated state data, or null if not found.
         */
        template <class T>
        SafePtr<T> invalidateStateData( bool dirty = true );

        /**
         * @brief Invalidate state data by a specific state id if it matches type T.
         * @tparam T Expected state data type.
         * @param id Identifier of the state to invalidate.
         * @param dirty true to mark the state dirty (default: true).
         * @return SafePtr<T> to the invalidated data or null if not found or type mismatch.
         */
        template <class T>
        SafePtr<T> invalidateStateDataById( hash_type id, bool dirty = true );

        /**
         * @brief Invalidate state data by a runtime type id and return it if it matches T.
         * @tparam T Expected state data type.
         * @param typeId Runtime type id of the state to invalidate.
         * @param dirty true to mark the state dirty (default: true).
         * @return SafePtr<T> to the invalidated data or null if not found or type mismatch.
         */
        template <class T>
        SafePtr<T> invalidateStateDataByTypeId( u32 typeId, bool dirty = true );

        /**
         * @brief Get a read-only pointer to the first state data derived from type T.
         * @tparam T State data type to search for.
         * @return SafeReadPtr<T> pointing to the data or null if not found.
         */
        template <class T>
        SafeReadPtr<T> getStateData() const;

        /**
         * @brief Get a read-only pointer to the state data for a given state id.
         * @tparam T Expected state data type.
         * @param id State identifier to search for.
         * @return SafeReadPtr<T> or null if not found or type mismatch.
         */
        template <class T>
        SafeReadPtr<T> getStateDataById( hash_type id ) const;

        /**
         * @brief Get a read-only pointer to the state data for a given runtime type id.
         * @tparam T Expected state data type.
         * @param typeId Runtime type id to search for.
         * @return SafeReadPtr<T> or null if not found or type mismatch.
         */
        template <class T>
        SafeReadPtr<T> getStateDataByTypeId( u32 typeId ) const;

        /**
         * @brief Find a managed state object by compile-time type T.
         * @tparam T State interface or implementation type to search for.
         * @return SmartPtr<T> cast from the stored IState pointer or null if not found.
         */
        template <class T>
        SmartPtr<T> getStateByType() const;

        /**
         * @brief Trigger an event on this context, invoking event listeners and state handlers.
         * @param eventType Category of event to trigger.
         * @param eventValue Numeric identifier or value for the event.
         * @param arguments Additional arguments to pass to event handlers.
         * @param sender Optional sender object that initiated the event.
         * @param object Optional target object related to the event.
         * @param event Optional event payload object.
         * @return Parameter containing a result or response from the event handling chain.
         */
        virtual Parameter triggerEvent( EventType eventType, hash_type eventValue,
                                        const Array<Parameter> &arguments,
                                        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                        SmartPtr<IEvent> event ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

    template <class T>
    SafePtr<T> IStateContext::invalidateStateData( bool dirty )
    {
        auto states = getStates();
        for( auto &state : states )
        {
            if( auto data = state->getData() )
            {
                if( data->isDerived<T>() )
                {
                    state->setDirty( dirty );
                    return SafePtr<T>( data );
                }
            }
        }

        return nullptr;
    }

    template <class T>
    SafePtr<T> IStateContext::invalidateStateDataById( hash_type id, bool dirty )
    {
        auto states = getStates();
        for( auto &state : states )
        {
            if( state && state->getId() == id )
            {
                if( auto data = state->getData() )
                {
                    if( data->isDerived<T>() )
                    {
                        state->setDirty( dirty );
                        return SafePtr<T>( data );
                    }
                }
            }
        }

        return nullptr;
    }

    template <class T>
    SafePtr<T> IStateContext::invalidateStateDataByTypeId( u32 typeId, bool dirty )
    {
        if( auto state = getStateByTypeId( typeId ) )
        {
            state->setDirty( dirty );
            return state->getData();
        }

        return nullptr;
    }

    template <class T>
    SafeReadPtr<T> IStateContext::getStateData() const
    {
        auto states = getStates();
        for( auto &state : states )
        {
            if( auto data = state->getData() )
            {
                if( data->isDerived<T>() )
                {
                    return SafeReadPtr<T>( data );
                }
            }
        }

        return nullptr;
    }

    template <class T>
    SafeReadPtr<T> IStateContext::getStateDataById( hash_type id ) const
    {
        auto states = getStates();
        for( auto &state : states )
        {
            if( state && state->getId() == id )
            {
                if( auto data = state->getData() )
                {
                    if( data->isDerived<T>() )
                    {
                        return SafeReadPtr<T>( data );
                    }
                }
            }
        }

        return nullptr;
    }

    template <class T>
    SafeReadPtr<T> IStateContext::getStateDataByTypeId( u32 typeId ) const
    {
        if( auto state = getStateByTypeId( typeId ) )
        {
            return state->getData();
        }

        return nullptr;
    }

    template <class T>
    SmartPtr<T> IStateContext::getStateByType() const
    {
        auto typeId = T::typeInfo();
        const auto state = getStateByTypeId( typeId );
        return workphone::reinterpret_pointer_cast<T>( state );
    }

}  // namespace workphone

#endif
