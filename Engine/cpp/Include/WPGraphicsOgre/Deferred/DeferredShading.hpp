/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2012 Torus Knot Software Ltd
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
same license as the rest of the engine.
-----------------------------------------------------------------------------
*/

/**
Implementation of a Deferred Shading engine in OGRE, using Multiple Render Targets and
CG high level language shaders.
    // W.J. :wumpus: van der Laan 2005 / Noam Gat 2009 //

Deferred shading renders the scene to a 'fat' texture format, using a shader that outputs colour,
normal, depth, and possible other attributes per fragment. Multi Render Target is required as we
are dealing with many outputs which get written into multiple render textures in the same pass.

After rendering the scene in this format, the shading (lighting) can be done as a post process.
This means that lighting is done in screen space, using light-representing geometry (sphere for
point light, cone for spot light and quad for directional) to render their contribution.

The wiki article explaining this demo can be found here :
  http://www.ogre3d.org/wiki/index.php/Deferred_Shading
*/

#ifndef H_DeferredShadingSystem
#define H_DeferredShadingSystem

#include "Ogre.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#    define WIN32_LEAN_AND_MEAN
#    include "windows.h"
#endif

#include "OgreCompositorInstance.h"
#include "OgreSceneManager.h"
#include "OgreSceneNode.h"
#include "OgreMaterial.h"
#include "OgreRenderTargetListener.h"
#include "GeomUtils.hpp"
#include <map>

using namespace Ogre;

/** System to manage Deferred Shading for a camera/render target.
 *  @note With the changes to the compositor framework, this class just
 *		selects which compositors to enable.
 */
class DeferredShadingSystem : public Ogre::RenderTargetListener
{
public:
    DeferredShadingSystem( Ogre::Viewport *vp, Ogre::SceneManager *sm, Ogre::Camera *cam );
    ~DeferredShadingSystem();

    enum DSMode
    {
        DSM_SHOWLIT = 0,      // The deferred shading mode
        DSM_SHOWCOLOUR = 1,   // Show diffuse (for debugging)
        DSM_SHOWNORMALS = 2,  // Show normals (for debugging)
        DSM_SHOWDSP = 3,      // Show depth and specular channel (for debugging)
        DSM_COUNT = 4
    };

    // The first render queue that does get rendered into the GBuffer
    // place objects (like skies) that should be before gbuffer before this one.
    static const Ogre::uint8 PRE_GBUFFER_RENDER_QUEUE;

    // The first render queue that does not get rendered into the GBuffer
    // place transparent (or other non gbuffer) objects after this one
    static const Ogre::uint8 POST_GBUFFER_RENDER_QUEUE;

    void initialize();

    /** Set rendering mode (one of DSMode)
     */
    void setMode( DSMode mode );

    DSMode getMode( void ) const;

    /** Set screen space ambient occlusion mode
     */
    void setSSAO( bool ssao );

    bool getSSAO() const;

    /** Activate or deactivate system
     */
    void setActive( bool active );

    Ogre::String getGBufferCompositorName() const
    {
        return m_sGBufferCompositorName;
    }
    void setGBufferCompositorName( Ogre::String gBufferCompositorName )
    {
        m_sGBufferCompositorName = gBufferCompositorName;
    }

    Ogre::String getShowLightingCompositorName() const
    {
        return m_sShowLightingCompositorName;
    }
    void setShowLightingCompositorName( Ogre::String showLightingCompositorName )
    {
        m_sShowLightingCompositorName = showLightingCompositorName;
    }

    Ogre::String getShowNormalsCompositorName() const
    {
        return m_sShowNormalsCompositorName;
    }
    void setShowNormalsCompositorName( Ogre::String showNormalsCompositorName )
    {
        m_sShowNormalsCompositorName = showNormalsCompositorName;
    }

    Ogre::String getShowDepthSpecularCompositorName() const
    {
        return m_sShowDepthSpecularCompositorName;
    }
    void setShowDepthSpecularCompositorName( Ogre::String showDepthSpecularCompositorName )
    {
        m_sShowDepthSpecularCompositorName = showDepthSpecularCompositorName;
    }

    Ogre::String getShowColourCompositorName() const
    {
        return m_sShowColourCompositorName;
    }
    void setShowColourCompositorName( Ogre::String showColourCompositorName )
    {
        m_sShowColourCompositorName = showColourCompositorName;
    }

    Ogre::String getLightCompositionPassName() const
    {
        return m_sLightCompositionPassName;
    }
    void setLightCompositionPassName( Ogre::String lightCompositionPassName )
    {
        m_sLightCompositionPassName = lightCompositionPassName;
    }

    Ogre::String getRenderTargetName() const
    {
        return m_renderTargetName;
    }
    void setRenderTargetName( Ogre::String renderTargetName )
    {
        m_renderTargetName = renderTargetName;
    }

    Ogre::Viewport *getViewport() const
    {
        return mViewport;
    }
    void setViewport( Ogre::Viewport *viewport )
    {
        mViewport = viewport;
    }

protected:
    class ViewportMaskLogic : public Ogre::RenderTargetListener,
                              public Ogre::CompositorInstance::Listener
    {
    public:
        ViewportMaskLogic( DeferredShadingSystem *system );
        ~ViewportMaskLogic()
        {
        }

        virtual void preViewportUpdate( const Ogre::RenderTargetViewportEvent &evt );

        DeferredShadingSystem *m_system;
    };

    Ogre::Viewport *mViewport;

    Ogre::SceneManager *mSceneMgr;
    Ogre::Camera *mCamera;

    Ogre::CompositorInstance *mGBufferInstance;
    // Filters
    Ogre::CompositorInstance *mInstance[DSM_COUNT];
    Ogre::CompositorInstance *mSSAOInstance;
    // Active/inactive
    bool mActive;
    DSMode mCurrentMode;
    bool mSSAO;

    String m_sGBufferCompositorName;
    String m_sShowLightingCompositorName;
    String m_sShowNormalsCompositorName;
    String m_sShowDepthSpecularCompositorName;
    String m_sShowColourCompositorName;

    String m_sLightCompositionPassName;

    String m_renderTargetName;

    // Used to unregister compositor logics and free memory
    typedef std::map<String, CompositorLogic *> CompositorLogicMap;
    CompositorLogicMap mCompositorLogics;

    void createResources();

    void logCurrentMode( void );
};

const ColourValue SAMPLE_COLORS[] = { ColourValue::Red,          ColourValue::Green,
                                      ColourValue::Blue,         ColourValue::White,
                                      ColourValue( 1, 1, 0, 1 ), ColourValue( 1, 0, 1, 1 ) };

#endif
