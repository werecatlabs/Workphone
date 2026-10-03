#ifndef _CSceneNode_H
#define _CSceneNode_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Script/IScriptReceiver.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Graphics/GraphicsSceneNode.hpp>
#include <OgreNode.h>

namespace workphone
{
    namespace render
    {
        /**
         * @class CSceneNodeOgre
         * @brief Ogre-based implementation of the engine SceneNode abstraction.
         *
         * CSceneNodeOgre wraps an underlying native `Ogre::SceneNode` and
         * implements the engine-level `SceneNode` interface. It adapts scene
         * graph operations (hierarchy management, attach/detach objects,
         * visibility, bounding volumes, transforms) to Ogre primitives while
         * maintaining engine-specific state for scripting, event listeners and
         * thread-safety.
         *
         * Responsibilities:
         * - Manage an associated `Ogre::SceneNode` pointer (non-owning).
         * - Forward attach/detach and transform operations to Ogre.
         * - Maintain cached local/world AABB and visibility/culled flags.
         * - Provide script bindings and state-listener integration.
         *
         * Thread-safety:
         * Internal spin read/write mutexes protect concurrent access to the
         * node and its state. Callers must still obey any external threading
         * rules imposed by the render system (e.g. only mutate scene graph
         * on the render thread when required).
         */
        class CSceneNodeOgre : public GraphicsSceneNode
        {
        public:
            /**
             * @brief Construct an empty wrapper.
             *
             * The underlying `Ogre::SceneNode` pointer is initially null.
             * Use `setSceneNode`, `addChildSceneNode` or `load` to populate.
             */
            CSceneNodeOgre();

            /**
             * @brief Construct and bind to a creator scene.
             * @param creator Smart pointer to the `IGraphicsScene` that created or owns this node.
             *
             * This constructor is typically used when the node is created by a
             * scene instance which should be recorded as the creator/owner.
             */
            CSceneNodeOgre( SmartPtr<IGraphicsScene> creator );

            /**
             * @brief Destructor.
             *
             * Releases internal listeners and clears references. Does not
             * implicitly delete the underlying `Ogre::SceneNode` (pointer is
             * non-owning); this class will detach from it and perform cleanup
             * required by the engine lifecycle.
             */
            ~CSceneNodeOgre() override;

            /**
             * @copydoc SceneNode::load
             *
             * Initialize or restore node state from serialized `data`. Expected
             * to create or configure the underlying `Ogre::SceneNode` when
             * necessary.
             *
             * @param data Optional serialized/context data used for initialization.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc SceneNode::unload
             *
             * Uninitializes the node, detaches objects and removes the Ogre
             * node from the scene graph if appropriate for the engine lifetime.
             *
             * @param data Optional context data for unload operations.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc SceneNode::attachObject
             *
             * Attach a graphics object (mesh, billboard, etc.) to this node.
             *
             * @param object Graphics object to attach. The node keeps a smart pointer.
             */
            void attachObject( SmartPtr<IGraphicsObject> object ) override;

            /**
             * @copydoc SceneNode::detachObject
             *
             * Detach `object` from this node if attached.
             *
             * @param object Graphics object to detach.
             */
            void detachObject( SmartPtr<IGraphicsObject> object ) override;

            /**
             * @copydoc SceneNode::detachAllObjects
             *
             * Detach all graphics objects attached to this node.
             */
            void detachAllObjects() override;

            /**
             * @copydoc SceneNode::addChildSceneNode
             *
             * Create a new named child scene node and return its engine wrapper.
             *
             * @param name Optional name for the created child node.
             * @return SmartPtr to the created IGraphicsSceneNode.
             */
            SmartPtr<IGraphicsSceneNode> addChildSceneNode(
                const String &name = StringUtil::EmptyString ) override;

            /**
             * @copydoc SceneNode::addChildSceneNode
             *
             * Create a child node at the given local position.
             *
             * @param position Local-space position for the new child node.
             * @return SmartPtr to the created child IGraphicsSceneNode.
             */
            SmartPtr<IGraphicsSceneNode> addChildSceneNode( const Vector3F &position ) override;

            /**
             * @copydoc SceneNode::getNumObjects
             * @return Number of attached graphics objects.
             */
            u32 getNumObjects() const override;

            /**
             * @copydoc SceneNode::addChild
             *
             * Attach an existing IGraphicsSceneNode as a child of this node.
             *
             * Ownership semantics follow the engine's SceneNode contract.
             *
             * @param child Node to add as a child.
             */
            void addChild( SmartPtr<IGraphicsSceneNode> child ) override;

            /**
             * @copydoc SceneNode::removeChild
             *
             * Remove a direct child node.
             *
             * @param child Child node to remove.
             * @return True if the child was removed successfully.
             */
            bool removeChild( SmartPtr<IGraphicsSceneNode> child ) override;

