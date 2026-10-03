#ifndef IPhysicsRigidStatic3_h__
#define IPhysicsRigidStatic3_h__

#include <Workphone/Interface/Physics/IRigidBody3.hpp>

namespace workphone
{
    namespace physics
    {

        class WPCore_API IRigidStatic3 : public IRigidBody3
        {
        public:
            ~IRigidStatic3() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // IPhysicsRigidStatic3_h__
