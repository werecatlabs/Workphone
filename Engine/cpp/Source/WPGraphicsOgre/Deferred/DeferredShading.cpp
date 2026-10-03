/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2013 Torus Knot Software Ltd
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
same license as the rest of the engine.
-----------------------------------------------------------------------------
*/
#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "DeferredShading.hpp"

#include "OgreConfigFile.h"
#include "OgreStringConverter.h"
#include "OgreException.h"

#include "OgreRoot.h"
#include "OgreRenderSystem.h"

#include "OgreEntity.h"
#include "OgreSubEntity.h"
#include "OgreRoot.h"

#include "OgreCompositor.h"
#include "OgreCompositorManager.h"
#include "OgreCompositorChain.h"
#include "OgreCompositorInstance.h"

#include "OgreLogManager.h"

#include "DeferredLightCP.hpp"
#include "SSAOLogic.hpp"
#include "GBufferSchemeHandler.hpp"
#include "NullSchemeHandler.hpp"

#include "SharedData.hpp"

namespace Ogre
{
    template <>
    SharedData *Singleton<SharedData>::msSingleton = 0;
}

using namespace Ogre;

const Ogre::uint8 DeferredShadingSystem::PRE_GBUFFER_RENDER_QUEUE = Ogre::RENDER_QUEUE_1;
const Ogre::uint8 DeferredShadingSystem::POST_GBUFFER_RENDER_QUEUE = Ogre::RENDER_QUEUE_8;

DeferredShadingSystem::DeferredShadingSystem( Viewport *vp, SceneManager *sm, Camera *cam ) :
    mViewport( vp ),
    mSceneMgr( sm ),
    mCamera( cam )
{
    // sm->setShadowTechnique(SHADOWTYPE_TEXTURE_ADDITIVE);
    // sm->setShadowTextureCasterMaterial("DeferredShading/Shadows/Caster");
    // mSceneMgr->setShadowTextureCount(1);
    // mSceneMgr->setShadowFarDistance(150);
    ////Use a value of "2" to use a different depth buffer pool and avoid sharing this with the
    ///Backbuffer's
    // mSceneMgr->setShadowTextureConfig( 0, 512, 512, PF_FLOAT16_R, 2 );
    // mSceneMgr->setShadowDirectionalLightExtrusionDistance(75);
    //
    // mViewport->getTarget()->addListener(new ViewportMaskLogic(this));

    m_sGBufferCompositorName = "DeferredShading/GBuffer";
    m_sShowLightingCompositorName = "DeferredShading/ShowLit";
    m_sShowNormalsCompositorName = "DeferredShading/ShowNormals";
    m_sShowDepthSpecularCompositorName = "DeferredShading/ShowDepthSpecular";
    m_sShowColourCompositorName = "DeferredShading/ShowColour";

    m_sLightCompositionPassName = "DeferredLight";
}

void DeferredShadingSystem::initialize()
{
    for( int i = 0; i < DSM_COUNT; ++i )
        mInstance[i] = 0;

    createResources();

    mActive = false;

    mSSAO = false;
    mCurrentMode = DSM_SHOWLIT;
    setActive( true );
}

DeferredShadingSystem::~DeferredShadingSystem()
{
    CompositorChain *chain = CompositorManager::getSingleton().getCompositorChain( mViewport );
    for( int i = 0; i < DSM_COUNT; ++i )
        chain->_removeInstance( mInstance[i] );
    CompositorManager::getSingleton().removeCompositorChain( mViewport );

    Ogre::CompositorManager &compMgr = Ogre::CompositorManager::getSingleton();
    CompositorLogicMap::const_iterator itor = mCompositorLogics.begin();
    CompositorLogicMap::const_iterator end = mCompositorLogics.end();
    while( itor != end )
    {
        compMgr.unregisterCompositorLogic( itor->first );
        delete itor->second;
        ++itor;
    }
    mCompositorLogics.clear();
}

void DeferredShadingSystem::setMode( DSMode mode )
{
    assert( 0 <= mode && mode < DSM_COUNT );

    // prevent duplicate setups
    if( mCurrentMode == mode && mInstance[mode]->getEnabled() == mActive )
        return;

    for( int i = 0; i < DSM_COUNT; ++i )
    {
        if( i == mode )
        {
            mInstance[i]->setEnabled( mActive );
        }
        else
        {
            mInstance[i]->setEnabled( false );
        }
    }

    mCurrentMode = mode;

    // mSSAOInstance->setEnabled(mActive && mSSAO && mCurrentMode == DSM_SHOWLIT);
}

void DeferredShadingSystem::setSSAO( bool ssao )
{
    if( ssao != mSSAO )
    {
        mSSAO = ssao;
        if( mActive && mCurrentMode == DSM_SHOWLIT )
        {
            mSSAOInstance->setEnabled( ssao );
        }
    }
}

bool DeferredShadingSystem::getSSAO() const
{
    return mSSAO;
}
void DeferredShadingSystem::setActive( bool active )
{
    if( mActive != active )
    {
        mActive = active;
        mGBufferInstance->setEnabled( active );

        // mCurrentMode could have changed with a prior call to setMode, so iterate all
        setMode( mCurrentMode );
    }
}

DeferredShadingSystem::DSMode DeferredShadingSystem::getMode( void ) const
{
    return mCurrentMode;
}

