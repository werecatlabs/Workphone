#include "WPGraphics/WPClawHammerPCH.hpp"
#include "WPGraphics/Particle/Affectors/ScaleAffector.hpp"
#include "WPGraphics/Particle/CParticleSystem.hpp"
#include "WPGraphics/Particle/ParticleData.hpp"
#include "WPGraphics/Particle/ParticleState.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        ScaleAffector::ScaleAffector()
        {
            m_scale = Vector3F( 30.0f, 200.0f, 1.0f );
        }

        ScaleAffector::~ScaleAffector()
        {
        }

        void ScaleAffector::update()
        {
            // CParticleSystem* particleSystem = (CParticleSystem*)m_particleSystem;
            // const Array<Particle*>& particles = particleSystem->getActiveParticles();
            // for(u32 i=0; i<particles.size(); ++i)
            //{
            //	Particle* particle = particles[i];

            //	f32 normalisedLifeTime = particle->m_lifeTime / particle->m_maxLifeTime;
            //	particle->m_scale += m_scale * dt;
            //}
        }

        void ScaleAffector::setScale( const Vector3F &scale )
        {
            m_scale = scale;
        }

        void ScaleAffector::calculateState( SmartPtr<IParticle> &particle, u32 stateIndex,
                                            void *data /*= nullptr */ )
        {
        }

        Vector3F ScaleAffector::getScale() const
        {
            return m_scale;
        }
    }  // namespace render
}  // namespace workphone
