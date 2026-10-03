#include "WPGraphics/WPClawHammerPCH.hpp"
#include "WPGraphics/Particle/ParticleSystemManager.hpp"
#include "WPGraphics/Particle/CParticleSystem.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        ParticleSystemManager::ParticleSystemManager()
        {
        }

        ParticleSystemManager::~ParticleSystemManager()
        {
        }

        void ParticleSystemManager::loadScript( const String &fileName )
        {
        }

        SmartPtr<IParticleSystem> ParticleSystemManager::addParticleSystem( hash32 id )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );

            SmartPtr<IParticleSystem> particleSystem;  // (new CParticleSystem);
            m_particleSystems[id] = particleSystem;
            return particleSystem;
        }

        void ParticleSystemManager::updateParticles()
        {
        }

        void ParticleSystemManager::updateRender()
        {
        }

        void ParticleSystemManager::update()
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
                auto it = m_particleSystems.begin();
                for( ; it != m_particleSystems.end(); ++it )
                {
                    SmartPtr<IParticleSystem> &particleSystem = it->second;
                    // particleSystem->update();
                }
            }
            break;
            case TaskId::Render:
            {
                RecursiveMutex::ScopedLock lock( m_mutex );
                auto it = m_particleSystems.begin();
                for( ; it != m_particleSystems.end(); ++it )
                {
                    SmartPtr<IParticleSystem> &particleSystem = it->second;
                    // particleSystem->update();
                }
            }
            break;
            default:
            {
            }
            }
        }

        void ParticleSystemManager::AnimationJob::execute()
        {
        }

        ParticleSystemManager::AnimationJob::~AnimationJob()
        {
            // m_owner = nullptr;
        }

        ParticleSystemManager::AnimationJob::AnimationJob( ParticleSystemManager *owner )
        //: m_owner(owner)
        {
        }
    }  // namespace render
}  // namespace workphone
