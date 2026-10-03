#ifndef IPlaneShape3_h__
#define IPlaneShape3_h__

#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <Workphone/Math/Plane3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a 3D plane collision shape.
         *
         * This class represents a physics shape that can be used in 3D physics simulations to represent
         * a plane. It is an abstract base class that defines the basic interface for a 3D plane
         * collision shape. This includes methods for getting and setting the distance from the origin,
         * the normal vector, and the plane equation of the shape.
         *
         * The `IPlaneShape3` class is intended to be subclassed to create specific types of 3D plane
         * collision shapes. Each subclass should implement the pure virtual methods defined in this
         * class to provide the specific functionality for the shape.
         *
         * @see IPhysicsShape3
         */
        class WPCore_API IPlaneShape3 : public IPhysicsShape3
        {
        public:
            /** Destructor. */
            ~IPlaneShape3() override;

            /**
             * Gets the distance from the origin to the plane.
             * @return The distance from the origin to the plane.
             */
            virtual real_Num getDistance() const = 0;

            /**
             * Sets the distance from the origin to the plane.
             * @param distance The new distance from the origin to the plane.
             */
            virtual void setDistance( real_Num distance ) = 0;

            /**
             * Gets the normal vector of the plane.
             * @return The normal vector of the plane.
             */
            virtual Vector3<real_Num> getNormal() const = 0;

            /**
             * Sets the normal vector of the plane.
             * @param normal The new normal vector of the plane.
             */
            virtual void setNormal( const Vector3<real_Num> &normal ) = 0;

            /**
             * Gets the plane equation of the shape.
             * @return The plane equation of the shape.
             */
            virtual Plane3<real_Num> getPlane() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // IPlaneShape3_h__
