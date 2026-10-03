#ifndef _IGraphicsSceneNode_H
#define _IGraphicsSceneNode_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class IGraphicsSceneNode
         * @brief Abstract interface for a node in the scene graph.
         *
         * Scene nodes form a hierarchical graph used by the rendering system.
         * Each node represents a transform (position, orientation, scale) and may
         * own attached graphics objects (meshes, lights, cameras, etc.) and child nodes.
         *
         * Implementations must provide storage, transform propagation, and integration
         * with the underlying graphics API / scene manager.
         *
         * Ownership and lifetime:
         * - The interface inherits from `ISharedObject` and therefore is reference counted.
         * - Methods returning `SmartPtr<T>` return shared references to the requested objects.
         *
         * Threading:
         * - Unless otherwise documented by a concrete implementation, callers should assume
         *   that methods are not thread-safe and must be called from the main/render thread.
         *
         * @note This header only declares the interface. Concrete behaviour is defined in
         * implementations.
         * @see ISharedObject, IGraphicsScene, IGraphicsObject
         */
        class WPCore_API IGraphicsSceneNode : public ISharedObject
        {
        public:
            /**
             * @brief Bitmask value representing all properties on a scene node.
             *
             * Used by state/property systems to request or set every supported property.
             */
            static const u32 ALL_PROPERTIES = ( 1 << 0 );

            /**
             * @brief Query id for retrieving the node's local axis-aligned bounding box (AABB).
             *
             * Use this hash with the state query system to obtain the node's AABB in local space.
             */
            static const hash_type STATE_QUERY_TYPE_LOCAL_AABB;

            /**
             * @brief Query id for retrieving the node's world axis-aligned bounding box (AABB).
             *
             * Use this hash with the state query system to obtain the node's AABB transformed
             * into world coordinates (includes parent transforms).
             */
            static const hash_type STATE_QUERY_TYPE_WORLD_AABB;

            /**
             * @brief Message id to send or update a node's position.
             *
             * Payload expected: a `Vector3<real_Num>` representing the new local position.
             */
            static const hash_type STATE_MESSAGE_POSITION;

            /**
             * @brief Message id to send or update a node's scale.
             *
             * Payload expected: a `Vector3<real_Num>` representing the new local scale.
             */
            static const hash_type STATE_MESSAGE_SCALE;

            /**
             * @brief Message id to send or update a node's orientation.
             *
             * Payload expected: a `Quaternion<real_Num>` representing the new local orientation.
             */
            static const hash_type STATE_MESSAGE_ORIENTATION;

            /**
             * @brief Message id to make the node look at a world-space position.
             *
             * Payload expected: a `Vector3<real_Num>` representing the target point in world space.
             */
            static const hash_type STATE_MESSAGE_LOOK_AT;

            /** @brief Message id used to add state/data to a node. */
            static const hash_type STATE_MESSAGE_ADD;

            /** @brief Message id used to remove state/data from a node. */
            static const hash_type STATE_MESSAGE_REMOVE;

            /** @brief Message id used to add a child node. */
            static const hash_type STATE_MESSAGE_ADD_CHILD;

            /** @brief Message id used to remove a child node. */
            static const hash_type STATE_MESSAGE_REMOVE_CHILD;

            /** @brief Message id used to attach an object to this node. */
            static const hash_type STATE_MESSAGE_ATTACH_OBJECT;

            /** @brief Message id used to detach an object from this node. */
            static const hash_type STATE_MESSAGE_DETACH_OBJECT;

            /** @brief Message id used to detach all objects from this node. */
            static const hash_type STATE_MESSAGE_DETACH_ALL_OBJECTS;

            /**
             * @brief Property key strings used when serializing/deserializing node properties.
             *
             * Concrete implementations and users should reference these static members when
             * building or parsing Properties objects to ensure consistent keys across the codebase.
             */
            static const String nameStr;
            static const String numObjectsStr;
            static const String sceneNodePositionStr;
            static const String sceneNodeScaleStr;
            static const String sceneNodeOrientationStr;
            static const String stateOrientationStr;

            IGraphicsSceneNode();

            IGraphicsSceneNode( u32 poolTypeId );

            /**
             * @brief Virtual destructor.
             *
             * Implementations should release native resources and detach children/objects as needed.
             */
            ~IGraphicsSceneNode() override;

            /**
             * @brief Get the full local transform (position, orientation, scale).
             * @return Transform3<real_Num> representing the node's local transform.
             */
            virtual Transform3<real_Num> getTransform() const = 0;

            /**
             * @brief Set the node's full local transform (position, orientation, scale).
             * @param t New local transform to apply.
             */
            virtual void setTransform( const Transform3<real_Num> &t ) = 0;

            /**
             * @brief Get the world transform (transform accumulated through parents).
             * @return Transform3<real_Num> representing the node's world transform.
             */
            virtual Transform3<real_Num> getWorldTransform() const = 0;

            /**
             * @brief Set the node's world transform.
             *
             * Setting world transform requires the implementation to compute and store
             * the corresponding local transform relative to the parent.
             *
             * @param t New world transform to apply.
             */
            virtual void setWorldTransform( const Transform3<real_Num> &t ) = 0;

            /**
             * @brief Set the node's local position.
             * @param position New local position relative to the parent node.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Get the node's local position.
             * @return Local position relative to the parent node.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * @brief Get the node's world position.
             * @return Position in world coordinates (after parent transforms are applied).
             */
            virtual Vector3<real_Num> getWorldPosition() const = 0;

            /**
             * @brief Set the node's rotation using Euler angles in degrees.
             *
             * The expected order/interpretation (pitch, yaw, roll) should be respected by the
             * implementation.
             *
             * @param degrees Euler angles in degrees.
             */
            virtual void setRotationFromDegrees( const Vector3<real_Num> &degrees ) = 0;

            /**
             * @brief Set the node's local orientation using a quaternion.
             * @param orientation New local orientation.
             */
            virtual void setOrientation( const Quaternion<real_Num> &orientation ) = 0;

            /**
             * @brief Get the node's local orientation quaternion.
             * @return Local orientation quaternion.
             */
            virtual Quaternion<real_Num> getOrientation() const = 0;

            /**
             * @brief Get the node's world orientation quaternion.
             * @return Orientation in world coordinates (parents applied).
             */
            virtual Quaternion<real_Num> getWorldOrientation() const = 0;

            /**
             * @brief Set the node's local scale.
             * @param scale Local scaling factors for X, Y, Z.
             */
            virtual void setScale( const Vector3<real_Num> &scale ) = 0;

            /**
             * @brief Get the node's local scale.
             * @return Local scale factors.
             */
            virtual Vector3<real_Num> getScale() const = 0;

            /**
             * @brief Get the node's world scale (accumulated through parents).
             * @return World scale factors.
             */
            virtual Vector3<real_Num> getWorldScale() const = 0;

            /**
             * @brief Get pointer to internal transform data cached for the render system.
             *
             * This exposes an implementation-specific pointer that the rendering backend
             * may use directly. The returned pointer is opaque and must not be dereferenced
             * by callers that do not know the underlying graphics implementation.
             *
             * @return Opaque pointer to the render system transform (may be nullptr).
             */
            virtual void *_getRenderSystemTransform() const = 0;

            /**
             * @brief Orient the node to face a world-space point.
             *
             * Implementations should compute an orientation so that the node's forward axis
             * points at `targetPoint`. Up-vector and yaw-axis behaviour depends on the concrete class.
             *
             * @param targetPoint Target point in world coordinates to look at.
             */
            virtual void lookAt( const Vector3<real_Num> &targetPoint ) = 0;

            /**
             * @brief Configure whether yaw uses a fixed axis or the node's local Y axis.
             *
             * When `useFixed` is true the provided `fixedAxis` will be treated as the up/yaw axis.
             * Default `fixedAxis` is `Vector3<real_Num>::UNIT_Y`.
             *
             * @param useFixed True to use fixed axis, false to use local Y axis.
             * @param fixedAxis Axis to use when `useFixed` is true.
             */
            virtual void setFixedYawAxis(
                bool useFixed, const Vector3<real_Num> &fixedAxis = Vector3<real_Num>::UNIT_Y ) = 0;

            /**
             * @brief Get the node's axis-aligned bounding box in local space.
             * @return Local-space AABB that encloses the node and its attached objects
             * (implementation-defined).
             */
            virtual AABB3<real_Num> getLocalAABB() const = 0;

            /**
             * @brief Set the node's local AABB.
             *
             * Used by some systems to override or supply bounding information for culling/queries.
             *
             * @param aabb Local-space axis-aligned bounding box.
             */
            virtual void setLocalAABB( const AABB3<real_Num> &aabb ) = 0;

            /**
             * @brief Get the node's world-space AABB (local AABB transformed by world transform).
             * @return World-space axis-aligned bounding box.
             */
            virtual AABB3<real_Num> getWorldAABB() const = 0;

            /**
             * @brief Set the node's world-space AABB.
             *
             * Some systems permit directly setting the world AABB when it cannot be derived
             * from local bounds or when specialized culling data is available.
             *
             * @param aabb World-space axis-aligned bounding box.
             */
            virtual void setWorldAABB( const AABB3<real_Num> &aabb ) = 0;

            /**
             * @brief Query whether this node should use static render-scene storage.
             * @return True when the node is intended for static scene data.
             */
            virtual bool isStatic() const = 0;

            /**
             * @brief Set whether this node should use static render-scene storage.
             * @param isstatic True for static scene data, false for dynamic scene data.
             */
            virtual void setStatic( bool isstatic ) = 0;

            /**
             * @brief Attach a graphics object (mesh, light, etc.) to this node.
             * @param object Smart pointer to the graphics object to attach.
             *
             * After attaching, the object will be transformed by this node's transform.
             */
            virtual void attachObject( SmartPtr<IGraphicsObject> object ) = 0;

            /**
             * @brief Detach a previously attached graphics object from this node.
             * @param object Smart pointer to the graphics object to detach.
             */
            virtual void detachObject( SmartPtr<IGraphicsObject> object ) = 0;

            /**
             * @brief Detach and release all graphics objects attached to this node.
             *
             * Implementations should ensure attached objects are removed from rendering and
             * their references are released.
             */
            virtual void detachAllObjects() = 0;

            /**
             * @brief Retrieve the list of graphics objects attached to this node.
             * @return Array of smart pointers to attached `IGraphicsObject` instances.
             *
             * The returned array is a snapshot; modifying it does not change the node's attachments.
             */
            virtual Array<SmartPtr<IGraphicsObject>> getObjects() const = 0;

            /**
             * @brief Get the number of attached graphics objects.
             * @return Count of attached objects.
             */
            virtual u32 getNumObjects() const = 0;

            /**
             * @brief Create and add a new child scene node with an optional name.
             * @param name Optional name for the child node. Defaults to an empty string.
             * @return Smart pointer to the newly created child node.
             */
            virtual SmartPtr<IGraphicsSceneNode> addChildSceneNode(
                const String &name = StringUtil::EmptyString ) = 0;

            /**
             * @brief Create and add a new child scene node positioned at `position` (local to this
             * node).
             * @param position Local position for the new child node.
             * @return Smart pointer to the newly created child node.
             */
            virtual SmartPtr<IGraphicsSceneNode> addChildSceneNode(
                const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Set the parent node. Intended for internal use by the scene system.
             * @param parent Smart pointer to the new parent node (may be nullptr to clear).
             *
             * Implementations should maintain child lists and update transforms accordingly.
             */
            virtual void setParent( SmartPtr<IGraphicsSceneNode> parent ) = 0;

            /**
             * @brief Get the parent of this node.
             * @return Smart pointer to the parent `IGraphicsSceneNode` or nullptr if this is a root
             * node.
             */
            virtual SmartPtr<IGraphicsSceneNode> getParent() const = 0;

            /**
             * @brief Add an existing node as a child of this node.
             * @param child Smart pointer to the child node to add.
             *
             * Ownership: this node should take/hold a reference to `child`.
             */
            virtual void addChild( SmartPtr<IGraphicsSceneNode> child ) = 0;

            /**
             * @brief Remove a child node from this node.
             * @param child Child node to remove.
             * @return True if the child was found and removed; false otherwise.
             */
            virtual bool removeChild( SmartPtr<IGraphicsSceneNode> child ) = 0;

            /**
             * @brief Remove and optionally destroy all child nodes.
             *
             * Implementations should decide whether children are only detached or also destroyed.
             */
            virtual void removeChildren() = 0;

            /**
             * @brief Find a direct child by name.
             * @param name Name of the child node to search for.
             * @return Smart pointer to the child node if found; nullptr otherwise.
             */
            virtual SmartPtr<IGraphicsSceneNode> findChild( const String &name ) = 0;

            /**
             * @brief Retrieve the current child nodes.
             * @return Array of smart pointers to child nodes.
             *
             * The returned array is a snapshot; modifying it does not affect the node's live children.
             */
            virtual Array<SmartPtr<IGraphicsSceneNode>> getChildren() const = 0;

            /**
             * @brief Replace the node's children with the provided array.
             * @param children Array of child nodes to set.
             *
             * The implementation should update parent pointers on the provided children.
             */
            virtual void setChildren( const Array<SmartPtr<IGraphicsSceneNode>> &children ) = 0;

            /**
             * @brief Get the number of child nodes.
             * @return Count of direct children.
             */
            virtual u32 getNumChildren() const = 0;

            /**
             * @brief Mark the node (and optionally its parent) as needing an update.
             *
             * Typically used to schedule transform recalculation or bounds updates prior to rendering.
             *
             * @param forceParentUpdate If true, also mark the parent as needing update.
             */
            virtual void needUpdate( bool forceParentUpdate = false ) = 0;

            /**
             * @brief Create a copy (clone) of this scene node and its attached objects.
             *
             * The clone should preserve transform, attachments and optionally be parented to `parent`.
             *
             * @param parent Optional parent for the cloned node. If nullptr, the clone is created
             * without parent.
             * @param name Optional name for the cloned node. Defaults to empty string.
             * @return Smart pointer to the cloned `IGraphicsSceneNode`.
             */
            virtual SmartPtr<IGraphicsSceneNode> clone(
                SmartPtr<IGraphicsSceneNode> parent = nullptr,
                const String &name = StringUtil::EmptyString ) const = 0;

            /**
             * @brief Recompute/update the node's bounding information.
             *
             * Implementations should recalculate local/world AABBs based on attached objects and
             * children.
             */
            virtual void updateBounds() = 0;

            /**
             * @brief Obtain a raw pointer to the underlying implementation object.
             *
             * Many concrete scene node implementations wrap a native object (for example an
             * engine-specific node). This method returns that raw pointer via the output parameter
             * `ppObject`.
             *
             * @param ppObject Output pointer receiving the underlying object pointer. May be set to
             * `nullptr` if no underlying object exists.
             */
            virtual void _getObject( void **ppObject ) const = 0;

            /**
             * @brief Get the scene manager that created this node.
             * @return Smart pointer to the creator `IGraphicsScene`.
             */
            virtual SmartPtr<IGraphicsScene> getCreator() const = 0;

            /**
             * @brief Set the creator/owner scene manager for this node.
             * @param creator Smart pointer to the `IGraphicsScene` that owns or created this node.
             */
            virtual void setCreator( SmartPtr<IGraphicsScene> creator ) = 0;

            /**
             * @brief Mark the node's internal state as dirty.
             *
             * Calling this signals that cached transform/state/bounds should be refreshed
             * before the next use (render, query, etc.).
             */
            virtual void makeDirty() = 0;

            /**
             * @brief Handle an incoming state message.
             * @param message Smart pointer to the state message.
             * @return true if the message was handled and should not be propagated further.
             */
            virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message ) = 0;

            /**
             * @brief Called when the observed state has changed.
             * @param state Smart pointer to the changed state.
             * @return true if the change was handled.
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif
