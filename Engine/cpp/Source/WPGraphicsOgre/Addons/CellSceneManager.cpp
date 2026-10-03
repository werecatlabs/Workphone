#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Addons/CellSceneManager.hpp>
#include <WPGraphicsOgre/Addons/CameraVisibilityState.hpp>
#include <WPGraphicsOgre/Jobs/SceneCullJob.hpp>
#include <OgreCamera.h>
#include <OgreViewport.h>

#define USE_MULTIPLE_RENDER_QUEUES

namespace workphone
{
    namespace render
    {

        using namespace Ogre;

        CellSceneManager::CellSceneManager( const Ogre::String &name ) : Ogre::SceneManager( name )
        {
            //m_graphicsObjectVisibilityPool.setNumElements( 500000 );
            m_cullThread = new boost::thread( CullThread( this ) );

            //_suppressRenderStateChanges(true);
        }

        CellSceneManager::~CellSceneManager()
        {
            delete m_cullThread;
        }

        Ogre::Camera *CellSceneManager::createCamera( const Ogre::String &name )
        {
            Ogre::Camera *camera = SceneManager::createCamera( name );
            Ogre::Any userAny( new CameraVisibilityState );
            ( (Ogre::MovableObject *)camera )->setUserAny( userAny );
            return camera;
        }

        void CellSceneManager::_queueSkiesForRendering( Ogre::Camera *cam )
        {
#ifdef USE_MULTIPLE_RENDER_QUEUES
            Ogre::Camera *currentCamera = this->getCurrentViewport()->getCamera();
            CameraVisibilityState *visibilityState =
                ( (Ogre::MovableObject *)currentCamera )->getUserAny().get<CameraVisibilityState *>();

            if( visibilityState->getUpdateCulling() && cam->getName() == "DefaultCamera" )
            {
                // SceneManager::_queueSkiesForRendering(cam);
            }
#else
            SceneManager::_queueSkiesForRendering( cam );
#endif
        }

        void CellSceneManager::renderSingleObject( Ogre::Renderable *rend, const Ogre::Pass *pass,
                                                   bool lightScissoringClipping, bool doLightIteration,
                                                   const Ogre::LightList *manualLightList /*= 0*/ )
        {
            SceneManager::renderSingleObject( rend, pass, lightScissoringClipping, doLightIteration,
                                              manualLightList );
            return;

            Ogre::Camera *currentCamera = this->getCurrentViewport()->getCamera();
            CameraVisibilityState *cameraVisibilityState =
                ( (Ogre::MovableObject *)currentCamera )->getUserAny().get<CameraVisibilityState *>();

            u32 targetHash = *reinterpret_cast<u32 *>( this->getCurrentViewport() );
            u32 passValue = *reinterpret_cast<u32 *>( const_cast<Ogre::Pass *>( pass ) );
            u32 mask = this->getCurrentViewport()->getVisibilityMask();
            u32 renderableHash = *reinterpret_cast<u32 *>( rend );

            GraphicsObjectVisibility *graphicsObjectVisibility;
            VisibilityStates &visibilityStates =
                cameraVisibilityState->m_states[targetHash][mask][passValue];
            VisibilityStates::iterator it = visibilityStates.find( renderableHash );
            if( it == visibilityStates.end() )
            {
                //graphicsObjectVisibility = m_graphicsObjectVisibilityPool.getFreeElement();
                visibilityStates[renderableHash] = graphicsObjectVisibility;
            }
            else
            {
                graphicsObjectVisibility = it->second;
            }

            if( graphicsObjectVisibility->m_pixelDrawn > 0 || cameraVisibilityState->getUpdateCulling() )
            {
                if( graphicsObjectVisibility->m_queryObject )
                {
                    graphicsObjectVisibility->m_occlusionQuery->beginOcclusionQuery();
                    renderSingleObjectDerived( rend, pass, lightScissoringClipping, doLightIteration,
                                               manualLightList );
                    graphicsObjectVisibility->m_occlusionQuery->endOcclusionQuery();
                }
                else
                {
                    renderSingleObjectDerived( rend, pass, lightScissoringClipping, doLightIteration,
                                               manualLightList );
                }

                graphicsObjectVisibility->m_nextUpdate = 0;
            }

            graphicsObjectVisibility->update();

            ++graphicsObjectVisibility->m_nextUpdate;
        }

