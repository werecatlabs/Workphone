#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/WPPhysicsConstraint2.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        WPPhysicsConstraint2::WPPhysicsConstraint2()
        {
        }
        WPPhysicsConstraint2::~WPPhysicsConstraint2()
        {
        }

        SmartPtr<IPhysicsBody2D> WPPhysicsConstraint2::getBodyA() const
        {
            return nullptr;
        }
        void WPPhysicsConstraint2::setBodyA( SmartPtr<IPhysicsBody2D> bodyA )
        {
        }
        SmartPtr<IPhysicsBody2D> WPPhysicsConstraint2::getBodyB() const
        {
            return nullptr;
        }
        void WPPhysicsConstraint2::setBodyB( SmartPtr<IPhysicsBody2D> bodyB )
        {
        }
        void *WPPhysicsConstraint2::getUserData() const
        {
            return nullptr;
        }
        void WPPhysicsConstraint2::setUserData( void *userData )
        {
        }

        void WPPhysicsConstraint2::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 WPPhysicsConstraint2::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 WPPhysicsConstraint2::typeInfo()
        {
            return sTypeInfo;
        }
        void WPPhysicsConstraint2::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 WPPhysicsConstraint2::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
