#ifndef Particle_h__
#define Particle_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace render
    {
        class ParticleState;

        class ParticleData
        {
        public:
            enum States
            {
                STATE_START,
                STATE_END,

                STATE_COUNT
            };

            ParticleData();
            ~ParticleData();

            void update( time_interval dt );

            time_interval getLifeTime() const;
            void setLifeTime( time_interval lifeTime );
            void addLifeTime( time_interval lifeTime );

            time_interval getMaxLifeTime() const;
            void setMaxLifeTime( time_interval maxLifeTime );

            IParticleEmitter *getEmitter() const;
            void setEmitter( IParticleEmitter *emitter );

            ParticleState *getPreviousState() const;
            ParticleState *getCurrentState() const;

            IParticle *getOwner() const;
            void setOwner( IParticle *owner );

            IParticleRenderer *getRenderer() const;
            void setRenderer( IParticleRenderer *renderer );

            IParticleTechnique *getTechnique() const;
            void setTechnique( IParticleTechnique *technique );

        public:
            IParticle *m_owner;
            IParticleRenderer *m_renderer;
            IParticleTechnique *m_technique;
            IParticleEmitter *m_emitter;

            ParticleState *m_previousState;
            ParticleState *m_currentState;

            void *m_graphicsData;
            void *m_interfaceData;

            time_interval m_lifeTime;
            time_interval m_maxLifeTime;
        };
    }  // namespace render
}  // namespace workphone

#endif  // Particle_h__
