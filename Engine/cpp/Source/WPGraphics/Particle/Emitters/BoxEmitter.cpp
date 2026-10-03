#include "WPGraphics/WPClawHammerPCH.hpp"

#include "WPGraphics/Particle/Emitters/BoxEmitter.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        BoxEmitter::BoxEmitter()
        {
        }

        BoxEmitter::~BoxEmitter()
        {
        }

        void BoxEmitter::initialise( SmartPtr<IBuildDirector> objectTemplate )
        {
        }

        void BoxEmitter::calculateState( SmartPtr<IParticle> particle, u32 stateIndex,
                                         void *data /*= nullptr */ )
        {
        }

        void BoxEmitter::setDimensions( const Vector3F &dimensions )
        {
            m_dimensions = dimensions;
        }

        Vector3F BoxEmitter::getDimensions() const
        {
            return m_dimensions;
        }
    }  // namespace render
}  // namespace workphone
