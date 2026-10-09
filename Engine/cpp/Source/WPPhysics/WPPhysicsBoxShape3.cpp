#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsBoxShape3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    WPPhysicsBoxShape3::WPPhysicsBoxShape3() :
        WPPhysicsShape3T<BoxShape3>(WORKPHONE_COLLISION_SHAPE_BOX)
    {
    }

    SmartPtr<IPhysicsShape3> WPPhysicsBoxShape3::clone()
    {
        auto shape = workphone::make_ptr<WPPhysicsBoxShape3>();
        shape->setExtents(getExtents());
        shape->setLocalPose(getLocalPose());
        shape->setSimulationFilterData(getSimulationFilterData());
        shape->setCollisionType(getCollisionType());
        shape->setCollisionMask(getCollisionMask());
        shape->setMaterial(getMaterial());
        shape->setEnabled(isEnabled());
        shape->setTrigger(isTrigger());
        return shape;
    }
} // namespace workphone::physics
