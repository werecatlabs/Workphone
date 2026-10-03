#ifndef __WheelController_h__
#define __WheelController_h__

#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>
#include <Workphone/Vehicle/VehicleComponent.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @brief A concrete implementation of a vehicle wheel component.
         *
         * The WheelComponent class provides a complete wheel simulation for vehicles,
         * including suspension dynamics, steering control, torque application, and
         * physics-based wheel behavior. It supports both steering and powered wheels
         * and implements various tire models including Pacejka tire modeling.
         *
         * @author Vehicle System Team
         * @version 1.0
         * @since Engine 1.0
         */
        class WPCore_API WheelComponent : public VehicleComponent<IWheelComponent>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes the wheel component with default values for all physical
             * properties including mass, radius, suspension settings, and wheel state.
             */
            WheelComponent();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of wheel resources and physics objects.
             */
            ~WheelComponent() override;

            /**
             * @brief Updates the wheel component's state.
             *
             * Performs per-frame updates including suspension calculations,
             * tire force computation, and wheel rotation updates based on
             * the current physics state and applied forces.
             *
             * @see updateWheel()
             * @see updateWheelPacejka()
             */
            void update() override;

            /**
             * @brief Adds torque to the wheel.
             *
             * Accumulates the specified torque value with the current wheel torque.
             * This is typically used for engine torque application or braking.
             *
             * @param torque The torque value to add in Newton-meters (Nm)
             */
            void addTorque( real_Num torque ) override;

            /**
             * @brief Sets the absolute torque value for the wheel.
             *
             * Directly sets the wheel's torque, replacing any previously set value.
             *
             * @param torque The new torque value in Newton-meters (Nm)
             */
            void setTorque( real_Num torque ) override;

            /**
             * @brief Gets the current torque applied to the wheel.
             *
             * @return The current torque value in Newton-meters (Nm)
             */
            real_Num getTorque() const override;

            /**
             * @brief Gets the mass of the wheel.
             *
             * @return The wheel mass in kilograms (kg)
             */
            real_Num getMass() const override;

            /**
             * @brief Sets the mass of the wheel.
             *
             * The wheel mass affects suspension behavior and vehicle dynamics.
             *
             * @param mass The new wheel mass in kilograms (kg)
             */
            void setMass( real_Num mass ) override;

            /**
             * @brief Gets the suspension spring rate.
             *
             * @return The spring rate in Newtons per meter (N/m)
             */
            real_Num getSpringRate() const override;

            /**
             * @brief Sets the suspension spring rate.
             *
             * Higher spring rates result in stiffer suspension.
             *
             * @param springRate The new spring rate in Newtons per meter (N/m)
             */
            void setSpringRate( real_Num springRate ) override;

            /**
             * @brief Gets the wheel radius.
             *
             * @return The wheel radius in meters (m)
             */
            real_Num getRadius() const override;

            /**
             * @brief Sets the wheel radius.
             *
             * The wheel radius affects tire contact area and rolling dynamics.
             *
             * @param radius The new wheel radius in meters (m)
             */
            void setRadius( real_Num radius ) override;

            /**
             * @brief Gets the maximum suspension travel distance.
             *
             * @return The suspension travel in meters (m)
             */
            real_Num getSuspensionTravel() const override;

            /**
             * @brief Sets the maximum suspension travel distance.
             *
             * This defines how far the suspension can compress and extend.
             *
             * @param suspensionTravel The new suspension travel in meters (m)
             */
            void setSuspensionTravel( real_Num suspensionTravel ) override;

            /**
             * @brief Gets the suspension damping coefficient.
             *
             * @return The damping coefficient in Newton-seconds per meter (Ns/m)
             */
            real_Num getDamping() const override;

            /**
             * @brief Sets the suspension damping coefficient.
             *
             * Higher damping values reduce suspension oscillations.
             *
             * @param damping The new damping coefficient in Newton-seconds per meter (Ns/m)
             */
            void setDamping( real_Num damping ) override;

            /**
             * @brief Gets the current suspension compression distance.
             *
             * @return The current suspension distance in meters (m)
             */
            real_Num getSuspensionDistance() const override;

            /**
             * @brief Sets the current suspension compression distance.
             *
             * @param suspensionDistance The new suspension distance in meters (m)
             */
            void setSuspensionDistance( real_Num suspensionDistance ) override;

            /**
             * @brief Gets the current steering angle of the wheel.
             *
             * @return The steering angle in radians
             */
            real_Num getSteeringAngle() const override;

            /**
             * @brief Sets the steering angle of the wheel.
             *
             * Only affects wheels marked as steering wheels.
             *
             * @param steeringAngle The new steering angle in radians
             */
            void setSteeringAngle( real_Num steeringAngle ) override;

            /**
             * @brief Checks if this wheel can be steered.
             *
             * @return True if this is a steering wheel, false otherwise
             */
            bool isSteeringWheel() const override;

            /**
             * @brief Sets whether this wheel can be steered.
             *
             * @param steeringWheel True to make this a steering wheel, false otherwise
             */
            void setSteeringWheel( bool steeringWheel ) override;

            /**
             * @brief Checks if this wheel receives engine power.
             *
             * @return True if this is a powered wheel, false otherwise
             */
            bool isPoweredWheel() const override;

            /**
             * @brief Sets whether this wheel receives engine power.
             *
             * @param poweredWheel True to make this a powered wheel, false otherwise
             */
            void setPoweredWheel( bool poweredWheel ) override;

            /**
             * @brief Gets the angular velocity of the wheel.
             *
             * @return The angular velocity in radians per second (rad/s)
             */
            physics_Num getAngularVelocity() const override;

            /**
             * @brief Sets the angular velocity of the wheel.
             *
             * @param angularVelocity The new angular velocity in radians per second (rad/s)
             */
            void setAngularVelocity( physics_Num angularVelocity ) override;

            /**
             * @brief Gets the current brake input value.
             *
             * @return The brake input as a normalized value (0.0 to 1.0)
             */
            physics_Num getBrake() const override;

            /**
             * @brief Sets the brake input value.
             *
             * @param brake The brake input as a normalized value (0.0 to 1.0)
             */
            void setBrake( physics_Num brake ) override;

            /**
             * @brief Gets the current tire model used for simulation.
             *
             * @return The active tire model
             */
            TireModel getTireModel() const override;

            /**
             * @brief Sets the tire model to use for simulation.
             *
             * @param tireModel The tire model to activate
             */
            void setTireModel( TireModel tireModel ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Updates the wheel using standard tire model.
             *
             * Performs basic wheel physics calculations including suspension forces,
             * tire friction, and rotation dynamics.
             */
            void updateWheel();

            /**
             * @brief Updates the wheel using Pacejka tire model.
             *
             * Applies advanced Pacejka tire modeling for more realistic tire behavior,
             * including slip angle and slip ratio calculations for accurate force generation.
             */
            void updatePacejka();

            /**
             * @brief Updates the wheel using brush tire model.
             *
             * Applies brush tire modeling for a simplified tire behavior,
             * focusing on basic friction and slip characteristics.
             */
            void updateBrushTireModel();

            //! @brief Ray cast hit information for ground contact detection
            SmartPtr<physics::IRaycastHit> m_hit;

            //! @brief Tire grip coefficient (0.0 to 1.0+)
            physics_Num m_grip = physics_Num( 1.0 );

            //! @brief Rotational inertia of the wheel in kg⋅m²
            physics_Num m_wheelInertia = physics_Num( 1.0 );

            //! @brief Rolling resistance coefficient
            physics_Num m_rollingResistance = physics_Num( 0.1 );

            //! @brief Current applied torque in Newton-meters (Nm)
            physics_Num m_torque = static_cast<physics_Num>( 0.0 );

            //! @brief Maximum braking torque in Newton-meters (Nm)
            physics_Num m_brakeFrictionTorque = static_cast<physics_Num>( 4000.0 );

            //! @brief Maximum handbrake torque in Newton-meters (Nm)
            physics_Num m_handbrakeFrictionTorque = static_cast<physics_Num>( 0.0 );

            //! @brief Brake input value (0.0 to 1.0)
            physics_Num m_brake = static_cast<physics_Num>( 0.0 );

            //! @brief Handbrake input value (0.0 to 1.0)
            physics_Num m_handbrake = static_cast<physics_Num>( 0.0 );

            //! @brief Wheel mass in kilograms (kg)
            physics_Num m_mass = static_cast<physics_Num>( 0.0 );

            //! @brief Suspension spring rate in Newtons per meter (N/m)
            physics_Num m_springRate = static_cast<physics_Num>( 0.0 );

            //! @brief Wheel radius in meters (m)
            physics_Num m_radius = static_cast<physics_Num>( 0.35 );

            //! @brief Current suspension compression distance in meters (m)
            physics_Num m_suspensionDistance = static_cast<physics_Num>( 0.52 );

            //! @brief Suspension spring force in Newtons (N)
            physics_Num m_springForce = static_cast<physics_Num>( 1000.0 );

            //! @brief Suspension damping coefficient in Newton-seconds per meter (Ns/m)
            physics_Num m_damping = static_cast<physics_Num>( 4000.0 );

            //! @brief Mass fraction of the wheel relative to total vehicle mass
            physics_Num m_massFraction = static_cast<physics_Num>( 0.25 );

            //! @brief Angular velocity of the wheel in radians per second (rad/s)
            physics_Num m_angularVelocity = static_cast<physics_Num>( 0.0 );

            //! @brief Current steering angle in radians
            physics_Num m_steeringAngle = static_cast<physics_Num>( 0.0 );

            //! @brief Air resistance coefficient affecting wheel rotation
            physics_Num m_airResistance = static_cast<physics_Num>( 0.05 );

            //! @brief Linear velocity vector of the wheel in 3D space
            Vector3<physics_Num> m_wheelVelocity;

            //! @brief Flag indicating if this wheel can be steered
            bool m_isSteeringWheel = false;

            //! @brief Flag indicating if this wheel receives engine power
            bool m_isPoweredWheel = false;

            //! @brief Current tire model being used for simulation
            TireModel m_tireModel = TireModel::Simple;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // WheelController_h__
