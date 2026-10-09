#ifndef WPPHYSICSCONSTRAINTFIXED2_HPP
#define WPPHYSICSCONSTRAINTFIXED2_HPP

#include <Workphone/Interface/Physics/IConstraintFixed2.hpp>

namespace workphone::physics
{
    class WPPhysicsConstraintFixed2 : public IConstraintFixed2
    {
    public:
        WPPhysicsConstraintFixed2();
        ~WPPhysicsConstraintFixed2() override;

        void *getUserData() const override;
        void setUserData(void *userData) override;

        WP_CLASS_REGISTER_DECL;
    };
}
#endif
