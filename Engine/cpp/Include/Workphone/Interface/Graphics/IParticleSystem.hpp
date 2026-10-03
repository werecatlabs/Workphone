#ifndef _IParticleSystem_H
#define _IParticleSystem_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @enum State
         * @brief Represents the state of the particle system.
         */
        enum class ParticleSystemState
        {
            Stopped = 0,   /**< The particle system is stopped. */
            Started,       /**< The particle system is running. */
            Paused,        /**< The particle system is paused. */
            PausedForTime, /**< The particle system is paused for a specific time. */
            StoppedFade    /**< The particle system is stopped with a fade-out effect. */
        };

        /**
         * @class IParticleSystem
         * @brief Interface for particle systems in a 3D scene.
         *
         * A particle system emits a group of particles that simulate various physical effects such as
         * fire, smoke, sparks, and dust. This interface defines methods to start, stop, pause, and
         * resume the particle system, as well as methods to set and get its properties and techniques.
         * The particle system can be scaled independently from the scene node to which it is attached.
         *
         * @see IGraphicsObject, IParticleTechnique, IParticle
         */
        class WPCore_API IParticleSystem : public IGraphicsObject
        {
        public:
            /**
             * @brief Virtual destructor.
             */
            ~IParticleSystem() override;

            /**
             * @brief Sets the fast forward time and interval for the particle system.
             *
             * Fast forwarding allows the particle system to simulate a period of time instantly, which
             * is useful for skipping the initial buildup of effects (e.g., to make a fire appear already
             * burning).
             *
             * @param time The time to fast forward in seconds.
             * @param interval The interval between updates in seconds.
             */
            virtual void setFastForward( f32 time, f32 interval ) = 0;

            /**
             * @brief Gets the fast forward time for the particle system.
             * @return The fast forward time in seconds.
             */
            virtual f32 getFastForwardTime() const = 0;

            /**
             * @brief Gets the fast forward interval for the particle system.
             * @return The fast forward interval in seconds.
             */
            virtual f32 getFastForwardInterval() const = 0;

            /**
             * @brief Sets the name of the template used as a blueprint for this particle system.
             * @param templateName The name of the template.
             */
            virtual void setTemplateName( const String &templateName ) = 0;

            /**
             * @brief Gets the name of the template used as a blueprint for this particle system.
             * @return The name of the template.
             */
            virtual String getTemplateName() const = 0;

            /**
             * @brief Sets the scale of the particle system, independent from the attached scene node.
             * @param scale The new scale as a 3D vector.
             */
            virtual void setScale( const Vector3<real_Num> &scale ) = 0;

            /**
             * @brief Gets the scale of the particle system.
             * @return The scale as a 3D vector.
             */
            virtual Vector3<real_Num> getScale() const = 0;

            /**
             * @brief Gets the current state of the particle system.
             * @return The state as an unsigned 32-bit integer (see State enum).
             */
            virtual ParticleSystemState getState() const = 0;

            /**
             * @brief Sets the state of the particle system.
             * @param state The new state as an unsigned 32-bit integer (see State enum).
             */
            virtual void setState( ParticleSystemState state ) = 0;

            /**
             * @brief Gets the number of currently active particles.
             *

             * * This is primarily useful for diagnostics, tests, and editor tooling.
             */
            virtual size_t getNumParticles() const = 0;

            /**
             * @brief Gets the number of configured emitters.
             *
             * A
             * code-created particle system must have at least one emitter before
             * entering
             * the started state can produce visible particles.
             */
            virtual size_t getNumEmitters() const = 0;

            /**
             * @brief Gets the number of configured techniques.
             * @return The number of techniques.
             */
            virtual u32 getNumTechniques() const = 0;

            /**
             * @brief Adds a new particle technique to the system.
             *
             * A technique defines how particles are emitted, affected, and rendered.
             *
             * @return A smart pointer to the newly created particle technique.
             */
            virtual SmartPtr<IParticleTechnique> addTechnique() = 0;

            /**
             * @brief Adds a new particle technique with a specific name.
             * @param name The name of the technique.
             * @return A smart pointer to the newly created particle technique.
             */
            virtual SmartPtr<IParticleTechnique> addTechnique( const String &name ) = 0;

            /**
             * @brief Removes a particle technique from the system.
             * @param technique A smart pointer to the technique to remove.
             */
            virtual void removeTechnique( SmartPtr<IParticleTechnique> technique ) = 0;

            /**
             * @brief Gets a particle technique by name.
             * @param name The name of the technique.
             * @return A smart pointer to the particle technique, or nullptr if not found.
             */
            virtual SmartPtr<IParticleTechnique> getTechnique( const String &name ) const = 0;

            /**
             * @brief Adds a new particle to the system.
             *
             * This is typically used internally by techniques or emitters.
             *
             * @return A smart pointer to the newly created particle.
             */
            virtual SmartPtr<IParticle> addParticle() = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace render
}  // namespace workphone

#endif
