#ifndef CParticleAffector_h__
#define CParticleAffector_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IParticleAffector.hpp>
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

        class PUParticleAffector : public CParticleNode<IParticleAffector>
        {
        public:
            PUParticleAffector();

            PUParticleAffector( ParticleUniverse::ParticleAffector *affector );

            ~PUParticleAffector();

            void calculateState( SmartPtr<IParticle> &particle, u32 stateIndex,
                                 void *data = nullptr ) override;

            SmartPtr<IParticleSystem> getParticleSystem() const override;

            void setParticleSystem( SmartPtr<IParticleSystem> particleSystem ) override;

        protected:
            ParticleUniverse::ParticleAffector *m_affector;
        };

    }  // namespace render
}  // namespace workphone

#endif  // CParticleAffector_h__
