#include "WPGraphics/WPClawHammerPCH.hpp"
#include "WPGraphics/Particle/CParticle.hpp"
#include "WPGraphics/Particle/ParticleData.hpp"

namespace workphone
{
    namespace render
    {
        CParticle::CParticle() : m_data( nullptr )
        {
        }

        CParticle::~CParticle()
        {
        }

        void *CParticle::getData() const
        {
            return m_data;
        }

        void CParticle::setData( void *data )
        {
            // m_data = (ParticleData*)data;
        }
    }  // namespace render
}  // namespace workphone
