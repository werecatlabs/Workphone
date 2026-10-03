#ifndef _WP_IGraphicsObject_H_
#define _WP_IGraphicsObject_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/Ray3.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class IGraphicsObject
         * @brief Abstract interface for a renderable object attachable to a scene node.
         *
         * IGraphicsObject describes the contract required by objects that participate in the
         * rendering pipeline and can be owned by a scene graph node. It centralizes state used
         * by scene management and rendering subsystems such as visibility, shadowing, render
         * ordering, flags, local bounds and scene ownership.
         *
         * Typical responsibilities:
         * - Expose and control visibility (simple boolean and bitmask-based).
         * - Enable/disable casting and receiving shadows.
         * - Maintain a local axis-aligned bounding box (AABB) for culling and intersection tests.
         * - Support attach/detach operations to scene nodes and expose the owning node.
         * - Provide access to renderer-native objects via a generic pointer.
         * - Support cloning to duplicate renderable state.
         *
         * @note Implementations are expected to be managed through SmartPtr and inherit lifetime
         *       semantics from ISharedObject.
         */
        class WPCore_API IGraphicsObject : public ISharedObject
        {
        public:
            /**
             * @brief Bitmask containing every known property key for a graphics object.
             *
             * Use this mask when operations should apply to every property supported by an
             * implementation (for example, when serializing or resetting state).
             */
            static const u32 AllProperties; /* 0x01*/

            /**
             * @brief Flag used to mark the object as an overlay (rendered on top of the scene).
             *
             * Overlay objects generally use a separate render pass or queue so they are always
             * drawn above scene geometry.
             */
            static const u32 OverlayFlag;

            /**
             * @brief Flag used to mark the object as a UI element.
             *
             * UI objects frequently use different render states (for example, no depth test)
             * and might be processed by dedicated UI render passes.
             */
            static const u32 UiFlag;

            /**
             * @brief Flag used to mark an object as a normal scene object.
             *
             * Scene objects participate in scene culling, lighting, and shadowing.
             */
            static const u32 SceneFlag;

            /**
             * @brief State message identifier dispatched when the render queue / Z-order changes.
             *
             * Systems listening for ordering changes can react to this message.
             */
            static const hash_type STATE_MESSAGE_RENDER_QUEUE;

            /**
             * @brief State message identifier dispatched when a direction-related state changes.
             *
             * Use this when orientation or direction information (that affects rendering) is updated.
             */
            static const hash_type STATE_MESSAGE_DIRECTION;

            /**
             * @brief Internal flag indicating the object is attached to a scene node.
             *
             * Managed by attach/detach operations.
             */
            static const u32 attachedFlag;

            /**
             * @brief Internal flag indicating whether the object receives shadows.
             */
            static const u32 receiveShadowsFlag;

            /**
             * @brief Internal flag indicating whether the object is currently visible.
             */
            static const u32 visibleFlag;

            /**
             * @brief Internal flag indicating whether the object casts shadows.
             */
            static const u32 castShadowsFlag;

            // Property key strings (used for property maps / reflection)
            static const String namePropertyStr;
            static const String visiblePropertyStr;
            static const String castShadowsPropertyStr;
            static const String receiveShadowsPropertyStr;
            static const String renderTechniquePropertyStr;
            static const String renderQueueGroupPropertyStr;
            static const String zOrderPropertyStr;
            static const String visibilityMaskPropertyStr;
            static const String localAABBPropertyStr;
            static const String attachedPropertyStr;
            static const String flagsPropertyStr;

            IGraphicsObject();

            IGraphicsObject( u32 poolTypeId );

            /**
             * @brief Virtual destructor.
             *
             * Ensures derived implementations are destroyed correctly when referenced through
             * an IGraphicsObject pointer.
             */
            ~IGraphicsObject() override;

            /**
             * @brief Enable or disable casting of shadows.
             * @param castShadows If true the object will cast shadows; otherwise it will not.
             *
             * @note Implementations should mark internal state dirty if shadow-related GPU
             *       resources or queues need updating.
             */
            virtual void setCastShadows( bool castShadows ) = 0;

            /**
             * @brief Query whether the object casts shadows.
             * @return True when the object is configured to cast shadows.
             */
            virtual bool getCastShadows() const = 0;

            /**
             * @brief Enable or disable receiving of shadows.
             * @param receiveShadows If true the object will receive shadows from other casters.
             */
            virtual void setReceiveShadows( bool receiveShadows ) = 0;

            /**
             * @brief Query whether the object receives shadows.
             * @return True when the object accepts shadows.
             */
            virtual bool getReceiveShadows() const = 0;

            /**
             * @brief Set the simple visible state (coarse on/off).
             * @param visible True to make the object visible; false to hide it.
             *
             * @details This boolean is a coarse control. For fine-grained per-camera or per-layer
             * visibility use setVisibilityFlags together with the SceneManager visibility mask.
             */
            virtual void setVisible( bool visible ) = 0;

            /**
             * @brief Query the simple visible state.
             * @return True if the object is currently marked visible via setVisible.
             */
            virtual bool isVisible() const = 0;

            /**
             * @brief Set the Z-order / render queue group for ordering.
             * @param zOrder Renderer-specific queue or Z-order value that controls draw order.
             *
             * @note Concrete renderers interpret this value according to their queue/phase model.
             */
            virtual void setZOrder( u32 zOrder ) = 0;

            /**
             * @brief Get the current Z-order / render queue group.
             * @return Current Z-order value.
             */
            virtual u32 getZOrder() const = 0;

            /**
             * @brief Set visibility flags used for bitmask-based visibility testing.
             * @param flags Bitmask controlling which scene manager visibility masks will include this
             * object.
             *
             * @details Final visibility is usually computed as (objectFlags & sceneVisibilityMask) != 0.
             * Use visibility flags to implement layers, groups, or camera-specific visibility.
             */
            virtual void setVisibilityFlags( u32 flags ) = 0;

            /**
             * @brief Get the visibility flags for this object.
             * @return Bitmask of visibility flags.
             */
            virtual u32 getVisibilityFlags() const = 0;

            /**
             * @brief Create a deep copy of this graphics object.
             * @param name Optional name for the clone. Interpretations of the name are
             * implementation-defined.
             * @return SmartPtr to a new IGraphicsObject containing duplicated render state.
             *
             * @details Implementations should copy all rendering-relevant state (materials, geometry
             * references, flags, local AABB, etc.) so the clone functions as an independent renderable.
             */
            virtual SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const = 0;

            /**
             * @brief Retrieve the underlying renderer-native object pointer.
             * @param ppObject Output pointer that receives the native object pointer (type
             * renderer-dependent).
             *
             * @note The concrete pointer type and ownership semantics vary by renderer. The pointer
             * may be null if there is no corresponding native object.
             */
            virtual void _getObject( void **ppObject ) const = 0;

            /**
             * @brief Set or clear a single flag in the object's flag mask.
             * @param flag Single flag bit to modify.
             * @param value True to set the bit; false to clear it.
             */
            virtual void setFlag( u32 flag, bool value ) = 0;

            /**
             * @brief Query a single boolean flag value.
             * @param flag Flag bit to test.
             * @return True if the given flag bit is set.
             */
            virtual bool getFlag( u32 flag ) const = 0;

            /**
             * @brief Get the full flags bitmask for this object.
             * @return Bitmask containing all flags currently set.
             */
            virtual u32 getFlags() const = 0;

            /**
             * @brief Replace the object's flags with the provided bitmask.
             * @param flags New complete flags bitmask.
             */
            virtual void setFlags( u32 flags ) = 0;

            /**
             * @brief Get the scene node that currently owns this object.
             * @return Pointer to the owner IGraphicsSceneNode, or nullptr if not attached.
             */
            virtual IGraphicsSceneNode *getOwnerPtr() const = 0;

            /**
             * @brief Get the scene node that currently owns this object.
             * @return SmartPtr to the owner IGraphicsSceneNode, or a null SmartPtr if not attached.
             */
            virtual SmartPtr<IGraphicsSceneNode> getOwner() const = 0;

            /**
             * @brief Set the owner scene node for this object.
             * @param sceneNode SmartPtr to the new owner node, or null to clear ownership.
             *
             * @details Implementations should update internal parent references and transform
             * information when ownership changes.
             */
            virtual void setOwner( SmartPtr<IGraphicsSceneNode> sceneNode ) = 0;

            /**
             * @brief Get the object's local axis-aligned bounding box (AABB).
             * @return AABB expressed in the object's local coordinate space that encloses its geometry.
             *
             * @note The local AABB is used for local-space culling and intersection tests; world-space
             * bounds are computed by combining the local AABB with the owner's world transform.
             */
            virtual AABB3<real_Num> getLocalAABB() const = 0;

            /**
             * @brief Set the object's local axis-aligned bounding box.
             * @param aabb Local-space AABB that should enclose the object's visible geometry.
             */
            virtual void setLocalAABB( const AABB3<real_Num> &aabb ) = 0;

            /**
             * @brief Attach this graphics object to a parent scene node.
             * @param parent SmartPtr to the parent IGraphicsSceneNode to attach to.
             *
             * @remarks Implementations should update attachment state and parent transforms.
             */
            virtual void attachToParent( SmartPtr<IGraphicsSceneNode> parent ) = 0;

            /**
             * @brief Detach this graphics object from the specified parent node.
             * @param parent SmartPtr to the parent IGraphicsSceneNode to detach from.
             *
             * @remarks If the object is not attached to the provided parent this call should be a no-op.
             */
            virtual void detachFromParent( SmartPtr<IGraphicsSceneNode> parent ) = 0;

            /**
             * @brief Query whether the object is currently attached to any scene node.
             * @return True if attached to a parent; false otherwise.
             */
            virtual bool isAttached() const = 0;

            /**
             * @brief Explicitly set the attached state flag.
             * @param attached True to mark as attached; false to mark as detached.
             *
             * @remarks Prefer using attachToParent/detachFromParent to keep parent references
             * consistent.
             */
            virtual void setAttached( bool attached ) = 0;

            /**
             * @brief Get the hash identifier of the render technique currently in use.
             * @return Hash value representing the render technique. Interpretation is
             * implementation-defined.
             */
            virtual hash_type getRenderTechnique() const = 0;

            /**
             * @brief Set the render technique by hash identifier.
             * @param renderTechnique Hash representing the technique to apply.
             */
            virtual void setRenderTechnique( hash_type renderTechnique ) = 0;

            /**
             * @brief Set the render queue group used to schedule this object for drawing.
             * @param queueID Renderer-specific queue group identifier.
             */
            virtual void setRenderQueueGroup( u32 queueID ) = 0;

            /**
             * @brief Get the current render queue group identifier.
             * @return Queue group id used by the renderer for ordering.
             */
            virtual u32 getRenderQueueGroup() const = 0;

            /**
             * @brief Retrieve the creator or manager graphics scene that owns this object.
             * @return SmartPtr to the creator IGraphicsScene instance, or null if none.
             */
            virtual SmartPtr<IGraphicsScene> getCreator() const = 0;

            /**
             * @brief Set the creator/manager graphics scene for this object.
             * @param creator SmartPtr to the IGraphicsScene that created or manages this object.
             */
            virtual void setCreator( SmartPtr<IGraphicsScene> creator ) = 0;

            /**
             * @brief Mark the object as dirty to indicate cached CPU/GPU data needs update.
             *
             * @details Implementations should use this signal to schedule necessary updates such as
             * re-uploading GPU buffers, recalculating derived bounds, rebuilding render batches, etc.
             */
            virtual void makeDirty() = 0;

            /**
             * @name Ray intersection helpers
             * @{
             */

            /**
             * @brief Fast boolean ray intersection test.
             * @param ray Ray (typically in world space) to test against the object.
             * @return True if the ray intersects the object; false otherwise.
             *
             * @note Implementations must document the coordinate space expected for the ray.
             */
            virtual bool intersects( const Ray3<real_Num> &ray ) const = 0;

            /**
             * @brief Ray intersection test that returns the hit distance.
             * @param ray Ray (typically in world space) to test against the object.
             * @param distance Out parameter set to the distance from the ray origin to the hit point if
             * a hit occurred.
             * @return True if the ray intersects the object; false otherwise.
             */
            virtual bool intersects( const Ray3<real_Num> &ray, real_Num &distance ) const = 0;

            /**
             * @brief Ray intersection test that returns the hit point and distance.
             * @param ray Ray (typically in world space) to test against the object.
             * @param hitPoint Out parameter set to the world-space intersection point if a hit occurred.
             * @param distance Out parameter set to the distance from the ray origin to the hit point if
             * a hit occurred.
             * @return True if the ray intersects the object; false otherwise.
             */
            virtual bool intersects( const Ray3<real_Num> &ray, Vector3<real_Num> &hitPoint,
                                     real_Num &distance ) const = 0;

            /** @} */

            /**
             * @brief Handle an incoming state message.
             * @param message SmartPtr to the IStateMessage to handle.
             * @return True if the message was handled and caused state changes.
             */
            virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message ) = 0;

            /**
             * @brief Handle notification that some state has changed.
             * @param state SmartPtr to the changed IState instance.
             * @return True if the change was handled and applied.
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif
