#ifndef CParticleAffector_h__
#define CParticleAffector_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/ParticleAffector.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticleNode.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        class CParticleAffector : public CParticleNode<ParticleAffector>
        {
        public:
            CParticleAffector();
            ~CParticleAffector() override;

            void calculateState( SmartPtr<IParticle> &particle, u32 stateIndex, void *data );

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // CParticleAffector_h__
