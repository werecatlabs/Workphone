#ifndef _PFXSSAO_H_
#define _PFXSSAO_H_

/*
Copyright (C) 2012 Ilija Boshkov

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
    Based on the SSAO Demo by nullsquared.
    Original Thread: http://www.ogre3d.org/forums/viewtopic.php?t=42350
*/

#include <Ogre.h>

class PFXSSAO : public Ogre::CompositorInstance::Listener, public Ogre::SceneManager::Listener
{
public:
    Ogre::SceneManager *mSceneMgr;
    Ogre::Camera *mCamera;
    Ogre::Viewport *mViewport;
    Ogre::RenderWindow *mWindow;
    Ogre::CompositorInstance *mCompositor;

    PFXSSAO( Ogre::RenderWindow *wnd, Ogre::Camera *cam );

    void initSSAO();

    void stopSSAO();

    void setEnabled( bool state );

    void toggle();

    void initShadows();

    void notifyMaterialRender( Ogre::uint32 pass_id, Ogre::MaterialPtr &mat );

    void shadowTextureCasterPreViewProj( Ogre::Light *light, Ogre::Camera *cam, size_t );
    void shadowTexturesUpdated( size_t );
    void shadowTextureReceiverPreViewProj( Ogre::Light *, Ogre::Frustum * );
    void preFindVisibleObjects( Ogre::SceneManager *, Ogre::SceneManager::IlluminationRenderStage,
                                Ogre::Viewport * );
    void postFindVisibleObjects( Ogre::SceneManager *, Ogre::SceneManager::IlluminationRenderStage,
                                 Ogre::Viewport * );
};
#endif
