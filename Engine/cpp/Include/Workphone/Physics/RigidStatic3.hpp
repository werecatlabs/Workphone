//
// Created by Zane Desir on 31/10/2021.
//

#ifndef WP_CRIGIDSTATIC_H
#define WP_CRIGIDSTATIC_H

#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Physics/RigidBody3.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @class RigidStatic3
         * @brief Represents a static rigid body in 3D space.
         *
         * A static rigid body is an object that does not move or react to forces
         * (such as gravity or collisions) but can still be collided with by other
         * dynamic objects. This class is typically used for environment geometry,
         * walls, floors, and other immobile structures.
         */
        class WPCore_API RigidStatic3 : public RigidBody3<IRigidStatic3>
        {
        public:
            /**
             * @brief Constructs a new RigidStatic3 instance.
             */
            RigidStatic3();

            /**
             * @brief Destroys the RigidStatic3 instance.
             */
            ~RigidStatic3() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // WP_CRIGIDSTATIC_H
