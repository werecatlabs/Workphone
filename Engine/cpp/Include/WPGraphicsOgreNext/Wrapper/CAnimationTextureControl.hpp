#ifndef _CAnimationTextureCtrl_H_
#define _CAnimationTextureCtrl_H_

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IAnimationTextureControl.hpp>
#include <OgreMaterial.h>
#include <OgreTextureUnitState.h>
#include <OgreController.h>

namespace workphone
{
    namespace render
    {
        class CAnimationTextureControl : public IAnimationTextureControl
        {
        public:
            CAnimationTextureControl();
            ~CAnimationTextureControl() override;

            void initialise( Ogre::TextureUnitState *textureUnit );

            void update( const s32 &task, const time_interval &t, const time_interval &dt );

            bool setAnimationEnabled( bool enabled ) override;
            bool setAnimationEnabled( bool enabled, f32 timePosition ) override;
            bool isAnimationEnabled() override;

            bool hasAnimationEnded() override;

            void setAnimationLoop( bool loop ) override;
            bool isAnimationLooping() const override;

            void setAnimationReversed() override;
            bool isAnimationReversed() override;

            bool setTimePosition( f32 timePosition ) override;
            f32 getTimePosition() const override;

        private:
            Ogre::TextureUnitState *m_textureUnit;
            Ogre::Controller<f32> *m_animController;

            u32 m_prevFrame;

            bool m_enabled;
        };

    }  // namespace render
}  // namespace workphone

#endif
