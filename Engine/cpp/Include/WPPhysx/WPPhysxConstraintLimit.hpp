#ifndef WPPhysxConstraintLimit_h__
#define WPPhysxConstraintLimit_h__

/**
 * @file WPPhysxConstraintLimit.hpp
 * @brief PhysX-backed implementation of a constraint limit.
 *
 * This header declares `PhysxConstraintLimit`, a concrete implementation
 * of `workphone::physics::ConstraintLimit` that adapts the engine's
 * generic constraint limit interface to the PhysX physics backend.
 */

#include <WPPhysx/WPPhysxConstraint.hpp>
#include <Workphone/Physics/ConstraintLimit.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief PhysX implementation of `ConstraintLimit`.
         *
         * `PhysxConstraintLimit` provides a backend-specific implementation
         * for constraint limit behavior when using the PhysX SDK. It
         * inherits the generic `ConstraintLimit` interface defined in the
         * engine so higher-level code can interact with limits without
         * depending on PhysX directly.
         *
         * Typical responsibilities:
         * - hold PhysX specific data required to represent a joint limit
         * - translate generic limit parameters to PhysX types during
         *   constraint setup
         * - clean up any PhysX resources on destruction
         */
        class PhysxConstraintLimit : public ConstraintLimit
        {
        public:
            /**
             * @brief Construct a new PhysxConstraintLimit.
             *
             * The constructor prepares any initial state required by the
             * PhysX-specific implementation. Heavy initialization that
             * depends on a PhysX scene or actors should be deferred to
             * the code that creates the actual constraint objects.
             */
            PhysxConstraintLimit();

            /**
             * @brief Destroy the PhysxConstraintLimit.
             *
             * Ensures any PhysX-owned resources associated with this limit
             * are released. The base class destructor will be called
             * automatically after derived cleanup.
             */
            ~PhysxConstraintLimit();

            /**
             * @brief Reflection/registration macro for the engine.
             *
             * This macro expands to declarations required by the engine's
             * runtime type system (serialization, reflection, factory
             * registration, etc.). Keep it in the public section so the
             * type system can access class meta-information.
             */
            WP_CLASS_REGISTER_DECL;
        };

    } // namespace physics
} // namespace workphone

#endif // WPPhysxConstraintLimit_h__
