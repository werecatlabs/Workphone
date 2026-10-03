#ifndef CBoxShape2_H
#define CBoxShape2_H

#include <Workphone/Interface/Physics/IBoxShape2.hpp>
#include <Workphone/Math/AABB2.hpp>
#include "WPPhysicsPrerequisites.hpp"
#include "Workphone/Math/Transform2.hpp"
#include <Workphone/Interface/Physics/IMassData2.hpp>

namespace workphone::physics
{

    /**
     * @class CBoxShape2
     * @brief Implementation of a 2D box physics shape.
     */
    class CBoxShape2 : public IBoxShape2
    {
    public:
        CBoxShape2();

        ~CBoxShape2() override;

        /** @brief Checks if the shape is currently attached to a physics body. */
        bool isAttached() const override;

        /** @brief Gets the bounding sphere of the box. */
        Sphere2<real_Num> getSphere() const override;

        /** @brief Gets the axis-aligned bounding box (AABB) of the shape. */
        AABB2<real_Num> getAABB() const override;

        /** @brief Sets the axis-aligned bounding box (AABB) of the shape. */
        void setAABB( const AABB2<real_Num> &box ) override;

        /** @brief Retrieves the vertices of the box in local coordinates. */
        void getPoints( Array<Vector2<real_Num>> &points ) const override;

        /**
         * @brief Retrieves the vertices of the box transformed into world coordinates.
         * @param points Array to store the resulting points.
         * @param tranform The transformation matrix to apply.
         */
        void getPoints( Array<Vector2<real_Num>> &points, const Transform2<real_Num> &tranform ) const;

        /**
         * @brief Computes the mass properties of the box based on a given density.
         * @param massData Pointer to the mass data structure to populate.
         * @param density The density of the material.
         */
        void computeMass( SmartPtr<IMassData2> massData, real_Num density ) const override;

        /** @brief Internal method to get the underlying physics object. */
        void _getObject( void **ppObject ) const override;

        /** @brief Gets the shape type identifier. */
        u8 getType() const override;

        /** @brief Gets the properties associated with this shape. */
        SmartPtr<Properties> getProperties() const override;

        /** @brief Sets the properties for this shape. */
        void setProperties( SmartPtr<Properties> properties ) override;

        /** @copydoc IPhysicsShape::setEnabled */
        virtual void setEnabled( bool enabled );

        /** @copydoc IPhysicsShape::isEnabled */
        virtual bool isEnabled() const;

        /** @brief Checks if the shape is configured as a trigger. */
        bool isTrigger() const;

        /** @brief Sets whether the shape should act as a trigger. */
        void setTrigger( bool trigger );

        /** @brief Sets the collision type mask. */
        void setCollisionType( u32 mask );

        /** @brief Gets the current collision type mask. */
        u32 getCollisionType() const;

        /** @brief Sets the collision mask to determine which types this shape collides with. */
        void setCollisionMask( u32 mask );

        /** @brief Gets the current collision mask. */
        u32 getCollisionMask() const;

        /** @brief Gets the state context of the shape. */
        SmartPtr<IStateContext> getStateContext() const;

        /** @brief Sets the state context for the shape. */
        void setStateContext( SmartPtr<IStateContext> stateContext );

        /** @brief Gets the state listener associated with this shape. */
        SmartPtr<IStateListener> getStateListener() const;

        /** @brief Sets the state listener for the shape. */
        void setStateListener( SmartPtr<IStateListener> stateListener );

        /**
         * @brief Static utility to check if two box shapes intersect.
         * @param boxA First box shape.
         * @param tranformA Transformation of the first box.
         * @param boxB Second box shape.
         * @param tranformB Transformation of the second box.
         * @return True if the boxes intersect, false otherwise.
         */
        static bool intersects( SmartPtr<IBoxShape2> boxA, const Transform2<real_Num> &tranformA,
                                SmartPtr<IBoxShape2> boxB, const Transform2<real_Num> &tranformB );

    private:
        /** @brief Internal helper to update the polygon points based on the current AABB. */
        void setPolyPoints();

        AABB2<real_Num>          m_box;                         ///< Local AABB of the box
        wp_collision_shape      *m_polygonShape = nullptr;      ///< Internal WorkphonePhysics collision shape
        SmartPtr<IStateContext>  m_stateContext;                ///< Context for managing state changes
        SmartPtr<IStateListener> m_stateListener;               ///< Listener for state events
        u32                      m_collisionType = 0xFFFFFFFFu; ///< Bitmask for collision type
        u32                      m_collisionMask = 0xFFFFFFFFu; ///< Bitmask for collision filtering
        bool m_enabled = true;  ///< Whether the shape is active in the physics simulation
        bool m_trigger = false; ///< Whether the shape is a trigger (no physical response)
    };
} // namespace workphone::physics

// end namespace

#endif
