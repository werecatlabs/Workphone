#ifndef WPPHYSICSSPHERESHAPE3_HPP
#define WPPHYSICSSPHERESHAPE3_HPP
#include <WPPhysics/WPPhysicsShape3T.hpp>
#include <Workphone/Physics/SphereShape.hpp>

namespace workphone::physics
{
    class WPPhysicsSphereShape3 : public WPPhysicsShape3T<SphereShape>
    {
    public:
        WPPhysicsSphereShape3();
        SmartPtr<IPhysicsShape3> clone() override;
    };
}
#endif
