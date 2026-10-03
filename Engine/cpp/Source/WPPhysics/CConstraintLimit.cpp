#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/CConstraintLimit.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        CConstraintLimit::CConstraintLimit()
        {
        }
        CConstraintLimit::~CConstraintLimit()
        {
        }

        real_Num CConstraintLimit::getRestitution() const
        {
            return 0.0f;
        }
        void CConstraintLimit::setRestitution( real_Num restitution )
        {
        }
        real_Num CConstraintLimit::getBounceThreshold() const
        {
            return 0.0f;
        }
        void CConstraintLimit::setBounceThreshold( real_Num bounceThreshold )
        {
        }
        real_Num CConstraintLimit::getStiffness() const
        {
            return 0.0f;
        }
        void CConstraintLimit::setStiffness( real_Num stiffness )
        {
        }
        real_Num CConstraintLimit::getDamping() const
        {
            return 0.0f;
        }
        void CConstraintLimit::setDamping( real_Num damping )
        {
        }
        real_Num CConstraintLimit::getContactDistance() const
        {
            return 0.0f;
        }
        void CConstraintLimit::setContactDistance( real_Num contactDistance )
        {
        }
        void *CConstraintLimit::getUserData() const
        {
            return nullptr;
        }
        void CConstraintLimit::setUserData( void *userData )
        {
        }

        void CConstraintLimit::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 CConstraintLimit::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 CConstraintLimit::typeInfo()
        {
            return sTypeInfo;
        }
        void CConstraintLimit::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 CConstraintLimit::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
