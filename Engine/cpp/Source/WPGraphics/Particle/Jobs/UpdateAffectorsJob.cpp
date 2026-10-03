#include "WPGraphics/WPClawHammerPCH.hpp"

#include "WPGraphics/Particle/Jobs/UpdateAffectorsJob.hpp"
#include <Workphone/Interface/Graphics/IParticle.hpp>
#include <Workphone/Interface/Graphics/IParticleAffector.hpp>

namespace workphone
{
    namespace render
    {
        UpdateAffectorsJob::UpdateAffectorsJob( const Array<SmartPtr<IParticleAffector>> &affectors ) :
            m_affectors( affectors )
        {
        }

        UpdateAffectorsJob::~UpdateAffectorsJob()
        {
        }

        void UpdateAffectorsJob::execute()
        {
            for( u32 i = 0; i < m_affectors.size(); ++i )
            {
                m_affectors[i]->update();
            }
        }
    }  // namespace render
}  // namespace workphone
