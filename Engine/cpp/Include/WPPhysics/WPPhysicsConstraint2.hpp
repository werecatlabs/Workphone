#ifndef WPPHYSICSCONSTRAINT2_HPP
#define WPPHYSICSCONSTRAINT2_HPP

#include <Workphone/Interface/Physics/IPhysicsConstraint2.hpp>

namespace workphone::physics
{
    class WPPhysicsConstraint2 : public IPhysicsConstraint2
    {
    public:
        WPPhysicsConstraint2();
        ~WPPhysicsConstraint2() override;

        SmartPtr<IPhysicsBody2D> getBodyA() const override;
        void setBodyA(SmartPtr<IPhysicsBody2D> bodyA) override;
        SmartPtr<IPhysicsBody2D> getBodyB() const override;
        void setBodyB(SmartPtr<IPhysicsBody2D> bodyB) override;

        void *getUserData() const override;
        void setUserData(void *userData) override;

        WP_CLASS_REGISTER_DECL;
    };
}
#endif
