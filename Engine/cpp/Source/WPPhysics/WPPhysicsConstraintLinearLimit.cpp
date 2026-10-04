#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/WPPhysicsConstraintLinearLimit.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        WPPhysicsConstraintLinearLimit::WPPhysicsConstraintLinearLimit()
        {
        }
        WPPhysicsConstraintLinearLimit::~WPPhysicsConstraintLinearLimit()
        {
        }

        real_Num WPPhysicsConstraintLinearLimit::getValue() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintLinearLimit::setValue( real_Num value )
        {
        }
        real_Num WPPhysicsConstraintLinearLimit::getRestitution() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintLinearLimit::setRestitution( real_Num restitution )
        {
        }
        real_Num WPPhysicsConstraintLinearLimit::getBounceThreshold() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintLinearLimit::setBounceThreshold( real_Num bounceThreshold )
        {
        }
        real_Num WPPhysicsConstraintLinearLimit::getStiffness() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintLinearLimit::setStiffness( real_Num stiffness )
        {
        }
        real_Num WPPhysicsConstraintLinearLimit::getDamping() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintLinearLimit::setDamping( real_Num damping )
        {
        }
        real_Num WPPhysicsConstraintLinearLimit::getContactDistance() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintLinearLimit::setContactDistance( real_Num contactDistance )
        {
        }
        void *WPPhysicsConstraintLinearLimit::getUserData() const
        {
            return nullptr;
        }
        void WPPhysicsConstraintLinearLimit::setUserData( void *userData )
        {
        }

        void WPPhysicsConstraintLinearLimit::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 WPPhysicsConstraintLinearLimit::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 WPPhysicsConstraintLinearLimit::typeInfo()
        {
            return sTypeInfo;
        }
        void WPPhysicsConstraintLinearLimit::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 WPPhysicsConstraintLinearLimit::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
