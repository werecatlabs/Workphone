#include "WPGraphics/WPClawHammerPCH.hpp"
#include "WPGraphics/Particle/Affectors/ColourAffector.hpp"
#include "WPGraphics/Particle/CParticleSystem.hpp"
#include "WPGraphics/Particle/ParticleData.hpp"
#include "WPGraphics/Particle/ParticleState.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        ColourAffector::ColourAffector() : m_interpolator( nullptr )
        {
            m_interpolator = new InterpolatorNonUniform4F;
        }

        ColourAffector::~ColourAffector()
        {
            WP_SAFE_DELETE( m_interpolator );
        }

        void ColourAffector::initialise( SmartPtr<IBuildDirector> objectTemplate )
        {
            /*
            SmartPtr<ParticleAffectorTemplate> particleAffectorTemplate;  // = objectTemplate;
            if( particleAffectorTemplate )
            {
                Array<std::pair<f32, Vector4<f32> > > values;

                Array<ParticleAffectorTemplate::PointTimeVector4> points =
                    particleAffectorTemplate->getColourPoints();
                values.reserve( points.size() );

                for( u32 i = 0; i < points.size(); ++i )
                {
                    ParticleAffectorTemplate::PointTimeVector4 point = points[i];
                    values.push_back( std::pair<f32, Vector4<f32> >( point.first, point.second ) );
                }

                m_interpolator->setValues( values );
            }
             */
        }

        void ColourAffector::update()
        {
            // CParticleSystem* particleSystem;// = (CParticleSystem*)getParticleSystem();
            // IParticleTechnique* particleTechnique = (IParticleTechnique*)getParent();
            // Array<SmartPtr<IParticle>> particles = particleTechnique->getParticles();

            // for(u32 i=0; i<particles.size(); ++i)
            //{
            //	SmartPtr<IParticle> particle = particles[i];
            //	ParticleData* particleData = (ParticleData*)particle->getData();

            //	f32 normalisedLifeTime = particleData->m_lifeTime / particleData->m_maxLifeTime;
            //	particleData->m_currentState->m_colour = m_interpolator->interpolate(normalisedLifeTime);
            //}
        }

        void ColourAffector::setInterpolator( InterpolatorNonUniform4F *interpolator )
        {
            m_interpolator = interpolator;
        }

        InterpolatorNonUniform4F *ColourAffector::getInterpolator() const
        {
            return m_interpolator;
        }

        void ColourAffector::calculateState( SmartPtr<IParticle> &particle, u32 stateIndex,
                                             void *data /*= nullptr */ )
        {
        }
    }  // namespace render
}  // namespace workphone
