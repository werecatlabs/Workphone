#ifndef WPPHYSICSCONSTRAINTFIXED2_HPP
#define WPPHYSICSCONSTRAINTFIXED2_HPP

#include <Workphone/Interface/Physics/IConstraintFixed2.hpp>

namespace workphone
{
    namespace physics
    {
        class WPPhysicsConstraintFixed2 : public IConstraintFixed2
        {
        public:
            WPPhysicsConstraintFixed2();
            virtual ~WPPhysicsConstraintFixed2() override;

            virtual void *getUserData() const override;
            virtual void setUserData( void *userData ) override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace physics
}  // namespace workphone
#endif
