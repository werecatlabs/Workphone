#ifndef WPPHYSICSCONSTRAINT_HPP
#define WPPHYSICSCONSTRAINT_HPP

#include <Workphone/Interface/Physics/IPhysicsConstraint.hpp>

namespace workphone::physics
{
    class WPPhysicsConstraint : public IPhysicsConstraint
    {
    public:
        WPPhysicsConstraint();
        ~WPPhysicsConstraint() override;

        void *getUserData() const override;
        void setUserData(void *userData) override;

        WP_CLASS_REGISTER_DECL;
    };
}
#endif
