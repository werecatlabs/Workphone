#ifndef __CPhysicsConstraint2_h__
#define __CPhysicsConstraint2_h__

#include <Workphone/Interface/Physics/IPhysicsConstraint2.hpp>

namespace workphone
{
    namespace physics
    {
        class CPhysicsConstraint2 : public IPhysicsConstraint2
        {
        public:
            CPhysicsConstraint2();
            virtual ~CPhysicsConstraint2() override;

            virtual SmartPtr<IPhysicsBody2D> getBodyA() const override;
            virtual void                     setBodyA( SmartPtr<IPhysicsBody2D> bodyA ) override;
            virtual SmartPtr<IPhysicsBody2D> getBodyB() const override;
            virtual void                     setBodyB( SmartPtr<IPhysicsBody2D> bodyB ) override;

            virtual void *getUserData() const override;
            virtual void  setUserData( void *userData ) override;

            WP_CLASS_REGISTER_DECL;
        };
    } // namespace physics
} // namespace workphone
#endif
