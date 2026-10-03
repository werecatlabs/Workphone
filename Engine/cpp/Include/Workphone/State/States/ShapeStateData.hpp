#ifndef ShapeState_h__
#define ShapeState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{

    class WPCore_API ShapeStateData : public StateData
    {
    public:
        ShapeStateData();
        ~ShapeStateData() override;

        WP_CLASS_REGISTER_DECL;

        Transform3<real_Num> localPose;
        u32 flags = physics::IPhysicsShape::ShapeFlagEnabled;
        u32 collisionType = std::numeric_limits<u32>::max();
        u32 collisionMask = std::numeric_limits<u32>::max();
    };

}  // namespace workphone

#endif  // ShapeState_h__
