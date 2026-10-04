#ifndef WP_IStateManager_H_
#define WP_IStateManager_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{

    /**
     * @class IStateManager
     * @brief Interface for managing state context and state objects within the system.
     *
     * The IStateManager is responsible for the lifecycle and coordination of state context objects and
     * their associated states. It provides methods for adding, removing, querying, and messaging state
     * contexts and states, as well as managing dirty state updates.
     *
     * @ingroup System
     */
    class WPCore_API IStateManager : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        ~IStateManager() override;

        /**
         * @brief Sends a message to all state objects associated with a specific task.
         *
         * @param taskId The ID of the task whose state objects will receive the message.
         * @param message The message to send to the state objects.
         */
        virtual void sendMessage( TaskId taskId, SmartPtr<IStateMessage> message ) = 0;

        /**
         * @brief Creates and adds a new state context object to the manager.
         *
         * @return A smart pointer to the newly created state context object.
         */
        virtual SmartPtr<IStateContext> addStateContext() = 0;

        /**
         * @brief Removes a state context object from the manager.
         *
         * @param stateContext A smart pointer to the state context object to remove.
         * @return True if the state context was successfully removed, false otherwise.
         */
        virtual bool removeStateContext( SmartPtr<IStateContext> stateContext ) = 0;

        /**
         * @brief Finds a state context object by its unique ID.
         *
         * @param id The unique identifier of the state context to find.
         * @return A smart pointer to the found state context, or null if not found.
         */
        virtual SmartPtr<IStateContext> findStateContext( u32 id ) const = 0;

        /**
         * @brief Retrieves all state context objects managed by this state manager.
         *
         * @return An array of smart pointers to all managed state context objects.
         */
        virtual Array<SmartPtr<IStateContext>> getStateContexts() const = 0;

        /**
         * @brief Gets the state queue associated with a specific task.
         *
         * @param taskId The ID of the task for which to retrieve the state queue.
         * @return A smart pointer to the state queue for the given task.
         */
        virtual SmartPtr<IStateQueue> getQueue( TaskId taskId ) = 0;

        /**
         * @brief Marks a specific state context and its internal states as dirty, indicating they need
         * updating.
         *
         * @param context A smart pointer to the state context to mark as dirty.
         */
        virtual void makeDirty( SmartPtr<IStateContext> context ) = 0;

        /**
         * @brief Marks all state contexts and their internal states as dirty, indicating they need
         * updating.
         */
        virtual void makeAllDirty() = 0;

        /**
         * @brief Adds a state context to the list of dirty contexts that require updating.
         *
         * @param context A smart pointer to the dirty state context to add.
         */
        virtual void addDirty( SmartPtr<IStateContext> context ) = 0;

        /**
         * @brief Adds a state context to the list of dirty contexts for a specific task.
         *
         * @param context A smart pointer to the dirty state context to add.
         * @param task The task to which the dirty state context should be associated.
         */
        virtual void addDirty( SmartPtr<IStateContext> context, TaskId task ) = 0;

        /**
         * @brief Retrieves all state objects of a given type using the type's static typeInfo().
         *
         * @tparam T The type of state objects to retrieve.
         * @return An array of smart pointers to state objects of type T.
         */
        template <class T>
        Array<SmartPtr<T>> getStatesByType();

        /**
         * @brief Retrieves state data of a given type using the type's static typeInfo().
         *
         * @tparam T The type of state data to retrieve.
         * @return A smart pointer to the state data of type T, or null if not found.
         */
        template <class T>
        SmartPtr<T> getStateDataByType() const;

        /**
         * @brief Sets the state data for a given type using the type's static typeInfo().
         *
         * @tparam T The type of state data to set.
         * @param data A smart pointer to the state data of type T to set.
         */
        template <class T>
        void setStateDataByType( SmartPtr<T> data );

        WP_CLASS_REGISTER_DECL;
    };

    /**
     * @brief Retrieves all state objects of a given type using the type's static typeInfo().
     *
     * @tparam T The type of state objects to retrieve.
     * @return An array of smart pointers to state objects of type T.
     */
    template <class T>
    Array<SmartPtr<T>> IStateManager::getStatesByType()
    {
        auto states = getStates( T::typeInfo() );
        return Array<SmartPtr<T>>( states.begin(), states.end() );
    }

    /**
     * @brief Retrieves state data of a given type using the type's static typeInfo().
     *
     * @tparam T The type of state data to retrieve.
     * @return A smart pointer to the state data of type T, or null if not found.
     */
    template <class T>
    SmartPtr<T> IStateManager::getStateDataByType() const
    {
        auto typeInfo = T::typeInfo();

        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );

        auto id = typeManager->getHash( typeInfo );
        auto stateData = getStateDataById( id );
        return workphone::static_pointer_cast<T>( stateData );
    }

    /**
     * @brief Sets the state data for a given type using the type's static typeInfo().
     *
     * @tparam T The type of state data to set.
     * @param data A smart pointer to the state data of type T to set.
     */
    template <class T>
    void IStateManager::setStateDataByType( SmartPtr<T> data )
    {
        auto typeInfo = T::typeInfo();

        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );

        auto id = typeManager->getHash( typeInfo );
        setStateDataById( id, data );
    }

}  // namespace workphone

#endif
