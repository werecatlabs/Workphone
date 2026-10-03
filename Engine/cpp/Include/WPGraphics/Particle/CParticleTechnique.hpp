#ifndef ParticleTechnique_h__
#define ParticleTechnique_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IParticleTechnique.hpp>
#include "Workphone/Core/ConcurrentQueue.hpp"
#include "Workphone/Thread/RecursiveMutex.hpp"
#include <Workphone/Core/HashMap.hpp>
#include "WPGraphics/Particle/CParticleNode.hpp"

namespace workphone
{
    namespace render
    {
        class CParticleTechnique : public CParticleNode<IParticleTechnique>
        {
        public:
            CParticleTechnique();
            ~CParticleTechnique() override;

            void update() override;

            void removeEmitter( SmartPtr<IParticleEmitter> emitter ) override;

            SmartPtr<IParticleAffector> addAffector( u32 id ) override;
            void removeAffector( SmartPtr<IParticleAffector> affector ) override;

            void addRenderer( SmartPtr<IParticleRenderer> renderer ) override;
            void removeRenderer( SmartPtr<IParticleRenderer> renderer ) override;

            SmartPtr<IParticleEmitter> getEmitter( hash32 hash ) const override;
            SmartPtr<IParticleAffector> getAffector( hash32 hash ) const override;
            SmartPtr<IParticleRenderer> getRenderer( hash32 hash ) const override;

            SmartPtr<IParticleEmitter> getEmitterByName( const String &name ) const override;
            SmartPtr<IParticleAffector> getAffectorByName( const String &name ) const override;
            SmartPtr<IParticleRenderer> getRendererByName( const String &name ) const override;

            void calculateState( SmartPtr<IParticle> particle, u32 stateIndex, void *data = nullptr );

            SmartPtr<IParticleSystem> getParticleSystem() const override;
            void setParticleSystem( SmartPtr<IParticleSystem> particleSystem ) override;

            Array<SmartPtr<IParticleEmitter>> getParticleEmitters() const override;
            Array<SmartPtr<IParticleAffector>> getParticleAffectors() const override;
            Array<SmartPtr<IParticleRenderer>> getParticleRenderers() const override;

            Array<SmartPtr<IParticle>> getParticles() const override;
            Array<SmartPtr<IParticle>> &getParticlesRef();
            u32 getNumParticles() const;

            void setParticles( const Array<SmartPtr<IParticle>> &particles ) override;
            void addParticle( SmartPtr<IParticle> particle ) override;
            void removeParticle( SmartPtr<IParticle> particle ) override;
            void clearParticles() override;

        protected:
            SmartPtr<IParticleSystem> m_particleSystem;

            ConcurrentQueue<SmartPtr<IParticle>> m_addParticlesQueue;
            ConcurrentQueue<SmartPtr<IParticle>> m_removeParticlesQueue;
            Array<SmartPtr<IParticle>> m_particles;
            u32 m_numParticles;

            using ParticleEmitters = HashMap<hash32, SmartPtr<IParticleEmitter>>;
            ParticleEmitters m_particleEmitters;

            using ParticleAffectors = HashMap<hash32, SmartPtr<IParticleAffector>>;
            ParticleAffectors m_particleAffectors;

            using ParticleRenderers = HashMap<hash32, SmartPtr<IParticleRenderer>>;
            ParticleRenderers m_particleRenderers;

            mutable RecursiveMutex m_mutex;

            static int m_nameExt;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ParticleSystemManager_h__
