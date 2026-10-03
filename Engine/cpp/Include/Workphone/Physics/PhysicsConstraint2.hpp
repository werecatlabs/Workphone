#ifndef PhysicsConstraint2_h__
#define PhysicsConstraint2_h__

#include <Workphone/Interface/Physics/IPhysicsConstraint2.hpp>

namespace workphone
{
    namespace physics
    {

        template <class T>
        class WPCore_API PhysicsConstraint2 : public T
        {
        public:
            PhysicsConstraint2();
            ~PhysicsConstraint2() override;

            WP_CLASS_REGISTER_TEMPLATE_DECL( PhysicsConstraint2, T );
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::physics, PhysicsConstraint2, T, T );

        template <class T>
        PhysicsConstraint2<T>::PhysicsConstraint2()
        {
        }

        template <class T>
        PhysicsConstraint2<T>::~PhysicsConstraint2()
        {
        }

    }  // namespace physics
}  // namespace workphone

#endif  // PhysicsConstraint2_h__
