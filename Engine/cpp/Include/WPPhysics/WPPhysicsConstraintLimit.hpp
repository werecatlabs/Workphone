#ifndef WPPHYSICSCONSTRAINTLIMIT_HPP
#define WPPHYSICSCONSTRAINTLIMIT_HPP

#include <Workphone/Interface/Physics/IConstraintLimit.hpp>

namespace workphone::physics
{
    class WPPhysicsConstraintLimit : public IConstraintLimit
    {
    public:
        WPPhysicsConstraintLimit();
        ~WPPhysicsConstraintLimit() override;

        real_Num getRestitution() const override;
        void setRestitution(real_Num restitution) override;
        real_Num getBounceThreshold() const override;
        void setBounceThreshold(real_Num bounceThreshold) override;
        real_Num getStiffness() const override;
        void setStiffness(real_Num stiffness) override;
        real_Num getDamping() const override;
        void setDamping(real_Num damping) override;
        real_Num getContactDistance() const override;
        void setContactDistance(real_Num contactDistance) override;

        void *getUserData() const override;
        void setUserData(void *userData) override;

        WP_CLASS_REGISTER_DECL;
    };
}
#endif
