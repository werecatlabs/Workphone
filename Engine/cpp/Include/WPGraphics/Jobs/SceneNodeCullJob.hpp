#ifndef SceneNodeCullJob_h__
#define SceneNodeCullJob_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IFrustum.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentHashMap.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @name Scene node cull flag bits.
         *
         * These bitmasks are intended to be used with
         * GraphicsSceneNode::setFlag / getFlag so that the result of a culling
         * pass can be queried lazily by later work running on the job queue.
         * @{
         */

        /**
         * @brief Flag bit set on a scene node when it has been culled (determined
         *        to be outside the viewing frustum) by a SceneNodeCullJob pass.
         *
         * The flag is cleared when the node is found to be visible. Consumers
         * that run after the cull job on the queue can read this flag with
         * GraphicsSceneNode::getFlag( SceneNodeCullFlag_Culled ) to decide
         * whether to skip processing for the node.
         */
        static const u32 SceneNodeCullFlag_Culled = ( 1 << 0 );

        /**
         * @brief Flag bit set on a scene node while a culling pass is in progress
         *        for that node.
         *
         * This allows a lazy update running on the job queue to detect that the
         * culling state is being (re)evaluated and avoid acting on stale results.
         */
        static const u32 SceneNodeCullFlag_Pending = ( 1 << 1 );

        /** @} */

        /**
         * @class SceneNodeCullJob
         * @brief Job that culls scene graph nodes against a camera frustum.
         *
         * SceneNodeCullJob traverses the scene graph starting from a root node
         * (typically the scene's root scene node) and, for each node, tests the
         * node's world-space axis-aligned bounding box against a frustum
         * (IFrustum). Depending on the result it sets or clears the
         * @ref SceneNodeCullFlag_Culled flag on the node.
         *
         * Because the result is written into the node's flag bitmask, no
         * dedicated output collection is required: a lazy update that runs
         * later on the job queue can simply query
         * `SceneNodeCullJob::isCulled( node )` (or
         * `GraphicsSceneNode::getFlag( SceneNodeCullFlag_Culled )`) to decide
         * whether to skip rendering or further processing for that node.
         *
         * Typical usage:
         * - Create a SceneNodeCullJob instance.
         * - Configure the source nodes via `setScene` (resolves the root node
         *   from a scene), `setRootNode`, or `setNodes` for an explicit list.
         * - Set the frustum used for visibility tests with `setFrustum`.
         * - Submit the job to the job system; `execute` will be invoked by the
         *   worker and the culled flag will be updated on every visited node.
         *
         * Threading: execution occurs on whatever thread the job system
         * schedules the job on. The visited nodes and the frustum must be safe
         * to read from that thread or be properly synchronized. The per-node
         * flag writes go through the node's own locked state data accessor and
         * are therefore safe with respect to other readers of the flags.
         *
         * @note The cull flag is stored on the concrete GraphicsSceneNode (the
         *       flag API is not part of IGraphicsSceneNode). Nodes that do not
         *       derive from GraphicsSceneNode are visited for traversal but do
         *       not carry a cull flag and are reported as not culled by
         *       `isCulled`.
         */

        class WPGraphics_API SceneNodeCullJob : public Job
        {
        public:
            /**
             * @brief Construct a new SceneNodeCullJob.
             *
             * Initializes internal state. The root node and frustum are
             * initially empty and should be configured before scheduling the
             * job. The cull flag bit defaults to
             * @ref SceneNodeCullFlag_Culled and recursion is enabled by default.
             */
            SceneNodeCullJob();

            /**
             * @brief Destroy the SceneNodeCullJob.
             *
             * Default destructor ensures proper cleanup of smart pointers.
             */
            ~SceneNodeCullJob() override;

            /**
             * @brief Execute the culling pass.
             *
             * Called by the job system when the job runs. If a node list has
             * been supplied via `setNodes`, only those nodes (and, when
             * `getRecursive` is true, their descendants) are visited.
             * Otherwise the root node (obtained from `getRootNode`, or resolved
             * from the scene set via `setScene`) is used and, when
             * `getRecursive` is true, the entire subtree below it is traversed.
             *
             * For every visited node the world AABB is tested against the
             * frustum (IFrustum::isObjectVisible) and the cull flag bit is set
             * when the node is outside the frustum and cleared when it is
             * visible. If no frustum is set the flag is cleared on every
             * visited node (nothing is culled).
             */
            void execute() override;

            /**
             * @brief Get the scene used to resolve the root node.
             * @return SmartPtr<IGraphicsScene> The scene, or an empty pointer
             *         if none has been set.
             */
            SmartPtr<IGraphicsScene> getScene() const;

            /**
             * @brief Set the scene whose root node will be traversed.
             * @param scene Smart pointer to a graphics scene. The job stores
             *               the scene and resolves its root scene node as the
             *               traversal root (equivalent to calling
             *               `setRootNode( scene->getRootSceneNode() )`).
             *
             * If the scene is null or has no root node, the traversal root is
             * cleared.
             */
            void setScene( SmartPtr<IGraphicsScene> scene );

            /**
             * @brief Get the root scene node used for the traversal.
             * @return SmartPtr<IGraphicsSceneNode> The root node, or an empty
             *         pointer if none has been set.
             */
            SmartPtr<IGraphicsSceneNode> getRootNode() const;

            /**
             * @brief Set the root scene node for the traversal.
             * @param rootNode Smart pointer to the root scene node. When
             *                 non-null and `getRecursive` is true, the
             *                 traversal descends into the node's children.
             */
            void setRootNode( SmartPtr<IGraphicsSceneNode> rootNode );

            /**
             * @brief Get the frustum used for visibility tests.
             * @return SmartPtr<IFrustum> The frustum, or an empty pointer if
             *         none has been set.
             */
            SmartPtr<IFrustum> getFrustum() const;

            /**
             * @brief Set the frustum used for visibility tests.
             * @param frustum Smart pointer to a frustum (commonly a camera).
             *                 When empty, no culling is performed and the cull
             *                 flag is cleared on every visited node.
             */
            void setFrustum( SmartPtr<IFrustum> frustum );

            /**
             * @brief Get the explicit list of nodes to cull.
             * @return Array<SmartPtr<IGraphicsSceneNode>> The list of nodes set
             *         via `setNodes`, or an empty array when none has been set.
             *
             * When non-empty, the job only visits these nodes (and, when
             * `getRecursive` is true, their descendants) instead of the root
             * node.
             */
            Array<SmartPtr<IGraphicsSceneNode>> getNodes() const;

            /**
             * @brief Set an explicit list of nodes to cull.
             * @param nodes The list of nodes to visit. Pass an empty array to
             *               fall back to using the root node.
             */
            void setNodes( const Array<SmartPtr<IGraphicsSceneNode>> &nodes );

            /**
             * @brief Query whether the traversal recurses into child nodes.
             * @return True when the traversal descends into children, false
             *         when only the supplied / root nodes are tested.
             */
            bool getRecursive() const;

            /**
             * @brief Enable or disable recursive traversal into children.
             * @param recursive True to descend into child nodes (default),
             *                  false to only test the supplied / root nodes.
             */
            void setRecursive( bool recursive );

            /**
             * @brief Get the flag bitmask used to mark a node as culled.
             * @return The bitmask that is set/cleared on each visited node.
             *
             * Defaults to @ref SceneNodeCullFlag_Culled.
             */
            u32 getCullFlag() const;

            /**
             * @brief Set the flag bitmask used to mark a node as culled.
             * @param flag The bitmask to set on a node when it is culled (and
             *             cleared when it is visible). Use this to override the
             *             default @ref SceneNodeCullFlag_Culled bit, for example
             *             to combine the cull state with other user flags.
             */
            void setCullFlag( u32 flag );

            /**
             * @brief Convenience helper to query the culled state of a node.
             * @param node The node to query. Null is treated as not culled.
             * @return True when the node's cull flag is set (the node was
             *         culled by the most recent pass), false otherwise.
             *
             * This reads the cull flag via the concrete GraphicsSceneNode flag
             * API so that lazy consumers on the job queue do not need to know
             * the exact flag bit in use. Nodes that do not derive from
             * GraphicsSceneNode are reported as not culled.
             */

            /**
             * @brief Get the camera whose frustum is being used for the cull.
             * @return SmartPtr<IGraphicsCamera> The camera, or an empty pointer
             *         when none has been set.
             *
             * When a camera is supplied (typically via `setCamera` or
             * `dispatchCullingJob` on the owning scene), the job manages a
             * per-camera visibility set in addition to the per-node cull flag.
             */
            SmartPtr<IGraphicsCamera> getCamera() const;

            /**
             * @brief Set the camera whose frustum is used for the cull.
             * @param camera Smart pointer to a camera. When non-null, the job
             *                resolves `camera->getId()` as the key for the
             *                per-camera visibility set and populates it with
             *                the visible scene nodes during `execute`.
             *
             * This is a convenience that pairs with `setFrustum`: callers that
             * already hold a camera can set both and the job will populate the
             * camera's visibility set instead of (or in addition to) just
             * writing per-node flags.
             */
            void setCamera( SmartPtr<IGraphicsCamera> camera );

            /**
             * @brief Get the visibility set for a specific camera.
             * @param camera The camera whose visibility set should be queried.
             * @return SmartPtr<CameraVisibilitySet> The set, or an empty pointer
             *         when no set has been created for that camera (because no
             *         cull pass has been run for it yet).
             */
            SmartPtr<CameraVisibilitySet> getVisibilitySet(
                const SmartPtr<IGraphicsCamera> &camera ) const;

            /**
             * @brief Get the visibility set for the camera currently bound via
             *        `setCamera`.
             * @return SmartPtr<CameraVisibilitySet> The set for the bound
             *         camera, or an empty pointer when no camera is bound or
             *         no set exists yet.
             */
            SmartPtr<CameraVisibilitySet> getActiveVisibilitySet() const;

            /**
             * @brief Get (or create) the visibility set for a specific camera.
             * @param camera The camera whose visibility set should be
             *                retrieved/created.
             * @return SmartPtr<CameraVisibilitySet> The set for `camera`. When
             *         no set existed for that camera yet a fresh empty set is
             *         created, added to the job's per-camera map, and returned.
             *
             * This is the primary hook used by `dispatchCullingJob` (and by
             * advanced callers) to ensure a per-camera visibility set exists
             * before submitting the job to the worker thread.
             */
            SmartPtr<CameraVisibilitySet> getOrCreateVisibilitySet(
                const SmartPtr<IGraphicsCamera> &camera );

            /**
             * @brief Remove every cached per-camera visibility set.
             *
             * Useful when a scene is being torn down or when a caller wants to
             * force a clean slate. Does not affect the per-node cull flags.
             */
            void clearVisibilitySets();
            bool isCulled( const SmartPtr<IGraphicsSceneNode> &node ) const;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Visit a single node and update its cull flag.
             *
             * Tests the node's world AABB against the frustum and sets or
             * clears the cull flag accordingly. When `recursive` is true the
             * traversal then descends into the node's children.
             *
             * @param node The node to test. Safe to call with a null pointer
             *              (no-op).
             * @param frustum Raw pointer to the frustum to test against, or
             *                null to force the node to be treated as visible
             *                (flag cleared).
             * @param recursive Whether to recurse into children.
             */
            void cullNode( IGraphicsSceneNode *node, IFrustum *frustum, bool recursive );

            /**
             * @brief Recursively traverse the subtree rooted at `node`.
             *
             * @param node The node to test and recurse from.
             * @param frustum Raw pointer to the frustum to test against.
             */
            void cullNodeRecursive( IGraphicsSceneNode *node, IFrustum *frustum );

            /**
             * @brief The scene whose root node is traversed (or empty). Kept as
             *        a smart pointer so the scene outlives the job.
             */
            SmartPtr<IGraphicsScene> m_scene;

            /**
             * @brief The root scene node for the traversal (or empty).
             */
            SmartPtr<IGraphicsSceneNode> m_rootNode;

            /**
             * @brief The frustum used for visibility tests (or empty).
             */
            SmartPtr<IFrustum> m_frustum;

            /**
             * @brief The camera whose frustum is being used (or empty). When set,
             *        the job populates the corresponding per-camera visibility
             *        set during execute().
             */
            SmartPtr<IGraphicsCamera> m_camera;

            /**
             * @brief The visibility set for the currently bound camera, cached so
             *        the worker thread can append without an extra map lookup
             *        per node.
             */
            SmartPtr<CameraVisibilitySet> m_activeVisibilitySet;

            /**
             * @brief Per-camera visibility sets keyed by IGraphicsObject::getId().
             */
            ConcurrentHashMap<hash_type, SmartPtr<CameraVisibilitySet>> m_visibilitySets;

            /**
             * @brief Optional explicit list of nodes to cull. When non-empty
             *        the job visits these instead of the root node.
             */
            ConcurrentArray<SmartPtr<IGraphicsSceneNode>> m_nodes;

            /**
             * @brief Whether the traversal recurses into child nodes.
             */
            AtomicValue<bool> m_recursive = true;

            /**
             * @brief The flag bitmask that is set on a node when it is culled.
             */
            AtomicValue<u32> m_cullFlag = SceneNodeCullFlag_Culled;
        };

    }  // namespace render
}  // namespace workphone

#endif  // SceneNodeCullJob_h__
