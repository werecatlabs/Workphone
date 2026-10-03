#ifndef __CSceneNode_h__
#define __CSceneNode_h__

#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>
#include <Workphone/State/States/BoundingBoxStateData.hpp>
#include <Workphone/State/States/SceneNodeStateData.hpp>
#include <Workphone/State/States/TransformStateData.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class SceneNode
         * @brief Node in a scene graph that manages transforms, hierarchy and attached graphics objects.
         *
         * SceneNode implements IGraphicsSceneNode and represents a transformable container in the scene
         * graph. It stores a local transform and computed world transform, manages parent/child
         * relationships, and holds references to graphics objects attached to the node. Thread-safe
         * containers and atomic/weak smart pointers are used where concurrent access is expected (see
         * member fields).
         *
         * Ownership and lifetime:
         * - Parents/children and attached graphics objects are stored using SmartPtr / AtomicSmartPtr.
         * - The scene manager that created this node is held as an AtomicWeakPtr; use getCreatorPtr()
         *   to obtain the raw pointer without affecting ownership.
         *
         * Typical usage:
         * - Modify local transform via setPosition/setOrientation/setScale or setTransform.
         * - Call needUpdate()/makeDirty() to mark transform/bounds as requiring recalculation.
         * - Attach/detach graphics objects using attachObject/detachObject.
         */
        class WPCore_API GraphicsSceneNode : public SharedGraphicsObject<IGraphicsSceneNode>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Sets up internal containers and default transform values. Does not register the node with
             * any scene manager; setCreator() must be called by the creating scene/system.
             */
            GraphicsSceneNode();

            GraphicsSceneNode( u32 poolTypeId );

            /**
             * @brief Virtual destructor.
             *
             * Releases all attached resources. Implementations should ensure children and attached
             * objects are released or detached in a safe manner.
             */
            ~GraphicsSceneNode() override;

            /**
             * @brief Initialize or configure the scene node from generic data.
             * @param data A SmartPtr to initialization data. The structure and expected contents are
             *             implementation-specific.
             *
             * Called to load any data required by the SceneNode. Implementations may interpret `data`
             * to restore transforms, properties, children or attachments.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload and release resources associated with this scene node.
             * @param data Optional data guiding the unload operation.
             *
             * After unload the node should be in a state safe for destruction or re-loading.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Detach a graphics object identified by a raw pointer.
             * @param object Raw pointer to the graphics object to detach.
             *
             * Detaches the object without taking ownership via a SmartPtr; use this when only a raw
             * pointer is available. Safe to call if object is not currently attached (no-op).
             */
            void detachObjectPtr( IGraphicsObject *object );

            /**
             * @brief Get the parent scene node.
             * @return SmartPtr<IGraphicsSceneNode> Smart pointer to the parent node, or nullptr if this
             * is a root node.
             *
             * The returned SmartPtr increases the reference count of the parent while held.
             */
            SmartPtr<IGraphicsSceneNode> getParent() const override;

            /**
             * @brief Set the parent for this node.
             * @param parent Smart pointer to the new parent scene node (may be nullptr to become root).
             *
             * Reparents this node. Implementations should update internal child lists of the old and new
             * parent and mark transforms/bounds dirty if required.
             */
            void setParent( SmartPtr<IGraphicsSceneNode> parent ) override;

            /**
             * @brief Get the raw pointer to the scene manager (creator) that created this node.
             * @return IGraphicsScene* Raw, non-owning pointer to the creator scene. May be nullptr.
             *
             * This does not affect reference counts; use getCreator() to obtain an owning SmartPtr.
             */
            IGraphicsScene *getCreatorPtr() const;

            /**
             * @brief Get the scene manager (creator) that created this node.
             * @return SmartPtr<IGraphicsScene> Smart pointer to the creator scene. May be nullptr.
             */
            SmartPtr<IGraphicsScene> getCreator() const override;

            /**
             * @brief Set the scene manager that created this node.
             * @param creator Smart pointer to the creator scene manager.
             */
            void setCreator( SmartPtr<IGraphicsScene> creator ) override;

            /**
             * @brief Get the local transform for this node.
             * @return Transform3<real_Num> Local transform (position, orientation, scale).
             */
            Transform3<real_Num> getTransform() const override;

            /**
             * @brief Set the local transform for this node.
             * @param t The new local transform (position, orientation, scale).
             *
             * Marks the node as dirty so world transform and bounds are recomputed as needed.
             */
            void setTransform( const Transform3<real_Num> &t ) override;

            /**
             * @brief Get the computed world transform for this node.
             * @return Transform3<real_Num> World transform which includes parent transforms.
             */
            Transform3<real_Num> getWorldTransform() const override;

            /**
             * @brief Manually set the world transform.
             * @param t The world transform to assign.
             *
             * Use with care: manual world transform assignment may bypass normal parent-relative
             * transform propagation and should typically be followed by a needUpdate()/makeDirty().
             */
            void setWorldTransform( const Transform3<real_Num> &t ) override;

            /**
             * @brief Set local position (translation) of the node.
             * @param position New local position vector.
             */
            void setPosition( const Vector3<real_Num> &position ) override;

            /**
             * @brief Get local position (translation) of the node.
             * @return Vector3<real_Num> Local position vector.
             */
            Vector3<real_Num> getPosition() const override;

            /**
             * @brief Get the world-space position of the node.
             * @return Vector3<real_Num> World-space position computed from world transform.
             */
            Vector3<real_Num> getWorldPosition() const override;

            /**
             * @brief Set local rotation using Euler angles in degrees.
             * @param degrees Euler angles in degrees (pitch, yaw, roll).
             *
             * Converts degrees to the internal orientation representation (typically quaternion).
             */
            void setRotationFromDegrees( const Vector3<real_Num> &degrees ) override;

            /**
             * @brief Set local orientation as a quaternion.
             * @param orientation Quaternion representing local orientation.
             */
            void setOrientation( const Quaternion<real_Num> &orientation ) override;

            /**
             * @brief Get the local orientation quaternion.
             * @return Quaternion<real_Num> Local orientation.
             */
            Quaternion<real_Num> getOrientation() const override;

            /**
             * @brief Get the world orientation quaternion.
             * @return Quaternion<real_Num> World orientation including parent rotations.
             */
            Quaternion<real_Num> getWorldOrientation() const override;

            /**
             * @brief Set local scale.
             * @param scale Local non-uniform scale vector.
             */
            void setScale( const Vector3<real_Num> &scale ) override;

            /**
             * @brief Get local scale.
             * @return Vector3<real_Num> Local scale vector.
             */
            Vector3<real_Num> getScale() const override;

            /**
             * @brief Get world scale.
             * @return Vector3<real_Num> World scale including parent scales.
             */
            Vector3<real_Num> getWorldScale() const override;

            /**
             * @brief Retrieve the render-system-specific transform representation.
             * @return void* Opaque pointer to underlying render system transform
             * (implementation-specific).
             *
             * This pointer is non-owning and only valid while the underlying render resources exist.
             */
            void *_getRenderSystemTransform() const override;

            /**
             * @brief Orient this node to look at a target point in world space.
             * @param targetPoint Target point in world coordinates.
             *
             * Changes the node's orientation so that its forward direction faces targetPoint.
             * The position is not modified. The exact forward/up conventions depend on implementation.
             */
            void lookAt( const Vector3<real_Num> &targetPoint ) override;

            /**
             * @brief Configure a fixed yaw axis for orientation operations.
             * @param useFixed True to enable a fixed yaw axis, false to use free yaw.
             * @param fixedAxis Axis vector to use as the fixed yaw axis (default: UNIT_Y).
             *
             * When enabled, lookAt and other orientation calculations will preserve the yaw around
             * the specified axis, preventing roll/tumble in certain cases.
             */
            void setFixedYawAxis(
                bool useFixed, const Vector3<real_Num> &fixedAxis = Vector3<real_Num>::UNIT_Y ) override;

            /**
             * @brief Get the local axis-aligned bounding box for this node.
             * @return AABB3<real_Num> Local-space axis-aligned bounding box.
             *
             * The local AABB typically encloses all attached objects; updateBounds() recomputes it.
             */
            AABB3<real_Num> getLocalAABB() const override;

            /**
             * @brief Set the local axis-aligned bounding box.
             * @param aabb New local AABB to assign.
             */
            void setLocalAABB( const AABB3<real_Num> &aabb ) override;

            /**
             * @brief Get the world axis-aligned bounding box for this node.
             * @return AABB3<real_Num> World-space AABB, transformed by the node's world transform.
             */
            AABB3<real_Num> getWorldAABB() const override;

            /**
             * @brief Set the world axis-aligned bounding box.
             * @param aabb World-space AABB to assign.
             *
             * Typically set by the scene/renderer after bounds computation.
             */
            void setWorldAABB( const AABB3<real_Num> &aabb ) override;

            /**
             * @brief Query whether this node should use static render-scene storage.
             * @return True when the node is intended for static scene data.
             */
            bool isStatic() const override;

            /**
             * @brief Set whether this node should use static render-scene storage.
             * @param isstatic True for static scene data, false for dynamic scene data.
             */
            void setStatic( bool isstatic ) override;

            /**
             * @brief Attach a graphics object to this node.
             * @param object Smart pointer to the graphics object to attach.
             *
             * The node keeps a SmartPtr to the object; attaching transfers or shares ownership
             * depending on the SmartPtr implementation.
             */
            void attachObject( SmartPtr<IGraphicsObject> object ) override;

            /**
             * @brief Detach a graphics object held by this node.
             * @param object Smart pointer to the graphics object to detach.
             *
             * Removes the object from this node's attachment list. If the passed SmartPtr is
             * equivalent to an attached object reference, that object will be detached.
             */
            void detachObject( SmartPtr<IGraphicsObject> object ) override;

            /**
             * @brief Detach all graphics objects from this node.
             *
             * Removes and releases references to every attached graphics object.
             */
            void detachAllObjects() override;

            /**
             * @brief Get all graphics objects attached to this node.
             * @return Array<SmartPtr<IGraphicsObject>> Array of SmartPtrs to attached objects.
             *
             * The returned array holds owning SmartPtrs; copying the array increases reference counts.
             */
            Array<SmartPtr<IGraphicsObject>> getObjects() const override;

            /**
             * @brief Get number of attached graphics objects.
             * @return u32 Count of currently attached graphics objects.
             */
            u32 getNumObjects() const override;

            /**
             * @brief Create and add a new child scene node with an optional name.
             * @param name Name for the new child node (optional).
             * @return SmartPtr<IGraphicsSceneNode> Smart pointer to the newly created child node.
             *
             * The created child will have this node set as its parent.
             */
            SmartPtr<IGraphicsSceneNode> addChildSceneNode(
                const String &name = StringUtil::EmptyString ) override;

            /**
             * @brief Create and add a new child scene node positioned at the given local position.
             * @param position Local position for the new child node.
             * @return SmartPtr<IGraphicsSceneNode> Smart pointer to the newly created child node.
             */
            SmartPtr<IGraphicsSceneNode> addChildSceneNode( const Vector3<real_Num> &position ) override;

            /**
             * @brief Add an existing scene node as a child of this node.
             * @param child Smart pointer to the child scene node to add.
             *
             * The child's parent will be updated to this node.
             */
            void addChild( SmartPtr<IGraphicsSceneNode> child ) override;

            /**
             * @brief Remove a child scene node.
             * @param child Smart pointer to the child scene node to remove.
             * @return bool True if the child was found and removed; false otherwise.
             *
             * Removal does not necessarily destroy the child if other references exist.
             */
            bool removeChild( SmartPtr<IGraphicsSceneNode> child ) override;

            /**
             * @brief Remove all child scene nodes from this node.
             *
             * Children are detached; they may remain alive if other references exist.
             */
            void removeChildren() override;

            /**
             * @brief Find a direct child by name.
             * @param name Name of the child to find.
             * @return SmartPtr<IGraphicsSceneNode> SmartPtr to the found child, or nullptr if not found.
             */
            SmartPtr<IGraphicsSceneNode> findChild( const String &name ) override;

            /**
             * @brief Get all child scene nodes.
             * @return Array<SmartPtr<IGraphicsSceneNode>> Array of smart pointers to child nodes.
             */
            Array<SmartPtr<IGraphicsSceneNode>> getChildren() const override;

            /**
             * @brief Replace this node's children with the provided array.
             * @param children Array of SmartPtrs to new child nodes.
             *
             * Existing children will be detached; ownership semantics follow SmartPtr behavior.
             */
            void setChildren( const Array<SmartPtr<IGraphicsSceneNode>> &children ) override;

            /**
             * @brief Get number of immediate child nodes.
             * @return u32 Number of child nodes.
             */
            u32 getNumChildren() const override;

            /**
             * @brief Mark this node (and optionally ancestors) as needing update.
             * @param forceParentUpdate If true, force parents to be marked dirty as well.
             *
             * The update flag typically triggers recomputation of world transforms and bounds.
             */
            void needUpdate( bool forceParentUpdate = false ) override;

            /**
             * @brief Clone this scene node.
             * @param parent Optional parent for the cloned node. If provided, the clone will be parented
             * to it.
             * @param name Optional name for the cloned node.
             * @return SmartPtr<IGraphicsSceneNode> Smart pointer to the cloned node.
             *
             * Cloning semantics (deep vs shallow copy of attachments/children) are
             * implementation-defined.
             */
            SmartPtr<IGraphicsSceneNode> clone(
                SmartPtr<IGraphicsSceneNode> parent = nullptr,
                const String &name = StringUtil::EmptyString ) const override;

            /**
             * @brief Recompute bounding volumes for this node (local and world).
             *
             * Typically aggregates bounds of attached objects and child nodes and updates local/world
             * AABBs.
             */
            void updateBounds() override;

            /**
             * @brief Retrieve the underlying native object pointer for this node.
             * @param ppObject Output pointer to receive the native object pointer.
             *
             * The pointer returned is implementation-specific and non-owning.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Get the Properties object associated with this node.
             * @return SmartPtr<Properties> Smart pointer to the node's Properties container.
             *
             * Properties store arbitrary key/value metadata for this node.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Set the Properties object for this node.
             * @param properties Smart pointer to a Properties object to associate with the node.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get all child shared objects (children + attached shared objects).
             * @return Array<SmartPtr<ISharedObject>> Array of child shared objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Set or clear a boolean flag bit on this node.
             * @param flag Bitmask identifying the flag to set or clear.
             * @param value True to set the flag, false to clear it.
             *
             * Flags are user-defined bit flags used to control rendering, update or other behavior.
             */
            void setFlag( u32 flag, bool value );

            /**
             * @brief Query a boolean flag value.
             * @param flag Bitmask identifying the flag to query.
             * @return bool True if the flag is set, false otherwise.
             */
            bool getFlag( u32 flag ) const;

            /**
             * @brief Overwrite all flags on this node.
             * @param flags New flags bitmask to assign.
             */
            void setFlags( u32 flags );

            /**
             * @brief Read current flags bitmask for this node.
             * @return u32 Current flags bitmask.
             */
            u32 getFlags() const;

            /**
             * @brief Mark this node as dirty, indicating transforms/bounds must be recalculated.
             *
             * Equivalent to needUpdate(true) for many implementations; used to force internal update.
             */
            void makeDirty() override;

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
            /**< The scene manager that created this node (weak, non-owning reference). */
            AtomicWeakPtr<IGraphicsScene> m_creator;

            /**< The parent scene node (weak reference to avoid cycles). */
            AtomicWeakPtr<IGraphicsSceneNode> m_parent;

            AtomicSmartPtr<BoundingBoxStateData> m_boundingBoxStateData;
            AtomicSmartPtr<SceneNodeStateData> m_sceneNodeStateData;
            AtomicSmartPtr<TransformStateData> m_transformStateData;

            /**< Thread-safe array of this node's children (atomic smart pointers). */
            ConcurrentArray<SmartPtr<IGraphicsSceneNode>> m_children;

            /**< Thread-safe array of graphics objects attached to this node (atomic smart pointers). */
            ConcurrentArray<SmartPtr<IGraphicsObject>> m_graphicsObjects;

            AtomicValue<bool> m_isStatic = false;

            static u32 m_idExt;
        };

        /**
         * @brief Get raw pointer to the scene manager that created this node.
         * @return IGraphicsScene* Raw, non-owning pointer to creator (may be nullptr).
         *
         * This inline accessor directly returns the pointer held by the atomic weak pointer.
         */
        inline IGraphicsScene *GraphicsSceneNode::getCreatorPtr() const
        {
            return m_creator.get();
        }
    }  // namespace render
}  // namespace workphone

#endif  // CSceneNode_h__
