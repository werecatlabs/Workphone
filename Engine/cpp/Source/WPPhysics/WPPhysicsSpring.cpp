#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/WPPhysicsSpring.hpp>

namespace workphone
{
    namespace physics
    {
        WPPhysicsSpring::WPPhysicsSpring()
        {
        }
        WPPhysicsSpring::~WPPhysicsSpring()
        {
        }

        real_Num WPPhysicsSpring::getStiffness() const
        {
            return 0.0f;
        }
        void WPPhysicsSpring::setStiffness( real_Num stiffness )
        {
        }
        real_Num WPPhysicsSpring::getDamping() const
        {
            return 0.0f;
        }
        void WPPhysicsSpring::setDamping( real_Num damping )
        {
        }
        void *WPPhysicsSpring::getUserData() const
        {
            return nullptr;
        }
        void WPPhysicsSpring::setUserData( void *userData )
        {
        }

        void WPPhysicsSpring::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 WPPhysicsSpring::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 WPPhysicsSpring::typeInfo()
        {
            return sTypeInfo;
        }
        void WPPhysicsSpring::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 WPPhysicsSpring::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
