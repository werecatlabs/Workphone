#ifndef CParticleRenderer_h__
#define CParticleRenderer_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IParticleRenderer.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        class CParticleRenderer : public IParticleRenderer
        {
        public:
            CParticleRenderer();
            ~CParticleRenderer() override;

            WP_CLASS_REGISTER_DECL;

        protected:
        };

    }  // namespace render
}  // namespace workphone

#endif  // CParticleRenderer_h__
