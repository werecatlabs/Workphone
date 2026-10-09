#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsSphereShape3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    WPPhysicsSphereShape3::WPPhysicsSphereShape3() :
        WPPhysicsShape3T<SphereShape>(WORKPHONE_COLLISION_SHAPE_SPHERE)
    {
    }

    SmartPtr<IPhysicsShape3> WPPhysicsSphereShape3::clone()
    {
        auto shape = workphone::make_ptr<WPPhysicsSphereShape3>();
        shape->setRadius(getRadius());
        shape->setLocalPose(getLocalPose());
        shape->setSimulationFilterData(getSimulationFilterData());
        shape->setCollisionType(getCollisionType());
        shape->setCollisionMask(getCollisionMask());
        shape->setMaterial(getMaterial());
        shape->setEnabled(isEnabled());
        shape->setTrigger(isTrigger());
        return shape;
    }
}
