#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace workphone
{
    namespace procedural
    {
        /// Engine-independent, SI-unit vehicle data generated alongside a procedural car.
        /// Coordinates match WPProcedural's vehicle meshes: +X right, +Y up, -Z forward.
        struct VehiclePhysicsVector3
        {
            double x = 0.0;
            double y = 0.0;
            double z = 0.0;
        };

        /// Symmetric inertia tensor about the vehicle centre of mass, in kg*m^2.
        struct VehicleInertiaTensor
        {
            double xx = 0.0;
            double yy = 0.0;
            double zz = 0.0;
            double xy = 0.0;
            double xz = 0.0;
            double yz = 0.0;
        };

        enum class VehiclePhysicsPreset
        {
            GrandPrix,
            GT,
            RoadSport
        };

        enum class DrivenAxle
        {
            Front,
            Rear,
            All
        };

        enum class WheelCorner : std::size_t
        {
            FrontLeft = 0,
            FrontRight,
            RearLeft,
            RearRight
        };

        struct VehicleMassElement
        {
            std::string name;
            double massKg = 0.0;
            VehiclePhysicsVector3 centre;
            /// Dimensions of an equivalent solid box, used to derive local inertia.
            VehiclePhysicsVector3 dimensions;
        };

        struct VehicleMassProperties
        {
            double massKg = 0.0;
            VehiclePhysicsVector3 centreOfMass;
            VehicleInertiaTensor inertia;
            double frontStaticWeightFraction = 0.5;
        };

        struct VehicleSuspensionConfig
        {
            double springRateNPerM = 0.0;
            double damperCompressionNPerMps = 0.0;
            double damperReboundNPerMps = 0.0;
            double antiRollRateNmPerRad = 0.0;
            double bumpTravelM = 0.0;
            double reboundTravelM = 0.0;
            double motionRatio = 1.0;
            double staticCamberRad = 0.0;
            double staticToeRad = 0.0;
            double casterRad = 0.0;
            double kingpinInclinationRad = 0.0;
        };

        struct VehicleTireConfig
        {
            double radiusM = 0.0;
            double widthM = 0.0;
            double wheelMassKg = 0.0;
            double wheelInertiaKgM2 = 0.0;
            double unloadedRadiusM = 0.0;
            double verticalStiffnessNPerM = 0.0;
            double longitudinalStiffnessNPerSlip = 0.0;
            double corneringStiffnessNPerRad = 0.0;
            double peakLongitudinalFriction = 0.0;
            double peakLateralFriction = 0.0;
            double rollingResistance = 0.0;
            double optimalTemperatureC = 0.0;
            double coldFrictionScale = 0.0;
            double loadSensitivity = 0.0;
        };

        struct VehicleWheelConfig
        {
            VehiclePhysicsVector3 hubPosition;
            VehicleSuspensionConfig suspension;
            VehicleTireConfig tire;
            bool steerable = false;
            bool driven = false;
            double maxSteerRad = 0.0;
            double brakeTorqueNm = 0.0;
            double handbrakeTorqueNm = 0.0;
        };

        struct VehicleAeroConfig
        {
            double referenceAreaM2 = 0.0;
            double dragCoefficient = 0.0;
            /// Positive coefficient produces downforce.
            double downforceCoefficient = 0.0;
            double frontDownforceFraction = 0.5;
            double airDensityKgPerM3 = 1.225;
            VehiclePhysicsVector3 centreOfPressure;
            double yawStabilityCoefficient = 0.0;
            double groundEffectRideHeightM = 0.0;
            double groundEffectSensitivity = 0.0;
        };

        struct VehicleDrivetrainConfig
        {
            DrivenAxle drivenAxle = DrivenAxle::Rear;
            double peakPowerW = 0.0;
            double peakTorqueNm = 0.0;
            double idleRpm = 0.0;
            double redlineRpm = 0.0;
            double finalDriveRatio = 1.0;
            double transmissionEfficiency = 1.0;
            std::vector<double> forwardGearRatios;
        };

        struct VehiclePhysicsConfig
        {
            std::uint64_t seed = 0;
            VehiclePhysicsPreset preset = VehiclePhysicsPreset::GrandPrix;
            double wheelbaseM = 0.0;
            double frontTrackM = 0.0;
            double rearTrackM = 0.0;
            double bodyLengthM = 0.0;
            double bodyWidthM = 0.0;
            double gravityMps2 = 9.80665;
            std::vector<VehicleMassElement> massElements;
            VehicleMassProperties massProperties;
            std::array<VehicleWheelConfig, 4> wheels;
            VehicleAeroConfig aero;
            VehicleDrivetrainConfig drivetrain;
        };

        struct VehiclePhysicsValidation
        {
            std::vector<std::string> errors;
            std::vector<std::string> warnings;

            bool isValid() const noexcept
            {
                return errors.empty();
            }
        };

    }  // namespace procedural
}  // namespace workphone
