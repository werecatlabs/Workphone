#ifndef ConstraintFixed3_h__
#define ConstraintFixed3_h__

/**
 * @file ConstraintFixed3.hpp
 * @brief Fixed 3D physics constraint implementation header.
 *
 * Provides the declaration for `ConstraintFixed3`, a concrete physics constraint
 * that enforces a fixed transform between two 3D bodies (no relative motion).
 */

#include <Workphone/Interface/Physics/IConstraintFixed3.hpp>
#include <Workphone/Physics/PhysicsConstraint3.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @class ConstraintFixed3
         * @brief Enforces a fixed transform between two 3D physics actors.
         *
         * `ConstraintFixed3` implements the `IConstraintFixed3` interface via the
         * `PhysicsConstraint3` CRTP base. A fixed constraint removes all relative
         * degrees of freedom between the connected bodies so they behave as a single
         * rigid entity at the configured local transforms.
         *
         * Responsibilities:
         * - Load and unload serialized constraint data.
         * - Provide child objects used by the engine's object graph.
         * - Expose and apply property sets for editor/runtime configuration.
         *
         * Lifetime and ownership:
         * Instances are typically created and managed by the physics scene or a factory.
         *
         * Threading:
         * Typical physics objects are not safe for concurrent modification. Callers
         * must ensure appropriate synchronization when accessing non-const methods
         * from multiple threads.
         */
        class WPCore_API ConstraintFixed3 : public PhysicsConstraint3<IConstraintFixed3>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes internal state. Concrete initialization that depends on the
             * physics backend or serialized data should happen in `load`.
             */
            ConstraintFixed3();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup in derived classes and when released through
             * interface pointers.
             */
            ~ConstraintFixed3() override;

            /**
             * @brief Load serialized constraint data.
             *
             * Reads state from a shared object (typically produced by the editor or
             * a saved scene) and configures the constraint accordingly.
             *
             * @param data Smart pointer to the serialized data object. The object is
             *             expected to implement the schema required by `IConstraintFixed3`.
             * @throws Implementation-defined exceptions on parsing or validation errors.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload or clear runtime state associated with this constraint.
             *
             * Called when the object is being removed or the scene is being torn down.
             * Implementations should release resources and disconnect from the physics
             * backend as needed.
             *
             * @param data Optional context data passed by the caller (may be nullptr).
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Physics3SharedObject<T>::getChildObjects
             *
             * @return Array of child `ISharedObject` pointers owned or referenced by
             *         this constraint (for example, referenced shapes or helper objects).
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @copydoc Physics3SharedObject<T>::getProperties
             *
             * @return Smart pointer to a `Properties` object representing this
             *         constraint's configurable parameters (e.g., local frames).
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc Physics3SharedObject<T>::setProperties
             *
             * Apply a property set to this constraint. This will update runtime
             * configuration and may trigger reinitialization of the underlying
             * physics constraint.
             *
             * @param properties Smart pointer to the property set to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Class registration macro for reflection / runtime type system.
             *
             * Expands to declarations required to register this class with the engine's
             * object factory and serialization system.
             */
            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace physics
}  // namespace workphone

#endif  // ConstraintFixed3_h__
