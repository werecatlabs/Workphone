#ifndef IParticleTechnique_h__
#define IParticleTechnique_h__

#include <Workphone/Interface/Graphics/IParticleNode.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class IParticleTechnique
         * @brief Interface for a particle technique, which manages emitters, affectors, renderers, and
         * particles.
         *
         * A particle technique is responsible for organizing and controlling the various components
         * (emitters, affectors, renderers, and particles) that make up a particle system. It provides
         * methods to add, remove, and retrieve these components, as well as to manage the collection of
         * particles.
         */
        class WPCore_API IParticleTechnique : public IParticleNode
        {
        public:
            /**
             * @brief Virtual destructor for safe cleanup of derived classes.
             */
            ~IParticleTechnique() override;

            /**
             * @brief Adds a new unnamed particle emitter to the technique.
             * @return A smart pointer to the newly created IParticleEmitter instance.
             */
            virtual SmartPtr<IParticleEmitter> addEmitter() = 0;

            /**
             * @brief Adds a new particle emitter with the specified name.
             * @param name The name to assign to the emitter.
             * @return A smart pointer to the newly created IParticleEmitter instance.
             */
            virtual SmartPtr<IParticleEmitter> addEmitter( const String &name ) = 0;

            /**
             * @brief Removes the specified emitter from the technique.
             * @param emitter The emitter to remove.
             */
            virtual void removeEmitter( SmartPtr<IParticleEmitter> emitter ) = 0;

            /**
             * @brief Gets all particle emitters managed by this technique.
             * @return An array of smart pointers to IParticleEmitter instances.
             */
            virtual Array<SmartPtr<IParticleEmitter>> getParticleEmitters() const = 0;

            /**
             * @brief Adds a new particle affector with the specified ID.
             * @param id The unique identifier for the affector.
             * @return A smart pointer to the newly created IParticleAffector instance.
             */
            virtual SmartPtr<IParticleAffector> addAffector( u32 id ) = 0;

            /**
             * @brief Removes the specified affector from the technique.
             * @param affector The affector to remove.
             */
            virtual void removeAffector( SmartPtr<IParticleAffector> affector ) = 0;

            /**
             * @brief Gets all particle affectors managed by this technique.
             * @return An array of smart pointers to IParticleAffector instances.
             */
            virtual Array<SmartPtr<IParticleAffector>> getParticleAffectors() const = 0;

            /**
             * @brief Adds a particle renderer to the technique.
             * @param renderer The renderer to add.
             */
            virtual void addRenderer( SmartPtr<IParticleRenderer> renderer ) = 0;

            /**
             * @brief Removes the specified renderer from the technique.
             * @param renderer The renderer to remove.
             */
            virtual void removeRenderer( SmartPtr<IParticleRenderer> renderer ) = 0;

            /**
             * @brief Gets all particle renderers managed by this technique.
             * @return An array of smart pointers to IParticleRenderer instances.
             */
            virtual Array<SmartPtr<IParticleRenderer>> getParticleRenderers() const = 0;

            /**
             * @brief Retrieves an emitter by its hash value.
             * @param hash The hash value identifying the emitter.
             * @return A smart pointer to the IParticleEmitter instance, or nullptr if not found.
             */
            virtual SmartPtr<IParticleEmitter> getEmitter( hash32 hash ) const = 0;

            /**
             * @brief Retrieves an affector by its hash value.
             * @param hash The hash value identifying the affector.
             * @return A smart pointer to the IParticleAffector instance, or nullptr if not found.
             */
            virtual SmartPtr<IParticleAffector> getAffector( hash32 hash ) const = 0;

            /**
             * @brief Retrieves a renderer by its hash value.
             * @param hash The hash value identifying the renderer.
             * @return A smart pointer to the IParticleRenderer instance, or nullptr if not found.
             */
            virtual SmartPtr<IParticleRenderer> getRenderer( hash32 hash ) const = 0;

            /**
             * @brief Retrieves an emitter by its name.
             * @param name The name of the emitter.
             * @return A smart pointer to the IParticleEmitter instance, or nullptr if not found.
             */
            virtual SmartPtr<IParticleEmitter> getEmitterByName( const String &name ) const = 0;

            /**
             * @brief Retrieves an affector by its name.
             * @param name The name of the affector.
             * @return A smart pointer to the IParticleAffector instance, or nullptr if not found.
             */
            virtual SmartPtr<IParticleAffector> getAffectorByName( const String &name ) const = 0;

            /**
             * @brief Retrieves a renderer by its name.
             * @param name The name of the renderer.
             * @return A smart pointer to the IParticleRenderer instance, or nullptr if not found.
             */
            virtual SmartPtr<IParticleRenderer> getRendererByName( const String &name ) const = 0;

            /**
             * @brief Gets all particles managed by this technique.
             * @return An array of smart pointers to IParticle instances.
             */
            virtual Array<SmartPtr<IParticle>> getParticles() const = 0;

            /**
             * @brief Sets the collection of particles for this technique.
             * @param particles The array of particles to set.
             */
            virtual void setParticles( const Array<SmartPtr<IParticle>> &particles ) = 0;

            /**
             * @brief Adds a particle to the technique.
             * @param particle The particle to add.
             */
            virtual void addParticle( SmartPtr<IParticle> particle ) = 0;

            /**
             * @brief Removes a particle from the technique.
             * @param particle The particle to remove.
             */
            virtual void removeParticle( SmartPtr<IParticle> particle ) = 0;

            /**
             * @brief Removes all particles from the technique.
             */
            virtual void clearParticles() = 0;

            /**
             * @brief Registers the class for reflection or serialization.
             */
            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace render
}  // namespace workphone

#endif  // IParticleTechnique_h__
