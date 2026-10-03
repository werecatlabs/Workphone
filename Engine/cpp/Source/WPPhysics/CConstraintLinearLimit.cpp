#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/CConstraintLinearLimit.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        CConstraintLinearLimit::CConstraintLinearLimit()
        {
        }
        CConstraintLinearLimit::~CConstraintLinearLimit()
        {
        }

        real_Num CConstraintLinearLimit::getValue() const
        {
            return 0.0f;
        }
        void CConstraintLinearLimit::setValue( real_Num value )
        {
        }
        real_Num CConstraintLinearLimit::getRestitution() const
        {
            return 0.0f;
        }
        void CConstraintLinearLimit::setRestitution( real_Num restitution )
        {
        }
        real_Num CConstraintLinearLimit::getBounceThreshold() const
        {
            return 0.0f;
        }
        void CConstraintLinearLimit::setBounceThreshold( real_Num bounceThreshold )
        {
        }
        real_Num CConstraintLinearLimit::getStiffness() const
        {
            return 0.0f;
        }
        void CConstraintLinearLimit::setStiffness( real_Num stiffness )
        {
        }
        real_Num CConstraintLinearLimit::getDamping() const
        {
            return 0.0f;
        }
        void CConstraintLinearLimit::setDamping( real_Num damping )
        {
        }
        real_Num CConstraintLinearLimit::getContactDistance() const
        {
            return 0.0f;
        }
        void CConstraintLinearLimit::setContactDistance( real_Num contactDistance )
        {
        }
        void *CConstraintLinearLimit::getUserData() const
        {
            return nullptr;
        }
        void CConstraintLinearLimit::setUserData( void *userData )
        {
        }

        void CConstraintLinearLimit::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 CConstraintLinearLimit::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 CConstraintLinearLimit::typeInfo()
        {
            return sTypeInfo;
        }
        void CConstraintLinearLimit::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 CConstraintLinearLimit::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
