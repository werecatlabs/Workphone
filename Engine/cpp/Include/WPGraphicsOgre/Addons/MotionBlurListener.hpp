#ifndef _MOTIONBLURLISTENER_H
#define _MOTIONBLURLISTENER_H

#include "Ogre.h"

namespace workphone
{

    class MotionBlurListener : public Ogre::CompositorInstance::Listener,
                               public Ogre::Singleton<MotionBlurListener>
    {
    public:
        MotionBlurListener();
        ~MotionBlurListener();

        void notifyViewportSize( int width, int height );
        void notifyCompositor( Ogre::CompositorInstance *instance );
        virtual void notifyMaterialSetup( Ogre::uint32 pass_id, Ogre::MaterialPtr &mat );
        virtual void notifyMaterialRender( Ogre::uint32 pass_id, Ogre::MaterialPtr &mat );

        void setBlurAdjust( Ogre::Real blurAdjust );
        Ogre::Real getBlurAdjust() const;

    private:
        Ogre::GpuProgramParametersSharedPtr m_blurParameters;
        unsigned long m_iLastRenderTime;
        Ogre::Real m_blurAdjust;
    };

}  // namespace workphone

#endif
