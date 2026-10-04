#ifndef IVehicleManager_h__
#define IVehicleManager_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @class IVehicleManager
         * @brief Interface for a vehicle manager, responsible for managing vehicle controllers and their
         * interactions
         */
        class WPCore_API IVehicleManager : public ISharedObject
        {
        public:
            /**
             * @brief Destructor
             */
            ~IVehicleManager() override;

            /**
             * @brief Creates a vehicle controller of the specified type
             * @param type The type of vehicle controller to create
             * @return A smart pointer to the newly created vehicle controller
             */
            virtual SmartPtr<IVehicle> createVehicle( hash64 type ) = 0;

            /**
             * @brief Destroys the specified vehicle controller
             * @param vehicle The vehicle controller to destroy
             */
            virtual void destroyVehicle( SmartPtr<IVehicle> vehicle ) = 0;

            /**
             * @brief Adds a vehicle controller to the manager
             * @param vehicle The vehicle controller to add
             */
            virtual void addVehicle( SmartPtr<IVehicle> vehicle ) = 0;

            /**
             * @brief Removes a vehicle controller from the manager
             * @param vehicle The vehicle controller to remove
             */
            virtual void removeVehicle( SmartPtr<IVehicle> vehicle ) = 0;

            /**
             * @brief Creates a vehicle controller by type
             * @tparam T The type of vehicle controller to create
             * @return A smart pointer to the newly created vehicle controller
             */
            template <class T>
            SmartPtr<T> createVehicleByType();

            WP_CLASS_REGISTER_DECL;
        };

        template <class T>
        SmartPtr<T> IVehicleManager::createVehicleByType()
        {
            auto typeInfo = T::typeInfo();
            WP_ASSERT( typeInfo != 0 );

            auto typeManager = TypeManager::instance();
            WP_ASSERT( typeManager );

            auto typeHash = typeManager->getHash( typeInfo );
            WP_ASSERT( typeHash != 0 );

            auto vehicle = createVehicle( typeHash );
            WP_ASSERT( workphone::dynamic_pointer_cast<T>( vehicle ) );
            return workphone::static_pointer_cast<T>( vehicle );
        }

    }  // namespace vehicle
}  // namespace workphone

#endif  // IVehicleManager_h__
