#ifndef _CAnimationTextureCtrl_H_
#define _CAnimationTextureCtrl_H_

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
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
            //static const u32 FRAME_EVENT_RENDER_QUEUED;

            CAnimationTextureControl();
            ~CAnimationTextureControl();

            void initialise( Ogre::TextureUnitState *textureUnit );

            void update();

            bool setAnimationEnabled( bool enabled );
            bool setAnimationEnabled( bool enabled, f32 timePosition );
            bool isAnimationEnabled();

            bool hasAnimationEnded();

            void setAnimationLoop( bool loop );
            bool isAnimationLooping() const;

            void setAnimationReversed();
            bool isAnimationReversed();

            bool setTimePosition( f32 timePosition );
            f32 getTimePosition() const;

        private:
            Ogre::TextureUnitState *m_textureUnit;
            Ogre::Controller<f32> *m_animController;

            u32 m_prevFrame;

            bool m_enabled;
        };

    }  // end namespace render
}  // namespace workphone

#endif
