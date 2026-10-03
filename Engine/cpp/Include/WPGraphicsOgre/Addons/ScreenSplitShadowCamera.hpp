#ifndef ScreenSplitShadowCamera_h__
#define ScreenSplitShadowCamera_h__

#include <OgreShadowCameraSetup.h>

namespace workphone
{

    class ScreenSplitShadowCamera : public Ogre::ShadowCameraSetup
    {
    public:
        /// Default constructor
        ScreenSplitShadowCamera();
        /// Destructor
        virtual ~ScreenSplitShadowCamera();

        /// Default shadow camera setup
        virtual void getShadowCamera( const Ogre::SceneManager *sm, const Ogre::Camera *cam,
                                      const Ogre::Viewport *vp, const Ogre::Light *light,
                                      Ogre::Camera *texCam, size_t iteration ) const;
    };

}  // namespace workphone

#endif  // ScreenSplitShadowCamera_h__
