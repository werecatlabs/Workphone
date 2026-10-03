#ifndef RigidbodyState_h__
#define RigidbodyState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Core/FixedArray.hpp>

namespace workphone
{
    class WPCore_API RigidbodyState : public StateData
    {
    public:
        RigidbodyState();
        ~RigidbodyState() override;

        Transform3<real_Num> kinematicTarget;

        Vector3<real_Num> position;
        Quaternion<real_Num> orientation;

        Vector3<real_Num> linearVelocity;
        Vector3<real_Num> angularVelocity;

        u32 rigidBodyFlags = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // RigidbodyState_h__
