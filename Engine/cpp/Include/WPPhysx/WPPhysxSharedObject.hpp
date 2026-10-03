#ifndef WPPhysxSharedObject_h__
#define WPPhysxSharedObject_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IState.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Template wrapper that adapts a shared object type for use with the PhysX-backed
         *        physics subsystem.
         *
         * This template inherits from the provided shared-object type `T` and forwards loading
         * operations to it while providing thread-safety helpers that integrate with the
         * application's physics manager. Objects of this type assume that the physics manager
         * is responsible for synchronization when accessed from the physics task.
         *
         * @tparam T The concrete shared-object type being wrapped. `T` must provide:
         *           - a `void load(SmartPtr<ISharedObject>)` member,
         *           - a `void unload(SmartPtr<ISharedObject>)` member,
         *           - a static `LoadingState getLoadingState()` accessor,
         *           - and any other interface members expected by consumers.
         */
        template <typename T>
        class PhysxSharedObject : public T
        {
        public:
            /**
             * @brief Default constructor.
             *
             * No special initialization is performed here; construction is delegated to the
             * base type `T`.
             */
            PhysxSharedObject();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper destruction through base pointers. Destruction behavior is
             * provided by the base type `T`.
             */
            virtual ~PhysxSharedObject();

            /**
             * @brief Load object data.
             *
             * Forwards the provided `data` to the underlying type `T`'s `load` implementation.
             *
             * @param data A smart pointer to an `ISharedObject` containing serialized or
             *             runtime data required by the object.
             */
            void load( SmartPtr<ISharedObject> data );

            /**
             * @brief Unload object data.
             *
             * Forwards the provided `data` to the underlying type `T`'s `unload` implementation.
             *
             * @param data A smart pointer to an `ISharedObject` that may be used during unload.
             */
            void unload( SmartPtr<ISharedObject> data );

            /**
             * @brief Query whether the current execution context is safe for direct access.
             *
             * This returns true when:
             *  - the wrapped object's loading state is `LoadingState::Loaded`, and
             *  - the currently executing task is the physics task managed by the application's
             *    physics manager.
             *
             * When this returns true callers may access the object without acquiring the
             * physics manager lock. Otherwise callers should use `lock()`/`unlock()` (or
             * `try_lock()`) to synchronize access.
             *
             * @return true if the current context is the physics task and the object is loaded.
             */
            bool isThreadSafe() const;

            /**
             * @brief Acquire the physics manager lock.
             *
             * Delegates to the application's physics manager. Use this to protect accesses to
             * physics-backed shared objects from non-physics threads.
             */
            void lock();

            /**
             * @brief Try to acquire the physics manager lock without blocking.
             *
             * Delegates to the application's physics manager.
             *
             * @return true if the lock was successfully acquired; false otherwise.
             */
            bool try_lock();

            /**
             * @brief Release the physics manager lock.
             *
             * Delegates to the application's physics manager. Only call `unlock()` if the
             * current thread previously acquired the lock via `lock()` or `try_lock()`.
             */
            void unlock();

            WP_CLASS_REGISTER_TEMPLATE_DECL( PhysxSharedObject, T );
        };

        template <typename T>
        PhysxSharedObject<T>::PhysxSharedObject() = default;

        template <typename T>
        PhysxSharedObject<T>::~PhysxSharedObject() = default;

        template <typename T>
        void PhysxSharedObject<T>::load( SmartPtr<ISharedObject> data )
        {
            T::load( data );
        }

        template <typename T>
        void PhysxSharedObject<T>::unload( SmartPtr<ISharedObject> data )
        {
            T::unload( data );
        }

        template <typename T>
        bool PhysxSharedObject<T>::isThreadSafe() const
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto physicsManager = applicationManager->getPhysicsManagerPtr();

            auto physicsTask = physicsManager->getPhysicsTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = T::getLoadingState();

            return loadingState == LoadingState::Loaded && task == physicsTask;
        }

        template <typename T>
        void PhysxSharedObject<T>::lock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto physicsManager = applicationManager->getPhysicsManagerPtr();
            physicsManager->lock();
        }

        template <typename T>
        bool PhysxSharedObject<T>::try_lock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto physicsManager = applicationManager->getPhysicsManagerPtr();
            return physicsManager->try_lock();
        }

        template <typename T>
        void PhysxSharedObject<T>::unlock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto physicsManager = applicationManager->getPhysicsManagerPtr();
            physicsManager->unlock();
        }

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::physics, PhysxSharedObject, T, T );

    } // namespace physics
} // namespace workphone

#endif // WPPhysxSharedObject_h__
