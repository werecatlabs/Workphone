#ifndef __IPhysicsCollisionShape2__H
#define __IPhysicsCollisionShape2__H

#include <Workphone/Interface/Physics/IPhysicsShape.hpp>
#include <Workphone/Interface/Physics/IMassData2.hpp>
#include <Workphone/Math/Sphere2.hpp>
#include <Workphone/Math/AABB2.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a 2D physics collision shape.
         *
         * This class represents the base interface for all 2D physics collision shapes in the system.
         * Collision shapes define the physical boundaries of objects in the 2D physics simulation.
         * They are used to detect collisions between objects and calculate physical responses.
         *
         * The interface provides functionality for:
         * - Getting geometric properties (sphere, AABB, points)
         * - Computing mass properties
         * - Managing shape type and properties
         * - Accessing underlying physics engine objects
         *
         * @see IPhysicsShape
         * @see ISphereShape2
         * @see IBoxShape2
         */
        class WPCore_API IPhysicsShape2 : public IPhysicsShape
        {
        public:
            /** Destructor */
            ~IPhysicsShape2() override;

            /**
             * @brief Gets the bounding sphere of the shape.
             *
             * @return A sphere that completely contains the shape.
             */
            virtual Sphere2<real_Num> getSphere() const = 0;

            /**
             * @brief Gets the axis-aligned bounding box of the shape.
             *
             * @return An AABB that completely contains the shape.
             */
            virtual AABB2<real_Num> getAABB() const = 0;

            /**
             * @brief Gets the vertices of the shape.
             *
             * @param points An array to store the shape's vertices.
             */
            virtual void getPoints( Array<Vector2<real_Num>> &points ) const = 0;

            /**
             * @brief Computes the mass properties of the shape.
             *
             * Computes the mass properties of this shape using its dimensions and density.
             * The inertia tensor is computed about the local origin.
             *
             * @param massData The mass data structure to store the computed properties.
             * @param density The density in kilograms per meter squared.
             */
            virtual void computeMass( SmartPtr<IMassData2> massData, real_Num density ) const = 0;

            /**
             * @brief Gets a pointer to the underlying physics engine object.
             *
             * This is dependent on the physics library used (e.g., Box2D, ODE).
             *
             * @param ppObject Pointer to store the underlying object pointer.
             */
            virtual void _getObject( void **ppObject ) const = 0;

            /**
             * @brief Gets the type of the shape.
             *
             * @return The type identifier of the shape.
             */
            virtual u8 getType() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace physics
}  // namespace workphone

#endif
