#ifndef PhysicsBodyMotionState_h__
#define PhysicsBodyMotionState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>

namespace workphone
{
    class WPCore_API PhysicsBodyMotionState : public StateData
    {
    public:
        PhysicsBodyMotionState();
        ~PhysicsBodyMotionState() override;

        Vector3<real_Num> linearVelocity = Vector3<real_Num>::zero();
        Vector3<real_Num> angularVelocity = Vector3<real_Num>::zero();
        Vector3<real_Num> force;
        Vector3<real_Num> torque;
        Vector3<real_Num> addedForce;
        Vector3<real_Num> addedTorque;
        u32 flags = 0;
        physics::ForceModeEnum forceMode = physics::ForceModeEnum::Force;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // PhysicsBodyMotionState_h__
