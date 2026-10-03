#ifndef __CPhysicsConstraint_h__
#define __CPhysicsConstraint_h__

#include <Workphone/Interface/Physics/IPhysicsConstraint.hpp>

namespace workphone
{
    namespace physics
    {
        class CPhysicsConstraint : public IPhysicsConstraint
        {
        public:
            CPhysicsConstraint();
            virtual ~CPhysicsConstraint() override;

            virtual void *getUserData() const override;
            virtual void  setUserData( void *userData ) override;

            WP_CLASS_REGISTER_DECL;
        };
    } // namespace physics
} // namespace workphone
#endif
