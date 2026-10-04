#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsBoxShape3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    WPPhysicsBoxShape3::WPPhysicsBoxShape3() :
        WPPhysicsShape3T<BoxShape3>( WORKPHONE_COLLISION_SHAPE_BOX )
    {
    }
} // namespace workphone::physics
