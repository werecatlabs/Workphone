#ifndef IAircraftPropeller_h__
#define IAircraftPropeller_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>

namespace workphone
{
    namespace vehicle
    {

        class WPCore_API IAircraftPropeller : public IVehicleComponent
        {
        public:
            ~IAircraftPropeller() override;

            virtual real_Num getDiameter() const = 0;
            virtual void setDiameter( real_Num diameter ) = 0;

            virtual Vector3<real_Num> getThrust() const = 0;
            virtual void setThrust( const Vector3<real_Num> &thrust ) = 0;

            virtual real_Num getThrustValue() const = 0;
            virtual void setThrustValue( real_Num thrustValue ) = 0;

            virtual real_Num getPropwash() const = 0;
            virtual void setPropwash( real_Num propwash ) = 0;

            virtual Vector3<real_Num> getThrustLine() const = 0;
            virtual void setThrustLine( const Vector3<real_Num> &thrustLine ) = 0;

            virtual real_Num getDownThrust() const = 0;
            virtual void setDownThrust( real_Num downThrust ) = 0;

            virtual real_Num getSideThrust() const = 0;
            virtual void setSideThrust( real_Num sideThrust ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IAircraftPropeller_h__
