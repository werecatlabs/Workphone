#ifndef IAircraftPowerUnit_h__
#define IAircraftPowerUnit_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IVehiclePowerUnit.hpp>

namespace workphone
{
    namespace vehicle
    {
        /** Interface for an aircraft attachment class. */
        class WPCore_API IAircraftPowerUnit : public IVehiclePowerUnit
        {
        public:
            ~IAircraftPowerUnit() override;

            virtual IAircraftPropeller *getPropellerPtr() const = 0;
            virtual SmartPtr<IAircraftPropeller> getPropeller() const = 0;
            virtual void setPropeller( SmartPtr<IAircraftPropeller> propeller ) = 0;

            virtual bool isElectric() const = 0;
            virtual void setElectric( bool electric ) = 0;

            virtual real_Num getThrustMultiplier() const = 0;
            virtual void setThrustMultiplier( real_Num thrustMultiplier ) = 0;

            virtual real_Num getTorqueMultiplier() const = 0;
            virtual void setTorqueMultiplier( real_Num torqueMultiplier ) = 0;

            virtual real_Num getPeakPowerW() const = 0;
            virtual void setPeakPowerW( real_Num peakPower ) = 0;

            virtual real_Num getMoi() const = 0;
            virtual void setMoi( real_Num moi ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IAircraftPowerUnit_h__
