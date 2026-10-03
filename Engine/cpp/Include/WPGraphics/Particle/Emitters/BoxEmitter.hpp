#ifndef CBoxEmitter_h__
#define CBoxEmitter_h__

#include <Workphone/Interface/Graphics/IParticleEmitter.hpp>
#include "WPGraphics/Particle/Emitters/CParticleEmitter.hpp"

namespace workphone
{
    namespace render
    {
        class BoxEmitter : public CParticleEmitter<IParticleEmitter>
        {
        public:
            BoxEmitter();
            ~BoxEmitter() override;

            void initialise( SmartPtr<IBuildDirector> objectTemplate ) override;

            void calculateState( SmartPtr<IParticle> particle, u32 stateIndex,
                                 void *data = nullptr ) override;

            Vector3F getDimensions() const;
            void setDimensions( const Vector3F &dimensions );

        protected:
            Vector3F m_dimensions;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CBoxEmitter_h__
