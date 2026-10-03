#ifndef __CSceneNodeOgreNext_H
#define __CSceneNodeOgreNext_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/GraphicsSceneNode.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <OgreNode.h>

namespace workphone
{
    namespace render
    {

        /**
         * @brief OgreNext implementation of a scene node wrapper.
         *
         * CSceneNodeOgreNext adapts the engine's SceneNode interface to an Ogre::SceneNode.
         * It manages attachment/detachment of graphics objects, child scene nodes, transform
         * updates and bounding box calculation. This class owns or references an underlying
         * Ogre::SceneNode and provides the bridge between engine code and OgreNext.
         */
        class CSceneNodeOgreNext : public GraphicsSceneNode
        {
        public:
            /**
             * @brief Ogre Node listener that forwards node lifecycle events back to the owner.
             *
             * This class is registered on an Ogre::Node to receive callbacks when the node
             * is updated, destroyed, attached or detached. It forwards those callbacks to
             * the owning CSceneNodeOgreNext instance.
             */
            class NodeListener : public Ogre::Node::Listener
            {
            public:
                /**
                 * @brief Construct a listener bound to an owner.
                 * @param owner Non-owning pointer to the CSceneNodeOgreNext that will receive callbacks.
                 */
                NodeListener( CSceneNodeOgreNext *owner );
                ~NodeListener() override;

                /**
                 * @brief Called by Ogre when the node is updated.
                 * @param node Pointer to the updated Ogre::Node (read-only).
                 */
                void nodeUpdated( const Ogre::Node *node ) override;

                /**
                 * @brief Called by Ogre when the node is destroyed.
                 * @param node Pointer to the destroyed Ogre::Node.
                 */
                void nodeDestroyed( const Ogre::Node *node ) override;

                /**
                 * @brief Called when the node is attached to another node.
                 * @param node Pointer to the attached Ogre::Node.
                 */
                void nodeAttached( const Ogre::Node *node ) override;

                /**
                 * @brief Called when the node is detached from its parent.
                 * @param node Pointer to the detached Ogre::Node.
                 */
                void nodeDetached( const Ogre::Node *node ) override;

            protected:
                /// Non-owning raw pointer to the owner; owner lifetime must outlive this listener.
                CSceneNodeOgreNext *m_owner = nullptr;
            };

            /** Default constructor */
            CSceneNodeOgreNext();

            /**
             * @brief Construct a scene node with a creator scene pointer.
             * @param creator Smart pointer to the graphics scene that created this node.
             */
            CSceneNodeOgreNext( SmartPtr<IGraphicsScene> creator );

            /** Destructor */
            ~CSceneNodeOgreNext() override;

            /**
             * @brief Load resources or state associated with this scene node.
             * @param data Optional data passed for loading (implementation-defined).
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload resources or state associated with this scene node.
             * @param data Optional data passed for unloading (implementation-defined).
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Enable or disable a fixed yaw axis for rotations.
             * @param useFixed True to use a fixed yaw axis.
             * @param fixedAxis The axis to use when fixed yaw is enabled. Defaults to UNIT_Y.
             */
            void setFixedYawAxis( bool useFixed, const Vector3F &fixedAxis = Vector3F::UNIT_Y ) override;

            /**
             * @copydoc IGraphicsSceneNode::setStatic
             */
            void setStatic( bool isstatic ) override;

            /**
             * @brief Get the local axis-aligned bounding box (AABB) of this node's contents.
             * @return AABB in local space.
             */
            AABB3F getLocalAABB() const override;

            /**
             * @brief Get the world axis-aligned bounding box (AABB) of this node.
             * @return AABB in world space.
             */
            AABB3F getWorldAABB() const override;

            /**
             * @brief Attach a graphics object to this scene node.
             * @param object Smart pointer to an IGraphicsObject to attach.
             *
             * Attached objects will inherit this node's transform and visibility.
             */
            void attachObject( SmartPtr<IGraphicsObject> object ) override;

            /**
             * @brief Detach a previously attached graphics object.
             * @param object Smart pointer to the IGraphicsObject to detach.
             */
            void detachObject( SmartPtr<IGraphicsObject> object ) override;

            /**
             * @brief Detach all attached graphics objects from this node.
             */
            void detachAllObjects() override;

            /**
             * @brief Create and add a child scene node with an optional name.
             * @param name Optional name for the child node.
             * @return SmartPtr to the newly created child scene node.
             */
            SmartPtr<IGraphicsSceneNode> addChildSceneNode(
                const String &name = StringUtil::EmptyString ) override;

