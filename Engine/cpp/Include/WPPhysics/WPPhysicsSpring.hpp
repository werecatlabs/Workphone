#ifndef WPPHYSICSSPRING_HPP
#define WPPHYSICSSPRING_HPP

#include <Workphone/Interface/Physics/IPhysicsSpring.hpp>

namespace workphone::physics
{
    class WPPhysicsSpring : public IPhysicsSpring
    {
    public:
        WPPhysicsSpring();
        ~WPPhysicsSpring() override;

        real_Num getStiffness() const override;
        void setStiffness(real_Num stiffness) override;
        real_Num getDamping() const override;
        void setDamping(real_Num damping) override;

        void *getUserData() const override;
        void setUserData(void *userData) override;

        WP_CLASS_REGISTER_DECL;
    };
}
#endif
