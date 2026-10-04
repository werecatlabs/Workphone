#ifndef WPPHYSICSCONSTRAINT_HPP
#define WPPHYSICSCONSTRAINT_HPP

#include <Workphone/Interface/Physics/IPhysicsConstraint.hpp>

namespace workphone
{
    namespace physics
    {
        class WPPhysicsConstraint : public IPhysicsConstraint
        {
        public:
            WPPhysicsConstraint();
            virtual ~WPPhysicsConstraint() override;

            virtual void *getUserData() const override;
            virtual void setUserData( void *userData ) override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace physics
}  // namespace workphone
#endif
