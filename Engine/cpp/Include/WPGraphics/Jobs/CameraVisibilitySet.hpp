#ifndef CameraVisibilitySet_h__
#define CameraVisibilitySet_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class CameraVisibilitySet
         * @brief Per-camera collection of scene nodes found visible by a cull pass.
         *
         * CameraVisibilitySet is populated by SceneNodeCullJob while it traverses
         * the scene graph against a given camera's frustum. Each visible node is
         * appended to the internal list; nodes that are culled are not added.
         *
         * The set is identified by the camera's hash id
         * (IGraphicsObject::getId) so that multiple cameras can have independent
         * visibility lists within the same cull job.
         *
         * Typical usage:
         * - The render pass (or another lazy consumer on the job queue) calls
         *   snapshot() to obtain a stable array of visible scene nodes for a
         *   given camera, then iterates that array instead of re-testing every
         *   node against the frustum.
         *
         * Threading: the underlying ConcurrentArray makes push_back / snapshot
         * safe to call concurrently with the render pass; in particular the
         * job appends to the array on its worker thread while the render pass
         * may snapshot on the render thread.
         */
        class WPGraphics_API CameraVisibilitySet : public ISharedObject
        {
        public:
            /**
             * @brief Construct an empty visibility set for the given camera.
             * @param cameraId The hash id (IGraphicsObject::getId) of the
             *                 camera this set belongs to.
             */
            explicit CameraVisibilitySet( hash_type cameraId );

            /**
             * @brief Destroy the visibility set.
             *
             * Default destructor releases any contained smart pointers.
             */
            ~CameraVisibilitySet() = default;

            /**
             * @brief Append a scene node to the set of visible nodes.
             *
             * Called by SceneNodeCullJob for every node that passes the
             * frustum test against this set's camera.
             */
            void addVisible( const SmartPtr<IGraphicsSceneNode> &node );

            /**
             * @brief Remove all entries from the set.
             *
             * Called by SceneNodeCullJob at the start of each pass so the set
             * reflects only the current frame's result.
             */
            void clear();

            /**
             * @brief Take a stable snapshot of the current visible nodes.
             * @return Array<SmartPtr<IGraphicsSceneNode>> A copy of the visible
             *         nodes at the time of the call.
             */
            Array<SmartPtr<IGraphicsSceneNode>> snapshot() const;

            /**
             * @brief Get the hash id of the camera this set belongs to.
             */
            hash_type getCameraId() const;

            /**
             * @brief Query the number of visible nodes currently stored.
             */
            u32 getNumVisible() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**< The hash id of the camera this set belongs to. */
            hash_type m_cameraId;

            /**< The visible scene nodes for this camera. */
            ConcurrentArray<SmartPtr<IGraphicsSceneNode>> m_visibleNodes;
        };

    }  // namespace render
}  // namespace workphone

#endif  // CameraVisibilitySet_h__
