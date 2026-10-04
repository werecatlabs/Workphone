#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/WPPhysicsConstraintD6.hpp>
#include <WPPhysics/WPPhysicsConversions3.hpp>
#include <WPPhysics/WPPhysicsConstraintDrive.hpp>
#include <WPPhysics/WPPhysicsConstraintLinearLimit.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        namespace
        {
            /* Map the C++ D6AxisEnum to the C89 wp_d6_axis. */
            wp_d6_axis toWpAxis( D6AxisEnum axis )
            {
                switch( axis )
                {
                case D6AxisEnum::eX:      return WORKPHONE_D6_AXIS_X;
                case D6AxisEnum::eY:      return WORKPHONE_D6_AXIS_Y;
                case D6AxisEnum::eZ:      return WORKPHONE_D6_AXIS_Z;
                case D6AxisEnum::eTWIST:  return WORKPHONE_D6_AXIS_TWIST;
                case D6AxisEnum::eSWING1: return WORKPHONE_D6_AXIS_SWING1;
                case D6AxisEnum::eSWING2: return WORKPHONE_D6_AXIS_SWING2;
                default:                  return WORKPHONE_D6_AXIS_X;
                }
            }

            /* Map the C89 wp_d6_motion back to the C++ D6MotionEnum. */
            D6MotionEnum fromWpMotion( wp_d6_motion motion )
            {
                switch( motion )
                {
                case WORKPHONE_D6_MOTION_LIMITED: return D6MotionEnum::eLIMITED;
                case WORKPHONE_D6_MOTION_FREE:   return D6MotionEnum::eFREE;
                case WORKPHONE_D6_MOTION_LOCKED:
                default:                          return D6MotionEnum::eLOCKED;
                }
            }

            /* Map the C++ D6MotionEnum to the C89 wp_d6_motion. */
            wp_d6_motion toWpMotion( D6MotionEnum type )
            {
                switch( type )
                {
                case D6MotionEnum::eLIMITED: return WORKPHONE_D6_MOTION_LIMITED;
                case D6MotionEnum::eFREE:    return WORKPHONE_D6_MOTION_FREE;
                case D6MotionEnum::eLOCKED:
                default:                     return WORKPHONE_D6_MOTION_LOCKED;
                }
            }

            /* Map the C++ D6DriveEnum to the C89 wp_d6_drive. */
            wp_d6_drive toWpDrive( D6DriveEnum index )
            {
                switch( index )
                {
                case D6DriveEnum::eX:     return WORKPHONE_D6_DRIVE_X;
                case D6DriveEnum::eY:     return WORKPHONE_D6_DRIVE_Y;
                case D6DriveEnum::eZ:     return WORKPHONE_D6_DRIVE_Z;
                case D6DriveEnum::eSWING: return WORKPHONE_D6_DRIVE_SWING;
                case D6DriveEnum::eTWIST: return WORKPHONE_D6_DRIVE_TWIST;
                case D6DriveEnum::eSLERP:  return WORKPHONE_D6_DRIVE_SLERP;
                default:                   return WORKPHONE_D6_DRIVE_X;
                }
            }

            /* Convert a JointActorIndexEnum to the C89 actor index (0 or 1). */
            wp_s32 toActorIndex( JointActorIndexEnum actor )
            {
                return actor == JointActorIndexEnum::eACTOR1 ? 1 : 0;
            }

            /* Translate a C++ ConstraintFlagEnum bit into the matching C89 flag bit. */
            wp_u32 toWpConstraintFlag( ConstraintFlagEnum flag )
            {
                switch( flag )
                {
                case ConstraintFlagEnum::eBROKEN:                return WORKPHONE_CONSTRAINT_FLAG_BROKEN;
                case ConstraintFlagEnum::ePROJECT_TO_ACTOR0:      return WORKPHONE_CONSTRAINT_FLAG_PROJECT_TO_ACTOR0;
                case ConstraintFlagEnum::ePROJECT_TO_ACTOR1:      return WORKPHONE_CONSTRAINT_FLAG_PROJECT_TO_ACTOR1;
                case ConstraintFlagEnum::ePROJECTION:            return WORKPHONE_CONSTRAINT_FLAG_PROJECTION;
                case ConstraintFlagEnum::eCOLLISION_ENABLED:     return WORKPHONE_CONSTRAINT_FLAG_COLLISION_ENABLED;
                case ConstraintFlagEnum::eREPORTING:             return WORKPHONE_CONSTRAINT_FLAG_REPORTING;
                case ConstraintFlagEnum::eVISUALIZATION:         return WORKPHONE_CONSTRAINT_FLAG_VISUALIZATION;
                case ConstraintFlagEnum::eDRIVE_LIMITS_ARE_FORCES: return WORKPHONE_CONSTRAINT_FLAG_DRIVE_LIMITS_FORCE;
                case ConstraintFlagEnum::eIMPROVED_SLERP:        return WORKPHONE_CONSTRAINT_FLAG_IMPROVED_SLERP;
                default:                                         return 0;
                }
            }

            /* Retrieve the raw wp_rigidbody* from an IPhysicsBody3, if any. */
            wp_rigidbody *bodyToWp( const SmartPtr<IPhysicsBody3> &body )
            {
                if( !body )
                {
                    return nullptr;
                }
                void *raw = nullptr;
                body->_getObject( &raw );
                return static_cast<wp_rigidbody *>( raw );
            }
        } // namespace

        WPPhysicsConstraintD6::WPPhysicsConstraintD6()
        {
            m_constraint = wp_constraint_create( WORKPHONE_CONSTRAINT_D6 );
        }

        WPPhysicsConstraintD6::~WPPhysicsConstraintD6()
        {
            if( m_constraint )
            {
                wp_constraint_destroy( m_constraint );
                m_constraint = nullptr;
            }
        }

        wp_constraint *WPPhysicsConstraintD6::getConstraint() const
        {
            return m_constraint;
        }

        /* ----------------------------------------------------------------- *
         * IConstraintD6
         * ----------------------------------------------------------------- */

        void WPPhysicsConstraintD6::setDrivePosition( const Transform3<real_Num> &pose )
        {
            if( !m_constraint )
            {
                return;
            }
            wp_constraint_set_drive_position( m_constraint, detail::toWp( pose.getPosition() ) );
            wp_constraint_set_drive_orientation( m_constraint, detail::toWp( pose.getOrientation() ) );
        }

        Transform3<real_Num> WPPhysicsConstraintD6::getDrivePosition() const
        {
            if( !m_constraint )
            {
                return Transform3<real_Num>();
            }
            return Transform3<real_Num>( detail::fromWp( wp_constraint_get_drive_position( m_constraint ) ),
                                         detail::fromWp( wp_constraint_get_drive_orientation( m_constraint ) ) );
        }

        void WPPhysicsConstraintD6::setDrive( D6DriveEnum index, SmartPtr<IConstraintDrive> drive )
        {
            const auto cIndex = toWpDrive( index );
            if( cIndex >= 0 && cIndex < WORKPHONE_D6_DRIVE_COUNT )
            {
                m_drives[static_cast<int>( cIndex )] = drive;
            }

            if( !m_constraint || !drive )
            {
                return;
            }

            wp_constraint_drive_desc desc;
            desc.stiffness     = static_cast<wp_f32>( drive->getStiffness() );
            desc.damping       = static_cast<wp_f32>( drive->getDamping() );
            desc.force_limit   = static_cast<wp_f32>( drive->getForceLimit() );
            desc.is_acceleration = drive->isAcceleration() ? 1 : 0;
            wp_constraint_set_drive( m_constraint, cIndex, desc );
        }

        SmartPtr<IConstraintDrive> WPPhysicsConstraintD6::getDrive( D6DriveEnum index ) const
        {
            const auto cIndex = toWpDrive( index );
            if( cIndex >= 0 && cIndex < WORKPHONE_D6_DRIVE_COUNT )
            {
                return m_drives[static_cast<int>( cIndex )];
            }
            return nullptr;
        }

        void WPPhysicsConstraintD6::setLinearLimit( SmartPtr<IConstraintLinearLimit> limit )
        {
            m_linearLimit = limit;

            if( !m_constraint || !limit )
            {
                return;
            }

            wp_constraint_linear_limit cLimit;
            cLimit.value            = static_cast<wp_f32>( limit->getValue() );
            cLimit.restitution      = static_cast<wp_f32>( limit->getRestitution() );
            cLimit.bounce_threshold = static_cast<wp_f32>( limit->getBounceThreshold() );
            cLimit.stiffness        = static_cast<wp_f32>( limit->getStiffness() );
            cLimit.damping          = static_cast<wp_f32>( limit->getDamping() );
            cLimit.contact_distance = static_cast<wp_f32>( limit->getContactDistance() );
            wp_constraint_set_linear_limit( m_constraint, cLimit );
        }

        SmartPtr<IConstraintLinearLimit> WPPhysicsConstraintD6::getLinearLimit() const
        {
            return m_linearLimit;
        }

        void WPPhysicsConstraintD6::setMotion( D6AxisEnum axis, D6MotionEnum type )
        {
            if( !m_constraint )
            {
                return;
            }
            wp_constraint_set_motion( m_constraint, toWpAxis( axis ), toWpMotion( type ) );
        }

        D6MotionEnum WPPhysicsConstraintD6::getMotion( D6AxisEnum axis ) const
        {
            if( !m_constraint )
            {
                return D6MotionEnum::eLOCKED;
            }
            return fromWpMotion( wp_constraint_get_motion( m_constraint, toWpAxis( axis ) ) );
        }

        /* ----------------------------------------------------------------- *
         * IPhysicsConstraint3
         * ----------------------------------------------------------------- */

        SmartPtr<IPhysicsBody3> WPPhysicsConstraintD6::getBodyA() const
        {
            return m_bodyA;
        }

        void WPPhysicsConstraintD6::setBodyA( SmartPtr<IPhysicsBody3> bodyA )
        {
            m_bodyA = bodyA;
            if( m_constraint )
            {
                wp_constraint_set_body_a( m_constraint, bodyToWp( bodyA ) );
            }
        }

        SmartPtr<IPhysicsBody3> WPPhysicsConstraintD6::getBodyB() const
        {
            return m_bodyB;
        }

        void WPPhysicsConstraintD6::setBodyB( SmartPtr<IPhysicsBody3> bodyB )
        {
            m_bodyB = bodyB;
            if( m_constraint )
            {
                wp_constraint_set_body_b( m_constraint, bodyToWp( bodyB ) );
            }
        }

        void WPPhysicsConstraintD6::setLocalPose( JointActorIndexEnum         actor,
                                         const Transform3<real_Num> &localPose )
        {
            if( !m_constraint )
            {
                return;
            }
            const auto idx = toActorIndex( actor );
            wp_constraint_set_local_position( m_constraint, idx, detail::toWp( localPose.getPosition() ) );
            wp_constraint_set_local_orientation( m_constraint, idx, detail::toWp( localPose.getOrientation() ) );
        }

        Transform3<real_Num> WPPhysicsConstraintD6::getLocalPose( JointActorIndexEnum actor ) const
        {
            if( !m_constraint )
            {
                return Transform3<real_Num>();
            }
            const auto idx = toActorIndex( actor );
            return Transform3<real_Num>( detail::fromWp( wp_constraint_get_local_position( m_constraint, idx ) ),
                                         detail::fromWp( wp_constraint_get_local_orientation( m_constraint, idx ) ) );
        }

        void WPPhysicsConstraintD6::setConstraintFlag( ConstraintFlagEnum flag, bool value )
        {
            if( !m_constraint )
            {
                return;
            }
            wp_constraint_set_flag( m_constraint, toWpConstraintFlag( flag ), value ? 1 : 0 );
        }

        ConstraintFlagEnum WPPhysicsConstraintD6::getConstraintFlags() const
        {
            if( !m_constraint )
            {
                return static_cast<ConstraintFlagEnum>( 0 );
            }
            return static_cast<ConstraintFlagEnum>( wp_constraint_get_flags( m_constraint ) );
        }

        void WPPhysicsConstraintD6::setBreakForce( real_Num force, real_Num torque )
        {
            if( !m_constraint )
            {
                return;
            }
            wp_constraint_set_break_force( m_constraint,
                                           static_cast<wp_f32>( force ),
                                           static_cast<wp_f32>( torque ) );
        }

        void WPPhysicsConstraintD6::getBreakForce( real_Num &force, real_Num &torque ) const
        {
            if( !m_constraint )
            {
                force = 0.0f;
                torque = 0.0f;
                return;
            }
            force = static_cast<real_Num>( wp_constraint_get_break_force( m_constraint ) );
            torque = static_cast<real_Num>( wp_constraint_get_break_torque( m_constraint ) );
        }

        void WPPhysicsConstraintD6::setProjectionLinearTolerance( real_Num tolerance )
        {
            if( !m_constraint )
            {
                return;
            }
            wp_constraint_set_projection_linear_tolerance( m_constraint,
                                                           static_cast<wp_f32>( tolerance ) );
        }

        real_Num WPPhysicsConstraintD6::getProjectionLinearTolerance() const
        {
            if( !m_constraint )
            {
                return 0.0f;
            }
            return static_cast<real_Num>( wp_constraint_get_projection_linear_tolerance( m_constraint ) );
        }

        void WPPhysicsConstraintD6::setProjectionAngularTolerance( real_Num tolerance )
        {
            if( !m_constraint )
            {
                return;
            }
            wp_constraint_set_projection_angular_tolerance( m_constraint,
                                                            static_cast<wp_f32>( tolerance ) );
        }

        real_Num WPPhysicsConstraintD6::getProjectionAngularTolerance() const
        {
            if( !m_constraint )
            {
                return 0.0f;
            }
            return static_cast<real_Num>( wp_constraint_get_projection_angular_tolerance( m_constraint ) );
        }

        /* ----------------------------------------------------------------- *
         * ISharedObject / user data
         * ----------------------------------------------------------------- */

        void *WPPhysicsConstraintD6::getUserData() const
        {
            if( m_constraint )
            {
                return wp_constraint_get_user_data( m_constraint );
            }
            return m_userData;
        }

        void WPPhysicsConstraintD6::setUserData( void *userData )
        {
            m_userData = userData;
            if( m_constraint )
            {
                wp_constraint_set_user_data( m_constraint, userData );
            }
        }
        /* ----------------------------------------------------------------- *
         * Class registration support
         * ----------------------------------------------------------------- */

        void WPPhysicsConstraintD6::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 WPPhysicsConstraintD6::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 WPPhysicsConstraintD6::typeInfo()
        {
            return sTypeInfo;
        }
        void WPPhysicsConstraintD6::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 WPPhysicsConstraintD6::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