        void CellSceneManager::prepareRenderQueue()
        {
            //#ifdef USE_MULTIPLE_RENDER_QUEUES
            //		Ogre::Camera* currentCamera = this->getCurrentViewport()->getCamera();
            //		CameraVisibilityState* visibilityState =
            //((Ogre::MovableObject*)currentCamera)->getUserAny().get<CameraVisibilityState*>();
            //
            //		Ogre::Vector3 cameraPosition = currentCamera->getRealPosition();
            //		Ogre::Quaternion cameraOrientation = currentCamera->getRealOrientation();
            //
            //		if( visibilityState->m_lastPosition.squaredDistance(cameraPosition) > 1.0 )
            //		{
            //			visibilityState->m_lastPosition = cameraPosition;
            //			visibilityState->setUpdateCulling(true);
            //		}
            //		//else if(!visibilityState->m_lastOrientation == (cameraOrientation,
            //Ogre::Degree(1.0f)))
            //		//{
            //		//	visibilityState->m_lastOrientation = cameraOrientation;
            //		//	visibilityState->setUpdateCulling(true);
            //		//}
            //		else
            //		{
            //			visibilityState->setUpdateCulling(false);
            //		}
            //
            //		Ogre::RenderQueue* q = getRenderQueue();
            //
            //		//if(q->isEmpty())
            //		//	visibilityState->setUpdateCulling(true);
            //
            //		if(visibilityState->getUpdateCulling())
            //		{
            //			//q->clear();
            //
            //			// If we're using a custom render sequence, define based on that
            //			Ogre::RenderQueueInvocationSequence* seq =
            //mCurrentViewport->_getRenderQueueInvocationSequence(); 			if (seq)
            //			{
            //				// Iterate once to crate / reset all
            //				Ogre::RenderQueueInvocationIterator invokeIt = seq->iterator();
            //				while (invokeIt.hasMoreElements())
            //				{
            //					Ogre::RenderQueueInvocation* invocation = invokeIt.getNext();
            //					Ogre::RenderQueueGroup* group =
            //						q->getQueueGroup(invocation->getRenderQueueGroupID());
            //					group->resetOrganisationModes();
            //				}
            //				// Iterate again to build up options (may be more than one)
            //				invokeIt = seq->iterator();
            //				while (invokeIt.hasMoreElements())
            //				{
            //					Ogre::RenderQueueInvocation* invocation = invokeIt.getNext();
            //					Ogre::RenderQueueGroup* group =
            //						q->getQueueGroup(invocation->getRenderQueueGroupID());
            //					group->addOrganisationMode(invocation->getSolidsOrganisation());
            //					// also set splitting options
            //					updateRenderQueueGroupSplitOptions(group,
            //invocation->getSuppressShadows(), 						invocation->getSuppressRenderStateChanges());
            //				}
            //
            //				mLastRenderQueueInvocationCustom = true;
            //			}
            //			else
            //			{
            //				if (mLastRenderQueueInvocationCustom)
            //				{
            //					Ogre::RenderQueue::QueueGroupIterator groupIter =
            //q->_getQueueGroupIterator(); 					while (groupIter.hasMoreElements())
            //					{
            //						Ogre::RenderQueueGroup* g = groupIter.getNext();
            //						g->defaultOrganisationMode();
            //					}
            //				}
            //
            //				// Global split options
            //				updateRenderQueueSplitOptions();
            //
            //				mLastRenderQueueInvocationCustom = false;
            //			}
            //		}
            //#else
            //		SceneManager::prepareRenderQueue();
            //#endif
        }

        Ogre::SceneManager::RenderContext *CellSceneManager::_pauseRendering()
        {
            //#ifdef USE_MULTIPLE_RENDER_QUEUES
            //		RenderContext* context = new RenderContext;
            //		context->renderQueue = mRenderQueue;
            //		//context->renderQueue = getCurrentRenderQueue();
            //		context->viewport = mCurrentViewport;
            //		context->camera = mCameraInProgress;
            //		context->activeChain = _getActiveCompositorChain();
            //
            //		context->rsContext = mDestRenderSystem->_pauseFrame();
            //		mRenderQueue = 0;
            //		return context;
            //#else
            //		return SceneManager::_pauseRendering();
            //#endif

            return nullptr;
        }

        void CellSceneManager::_resumeRendering( Ogre::SceneManager::RenderContext *context )
        {
            //#ifdef USE_MULTIPLE_RENDER_QUEUES
            //		mRenderQueue = context->renderQueue;
            //		_setActiveCompositorChain(context->activeChain);
            //		Ogre::Viewport* vp = context->viewport;
            //		Ogre::Camera* camera = context->camera;
            //
            //		// Tell params about viewport
            //		mAutoParamDataSource->setCurrentViewport(vp);
            //		// Set the viewport - this is deliberately after the shadow texture update
            //		setViewport(vp);
            //
            //		// Tell params about camera
            //		mAutoParamDataSource->setCurrentCamera(camera, mCameraRelativeRendering);
            //		// Set autoparams for finite dir light extrusion
            //		mAutoParamDataSource->setShadowDirLightExtrusionDistance(mShadowDirLightExtrudeDist);
            //
            //		// Tell params about current ambient light
            //		mAutoParamDataSource->setAmbientLightColour(mAmbientLight);
            //		// Tell rendersystem
            //		mDestRenderSystem->setAmbientLight(mAmbientLight.r, mAmbientLight.g,
            //mAmbientLight.b);
            //
            //		// Tell params about render target
            //		mAutoParamDataSource->setCurrentRenderTarget(vp->getTarget());
            //
            //
            //		// Set camera window clipping planes (if any)
            //		if (mDestRenderSystem->getCapabilities()->hasCapability(Ogre::RSC_USER_CLIP_PLANES))
            //		{
            //			mDestRenderSystem->resetClipPlanes();
            //			if (camera->isWindowSet())
            //			{
            //				mDestRenderSystem->setClipPlanes(camera->getWindowPlanes());
            //			}
            //		}
            //		mCameraInProgress = context->camera;
            //		mDestRenderSystem->_resumeFrame(context->rsContext);
            //
            //		// Set rasterisation mode
            //		mDestRenderSystem->_setPolygonMode(mCameraInProgress->getPolygonMode());
            //
            //		// Set initial camera state
            //		mDestRenderSystem->_setProjectionMatrix(mCameraInProgress->getProjectionMatrixRS());
            //
            //		mCachedViewMatrix = mCameraInProgress->getViewMatrix(true);
            //
            //		if (mCameraRelativeRendering)
            //		{
            //			mCachedViewMatrix.setTrans(Ogre::Vector3::ZERO);
            //			//mCameraRelativePosition = mCameraInProgress->getDerivedPosition();
            //		}
            //		mDestRenderSystem->_setTextureProjectionRelativeTo(mCameraRelativeRendering,
            //mCameraInProgress->getDerivedPosition());
            //
            //
            //		//setViewMatrix(mCachedViewMatrix);
            //		delete context;
            //#else
            //		SceneManager::_resumeRendering(context);
            //#endif
        }

