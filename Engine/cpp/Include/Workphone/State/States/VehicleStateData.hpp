#ifndef ModelStateData_h__
#define ModelStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    class WPCore_API VehicleStateData : public StateData
    {
    public:
        VehicleStateData();
        ~VehicleStateData() override;

        Vector3<real_Num> position;
        Quaternion<real_Num> orientation;
        Vector3<real_Num> scale;

        Vector3<real_Num> linearVelocity;
        Vector3<real_Num> angularVelocity;

        Vector3<real_Num> forcePosition;
        Vector3<real_Num> force;
        Vector3<real_Num> torque;

        Vector3<real_Num> derivedLinearVelocity;
        Vector3<real_Num> derivedAngularVelocity;

        s32 isVisible = 0;
        s32 modelType = 0;
        s32 modelState = 0;
        s32 modelSubState = 0;
        s32 isReady = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // SaracenModelData_h__