void DeferredShadingSystem::createResources( void )
{
    CompositorManager &compMan = CompositorManager::getSingleton();

    MaterialManager::getSingleton().addListener( new GBufferSchemeHandler( this ), "GBuffer" );
    MaterialManager::getSingleton().addListener( new NullSchemeHandler, "NoGBuffer" );

    compMan.registerCustomCompositionPass( getLightCompositionPassName(),
                                           new DeferredLightCompositionPass( this, mSceneMgr ) );

    // mCompositorLogics["SSAOLogic"] = new SSAOLogic;
    // compMan.registerCompositorLogic("SSAOLogic", mCompositorLogics["SSAOLogic"]);
    //

    // Create the main GBuffer compositor
    mGBufferInstance = compMan.addCompositor( mViewport, getGBufferCompositorName() );

    // Ogre::CompositionTechnique::TargetPassIterator tpIt =
    // mGBufferInstance->getTechnique()->getTargetPassIterator(); while(tpIt.hasMoreElements())
    //{
    //	Ogre::CompositionTargetPass* pass = tpIt.getNext();
    //	pass->setVisibilityMask(mViewport->getVisibilityMask());
    // }

    // Create filters
    mInstance[DSM_SHOWLIT] = compMan.addCompositor( mViewport, getShowLightingCompositorName() );
    mInstance[DSM_SHOWNORMALS] = compMan.addCompositor( mViewport, getShowNormalsCompositorName() );
    mInstance[DSM_SHOWDSP] = compMan.addCompositor( mViewport, getShowDepthSpecularCompositorName() );
    mInstance[DSM_SHOWCOLOUR] = compMan.addCompositor( mViewport, getShowColourCompositorName() );

    // tpIt = mInstance[DSM_SHOWLIT]->getTechnique()->getTargetPassIterator();
    // while(tpIt.hasMoreElements())
    //{
    //	Ogre::CompositionTargetPass* targetPass = tpIt.getNext();
    //	targetPass->setVisibilityMask(mViewport->getVisibilityMask());
    // }

    // tpIt = mInstance[DSM_SHOWNORMALS]->getTechnique()->getTargetPassIterator();
    // while(tpIt.hasMoreElements())
    //{
    //	Ogre::CompositionTargetPass* pass = tpIt.getNext();
    //	pass->setVisibilityMask(mViewport->getVisibilityMask());
    // }

    // tpIt = mInstance[DSM_SHOWDSP]->getTechnique()->getTargetPassIterator();
    // while(tpIt.hasMoreElements())
    //{
    //	Ogre::CompositionTargetPass* pass = tpIt.getNext();
    //	pass->setVisibilityMask(mViewport->getVisibilityMask());
    // }

    // tpIt = mInstance[DSM_SHOWCOLOUR]->getTechnique()->getTargetPassIterator();
    // while(tpIt.hasMoreElements())
    //{
    //	Ogre::CompositionTargetPass* pass = tpIt.getNext();
    //	pass->setVisibilityMask(mViewport->getVisibilityMask());
    // }

    // mSSAOInstance =  compMan.addCompositor(mViewport, "DeferredShading/SSAO");
}

void DeferredShadingSystem::logCurrentMode( void )
{
    if( mActive == false )
    {
        LogManager::getSingleton().logMessage( "No Compositor Enabled!" );
        return;
    }

    CompositorInstance *ci = mInstance[mCurrentMode];
    assert( ci->getEnabled() == true );

    LogManager::getSingleton().logMessage( "Current Mode: " );
    LogManager::getSingleton().logMessage( ci->getCompositor()->getName() );
}

DeferredShadingSystem::ViewportMaskLogic::ViewportMaskLogic( DeferredShadingSystem *system ) :
    m_system( system )
{
}

void DeferredShadingSystem::ViewportMaskLogic::preViewportUpdate(
    const Ogre::RenderTargetViewportEvent &evt )
{
    Ogre::CompositionTechnique::TargetPassIterator tpIt =
        m_system->mGBufferInstance->getTechnique()->getTargetPassIterator();
    while( tpIt.hasMoreElements() )
    {
        Ogre::CompositionTargetPass *pass = tpIt.getNext();
        pass->setVisibilityMask( m_system->mViewport->getVisibilityMask() );
    }

    tpIt = m_system->mInstance[DSM_SHOWLIT]->getTechnique()->getTargetPassIterator();
    while( tpIt.hasMoreElements() )
    {
        Ogre::CompositionTargetPass *targetPass = tpIt.getNext();
        targetPass->setVisibilityMask( m_system->mViewport->getVisibilityMask() );
    }

    tpIt = m_system->mInstance[DSM_SHOWNORMALS]->getTechnique()->getTargetPassIterator();
    while( tpIt.hasMoreElements() )
    {
        Ogre::CompositionTargetPass *pass = tpIt.getNext();
        pass->setVisibilityMask( m_system->mViewport->getVisibilityMask() );
    }

    tpIt = m_system->mInstance[DSM_SHOWDSP]->getTechnique()->getTargetPassIterator();
    while( tpIt.hasMoreElements() )
    {
        Ogre::CompositionTargetPass *pass = tpIt.getNext();
        pass->setVisibilityMask( m_system->mViewport->getVisibilityMask() );
    }

    tpIt = m_system->mInstance[DSM_SHOWCOLOUR]->getTechnique()->getTargetPassIterator();
    while( tpIt.hasMoreElements() )
    {
        Ogre::CompositionTargetPass *pass = tpIt.getNext();
        pass->setVisibilityMask( m_system->mViewport->getVisibilityMask() );
    }
}
