#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticleTechnique.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CParticleTechnique, CParticleNode<ParticleTechnique> );

    CParticleTechnique::CParticleTechnique()
    {
    }

    CParticleTechnique::~CParticleTechnique()
    {
    }

}  // namespace workphone::render
