#include "WPGraphics/WPClawHammerPCH.hpp"

#include "WPGraphics/Particle/Jobs/ParticleUpdateJob.hpp"
#include "WPGraphics/Particle/ParticleData.hpp"
#include <Workphone/Interface/Graphics/IParticle.hpp>

namespace workphone
{
    namespace render
    {
        ParticleUpdateJob::ParticleUpdateJob( Array<SmartPtr<IParticle>> &particles ) :
            m_particles( particles )
        {
        }

        ParticleUpdateJob::~ParticleUpdateJob()
        {
        }

        void ParticleUpdateJob::execute()
        {
            for( u32 i = 0; i < m_particles.size(); ++i )
            {
                SmartPtr<IParticle> &particle = m_particles[i];
                auto particleData = static_cast<ParticleData *>( particle->getData() );
                particleData->update( 0.0 );
            }
        }
    }  // namespace render
}  // namespace workphone
