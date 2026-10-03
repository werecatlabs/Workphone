#ifndef RigidBodySortData_h__
#define RigidBodySortData_h__

#include <Workphone/Interface/Physics/IPhysicsBody2D.hpp>

namespace workphone::physics
{
    struct RigidBodySortData
    {
        RigidBodySortData() : body( nullptr )
        {
        }

        RigidBodySortData( IPhysicsBody2D *body ) : body( body )
        {
            yPos = body->getPosition().Y();
        }

        bool operator<( const RigidBodySortData &other ) const
        {
            return yPos > other.yPos;
        }

        real_Num        yPos;
        IPhysicsBody2D *body;
    };
} // namespace workphone::physics

#endif // RigidBodySortData_h__
