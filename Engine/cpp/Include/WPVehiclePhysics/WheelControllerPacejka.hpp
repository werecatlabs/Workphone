#ifndef WheelControllerPacejka_h__
#define WheelControllerPacejka_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>
#include <WPVehiclePhysics/CVehicleComponent.hpp>

namespace workphone
{
    /**
     * @brief Wheel controller implementation using the Pacejka tire model.
     *
     * This class implements a realistic tire physics model based on the Pacejka Magic Formula,
     * which is widely used in vehicle dynamics simulation. The Pacejka model provides accurate
     * tire force calculations for both longitudinal (slip) and lateral (cornering) forces.
     *
     * The controller handles:
     * - Longitudinal and lateral tire force calculations using Pacejka coefficients
     * - Suspension dynamics with spring and damper simulation
     * - Slip ratio and slip angle calculations
     * - Ground contact detection and raycast-based contact point determination
     * - Combined tire force calculations for realistic tire behavior
     * - Torque application from drivetrain, braking, and steering inputs
     *
     * @see https://en.wikipedia.org/wiki/Hans_B._Pacejka for more information on the Pacejka model
     *
     * @note This implementation assumes metric units (meters, kilograms, Newtons)
     * @author Vehicle Physics System
     * @version 1.0
     */
    class WPVehiclePhysics_API WheelControllerPacejka : public CVehicleComponent<IWheelComponent>
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the wheel controller with default Pacejka coefficients and
         * reasonable default values for wheel parameters.
         */
        WheelControllerPacejka();

        /**
         * @brief Copy constructor (deleted).
         *
         * Copy construction is not allowed for this class.
         */
        WheelControllerPacejka(const WheelControllerPacejka &other) = delete;

        /**
         * @brief Destructor.
         */
        ~WheelControllerPacejka() override;

        /**
         * @brief Unloads the wheel controller and cleans up resources.
         * @param data Shared object data (unused in this implementation)
         */
        void unload(SmartPtr<ISharedObject> data) override;

        /**
         * @brief Updates the wheel controller physics simulation.
         *
         * This method is called each frame to update the wheel's state,
         * including suspension, tire forces, and angular velocity.
         */
        void update() override;

        // IWheelComponent interface implementations

        /**
         * @brief Adds torque to the wheel.
         * @param torque The amount of torque to add in Newton-meters
         * @note Currently not implemented in this version
         */
        void addTorque(physics_Num torque) override;

        /**
         * @brief Sets the torque applied to the wheel.
         * @param torque The torque value in Newton-meters
         * @note Currently not implemented in this version
         */
        void setTorque(physics_Num torque) override;

        /**
         * @brief Gets the current torque applied to the wheel.
         * @return The current torque in Newton-meters
         * @note Currently returns 0.0 in this implementation
         */
        physics_Num getTorque() const override;

        /**
         * @brief Gets the mass of the chassis portion supported by this wheel.
         * @return The mass in kilograms
         */
        physics_Num getMass() const override;

        /**
         * @brief Sets the mass of the chassis portion supported by this wheel.
         * @param mass The mass in kilograms
         */
        void setMass(physics_Num mass) override;

        /**
         * @brief Gets the wheel radius.
         * @return The wheel radius in meters
         */
        physics_Num getRadius() const override;

        /**
         * @brief Sets the wheel radius.
         * @param radius The wheel radius in meters
         */
        void setRadius(physics_Num radius) override;

        /**
         * @brief Gets the maximum suspension travel distance.
         * @return The suspension travel in meters
         */
        physics_Num getSuspensionTravel() const override;

        /**
         * @brief Sets the maximum suspension travel distance.
         * @param suspensionTravel The suspension travel in meters
         */
        void setSuspensionTravel(physics_Num suspensionTravel) override;

        /**
         * @brief Gets the suspension damping coefficient.
         * @return The damping coefficient in kg/s
         */
        physics_Num getDamping() const override;

        /**
         * @brief Sets the suspension damping coefficient.
         * @param damping The damping coefficient in kg/s
         */
        void setDamping(physics_Num damping) override;

        /**
         * @brief Gets the suspension spring rate.
         * @return The spring rate in N/m
         */
        physics_Num getSpringRate() const override;

        /**
         * @brief Sets the suspension spring rate.
         * @param springRate The spring rate in N/m
         */
        void setSpringRate(physics_Num springRate) override;

