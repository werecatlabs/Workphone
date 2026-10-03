#ifndef WP_CBOXSHAPE3_HPP
#define WP_CBOXSHAPE3_HPP

#include <WPPhysics/CPhysicsShape3.hpp>
#include <Workphone/Interface/Physics/IBoxShape3.hpp>

namespace workphone::physics
{
    /**
     * @class CBoxShape3
     * @brief Implementation of a box-shaped physics collider.
     *
     * This class represents a 3D box shape used in the physics simulation,
     * providing functionality to manage its extents, pose, material, and collision properties.
     */
    class CBoxShape3 : public CPhysicsShape3, public IBoxShape3
    {
    public:
        CBoxShape3();

        /** @brief Gets the physics material assigned to this shape. */
        SmartPtr<IPhysicsMaterial3> getMaterial() const override;

        /** @brief Sets the physics material for this shape. */
        void setMaterial( SmartPtr<IPhysicsMaterial3> material ) override;

        /** @brief Sets the local pose (position and rotation) of the shape relative to its actor. */
        void setLocalPose( const Transform3<real_Num> &pose ) override;

        /** @brief Gets the local pose of the shape relative to its actor. */
        Transform3<real_Num> getLocalPose() const override;

        /** @brief Sets the simulation filter data used for collision filtering. */
        void setSimulationFilterData( const FilterData &data ) override;

        /** @brief Gets the current simulation filter data. */
        FilterData getSimulationFilterData() const override;

        /** @brief Associates this shape with a physics body (actor). */
        void setActor( SmartPtr<IPhysicsBody3> body ) override;

        /** @brief Gets the physics body (actor) associated with this shape. */
        SmartPtr<IPhysicsBody3> getActor() const override;

        /** @brief Internal method to retrieve the underlying physics engine object. */
        void _getObject( void **ppObject ) const override;

        /** @brief Checks if the shape data has been initialized. */
        bool hasShapeData() const override;

        /** @brief Creates a deep copy of the shape. */
        SmartPtr<IPhysicsShape3> clone() override;

        /** @brief Checks if the shape is currently attached to a physics body. */
        bool isAttached() const override;

        /** @brief Enables or disables the shape for physics simulation. */
        void setEnabled( bool enabled ) override;

        /** @brief Returns whether the shape is enabled. */
        bool isEnabled() const override;

        /** @brief Sets whether this shape acts as a trigger (detects overlap without physical response).
         */
        void setTrigger( bool trigger ) override;

        /** @brief Returns whether the shape is configured as a trigger. */
        bool isTrigger() const override;

        /** @brief Sets the state context for the shape. */
        void setStateContext( SmartPtr<IStateContext> stateContext ) override;

        /** @brief Gets the current state context of the shape. */
        SmartPtr<IStateContext> getStateContext() const override;

        /** @brief Sets the listener that will be notified of state changes. */
        void setStateListener( SmartPtr<IStateListener> stateListener ) override;

        /** @brief Gets the current state listener. */
        SmartPtr<IStateListener> getStateListener() const override;

        /** @brief Sets the collision type mask. */
        void setCollisionType( u32 mask ) override;

        /** @brief Gets the collision type mask. */
        u32 getCollisionType() const override;

        /** @brief Sets the collision mask used to determine which types to collide with. */
        void setCollisionMask( u32 mask ) override;

        /** @brief Gets the collision mask. */
        u32 getCollisionMask() const override;

        /** @brief Handles state changes triggered by a state message. */
        bool handleStateChanged( const SmartPtr<IStateMessage> &message ) override;

        /** @brief Handles state changes when the state itself is updated. */
        bool handleStateChanged( SmartPtr<IState> &state ) override;

        /** @brief Gets the half-extents of the box. */
        Vector3<real_Num> getExtents() const override;

        /** @brief Sets the half-extents of the box. */
        void setExtents( const Vector3<real_Num> &extents ) override;

        /** @brief Gets the Axis-Aligned Bounding Box (AABB) of the shape. */
        AABB3<real_Num> getAABB() const override;

        /** @brief Sets the Axis-Aligned Bounding Box (AABB) of the shape. */
        void setAABB( const AABB3<real_Num> &box ) override;
    };
} // namespace workphone::physics

#endif
