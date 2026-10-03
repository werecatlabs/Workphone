#ifndef ParticleSystemManager_h__
#define ParticleSystemManager_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Graphics/IParticleManager.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include "CParticleTechnique.hpp"
#include "Particle/ParticleData.hpp"
#include <list>
#include <Workphone/Core/HashMap.hpp>
#include "Workphone/System/Job.hpp"
#include "Workphone/Thread/RecursiveMutex.hpp"

namespace workphone
{
    namespace render
    {
        class ParticleSystemManager : public IParticleManager
        {
        public:
            ParticleSystemManager();
            ~ParticleSystemManager() override;

            void loadScript( const String &fileName );

            SmartPtr<IParticleSystem> addParticleSystem( hash32 id ) override;

            void update() override;
            void updateParticles();
            void updateRender();

        protected:
            class AnimationJob : public Job
            {
            public:
                AnimationJob( ParticleSystemManager *owner );
                ~AnimationJob() override;

                void execute() override;

                ParticleSystemManager *m_owner;
            };

            using ParticleSystems = HashMap<hash32, SmartPtr<IParticleSystem>>;
            ParticleSystems m_particleSystems;

            Array<SmartPtr<IParticleSystem>> m_particleSystemsCache;

            RecursiveMutex m_mutex;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ParticleSystemManager_h__
