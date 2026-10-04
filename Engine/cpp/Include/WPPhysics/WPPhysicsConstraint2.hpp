#ifndef WPPHYSICSCONSTRAINT2_HPP
#define WPPHYSICSCONSTRAINT2_HPP

#include <Workphone/Interface/Physics/IPhysicsConstraint2.hpp>

namespace workphone
{
    namespace physics
    {
        class WPPhysicsConstraint2 : public IPhysicsConstraint2
        {
        public:
            WPPhysicsConstraint2();
            virtual ~WPPhysicsConstraint2() override;

            virtual SmartPtr<IPhysicsBody2D> getBodyA() const override;
            virtual void setBodyA( SmartPtr<IPhysicsBody2D> bodyA ) override;
            virtual SmartPtr<IPhysicsBody2D> getBodyB() const override;
            virtual void setBodyB( SmartPtr<IPhysicsBody2D> bodyB ) override;

            virtual void *getUserData() const override;
            virtual void setUserData( void *userData ) override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace physics
}  // namespace workphone
#endif