        /**
         * @brief Gets the current suspension compression distance.
         * @return The suspension distance in meters
         */
        physics_Num getSuspensionDistance() const override;

        /**
         * @brief Sets the current suspension compression distance.
         * @param suspensionDistance The suspension distance in meters
         */
        void setSuspensionDistance(physics_Num suspensionDistance) override;

        /**
         * @brief Gets the current steering angle of the wheel.
         * @return The steering angle in degrees
         */
        physics_Num getSteeringAngle() const override;

        /**
         * @brief Sets the steering angle of the wheel.
         * @param steeringAngle The steering angle in degrees
         */
        void setSteeringAngle(physics_Num steeringAngle) override;

        /**
         * @brief Checks if this wheel is a steering wheel.
         * @return True if this wheel responds to steering input
         */
        bool isSteeringWheel() const override;

        /**
         * @brief Sets whether this wheel is a steering wheel.
         * @param isSteeringWheel True if this wheel should respond to steering input
         */
        void setSteeringWheel(bool isSteeringWheel) override;

        // Pacejka-specific tire force calculation methods

        /**
         * @brief Calculates longitudinal tire force using the Pacejka formula.
         *
         * Computes the force in the direction of wheel rolling based on the normal load
         * and slip ratio using the Pacejka Magic Formula with B coefficients.
         *
         * @param Fz Normal force on the tire in Newtons
         * @param slip Slip ratio (dimensionless, typically -1.0 to 1.0)
         * @return Longitudinal force in Newtons
         */
        float calcLongitudinalForceUnit(float Fz, float slip);

        /**
         * @brief Calculates lateral tire force using the Pacejka formula.
         *
         * Computes the side force based on the normal load and slip angle
         * using the Pacejka Magic Formula with A coefficients.
         *
         * @param Fz Normal force on the tire in Newtons
         * @param slipAngle Slip angle in radians
         * @return Lateral force in Newtons
         */
        float calcLateralForceUnit(float Fz, float slipAngle);

        /**
         * @brief Calculates combined tire forces for both longitudinal and lateral directions.
         *
         * Uses a circle of friction approach to combine longitudinal and lateral forces
         * when both slip and slip angle are present, preventing unrealistic force magnitudes.
         *
         * @param Fz Normal force on the tire in Newtons
         * @param slip Slip ratio (dimensionless)
         * @param slipAngle Slip angle in radians
         * @return Combined force vector in local wheel coordinates
         */
        Vector3<physics_Num> combinedForce(float Fz, float slip, float slipAngle);

        /**
         * @brief Initializes the maximum slip and slip angle values for tire force calculations.
         *
         * This method finds the slip ratio and slip angle that produce maximum tire forces
         * by iterating through the Pacejka curves. These values are used for normalization
         * in the combined force calculations.
         */
        void initSlipMaxima();

        /**
         * @brief Calculates the current slip ratio of the tire.
         *
         * Slip ratio is the difference between wheel velocity and road velocity
         * relative to the road velocity. Positive values indicate acceleration,
         * negative values indicate braking.
         *
         * @return Slip ratio (dimensionless, typically -1.0 to 1.0)
         */
        float calculateSlipRatio();

        /**
         * @brief Calculates the current slip angle of the tire.
         *
         * Slip angle is the angle between the wheel's direction of travel
         * and the direction it's pointing, which generates lateral forces.
         *
         * @return Slip angle in radians
         */
        float calculateSlipAngle();

        /**
         * @brief Calculates the total road force applied to the wheel.
         *
         * This method integrates tire forces over multiple sub-steps to handle
         * the coupling between tire forces and wheel angular velocity accurately.
         *
         * @param dt Time step in seconds
         * @return Total road force vector in world coordinates
         */
        Vector3<physics_Num> roadForce(physics_Num dt);

        /**
         * @brief Calculates the suspension force based on compression and velocity.
         *
         * Combines spring force (proportional to compression) and damper force
         * (proportional to compression velocity) to generate the total suspension force.
         *
         * @return Suspension force vector in world coordinates
         */
        Vector3<physics_Num> suspensionForce();

