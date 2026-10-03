#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/CPhysicsConstraint.hpp>

namespace workphone
{
    namespace physics
    {
        CPhysicsConstraint::CPhysicsConstraint()
        {
        }
        CPhysicsConstraint::~CPhysicsConstraint()
        {
        }

        void *CPhysicsConstraint::getUserData() const
        {
            return nullptr;
        }
        void CPhysicsConstraint::setUserData( void *userData )
        {
        }

        void CPhysicsConstraint::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 CPhysicsConstraint::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 CPhysicsConstraint::typeInfo()
        {
            return sTypeInfo;
        }
        void CPhysicsConstraint::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 CPhysicsConstraint::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
