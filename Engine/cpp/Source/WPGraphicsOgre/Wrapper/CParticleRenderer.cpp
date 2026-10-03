#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CParticleRenderer.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {

        PUParticleRenderer::PUParticleRenderer( ParticleUniverse::ParticleRenderer *renderer ) :
            m_renderer( renderer )
        {
        }

        PUParticleRenderer::PUParticleRenderer()
        {
        }

        PUParticleRenderer::~PUParticleRenderer()
        {
        }

    }  // namespace render
}  // namespace workphone
