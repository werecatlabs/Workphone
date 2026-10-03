#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/Particle/Jobs/UpdateEmittersJob.hpp>
#include <Workphone/Interface/Graphics/IParticleEmitter.hpp>

namespace workphone
{
    namespace render
    {
        UpdateEmittersJob::UpdateEmittersJob( const Array<SmartPtr<IParticleEmitter>> &emitters ) :
            m_emitters( emitters )
        {
        }

        UpdateEmittersJob::~UpdateEmittersJob()
        {
        }

        void UpdateEmittersJob::execute()
        {
            for( u32 i = 0; i < m_emitters.size(); ++i )
            {
                m_emitters[i]->update();
            }
        }
    }  // namespace render
}  // namespace workphone
