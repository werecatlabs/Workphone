//
// Created by Zane Desir on 31/10/2021.
// Updated with Doxygen comments by GitHub Copilot on 21/01/2026
//

#ifndef WP_RIGIDDYNAMIC_H
#define WP_RIGIDDYNAMIC_H

#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <Workphone/Physics/RigidBody3.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief Concrete implementation of a dynamic rigid body with extended runtime controls.
         *
         * RigidDynamic3 extends the generic RigidBody3 functionality by exposing common
         * dynamic-specific controls such as damping, sleeping/wake behavior, kinematic
         * target control, solver iteration tuning and contact reporting thresholds.
         *
         * This class implements the IRigidDynamic3 interface and is intended to be used
         * by physics scenes/managers to control simulation properties for dynamic actors.
         */
        class WPCore_API RigidDynamic3 : public RigidBody3<IRigidDynamic3>
        {
        public:
            /**
             * @brief Construct a new RigidDynamic3 instance.
             *
             * Initializes internal dynamic state to sensible defaults.
             */
            RigidDynamic3();

            /**
             * @brief Virtual destructor.
             *
             * Ensures derived destructors are invoked and resources released.
             */
            ~RigidDynamic3() override;

            /**
             * @brief Set the kinematic target transform for a kinematic actor.
             *
             * For kinematic bodies, this specifies the destination that the actor should
             * move toward during the next simulation step. Non-kinematic actors should
             * ignore this call.
             *
             * @param destination Target transform (position + orientation) in world space.
             */
            void setKinematicTarget( const Transform3<real_Num> &destination ) override;

            /**
             * @brief Retrieve the currently scheduled kinematic target.
             *
             * If a kinematic target is present, it is written into @p target and the
             * function returns true. Returns false if no kinematic target is scheduled
             * or the actor is not kinematic.
             *
             * @param[out] target Receives the active kinematic transform if available.
             * @return true if a kinematic target exists and was written to @p target.
             * @return false otherwise.
             */
            bool getKinematicTarget( Transform3<real_Num> &target ) override;

            /**
             * @brief Set linear damping applied each simulation step.
             *
             * Linear damping reduces linear velocity over time to simulate effects like
             * air resistance. The value should be non-negative; typical values are small
             * (e.g., 0.0 - 0.1).
             *
             * @param damping Linear damping coefficient.
             */
            void setLinearDamping( real_Num damping ) override;

            /**
             * @brief Get the current linear damping coefficient.
             *
             * @return Current linear damping value.
             */
            real_Num getLinearDamping() const override;

            /**
             * @brief Set angular damping applied each simulation step.
             *
             * Angular damping reduces angular velocity over time.
             *
             * @param damping Angular damping coefficient.
             */
            void setAngularDamping( real_Num damping ) override;

            /**
             * @brief Get the current angular damping coefficient.
             *
             * @return Current angular damping value.
             */
            real_Num getAngularDamping() const override;

            /**
             * @brief Limit the maximum angular velocity of the actor.
             *
             * Prevents unstable behaviour caused by extremely high angular speeds by
             * clamping to @p maxAngVel.
             *
             * @param maxAngVel Maximum allowed angular velocity (radians per second).
             */
            void setMaxAngularVelocity( real_Num maxAngVel ) override;

            /**
             * @brief Get the configured maximum angular velocity.
             *
             * @return The maximum allowed angular velocity.
             */
            real_Num getMaxAngularVelocity() const override;

            /**
             * @brief Query whether the actor is currently sleeping (inactive).
             *
             * Sleeping actors are excluded from most simulation processing until they
             * are woken. This helps performance and stability.
             *
             * @return true if the actor is sleeping.
             * @return false if the actor is active.
             */
            bool isSleeping() const override;

            /**
             * @brief Set the sleep threshold for kinetic energy below which the actor may sleep.
             *
             * Lower thresholds make the actor less likely to sleep. Value is typically
             * a small non-negative scalar representing squared velocity or energy depending
             * on the physics backend.
             *
             * @param threshold Sleep threshold value.
             */
            void setSleepThreshold( real_Num threshold ) override;

            /**
             * @brief Get the current sleep threshold.
             *
             * @return Current sleep threshold value.
             */
            real_Num getSleepThreshold() const override;

            /**
             * @brief Set the stabilization threshold used for internal stabilization heuristics.
             *
             * The stabilization threshold is used to stabilize persistent contacts and
             * prevent jitter. Tune this together with solver iterations for best results.
             *
             * @param threshold Stabilization threshold value.
             */
            void setStabilizationThreshold( real_Num threshold ) override;

            /**
             * @brief Get the current stabilization threshold.
             *
             * @return Current stabilization threshold value.
             */
            real_Num getStabilizationThreshold() const override;

            /**
             * @brief Set the wake counter for the actor.
             *
             * The wake counter is typically a time-like value that keeps the actor awake
             * for a given period after external activity. Setting this to a positive
             * value will keep the actor active for that many seconds/steps depending on the backend.
             *
             * @param wakeCounterValue New wake counter value.
             */
            void setWakeCounter( real_Num wakeCounterValue ) override;

            /**
             * @brief Get the current wake counter value.
             *
             * @return The wake counter.
             */
            real_Num getWakeCounter() const override;

            /**
             * @brief Explicitly wake this actor so it becomes active in the simulation.
             *
             * Calling wakeUp forces the actor out of sleep and allows it to participate
             * in simulation immediately.
             */
            void wakeUp() override;

            /**
             * @brief Force the actor to sleep (become inactive) immediately.
             *
             * Use with care: sleeping actors will not respond until woken.
             */
            void putToSleep() override;

            /**
             * @brief Set solver iteration counts for position and velocity constraints.
             *
             * Increasing iterations can improve stability and convergence at the cost
             * of CPU. Typical defaults: position iters >= velocity iters >= 1.
             *
             * @param minPositionIters Minimum solver iterations for position constraints.
             * @param minVelocityIters Minimum solver iterations for velocity constraints (default 1).
             */
            void setSolverIterationCounts( u32 minPositionIters, u32 minVelocityIters = 1 ) override;

            /**
             * @brief Retrieve the solver iteration counts.
             *
             * @param[out] minPositionIters Receives the minimum position iterations.
             * @param[out] minVelocityIters Receives the minimum velocity iterations.
             */
            void getSolverIterationCounts( u32 &minPositionIters, u32 &minVelocityIters ) const override;

            /**
             * @brief Get the contact report threshold used to trigger contact callbacks.
             *
             * Contacts with relative velocity or separation below this threshold may be
             * filtered from reporting depending on the backend.
             *
             * @return The contact report threshold value.
             */
            real_Num getContactReportThreshold() const override;

            /**
             * @brief Set the contact report threshold.
             *
             * Use this to control when contact events are generated for this actor.
             *
             * @param threshold New contact report threshold.
             */
            void setContactReportThreshold( real_Num threshold ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Internal flags storing runtime state for the dynamic actor.
             *
             * This bitfield is used to cache or track dynamic-specific boolean states
             * (for example: kinematic/awake flags, custom behaviour toggles, etc.)
             * Interpretation of individual bits is implementation-defined.
             */
            u32 m_rigidDynamicflags = 0;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // WP_RIGIDDYNAMIC_H
