#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/CPhysicsConstraint2.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        CPhysicsConstraint2::CPhysicsConstraint2()
        {
        }
        CPhysicsConstraint2::~CPhysicsConstraint2()
        {
        }

        SmartPtr<IPhysicsBody2D> CPhysicsConstraint2::getBodyA() const
        {
            return nullptr;
        }
        void CPhysicsConstraint2::setBodyA( SmartPtr<IPhysicsBody2D> bodyA )
        {
        }
        SmartPtr<IPhysicsBody2D> CPhysicsConstraint2::getBodyB() const
        {
            return nullptr;
        }
        void CPhysicsConstraint2::setBodyB( SmartPtr<IPhysicsBody2D> bodyB )
        {
        }
        void *CPhysicsConstraint2::getUserData() const
        {
            return nullptr;
        }
        void CPhysicsConstraint2::setUserData( void *userData )
        {
        }

        void CPhysicsConstraint2::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 CPhysicsConstraint2::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 CPhysicsConstraint2::typeInfo()
        {
            return sTypeInfo;
        }
        void CPhysicsConstraint2::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 CPhysicsConstraint2::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
