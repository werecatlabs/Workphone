#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/CPhysicsSpring.hpp>

namespace workphone
{
    namespace physics
    {
        CPhysicsSpring::CPhysicsSpring()
        {
        }
        CPhysicsSpring::~CPhysicsSpring()
        {
        }

        real_Num CPhysicsSpring::getStiffness() const
        {
            return 0.0f;
        }
        void CPhysicsSpring::setStiffness( real_Num stiffness )
        {
        }
        real_Num CPhysicsSpring::getDamping() const
        {
            return 0.0f;
        }
        void CPhysicsSpring::setDamping( real_Num damping )
        {
        }
        void *CPhysicsSpring::getUserData() const
        {
            return nullptr;
        }
        void CPhysicsSpring::setUserData( void *userData )
        {
        }

        void CPhysicsSpring::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 CPhysicsSpring::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 CPhysicsSpring::typeInfo()
        {
            return sTypeInfo;
        }
        void CPhysicsSpring::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 CPhysicsSpring::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
