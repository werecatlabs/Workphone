#ifndef SphereShape_h__
#define SphereShape_h__

/**
 * @file SphereShape.hpp
 * @brief 3D sphere collision/physics shape declaration.
 *
 * This file declares the `workphone::physics::SphereShape` class which
 * implements a sphere-shaped physics primitive. The class wraps engine
 * specific shape behavior and exposes radius accessors and lifecycle
 * methods used by the physics subsystem.
 */

#include <Workphone/Interface/Physics/ISphereShape3.hpp>
#include <Workphone/Physics/PhysicsShape3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @class SphereShape
         * @brief Concrete physics shape representing a 3D sphere.
         *
         * `SphereShape` derives from `PhysicsShape3<ISphereShape3>` and
         * provides loading/unloading hooks plus radius accessors for the
         * sphere primitive. This shape is intended to be used by physics
         * managers and rigid bodies that require a spherical collision
         * primitive.
         *
         * Lifetime:
         * - Constructed with a default state.
         * - `load` is called to initialize from serialized/shared data.
         * - `unload` releases any resources acquired during `load`.
         *
         * Thread-safety and ownership follow the conventions established by
         * the engine's `PhysicsShape3` and `ISharedObject` usage.
         */
        class WPCore_API SphereShape : public PhysicsShape3<ISphereShape3>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes the sphere shape to default values. Does not allocate
             * engine-specific resources; call `load` to initialize from data.
             */
            SphereShape();

            /**
             * @brief Virtual destructor.
             *
             * Ensures derived resources are properly released. `unload` should
             * be called prior to destruction if the object was previously loaded.
             */
            ~SphereShape() override;

            /**
             * @brief Load initialization data for this shape.
             *
             * Implementations should extract relevant parameters (such as radius)
             * from `data` and create any underlying engine-specific resources.
             *
             * @param data Smart pointer to a shared object containing serialized
             *             or structured initialization data. May be null depending
             *             on caller conventions.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload and release resources created by `load`.
             *
             * After `unload` the object should be in the same state as after
             * construction. Implementations must be safe to call even if `load`
             * was never called.
             *
             * @param data Optional pointer to a shared object that may help in
             *             cleaning up or saving state prior to release.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Set the radius of the sphere.
             *
             * Updates the internal radius and any engine-specific representation.
             * Callers should ensure any dependent objects are updated if required.
             *
             * @param radius New radius value (in engine world units). Values <= 0
             *               may be rejected by the implementation.
             */
            void setRadius( real_Num radius );

            /**
             * @brief Get the current radius of the sphere.
             * @return The radius value as stored by this shape (in world units).
             */
            real_Num getRadius() const;

            /**
             * @brief Macro for runtime type registration.
             *
             * Expands to declarations used by the engine's object registration
             * / reflection system. Kept here to match other shape declarations.
             */
            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace physics
}  // namespace workphone

#endif  // SphereShape_h__
