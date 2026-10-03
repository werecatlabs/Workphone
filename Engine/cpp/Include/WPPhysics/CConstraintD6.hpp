#ifndef __CConstraintD6_h__
#define __CConstraintD6_h__

#include <Workphone/Interface/Physics/IConstraintD6.hpp>

// C89 WorkphonePhysics backend
extern "C"
{
#include <WorkphonePhysics/workphone_physics_constraint.h>
}

namespace workphone
{
    namespace physics
    {
        class CConstraintD6 : public IConstraintD6
        {
        public:
            CConstraintD6();
            virtual ~CConstraintD6() override;

            virtual void                 setDrivePosition( const Transform3<real_Num> &pose ) override;
            virtual Transform3<real_Num> getDrivePosition() const override;
            virtual void setDrive( D6DriveEnum index, SmartPtr<IConstraintDrive> drive ) override;
            virtual SmartPtr<IConstraintDrive> getDrive( D6DriveEnum index ) const override;
            virtual void setLinearLimit( SmartPtr<IConstraintLinearLimit> limit ) override;
            virtual SmartPtr<IConstraintLinearLimit> getLinearLimit() const override;
            virtual void         setMotion( D6AxisEnum axis, D6MotionEnum type ) override;
            virtual D6MotionEnum getMotion( D6AxisEnum axis ) const override;

            virtual SmartPtr<IPhysicsBody3> getBodyA() const override;
            virtual void                    setBodyA( SmartPtr<IPhysicsBody3> bodyA ) override;
            virtual SmartPtr<IPhysicsBody3> getBodyB() const override;
            virtual void                    setBodyB( SmartPtr<IPhysicsBody3> bodyB ) override;
            virtual void                 setLocalPose( JointActorIndexEnum         actor,
                                                       const Transform3<real_Num> &localPose ) override;
            virtual Transform3<real_Num> getLocalPose( JointActorIndexEnum actor ) const override;
            virtual void               setConstraintFlag( ConstraintFlagEnum flag, bool value ) override;
            virtual ConstraintFlagEnum getConstraintFlags() const override;
            virtual void               setBreakForce( real_Num force, real_Num torque ) override;
            virtual void               getBreakForce( real_Num &force, real_Num &torque ) const override;
            virtual void               setProjectionLinearTolerance( real_Num tolerance ) override;
            virtual real_Num           getProjectionLinearTolerance() const override;
            virtual void               setProjectionAngularTolerance( real_Num tolerance ) override;
            virtual real_Num           getProjectionAngularTolerance() const override;

            virtual void *getUserData() const override;
            virtual void  setUserData( void *userData ) override;

            /**
             * @brief Returns the underlying C89 WorkphonePhysics constraint handle.
             * @return Pointer to the internal wp_constraint.
             */
            wp_constraint *getConstraint() const;

            WP_CLASS_REGISTER_DECL;

        private:
            wp_constraint *m_constraint = nullptr; ///< Underlying C89 constraint (D6 joint)

            SmartPtr<IPhysicsBody3> m_bodyA; ///< First body involved in the constraint
            SmartPtr<IPhysicsBody3> m_bodyB; ///< Second body involved in the constraint

            // Cached drive/limit objects so getters can return the last value set.
            SmartPtr<IConstraintDrive>       m_drives[WORKPHONE_D6_DRIVE_COUNT];
            SmartPtr<IConstraintLinearLimit> m_linearLimit;

            void *m_userData = nullptr; ///< Opaque user data
        };
    } // namespace physics
} // namespace workphone
#endif