        Ogre::RenderQueue *CellSceneManager::getCurrentRenderQueue()
        {
#ifdef USE_MULTIPLE_RENDER_QUEUES
            Ogre::Viewport *viewport = this->getCurrentViewport();
            if( !viewport )
            {
                return SceneManager::getRenderQueue();
            }

            Ogre::Camera *currentCamera = viewport->getCamera();
            CameraVisibilityState *cameraVisibilityState =
                ( (Ogre::MovableObject *)currentCamera )->getUserAny().get<CameraVisibilityState *>();

#    if 1
            u32 targetHash = *reinterpret_cast<u32 *>( this->getCurrentViewport() );
            u32 mask = this->getCurrentViewport()->getVisibilityMask();

            CameraVisibilityState::RenderQueues &renderQueues =
                cameraVisibilityState->m_renderQueueStates[targetHash];
            CameraVisibilityState::RenderQueues::iterator it = renderQueues.find( mask );
            if( it != renderQueues.end() )
            {
                return it->second;
            }

            Ogre::RenderQueue *renderQueue = new Ogre::RenderQueue;

            // init render queues that do not need shadows
            renderQueue->getQueueGroup( Ogre::RENDER_QUEUE_BACKGROUND )->setShadowsEnabled( false );
            renderQueue->getQueueGroup( Ogre::RENDER_QUEUE_OVERLAY )->setShadowsEnabled( false );
            renderQueue->getQueueGroup( Ogre::RENDER_QUEUE_SKIES_EARLY )->setShadowsEnabled( false );
            renderQueue->getQueueGroup( Ogre::RENDER_QUEUE_SKIES_LATE )->setShadowsEnabled( false );

            renderQueues[mask] = renderQueue;

            return renderQueue;
#    else
            return cameraVisibilityState->m_renderQueue;
#    endif

#else
            return SceneManager::getRenderQueue();
#endif
        }

        Ogre::RenderQueue *CellSceneManager::getRenderQueue()
        {
            //#ifdef USE_MULTIPLE_RENDER_QUEUES
            //		///if(!mRenderQueue)
            //			mRenderQueue = getCurrentRenderQueue();
            //
            //		return mRenderQueue;
            //#else
            //		return SceneManager::getRenderQueue();
            //#endif

            return nullptr;
        }

        const Ogre::String &CellSceneManager::getTypeName( void ) const
        {
            return "CellSceneManager";
        }

