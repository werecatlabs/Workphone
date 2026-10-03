#include "WPGraphics/WPClawHammerPCH.hpp"
#include "WPGraphics/Jobs/SceneNodeCullJob.hpp"
#include "WPGraphics/Jobs/CameraVisibilitySet.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {

        WP_CLASS_REGISTER_DERIVED( workphone::render, SceneNodeCullJob, Job );

        SceneNodeCullJob::SceneNodeCullJob() = default;

        SceneNodeCullJob::~SceneNodeCullJob() = default;

        void SceneNodeCullJob::execute()
        {
            // Resolve the frustum to test against. When no frustum is set the
            // pass degrades to "nothing is culled" and the cull flag is simply
            // cleared on every visited node.
            IFrustum *frustum = m_frustum.get();

            // Resolve the active per-camera visibility set so it reflects only
            // the current frame's results. The set is created lazily on demand
            // when setCamera() was called or getOrCreateVisibilitySet() was
            // invoked before scheduling the job. When no camera is bound no
            // set is touched here (cullNode() will simply skip appending).
            if( m_camera )
            {
                const hash_type cameraId = m_camera->getId();
                auto found = m_visibilitySets.tryGet( cameraId, m_activeVisibilitySet );
                if( found )
                {
                    m_activeVisibilitySet->clear();
                }
                else
                {
                    m_activeVisibilitySet = nullptr;
                }
            }
            else
            {
                m_activeVisibilitySet = nullptr;
            }

            // Build the set of starting nodes. When an explicit node list has
            // been supplied it takes precedence over the root node; otherwise
            // the root node is used as the single starting node.
            auto startNodes = m_nodes.snapshot();
            if( startNodes.empty() )
            {
                if( auto root = m_rootNode )
                {
                    startNodes.push_back( root );
                }
            }

            const bool recursive = m_recursive.get();

            for( auto &node : startNodes )
            {
                cullNode( node.get(), frustum, recursive );
            }
        }

        SmartPtr<IGraphicsScene> SceneNodeCullJob::getScene() const
        {
            return m_scene;
        }

        void SceneNodeCullJob::setScene( SmartPtr<IGraphicsScene> scene )
        {
            m_scene = scene;

            if( scene )
            {
                m_rootNode = scene->getRootSceneNode();
            }
            else
            {
                m_rootNode = nullptr;
            }
        }

        SmartPtr<IGraphicsSceneNode> SceneNodeCullJob::getRootNode() const
        {
            return m_rootNode;
        }

        void SceneNodeCullJob::setRootNode( SmartPtr<IGraphicsSceneNode> rootNode )
        {
            m_rootNode = rootNode;
        }

        SmartPtr<IFrustum> SceneNodeCullJob::getFrustum() const
        {
            return m_frustum;
        }

        void SceneNodeCullJob::setFrustum( SmartPtr<IFrustum> frustum )
        {
            m_frustum = frustum;
        }

        Array<SmartPtr<IGraphicsSceneNode>> SceneNodeCullJob::getNodes() const
        {
            return m_nodes.snapshot();
        }

        void SceneNodeCullJob::setNodes( const Array<SmartPtr<IGraphicsSceneNode>> &nodes )
        {
            m_nodes = { nodes.begin(), nodes.end() };
        }

        bool SceneNodeCullJob::getRecursive() const
        {
            return m_recursive.get();
        }

        void SceneNodeCullJob::setRecursive( bool recursive )
        {
            m_recursive = recursive;
        }

        u32 SceneNodeCullJob::getCullFlag() const
        {
            return m_cullFlag.get();
        }

        void SceneNodeCullJob::setCullFlag( u32 flag )
        {
            m_cullFlag = flag;
        }

        bool SceneNodeCullJob::isCulled( const SmartPtr<IGraphicsSceneNode> &node ) const
        {
            if( !node )
            {
                return false;
            }

            const auto flag = getCullFlag();
            if( flag == 0 )
            {
                return false;
            }

            // The flag API lives on the concrete GraphicsSceneNode. Nodes that
            // do not derive from it have no cull state and are treated as not
            // culled so that lazy consumers simply skip them.
            auto *sceneNode = dynamic_cast<GraphicsSceneNode *>( node.get() );
            if( !sceneNode )
            {
                return false;
            }

            return sceneNode->getFlag( flag );
        }

        SmartPtr<IGraphicsCamera> SceneNodeCullJob::getCamera() const
        {
            return m_camera;
        }

        void SceneNodeCullJob::setCamera( SmartPtr<IGraphicsCamera> camera )
        {
            m_camera = camera;
            m_activeVisibilitySet = nullptr;

            if( camera )
            {
                // Ensure a visibility set exists for this camera and cache it
                // as the active set so execute() does not need to look it up.
                m_activeVisibilitySet = getOrCreateVisibilitySet( camera );
            }
        }

        SmartPtr<CameraVisibilitySet> SceneNodeCullJob::getVisibilitySet(
            const SmartPtr<IGraphicsCamera> &camera ) const
        {
            if( !camera )
            {
                return nullptr;
            }

            SmartPtr<CameraVisibilitySet> set;
            if( m_visibilitySets.tryGet( camera->getId(), set ) )
            {
                return set;
            }
            return nullptr;
        }

        SmartPtr<CameraVisibilitySet> SceneNodeCullJob::getActiveVisibilitySet() const
        {
            return m_activeVisibilitySet;
        }

        SmartPtr<CameraVisibilitySet> SceneNodeCullJob::getOrCreateVisibilitySet(
            const SmartPtr<IGraphicsCamera> &camera )
        {
            if( !camera )
            {
                return nullptr;
            }

            const hash_type cameraId = camera->getId();
            SmartPtr<CameraVisibilitySet> set;
            if( m_visibilitySets.tryGet( cameraId, set ) && set )
            {
                return set;
            }

            // Create a fresh set and insert it into the per-camera map. The
            // ConcurrentHashmapBase::operator[] already handles insertion under
            // an exclusive lock so concurrent calls for the same camera are
            // safe.
            auto factoryManager = core::IApplicationManager::instancePtr();
            if( factoryManager )
            {
                if( auto appFactory = factoryManager->getFactoryManagerPtr() )
                {
                    set = appFactory->make_ptr<CameraVisibilitySet>( cameraId );
                }
            }

            if( !set )
            {
                set = workphone::make_ptr<CameraVisibilitySet>( cameraId );
            }

            m_visibilitySets[cameraId] = set;
            return set;
        }

        void SceneNodeCullJob::clearVisibilitySets()
        {
            m_visibilitySets.clear();
            m_activeVisibilitySet = nullptr;
        }

        void SceneNodeCullJob::cullNode( IGraphicsSceneNode *node, IFrustum *frustum, bool recursive )
        {
            if( !node )
            {
                return;
            }

            const auto flag = getCullFlag();

            // Determine visibility. With no frustum configured nothing is
            // considered culled, so the flag is always cleared.
            bool visible = true;

            // The flag API lives on the concrete GraphicsSceneNode. Only nodes
            // that derive from it can carry the cull flag; interface-only nodes
            // are visited for traversal but skip the flag write.
            auto *sceneNode = dynamic_cast<GraphicsSceneNode *>( node );
            if( flag != 0 && sceneNode )
            {
                if( frustum )
                {
                    const auto worldAABB = node->getWorldAABB();
                    visible = frustum->isObjectVisible( worldAABB );
                }

                sceneNode->setFlag( flag, !visible );
            }

            // Append the node to the active per-camera visibility set when it
            // was found visible. The set is only present when setCamera() (or
            // getOrCreateVisibilitySet()) has been used; when no set is bound
            // the per-node flag above is the only result that is recorded.
            if( visible && m_activeVisibilitySet )
            {
                m_activeVisibilitySet->addVisible( SmartPtr<IGraphicsSceneNode>( node ) );
            }

            if( recursive )
            {
                cullNodeRecursive( node, frustum );
            }
        }

        void SceneNodeCullJob::cullNodeRecursive( IGraphicsSceneNode *node, IFrustum *frustum )
        {
            if( !node )
            {
                return;
            }

            // Take a snapshot of the children so the traversal is robust to
            // concurrent structural changes to the graph.
            const auto children = node->getChildren();
            for( auto &child : children )
            {
                cullNode( child.get(), frustum, true );
            }
        }

    }  // namespace render
}  // namespace workphone
