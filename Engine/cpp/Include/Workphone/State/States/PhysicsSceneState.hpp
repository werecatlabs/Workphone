#ifndef PhysicsSceneState_h__
#define PhysicsSceneState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>
#include <Workphone/State/States/RigidbodyState.hpp>

namespace workphone
{
    class WPCore_API PhysicsSceneState : public StateData
    {
    public:
        PhysicsSceneState();
        ~PhysicsSceneState() override;

        Vector3<real_Num> gravity = Vector3<real_Num>( 0.0f, -9.81f, 0.0f );
        Vector3<real_Num> size = Vector3<real_Num>( 100.0f, 100.0f, 100.0f );
        u32 minThreads = 0;
        u32 maxThreads = 4;

        physics::SpatialPartitioningMethodEnum spatialPartitioning =
            physics::SpatialPartitioningMethodEnum::None;
        physics::SpatialPartitioningOptions spatialOptions;
        physics::ContactOptions contactOptions;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // PhysicsSceneState_h__
