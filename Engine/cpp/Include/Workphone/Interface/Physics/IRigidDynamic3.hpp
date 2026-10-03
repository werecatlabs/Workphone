#ifndef IRigidDynamic3_h__
#define IRigidDynamic3_h__

#include <Workphone/Interface/Physics/IRigidBody3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a dynamic rigid body.
         */
        class WPCore_API IRigidDynamic3 : public IRigidBody3
        {
        public:
            /**
             * @brief Destructor for the IRigidDynamic3 class.
             */
            ~IRigidDynamic3() override;

            /**
             * @brief Sets the kinematic target for this dynamic rigid body.
             * @param destination The destination transform.
             */
            virtual void setKinematicTarget( const Transform3<real_Num> &destination ) = 0;

            /**
             * @brief Gets the kinematic target for this dynamic rigid body.
             * @param target The target transform to be filled.
             * @return True if a kinematic target exists for this rigid body, false otherwise.
             */
            virtual bool getKinematicTarget( Transform3<real_Num> &target ) = 0;

            /**
             * @brief Determines if this dynamic rigid body is kinematic.
             * @return True if this rigid body is kinematic, false otherwise.
             */
            virtual bool isKinematic() const = 0;

            /**
             * @brief Sets whether or not this dynamic rigid body is kinematic.
             * @param kinematic True if the body should be kinematic, false otherwise.
             */
            virtual void setKinematic( bool kinematic ) = 0;

            /**
             * @brief Sets the linear damping coefficient for this dynamic rigid body.
             * @param damping The damping coefficient to set.
             */
            virtual void setLinearDamping( real_Num damping ) = 0;

            /**
             * @brief Gets the linear damping coefficient for this dynamic rigid body.
             * @return The linear damping coefficient.
             */
            virtual real_Num getLinearDamping() const = 0;

            /**
             * @brief Sets the angular damping coefficient for this dynamic rigid body.
             * @param damping The damping coefficient to set.
             */
            virtual void setAngularDamping( real_Num damping ) = 0;

            /**
             * @brief Gets the angular damping coefficient for this dynamic rigid body.
             * @return The angular damping coefficient.
             */
            virtual real_Num getAngularDamping() const = 0;

            /**
             * @brief Sets the maximum angular velocity for this dynamic rigid body.
             * @param maxAngVel The maximum angular velocity to set.
             */
            virtual void setMaxAngularVelocity( real_Num maxAngVel ) = 0;

            /**
             * @brief Gets the maximum angular velocity for this dynamic rigid body.
             * @return The maximum angular velocity.
             */
            virtual real_Num getMaxAngularVelocity() const = 0;

            /**
             * @brief Determines if this dynamic rigid body is sleeping.
             * @return True if this rigid body is sleeping, false otherwise.
             */
            virtual bool isSleeping() const = 0;

            /**
             * @brief Sets the threshold for when this dynamic rigid body is put to sleep.
             * @param threshold The sleep threshold to set.
             */
            virtual void setSleepThreshold( real_Num threshold ) = 0;

            /**
             * @brief Gets the threshold for when this dynamic rigid body is put to sleep.
             * @return The sleep threshold.
             */
            virtual real_Num getSleepThreshold() const = 0;

            /**
             * @brief Sets the threshold for when this dynamic rigid body is stabilized.
             * @param threshold The stabilization threshold to set.
             */
            virtual void setStabilizationThreshold( real_Num threshold ) = 0;

            /**
             * @brief Gets the threshold for when this dynamic rigid body is stabilized.
             * @return The stabilization threshold.
             */
            virtual real_Num getStabilizationThreshold() const = 0;

            /**
             * @brief Sets the wake counter for this dynamic rigid body.
             * @param wakeCounterValue The wake counter value to set.
             */
            virtual void setWakeCounter( real_Num wakeCounterValue ) = 0;

            /**
             * @brief Gets the wake counter for this dynamic rigid body.
             * @return The wake counter value.
             */
            virtual real_Num getWakeCounter() const = 0;

            /**
             * @brief Wake up this dynamic rigid body.
             */
            void wakeUp() override = 0;

            /**
             * @brief Put this dynamic rigid body to sleep.
             */
            virtual void putToSleep() = 0;

            /**
             * @brief Set the minimum iteration counts for the solver.
             * @param minPositionIters The minimum position iterations to set.
             * @param minVelocityIters The minimum velocity iterations to set (default is 1).
             */
            virtual void setSolverIterationCounts( u32 minPositionIters, u32 minVelocityIters = 1 ) = 0;

            /**
             * @brief Get the minimum iteration counts for the solver.
             * @param minPositionIters The minimum position iterations.
             * @param minVelocityIters The minimum velocity iterations.
             */
            virtual void getSolverIterationCounts( u32 &minPositionIters,
                                                   u32 &minVelocityIters ) const = 0;

            /**
             * @brief Get the contact report threshold for this dynamic rigid body.
             * @return The contact report threshold.
             */
            virtual real_Num getContactReportThreshold() const = 0;

            /**
             * @brief Set the contact report threshold for this dynamic rigid body.
             * @param threshold The contact report threshold to set.
             */
            virtual void setContactReportThreshold( real_Num threshold ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // IRigidDynamic3_h__
