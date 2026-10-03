#ifndef PhysicsMaterial2_h__
#define PhysicsMaterial2_h__

#include <WPPhysics/WPPhysicsPrerequisites.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial2.hpp>
#include <Workphone/Interface/Script/IScriptReceiver.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone::physics
{

    /**
     * @class PhysicsMaterial2
     * @brief Implementation of a 2D physics material, handling friction, restitution, and contact
     * properties.
     */
    class PhysicsMaterial2 : public IPhysicsMaterial2
    {
    public:
        /**
         * @class ScriptReceiver
         * @brief Provides a scriptable interface to modify PhysicsMaterial2 properties.
         */
        class ScriptReceiver : public IScriptReceiver
        {
        public:
            ScriptReceiver( PhysicsMaterial2 *material );

            s32 setProperty( hash_type id, const Parameter &param ) override;

            s32 setProperty( hash_type id, const Parameters &params ) override;

            s32 setProperty( hash_type hash, void *param ) override;

            s32 getProperty( hash_type id, Parameter &param ) const override;

            s32 getProperty( hash_type id, Parameters &params ) const override;

            s32 getProperty( hash_type hash, void *param ) const override;

        private:
            PhysicsMaterial2 *m_material;
        };

        PhysicsMaterial2();

        ~PhysicsMaterial2() override;

        void setFriction( real_Num friction, s32 direction ) override;

        real_Num getFriction( s32 direction ) const override;

        void setRestitution( real_Num restitution ) override;

        real_Num getRestitution() const override;

        void setContactPosition( const Vector2<real_Num> &position ) override;

        Vector2<real_Num> getContactPosition() const override;

        void setContactNormal( const Vector2<real_Num> &normal ) override;

        Vector2<real_Num> getContactNormal() const override;

        void setPhysicsBodyA( IPhysicsBody2D *body );

        void setPhysicsBodyA( SmartPtr<IPhysicsBody2D> body ) override;

        SmartPtr<IPhysicsBody2D> getPhysicsBodyA() const override;

        void setPhysicsBodyB( SmartPtr<IPhysicsBody2D> body ) override;

        SmartPtr<IPhysicsBody2D> getPhysicsBodyB() const override;

        unsigned int m_collisionID;

        // Impulse base solver parameters
        real_Num m_elasticCoef;      ///< Elasticity coefficient for impulse solver.
        real_Num m_static_friction;  ///< Static friction coefficient.
        real_Num m_dynamic_friction; ///< Dynamic friction coefficient.

        // ODE solver parameters
        int      mode; ///< ODE contact mode flags.
        real_Num mu;   ///< Coulomb friction coefficient (0 to dInfinity). 0 is frictionless, dInfinity
                       ///< never slips.
        real_Num mu2; ///< Optional Coulomb friction coefficient for friction direction 2 (0..dInfinity).
        real_Num bounce;     ///< Restitution parameter (0..1). 0 is non-bouncy, 1 is maximum bouncyness.
        real_Num bounce_vel; ///< Minimum incoming velocity required for bounce.
        real_Num soft_erp;   ///< Contact normal "softness" Error Reduction Parameter (ERP).
        real_Num soft_cfm;   ///< Contact normal "softness" Constraint Force Mixing (CFM).
        real_Num motion1;    ///< Surface velocity in friction direction 1.
        real_Num motion2;    ///< Surface velocity in friction direction 2.
        real_Num slip1;      ///< Force-dependent-slip (FDS) coefficient for friction direction 1.
        real_Num slip2;      ///< Force-dependent-slip (FDS) coefficient for friction direction 2.

    protected:
        SmartPtr<IPhysicsBody2D> m_bodyA; ///< Reference to the first physics body in contact.
        SmartPtr<IPhysicsBody2D> m_bodyB; ///< Reference to the second physics body in contact.

        Vector2<real_Num> m_normal;   ///< Contact normal vector.
        Vector2<real_Num> m_position; ///< Contact point position.

        real_Num        m_restitution; ///< Material restitution (bounciness).
        Array<real_Num> m_friction;    ///< Friction coefficients per direction.
    };
} // namespace workphone::physics

// end namespace

#endif // PhysicsMaterial2_h__
