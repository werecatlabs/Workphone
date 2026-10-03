#ifndef ConstraintLimit_h__
#define ConstraintLimit_h__

#include <Workphone/Interface/Physics/IConstraintLimit.hpp>
#include <Workphone/Physics/PhysicsShape3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Concrete implementation of IConstraintLimit describing limit behaviour for constraints.
         *
         * This class stores tunable parameters that control how a constraint limit
         * responds when reached (for example in joint limits). Parameters include
         * restitution (bounciness), bounce threshold (minimum relative velocity
         * required to produce a bounce), stiffness and damping (spring-like response),
         * and a contact distance used to trigger the limit before exact penetration.
         *
         * All numerical values use the project's numeric type `real_Num`.
         */
        class WPCore_API ConstraintLimit : public IConstraintLimit
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes all parameters to zero (no bounce, no stiffness/damping,
             * zero contact distance).
             */
            ConstraintLimit();

            /**
             * @brief Destructor.
             */
            ~ConstraintLimit();

            /**
             * @brief Get the coefficient of restitution for the constraint limit.
             *
             * The restitution value controls how "bouncy" the collision response is
             * when the limit is reached. Typical values range from 0 (perfectly
             * inelastic) to 1 (perfectly elastic). Values outside [0,1] may be
             * supported by the underlying physics engine but can produce non-physical
             * response.
             *
             * @return restitution coefficient as real_Num.
             */
            real_Num getRestitution() const override;

            /**
             * @brief Set the coefficient of restitution for the constraint limit.
             *
             * @param restitution Coefficient of restitution. Recommended range: [0, 1].
             *                    Values outside this range may produce unexpected behaviour.
             */
            void setRestitution( real_Num restitution ) override;

            /**
             * @brief Get the bounce threshold velocity.
             *
             * When the relative impact velocity at the limit is below this threshold,
             * the engine will generally not apply restitution (bounce). This prevents
             * tiny oscillations from numerical noise.
             *
             * @return bounce threshold velocity as real_Num.
             */
            real_Num getBounceThreshold() const override;

            /**
             * @brief Set the bounce threshold velocity.
             *
             * @param bounceThreshold Minimum relative velocity required to trigger a bounce.
             *                        Units match the project's linear velocity units.
             */
            void setBounceThreshold( real_Num bounceThreshold ) override;

            /**
             * @brief Get the stiffness used for enforcing the limit (spring strength).
             *
             * Stiffness determines how strongly the constraint resists penetration when
             * the limit is exceeded. Larger values make the response harder (closer to
             * a rigid stop); smaller values make it softer.
             *
             * @return stiffness as real_Num.
             */
            real_Num getStiffness() const override;

            /**
             * @brief Set the stiffness used for enforcing the limit.
             *
             * @param stiffness Spring-like stiffness coefficient. Interpretation and
             *                  valid ranges depend on the physics implementation.
             */
            void setStiffness( real_Num stiffness ) override;

            /**
             * @brief Get the damping applied to the limit response.
             *
             * Damping dissipates energy from the limit response to reduce oscillation.
             * Typical values are non-negative; very large values can overdamp the response.
             *
             * @return damping coefficient as real_Num.
             */
            real_Num getDamping() const override;

            /**
             * @brief Set the damping applied to the limit response.
             *
             * @param damping Damping coefficient. Units and effect depend on the
             *                physics backend; non-negative values are recommended.
             */
            void setDamping( real_Num damping ) override;

            /**
             * @brief Get the contact distance for the constraint limit.
             *
             * Contact distance defines a pre-contact region: the limit can be treated
             * as active when the constrained body is within this distance from the
             * limit plane/angle. This is useful to apply corrective forces before
             * full penetration occurs.
             *
             * @return contact distance as real_Num.
             */
            real_Num getContactDistance() const override;

            /**
             * @brief Set the contact distance for the constraint limit.
             *
             * @param contactDistance Distance (in the project's linear units) used to
             *                        activate the limit before full penetration.
             */
            void setContactDistance( real_Num contactDistance ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Coefficient of restitution (bounciness). Default = 0.0. */
            real_Num m_restitution = real_Num( 0.0 );

            /**
             * @brief Bounce threshold - the minimum impact velocity required for bounce.
             *
             * Small velocities below this threshold will not produce restitution to
             * avoid jitter from numerical noise. Default = 0.0.
             */
            real_Num m_bounceThreshold = real_Num( 0.0 );

            /**
             * @brief Stiffness of the constraint limit (spring strength).
             *
             * Controls how strongly the limit resists penetration. Default = 0.0.
             */
            real_Num m_stiffness = real_Num( 0.0 );

            /**
             * @brief Damping applied to limit response to reduce oscillations.
             *
             * Default = 0.0.
             */
            real_Num m_damping = real_Num( 0.0 );

            /**
             * @brief Contact distance used to trigger limit response before full penetration.
             *
             * Default = 0.0.
             */
            real_Num m_contactDistance = real_Num( 0.0 );
        };

    }  // namespace physics
}  // namespace workphone

#endif  // ConstraintLimit_h__
