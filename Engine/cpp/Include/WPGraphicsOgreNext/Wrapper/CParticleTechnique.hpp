#ifndef CParticleTechnique_h__
#define CParticleTechnique_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/ParticleTechnique.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticleNode.hpp>

namespace workphone
{
    namespace render
    {

        class CParticleTechnique : public CParticleNode<ParticleTechnique>
        {
        public:
            CParticleTechnique();
            ~CParticleTechnique();

            WP_CLASS_REGISTER_DECL;

        protected:
        };

    }  // namespace render
}  // namespace workphone

#endif  // CParticleTechnique_h__
