#ifndef WPPHYSICSSPRING_HPP
#define WPPHYSICSSPRING_HPP

#include <Workphone/Interface/Physics/IPhysicsSpring.hpp>

namespace workphone
{
    namespace physics
    {
        class WPPhysicsSpring : public IPhysicsSpring
        {
        public:
            WPPhysicsSpring();
            virtual ~WPPhysicsSpring() override;

            virtual real_Num getStiffness() const override;
            virtual void setStiffness( real_Num stiffness ) override;
            virtual real_Num getDamping() const override;
            virtual void setDamping( real_Num damping ) override;

            virtual void *getUserData() const override;
            virtual void setUserData( void *userData ) override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace physics
}  // namespace workphone
#endif
