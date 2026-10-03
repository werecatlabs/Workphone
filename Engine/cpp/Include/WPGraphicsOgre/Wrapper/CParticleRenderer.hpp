#ifndef CParticleRenderer_h__
#define CParticleRenderer_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IParticleRenderer.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsObjectOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CParticleNode.hpp>
#include <Workphone/Core/HashMap.hpp>

#if WP_OGRE_USE_PARTICLE_UNIVERSE
#    include "ParticleUniverseSystemManager.h"
#endif

namespace workphone
{
    namespace render
    {

        class PUParticleRenderer : public CParticleNode<IParticleRenderer>
        {
        public:
            PUParticleRenderer();

            PUParticleRenderer( ParticleUniverse::ParticleRenderer *renderer );

            ~PUParticleRenderer();

        protected:
            ParticleUniverse::ParticleRenderer *m_renderer;
        };

    }  // namespace render
}  // namespace workphone

#endif  // CParticleRenderer_h__