        /**
         * @brief Calculates longitudinal tire force with full Pacejka formula.
         * @param Fz Normal force on the tire in Newtons
         * @param slip Slip ratio (dimensionless)
         * @return Longitudinal force in Newtons
         */
        float calcLongitudinalForce(float Fz, float slip);

        /**
         * @brief Calculates lateral tire force with full Pacejka formula.
         * @param Fz Normal force on the tire in Newtons
         * @param slipAngle Slip angle in radians
         * @return Lateral force in Newtons
         */
        float calcLateralForce(float Fz, float slipAngle);

        /**
         * @brief Main wheel update method called by the physics system.
         *
         * Performs ground contact detection, calculates tire and suspension forces,
         * and updates the wheel's angular velocity and position.
         *
         * @param task Current execution task ID
         * @param t Current time in seconds
         * @param dt Time step in seconds
         */
        void updateWheel(const int &task, const double &t, const double &dt);

        /**
         * @brief Gets the wheel's position relative to the vehicle.
         * @return Local position vector in vehicle coordinates
         */
        Vector3<physics_Num> getLocalPosition() const;

        /**
         * @brief Sets the wheel's position relative to the vehicle.
         * @param localPosition Local position vector in vehicle coordinates
         */
        void setLocalPosition(const Vector3<physics_Num> &localPosition);

        /**
         * @brief Gets the wheel's position in world coordinates.
         * @return World position vector
         */
        Vector3<physics_Num> getWorldPosition();

        /**
         * @brief Updates the compression force based on vehicle mass and gravity.
         *
         * Calculates the spring force needed to support the vehicle's weight
         * when the suspension is fully compressed.
         */
        void updateCompressionForce();

        /**
         * @brief Checks if this wheel receives power from the drivetrain.
         * @return True if this wheel is powered by the engine
         */
        bool isPoweredWheel() const override;

        /**
         * @brief Sets whether this wheel receives power from the drivetrain.
         * @param poweredWheel True if this wheel should be powered by the engine
         */
        void setPoweredWheel(bool poweredWheel) override;

        // Accessor methods for member variables

        /** @brief Gets current wheel velocity in world coordinates */
        Vector3<physics_Num> getWheelVelo() const;
        /** @brief Sets current wheel velocity in world coordinates */
        void setWheelVelo(const Vector3<physics_Num> &wheelVelo);

        /** @brief Gets current wheel velocity in local wheel coordinates */
        Vector3<physics_Num> getLocalVelo() const;
        /** @brief Sets current wheel velocity in local wheel coordinates */
        void setLocalVelo(const Vector3<physics_Num> &localVelo);

        /** @brief Gets ground normal vector at contact point */
        Vector3<physics_Num> getGroundNormal() const;
        /** @brief Sets ground normal vector at contact point */
        void setGroundNormal(const Vector3<physics_Num> &groundNormal);

        /** @brief Gets current suspension force vector */
        Vector3<physics_Num> getSuspensionForceVector() const;
        /** @brief Sets current suspension force vector */
        void setSuspensionForceVector(const Vector3<physics_Num> &suspensionForce);

        /** @brief Gets current road/tire force vector */
        Vector3<physics_Num> getRoadForceVector() const;
        /** @brief Sets current road/tire force vector */
        void setRoadForceVector(const Vector3<physics_Num> &roadForce);

        /** @brief Gets local up direction vector */
        Vector3<physics_Num> getUp() const;
        /** @brief Sets local up direction vector */
        void setUp(const Vector3<physics_Num> &up);

        /** @brief Gets local right direction vector */
        Vector3<physics_Num> getRight() const;
        /** @brief Sets local right direction vector */
        void setRight(const Vector3<physics_Num> &right);

        /** @brief Gets local forward direction vector */
        Vector3<physics_Num> getForward() const;
        /** @brief Sets local forward direction vector */
        void setForward(const Vector3<physics_Num> &forward);

        /** @brief Gets current wheel rotation quaternion */
        Quaternion<physics_Num> getLocalRotation() const;
        /** @brief Sets current wheel rotation quaternion */
        void setLocalRotation(const Quaternion<physics_Num> &localRotation);

        /** @brief Gets inverse of current wheel rotation quaternion */
        Quaternion<physics_Num> getInverseLocalRotation() const;
        /** @brief Sets inverse of current wheel rotation quaternion */
        void setInverseLocalRotation(const Quaternion<physics_Num> &inverseLocalRotation);

