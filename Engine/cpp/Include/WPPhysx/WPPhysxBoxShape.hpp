#ifndef WPPhysxBoxShape_h__
#define WPPhysxBoxShape_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Physics/BoxShape3.hpp>
#include <WPPhysx/WPPhysxShape.hpp>
#include <geometry/PxBoxGeometry.h>

namespace workphone
{
    namespace physics
    {

        /**
         * @file WPPhysxBoxShape.hpp
         * @brief PhysX implementation of a 3D box collision shape.
         *
         * This class adapts the engine's BoxShape3 interface to a PhysX
         * box geometry and PhysX shape. It handles creation and teardown
         * of the underlying PhysX geometry, responds to state changes,
         * and validates the shape before use.
         */
        class PhysxBoxShape : public PhysxShape<BoxShape3>
        {
        public:
            /**
             * @brief Create a new PhysxBoxShape instance.
             *
             * The constructor performs minimal initialization; the actual
             * PhysX shape is created when `createShape()` is called (usually
             * during load).
             */
            PhysxBoxShape();

            /**
             * @brief Virtual destructor.
             *
             * Ensures derived resources are released. The destructor will
             * call unload logic via the base class as required.
             */
            ~PhysxBoxShape() override;

            /**
             * @brief Load shape data from a serialized or shared object.
             *
             * This method reads the provided `data` (typically a descriptor
             * or resource blob) and configures the BoxShape3 state used to
             * create the PhysX geometry. After loading, `createShape()` may
             * be invoked to instantiate the native PhysX objects.
             *
             * @param data Smart pointer to an ISharedObject containing shape configuration.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload and release any native PhysX resources associated with this shape.
             *
             * Called when the shape is being removed or the owning object is destroyed.
             * Implementations should make this idempotent and safe to call multiple times.
             *
             * @param data Smart pointer to an ISharedObject (contextual data, if any).
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Check whether the shape is valid and ready to be used by the physics simulation.
             *
             * Typical checks include verifying dimensions are positive and the underlying
             * PhysX objects were successfully created.
             *
             * @return true if the shape is valid; false otherwise.
             */
            bool isValid() const override;

            /**
             * @brief Handle state-change messages (message-based API).
             *
             * Processes incoming state messages that may affect the shape (for example,
             * scale, extents, or material changes). If the message resulted in a change
             * that requires updating the PhysX geometry, this method should trigger
             * the necessary updates.
             *
             * @param message Smart pointer to a state message describing the change.
             * @return true if the state change was handled by this shape; false otherwise.
             */
            bool handleStateChanged( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Handle state-change via direct state object (state-based API).
             *
             * Alternate entry point for state updates. Implementations should mirror
             * the behavior of the message-based handler.
             *
             * @param state Smart pointer to the new state to apply.
             * @return true if the state change was handled by this shape; false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /** Macro used to register the class with the engine's RTTI / factory system. */
            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Create the underlying PhysX shape and attach it to the actor.
             *
             * This method is responsible for converting the current BoxShape3 state
             * (extents, local transform, material, flags, etc.) into a PhysX shape
             * and registering it with the PhysX actor/rigid body. Implementations
             * should be robust to repeated calls (recreate or update as necessary).
             */
            void createShape() override;

            /**
             * @brief Build and return a PhysX box geometry from the current BoxShape3 extents.
             *
             * The returned geometry will reflect the half-extents expected by PhysX,
             * taking into account any scale stored in the BoxShape3 instance.
             *
             * @return physx::PxBoxGeometry representing this shape's extents.
             */
            physx::PxBoxGeometry createGeometry();
        };
    } // end namespace physics
} // namespace workphone

#endif // WPPhysxBoxShape_h__
