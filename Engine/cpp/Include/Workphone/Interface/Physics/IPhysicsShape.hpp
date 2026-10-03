#ifndef IPhysicsShape_h__
#define IPhysicsShape_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Base interface for collision shapes used by the physics system.
         *
         * IPhysicsShape defines the contract for all collision-shape implementations
         * (2D/3D and engine-specific shapes). A shape represents the collision geometry
         * used to detect overlaps and generate contact/trigger events with other shapes.
         *
         * Responsibilities:
         * - Control of enable/disable and trigger behaviour.
         * - Collision filtering via type/mask bitfields.
         * - Association with state management (context/listener).
         * - Query whether the shape is attached to a body.
         *
         * Implementations must be safe to use through SmartPtr<IPhysicsShape> and follow
         * the reference-counting / ownership model implied by `ISharedObject`.
         *
         * @note Methods are intended to be called from the main physics thread unless
         *       an implementation documents its own thread-safety guarantees.
         *
         * @see IPhysicsShape2, IPhysicsShape3
         * @ingroup Physics
         */
        class WPCore_API IPhysicsShape : public ISharedObject
        {
        public:
            /** Hash value used when creating shapes through a factory interface. */
            static const hash_type CREATE_SHAPE_HASH;

            /**
             * @name Shape flags
             * Flags used internally and by clients to describe shape state.
             * @{
             */

            /** Reserved for internal use (no external meaning guaranteed). */
            static const u32 ShapeFlagReserved;

            /**
             * Shape is enabled and participates in collision detection.
             * Clear this flag to disable collision for the shape while keeping it attached.
             */
            static const u32 ShapeFlagEnabled;

            /**
             * Shape behaves as a trigger volume:
             * - it detects overlaps and generates events
             * - it does not produce physical collision responses
             */
            static const u32 ShapeFlagTrigger;

            /** @} */

            /** Virtual destructor - allows proper cleanup in derived classes. */
            ~IPhysicsShape() override;

            /**
             * @brief Returns whether this shape is currently attached to a physics body.
             *
             * When attached the shape's transform and lifecycle are typically managed
             * by a parent body object. Detached shapes may be reused or destroyed.
             *
             * @return true if the shape is attached to a body; false otherwise.
             */
            virtual bool isAttached() const = 0;

            /**
             * @brief Enable or disable the shape.
             *
             * Disabling a shape removes it from collision detection without detaching it
             * from its parent body. This is cheaper than destroying and recreating the shape
             * when temporary disabling is required.
             *
             * @param enabled Pass true to enable collision detection for the shape,
             *                false to disable it.
             */
            virtual void setEnabled( bool enabled ) = 0;

            /**
             * @brief Query whether the shape is enabled.
             *
             * @return true when the shape participates in collision detection.
             */
            virtual bool isEnabled() const = 0;

            /**
             * @brief Set the shape to act as a trigger volume.
             *
             * Trigger shapes detect overlaps and commonly emit enter/exit events,
             * but do not produce forces or block movement.
             *
             * @param trigger Pass true to mark the shape as a trigger; false otherwise.
             */
            virtual void setTrigger( bool trigger ) = 0;

            /**
             * @brief Query whether the shape is a trigger.
             *
             * @return true if the shape will only generate trigger events and not physical responses.
             */
            virtual bool isTrigger() const = 0;

            /**
             * @brief Assign a state context to the shape.
             *
             * The state context is used to store and manage runtime state for the shape,
             * such as parameters that may change during execution (user-defined properties,
             * persistent flags, etc.). Ownership is held by SmartPtr; implementations should
             * store a copy of the SmartPtr.
             *
             * @param stateContext Smart pointer to the state context to associate with this shape.
             */
            virtual void setStateContext( SmartPtr<IStateContext> stateContext ) = 0;

            /**
             * @brief Retrieve the associated state context.
             *
             * @return SmartPtr<IStateContext> The current state context or null if none set.
             */
            virtual SmartPtr<IStateContext> getStateContext() const = 0;

            /**
             * @brief Set a listener to receive state change notifications for this shape.
             *
             * The listener will be invoked (by the implementation) when the shape's state
             * changes. The observer is stored as a SmartPtr; no raw pointer ownership is taken.
             *
             * @param stateListener Smart pointer to an object implementing IStateListener.
             */
            virtual void setStateListener( SmartPtr<IStateListener> stateListener ) = 0;

            /**
             * @brief Get the current state listener.
             *
             * @return SmartPtr<IStateListener> The registered state listener, or null if none.
             */
            virtual SmartPtr<IStateListener> getStateListener() const = 0;

            /**
             * @brief Set the collision type (also called category or group).
             *
             * Collision filtering commonly uses two bitfields:
             * - collision type (category) assigned to this shape
             * - collision mask describing which categories this shape collides with
             *
             * These are typically 32-bit bitmasks where each bit represents a category.
             *
             * @param mask Bitmask representing the collision type/category for this shape.
             */
            virtual void setCollisionType( u32 mask ) = 0;

            /**
             * @brief Get the collision type/category bitmask assigned to this shape.
             *
             * @return u32 Bitmask representing this shape's collision type.
             */
            virtual u32 getCollisionType() const = 0;

            /**
             * @brief Set the collision mask for this shape.
             *
             * The mask controls which collision types (categories) this shape will consider
             * for collision checks. A bitwise AND between this mask and another shape's
             * collision type determines whether they are tested for collision.
             *
             * @param mask Bitmask describing which categories this shape will collide with.
             */
            virtual void setCollisionMask( u32 mask ) = 0;

            /**
             * @brief Get the collision mask for this shape.
             *
             * @return u32 The bitmask that controls which categories this shape collides with.
             */
            virtual u32 getCollisionMask() const = 0;

            /**
             * @brief Handle a state message directed at this shape.
             *
             * Implementations should apply the message to update internal state as required.
             *
             * @param message Constant reference to an IStateMessage carrying the update.
             * @return true if the message was handled and caused a change; false otherwise.
             */
            virtual bool handleStateChanged( const SmartPtr<IStateMessage> &message ) = 0;

            /**
             * @brief Handle a direct state object update.
             *
             * This overload accepts a mutable SmartPtr to an IState object which may be
             * modified or consumed by the handler.
             *
             * @param state SmartPtr to the state to handle.
             * @return true if handling the state resulted in changes; false otherwise.
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // IPhysicsShape_h__