        void CellSceneManager::_findVisibleObjects( Ogre::Camera *cam,
                                                    Ogre::VisibleObjectsBoundsInfo *visibleBounds,
                                                    bool onlyShadowCasters )
        {
#ifdef USE_MULTIPLE_RENDER_QUEUES
            CameraVisibilityState *visibilityState =
                ( (Ogre::MovableObject *)cam )->getUserAny().get<CameraVisibilityState *>();

            if( visibilityState->getUpdateCulling() )
            {
                const Ogre::Plane *planes = cam->getFrustumPlanes();
                for( u32 i = 0; i < 6; ++i )
                {
                    visibilityState->m_planes[i] = planes[i];
                }
            }

            // SceneManager::_findVisibleObjects( cam, visibleBounds, onlyShadowCasters );
            // return;
            //

            static bool updateQueue = true;

            if( visibilityState->getUpdateCulling() )
            {
#    if 1
                //visibilityState->m_renderQueueVisibilityStatePool.freeAllElements();
                visibilityState->m_renderQueueVisibilityStates.clear();

                addVisibleObjects( getRootSceneNode(), cam, visibleBounds, onlyShadowCasters,
                                   visibilityState );

                visibilityState->m_nextUpdate = 0;

                Ogre::RenderQueue *renderQueue = getRenderQueue();
                renderQueue->clear( true );

                RenderQueueVisibilityStates::iterator visibilityStateIt =
                    visibilityState->m_renderQueueVisibilityStates.begin();
                for( ; visibilityStateIt != visibilityState->m_renderQueueVisibilityStates.end();
                     ++visibilityStateIt )
                {
                    _addToRenderQueue( ( *visibilityStateIt )->m_sceneNode, cam, renderQueue,
                                       onlyShadowCasters, visibleBounds );
                }
#    else
                if( visibilityState->getState() == CameraVisibilityState::STATE_READY )
                {
                    SceneCullJobPtr sceneCullJob( new SceneCullJob, true );
                    sceneCullJob->setSceneManager( this );
                    sceneCullJob->setCamera( cam );
                    sceneCullJob->setVisibleBounds( visibleBounds );
                    sceneCullJob->setOnlyShadowCasters( onlyShadowCasters );
                    sceneCullJob->setVisibilityState( visibilityState );
                    sceneCullJob->setRenderQueue( getRenderQueue() );

                    SmartPtr<IJobQueue> jobQueue = IApplicationManager::instance()->getJobQueue();
                    jobQueue->queue( sceneCullJob );

                    // sceneCullJob->update(0, 0);
                }

#    endif
            }
            else
            {
                Ogre::RenderQueue *renderQueue = getRenderQueue();
                int stop = 0;
                stop = 0;
            }

            ++visibilityState->m_nextUpdate;
#else

            CameraVisibilityState *visibilityState =
                ( (Ogre::MovableObject *)cam )->getUserAny().get<CameraVisibilityState *>();

            if( visibilityState->getUpdateCulling() )
            {
                const Ogre::Plane *planes = cam->getFrustumPlanes();
                for( u32 i = 0; i < 6; ++i )
                {
                    visibilityState->m_planes[i] = planes[i];
                }
            }

            // SceneManager::_findVisibleObjects( cam, visibleBounds, onlyShadowCasters );
            // return;

            // if(visibilityState->getUpdateCulling())
            //{
            //	visibilityState->m_renderQueueVisibilityStatePool.freeAllElements();
            //	visibilityState->m_renderQueueVisibilityStates.clear();

            //	addVisibleObjects( getRootSceneNode(), cam, visibleBounds, onlyShadowCasters,
            //visibilityState );

            //	visibilityState->m_nextUpdate = 0;
            //}

            SceneCullJobPtr sceneCullJob( new SceneCullJob, true );
            sceneCullJob->setSceneManager( this );
            sceneCullJob->setCamera( cam );
            sceneCullJob->setVisibleBounds( visibleBounds );
            sceneCullJob->setOnlyShadowCasters( onlyShadowCasters );
            sceneCullJob->setVisibilityState( visibilityState );
            sceneCullJob->update( 0, 0 );

            Ogre::RenderQueue *renderQueue = getRenderQueue();
            renderQueue->clear();

            RenderQueueVisibilityStates::iterator visibilityStateIt =
                visibilityState->m_renderQueueVisibilityStates.begin();
            for( ; visibilityStateIt != visibilityState->m_renderQueueVisibilityStates.end();
                 ++visibilityStateIt )
            {
                _addToRenderQueue( ( *visibilityStateIt )->m_sceneNode, cam, renderQueue,
                                   onlyShadowCasters, visibleBounds );
            }

            ++visibilityState->m_nextUpdate;

#endif
        }

        void CellSceneManager::addVisibleObjects( Ogre::SceneNode *sceneNode, Ogre::Camera *cam,
                                                  Ogre::VisibleObjectsBoundsInfo *visibleBounds,
                                                  bool onlyShadowCasters,
                                                  CameraVisibilityState *visibilityState )
        {
            if( visibilityState->isVisible( sceneNode->_getWorldAABB() ) )
            {
                RenderQueueVisibilityState *state;  // =
                //visibilityState->m_renderQueueVisibilityStatePool.getFreeElement();
                if( state )
                {
                    state->m_sceneNode = sceneNode;

                    visibilityState->m_renderQueueVisibilityStates.push_back( state );
                }

                Ogre::Node::ChildNodeIterator childIt = sceneNode->getChildIterator();
                while( childIt.hasMoreElements() )
                {
                    Ogre::SceneNode *child = (Ogre::SceneNode *)childIt.getNext();
                    addVisibleObjects( child, cam, visibleBounds, onlyShadowCasters, visibilityState );
                }
            }
        }

        void CellSceneManager::_addToRenderQueue( Ogre::SceneNode *sceneNode, Ogre::Camera *cam,
                                                  Ogre::RenderQueue *queue, bool onlyShadowCasters,
                                                  Ogre::VisibleObjectsBoundsInfo *visibleBounds )
        {
            Ogre::SceneNode::ObjectIterator it = sceneNode->getAttachedObjectIterator();
            while( it.hasMoreElements() )
            {
                Ogre::MovableObject *mo = it.getNext();
                queue->processVisibleObject( mo, cam, onlyShadowCasters, visibleBounds );
            }
        }

