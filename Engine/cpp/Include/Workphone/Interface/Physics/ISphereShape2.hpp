#ifndef ISphereShape2_h__
#define ISphereShape2_h__

#include <Workphone/Interface/Physics/IPhysicsShape2.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a 2D sphere physics shape.
         *
         * This interface defines methods for setting and getting the radius of the sphere shape.
         */
        class WPCore_API ISphereShape2 : public IPhysicsShape2
        {
        public:
            /**
             * @brief Destructor for the sphere shape.
             */
            ~ISphereShape2() override;

            /**
             * @brief Set the radius of the sphere shape.
             * @param radius The radius to set for the sphere shape.
             */
            virtual void setRadius( real_Num radius ) = 0;

            /**
             * @brief Get the radius of the sphere shape.
             * @return The radius of the sphere shape.
             */
            virtual real_Num getRadius() const = 0;

            /**
             * @brief Macro to declare the class type for registration with the object factory.
             */
            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // ISphereShape2_h__
