#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "WPGraphicsOgre/Jobs/SceneCullJob.hpp"
#include "WPGraphicsOgre/Addons/CameraVisibilityState.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {

        SceneCullJob::SceneCullJob() : m_visibilityState( nullptr ), m_renderQueue( nullptr )
        {
        }

        SceneCullJob::~SceneCullJob()
        {
            m_visibilityState->setState( CameraVisibilityState::STATE_READY );
        }

        void SceneCullJob::update()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto timer = applicationManager->getTimer();

            auto task = Thread::getCurrentTask();
            auto t = timer->getTime();
            auto dt = timer->getDeltaTime();

            switch( task )
            {
            case TaskId::Render:
            {
                findVisible();
            }
            break;
            default:
            {
                findVisible();
            }
            }
        }

        void SceneCullJob::_addToRenderQueue( Ogre::SceneNode *sceneNode, Ogre::Camera *cam,
                                              Ogre::RenderQueue *queue, bool onlyShadowCasters,
                                              Ogre::VisibleObjectsBoundsInfo *visibleBounds )
        {
            Ogre::SceneNode::ObjectIterator it = sceneNode->getAttachedObjectIterator();
            while( it.hasMoreElements() )
            {
                Ogre::MovableObject *mo = it.getNext();
                // if((mo->getTypeFlags() & Ogre::SceneManager::FRUSTUM_TYPE_MASK))
                //	continue;

                // Ogre::Camera* pCamera = dynamic_cast<Ogre::Camera*>(mo);
                // if(pCamera)
                //	continue;

                queue->processVisibleObject( mo, cam, onlyShadowCasters, visibleBounds );
            }
        }

        u32 SceneCullJob::getProgress() const
        {
            return 0;
        }

        void SceneCullJob::setProgress( u32 progress )
        {
        }

        s32 SceneCullJob::getPriority() const
        {
            return 0;
        }

        void SceneCullJob::setPriority( s32 priority )
        {
        }

        CameraVisibilityState *SceneCullJob::getVisibilityState() const
        {
            return m_visibilityState;
        }

        void SceneCullJob::setVisibilityState( CameraVisibilityState *visibilityState )
        {
            m_visibilityState = visibilityState;
            m_visibilityState->setState( CameraVisibilityState::STATE_QUEUED );
        }

        Ogre::SceneManager *SceneCullJob::getSceneManager() const
        {
            return m_sceneManager;
        }

        void SceneCullJob::setSceneManager( Ogre::SceneManager *sceneManager )
        {
            m_sceneManager = sceneManager;
        }

        Ogre::Camera *SceneCullJob::getCamera() const
        {
            return m_camera;
        }

        void SceneCullJob::setCamera( Ogre::Camera *camera )
        {
            m_camera = camera;
        }

        Ogre::VisibleObjectsBoundsInfo *SceneCullJob::getVisibleBounds() const
        {
            return m_visibleBounds;
        }

        void SceneCullJob::setVisibleBounds( Ogre::VisibleObjectsBoundsInfo *visibleBounds )
        {
            m_visibleBounds = visibleBounds;
        }

        bool SceneCullJob::getOnlyShadowCasters() const
        {
            return m_onlyShadowCasters;
        }

        void SceneCullJob::setOnlyShadowCasters( bool onlyShadowCasters )
        {
            m_onlyShadowCasters = onlyShadowCasters;
        }

        Ogre::RenderQueue *SceneCullJob::getRenderQueue() const
        {
            return m_renderQueue;
        }

        void SceneCullJob::setRenderQueue( Ogre::RenderQueue *renderQueue )
        {
            m_renderQueue = renderQueue;
        }

        void SceneCullJob::addVisibleObjects( Ogre::SceneNode *sceneNode, Ogre::Camera *cam,
                                              Ogre::VisibleObjectsBoundsInfo *visibleBounds,
                                              bool onlyShadowCasters,
                                              CameraVisibilityState *visibilityState )
        {
            if( visibilityState->isVisible( sceneNode->_getWorldAABB() ) )
            {
                RenderQueueVisibilityState *state = new RenderQueueVisibilityState;
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

        void SceneCullJob::findVisible()
        {
            // RecursiveMutex::ScopedLock lock(OgreMutex);

            static bool updateQueue = false;

            if( updateQueue || m_visibilityState->getUpdateCulling() )
            {
                {
                    RenderQueueVisibilityStates::iterator visibilityStateIt =
                        m_visibilityState->m_renderQueueVisibilityStates.begin();
                    for( ; visibilityStateIt != m_visibilityState->m_renderQueueVisibilityStates.end();
                         ++visibilityStateIt )
                    {
                        RenderQueueVisibilityState *renderQueueVisibilityState = ( *visibilityStateIt );
                        // RenderQueueVisibilityStatePool::free(renderQueueVisibilityState);
                    }

                    m_visibilityState->m_renderQueueVisibilityStates.clear();

                    addVisibleObjects( m_sceneManager->getRootSceneNode(), m_camera, m_visibleBounds,
                                       m_onlyShadowCasters, m_visibilityState );

                    m_visibilityState->m_nextUpdate = 0;
                }

                Ogre::RenderQueue *renderQueue = getRenderQueue();
                renderQueue->clear();

                {
                    ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
                    RenderQueueVisibilityStates::iterator visibilityStateIt =
                        m_visibilityState->m_renderQueueVisibilityStates.begin();
                    for( ; visibilityStateIt != m_visibilityState->m_renderQueueVisibilityStates.end();
                         ++visibilityStateIt )
                    {
                        _addToRenderQueue( ( *visibilityStateIt )->m_sceneNode, m_camera, renderQueue,
                                           m_onlyShadowCasters, m_visibleBounds );
                    }
                }
            }
        }

    }  // namespace render
}  // namespace workphone
