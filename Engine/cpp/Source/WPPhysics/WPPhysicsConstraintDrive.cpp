#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/WPPhysicsConstraintDrive.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        WPPhysicsConstraintDrive::WPPhysicsConstraintDrive()
        {
        }
        WPPhysicsConstraintDrive::~WPPhysicsConstraintDrive()
        {
        }

        real_Num WPPhysicsConstraintDrive::getForceLimit() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintDrive::setForceLimit( real_Num forceLimit )
        {
        }
        D6JointDriveFlagEnum WPPhysicsConstraintDrive::getDriveFlags() const
        {
            return static_cast<D6JointDriveFlagEnum>( 0 );
        }
        void WPPhysicsConstraintDrive::setDriveFlags( D6JointDriveFlagEnum driveFlags )
        {
        }
        void WPPhysicsConstraintDrive::setIsAcceleration( bool acceleration ) const
        {
        }
        bool WPPhysicsConstraintDrive::isAcceleration() const
        {
            return false;
        }
        real_Num WPPhysicsConstraintDrive::getStiffness() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintDrive::setStiffness( real_Num stiffness )
        {
        }
        real_Num WPPhysicsConstraintDrive::getDamping() const
        {
            return 0.0f;
        }
        void WPPhysicsConstraintDrive::setDamping( real_Num damping )
        {
        }
        void *WPPhysicsConstraintDrive::getUserData() const
        {
            return nullptr;
        }
        void WPPhysicsConstraintDrive::setUserData( void *userData )
        {
        }

        void WPPhysicsConstraintDrive::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 WPPhysicsConstraintDrive::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 WPPhysicsConstraintDrive::typeInfo()
        {
            return sTypeInfo;
        }
        void WPPhysicsConstraintDrive::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 WPPhysicsConstraintDrive::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
