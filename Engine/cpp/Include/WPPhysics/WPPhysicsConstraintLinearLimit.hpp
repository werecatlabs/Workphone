#ifndef WPPHYSICSCONSTRAINTLINEARLIMIT_HPP
#define WPPHYSICSCONSTRAINTLINEARLIMIT_HPP

#include <Workphone/Interface/Physics/IConstraintLinearLimit.hpp>

namespace workphone
{
    namespace physics
    {
        class WPPhysicsConstraintLinearLimit : public IConstraintLinearLimit
        {
        public:
            WPPhysicsConstraintLinearLimit();
            virtual ~WPPhysicsConstraintLinearLimit() override;

            virtual real_Num getValue() const override;
            virtual void setValue( real_Num value ) override;

            virtual real_Num getRestitution() const override;
            virtual void setRestitution( real_Num restitution ) override;
            virtual real_Num getBounceThreshold() const override;
            virtual void setBounceThreshold( real_Num bounceThreshold ) override;
            virtual real_Num getStiffness() const override;
            virtual void setStiffness( real_Num stiffness ) override;
            virtual real_Num getDamping() const override;
            virtual void setDamping( real_Num damping ) override;
            virtual real_Num getContactDistance() const override;
            virtual void setContactDistance( real_Num contactDistance ) override;

            virtual void *getUserData() const override;
            virtual void setUserData( void *userData ) override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace physics
}  // namespace workphone
#endif
