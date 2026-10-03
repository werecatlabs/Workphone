#ifndef __VehicleManager_h__
#define __VehicleManager_h__

#include <Workphone/Interface/Vehicle/IVehicleManager.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @brief Concrete implementation of the IVehicleManager interface.
         *
         * VehicleManager is responsible for the lifetime and update cycle of all
         * vehicle instances created and managed by the system. It provides methods
         * to create, destroy, add and remove vehicles as well as the usual update
         * hooks used by the engine update loop.
         *
         * Thread-safety:
         * The manager stores vehicles in a thread-safe collection. Access to the
         * internal collection can also be explicitly guarded using the lock,
         * try_lock and unlock methods when performing compound operations.
         */
        class WPCore_API VehicleManager : public IVehicleManager
        {
        public:
            /**
             * @brief Construct a new VehicleManager.
             *
             * Initializes internal structures used to store and manage vehicles.
             */
            VehicleManager();

            /**
             * @brief Destroy the VehicleManager.
             *
             * Cleans up managed vehicles and releases any resources held by the manager.
             */
            ~VehicleManager() override;

            /**
             * @brief Load manager state or resources from the provided data object.
             *
             * @param data Shared object carrying configuration or serialized state.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Reload manager state or resources using the provided data object.
             *
             * Used to refresh configuration or re-initialize resources while the
             * application is running.
             *
             * @param data Shared object carrying configuration or serialized state.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload manager resources and detach from external systems.
             *
             * After unload, the manager should be in a clean state suitable for
             * destruction or re-initialization.
             *
             * @param data Optional shared object containing context for unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Called before the main update loop to perform preparation work.
             *
             * Implements IVehicleManager::preUpdate. Typical tasks:
             * - prepare vehicle state for the upcoming frame,
             * - process pending additions/removals that must happen before updates.
             */
            void preUpdate() override;

            /**
             * @brief Main per-frame update for all managed vehicles.
             *
             * Implements IVehicleManager::update. This should iterate the vehicle
             * collection and advance vehicle simulation/logic for the current frame.
             */
            void update() override;

            /**
             * @brief Called after the main update loop to perform finalization work.
             *
             * Implements IVehicleManager::postUpdate. Typical tasks:
             * - apply deferred state changes,
             * - cleanup completed operations.
             */
            void postUpdate() override;

            /**
             * @brief Create a new vehicle instance of the specified type.
             *
             * The created vehicle is owned by a SmartPtr and will be managed by the
             * VehicleManager (unless explicitly removed).
             *
             * @param type Hash identifier specifying the vehicle type to create.
             * @return SmartPtr<IVehicle> Smart pointer to the newly created vehicle.
             */
            SmartPtr<IVehicle> createVehicle( hash64 type ) override;

            /**
             * @brief Destroy a vehicle previously created or managed by this manager.
             *
             * This will remove the vehicle from the internal collection (if present)
             * and release any resources owned by it. Passing a null SmartPtr is a
             * no-op.
             *
             * @param vehicle Smart pointer to the vehicle to destroy.
             */
            void destroyVehicle( SmartPtr<IVehicle> vehicle ) override;

            /**
             * @brief Add an externally created vehicle to this manager.
             *
             * If the vehicle is already present, this call has no effect. The manager
             * takes shared ownership of the vehicle via the provided SmartPtr.
             *
             * @param vehicle Smart pointer to the vehicle to add.
             */
            void addVehicle( SmartPtr<IVehicle> vehicle ) override;

            /**
             * @brief Remove a vehicle from this manager without destroying it.
             *
             * After removal the caller retains ownership via their SmartPtr. If the
             * vehicle is not present this call has no effect.
             *
             * @param vehicle Smart pointer to the vehicle to remove.
             */
            void removeVehicle( SmartPtr<IVehicle> vehicle ) override;

            /**
             * @brief Acquire the internal mutex protecting the vehicle collection.
             *
             * Use this when performing a sequence of operations that must be atomic
             * with respect to other threads accessing the manager.
             */
            void lock() override;

            /**
             * @brief Try to acquire the internal mutex without blocking.
             *
             * @return true if the lock was acquired, false otherwise.
             */
            bool try_lock() override;

            /**
             * @brief Release the internal mutex previously acquired with lock().
             */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Thread-safe dynamic array holding shared pointers to all managed vehicles. */
            ConcurrentArray<SmartPtr<IVehicle>> m_vehicles;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // VehicleManager_h__
