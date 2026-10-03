#include "WPGraphics/WPClawHammerPCH.hpp"
#include "WPGraphics/Particle/CParticleTechnique.hpp"
#include "WPGraphics/Particle/Jobs/UpdateAffectorsJob.hpp"
#include "WPGraphics/Particle/Jobs/UpdateEmittersJob.hpp"
#include "WPGraphics/Particle/Jobs/ParticleUpdateJob.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        int CParticleTechnique::m_nameExt = 0;

        CParticleTechnique::CParticleTechnique() : m_numParticles( 0 )
        {
            String name = String( "CParticleTechnique" ) + StringUtil::toString( m_nameExt++ );
            setName( name );
        }

        CParticleTechnique::~CParticleTechnique()
        {
        }

        void CParticleTechnique::update()
        {
            auto applicationManager = core::ApplicationManager::instance();
            auto timer = applicationManager->getTimer();

            auto task = Thread::getCurrentTask();
            auto t = timer->getTime();
            auto dt = timer->getDeltaTime();

            switch( task )
            {
            case TaskId::Application:
            {
                RecursiveMutex::ScopedLock lock( m_mutex );

                // remove old particles
                SmartPtr<IParticle> particle;
                while( m_addParticlesQueue.try_pop( particle ) )
                {
                    if( m_particles.size() < 500 )
                        m_particles.push_back( particle );
                }

                while( m_removeParticlesQueue.try_pop( particle ) )
                {
                    auto it = std::find( m_particles.begin(), m_particles.end(), particle );
                    if( it != m_particles.end() )
                    {
                        m_particles.erase( it );
                    }
                }

                Array<SmartPtr<IParticle>> &particles = getParticlesRef();

                Array<SmartPtr<IParticleEmitter>> emitters;
                auto emittersIt = m_particleEmitters.begin();
                for( ; emittersIt != m_particleEmitters.end(); ++emittersIt )
                {
                    emitters.push_back( emittersIt->second );
                }

                Array<SmartPtr<IParticleAffector>> affectors;
                auto it = m_particleAffectors.begin();
                for( ; it != m_particleAffectors.end(); ++it )
                {
                    affectors.push_back( it->second );
                }

                auto applicationManager = core::ApplicationManager::instance();
                // SmartPtr<IJobQueue> jobQueue = engine->getJobQueue();

                // JobPtr particleUpdateJob(new ParticleUpdateJob(particles, ));
                ////jobQueue->queue(particleUpdateJob);
                // particleUpdateJob->execute();

                // JobPtr updateEmittersJob(new UpdateEmittersJob(emitters, ));
                ////jobQueue->queue(updateEmittersJob);
                // updateEmittersJob->execute();

                // JobPtr updateAffectorsJob(new UpdateAffectorsJob(affectors, ));
                ////jobQueue->queue(updateAffectorsJob);
                // updateAffectorsJob->execute();
            }
            break;
            }
        }

        void CParticleTechnique::removeRenderer( SmartPtr<IParticleRenderer> renderer )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );

            auto handle = renderer->getHandle();
            auto hash = handle->getHash();
            m_particleRenderers.erase( static_cast<hash32>( hash ) );
        }

        void CParticleTechnique::addRenderer( SmartPtr<IParticleRenderer> renderer )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );

            auto handle = renderer->getHandle();
            auto hash = handle->getHash();
            m_particleRenderers[static_cast<hash32>( hash )] = renderer;
        }

        void CParticleTechnique::removeAffector( SmartPtr<IParticleAffector> affector )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );

            auto handle = affector->getHandle();
            auto hash = handle->getHash();
            m_particleAffectors.erase( static_cast<hash32>( hash ) );
        }

        SmartPtr<IParticleAffector> CParticleTechnique::addAffector( u32 id )
        {
            // RecursiveMutex::ScopedLock lock( m_mutex );

            // auto handle = affector->getHandle();
            // auto hash = handle->getHash();
            // m_particleAffectors[hash] = affector;

            return nullptr;
        }

        void CParticleTechnique::removeEmitter( SmartPtr<IParticleEmitter> emitter )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );

            auto handle = emitter->getHandle();
            auto hash = handle->getHash();
            m_particleEmitters.erase( static_cast<hash32>( hash ) );
        }

        SmartPtr<IParticleEmitter> CParticleTechnique::getEmitter( hash32 hash ) const
        {
            return nullptr;
        }

        SmartPtr<IParticleAffector> CParticleTechnique::getAffector( hash32 hash ) const
        {
            return nullptr;
        }

        SmartPtr<IParticleRenderer> CParticleTechnique::getRenderer( hash32 hash ) const
        {
            return nullptr;
        }

        SmartPtr<IParticleEmitter> CParticleTechnique::getEmitterByName( const String &name ) const
        {
            return nullptr;
        }

        SmartPtr<IParticleAffector> CParticleTechnique::getAffectorByName( const String &name ) const
        {
            return nullptr;
        }

        SmartPtr<IParticleRenderer> CParticleTechnique::getRendererByName( const String &name ) const
        {
            return nullptr;
        }

        void CParticleTechnique::calculateState( SmartPtr<IParticle> particle, u32 stateIndex,
                                                 void *data /*= nullptr */ )
        {
        }

        SmartPtr<IParticleSystem> CParticleTechnique::getParticleSystem() const
        {
            return m_particleSystem;
        }

        void CParticleTechnique::setParticleSystem( SmartPtr<IParticleSystem> particleSystem )
        {
            m_particleSystem = particleSystem;
        }

        Array<SmartPtr<IParticleEmitter>> CParticleTechnique::getParticleEmitters() const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );

            Array<SmartPtr<IParticleEmitter>> emitters;
            return emitters;
        }

        Array<SmartPtr<IParticleAffector>> CParticleTechnique::getParticleAffectors() const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );

            Array<SmartPtr<IParticleAffector>> emitters;
            return emitters;
        }

        Array<SmartPtr<IParticleRenderer>> CParticleTechnique::getParticleRenderers() const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );

            Array<SmartPtr<IParticleRenderer>> renderers;

            auto it = m_particleRenderers.begin();
            for( ; it != m_particleRenderers.end(); ++it )
            {
                renderers.push_back( it->second );
            }

            return renderers;
        }

        void CParticleTechnique::addParticle( SmartPtr<IParticle> particle )
        {
            m_addParticlesQueue.push( particle );
            ++m_numParticles;
        }

        u32 CParticleTechnique::getNumParticles() const
        {
            return static_cast<u32>( m_particles.size() );
        }

        void CParticleTechnique::setParticles( const Array<SmartPtr<IParticle>> &particles )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            // m_particles = particles;
        }

        Array<SmartPtr<IParticle>> CParticleTechnique::getParticles() const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );

            Array<SmartPtr<IParticle>> particles;
            particles.reserve( m_particles.size() );

            auto it = m_particles.begin();
            for( ; it != m_particles.end(); ++it )
            {
                particles.push_back( ( *it ) );
            }

            return particles;
        }

        Array<SmartPtr<IParticle>> &CParticleTechnique::getParticlesRef()
        {
            return m_particles;
        }

        void CParticleTechnique::clearParticles()
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            m_particles.clear();
        }

        void CParticleTechnique::removeParticle( SmartPtr<IParticle> particle )
        {
            m_removeParticlesQueue.push( particle );
            --m_numParticles;
        }
    }  // namespace render
}  // namespace workphone
