#ifndef WPPhysxPlaneShape_h__
#define WPPhysxPlaneShape_h__

#include <Workphone/Physics/PlaneShape.hpp>
#include <WPPhysx/WPPhysxShape.hpp>

/**
 * @file WPPhysxPlaneShape.hpp
 * @brief PhysX implementation wrapper for a plane collision shape.
 *
 * This header declares `PhysxPlaneShape`, an adapter that implements the
 * engine `PlaneShape` interface using the PhysX SDK via the `PhysxShape`
 * base template.
 */
namespace workphone
{
    namespace physics
    {
        /**
         * @brief PhysX-backed implementation of a plane collision shape.
         *
         * `PhysxPlaneShape` adapts the engine-level `PlaneShape` to the
         * PhysX runtime by creating and managing the underlying PhysX
         * geometry and shape resources for an infinite plane.
         */
        class PhysxPlaneShape : public PhysxShape<PlaneShape>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes internal members to a safe default state. Actual
             * PhysX resources are created by `createShape()`.
             */
            PhysxPlaneShape();

            /**
             * @brief Virtual destructor.
             *
             * Ensures derived cleanup is executed. Calls to `destroyShape()`
             * should release any PhysX resources prior to destruction.
             */
            ~PhysxPlaneShape() override;

            /**
             * @brief Load shape parameters from a shared object.
             *
             * Extract persistent plane data (for example plane normal and
             * distance) from `data` and initialize this object's state.
             * @param data Smart pointer to an `ISharedObject` containing
             * serialized shape information.
             * @see PhysxShape::load
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload or store runtime state into the shared object.
             *
             * Use this to write back any state that should be persisted,
             * or to clear references to external resources.
             * @param data Smart pointer to an `ISharedObject` used for
             * persisting shape information.
             * @see PhysxShape::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Create the underlying PhysX shape and geometry.
             *
             * This will instantiate the PhysX plane geometry and attach the
             * shape to the parent actor. Implementations should guard
             * against creating duplicate shapes if called multiple times.
             * @see PhysxShape::createShape
             */
            void createShape() override;

            /**
             * @brief Destroy the underlying PhysX shape.
             *
             * Releases any PhysX resources allocated by `createShape()` and
             * ensures no dangling pointers remain.
             */
            void destroyShape();

            WP_CLASS_REGISTER_DECL;
        };
    } // end namespace physics
} // namespace workphone

#endif // WPPhysxPlaneShape_h__
