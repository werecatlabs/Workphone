#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/ParticleTechnique.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IParticle.hpp>
#include <Workphone/Interface/Graphics/IParticleAffector.hpp>
#include <Workphone/Interface/Graphics/IParticleEmitter.hpp>
#include <Workphone/Interface/Graphics/IParticleRenderer.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/Math/Math.hpp>
#include <algorithm>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, ParticleTechnique, IParticleTechnique );

    ParticleTechnique::ParticleTechnique() :
        m_maxParticles( 10000 ),
        m_lodDistance( 0.0f ),
        m_lodEnabled( false ),
        m_updateEnabled( true ),
        m_cullMode( CullMode::None ),
        m_cullDistance( 1000.0f ),
        m_particlePoolSize( 1000 ),
        m_sortMode( SortMode::None ),
        m_simulationSpace( SimulationSpace::World )
    {
        // Pre-allocate particle pool for better performance
        m_particles.reserve( m_particlePoolSize );
    }

    ParticleTechnique::~ParticleTechnique()
    {
        clearParticles();
        m_emitters.clear();
        m_affectors.clear();
        m_renderers.clear();
    }

    void ParticleTechnique::update()
    {
        if( !m_updateEnabled )
        {
            return;
        }

        auto particleSystem = getParticleSystem();
        if( !particleSystem )
        {
            return;
        }

        // Check LOD and culling
        if( shouldSkipUpdate() )
        {
            return;
        }

        f32 deltaTime =
            static_cast<f32>( core::IApplicationManager::instancePtr()->getTimer()->getTime() );

        // Update all emitters first to generate new particles
        for( auto &emitter : m_emitters )
        {
            if( emitter )
            {
                emitter->update();
            }
        }

        // Apply affectors to all particles
        for( auto &particle : m_particles )
        {
            if( !particle )
            {
                continue;
            }

            // Apply each affector in sequence
            for( auto &affector : m_affectors )
            {
                if( affector )
                {
                    affector->calculateState( particle, 0, nullptr );
                }
            }
        }

        // Update particle states (lifetime, etc.)
        updateParticles( deltaTime );

        // Remove dead particles
        cleanupDeadParticles();

        // Sort particles if needed (for transparency rendering)
        if( m_sortMode != SortMode::None )
        {
            sortParticles();
        }
    }

    void ParticleTechnique::updateParticles( f32 deltaTime )
    {
        for( auto &particle : m_particles )
        {
            if( !particle )
            {
                continue;
            }

            // Update particle lifetime
            void *dataPtr = particle->getData();
            if( dataPtr )
            {
                // ParticleData would be updated here
                // This is renderer-specific implementation
            }
        }
    }

    void ParticleTechnique::cleanupDeadParticles()
    {
        // Remove null or dead particles
        m_particles.erase(
            std::remove_if(
                m_particles.begin(), m_particles.end(),
                []( const SmartPtr<IParticle> &particle ) -> bool {
                    if( !particle )
                    {
                        return true;
                    }

                    // Check if particle is dead based on its data
                    void *dataPtr = particle->getData();
                    if( dataPtr )
                    {
                        // Check particle lifetime/data for death condition
                        // This is renderer-specific implementation
                        return false;  // Placeholder - actual implementation depends on IParticle
                    }

                    return false;
                } ),
            m_particles.end() );
    }

    bool ParticleTechnique::shouldSkipUpdate() const
    {
        // LOD check
        if( m_lodEnabled && m_lodDistance > 0.0f )
        {
            auto particleSystem = getParticleSystem();
            if( particleSystem )
            {
                // Calculate distance to camera
                // This would require camera position from the scene
                // For now, return false to always update
            }
        }

        // Culling check
        if( m_cullMode != CullMode::None )
        {
            // Distance culling
            if( m_cullMode == CullMode::Distance && m_cullDistance > 0.0f )
            {
                // Would check distance to camera here
                return false;
            }

            // Frustum culling would require bounds and camera frustum
            if( m_cullMode == CullMode::Frustum )
            {
                return false;
            }
        }

        return false;
    }

    void ParticleTechnique::sortParticles()
    {
        if( m_particles.empty() )
        {
            return;
        }

        switch( m_sortMode )
        {
        case SortMode::Distance:
        {
            // Sort by distance to camera (far to near for transparency)
            std::sort( m_particles.begin(), m_particles.end(),
                       []( const SmartPtr<IParticle> &a, const SmartPtr<IParticle> &b ) -> bool {
                           if( !a || !b )
                               return false;

                           void *dataA = a->getData();
                           void *dataB = b->getData();

                           if( !dataA || !dataB )
                               return false;

                           // Compare distances - actual implementation depends on particle data structure
                           return false;  // Placeholder
                       } );
            break;
        }

        case SortMode::Age:
        {
            // Sort by age (oldest first)
            std::sort( m_particles.begin(), m_particles.end(),
                       []( const SmartPtr<IParticle> &a, const SmartPtr<IParticle> &b ) -> bool {
                           if( !a || !b )
                               return false;

                           void *dataA = a->getData();
                           void *dataB = b->getData();

                           if( !dataA || !dataB )
                               return false;

                           // Compare ages - actual implementation depends on particle data structure
                           return false;  // Placeholder
                       } );
            break;
        }

        default:
            break;
        }
    }

    SmartPtr<IParticleEmitter> ParticleTechnique::addEmitter()
    {
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

        if( auto factoryManager = graphicsSystem->getFactoryManagerPtr() )
        {
            if( auto emitter = factoryManager->make_object<IParticleEmitter>() )
            {
                emitter->setParticleSystem( getParticleSystem() );
                m_emitters.push_back( emitter );
                return emitter;
            }
        }

        return nullptr;
    }

    SmartPtr<IParticleEmitter> ParticleTechnique::addEmitter( const String &name )
    {
        auto emitter = addEmitter();
        if( emitter )
        {
            emitter->setName( name );
        }

        return emitter;
    }

    SmartPtr<IParticleSystem> ParticleTechnique::getParticleSystem() const
    {
        auto p = m_particleSystem.lock();
        return p;
    }

    void ParticleTechnique::setParticleSystem( SmartPtr<IParticleSystem> particleSystem )
    {
        m_particleSystem = particleSystem;

        // Propagate to all child nodes
        for( auto &emitter : m_emitters )
        {
            if( emitter )
            {
                emitter->setParticleSystem( particleSystem );
            }
        }

        for( auto &affector : m_affectors )
        {
            if( affector )
            {
                affector->setParticleSystem( particleSystem );
            }
        }
    }

    void ParticleTechnique::removeEmitter( SmartPtr<IParticleEmitter> emitter )
    {
        if( !emitter )
        {
            return;
        }

        m_emitters.erase( std::remove( m_emitters.begin(), m_emitters.end(), emitter ),
                          m_emitters.end() );
    }

    Array<SmartPtr<IParticleEmitter>> ParticleTechnique::getParticleEmitters() const
    {
        return m_emitters;
    }

    SmartPtr<IParticleAffector> ParticleTechnique::addAffector( u32 id )
    {
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

        if( auto factoryManager = graphicsSystem->getFactoryManagerPtr() )
        {
            if( auto affector = factoryManager->make_object<IParticleAffector>() )
            {
                affector->setParticleSystem( getParticleSystem() );
                m_affectors.push_back( affector );
                return affector;
            }
        }

        return nullptr;
    }

    void ParticleTechnique::removeAffector( SmartPtr<IParticleAffector> affector )
    {
        if( !affector )
        {
            return;
        }

        m_affectors.erase( std::remove( m_affectors.begin(), m_affectors.end(), affector ),
                           m_affectors.end() );
    }

    Array<SmartPtr<IParticleAffector>> ParticleTechnique::getParticleAffectors() const
    {
        return m_affectors;
    }

    void ParticleTechnique::addRenderer( SmartPtr<IParticleRenderer> renderer )
    {
        if( renderer )
        {
            m_renderers.push_back( renderer );
        }
    }

    void ParticleTechnique::removeRenderer( SmartPtr<IParticleRenderer> renderer )
    {
        if( !renderer )
        {
            return;
        }

        m_renderers.erase( std::remove( m_renderers.begin(), m_renderers.end(), renderer ),
                           m_renderers.end() );
    }

    Array<SmartPtr<IParticleRenderer>> ParticleTechnique::getParticleRenderers() const
    {
        return m_renderers;
    }

    SmartPtr<IParticleEmitter> ParticleTechnique::getEmitter( hash32 hash ) const
    {
        for( const auto &emitter : m_emitters )
        {
            if( emitter && StringUtil::getHash( emitter->getName() ) == hash )
                return emitter;
        }
        return nullptr;
    }

    SmartPtr<IParticleAffector> ParticleTechnique::getAffector( hash32 hash ) const
    {
        for( const auto &affector : m_affectors )
        {
            if( affector && StringUtil::getHash( affector->getName() ) == hash )
                return affector;
        }
        return nullptr;
    }

    SmartPtr<IParticleRenderer> ParticleTechnique::getRenderer( hash32 hash ) const
    {
        for( const auto &renderer : m_renderers )
        {
            if( renderer && StringUtil::getHash( renderer->getName() ) == hash )
                return renderer;
        }
        return nullptr;
    }

    SmartPtr<IParticleEmitter> ParticleTechnique::getEmitterByName( const String &name ) const
    {
        for( const auto &emitter : m_emitters )
        {
            if( emitter && emitter->getName() == name )
                return emitter;
        }
        return nullptr;
    }

    SmartPtr<IParticleAffector> ParticleTechnique::getAffectorByName( const String &name ) const
    {
        for( const auto &affector : m_affectors )
        {
            if( affector && affector->getName() == name )
                return affector;
        }
        return nullptr;
    }

    SmartPtr<IParticleRenderer> ParticleTechnique::getRendererByName( const String &name ) const
    {
        for( const auto &renderer : m_renderers )
        {
            if( renderer && renderer->getName() == name )
                return renderer;
        }
        return nullptr;
    }

    Array<SmartPtr<IParticle>> ParticleTechnique::getParticles() const
    {
        return m_particles;
    }

    void ParticleTechnique::setParticles( const Array<SmartPtr<IParticle>> &particles )
    {
        m_particles = particles;
    }

    void ParticleTechnique::addParticle( SmartPtr<IParticle> particle )
    {
        if( !particle )
        {
            return;
        }

        // Respect max particle limit
        if( m_particles.size() >= m_maxParticles )
        {
            // Optional: Remove oldest particle when limit reached
            if( !m_particles.empty() )
            {
                m_particles.erase( m_particles.begin() );
            }
        }

        m_particles.push_back( particle );
    }

    void ParticleTechnique::removeParticle( SmartPtr<IParticle> particle )
    {
        if( !particle )
        {
            return;
        }

        m_particles.erase( std::remove( m_particles.begin(), m_particles.end(), particle ),
                           m_particles.end() );
    }

    void ParticleTechnique::clearParticles()
    {
        m_particles.clear();
    }

    // Additional technique configuration methods
    void ParticleTechnique::setMaxParticles( size_t max )
    {
        m_maxParticles = max;
        m_particles.reserve( max );
    }

    size_t ParticleTechnique::getMaxParticles() const
    {
        return m_maxParticles;
    }

    void ParticleTechnique::setLODEnabled( bool enabled )
    {
        m_lodEnabled = enabled;
    }

    bool ParticleTechnique::isLODEnabled() const
    {
        return m_lodEnabled;
    }

    void ParticleTechnique::setLODDistance( f32 distance )
    {
        m_lodDistance = Math<f32>::max( 0.0f, distance );
    }

    f32 ParticleTechnique::getLODDistance() const
    {
        return m_lodDistance;
    }

    void ParticleTechnique::setUpdateEnabled( bool enabled )
    {
        m_updateEnabled = enabled;
    }

    bool ParticleTechnique::isUpdateEnabled() const
    {
        return m_updateEnabled;
    }

    void ParticleTechnique::setCullMode( CullMode mode )
    {
        m_cullMode = mode;
    }

    ParticleTechnique::CullMode ParticleTechnique::getCullMode() const
    {
        return m_cullMode;
    }

    void ParticleTechnique::setCullDistance( f32 distance )
    {
        m_cullDistance = Math<f32>::max( 0.0f, distance );
    }

    f32 ParticleTechnique::getCullDistance() const
    {
        return m_cullDistance;
    }

    void ParticleTechnique::setParticlePoolSize( size_t size )
    {
        m_particlePoolSize = size;
        m_particles.reserve( size );
    }

    size_t ParticleTechnique::getParticlePoolSize() const
    {
        return m_particlePoolSize;
    }

    void ParticleTechnique::setSortMode( SortMode mode )
    {
        m_sortMode = mode;
    }

    ParticleTechnique::SortMode ParticleTechnique::getSortMode() const
    {
        return m_sortMode;
    }

    void ParticleTechnique::setSimulationSpace( SimulationSpace space )
    {
        m_simulationSpace = space;
    }

    ParticleTechnique::SimulationSpace ParticleTechnique::getSimulationSpace() const
    {
        return m_simulationSpace;
    }

    size_t ParticleTechnique::getNumActiveParticles() const
    {
        return m_particles.size();
    }

    size_t ParticleTechnique::getNumEmitters() const
    {
        return m_emitters.size();
    }

    size_t ParticleTechnique::getNumAffectors() const
    {
        return m_affectors.size();
    }

    size_t ParticleTechnique::getNumRenderers() const
    {
        return m_renderers.size();
    }

    void ParticleTechnique::reset()
    {
        clearParticles();

        // Reset all emitters
        for( auto &emitter : m_emitters )
        {
            if( emitter )
            {
                // Emitter reset would go here if IParticleEmitter had reset method
            }
        }
    }

}  // namespace workphone::render
