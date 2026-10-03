#ifndef CPointEmitter_h__
#define CPointEmitter_h__

#include <Workphone/Interface/Graphics/IParticleEmitter.hpp>
#include "WPGraphics/Particle/Emitters/CParticleEmitter.hpp"

namespace workphone
{
    namespace render
    {
        class PointEmitter : public CParticleEmitter<IParticleEmitter>
        {
        public:
            PointEmitter();
            ~PointEmitter() override;

            void update() override;

            void initialise( SmartPtr<IBuildDirector> objectTemplate ) override;

        protected:
            time_interval m_nextEmissionTime;
        };
    }  // namespace render
}  // namespace workphone

#endif  // PointEmitter_h__
