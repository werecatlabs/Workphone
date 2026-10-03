#ifndef WPPhysxConstraintFixed3_h__
#define WPPhysxConstraintFixed3_h__

#include <WPPhysx/WPPhysxConstraint.hpp>
#include <Workphone/Physics/ConstraintFixed3.hpp>

/**
 * @file WPPhysxConstraintFixed3.hpp
 * @brief PhysX-backed implementation of the `ConstraintFixed3` workphone physics constraint.
 *
 * This header declares `PhysxConstraintFixed3`, a concrete bridge between the
 * engine's `ConstraintFixed3` data model and the PhysX implementation details.
 */
namespace workphone
{
    namespace physics
    {
        /**
         * @brief PhysX implementation for a 3-degree fixed constraint.
         *
         * This class adapts `ConstraintFixed3` to the PhysX runtime by
         * implementing loading/unloading and reacting to state changes.
         * It specializes the `PhysxConstraint` template for the
         * `ConstraintFixed3` constraint type.
         */
        class PhysxConstraintFixed3 : public PhysxConstraint<ConstraintFixed3>
        {
        public:
            /**
             * @brief Construct a new PhysxConstraintFixed3 object.
             *
             * The constructor initializes internal PhysX resources to a
             * safe default state. Heavy initialization is deferred to
             * `load`.
             */
            PhysxConstraintFixed3();

            /**
             * @brief Destroy the PhysxConstraintFixed3 object.
             *
             * The destructor releases any remaining PhysX resources. It
             * should be safe to call even if `unload` was not previously
             * invoked.
             */
            ~PhysxConstraintFixed3() override;

            /**
             * @brief Load the constraint from a shared data object.
             *
             * This method creates or updates the underlying PhysX objects
             * based on the data contained in `data` (typically a
             * `ConstraintFixed3` instance wrapped in a `SmartPtr`).
             *
             * @param data Shared object containing the serialized/managed
             *             constraint data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload and release the constraint resources.
             *
             * This method tears down any PhysX objects created by `load`
             * and returns the instance to a safe, unloaded state.
             *
             * @param data Shared object that was previously used to load
             *             the constraint (may be null or unchanged).
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Handle a state change message.
             *
             * This overload receives a high-level state message and applies
             * relevant updates to the PhysX constraint.
             *
             * @param message Message containing state change information.
             */
            void handleStateChanged( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Handle a direct state object change.
             *
             * This overload is used when the full state object is provided
             * and the implementation should synchronize internal
             * representation with the new state.
             *
             * @param state The new state to apply to this constraint.
             */
            void handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @brief Runtime registration macro used by the engine's RTTI or
             *        object factory system.
             */
            WP_CLASS_REGISTER_DECL;
        };
    } // end namespace physics
} // namespace workphone

#endif // WPPhysxConstraintFixed3_h__