        /** @brief Gets raycast hit result for ground contact detection */
        SmartPtr<physics::IRaycastHit> getHit() const;
        /** @brief Sets raycast hit result for ground contact detection */
        void setHit(SmartPtr<physics::IRaycastHit> hit);

        /** @brief Gets wheel rotational inertia in kg⋅m² */
        physics_Num getInertia() const;
        /** @brief Sets wheel rotational inertia in kg⋅m² */
        void setInertia(physics_Num inertia);

        /** @brief Gets tire grip multiplier (scales all tire forces) */
        physics_Num getGrip() const;
        /** @brief Sets tire grip multiplier (scales all tire forces) */
        void setGrip(physics_Num grip) override;

        /** @brief Gets maximum brake torque in Newton-meters */
        physics_Num getBrakeFrictionTorque() const;
        /** @brief Sets maximum brake torque in Newton-meters */
        void setBrakeFrictionTorque(physics_Num brakeFrictionTorque);

        /** @brief Gets maximum handbrake torque in Newton-meters */
        physics_Num getHandbrakeFrictionTorque() const;
        /** @brief Sets maximum handbrake torque in Newton-meters */
        void setHandbrakeFrictionTorque(physics_Num handbrakeFrictionTorque);

        /** @brief Gets base rolling resistance torque in Newton-meters */
        physics_Num getFrictionTorque() const;
        /** @brief Sets base rolling resistance torque in Newton-meters */
        void setFrictionTorque(physics_Num frictionTorque);

        /** @brief Gets maximum steering angle in degrees */
        physics_Num getMaxSteeringAngle() const;
        /** @brief Sets maximum steering angle in degrees */
        void setMaxSteeringAngle(physics_Num maxSteeringAngle);

        /** @brief Gets fraction of vehicle mass supported by this wheel */
        physics_Num getMassFraction() const;
        /** @brief Sets fraction of vehicle mass supported by this wheel */
        void setMassFraction(physics_Num massFraction);

        /** @brief Gets engine torque applied to this wheel in Newton-meters */
        physics_Num getDriveTorque() const;
        /** @brief Sets engine torque applied to this wheel in Newton-meters */
        void setDriveTorque(physics_Num driveTorque);

        /** @brief Gets drivetrain friction torque in Newton-meters */
        physics_Num getDriveFrictionTorque() const;
        /** @brief Sets drivetrain friction torque in Newton-meters */
        void setDriveFrictionTorque(physics_Num driveFrictionTorque);

        /** @brief Gets brake input (0.0 to 1.0) */
        physics_Num getBrake() const override;
        /** @brief Sets brake input (0.0 to 1.0) */
        void setBrake(physics_Num brake) override;

        /** @brief Gets handbrake input (0.0 to 1.0) */
        physics_Num getHandbrake() const;
        /** @brief Sets handbrake input (0.0 to 1.0) */
        void setHandbrake(physics_Num handbrake);

        /** @brief Gets drivetrain inertia as seen by this wheel in kg⋅m² */
        physics_Num getDrivetrainInertia() const;
        /** @brief Sets drivetrain inertia as seen by this wheel in kg⋅m² */
        void setDrivetrainInertia(physics_Num drivetrainInertia);

        /** @brief Gets external suspension force (e.g., from anti-roll bars) in Newtons */
        physics_Num getSuspensionForceInput() const;
        /** @brief Sets external suspension force (e.g., from anti-roll bars) in Newtons */
        void setSuspensionForceInput(physics_Num suspensionForceInput);

        /** @brief Gets current wheel angular velocity in radians per second */
        physics_Num getAngularVelocity() const override;
        /** @brief Sets current wheel angular velocity in radians per second */
        void setAngularVelocity(physics_Num angularVelocity) override;

        /** @brief Gets current slip ratio (dimensionless) */
        physics_Num getSlipRatio() const;
        /** @brief Sets current slip ratio (dimensionless) */
        void setSlipRatio(physics_Num slipRatio);

        /** @brief Gets current slip velocity magnitude in m/s */
        physics_Num getSlipVelo() const;
        /** @brief Sets current slip velocity magnitude in m/s */
        void setSlipVelo(physics_Num slipVelo);

