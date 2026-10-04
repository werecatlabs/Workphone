#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/WPPhysicsConstraintLimit.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        WPPhysicsConstraintLimit::WPPhysicsConstraintLimit()
        {
        }
        WPPhysicsConstraintLimit::~WPPhysicsConstraintLimit()
        {
        }

        real_Num WPPhysicsConstraintLimit::getRestitution() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintLimit::setRestitution( real_Num restitution )
        {
        }
        real_Num WPPhysicsConstraintLimit::getBounceThreshold() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintLimit::setBounceThreshold( real_Num bounceThreshold )
        {
        }
        real_Num WPPhysicsConstraintLimit::getStiffness() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintLimit::setStiffness( real_Num stiffness )
        {
        }
        real_Num WPPhysicsConstraintLimit::getDamping() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintLimit::setDamping( real_Num damping )
        {
        }
        real_Num WPPhysicsConstraintLimit::getContactDistance() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintLimit::setContactDistance( real_Num contactDistance )
        {
        }
        void *WPPhysicsConstraintLimit::getUserData() const
        {
            return nullptr;
        }
        void WPPhysicsConstraintLimit::setUserData( void *userData )
        {
        }

        void WPPhysicsConstraintLimit::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 WPPhysicsConstraintLimit::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 WPPhysicsConstraintLimit::typeInfo()
        {
            return sTypeInfo;
        }
        void WPPhysicsConstraintLimit::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 WPPhysicsConstraintLimit::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
