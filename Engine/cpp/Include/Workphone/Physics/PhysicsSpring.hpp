#ifndef PhysicsSpring_h__
#define PhysicsSpring_h__

#include <Workphone/Interface/Physics/IPhysicsSpring.hpp>

namespace workphone
{
    namespace physics
    {

        template <class T>
        class PhysicsSpring : public T
        {
        public:
            WP_CLASS_REGISTER_TEMPLATE_DECL( PhysicsSpring, T );

        protected:
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, PhysicsSpring, T, T );

    }  // namespace physics
}  // namespace workphone

#endif  // PhysicsSpring_h__
