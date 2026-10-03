#ifndef IPhysicsCompositeShape3_h__
#define IPhysicsCompositeShape3_h__

#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a composite 3D physics shape.
         *
         * The `IPhysicsCompositeShape3` represents a compound collision shape composed of
         * multiple `IPhysicsShape3` instances. Use implementations of this interface when
         * a single primitive shape is insufficient and you need to combine several
         * sub-shapes to form a complex collider.
         *
         * Ownership and lifetime:
         * - Shapes are represented using `SmartPtr<IPhysicsShape3>`. Implementations should
         *   store these smart pointers to maintain shared ownership.
         * - The `getShapes()` method returns an `Array<SmartPtr<IPhysicsShape3>>` by value,
         *   which is a copy of the internal list. Modifying the returned array will not
         *   affect the composite unless `setShapes()` is called with the modified list.
         *
         * Thread-safety:
         * - Thread-safety guarantees are implementation-defined. Callers should assume no
         *   implicit synchronization and protect access externally if used from multiple threads.
         */
        class WPCore_API IPhysicsCompositeShape3 : public IPhysicsShape3
        {
        public:
            /**
             * @brief Virtual destructor.
             *
             * Ensures derived implementations are destroyed correctly through this interface.
             */
            ~IPhysicsCompositeShape3() override;

            /**
             * @brief Get the child shapes that make up this composite.
             *
             * @return An array of smart pointers to `IPhysicsShape3` representing the sub-shapes.
             *         The returned array is a copy of the internal list; each element is a
             *         `SmartPtr` so callers share ownership of the shapes.
             */
            virtual Array<SmartPtr<IPhysicsShape3>> getShapes() const = 0;

            /**
             * @brief Replace the child shapes that make up this composite.
             *
             * Implementations should store the provided `SmartPtr` instances and update any
             * internal collision representation as required.
             *
             * @param shapes Array of `SmartPtr<IPhysicsShape3>` that will become the new children
             *               of this composite. The caller retains shared ownership via `SmartPtr`.
             */
            virtual void setShapes( const Array<SmartPtr<IPhysicsShape3>> &shapes ) = 0;

            /**
             * @brief Class registration macro.
             *
             * Macro used by the framework for runtime type registration/reflection.
             * Keep this in derived classes to ensure they are registered with the type system.
             */
            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // IPhysicsCompositeShape3_h__
