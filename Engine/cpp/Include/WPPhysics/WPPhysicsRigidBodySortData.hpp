#ifndef WPPHYSICSRIGIDBODYSORTDATA_HPP
#define WPPHYSICSRIGIDBODYSORTDATA_HPP

#include <Workphone/Interface/Physics/IPhysicsBody2D.hpp>

namespace workphone::physics
{
    struct WPPhysicsRigidBodySortData
    {
        WPPhysicsRigidBodySortData() : body(nullptr)
        {
        }

        WPPhysicsRigidBodySortData(IPhysicsBody2D *body) : body(body)
        {
            yPos = body->getPosition().Y();
        }

        bool operator<(const WPPhysicsRigidBodySortData &other) const
        {
            return yPos > other.yPos;
        }

        real_Num yPos;
        IPhysicsBody2D *body;
    };
} // namespace workphone::physics

#endif  // WPPHYSICSRIGIDBODYSORTDATA_HPP
