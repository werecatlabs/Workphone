// "Depth of Field" demo for Ogre
// Copyright (C) 2006  Christian Lindequist Larsen
//
// This code is in the public domain. You may do whatever you want with it.

#ifndef __DepthOfFieldEffect_H__
#define __DepthOfFieldEffect_H__

#include "Ogre.h"

namespace workphone
{
    namespace render
    {

        class DepthOfFieldEffect : public Ogre::CompositorInstance::Listener,
                                   public Ogre::RenderTargetListener,
                                   public Ogre::RenderQueue::RenderableListener
        {
        public:
            static const int BLUR_DIVISOR;

            enum PassId
            {
                BlurPass,
                OutputPass
            };

            DepthOfFieldEffect( Ogre::Viewport *viewport );
            ~DepthOfFieldEffect();

            float getNearDepth() const;
            float getFocalDepth() const;
            float getFarDepth() const;
            void setFocalDepths( float nearDepth, float focalDepth, float farDepth );
            float getFarBlurCutoff() const;
            void setFarBlurCutoff( float cutoff );
            bool getEnabled() const;
            void setEnabled( bool value );

        private:
            void createDOFCompositor();

            void createDepthRenderTexture();
            void destroyDepthRenderTexture();

            // Implementation of Ogre::CompositorInstance::Listener
            void notifyMaterialSetup( Ogre::uint32 passId, Ogre::MaterialPtr &material );

            // Implementation of Ogre::RenderTargetListener
            void preViewportUpdate( const Ogre::RenderTargetViewportEvent &evt );
            void postViewportUpdate( const Ogre::RenderTargetViewportEvent &evt );

            // Implementation of Ogre::RenderQueue::RenderableListener
            bool renderableQueued( Ogre::Renderable *rend, Ogre::uint8 groupID, Ogre::ushort priority,
                                   Ogre::Technique **ppTech );

            virtual bool renderableQueued( Ogre::Renderable *rend, Ogre::uint8 groupID,
                                           Ogre::ushort priority, Ogre::Technique **ppTech,
                                           Ogre::RenderQueue * );

            Ogre::Viewport *depthViewport;
            Ogre::Viewport *mViewport;
            Ogre::MaterialPtr mDepthMaterial;
            Ogre::Technique *mDepthTechnique;
            Ogre::Technique *mAmbientTechnique;
            Ogre::TexturePtr depthTexture;
            // Ogre::CompositorInstance* mCompositor;
            // Ogre::RenderQueueInvocationSequence* invocationSequence;

            float mNearDepth;
            float mFocalDepth;
            float mFarDepth;
            float mFarBlurCutoff;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // __DepthOfFieldEffect_H__
