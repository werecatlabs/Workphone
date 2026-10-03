#ifndef PhysicsMaterialStateData_h__
#define PhysicsMaterialStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>
#include <Workphone/Core/FixedArray.hpp>

namespace workphone
{
    class WPCore_API PhysicsMaterialStateData : public StateData
    {
    public:
        PhysicsMaterialStateData();
        ~PhysicsMaterialStateData() override;

        SmartPtr<physics::IRigidBody3> physicsBodyA;
        SmartPtr<physics::IRigidBody3> physicsBodyB;
        FixedArray<real_Num, 2> staticFriction = { 0.5f, 0.5f };
        FixedArray<real_Num, 2> dynamicFriction = { 0.5f, 0.5f };
        f32 restitution = 0.5f;
        f32 rollingFriction = 0.5f;
        physics::FrictionCombineMode frictionCombineMode = physics::FrictionCombineMode::Average;
        physics::RestitutionCombineMode restitutionCombineMode =
            physics::RestitutionCombineMode::Average;
        String materialName;

        Vector3<real_Num> contactPosition;
        Vector3<real_Num> contactNormal;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // PhysicsMaterialStateData_h__
