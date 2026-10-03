#ifndef ParticleTechnique_h__
#define ParticleTechnique_h__

#include <Workphone/Interface/Graphics/IParticleTechnique.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/Interface/Graphics/IParticleEmitter.hpp>
#include <Workphone/Interface/Graphics/IParticleAffector.hpp>
#include <Workphone/Interface/Graphics/IParticleRenderer.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class ParticleTechnique
         * @brief Implements a particle technique, managing emitters, affectors, renderers, and
         * particles.
         *
         * A particle technique is responsible for managing the lifecycle and behavior of a group of
         * particles, including their emitters, affectors, and renderers. It provides methods to add,
         * remove, and access these components, as well as to update the technique each frame.
         *
         * Features:
         * - LOD (Level of Detail) support with distance-based skipping
         * - Particle pooling for performance optimization
         * - Multiple culling modes (distance, frustum)
         * - Particle sorting (by distance or age)
         * - Simulation space selection (world or local)
         * - Max particle limits with automatic cleanup
         * - Named accessors for components
         */
        class WPCore_API ParticleTechnique : public IParticleTechnique
        {
        public:
            /**
             * @enum CullMode
             * @brief Defines the culling mode for the technique.
             */
            enum class CullMode
            {
                None = 0,  ///< No culling, always update
                Distance,  ///< Cull based on distance to camera
                Frustum    ///< Cull based on camera frustum
            };

            /**
             * @enum SortMode
             * @brief Defines the particle sorting mode.
             */
            enum class SortMode
            {
                None = 0,  ///< No sorting
                Distance,  ///< Sort by distance to camera (far to near)
                Age        ///< Sort by age (oldest first)
            };

            /**
             * @enum SimulationSpace
             * @brief Defines the coordinate space for particle simulation.
             */
            enum class SimulationSpace
            {
                World = 0,  ///< Simulate in world space
                Local       ///< Simulate in local space (relative to emitter)
            };

            /**
             * @brief Constructs a new ParticleTechnique instance.
             */
            ParticleTechnique();

            /**
             * @brief Destroys the ParticleTechnique instance.
             */
            ~ParticleTechnique() override;

            /**
             * @brief Updates the particle technique, processing all emitters, affectors, and particles.
             *
             * This method should be called every frame to advance the particle simulation.
             * It handles emitter updates, affector applications, particle lifetime management,
             * and cleanup of dead particles.
             */
            void update() override;

            /**
             * @brief Adds a new unnamed particle emitter to the technique.
             * @return A smart pointer to the created particle emitter.
             */
            SmartPtr<IParticleEmitter> addEmitter() override;

            /**
             * @brief Adds a new named particle emitter to the technique.
             * @param name The name of the emitter.
             * @return A smart pointer to the created particle emitter.
             */
            SmartPtr<IParticleEmitter> addEmitter( const String &name ) override;

            /**
             * @brief Gets the particle system that owns this technique.
             * @return A smart pointer to the owning particle system.
             */
            SmartPtr<IParticleSystem> getParticleSystem() const override;

            /**
             * @brief Sets the particle system that owns this technique.
             * @param particleSystem A smart pointer to the particle system.
             */
            void setParticleSystem( SmartPtr<IParticleSystem> particleSystem ) override;

            /**
             * @brief Removes a particle emitter from the technique.
             * @param emitter A smart pointer to the emitter to remove.
             */
            void removeEmitter( SmartPtr<IParticleEmitter> emitter ) override;

            /**
             * @brief Gets all particle emitters managed by this technique.
             * @return An array of smart pointers to the emitters.
             */
            Array<SmartPtr<IParticleEmitter>> getParticleEmitters() const override;

            /**
             * @brief Adds a new particle affector to the technique.
             * @param id The identifier for the affector type.
             * @return A smart pointer to the created particle affector.
             */
            SmartPtr<IParticleAffector> addAffector( u32 id ) override;

            /**
             * @brief Removes a particle affector from the technique.
             * @param affector A smart pointer to the affector to remove.
             */
            void removeAffector( SmartPtr<IParticleAffector> affector ) override;

            /**
             * @brief Gets all particle affectors managed by this technique.
             * @return An array of smart pointers to the affectors.
             */
            Array<SmartPtr<IParticleAffector>> getParticleAffectors() const override;

            /**
             * @brief Adds a particle renderer to the technique.
             * @param renderer A smart pointer to the renderer to add.
             */
            void addRenderer( SmartPtr<IParticleRenderer> renderer ) override;

            /**
             * @brief Removes a particle renderer from the technique.
             * @param renderer A smart pointer to the renderer to remove.
             */
            void removeRenderer( SmartPtr<IParticleRenderer> renderer ) override;

            /**
             * @brief Gets all particle renderers managed by this technique.
             * @return An array of smart pointers to the renderers.
             */
            Array<SmartPtr<IParticleRenderer>> getParticleRenderers() const override;

            /**
             * @brief Gets a particle emitter by its hash identifier.
             * @param hash The hash identifier of the emitter.
             * @return A smart pointer to the emitter, or nullptr if not found.
             */
            SmartPtr<IParticleEmitter> getEmitter( hash32 hash ) const override;

            /**
             * @brief Gets a particle affector by its hash identifier.
             * @param hash The hash identifier of the affector.
             * @return A smart pointer to the affector, or nullptr if not found.
             */
            SmartPtr<IParticleAffector> getAffector( hash32 hash ) const override;

            /**
             * @brief Gets a particle renderer by its hash identifier.
             * @param hash The hash identifier of the renderer.
             * @return A smart pointer to the renderer, or nullptr if not found.
             */
            SmartPtr<IParticleRenderer> getRenderer( hash32 hash ) const override;

            /**
             * @brief Gets a particle emitter by its name.
             * @param name The name of the emitter.
             * @return A smart pointer to the emitter, or nullptr if not found.
             */
            SmartPtr<IParticleEmitter> getEmitterByName( const String &name ) const override;

            /**
             * @brief Gets a particle affector by its name.
             * @param name The name of the affector.
             * @return A smart pointer to the affector, or nullptr if not found.
             */
            SmartPtr<IParticleAffector> getAffectorByName( const String &name ) const override;

            /**
             * @brief Gets a particle renderer by its name.
             * @param name The name of the renderer.
             * @return A smart pointer to the renderer, or nullptr if not found.
             */
            SmartPtr<IParticleRenderer> getRendererByName( const String &name ) const override;

            /**
             * @brief Gets all particles managed by this technique.
             * @return An array of smart pointers to the particles.
             */
            Array<SmartPtr<IParticle>> getParticles() const override;

            /**
             * @brief Sets the particles managed by this technique.
             * @param particles An array of smart pointers to the particles.
             */
            void setParticles( const Array<SmartPtr<IParticle>> &particles ) override;

            /**
             * @brief Adds a particle to the technique.
             * @param particle A smart pointer to the particle to add.
             */
            void addParticle( SmartPtr<IParticle> particle ) override;

            /**
             * @brief Removes a particle from the technique.
             * @param particle A smart pointer to the particle to remove.
             */
            void removeParticle( SmartPtr<IParticle> particle ) override;

            /**
             * @brief Removes all particles from the technique.
             */
            void clearParticles() override;

            // Additional configuration methods

            /**
             * @brief Sets the maximum number of particles allowed in this technique.
             * @param max Maximum particle count.
             */
            void setMaxParticles( size_t max );

            /**
             * @brief Gets the maximum particle count.
             * @return Maximum number of particles.
             */
            size_t getMaxParticles() const;

            /**
             * @brief Enables or disables LOD (Level of Detail) for this technique.
             * @param enabled True to enable LOD, false to disable.
             */
            void setLODEnabled( bool enabled );

            /**
             * @brief Checks if LOD is enabled.
             * @return True if enabled, false otherwise.
             */
            bool isLODEnabled() const;

            /**
             * @brief Sets the LOD distance threshold.
             * @param distance Distance in world units beyond which LOD is applied.
             */
            void setLODDistance( f32 distance );

            /**
             * @brief Gets the LOD distance.
             * @return LOD distance threshold.
             */
            f32 getLODDistance() const;

            /**
             * @brief Enables or disables technique updates.
             * @param enabled True to enable updates, false to disable.
             */
            void setUpdateEnabled( bool enabled );

            /**
             * @brief Checks if updates are enabled.
             * @return True if enabled, false otherwise.
             */
            bool isUpdateEnabled() const;

            /**
             * @brief Sets the culling mode.
             * @param mode The culling mode to use.
             */
            void setCullMode( CullMode mode );

            /**
             * @brief Gets the culling mode.
             * @return Current culling mode.
             */
            CullMode getCullMode() const;

            /**
             * @brief Sets the culling distance.
             * @param distance Distance in world units for distance culling.
             */
            void setCullDistance( f32 distance );

            /**
             * @brief Gets the culling distance.
             * @return Culling distance threshold.
             */
            f32 getCullDistance() const;

            /**
             * @brief Sets the particle pool size for pre-allocation.
             * @param size Number of particles to pre-allocate.
             */
            void setParticlePoolSize( size_t size );

            /**
             * @brief Gets the particle pool size.
             * @return Pre-allocated pool size.
             */
            size_t getParticlePoolSize() const;

            /**
             * @brief Sets the particle sorting mode.
             * @param mode The sorting mode to use.
             */
            void setSortMode( SortMode mode );

            /**
             * @brief Gets the sorting mode.
             * @return Current sorting mode.
             */
            SortMode getSortMode() const;

            /**
             * @brief Sets the simulation space.
             * @param space The simulation space (world or local).
             */
            void setSimulationSpace( SimulationSpace space );

            /**
             * @brief Gets the simulation space.
             * @return Current simulation space.
             */
            SimulationSpace getSimulationSpace() const;

            /**
             * @brief Gets the number of active particles.
             * @return Current particle count.
             */
            size_t getNumActiveParticles() const;

            /**
             * @brief Gets the number of emitters.
             * @return Emitter count.
             */
            size_t getNumEmitters() const;

            /**
             * @brief Gets the number of affectors.
             * @return Affector count.
             */
            size_t getNumAffectors() const;

            /**
             * @brief Gets the number of renderers.
             * @return Renderer count.
             */
            size_t getNumRenderers() const;

            /**
             * @brief Resets the technique to initial state.
             *
             * Clears all particles and resets emitter states.
             */
            void reset();

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Weak pointer to the owning particle system. */
            WeakPtr<IParticleSystem> m_particleSystem;

            /** Array of particle affectors. */
            Array<SmartPtr<IParticleAffector>> m_affectors;

            /** Array of particle emitters. */
            Array<SmartPtr<IParticleEmitter>> m_emitters;

            /** Array of particle renderers. */
            Array<SmartPtr<IParticleRenderer>> m_renderers;

            /** Array of particles managed by this technique. */
            Array<SmartPtr<IParticle>> m_particles;

            /** Maximum number of particles allowed. */
            size_t m_maxParticles;

            /** LOD distance threshold. */
            f32 m_lodDistance;

            /** LOD enabled flag. */
            bool m_lodEnabled;

            /** Update enabled flag. */
            bool m_updateEnabled;

            /** Culling mode. */
            CullMode m_cullMode;

            /** Culling distance threshold. */
            f32 m_cullDistance;

            /** Pre-allocated particle pool size. */
            size_t m_particlePoolSize;

            /** Particle sorting mode. */
            SortMode m_sortMode;

            /** Simulation space. */
            SimulationSpace m_simulationSpace;

            /**
             * @brief Updates particle states (lifetime, etc.).
             * @param deltaTime Time elapsed since last update.
             */
            void updateParticles( f32 deltaTime );

            /**
             * @brief Removes dead particles from the technique.
             */
            void cleanupDeadParticles();

            /**
             * @brief Checks if the technique should skip updates (LOD/culling).
             * @return True if updates should be skipped.
             */
            bool shouldSkipUpdate() const;

            /**
             * @brief Sorts particles according to the sort mode.
             */
            void sortParticles();
        };
    }  // namespace render
}  // namespace workphone

#endif  // ParticleTechnique_h__