        /** @brief Gets current suspension compression (0.0 = uncompressed, 1.0 = fully compressed) */
        physics_Num getCompression() const override;
        /** @brief Sets current suspension compression (0.0 = uncompressed, 1.0 = fully compressed) */
        void setCompression(physics_Num compression);

        /** @brief Gets spring force at full compression in Newtons */
        physics_Num getFullCompressionSpringForce() const;
        /** @brief Sets spring force at full compression in Newtons */
        void setFullCompressionSpringForce(physics_Num fullCompressionSpringForce);

        /** @brief Gets current wheel rotation angle in radians */
        physics_Num getRotation() const;
        /** @brief Sets current wheel rotation angle in radians */
        void setRotation(physics_Num rotation);

        /** @brief Gets current normal force from ground in Newtons */
        physics_Num getNormalForce() const;
        /** @brief Sets current normal force from ground in Newtons */
        void setNormalForce(physics_Num normalForce);

        /** @brief Gets current slip angle in radians */
        physics_Num getSlipAngle() const;
        /** @brief Sets current slip angle in radians */
        void setSlipAngle(physics_Num slipAngle);

        /** @brief Gets maximum slip ratio for force normalization */
        physics_Num getMaxSlip() const;
        /** @brief Sets maximum slip ratio for force normalization */
        void setMaxSlip(physics_Num maxSlip);

        /** @brief Gets maximum slip angle for force normalization */
        physics_Num getMaxAngle() const;
        /** @brief Sets maximum slip angle for force normalization */
        void setMaxAngle(physics_Num maxAngle);

        /** @brief Gets previous steering angle for interpolation */
        physics_Num getOldAngle() const;

        /** @brief Sets previous steering angle for interpolation */
        void setOldAngle(physics_Num oldAngle);

        /** @brief Gets total vehicle mass in kilograms */
        physics_Num getChassisMass() const;

        /** @brief Sets total vehicle mass in kilograms */
        void setChassisMass(physics_Num chassisMass);

        /** @brief Gets last skid state for audio/visual effects */
        int getLastSkid() const;

        /** @brief Sets last skid state for audio/visual effects */
        void setLastSkid(int lastSkid);

        /** @brief Gets true if wheel is currently in contact with ground */
        bool isOnGround() const;

        /** @brief Sets true if wheel is currently in contact with ground */
        void setOnGround(bool onGround);

        /** @brief Gets Pacejka lateral force coefficients (A parameters) */
        const Array<physics_Num> &getPacejkaA() const;

        /** @brief Sets Pacejka lateral force coefficients (A parameters) */
        void setPacejkaA(const Array<physics_Num> &pacejkaA);

        /** @brief Gets Pacejka longitudinal force coefficients (B parameters) */
        const Array<physics_Num> &getPacejkaB() const;

        /** @brief Sets Pacejka longitudinal force coefficients (B parameters) */
        void setPacejkaB(const Array<physics_Num> &pacejkaB);

        TireModel getTireModel() const override;

        void setTireModel(TireModel tireModel) override;

        SmartPtr<Properties> getProperties() const override;
        void setProperties(SmartPtr<Properties> properties) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /** @brief Current wheel velocity in world coordinates */
        Vector3<physics_Num> m_wheelVelo;

        /** @brief Current wheel velocity in local wheel coordinates */
        Vector3<physics_Num> m_localVelo;

        /** @brief Ground normal vector at contact point */
        Vector3<physics_Num> m_groundNormal;

        /** @brief Current suspension force vector */
        Vector3<physics_Num> m_suspensionForce;

        /** @brief Current road/tire force vector */
        Vector3<physics_Num> m_roadForce;

        /** @brief Local up direction vector */
        Vector3<physics_Num> m_up;

        /** @brief Local right direction vector */
        Vector3<physics_Num> m_right;

        /** @brief Local forward direction vector */
        Vector3<physics_Num> m_forward;

        /** @brief Wheel position relative to vehicle center */
        Vector3<physics_Num> m_localPosition;

        /** @brief Current wheel rotation quaternion */
        Quaternion<physics_Num> m_localRotation;

        /** @brief Inverse of current wheel rotation quaternion */
        Quaternion<physics_Num> m_inverseLocalRotation;

        /** @brief Raycast hit result for ground contact detection */
        SmartPtr<physics::IRaycastHit> m_hit;

        // Wheel physical specifications

