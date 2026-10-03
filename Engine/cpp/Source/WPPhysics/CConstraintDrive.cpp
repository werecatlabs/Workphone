#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/CConstraintDrive.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        CConstraintDrive::CConstraintDrive()
        {
        }
        CConstraintDrive::~CConstraintDrive()
        {
        }

        real_Num CConstraintDrive::getForceLimit() const
        {
            return 0.0f;
        }
        void CConstraintDrive::setForceLimit( real_Num forceLimit )
        {
        }
        D6JointDriveFlagEnum CConstraintDrive::getDriveFlags() const
        {
            return static_cast<D6JointDriveFlagEnum>( 0 );
        }
        void CConstraintDrive::setDriveFlags( D6JointDriveFlagEnum driveFlags )
        {
        }
        void CConstraintDrive::setIsAcceleration( bool acceleration ) const
        {
        }
        bool CConstraintDrive::isAcceleration() const
        {
            return false;
        }
        real_Num CConstraintDrive::getStiffness() const
        {
            return 0.0f;
        }
        void CConstraintDrive::setStiffness( real_Num stiffness )
        {
        }
        real_Num CConstraintDrive::getDamping() const
        {
            return 0.0f;
        }
        void CConstraintDrive::setDamping( real_Num damping )
        {
        }
        void *CConstraintDrive::getUserData() const
        {
            return nullptr;
        }
        void CConstraintDrive::setUserData( void *userData )
        {
        }

        void CConstraintDrive::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 CConstraintDrive::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 CConstraintDrive::typeInfo()
        {
            return sTypeInfo;
        }
        void CConstraintDrive::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 CConstraintDrive::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
