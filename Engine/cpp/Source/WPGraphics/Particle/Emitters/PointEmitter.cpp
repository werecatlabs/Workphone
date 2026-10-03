#include "WPGraphics/WPClawHammerPCH.hpp"
#include "WPGraphics/Particle/Emitters/PointEmitter.hpp"
#include "WPGraphics/Particle/CParticleSystem.hpp"
#include "WPGraphics/Particle/ParticleState.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        //-------------------------------------------------
        PointEmitter::PointEmitter() : m_nextEmissionTime( 0.0 )
        {
        }

        //-------------------------------------------------
        PointEmitter::~PointEmitter()
        {
        }

        //-------------------------------------------------
        void PointEmitter::update()
        {
            /*		if(m_nextEmissionTime < t)
                    {
                        CParticleSystem* particleSystem = (CParticleSystem*)getParticleSystem();

                        SmartPtr<IParticle> particle = particleSystem->createParticle(this);
                        if ( particle )
                        {
                            ParticleData* particleData = (ParticleData*)particle->getData();
                            if ( particleData )
                            {
                                ParticleState* currentState = particleData->getCurrentState();
                                Vector3F velocity = m_direction * m_velocity;
                                currentState->m_velocity = velocity;
                                currentState->m_position = Vector3F::zero();
                                currentState->m_scale = m_particleSize;
                                currentState->m_colour = Vector4F(1, 1, 1, 1);

                                particleData->setLifeTime(0.0f);

                                time_interval maxLifeTime = MathF::RangedRandom(m_timeToLiveMin,
               m_timeToLiveMax); particleData->setMaxLifeTime(maxLifeTime);
                            }
                        }

                        m_nextEmissionTime = t + (1.0f / m_emissionRate);
                    }	*/
        }

        //-------------------------------------------------
        void PointEmitter::initialise( SmartPtr<IBuildDirector> objectTemplate )
        {
            /*		SmartPtr<ParticleEmitterTemplate> particleEmitterTemplate = objectTemplate;
                    if(particleEmitterTemplate)
                    {
                        m_particleSize = particleEmitterTemplate->getParticleSize();
                        m_direction = particleEmitterTemplate->getDirection();
                        m_emissionRate = particleEmitterTemplate->getEmissionsPerSecond();
                        m_velocity = particleEmitterTemplate->getVelocity();

                        m_timeToLive = particleEmitterTemplate->getTimeToLive();
                        m_timeToLiveMin = particleEmitterTemplate->getTimeToLiveMin();
                        m_timeToLiveMax = particleEmitterTemplate->getTimeToLiveMax();
                    }	*/
        }
    }  // namespace render
}  // namespace workphone
