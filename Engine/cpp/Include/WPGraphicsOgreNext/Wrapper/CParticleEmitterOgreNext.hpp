#ifndef CParticleEmitter_h__
#define CParticleEmitter_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/ParticleEmitter.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticleNode.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        class CParticleEmitterOgreNext : public CParticleNode<ParticleEmitter>
        {
        public:
            CParticleEmitterOgreNext();
            ~CParticleEmitterOgreNext() override;

            WP_CLASS_REGISTER_DECL;

        protected:
        };

    }  // namespace render
}  // namespace workphone

#endif  // CParticleEmitter_h__
