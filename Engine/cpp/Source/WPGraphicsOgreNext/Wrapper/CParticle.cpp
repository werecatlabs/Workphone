#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticle.hpp>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, CParticle, IParticle );

        CParticle::CParticle()
        {
        }

        CParticle::~CParticle()
        {
        }

        Ogre::Particle *CParticle::getParticle() const
        {
            return m_particle;
        }

        void CParticle::setParticle( Ogre::Particle *particle )
        {
            m_particle = particle;
        }

        void *CParticle::getData() const
        {
            return m_data;
        }

        void CParticle::setData( void *data )
        {
            m_data = data;
        }

    }  // namespace render
}  // namespace workphone
