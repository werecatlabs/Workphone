#ifndef PhysicsBodyState_h__
#define PhysicsBodyState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Core/FixedArray.hpp>

namespace workphone
{

    class WPCore_API MotionState : public StateData
    {
    public:
        MotionState &operator=( const MotionState &other )
        {
            linearVelocity = other.linearVelocity;
            angularVelocity = other.angularVelocity;
            return *this;
        }

        Vector3<real_Num> linearVelocity;
        Vector3<real_Num> angularVelocity;
    };

    class WPCore_API PhysicsBodyState : public StateData
    {
    public:
        PhysicsBodyState();
        ~PhysicsBodyState() override;

        Transform3<real_Num> transform;

        Vector3<real_Num> position;
        Quaternion<real_Num> orientation;

        Vector3<real_Num> linearVelocity;
        Vector3<real_Num> angularVelocity;

        real_Num mass = static_cast<real_Num>( 1.0 );

        u32 collisionType = 0;
        u32 collisionMask = 0;

        u32 flags = 0;
        u32 actorFlags = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // RigidbodyState_h__
