#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxConstraintDrive.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxConstraintDrive,
                               PhysxSharedObject<ConstraintDrive> );

    PhysxConstraintDrive::PhysxConstraintDrive() = default;

    PhysxConstraintDrive::~PhysxConstraintDrive() = default;

    auto PhysxConstraintDrive::getForceLimit() const -> physics_Num
    {
        return 0;
    }

    void PhysxConstraintDrive::setForceLimit( physics_Num forceLimit )
    {
    }

    auto PhysxConstraintDrive::getDriveFlags() const -> D6JointDriveFlagEnum
    {
        return static_cast<D6JointDriveFlagEnum>( 0 );
    }

    void PhysxConstraintDrive::setDriveFlags( D6JointDriveFlagEnum driveFlags )
    {
    }

    void PhysxConstraintDrive::setIsAcceleration( bool acceleration ) const
    {
    }

    auto PhysxConstraintDrive::isAcceleration() const -> bool
    {
        return false;
    }

    auto PhysxConstraintDrive::getStiffness() const -> physics_Num
    {
        return 0;
    }

    void PhysxConstraintDrive::setStiffness( physics_Num stiffness )
    {
    }

    auto PhysxConstraintDrive::getDamping() const -> physics_Num
    {
        return 0;
    }

    void PhysxConstraintDrive::setDamping( physics_Num damping )
    {
    }

} // namespace workphone::physics
