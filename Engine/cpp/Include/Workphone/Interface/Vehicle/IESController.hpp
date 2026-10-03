#ifndef IESController_h__
#define IESController_h__

#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>

namespace workphone
{
    namespace vehicle
    {
        class WPCore_API IESController : public IVehicleComponent
        {
        public:
            ~IESController() override;

            virtual SmartPtr<IBatteryPack> getBatteryPack() const = 0;
            virtual void setBatteryPack( SmartPtr<IBatteryPack> batteryPack ) = 0;

            virtual SmartPtr<IESController> getEsc() const = 0;
            virtual void setEsc( SmartPtr<IESController> esc ) = 0;

            virtual SmartPtr<IVehiclePowerUnit> getMotor() const = 0;
            virtual void setMotor( SmartPtr<IVehiclePowerUnit> motor ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IESController_h__