            /**
             * @copydoc SceneNode::getChildren
             * @return Array of direct child scene nodes.
             */
            Array<SmartPtr<IGraphicsSceneNode>> getChildren() const override;

            /**
             * @copydoc SceneNode::getNumChildren
             * @return Number of direct children.
             */
            u32 getNumChildren() const override;

            /**
             * @copydoc SceneNode::needUpdate
             *
             * Mark this node as needing an update. If `forceParentUpdate` is true,
             * parent nodes in the hierarchy will also be marked for update.
             *
             * @param forceParentUpdate If true, propagate update requirement to parents.
             */
            void needUpdate( bool forceParentUpdate = false ) override;

            /**
             * @copydoc SceneNode::clone
             *
             * Create a deep or shallow clone of this node. The implementation may
             * clone attached objects and optionally attach the clone to `parent`
             * and/or assign it `name`.
             *
             * @param parent Optional parent to attach the clone to.
             * @param name Optional name for the cloned node.
             * @return SmartPtr to the cloned node.
             */
            SmartPtr<IGraphicsSceneNode> clone( SmartPtr<IGraphicsSceneNode> parent = nullptr,
                                        const String &name = StringUtil::EmptyString ) const override;

            /**
             * @brief Toggle debugging display of the bounding box for this node.
             * @param show True to display the bounding box.
             */
            void showBoundingBox( bool show );

            /**
             * @brief Query whether the debug bounding box is shown.
             * @return True if the bounding box is currently visible for debug.
             */
            bool getShowBoundingBox() const;

            /**
             * @copydoc SceneNode::_getObject
             *
             * Expose the raw underlying object pointer to external systems that
             * require direct native access (opaque pointer semantics).
             *
             * @param ppObject Output pointer location that will receive the pointer.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Retrieve the underlying Ogre::SceneNode pointer.
             * @return Pointer to the underlying `Ogre::SceneNode`, or nullptr if unset.
             *
             * The pointer is non-owning; callers must not delete it.
             */
            Ogre::SceneNode *getSceneNode() const;

            /**
             * @brief Bind this wrapper to an existing Ogre::SceneNode.
             * @param sceneNode Raw pointer to an existing `Ogre::SceneNode`.
             *
             * Caller remains responsible for the lifetime of `sceneNode`.
             * After calling this, the wrapper will attach listeners and may
             * cache state from the provided node.
             */
            void setSceneNode( Ogre::SceneNode *sceneNode );

            /**
             * @copydoc SceneNode::updateBounds
             *
             * Update cached local and world bounding volumes for this node.
             */
            void updateBounds() override;

            /**
             * @brief Set visibility mask/flags for this node.
             * @param flags Bitmask to use for visibility/culling systems.
             *
             * Visibility flags are intended to be used by render queues and
             * culling systems to quickly include/exclude nodes.
             */
            void setVisibilityFlags( u32 flags );

            /**
             * @brief Get current visibility flags bitmask.
             * @return Visibility flags.
             */
            u32 getVisibilityFlags() const;

            /**
             * @copydoc IGraphicsSceneNode::getProperties
             *
             * Return a serializable `Properties` object representing this node's state.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IGraphicsSceneNode::setProperties
             *
             * Apply the provided serializable properties to this node.
             *
             * @param properties Properties instance with values to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @copydoc IGraphicsSceneNode::getChildObjects
             *
             * Return non-scene-node child objects associated with this node.
             *
             * @return Array of `ISharedObject` representing child objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @class ScriptReceiver
             * @brief Script-facing adapter that exposes node properties and actions.
             *
             * Implements `IScriptReceiver` and forwards script get/set/call
             * operations to the owning `CSceneNodeOgre` instance. The receiver
             * does not own the node pointer.
             */
            class ScriptReceiver : public IScriptReceiver
            {
            public:
                /**
                 * @brief Construct a ScriptReceiver bound to a node.
                 * @param node Owning `CSceneNodeOgre` instance (non-owning).
                 */
                ScriptReceiver( CSceneNodeOgre *node );

                /**
                 * @brief Set a single property by id.
                 * @param id Property hash id.
                 * @param param Value to set.
                 * @return Engine-specific status code (0/negative for error).
                 */
                s32 setProperty( hash_type id, const Parameter &param ) override;

                /**
                 * @brief Set multiple properties using a parameters list.
                 * @param id Property hash id.
                 * @param params List of parameters.
                 * @return Status code.
                 */
                s32 setProperty( hash_type id, const Parameters &params ) override;

                /**
                 * @brief Generic set property using raw pointer.
                 * @param hash Property hash id.
                 * @param param Raw pointer to value.
                 * @return Status code.
                 */
                s32 setProperty( hash_type hash, void *param ) override;

