#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CParticleAffector.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {

        PUParticleAffector::PUParticleAffector( ParticleUniverse::ParticleAffector *affector ) :
            m_affector( affector )
        {
        }

        PUParticleAffector::PUParticleAffector()
        {
        }

        PUParticleAffector::~PUParticleAffector()
        {
        }

        void PUParticleAffector::calculateState( SmartPtr<IParticle> &particle, u32 stateIndex,
                                                 void *data /*= nullptr*/ )
        {
        }

        SmartPtr<IParticleSystem> PUParticleAffector::getParticleSystem() const
        {
            return nullptr;
        }

        void PUParticleAffector::setParticleSystem( SmartPtr<IParticleSystem> particleSystem )
        {
        }

    }  // namespace render
}  // namespace workphone
