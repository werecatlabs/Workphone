#ifndef CParticle_h__
#define CParticle_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IParticle.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticleNode.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {
        class CParticle : public IParticle
        {
        public:
            CParticle();
            ~CParticle() override;

            Ogre::Particle *getParticle() const;

            void setParticle( Ogre::Particle *particle );

            void *getData() const override;
            void setData( void *data ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Ogre::Particle *m_particle = nullptr;
            void *m_data = nullptr;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CParticle_h__
