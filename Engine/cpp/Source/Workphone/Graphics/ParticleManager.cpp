#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/ParticleManager.hpp>
#include <Workphone/Core/HashMap.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Interface/IBuildDirector.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IParticle.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/Math/Math.hpp>
#include <algorithm>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ParticleManager, IParticleManager );

        ParticleManager::ParticleManager() :
            m_updateEnabled( true ),
            m_maxSystems( 1000 ),
            m_cullDistance( 500.0f ),
            m_cullingEnabled( false ),
            m_autoSort( false ),
            m_budgetMS( 10.0f ),
            m_currentUpdateMS( 0.0f )
        {
            // Pre-allocate cache for performance
            m_particleSystemsCache.reserve( 256 );
        }

        ParticleManager::~ParticleManager()
        {
            clear();
        }

        SmartPtr<IParticleSystem> ParticleManager::addParticleSystem( hash32 id )
        {
            // Check if system already exists
            auto it = m_particleSystems.find( id );
            if( it != m_particleSystems.end() )
            {
                return it->second;
            }

            // Check system limit
            if( m_particleSystems.size() >= m_maxSystems )
            {
                // Log warning or handle error
                return nullptr;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return nullptr;
            }

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( !graphicsSystem )
            {
                return nullptr;
            }

            auto graphicsScene = graphicsSystem->getGraphicsScenePtr();
            if( !graphicsScene )
            {
                return nullptr;
            }

            // Create new particle system through scene
            auto system = graphicsScene->addGraphicsObjectByType<IParticleSystem>();
            if( !system )
            {
                return nullptr;
            }

            // Store in both map and cache for different access patterns
            m_particleSystems[id] = system;
            m_particleSystemsCache.push_back( system );

            // Sort cache if auto-sort is enabled
            if( m_autoSort )
            {
                sortSystems();
            }

            return system;
        }

        void ParticleManager::removeParticleSystem( hash32 id )
        {
            auto it = m_particleSystems.find( id );
            if( it != m_particleSystems.end() )
            {
                auto sys = it->second;

                // Remove from map
                m_particleSystems.erase( it );

                // Remove from cache
                m_particleSystemsCache.erase(
                    std::remove( m_particleSystemsCache.begin(), m_particleSystemsCache.end(), sys ),
                    m_particleSystemsCache.end() );

                // Clean up the system
                if( sys )
                {
                    if( auto creator = sys->getCreator() )
                    {
                        creator->removeGraphicsObject( sys );
                    }
                }
            }
        }

        SmartPtr<IParticleSystem> ParticleManager::getParticleSystem( hash32 id ) const
        {
            auto it = m_particleSystems.find( id );
            if( it != m_particleSystems.end() )
                return it->second;
            return nullptr;
        }

        SmartPtr<IParticleSystem> ParticleManager::getParticleSystemByName( const String &name ) const
        {
            for( const auto &sys : m_particleSystemsCache )
            {
                if( sys && sys->getTemplateName() == name )
                    return sys;
            }
            return nullptr;
        }

        Array<SmartPtr<IParticleSystem>> ParticleManager::getParticleSystems() const
        {
            return m_particleSystemsCache;
        }

        void ParticleManager::clear()
        {
            // Remove all systems properly
            for( auto &sys : m_particleSystemsCache )
            {
                if( sys )
                {
                    if( auto creator = sys->getCreator() )
                    {
                        creator->removeGraphicsObject( sys );
                    }
                }
            }

            m_particleSystems.clear();
            m_particleSystemsCache.clear();
        }

        void ParticleManager::pause()
        {
            for( auto &sys : m_particleSystemsCache )
            {
                if( sys )
                    sys->setState( ParticleSystemState::Paused );
            }
        }

        void ParticleManager::resume()
        {
            for( auto &sys : m_particleSystemsCache )
            {
                if( sys )
                    sys->setState( ParticleSystemState::Started );
            }
        }

        void ParticleManager::enable()
        {
            m_updateEnabled = true;
            for( auto &sys : m_particleSystemsCache )
            {
                if( sys )
                    sys->setState( ParticleSystemState::Started );
            }
        }

        void ParticleManager::disable()
        {
            m_updateEnabled = false;
            for( auto &sys : m_particleSystemsCache )
            {
                if( sys )
                    sys->setState( ParticleSystemState::Stopped );
            }
        }

        void ParticleManager::update()
        {
            if( !m_updateEnabled )
            {
                return;
            }

            auto timer = core::IApplicationManager::instancePtr()->getTimer();
            if( !timer )
            {
                return;
            }

            f32 deltaTime = static_cast<f32>( timer->getTime() );
            m_currentUpdateMS = 0.0f;

            // Update systems with budget tracking
            for( auto &sys : m_particleSystemsCache )
            {
                if( !sys )
                {
                    continue;
                }

                // Check if we've exceeded our time budget
                if( m_currentUpdateMS >= m_budgetMS )
                {
                    // Budget exceeded, will continue next frame
                    break;
                }

                // Culling check
                if( m_cullingEnabled )
                {
                    if( shouldCullSystem( sys ) )
                    {
                        continue;
                    }
                }

                // Update the system
                sys->update();

                // Track update time (approximate)
                m_currentUpdateMS += deltaTime * 1000.0f;
            }
        }

        bool ParticleManager::shouldCullSystem( const SmartPtr<IParticleSystem> &system ) const
        {
            if( !system || !m_cullingEnabled )
            {
                return false;
            }

            // Distance-based culling
            if( m_cullDistance > 0.0f )
            {
                // Would calculate distance to camera here
                // For now, return false to always update
                return false;
            }

            // Frustum culling would require system bounds and camera frustum
            return false;
        }

        void ParticleManager::sortSystems()
        {
            if( !m_autoSort )
            {
                return;
            }

            // Sort by priority or distance for proper rendering order
            std::sort(
                m_particleSystemsCache.begin(), m_particleSystemsCache.end(),
                []( const SmartPtr<IParticleSystem> &a, const SmartPtr<IParticleSystem> &b ) -> bool {
                    if( !a || !b )
                        return false;

                    // Sort by template name for now (could be enhanced with priority)
                    return a->getTemplateName() < b->getTemplateName();
                } );
        }

        void ParticleManager::setCullingEnabled( bool enabled )
        {
            m_cullingEnabled = enabled;
        }

        bool ParticleManager::isCullingEnabled() const
        {
            return m_cullingEnabled;
        }

        void ParticleManager::setCullDistance( f32 distance )
        {
            m_cullDistance = Math<f32>::max( 0.0f, distance );
        }

        f32 ParticleManager::getCullDistance() const
        {
            return m_cullDistance;
        }

        void ParticleManager::setMaxSystems( size_t max )
        {
            m_maxSystems = max;
        }

        size_t ParticleManager::getMaxSystems() const
        {
            return m_maxSystems;
        }

        size_t ParticleManager::getNumSystems() const
        {
            return m_particleSystems.size();
        }

        void ParticleManager::setAutoSort( bool autoSort )
        {
            m_autoSort = autoSort;
            if( autoSort )
            {
                sortSystems();
            }
        }

        bool ParticleManager::isAutoSort() const
        {
            return m_autoSort;
        }

        void ParticleManager::setBudgetMS( f32 budget )
        {
            m_budgetMS = Math<f32>::max( 1.0f, budget );
        }

        f32 ParticleManager::getBudgetMS() const
        {
            return m_budgetMS;
        }

        f32 ParticleManager::getCurrentUpdateMS() const
        {
            return m_currentUpdateMS;
        }

        void ParticleManager::setUpdateEnabled( bool enabled )
        {
            m_updateEnabled = enabled;
        }

        bool ParticleManager::isUpdateEnabled() const
        {
            return m_updateEnabled;
        }

        SmartPtr<IParticleSystem> ParticleManager::getSystemByIndex( size_t index ) const
        {
            if( index < m_particleSystemsCache.size() )
            {
                return m_particleSystemsCache[index];
            }
            return nullptr;
        }

        void ParticleManager::preloadSystem( const String &templateName )
        {
            // Preload particle system template/resources
            // This would load the template data for faster instantiation
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return;
            }

            //auto resourceSystem = applicationManager->getResourceSystem();
            //if( !resourceSystem )
            //{
            //    return;
            //}

            // Load template resource
            // Implementation depends on resource system interface
        }

        void ParticleManager::unloadUnusedSystems()
        {
            // Remove systems that haven't been used recently
            // This would track last-access time for each system
            Array<hash32> systemsToRemove;

            for( const auto &pair : m_particleSystems )
            {
                const auto &id = pair.first;
                const auto &system = pair.second;

                if( system && system->getState() == ParticleSystemState::Stopped )
                {
                    // Mark for removal after timeout
                    systemsToRemove.push_back( id );
                }
            }

            // Remove marked systems
            for( auto id : systemsToRemove )
            {
                removeParticleSystem( id );
            }
        }

        void ParticleManager::optimize()
        {
            // Perform optimization tasks:
            // - Remove dead particles from all systems
            // - Consolidate similar systems
            // - Rebuild spatial data structures

            for( auto &sys : m_particleSystemsCache )
            {
                if( sys )
                {
                    // System-specific optimization would go here
                }
            }

            // Trim cache capacity if significantly over-sized
            if( m_particleSystemsCache.capacity() > m_particleSystemsCache.size() * 2 )
            {
                //m_particleSystemsCache.shrink_to_fit();
            }
        }

        Array<SmartPtr<IParticleSystem>> ParticleManager::getSystemsByTemplate(
            const String &templateName ) const
        {
            Array<SmartPtr<IParticleSystem>> result;

            for( const auto &sys : m_particleSystemsCache )
            {
                if( sys && sys->getTemplateName() == templateName )
                {
                    result.push_back( sys );
                }
            }

            return result;
        }

        Array<SmartPtr<IParticleSystem>> ParticleManager::getSystemsByState(
            ParticleSystemState state ) const
        {
            Array<SmartPtr<IParticleSystem>> result;

            for( const auto &sys : m_particleSystemsCache )
            {
                if( sys && sys->getState() == state )
                {
                    result.push_back( sys );
                }
            }

            return result;
        }

        void ParticleManager::stopAll()
        {
            for( auto &sys : m_particleSystemsCache )
            {
                if( sys )
                {
                    sys->setState( ParticleSystemState::Stopped );
                }
            }
        }

        void ParticleManager::startAll()
        {
            for( auto &sys : m_particleSystemsCache )
            {
                if( sys )
                {
                    sys->setState( ParticleSystemState::Started );
                }
            }
        }

        void ParticleManager::resetAll()
        {
            for( auto &sys : m_particleSystemsCache )
            {
                if( sys )
                {
                    sys->setState( ParticleSystemState::Stopped );
                    // Reset system properties to defaults
                }
            }
        }

    }  // namespace render
}  // namespace workphone
