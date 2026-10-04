#ifndef WPPHYSICSSHAPE3_HPP
#define WPPHYSICSSHAPE3_HPP

#include <WPPhysics/WPPhysicsConversions3.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>

namespace workphone::physics
{
    /**
     * @class WPPhysicsShape3
     * @brief Implementation of a 3D physics collision shape.
     *
     * This class manages the lifecycle and properties of a physics shape, including its material,
     * local pose, and simulation filter data.
     */
    class WPPhysicsShape3
    {
    public:
        /**
         * @brief Constructs a WPPhysicsShape3 with the specified collision shape type.
         * @param type The type of collision shape to create.
         */
        explicit WPPhysicsShape3( wp_collision_shape_type type );

        /**
         * @brief Destroys the WPPhysicsShape3 instance.
         */
        virtual ~WPPhysicsShape3();

        /**
         * @brief Retrieves the physics material associated with this shape.
         * @return SmartPtr to the physics material.
         */
        SmartPtr<IPhysicsMaterial3> getMaterial() const;

        /**
         * @brief Sets the physics material for this shape.
         * @param material SmartPtr to the material to assign.
         */
        void setMaterial( SmartPtr<IPhysicsMaterial3> material );

        /**
         * @brief Sets the local pose (position and orientation) of the shape relative to the actor.
         * @param pose The transform to apply.
         */
        void setLocalPose( const Transform3<real_Num> &pose );

        /**
         * @brief Retrieves the local pose of the shape.
         * @return The current local transform.
         */
        Transform3<real_Num> getLocalPose() const;
        /**
         * @brief Sets the simulation filter data for collision detection.
         * @param data The filter data to apply.
         */
        void setSimulationFilterData( const FilterData &data );

        /**
         * @brief Retrieves the current simulation filter data.
         * @return The filter data used for collisions.
         */
        FilterData getSimulationFilterData() const;

        /**
         * @brief Assigns this shape to a physics body (actor).
         * @param body SmartPtr to the physics body.
         */
        void setActor( SmartPtr<IPhysicsBody3> body );

        /**
         * @brief Retrieves the physics body (actor) this shape is attached to.
         * @return SmartPtr to the actor, or nullptr if not attached.
         */
        SmartPtr<IPhysicsBody3> getActor() const;
        /**
         * @brief Retrieves the raw underlying object pointer.
         * @param ppObject Pointer to store the object address.
         */
        void _getObject( void **ppObject ) const;

        /**
         * @brief Checks if the shape has valid data initialized.
         * @return True if shape data exists, false otherwise.
         */
        bool hasShapeData() const;

        /**
         * @brief Creates a clone of this physics shape.
         * @return A SmartPtr to the cloned shape.
         */
        SmartPtr<IPhysicsShape3> clone();

        /**
         * @brief Checks if the shape is currently attached to a physics body.
         * @return True if attached, false otherwise.
         */
        bool isAttached() const;
        /**
         * @brief Enables or disables the physics shape.
         * @param enabled True to enable, false to disable.
         */
        void setEnabled( bool enabled );

        /**
         * @brief Checks if the physics shape is enabled.
         * @return True if enabled, false otherwise.
         */
        bool isEnabled() const;

        /**
         * @brief Sets whether this shape acts as a trigger.
         * @param trigger True to set as trigger, false for solid collision.
         */
        void setTrigger( bool trigger );

        /**
         * @brief Checks if this shape is configured as a trigger.
         * @return True if it is a trigger.
         */
        bool isTrigger() const;
        /**
         * @brief Sets the state context for the shape.
         * @param stateContext SmartPtr to the state context.
         */
        void setStateContext( SmartPtr<IStateContext> stateContext );

        /**
         * @brief Retrieves the current state context.
         * @return SmartPtr to the state context.
         */
        SmartPtr<IStateContext> getStateContext() const;

        /**
         * @brief Sets the listener for state changes.
         * @param stateListener SmartPtr to the state listener.
         */
        void setStateListener( SmartPtr<IStateListener> stateListener );

        /**
         * @brief Retrieves the current state listener.
         * @return SmartPtr to the state listener.
         */
        SmartPtr<IStateListener> getStateListener() const;
        /**
         * @brief Sets the collision type mask.
         * @param mask The collision type bitmask.
         */
        void setCollisionType( u32 mask );

        /**
         * @brief Retrieves the collision type mask.
         * @return The current collision type bitmask.
         */
        u32 getCollisionType() const;

        /**
         * @brief Sets the collision mask.
         * @param mask The collision mask bitmask.
         */
        void setCollisionMask( u32 mask );

        /**
         * @brief Retrieves the collision mask.
         * @return The current collision mask bitmask.
         */
        u32 getCollisionMask() const;

        /**
         * @brief Handles state change messages.
         * @param message The state message to process.
         * @return True if the message was handled.
         */
        bool handleStateChanged( const SmartPtr<IStateMessage> &message );

        /**
         * @brief Handles transition to a new state.
         * @param state The target state.
         * @return True if the state change was handled.
         */
        bool handleStateChanged( SmartPtr<IState> &state );

        /**
         * @brief Retrieves the extents of the shape.
         * @return Vector3 containing the extents.
         */
        Vector3<real_Num> getExtents() const;

        /**
         * @brief Sets the extents of the shape.
         * @param extents The Vector3 extents to set.
         */
        void setExtents( const Vector3<real_Num> &extents );

        /**
         * @brief Retrieves the Axis-Aligned Bounding Box (AABB) of the shape.
         * @return The AABB3 of the shape.
         */
        AABB3<real_Num> getAABB() const;

        /**
         * @brief Sets the AABB of the shape.
         * @param box The AABB3 to set.
         */
        void setAABB( const AABB3<real_Num> &box );

        /**
         * @brief Sets the radius of the shape.
         * @param radius The radius value.
         */
        void setRadius( real_Num radius );

        /**
         * @brief Retrieves the radius of the shape.
         * @return The current radius.
         */
        real_Num getRadius() const;

        /**
         * @brief Retrieves the underlying physics engine shape.
         * @return Pointer to the wp_collision_shape.
         */
        wp_collision_shape *getShape() const;

    private:
        wp_collision_shape *m_shape =
            nullptr;  ///< Internal pointer to the physics engine's collision shape.
        SmartPtr<IPhysicsMaterial3> m_material;  ///< The material assigned to this shape.
        WeakPtr<IPhysicsBody3> m_actor;  ///< Weak reference to the physics body owning this shape.
        SmartPtr<IStateContext> m_stateContext;    ///< Context for state management.
        SmartPtr<IStateListener> m_stateListener;  ///< Listener for state change notifications.
        AABB3<real_Num> m_aabb;                    ///< Cached axis-aligned bounding box.
        Vector3<real_Num> m_boxExtents = Vector3<real_Num>::unit();
        Vector3<real_Num> m_boxScale = Vector3<real_Num>::unit();
    };
}  // namespace workphone::physics

#endif
