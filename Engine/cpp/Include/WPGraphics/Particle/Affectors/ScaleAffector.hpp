#ifndef ScaleAffector_h__
#define ScaleAffector_h__

#include <Workphone/Interface/Graphics/IParticleAffector.hpp>
#include "WPGraphics/Particle/CParticleNode.hpp"

namespace workphone
{
    namespace render
    {
        class ScaleAffector : public CParticleNode<IParticleAffector>
        {
        public:
            ScaleAffector();
            ~ScaleAffector() override;

            void update() override;

            Vector3F getScale() const;
            void setScale( const Vector3F &scale );

            void calculateState( SmartPtr<IParticle> &particle, u32 stateIndex,
                                 void *data = nullptr ) override;

        protected:
            Vector3F m_scale;
        };

        using ScaleAffectorPtr = SmartPtr<ScaleAffector>;
    }  // namespace render
}  // namespace workphone

#endif  // ScaleAffector_h__
