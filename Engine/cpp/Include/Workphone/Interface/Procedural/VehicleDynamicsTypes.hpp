#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/VehiclePhysicsTypes.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace workphone
{
    namespace procedural
    {
        enum class VehicleAssistMode
        {
            None,
            Sport,
            Arcade
        };

        struct WPCore_API VehicleControlInput
        {
            double throttle = 0.0;
            double brake = 0.0;
            double steering = 0.0;
            double handbrake = 0.0;
            bool automaticTransmission = true;
            bool shiftUp = false;
            bool shiftDown = false;
        };

        /// Contact properties supplied by the world for one wheel.
        struct WPCore_API VehicleSurfaceSample
        {
            double frictionScale = 1.0;
            double rollingResistanceScale = 1.0;
            double wetness = 0.0;
            double groundHeightM = 0.0;
            bool grounded = true;
        };

        struct WPCore_API VehicleDynamicsTuning
        {
            VehicleAssistMode assistMode = VehicleAssistMode::None;
            double steeringRatePerSecond = 8.0;
            double highSpeedSteerFraction = 0.42;
            double highSpeedSteerReferenceMps = 75.0;
            double shiftDurationSeconds = 0.085;
            double upshiftRpmFraction = 0.965;
            double downshiftRpmFraction = 0.48;
            double clutchResponsePerSecond = 16.0;
            double tireRelaxationPerSecond = 18.0;
            double sportYawDamping = 0.20;
            double arcadeYawDamping = 0.65;
            double arcadeLateralDamping = 1.8;
        };

        /// Runtime modifiers from damage, assists, power limits or gameplay rules.
        struct WPCore_API VehicleDynamicsModifiers
        {
            double driveTorqueScale = 1.0;
            double tireGripScale = 1.0;
            double steeringBias = 0.0;
            bool controlsLocked = false;
        };

        struct WPCore_API VehicleWheelDynamicsState
        {
            double angularVelocityRadPerSec = 0.0;
            double steerAngleRad = 0.0;
            double suspensionCompressionM = 0.0;
            double tireTemperatureC = 25.0;
        };

        struct WPCore_API VehicleDynamicsState
        {
            VehiclePhysicsVector3 position;
            VehiclePhysicsVector3 linearVelocity;
            /// Right-handed rotation about +Y. Zero faces local/world -Z.
            double yawRad = 0.0;
            double yawRateRadPerSec = 0.0;
            double engineRpm = 0.0;
            std::uint32_t currentGear = 1;
            double shiftTimeRemaining = 0.0;
            double smoothedSteering = 0.0;
            double previousLongitudinalAccelerationMps2 = 0.0;
            double previousLateralAccelerationMps2 = 0.0;
            std::array<VehicleWheelDynamicsState, 4> wheels;
            std::uint64_t simulationTick = 0;
        };

        struct WPCore_API VehicleWheelTelemetry
        {
            double normalLoadN = 0.0;
            double longitudinalSlipRatio = 0.0;
            double slipAngleRad = 0.0;
            double longitudinalForceN = 0.0;
            double lateralForceN = 0.0;
            double combinedSlip = 0.0;
            double contactPatchSpeedMps = 0.0;
            bool grounded = false;
        };

        struct WPCore_API VehicleDynamicsTelemetry
        {
            double speedMps = 0.0;
            double longitudinalSpeedMps = 0.0;
            double lateralSpeedMps = 0.0;
            double longitudinalAccelerationMps2 = 0.0;
            double lateralAccelerationMps2 = 0.0;
            double steeringAngleRad = 0.0;
            double engineLoad = 0.0;
            double drivelineTorqueNm = 0.0;
            double aerodynamicDragN = 0.0;
            double aerodynamicDownforceN = 0.0;
            double wheelSpin = 0.0;
            double wheelLock = 0.0;
            double driftFactor = 0.0;
            double brakeLight = 0.0;
            std::array<VehicleWheelTelemetry, 4> wheels;
        };

        struct WPCore_API VehicleDynamicsValidation
        {
            std::vector<std::string> errors;

            bool isValid() const noexcept
            {
                return errors.empty();
            }
        };

    }  // namespace procedural
}  // namespace workphone