        /** @brief Wheel radius in meters */
        physics_Num m_radius = 0.0;

        /** @brief Maximum suspension travel distance in meters */
        physics_Num m_suspensionTravel = 0.0;

        /** @brief Suspension damping coefficient in kg/s */
        physics_Num m_damping = 0.0;

        /** @brief Wheel rotational inertia in kg⋅m² */
        physics_Num m_inertia = 0.0;

        /** @brief Tire grip multiplier (scales all tire forces) */
        physics_Num m_grip = 1.0;

        /** @brief Maximum brake torque in Newton-meters */
        physics_Num m_brakeFrictionTorque = 0.0;

        /** @brief Maximum handbrake torque in Newton-meters */
        physics_Num m_handbrakeFrictionTorque = 0.0;

        /** @brief Base rolling resistance torque in Newton-meters */
        physics_Num m_frictionTorque = 0.0;

        /** @brief Maximum steering angle in degrees */
        physics_Num m_maxSteeringAngle = 90.0;

        /** @brief Fraction of vehicle mass supported by this wheel */
        physics_Num m_massFraction = 0.25;

        // Input values from vehicle systems

        /** @brief Engine torque applied to this wheel in Newton-meters */
        physics_Num m_driveTorque = 0.0;

        /** @brief Drivetrain friction torque in Newton-meters */
        physics_Num m_driveFrictionTorque = 0.0;

        /** @brief Brake input (0.0 to 1.0) */
        physics_Num m_brake = 0.0;

        /** @brief Handbrake input (0.0 to 1.0) */
        physics_Num m_handbrake = 0.0;

        /** @brief Current steering angle input in degrees */
        physics_Num m_steeringAngle = 0.0;

        /** @brief Drivetrain inertia as seen by this wheel in kg⋅m² */
        physics_Num m_drivetrainInertia = 0.0;

        /** @brief External suspension force (e.g., from anti-roll bars) in Newtons */
        physics_Num m_suspensionForceInput = 0.0;

        // Output/state values

        /** @brief Current wheel angular velocity in radians per second */
        physics_Num m_angularVelocity = 0.0;

        /** @brief Current slip ratio (dimensionless) */
        physics_Num m_slipRatio = 0.0;

        /** @brief Current slip velocity magnitude in m/s */
        physics_Num m_slipVelo = 0.0;

        /** @brief Current suspension compression (0.0 = uncompressed, 1.0 = fully compressed) */
        physics_Num m_compression = 0.0;

        // Internal state variables

        /** @brief Spring force at full compression in Newtons */
        physics_Num m_fullCompressionSpringForce = 0.0;

        /** @brief Current wheel rotation angle in radians */
        physics_Num m_rotation = 0.0;

        /** @brief Current normal force from ground in Newtons */
        physics_Num m_normalForce = 0.0;

        /** @brief Current slip angle in radians */
        physics_Num m_slipAngle = 0.0;

        // Cached Pacejka calculation values

        /** @brief Maximum slip ratio for force normalization */
        physics_Num m_maxSlip = 0.0;

        /** @brief Maximum slip angle for force normalization */
        physics_Num m_maxAngle = 0.0;

        /** @brief Previous steering angle for interpolation */
        physics_Num m_oldAngle = 0.0;

        /** @brief Total vehicle mass in kilograms */
        physics_Num m_chassisMass = 0.0;

        /** @brief Suspension spring rate in N/m */
        physics_Num m_springRate = 1.0;

        /** @brief Current suspension distance in meters */
        physics_Num m_suspensionDistance = 1.0;

        /** @brief Last skid state for audio/visual effects */
        int m_lastSkid = 0;

        /** @brief True if wheel is currently in contact with ground */
        bool m_onGround = false;

        /** @brief True if this wheel responds to steering input */
        bool m_isSteeringWheel = false;

        /** @brief True if this wheel receives engine power */
        bool m_isPoweredWheel = false;

        /** @brief Pacejka lateral force coefficients (A parameters) */
        Array<physics_Num> m_pacejkaA;

        /** @brief Pacejka longitudinal force coefficients (B parameters) */
        Array<physics_Num> m_pacejkaB;

        /** @brief Static counter for generating unique wheel IDs */
        static s32 m_ext;
    };
} // namespace workphone

#endif // WheelControllerPacejka_h__
