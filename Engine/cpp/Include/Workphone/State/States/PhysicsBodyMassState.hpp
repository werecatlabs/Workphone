#ifndef PhysicsBodyMassState_h__
#define PhysicsBodyMassState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Core/FixedArray.hpp>

namespace workphone
{
    class WPCore_API PhysicsBodyMassState : public StateData
    {
    public:
        PhysicsBodyMassState();
        ~PhysicsBodyMassState() override;

        Transform3<real_Num> massSpaceLocalPose;
        Vector3<real_Num> inertiaTensor;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // PhysicsBodyMassState_h__
