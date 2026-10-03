#include "WPGraphics/WPClawHammerPCH.hpp"

#include "WPGraphics/Particle/Emitters/CParticleEmitter.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        template <class T>
        int CParticleEmitter<T>::m_nameExt = 0;

        template <class T>
        CParticleEmitter<T>::CParticleEmitter()
        {
            String name = String( "CParticleEmitter" ) + StringUtil::toString( m_nameExt++ );
            // m_handle->setName(name);
        }

        template <class T>
        CParticleEmitter<T>::~CParticleEmitter()
        {
        }

        template class CParticleEmitter<IParticleEmitter>;
    }  // namespace render
}  // namespace workphone