                /**
                 * @brief Get a single property value.
                 * @param id Property hash id.
                 * @param param Out parameter to receive the value.
                 * @return Status code.
                 */
                s32 getProperty( hash_type id, Parameter &param ) const override;

                /**
                 * @brief Get multiple property values.
                 * @param id Property hash id.
                 * @param params Out parameters list to populate.
                 * @return Status code.
                 */
                s32 getProperty( hash_type id, Parameters &params ) const override;

                /**
                 * @brief Generic get property using raw pointer.
                 * @param hash Property hash id.
                 * @param param Raw pointer to receive the value.
                 * @return Status code.
                 */
                s32 getProperty( hash_type hash, void *param ) const override;

                /**
                 * @brief Call a scripted function on this node.
                 * @param hashId Function id/hash.
                 * @param params Input parameters.
                 * @param results Output results.
                 * @return Status code.
                 */
                s32 callFunction( hash_type hashId, const Parameters &params,
                                  Parameters &results ) override;

            private:
                CSceneNodeOgre *m_node = nullptr; /**< Owning node pointer (non-owning). */
            };

            /**
             * @class SceneNodeStateListener
             * @brief Receives engine state messages and forwards them to the node.
             *
             * Implements `IStateListener` to handle state messages (e.g. property
             * updates) targeted at this scene node. The listener keeps a
             * non-owning pointer to its owner.
             */
            class SceneNodeStateListener : public IStateListener
            {
            public:
                SceneNodeStateListener();
                ~SceneNodeStateListener() override;

                /**
                 * @brief Handle an incoming state message.
                 * @param message Message to process.
                 * @return True if the message was handled by this listener.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handle a state object change event.
                 * @param state Changed state instance.
                 * @return True if the change was handled.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Get the current owner node.
                 * @return Pointer to the owning CSceneNodeOgre (may be nullptr).
                 */
                CSceneNodeOgre *getOwner() const;

                /**
                 * @brief Set the owner node for this listener.
                 * @param owner Owning CSceneNodeOgre instance (non-owning).
                 */
                void setOwner( CSceneNodeOgre *owner );

            protected:
                CSceneNodeOgre *m_owner = nullptr; /**< Non-owning pointer to owner. */
            };

            /**
             * @class NodeListener
             * @brief Listens to native Ogre::Node lifecycle and update events.
             *
             * Receives `Ogre::Node::Listener` callbacks (nodeUpdated, nodeDestroyed,
             * nodeAttached, nodeDetached) so the wrapper can synchronize internal
             * state and react to native lifecycle changes.
             */
            class NodeListener : public Ogre::Node::Listener
            {
            public:
                /**
                 * @brief Construct a NodeListener bound to an owner node.
                 * @param owner Owning `CSceneNodeOgre` instance (non-owning).
                 */
                NodeListener( CSceneNodeOgre *owner );
                ~NodeListener() override;

                /**
                 * @brief Ogre callback invoked when the underlying node is updated.
                 * @param node The Ogre node that triggered the update.
                 */
                void nodeUpdated( const Ogre::Node *node ) override;

                /**
                 * @brief Ogre callback invoked when the underlying node is destroyed.
                 * @param node The destroyed Ogre node.
                 */
                void nodeDestroyed( const Ogre::Node *node ) override;

                /**
                 * @brief Ogre callback invoked when the underlying node is attached.
                 * @param node The attached Ogre node.
                 */
                void nodeAttached( const Ogre::Node *node ) override;

                /**
                 * @brief Ogre callback invoked when the underlying node is detached.
                 * @param node The detached Ogre node.
                 */
                void nodeDetached( const Ogre::Node *node ) override;

            protected:
                CSceneNodeOgre *m_owner = nullptr; /**< Non-owning pointer to owner. */
            };

            /**
             * @brief Recompute and cache the node bounding box.
             *
             * Internal helper called after attachments or transform changes to
             * refresh cached local and world AABB values.
             */
            void _updateBoundingBox();

            /**
             * @brief Tear down state/listener context for this node.
             *
             * Detaches and releases any listeners and state objects owned by
             * the node, clearing state-related resources.
             */
            void destroyStateContext();

            AABB3F calculateAABB() const;

            NodeListener *m_nodeListener = nullptr; /**< Listener for native Ogre node events. */
            Ogre::SceneNode *m_sceneNode = nullptr; /**< Underlying Ogre::SceneNode (non-owning). */

            atomic_u32 m_lastUpdate;      /**< Timestamp/counter of the last update. */
            atomic_u32 m_transformUpdate; /**< Timestamp/counter of the last transform update. */

            atomic_bool m_isCulled; /**< Whether the node is flagged as culled. */

            static u32 m_nameExt; /**< Static counter used to generate unique node names when needed. */
        };
    }  // end namespace render
}  // namespace workphone

#endif
