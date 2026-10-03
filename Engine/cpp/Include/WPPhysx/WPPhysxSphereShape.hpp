#ifndef WPPhysxSphereShape_h__
#define WPPhysxSphereShape_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Physics/SphereShape.hpp>
#include <WPPhysx/WPPhysxShape.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @file WPPhysxSphereShape.hpp
         * @brief PhysX-backed implementation for a sphere collision shape.
         *
         * This header declares `PhysxSphereShape`, a concrete implementation
         * that adapts the generic `SphereShape` interface to the PhysX runtime
         * via the `PhysxShape` helper template.
         */

        /**
         * @brief PhysX implementation of a sphere collision shape.
         *
         * This class binds the engine-level `SphereShape` (logical shape and
         * parameters) to a PhysX-specific representation. It is responsible
         * for creating and destroying the underlying PhysX shape object and
         * for loading/unloading any serialized/shared data required by the
         * shape.
         *
         * Inheritance:
         * - `PhysxShape<SphereShape>` provides common PhysX shape lifecycle
         *   helpers and integration with the engine's shape abstraction.
         */
        class PhysxSphereShape : public PhysxShape<SphereShape>
        {
        public:
            /**
             * @brief Construct a new PhysxSphereShape.
             *
             * Initializes internal state; does not create the PhysX shape.
             * Users should call `load` or otherwise trigger `createShape`
             * through the owning physics actor/system.
             */
            PhysxSphereShape();

            /**
             * @brief Destroy the PhysxSphereShape.
             *
             * Ensures any PhysX resources owned by this object are released.
             * Destruction should be safe even if `load`/`createShape` was not
             * previously called.
             */
            ~PhysxSphereShape() override;

            /**
             * @brief Load shape data from a shared object.
             *
             * Typically called when the shape is being created or restored from
             * serialized data. The provided `data` pointer may contain shape
             * parameters (radius, local transform, material references, etc.).
             *
             * @param data Shared object containing serialized or runtime data
             *             needed to initialize the shape. Ownership is not
             *             transferred; the SmartPtr wrapper ensures the object
             *             remains alive for the duration of this call if needed.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload shape data and release runtime resources.
             *
             * Called when the shape is being removed or when its data must be
             * cleared. Implementations should undo any work performed in
             * `load` and ensure the PhysX representation is destroyed or
             * detached from its actor.
             *
             * @param data Optional shared object which may contain context for
             *             the unload operation (can be null).
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Macro for registering this class with the engine's RTTI/factory.
             *
             * Expands to declarations required by the engine's class registration
             * system. See macro definition for details.
             */
            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Create the underlying PhysX shape object.
             *
             * This method is called by the base `PhysxShape` lifecycle code when
             * the PhysX actor/body is available and the shape should be created
             * in the PhysX scene. Implementations must populate the PhysX
             * shape using current `SphereShape` parameters (for example, the
             * sphere radius and local pose).
             */
            void createShape() override;
        };
    } // end namespace physics
} // namespace workphone

#endif // WPPhysxSphereShape_h__