        void CellSceneManager::renderSingleObjectDerived( Ogre::Renderable *rend, const Ogre::Pass *pass,
                                                          bool lightScissoringClipping,
                                                          bool doLightIteration,
                                                          const Ogre::LightList *manualLightList )
        {
            // using namespace Ogre;

            // unsigned short numMatrices;
            // RenderOperation ro;

            // OgreProfileBeginGPUEvent("Material: " + pass->getParent()->getParent()->getName());
            //// Set up rendering operation
            //// I know, I know, const_cast is nasty but otherwise it requires all internal
            //// state of the Renderable assigned to the rop to be mutable
            // const_cast<Renderable*>(rend)->getRenderOperation(ro);
            // ro.srcRenderable = rend;

            // GpuProgram* vprog = pass->hasVertexProgram() ? pass->getVertexProgram().get() : 0;

            // bool passTransformState = true;

            // if (vprog)
            //{
            //	passTransformState = vprog->getPassTransformStates();
            // }

            //// Set world transformation
            // numMatrices = rend->getNumWorldTransforms();

            // if (numMatrices > 0)
            //{
            //	rend->getWorldTransforms(mTempXform);

            //	if (mCameraRelativeRendering && !rend->getUseIdentityView())
            //	{
            //		for (unsigned short i = 0; i < numMatrices; ++i)
            //		{
            //			mTempXform[i].setTrans(mTempXform[i].getTrans() - mCameraRelativePosition);
            //		}
            //	}

            //	if (passTransformState)
            //	{
            //		if (numMatrices > 1)
            //		{
            //			mDestRenderSystem->_setWorldMatrices(mTempXform, numMatrices);
            //		}
            //		else
            //		{
            //			mDestRenderSystem->_setWorldMatrix(*mTempXform);
            //		}
            //	}
            //}
            //// Issue view / projection changes if any
            // useRenderableViewProjMode(rend, passTransformState);

            //// mark per-object params as dirty
            // mGpuParamsDirty |= (uint16)GPV_PER_OBJECT;

            // if (!mSuppressRenderStateChanges)// && m_pass != pass)
            //{
            //	//m_pass = const_cast<Ogre::Pass*>(pass);

            //	bool passSurfaceAndLightParams = true;

            //	if (pass->isProgrammable())
            //	{
            //		// Tell auto params object about the renderable change
            //		mAutoParamDataSource->setCurrentRenderable(rend);
            //		// Tell auto params object about the world matrices, eliminated query from renderable
            //again 		mAutoParamDataSource->setWorldMatrices(mTempXform, numMatrices); 		if (vprog)
            //		{
            //			passSurfaceAndLightParams = vprog->getPassSurfaceAndLightStates();
            //		}
            //	}

            //	// Reissue any texture gen settings which are dependent on view matrix
            //	Pass::ConstTextureUnitStateIterator texIter =  pass->getTextureUnitStateIterator();
            //	size_t unit = 0;
            //	while(texIter.hasMoreElements())
            //	{
            //		TextureUnitState* pTex = texIter.getNext();
            //		if (pTex->hasViewRelativeTextureCoordinateGeneration())
            //		{
            //			mDestRenderSystem->_setTextureUnitSettings(unit, *pTex);
            //		}
            //		++unit;
            //	}

            //	// Sort out normalisation
            //	// Assume first world matrix representative - shaders that use multiple
            //	// matrices should control renormalisation themselves
            //	if ((pass->getNormaliseNormals() || mNormaliseNormalsOnScale)
            //		&& mTempXform[0].hasScale())
            //		mDestRenderSystem->setNormaliseNormals(true);
            //	else
            //		mDestRenderSystem->setNormaliseNormals(false);

            //	// Sort out negative scaling
            //	// Assume first world matrix representative
            //	if (mFlipCullingOnNegativeScale)
            //	{
            //		CullingMode cullMode = mPassCullingMode;

            //		if (mTempXform[0].hasNegativeScale())
            //		{
            //			switch(mPassCullingMode)
            //			{
            //			case CULL_CLOCKWISE:
            //				cullMode = CULL_ANTICLOCKWISE;
            //				break;
            //			case CULL_ANTICLOCKWISE:
            //				cullMode = CULL_CLOCKWISE;
            //				break;
            //			case CULL_NONE:
            //				break;
            //			};
            //		}

            //		// this also copes with returning from negative scale in previous render op
            //		// for same pass
            //		if (cullMode != mDestRenderSystem->_getCullingMode())
            //			mDestRenderSystem->_setCullingMode(cullMode);
            //	}

            //	// Set up the solid / wireframe override
            //	// Precedence is Camera, Object, Material
            //	// Camera might not override object if not overrideable
            //	PolygonMode reqMode = pass->getPolygonMode();
            //	if (pass->getPolygonModeOverrideable() && rend->getPolygonModeOverrideable())
            //	{
            //		PolygonMode camPolyMode = mCameraInProgress->getPolygonMode();
            //		// check camera detial only when render detail is overridable
            //		if (reqMode > camPolyMode)
            //		{
            //			// only downgrade detail; if cam says wireframe we don't go up to solid
            //			reqMode = camPolyMode;
            //		}
            //	}
            //	mDestRenderSystem->_setPolygonMode(reqMode);

            //	if (doLightIteration)
            //	{
            //		// Create local light list for faster light iteration setup
            //		static LightList localLightList;

            //		// Here's where we issue the rendering operation to the render system
            //		// Note that we may do this once per light, therefore it's in a loop
            //		// and the light parameters are updated once per traversal through the
            //		// loop
            //		const LightList& rendLightList = rend->getLights();

            //		bool iteratePerLight = pass->getIteratePerLight();

            //		// deliberately unsigned in case start light exceeds number of lights
            //		// in which case this pass would be skipped
            //		int lightsLeft = 1;
            //		if (iteratePerLight)
            //		{
            //			lightsLeft = static_cast<int>(rendLightList.size()) - pass->getStartLight();
            //			// Don't allow total light count for all iterations to exceed max per pass
            //			if (lightsLeft > static_cast<int>(pass->getMaxSimultaneousLights()))
            //			{
            //				lightsLeft = static_cast<int>(pass->getMaxSimultaneousLights());
            //			}
            //		}

            //		const LightList* pLightListToUse;
            //		// Start counting from the start light
            //		size_t lightIndex = pass->getStartLight();
            //		size_t depthInc = 0;

            //		while (lightsLeft > 0)
            //		{
            //			// Determine light list to use
            //			if (iteratePerLight)
            //			{
            //				// Starting shadow texture index.
            //				size_t shadowTexIndex = mShadowTextures.size();
            //				if (mShadowTextureIndexLightList.size() > lightIndex)
            //					shadowTexIndex = mShadowTextureIndexLightList[lightIndex];

            //				localLightList.resize(pass->getLightCountPerIteration());

            //				LightList::iterator destit = localLightList.begin();
            //				unsigned short numShadowTextureLights = 0;
            //				for (; destit != localLightList.end()
            //					&& lightIndex < rendLightList.size();
            //					++lightIndex, --lightsLeft)
            //				{
            //					Light* currLight = rendLightList[lightIndex];

            //					// Check whether we need to filter this one out
            //					if ((pass->getRunOnlyForOneLightType() &&
            //						pass->getOnlyLightType() != currLight->getType()) ||
            //						(pass->getLightMask() & currLight->getLightMask()) == 0)
            //					{
            //						// Skip
            //						// Also skip shadow texture(s)
            //						if (isShadowTechniqueTextureBased())
            //						{
            //							shadowTexIndex +=
            //mShadowTextureCountPerType[currLight->getType()];
            //						}
            //						continue;
            //					}

            //					*destit++ = currLight;

            //					// potentially need to update content_type shadow texunit
            //					// corresponding to this light
            //					if (isShadowTechniqueTextureBased())
            //					{
            //						size_t textureCountPerLight =
            //mShadowTextureCountPerType[currLight->getType()]; 						for (size_t j = 0; j <
            //textureCountPerLight && shadowTexIndex < mShadowTextures.size(); ++j)
            //						{
            //							// link the numShadowTextureLights'th shadow texture unit
            //							unsigned short tuindex =
            //								pass->_getTextureUnitWithContentTypeIndex(
            //								TextureUnitState::CONTENT_SHADOW, numShadowTextureLights);
            //							if (tuindex > pass->getNumTextureUnitStates()) break;

            //							// I know, nasty const_cast
            //							TextureUnitState* tu =
            //								const_cast<TextureUnitState*>(
            //								pass->getTextureUnitState(tuindex));
            //							const Ogre::TexturePtr& shadowTex =
            //mShadowTextures[shadowTexIndex]; 							tu->_setTexturePtr(shadowTex); 							Camera *cam =
            //shadowTex->getBuffer()->getRenderTarget()->getViewport(0)->getCamera();
            //							tu->setProjectiveTexturing(!pass->hasVertexProgram(), cam);
            //							mAutoParamDataSource->setTextureProjector(cam,
            //numShadowTextureLights);
            //							++numShadowTextureLights;
            //							++shadowTexIndex;
            //							// Have to set TU on rendersystem right now, although
            //							// autoparams will be set later
            //							mDestRenderSystem->_setTextureUnitSettings(tuindex, *tu);
            //						}
            //					}

            //				}
            //				// Did we run out of lights before slots? e.g. 5 lights, 2 per iteration
            //				if (destit != localLightList.end())
            //				{
            //					localLightList.erase(destit, localLightList.end());
            //					lightsLeft = 0;
            //				}
            //				pLightListToUse = &localLightList;

            //				// deal with the case where we found no lights
            //				// since this is light iteration, we shouldn't render at all
            //				if (pLightListToUse->empty())
            //					return;

            //			}
            //			else // !iterate per light
            //			{
            //				// Use complete light list potentially adjusted by start light
            //				if (pass->getStartLight() || pass->getMaxSimultaneousLights() !=
            //OGRE_MAX_SIMULTANEOUS_LIGHTS || 					pass->getLightMask() != 0xFFFFFFFF)
            //				{
            //					// out of lights?
            //					// skip manual 2nd lighting passes onwards if we run out of lights, but never the
            //first one 					if (pass->getStartLight() > 0 && 						pass->getStartLight() >= rendLightList.size())
            //					{
            //						break;
            //					}
            //					else
            //					{
            //						localLightList.clear();
            //						LightList::const_iterator copyStart = rendLightList.begin();
            //						std::advance(copyStart, pass->getStartLight());
            //						// Clamp lights to copy to avoid overrunning the end of the list
            //						size_t lightsCopied = 0, lightsToCopy = std::min(
            //							static_cast<size_t>(pass->getMaxSimultaneousLights()),
            //							rendLightList.size() - pass->getStartLight());

            //						//localLightList.insert(localLightList.begin(),
            //						//	copyStart, copyEnd);

            //						// Copy lights over
            //						for(LightList::const_iterator iter = copyStart; iter != rendLightList.end() &&
            //lightsCopied < lightsToCopy; ++iter)
            //						{
            //							if((pass->getLightMask() & (*iter)->getLightMask()) != 0)
            //							{
            //								localLightList.push_back(*iter);
            //								lightsCopied++;
            //							}
            //						}

            //						pLightListToUse = &localLightList;
            //					}
            //				}
            //				else
            //				{
            //					pLightListToUse = &rendLightList;
            //				}
            //				lightsLeft = 0;
            //			}

            //			fireRenderSingleObject(rend, pass, mAutoParamDataSource, pLightListToUse,
            //mSuppressRenderStateChanges);

            //			// Do we need to update GPU program parameters?
            //			if (pass->isProgrammable())
            //			{
            //				useLightsGpuProgram(pass, pLightListToUse);
            //			}
            //			// Do we need to update light states?
            //			// Only do this if fixed-function vertex lighting applies
            //			if (pass->getLightingEnabled() && passSurfaceAndLightParams)
            //			{
            //				useLights(*pLightListToUse, pass->getMaxSimultaneousLights());
            //			}
            //			// optional light scissoring & clipping
            //			ClipResult scissored = CLIPPED_NONE;
            //			ClipResult clipped = CLIPPED_NONE;
            //			if (lightScissoringClipping &&
            //				(pass->getLightScissoringEnabled() || pass->getLightClipPlanesEnabled()))
            //			{
            //				// if there's no lights hitting the scene, then we might as
            //				// well stop since clipping cannot include anything
            //				if (pLightListToUse->empty())
            //					continue;

            //				if (pass->getLightScissoringEnabled())
            //					scissored = buildAndSetScissor(*pLightListToUse, mCameraInProgress);

            //				if (pass->getLightClipPlanesEnabled())
            //					clipped = buildAndSetLightClip(*pLightListToUse);

            //				if (scissored == CLIPPED_ALL || clipped == CLIPPED_ALL)
            //					continue;
            //			}
            //			// issue the render op
            //			// nfz: check for gpu_multipass
            //			mDestRenderSystem->setCurrentPassIterationCount(pass->getPassIterationCount());
            //			// We might need to update the depth bias each iteration
            //			if (pass->getIterationDepthBias() != 0.0f)
            //			{
            //				float depthBiasBase = pass->getDepthBiasConstant() +
            //					pass->getIterationDepthBias() * depthInc;
            //				// depthInc deals with light iteration

            //				// Note that we have to set the depth bias here even if the depthInc
            //				// is zero (in which case you would think there is no change from
            //				// what was set in _setPass(). The reason is that if there are
            //				// multiple Renderables with this Pass, we won't go through _setPass
            //				// again at the start of the iteration for the next Renderable
            //				// because of Pass state grouping. So set it always

            //				// Set modified depth bias right away
            //				mDestRenderSystem->_setDepthBias(depthBiasBase,
            //pass->getDepthBiasSlopeScale());

            //				// Set to increment internally too if rendersystem iterates
            //				mDestRenderSystem->setDeriveDepthBias(true,
            //					depthBiasBase, pass->getIterationDepthBias(),
            //					pass->getDepthBiasSlopeScale());
            //			}
            //			else
            //			{
            //				mDestRenderSystem->setDeriveDepthBias(false);
            //			}
            //			depthInc += pass->getPassIterationCount();

            //			// Finalise GPU parameter bindings
            //			updateGpuProgramParameters(pass);

            //			if (rend->preRender(this, mDestRenderSystem))
            //				mDestRenderSystem->_render(ro);
            //			rend->postRender(this, mDestRenderSystem);

            //			if (scissored == CLIPPED_SOME)
            //				resetScissor();
            //			if (clipped == CLIPPED_SOME)
            //				resetLightClip();
            //		} // possibly iterate per light
            //	}
            //	else // no automatic light processing
            //	{
            //		// Even if manually driving lights, check light type passes
            //		bool skipBecauseOfLightType = false;
            //		if (pass->getRunOnlyForOneLightType())
            //		{
            //			if (!manualLightList ||
            //				(manualLightList->size() == 1 &&
            //				manualLightList->at(0)->getType() != pass->getOnlyLightType()))
            //			{
            //				skipBecauseOfLightType = true;
            //			}
            //		}

            //		if (!skipBecauseOfLightType)
            //		{
            //			fireRenderSingleObject(rend, pass, mAutoParamDataSource, manualLightList,
            //mSuppressRenderStateChanges);
            //			// Do we need to update GPU program parameters?
            //			if (pass->isProgrammable())
            //			{
            //				// Do we have a manual light list?
            //				if (manualLightList)
            //				{
            //					useLightsGpuProgram(pass, manualLightList);
            //				}

            //			}

            //			// Use manual lights if present, and not using vertex programs that don't use fixed
            //pipeline 			if (manualLightList && 				pass->getLightingEnabled() && passSurfaceAndLightParams)
            //			{
            //				useLights(*manualLightList, pass->getMaxSimultaneousLights());
            //			}

            //			// optional light scissoring
            //			ClipResult scissored = CLIPPED_NONE;
            //			ClipResult clipped = CLIPPED_NONE;
            //			if (lightScissoringClipping && manualLightList &&
            //pass->getLightScissoringEnabled())
            //			{
            //				scissored = buildAndSetScissor(*manualLightList, mCameraInProgress);
            //			}
            //			if (lightScissoringClipping && manualLightList &&
            //pass->getLightClipPlanesEnabled())
            //			{
            //				clipped = buildAndSetLightClip(*manualLightList);
            //			}

            //			// don't bother rendering if clipped / scissored entirely
            //			if (scissored != CLIPPED_ALL && clipped != CLIPPED_ALL)
            //			{
            //				// issue the render op
            //				// nfz: set up multipass rendering
            //				mDestRenderSystem->setCurrentPassIterationCount(pass->getPassIterationCount());
            //				// Finalise GPU parameter bindings
            //				updateGpuProgramParameters(pass);

            //				if (rend->preRender(this, mDestRenderSystem))
            //					mDestRenderSystem->_render(ro);
            //				rend->postRender(this, mDestRenderSystem);
            //			}
            //			if (scissored == CLIPPED_SOME)
            //				resetScissor();
            //			if (clipped == CLIPPED_SOME)
            //				resetLightClip();

            //		} // !skipBecauseOfLightType
            //	}

            //}
            // else // mSuppressRenderStateChanges
            //{
            //	fireRenderSingleObject(rend, pass, mAutoParamDataSource, NULL,
            //mSuppressRenderStateChanges);
            //	// Just render
            //	mDestRenderSystem->setCurrentPassIterationCount(1);
            //	if (rend->preRender(this, mDestRenderSystem))
            //		mDestRenderSystem->_render(ro);
            //	rend->postRender(this, mDestRenderSystem);
            //}

            //// Reset view / projection changes if any
            // resetViewProjMode(passTransformState);
            // OgreProfileEndGPUEvent("Material: " + pass->getParent()->getParent()->getName());
        }

