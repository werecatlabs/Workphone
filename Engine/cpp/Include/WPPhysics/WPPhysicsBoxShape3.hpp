#ifndef WPPHYSICSBOXSHAPE3_HPP
#define WPPHYSICSBOXSHAPE3_HPP

#include <WPPhysics/WPPhysicsShape3T.hpp>
#include <Workphone/Physics/BoxShape3.hpp>

namespace workphone::physics
{
    /**
     * @class WPPhysicsBoxShape3
     * @brief Implementation of a box-shaped physics collider.
     *
     * This class represents a 3D box shape used in the physics simulation,
     * providing functionality to manage its extents, pose, material, and collision properties.
     */
    class WPPhysicsBoxShape3 : public WPPhysicsShape3T<BoxShape3>
    {
    public:
        WPPhysicsBoxShape3();
    };
}  // namespace workphone::physics

#endif
