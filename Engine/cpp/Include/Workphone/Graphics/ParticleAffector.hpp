#ifndef ParticleAffector_h__
#define ParticleAffector_h__

#include <Workphone/Interface/Graphics/IParticleAffector.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/Interface/Graphics/IParticleTechnique.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class ParticleAffector
         * @brief Base class for particle affectors that modify particle behavior over time.
         *
         * Particle affectors are responsible for applying forces, transformations, and other
         * modifications to particles in a particle system. This base class provides the core
         * functionality including node hierarchy support, position tracking, and the interface
         * for calculating particle state.
         *
         * Derived classes can implement specific affector behaviors such as:
         * - Gravity: Apply constant downward force
         * - Wind: Apply directional force
         * - Color: Interpolate particle color over time
         * - Scale: Scale particle over lifetime
         * - Rotation: Rotate particle over time
         * - Attraction: Pull particles toward a point
         * - Repulsion: Push particles away from a point
         * - Vortex: Create swirling motion
         * - Drag: Apply velocity damping
         * - Randomizer: Add random variations
         */
        class WPCore_API ParticleAffector : public IParticleAffector
        {
        public:
            /**
             * @brief Constructs a new ParticleAffector instance.
             */
            ParticleAffector();

            /**
             * @brief Destroys the ParticleAffector instance.
             */
            ~ParticleAffector() override;

            /**
             * @brief Calculates the state of a particle using the given index.
             *
             * This method is called for each particle in the system to apply the affector's
             * influence. Derived classes should override this method to implement specific
             * affector behaviors.
             *
             * @param particle The particle whose state to calculate.
             * @param stateIndex The index of the state to calculate.
             * @param data Optional data to use in the calculation (affector-specific).
             */
            void calculateState( SmartPtr<IParticle> &particle, u32 stateIndex,
                                 void *data = nullptr ) override;

            // IParticleNode implementation
            void addChild( SmartPtr<IParticleNode> child ) override;
            void addChild( SmartPtr<IParticleNode> child, s32 index ) override;
            void removeChild( SmartPtr<IParticleNode> child ) override;
            void remove() override;
            u32 getNumChildren() const override;
            SmartPtr<IParticleNode> getChildByIndex( u32 index ) const override;
            SmartPtr<IParticleNode> getChildById( hash32 id ) const override;
            Array<SmartPtr<IParticleNode>> getChildren() const override;
            SmartPtr<IParticleNode> getParent() const override;
            void setParent( SmartPtr<IParticleNode> parent ) override;
            void setPosition( const Vector3<real_Num> &position ) override;
            Vector3<real_Num> getPosition() const override;
            Vector3<real_Num> getAbsolutePosition() const override;
            SmartPtr<IParticleSystem> getParticleSystem() const override;
            void setParticleSystem( SmartPtr<IParticleSystem> particleSystem ) override;

            /**
             * @brief Gets the technique that owns this affector.
             * @return A smart pointer to the owning technique, or nullptr if not assigned.
             */
            SmartPtr<IParticleTechnique> getTechnique() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Array of child nodes. */
            Array<SmartPtr<IParticleNode>> m_children;

            /** Weak pointer to parent node. */
            WeakPtr<IParticleNode> m_parent;

            /** Local position of this affector. */
            Vector3<real_Num> m_position;

            /** Weak pointer to the owning particle system. */
            WeakPtr<IParticleSystem> m_particleSystem;
        };

    }  // namespace render
}  // namespace workphone

#endif  // ParticleAffector_h__
