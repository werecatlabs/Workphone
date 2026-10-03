#ifndef IVehicleAi_h__
#define IVehicleAi_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    /**
     * @class IVehicleAi
     * @brief Interface for vehicle artificial intelligence.
     *
     * This interface defines the core functionality for managing AI logic
     * associated with a vehicle, including its controller and the overall AI manager.
     */
    class WPCore_API IVehicleAi : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor for IVehicleAi.
         */
        ~IVehicleAi() override;

        /**
         * @brief Gets the vehicle controller associated with this AI.
         * @return A smart pointer to the vehicle controller.
         */
        virtual SmartPtr<vehicle::IVehicle> getVehicleController() const = 0;

        /**
         * @brief Sets the vehicle controller associated with this AI.
         * @param controller A smart pointer to the vehicle controller.
         */
        virtual void setVehicleController( SmartPtr<vehicle::IVehicle> controller ) = 0;

        /**
         * @brief Gets the vehicle AI manager associated with this AI.
         * @return A smart pointer to the vehicle AI manager.
         */
        virtual SmartPtr<IVehicleAiManager> getVehicleManager() const = 0;

        /**
         * @brief Sets the vehicle AI manager associated with this AI.
         * @param manager A smart pointer to the vehicle AI manager.
         */
        virtual void setVehicleManager( SmartPtr<IVehicleAiManager> manager ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IVehicleAi_h__
