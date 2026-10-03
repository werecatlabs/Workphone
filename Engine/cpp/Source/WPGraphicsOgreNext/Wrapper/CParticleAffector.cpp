#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticleAffector.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, CParticleAffector, CParticleNode<ParticleAffector> );

    CParticleAffector::CParticleAffector()
    {
    }

    CParticleAffector::~CParticleAffector()
    {
    }

    void CParticleAffector::calculateState( SmartPtr<IParticle> &particle, u32 stateIndex, void *data )
    {
        // Default implementation - can be overridden in derived classes
        // This method is responsible for applying affector effects to particles
    }

}  // namespace workphone::render