            /**
             * @brief Create and add a child scene node positioned at the given location.
             * @param position Local position for the new child node.
             * @return SmartPtr to the newly created child scene node.
             */
            SmartPtr<IGraphicsSceneNode> addChildSceneNode( const Vector3F &position ) override;

            /**
             * @brief Add an existing scene node as a child.
             * @param child SmartPtr to the child node to add.
             */
            void addChild( SmartPtr<IGraphicsSceneNode> child ) override;

            /**
             * @brief Remove a child scene node.
             * @param child SmartPtr to the child node to remove.
             * @return true if the child was removed successfully.
             */
            bool removeChild( SmartPtr<IGraphicsSceneNode> child ) override;

            /**
             * @brief Mark this node (and optionally its parent) as needing an update.
             * @param forceParentUpdate When true, propagate the update requirement to the parent.
             */
            void needUpdate( bool forceParentUpdate = false ) override;

            /**
             * @brief Create a deep copy of this scene node.
             * @param parent Optional new parent for the cloned node (defaults to nullptr).
             * @param name Optional name for the cloned node.
             * @return SmartPtr to the cloned IGraphicsSceneNode.
             */
            SmartPtr<IGraphicsSceneNode> clone(
                SmartPtr<IGraphicsSceneNode> parent = nullptr,
                const String &name = StringUtil::EmptyString ) const override;

            /**
             * @brief Retrieve the underlying native object.
             * @param ppObject Out parameter that receives a pointer to the native object (Ogre::SceneNode*).
             *
             * The returned pointer is a void* typed pointer to allow generic interop across wrappers.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Get the underlying Ogre::SceneNode pointer.
             * @return Pointer to the Ogre::SceneNode, or nullptr if not set.
             */
            Ogre::SceneNode *getSceneNode() const;

            /**
             * @brief Recalculate and update this node's bounding volumes.
             *
             * This should be called when attached objects or children change in a way that affects bounds.
             */
            void updateBounds() override;

            /** @copydoc SceneNode::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Calculate and return the node's AABB based on current attachments and children.
             * @return Axis-aligned bounding box in local space.
             */
            AABB3F calculateAABB() const;

            /**
             * @brief Associate this wrapper with an existing Ogre::SceneNode.
             * @param sceneNode Pointer to an already created Ogre::SceneNode.
             *
             * This will attach internal listeners and migrate any necessary state so the wrapper
             * reflects the provided native node.
             */
            void setupNode( Ogre::SceneNode *sceneNode );

            /**
             * @brief Destroy the wrapper and release any native resources.
             *
             * Does not necessarily destroy the underlying Ogre::SceneNode if ownership is external,
             * but releases listeners and internal references.
             */
            void destroy();

            /**
             * @brief Set the local transform for this node.
             * @param t Transform to apply in local space.
             */
            void setTransform( const Transform3<real_Num> &t ) override;

            void setPosition( const Vector3F &position ) override;
            Vector3F getPosition() const override;
            Vector3F getWorldPosition() const override;
            void setOrientation( const QuaternionF &orientation ) override;
            QuaternionF getOrientation() const override;
            QuaternionF getWorldOrientation() const override;
            void setScale( const Vector3F &scale ) override;
            Vector3F getScale() const override;
            Vector3F getWorldScale() const override;

            /**
             * @brief Set the world transform for this node.
             * @param t Transform to apply in world space.
             *
             * The implementation should convert world transform into local space relative to the parent.
             */
            void setWorldTransform( const Transform3<real_Num> &t ) override;

            /**
             * @brief Handle an incoming state message.
             * @param message Smart pointer to the state message.
             * @return true if the message was handled and should not be propagated further.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Called when the observed state has changed.
             * @param state Smart pointer to the changed state.
             * @return true if the change was handled.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Retrieve the underlying render system transform pointer.
             * @return Pointer to the render system-specific transform representation.
             *
             * This is used internally by the rendering pipeline to access low-level transform data.
             */
            void *_getRenderSystemTransform() const override;

            /**
             * @brief Set up the state context for this scene node.
             */
            void setupStateContext();

            /// Listener registered with the Ogre node to receive node events.
            NodeListener *m_nodeListener = nullptr;

            /// Pointer to the underlying Ogre::SceneNode (may be null).
            Ogre::SceneNode *m_sceneNode = nullptr;
        };
    }  // end namespace render
}  // namespace workphone

#endif
