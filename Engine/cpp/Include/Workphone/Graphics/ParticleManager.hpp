#ifndef ParticleManager_h__
#define ParticleManager_h__

#include <Workphone/Interface/Graphics/IParticleManager.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/Core/HashMap.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class ParticleManager
         * @brief Manages multiple particle systems with performance optimization and scene integration.
         *
         * The ParticleManager provides a high-level interface for managing collections of particle
         * systems. It handles system creation, removal, updates, and optimization features such as
         * culling, budgeting, and automatic sorting.
         *
         * Features:
         * - Scene integration with graphics system
         * - Performance budgeting (configurable MS per frame)
         * - Distance-based culling
         * - Auto-sorting for proper rendering order
         * - System queries (by template, state, index)
         * - Bulk operations (pause, resume, enable, disable)
         * - Memory optimization (pre-allocation, cleanup)
         * - Template preloading for faster instantiation
         */
        class WPCore_API ParticleManager : public IParticleManager
        {
        public:
            /**
             * @brief Constructs a new ParticleManager instance.
             */
            ParticleManager();

            /**
             * @brief Destroys the ParticleManager instance.
             */
            ~ParticleManager() override;

            /**
             * @brief Adds a new particle system with the given id.
             * @param id The unique identifier for the particle system.
             * @return A smart pointer to the newly created particle system.
             */
            SmartPtr<IParticleSystem> addParticleSystem( hash32 id ) override;

            /**
             * @brief Removes a particle system by its id.
             * @param id The unique identifier of the particle system to remove.
             */
            void removeParticleSystem( hash32 id ) override;

            /**
             * @brief Gets a particle system by its id.
             * @param id The unique identifier of the particle system.
             * @return A smart pointer to the particle system, or nullptr if not found.
             */
            SmartPtr<IParticleSystem> getParticleSystem( hash32 id ) const override;

            /**
             * @brief Gets a particle system by its name.
             * @param name The name of the particle system.
             * @return A smart pointer to the particle system, or nullptr if not found.
             */
            SmartPtr<IParticleSystem> getParticleSystemByName( const String &name ) const override;

            /**
             * @brief Gets a list of all managed particle systems.
             * @return An array of smart pointers to all particle systems.
             */
            Array<SmartPtr<IParticleSystem>> getParticleSystems() const override;

            /**
             * @brief Removes all particle systems from the manager.
             */
            void clear() override;

            /**
             * @brief Pauses all managed particle systems.
             */
            void pause() override;

            /**
             * @brief Resumes all managed particle systems.
             */
            void resume() override;

            /**
             * @brief Enables all managed particle systems.
             */
            void enable() override;

            /**
             * @brief Disables all managed particle systems.
             */
            void disable() override;

            /**
             * @brief Updates all managed particle systems.
             *
             * This method should be called every frame to advance all particle simulations.
             * It respects performance budgets and culling settings.
             */
            void update();

            // Configuration methods

            /**
             * @brief Enables or disables culling.
             * @param enabled True to enable culling, false to disable.
             */
            void setCullingEnabled( bool enabled );

            /**
             * @brief Checks if culling is enabled.
             * @return True if enabled, false otherwise.
             */
            bool isCullingEnabled() const;

            /**
             * @brief Sets the culling distance threshold.
             * @param distance Distance in world units beyond which systems are culled.
             */
            void setCullDistance( f32 distance );

            /**
             * @brief Gets the culling distance.
             * @return Culling distance threshold.
             */
            f32 getCullDistance() const;

            /**
             * @brief Sets the maximum number of particle systems allowed.
             * @param max Maximum system count.
             */
            void setMaxSystems( size_t max );

            /**
             * @brief Gets the maximum system count.
             * @return Maximum number of systems.
             */
            size_t getMaxSystems() const;

            /**
             * @brief Gets the current number of managed systems.
             * @return Current system count.
             */
            size_t getNumSystems() const;

            /**
             * @brief Enables or disables automatic sorting of systems.
             * @param autoSort True to enable auto-sort, false to disable.
             */
            void setAutoSort( bool autoSort );

            /**
             * @brief Checks if auto-sort is enabled.
             * @return True if enabled, false otherwise.
             */
            bool isAutoSort() const;

            /**
             * @brief Sets the update time budget in milliseconds.
             * @param budget Maximum milliseconds to spend updating per frame.
             */
            void setBudgetMS( f32 budget );

            /**
             * @brief Gets the update budget.
             * @return Budget in milliseconds.
             */
            f32 getBudgetMS() const;

            /**
             * @brief Gets the current frame's update time.
             * @return Time spent updating in current frame (ms).
             */
            f32 getCurrentUpdateMS() const;

            /**
             * @brief Enables or disables manager updates.
             * @param enabled True to enable updates, false to disable.
             */
            void setUpdateEnabled( bool enabled );

            /**
             * @brief Checks if updates are enabled.
             * @return True if enabled, false otherwise.
             */
            bool isUpdateEnabled() const;

            /**
             * @brief Gets a system by its index in the cache.
             * @param index Zero-based index.
             * @return Smart pointer to the system, or nullptr if out of range.
             */
            SmartPtr<IParticleSystem> getSystemByIndex( size_t index ) const;

            /**
             * @brief Preloads a particle system template.
             * @param templateName Name of the template to preload.
             */
            void preloadSystem( const String &templateName );

            /**
             * @brief Unloads unused particle systems.
             *
             * Removes systems that have been stopped and are no longer needed.
             */
            void unloadUnusedSystems();

            /**
             * @brief Optimizes all managed systems.
             *
             * Performs optimization tasks such as removing dead particles,
             * consolidating similar systems, and rebuilding spatial data.
             */
            void optimize();

            /**
             * @brief Gets all systems with a specific template name.
             * @param templateName Template name to search for.
             * @return Array of matching systems.
             */
            Array<SmartPtr<IParticleSystem>> getSystemsByTemplate( const String &templateName ) const;

            /**
             * @brief Gets all systems in a specific state.
             * @param state State to search for.
             * @return Array of matching systems.
             */
            Array<SmartPtr<IParticleSystem>> getSystemsByState( ParticleSystemState state ) const;

            /**
             * @brief Stops all managed particle systems.
             */
            void stopAll();

            /**
             * @brief Starts all managed particle systems.
             */
            void startAll();

            /**
             * @brief Resets all managed particle systems.
             *
             * Stops all systems and resets their properties to defaults.
             */
            void resetAll();

            WP_CLASS_REGISTER_DECL;

        private:
            using ParticleSystems = HashMap<hash32, SmartPtr<IParticleSystem>>;

            /** Map of particle systems by ID. */
            ParticleSystems m_particleSystems;

            /** Cache of particle systems for iteration. */
            Array<SmartPtr<IParticleSystem>> m_particleSystemsCache;

            /** Update enabled flag. */
            bool m_updateEnabled;

            /** Maximum number of systems allowed. */
            size_t m_maxSystems;

            /** Culling enabled flag. */
            bool m_cullingEnabled;

            /** Culling distance threshold. */
            f32 m_cullDistance;

            /** Auto-sort enabled flag. */
            bool m_autoSort;

            /** Update time budget in milliseconds. */
            f32 m_budgetMS;

            /** Current frame's update time in milliseconds. */
            f32 m_currentUpdateMS;

            /**
             * @brief Checks if a system should be culled.
             * @param system The system to check.
             * @return True if the system should be culled.
             */
            bool shouldCullSystem( const SmartPtr<IParticleSystem> &system ) const;

            /**
             * @brief Sorts the system cache.
             */
            void sortSystems();
        };

    }  // namespace render
}  // namespace workphone

#endif  // ParticleManager_h__
