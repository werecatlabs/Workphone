#ifndef ColourAffector_h__
#define ColourAffector_h__

#include <Workphone/Interface/Graphics/IParticleAffector.hpp>
#include <Workphone/Math/InterpolatorNonUniform4.hpp>
#include "WPGraphics/Particle/CParticleNode.hpp"

namespace workphone
{
    namespace render
    {
        class ColourAffector : public CParticleNode<IParticleAffector>
        {
        public:
            ColourAffector();
            ~ColourAffector() override;

            void initialise( SmartPtr<IBuildDirector> objectTemplate ) override;

            void update() override;

            InterpolatorNonUniform4F *getInterpolator() const;
            void setInterpolator( InterpolatorNonUniform4F *interpolator );

            void calculateState( SmartPtr<IParticle> &particle, u32 stateIndex,
                                 void *data = nullptr ) override;

        protected:
            InterpolatorNonUniform4F *m_interpolator;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ColourAffector_h__
