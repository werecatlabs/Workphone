#ifndef IAircraftCallback_h__
#define IAircraftCallback_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IVehicleCallback.hpp>
#include <Workphone/Core/FixedArray.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @brief Callback interface used by aircraft implementations.
         *
         * Implementations of this interface are used to supply and consume
         * runtime data for an aircraft instance (forces, torques, state and
         * callback handlers). All vectors and orientations follow the engine's
         * established coordinate conventions (world vs local as noted on each
         * method). Implementers must ensure this interface is safe to call
         * from the physics/update thread if the system expects threaded use.
         */
        class WPCore_API IAircraftCallback : public IVehicleCallback
        {
        public:
            /**
             * @brief Virtual destructor.
             */
            ~IAircraftCallback() override;

            /**
             * @brief Load or return textual data associated with the aircraft.
             *
             * @param filePath Path or identifier for the requested data.
             * @return String Contents of the requested data (empty on failure).
             *
             * @note This is an override of IVehicleCallback::getData and should
             *       not perform blocking IO on a real-time thread unless the
             *       caller expects it.
             */
            String getData( const String &filePath ) override = 0;

            /**
             * @brief Apply a world-space force to a body at a given world-space
             *        position.
             *
             * @param bodyId Engine-specific identifier of the rigid body.
             * @param force World-space force vector to apply (N or engine units).
             * @param pos   World-space application point for the force.
             *
             * @note The engine will integrate forces supplied here during the
             *       physics step. Implementations should validate bodyId or
             *       ignore invalid ids gracefully.
             */
            void addForce( s32 bodyId, const Vector3<real_Num> &force,
                           const Vector3<real_Num> &pos ) override = 0;

            /**
             * @brief Apply a world-space force to a body (force applied at the
             *        body's center of mass).
             *
             * @param bodyId Engine-specific identifier of the rigid body.
             * @param force World-space force vector to apply (N or engine units).
             *
             * @note This overload applies the force at the center of mass and
             *       will only change linear momentum (no torque produced unless
             *       the implementation transforms it).
             */
            virtual void addForce( s32 bodyId, const Vector3<real_Num> &force ) = 0;

            /**
             * @brief Apply a world-space torque to a body.
             *
             * @param bodyId Engine-specific identifier of the rigid body.
             * @param torque World-space torque vector to apply (Nm or engine units).
             */
            void addTorque( s32 bodyId, const Vector3<real_Num> &torque ) override = 0;

            /**
             * @brief Apply a force expressed in the body's local coordinate
             *        system at a local position.
             *
             * @param bodyId Engine-specific identifier of the rigid body.
             * @param force Local-space force vector to apply.
             * @param pos   Local-space application point for the force.
             *
             * @note The implementation is responsible for transforming local
             *       quantities to world-space as needed.
             */
            void addLocalForce( s32 bodyId, const Vector3<real_Num> &force,
                                const Vector3<real_Num> &pos ) override = 0;

            /**
             * @brief Apply a torque expressed in the body's local coordinate system.
             *
             * @param bodyId Engine-specific identifier of the rigid body.
             * @param torque Local-space torque vector to apply.
             */
            void addLocalTorque( s32 bodyId, const Vector3<real_Num> &torque ) override = 0;

            /**
             * @brief Get the body's angular velocity in world-space.
             *
             * @return Vector3<real_Num> Angular velocity (radians per second).
             */
            Vector3<real_Num> getAngularVelocity() const override = 0;

            /**
             * @brief Get the body's linear velocity in world-space.
             *
             * @return Vector3<real_Num> Linear velocity (meters per second or engine units).
             */
            Vector3<real_Num> getLinearVelocity() const override = 0;

            /**
             * @brief Get the body's angular velocity in local (body) coordinates.
             *
             * @return Vector3<real_Num> Local angular velocity.
             */
            Vector3<real_Num> getLocalAngularVelocity() const override = 0;

            /**
             * @brief Get the body's linear velocity in local (body) coordinates.
             *
             * @return Vector3<real_Num> Local linear velocity.
             */
            Vector3<real_Num> getLocalLinearVelocity() const override = 0;

            /**
             * @brief Get the body's world-space position.
             *
             * @return Vector3<real_Num> Position in world coordinates.
             */
            Vector3<real_Num> getPosition() const override = 0;

            /**
             * @brief Get the body's world-space orientation.
             *
             * @return Quaternion<real_Num> Orientation quaternion (world relative).
             */
            Quaternion<real_Num> getOrientation() const override = 0;

            /**
             * @brief Retrieve the raw callback function pointer registered with
             *        the vehicle/aircraft.
             *
             * @return void* Pointer previously set via setCallbackFunction or nullptr.
             *
             * @note The pointer type is intentionally untyped here; callers must
             *       cast to the expected function signature.
             */
            void *getCallbackFunction() const override = 0;

            /**
             * @brief Register a raw callback function pointer.
             *
             * @param func Pointer to a function or callable object (untyped).
             *
             * @note Ownership and lifetime of the function pointer remain with
             *       the caller. Implementations should not attempt to free it.
             */
            void setCallbackFunction( void *func ) override = 0;

            /**
             * @brief Retrieve the raw data callback function pointer.
             *
             * @return void* Pointer previously set via setCallbackDataFunction or nullptr.
             */
            void *getCallbackDataFunction() const override = 0;

            /**
             * @brief Register a raw data-callback function pointer.
             *
             * @param func Pointer to a data-callback function (untyped).
             */
            void setCallbackDataFunction( void *func ) override = 0;

            /**
             * @brief Get the current input data snapshot for the aircraft.
             *
             * @return FixedArray<f32, 8> Array containing input channel values
             *         (range and meaning depend on platform-specific mapping).
             */
            FixedArray<f32, 8> getInputData() const override = 0;

            /**
             * @brief Get current control surface / control angles.
             *
             * @return FixedArray<f32, 11> Array of control angles in radians or
             *         engine units (mapping depends on aircraft implementation).
             */
            FixedArray<f32, 11> getControlAngles() const override = 0;

            /**
             * @brief Compute the point velocity at a specified world-space point.
             *
             * @param p World-space point where the velocity should be evaluated.
             * @return Vector3<real_Num> Velocity of the point in world-space.
             *
             * @note This typically combines linear and angular velocity of the
             *       body to compute the velocity at the given offset.
             */
            Vector3<real_Num> getPointVelocity( const Vector3<real_Num> &p ) override = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IAircraftCallback_h__
