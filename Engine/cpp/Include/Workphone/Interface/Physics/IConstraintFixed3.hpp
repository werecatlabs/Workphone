#ifndef IConstraintFixed3_h__
#define IConstraintFixed3_h__

#include <Workphone/Interface/Physics/IPhysicsConstraint3.hpp>

namespace workphone
{
    namespace physics
    {

        class WPCore_API IConstraintFixed3 : public IPhysicsConstraint3
        {
        public:
            ~IConstraintFixed3() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // IConstraintFixed3_h__
