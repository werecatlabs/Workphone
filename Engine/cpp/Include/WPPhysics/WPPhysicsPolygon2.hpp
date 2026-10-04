#ifndef WPPHYSICSPOLYGON2_HPP
#define WPPHYSICSPOLYGON2_HPP

#include "WPPhysics/WPPhysicsPrerequisites.hpp"
#include <Workphone/Interface/Physics/IPhysicsShape2.hpp>
#include <Workphone/Math/Polygon2.hpp>

namespace workphone::physics
{
    class WPPhysicsPolygon2 : public IPhysicsShape2
    {
    public:
    private:
        Polygon2F m_polygon;
    };
}  // namespace workphone::physics

// end namespace

#endif
