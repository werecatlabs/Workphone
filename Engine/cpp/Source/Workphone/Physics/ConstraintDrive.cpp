#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/ConstraintDrive.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace physics
    {
        WP_CLASS_REGISTER_DERIVED( workphone::physics, ConstraintDrive, IConstraintDrive );

        ConstraintDrive::ConstraintDrive() :
            m_stiffness( static_cast<real_Num>( 0.0 ) ),
            m_damping( static_cast<real_Num>( 0.0 ) ),
            m_forceLimit( static_cast<real_Num>( 0.0 ) ),
            m_driveFlags( D6JointDriveFlagEnum::eACCELERATION ),
            m_isAcceleration( true )
        {
        }

        ConstraintDrive::~ConstraintDrive() = default;

        void ConstraintDrive::load( SmartPtr<ISharedObject> data )
        {
            // Implementation for loading constraint drive data
            // This would typically deserialize settings from the data object
        }

        void ConstraintDrive::unload( SmartPtr<ISharedObject> data )
        {
            // Implementation for unloading constraint drive data
            // This would typically reset or clear the drive settings
            m_stiffness = static_cast<real_Num>( 0.0 );
            m_damping = static_cast<real_Num>( 0.0 );
            m_forceLimit = static_cast<real_Num>( 0.0 );
            m_driveFlags = D6JointDriveFlagEnum::eACCELERATION;
            m_isAcceleration = true;
        }

        real_Num ConstraintDrive::getStiffness() const
        {
            return m_stiffness;
        }

        void ConstraintDrive::setStiffness( real_Num stiffness )
        {
            m_stiffness = stiffness;
        }

        real_Num ConstraintDrive::getDamping() const
        {
            return m_damping;
        }

        void ConstraintDrive::setDamping( real_Num damping )
        {
            m_damping = damping;
        }

        real_Num ConstraintDrive::getForceLimit() const
        {
            return m_forceLimit;
        }

        void ConstraintDrive::setForceLimit( real_Num forceLimit )
        {
            m_forceLimit = forceLimit;
        }

        D6JointDriveFlagEnum ConstraintDrive::getDriveFlags() const
        {
            return m_driveFlags;
        }

        void ConstraintDrive::setDriveFlags( D6JointDriveFlagEnum driveFlags )
        {
            m_driveFlags = driveFlags;

            // Update acceleration flag based on drive flags
            m_isAcceleration = ( static_cast<u32>( driveFlags ) &
                                 static_cast<u32>( D6JointDriveFlagEnum::eACCELERATION ) ) != 0;
        }

        void ConstraintDrive::setIsAcceleration( bool acceleration ) const
        {
            // Note: This method is marked const in the interface but modifies state
            // This is likely a design issue in the interface, but we'll implement as required
            const_cast<ConstraintDrive *>( this )->m_isAcceleration = acceleration;

            // Update drive flags to match acceleration setting
            if( acceleration )
            {
                const_cast<ConstraintDrive *>( this )->m_driveFlags =
                    D6JointDriveFlagEnum::eACCELERATION;
            }
            else
            {
                const_cast<ConstraintDrive *>( this )->m_driveFlags =
                    static_cast<D6JointDriveFlagEnum>( 0 );
            }
        }

        bool ConstraintDrive::isAcceleration() const
        {
            return m_isAcceleration;
        }

    }  // namespace physics
}  // namespace workphone
