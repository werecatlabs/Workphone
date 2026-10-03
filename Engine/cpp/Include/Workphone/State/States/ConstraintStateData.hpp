#ifndef ConstraintStateData_h__
#define ConstraintStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{

    class WPCore_API ConstraintStateData : public StateData
    {
    public:
        ConstraintStateData();
        ~ConstraintStateData() override;

        AtomicWeakPtr<physics::IPhysicsBody3> bodyA;
        AtomicWeakPtr<physics::IPhysicsBody3> bodyB;
        Transform3<real_Num> poses[(u32)physics::JointActorIndexEnum::COUNT];
        real_Num force = static_cast<real_Num>( 0.0 );
        real_Num torque = static_cast<real_Num>( 0.0 );

        real_Num linearTolerance = static_cast<real_Num>( 0.0 );
        real_Num angularTolerance = static_cast<real_Num>( 0.0 );

        u32 flags = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ConstraintStateData_h__
