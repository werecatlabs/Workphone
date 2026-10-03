#ifndef IAircraftTurbineUnit_h__
#define IAircraftTurbineUnit_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>

namespace workphone
{
    namespace vehicle
    {
        class WPCore_API IAircraftTurbineUnit : public IVehicleComponent
        {
        public:
            ~IAircraftTurbineUnit() override;

            virtual SmartPtr<IBatteryPack> &getBatteryPack() = 0;
            virtual const SmartPtr<IBatteryPack> &getBatteryPack() const = 0;
            virtual void setBatteryPack( SmartPtr<IBatteryPack> batteryPack ) = 0;

            virtual SmartPtr<IESController> &getESC() = 0;
            virtual const SmartPtr<IESController> &getESC() const = 0;
            virtual void setESC( SmartPtr<IESController> escController ) = 0;

            virtual SmartPtr<IAircraftPowerUnit> &getPowerUnit() = 0;
            virtual const SmartPtr<IAircraftPowerUnit> &getPowerUnit() const = 0;
            virtual void setPowerUnit( SmartPtr<IAircraftPowerUnit> powerUnit ) = 0;

            virtual SmartPtr<IAircraftPropeller> &getPropeller() = 0;
            virtual const SmartPtr<IAircraftPropeller> &getPropeller() const = 0;
            virtual void setPropeller( SmartPtr<IAircraftPropeller> propeller ) = 0;

            virtual Vector3<real_Num> getThrust() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  //  IAircraftTurbineUnit_h__
