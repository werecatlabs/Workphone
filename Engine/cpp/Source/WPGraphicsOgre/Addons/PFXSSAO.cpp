#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Addons/PFXSSAO.hpp>
#include <Ogre.h>

PFXSSAO::PFXSSAO( Ogre::RenderWindow *wnd, Ogre::Camera *cam ) :
    mSceneMgr( 0 ),
    mCamera( 0 ),
    mViewport( 0 ),
    mWindow( 0 ),
    mCompositor( 0 )
{
    mWindow = wnd;
    mCamera = cam;
    Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering( Ogre::TFO_ANISOTROPIC );
    mSceneMgr = mCamera->getSceneManager();
    mViewport = mCamera->getViewport();
    initShadows();
    initSSAO();
}

void PFXSSAO::initSSAO()
{
    mCompositor = Ogre::CompositorManager::getSingleton().addCompositor( mViewport, "ssao" );
    mCompositor->setEnabled( true );
    mCompositor->addListener( this );
}

void PFXSSAO::stopSSAO()
{
}

void PFXSSAO::setEnabled( bool state )
{
    mCompositor->setEnabled( state );
}

void PFXSSAO::toggle()
{
    mCompositor->setEnabled( !mCompositor->getEnabled() );
}

void PFXSSAO::initShadows()
{
    mSceneMgr->setShadowTextureSelfShadow( true );
    //mSceneMgr->setShadowTextureCasterMaterial( "shadow_caster" );
    mSceneMgr->setShadowTextureCount( 4 );

    mSceneMgr->setShadowTextureSize( 256 );
    mSceneMgr->setShadowTexturePixelFormat( Ogre::PF_FLOAT16_RGB );
    mSceneMgr->setShadowCasterRenderBackFaces( false );

    const unsigned numShadowRTTs = mSceneMgr->getShadowTextureCount();
    for( unsigned i = 0; i < numShadowRTTs; ++i )
    {
        Ogre::TexturePtr tex = mSceneMgr->getShadowTexture( i );
        Ogre::Viewport *vp = tex->getBuffer()->getRenderTarget()->getViewport( 0 );
        vp->setBackgroundColour( Ogre::ColourValue( 1, 1, 1, 1 ) );
        vp->setClearEveryFrame( true );
    }
    mSceneMgr->setShadowTechnique( Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED );
    mSceneMgr->addListener( this );
}

void PFXSSAO::notifyMaterialRender( Ogre::uint32 pass_id, Ogre::MaterialPtr &mat )
{
    if( pass_id != 42 )
        return;

    Ogre::Vector3 farCorner = mCamera->getViewMatrix( true ) * mCamera->getWorldSpaceCorners()[4];
    Ogre::Pass *pass = mat->getBestTechnique()->getPass( 0 );
    Ogre::GpuProgramParametersSharedPtr params = pass->getVertexProgramParameters();
    if( params->_findNamedConstantDefinition( "farCorner" ) )
        params->setNamedConstant( "farCorner", farCorner );

    params = pass->getFragmentProgramParameters();
    static const Ogre::Matrix4 CLIP_SPACE_TO_IMAGE_SPACE( 0.5, 0, 0, 0.5, 0, -0.5, 0, 0.5, 0, 0, 1, 0, 0,
                                                          0, 0, 1 );
    if( params->_findNamedConstantDefinition( "ptMat" ) )
        params->setNamedConstant(
            "ptMat", CLIP_SPACE_TO_IMAGE_SPACE * mCamera->getProjectionMatrixWithRSDepth() );
    if( params->_findNamedConstantDefinition( "far" ) )
        params->setNamedConstant( "far", mCamera->getFarClipDistance() );
}

void PFXSSAO::shadowTextureCasterPreViewProj( Ogre::Light *light, Ogre::Camera *cam, size_t )
{
    float range = light->getAttenuationRange();
    cam->setNearClipDistance( 0.01 );
    cam->setFarClipDistance( 99990 );
}

void PFXSSAO::shadowTexturesUpdated( size_t )
{
}

void PFXSSAO::shadowTextureReceiverPreViewProj( Ogre::Light *, Ogre::Frustum * )
{
}

void PFXSSAO::preFindVisibleObjects( Ogre::SceneManager *, Ogre::SceneManager::IlluminationRenderStage,
                                     Ogre::Viewport * )
{
}

void PFXSSAO::postFindVisibleObjects( Ogre::SceneManager *, Ogre::SceneManager::IlluminationRenderStage,
                                      Ogre::Viewport * )
{
}
