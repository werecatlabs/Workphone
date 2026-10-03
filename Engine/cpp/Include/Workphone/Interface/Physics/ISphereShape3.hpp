#ifndef ISphereShape3_h__
#define ISphereShape3_h__

#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a sphere collision shape.
         *
         * This class represents a sphere-shaped physics shape that can be used in 3D physics
         * simulations. It is an abstract base class that defines the basic interface for a sphere
         * collision shape, including methods for setting and getting the radius of the sphere.
         *
         * The `ISphereShape3` class is intended to be subclassed to create specific types of
         * sphere-shaped collision shapes. Each subclass should implement the pure virtual methods
         * defined in this class to provide the specific functionality for the shape.
         *
         * @see IPhysicsShape, IPhysicsShape3, ICapsuleShape3
         */
        class WPCore_API ISphereShape3 : public IPhysicsShape3
        {
        public:
            /** Destructor. */
            ~ISphereShape3() override;

            /**
             * @brief Sets the radius of the sphere.
             * @param radius The new radius of the sphere.
             */
            virtual void setRadius( real_Num radius ) = 0;

            /**
             * @brief Gets the radius of the sphere.
             * @return The radius of the sphere.
             */
            virtual real_Num getRadius() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // ISphereShape3_h__