        CellSceneManagerFactory::CellSceneManagerFactory()
        {
        }

        CellSceneManagerFactory::~CellSceneManagerFactory()
        {
        }

        // Ogre::SceneNode* CellSceneManager::createSceneNodeImpl(void)
        //{
        //	//return new (m_sceneNodePool.malloc()) Ogre::SceneNode(this);
        //	//return new (m_sceneNodePool.malloc()) Ogre::SceneNode(this);
        //	return new Ogre::SceneNode(this);
        //}
        //
        // Ogre::SceneNode* CellSceneManager::createSceneNodeImpl(const Ogre::String& name)
        //{
        //	//return new (m_sceneNodePool.malloc()) Ogre::SceneNode(this, name);
        //	return 0;
        // }

        const Ogre::String CellSceneManagerFactory::FACTORY_TYPE_NAME = "CellSceneManager";

        void CellSceneManagerFactory::initMetaData( void ) const
        {
            // mMetaData.typeName = FACTORY_TYPE_NAME;
            // mMetaData.description = "Scene manager organising the scene on the basis of a cell.";
            // mMetaData.sceneTypeMask = 0xFFFF; // support all types
            // mMetaData.worldGeometrySupported = false;
        }

        Ogre::SceneManager *CellSceneManagerFactory::createInstance( const Ogre::String &instanceName )
        {
            return OGRE_NEW CellSceneManager( instanceName );
        }

        void CellSceneManagerFactory::destroyInstance( Ogre::SceneManager *instance )
        {
            OGRE_DELETE instance;
        }

        CellSceneManager::CullThread::CullThread( CellSceneManager *owner ) : m_owner( owner )
        {
        }

        void CellSceneManager::CullThread::operator()()
        {
            while( true )
            {
                // Sleep(1000);
            }
        }

    }  // namespace render
}  // namespace workphone
