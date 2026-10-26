#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/System/SourceBudgetStdString.hpp>
#include <Workphone/System/SourceBudgetVector.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace workphone
{
    namespace procedural
    {
        /// Renderer-neutral, SI-unit vehicle data generated alongside procedural vehicles.
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
            RoadSport,
            HelicopterLittleBird,  ///< Static helicopter asset metadata; no car/flight dynamics.
            Warbird                ///< Static fixed-wing asset metadata; no flight dynamics.
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

        struct VehicleMassElement : SourceBudgetObject
        {
            SourceBudgetStdString name;
            double massKg = 0.0;
            VehiclePhysicsVector3 centre;
            /// Dimensions of an equivalent solid box, used to derive local inertia.
            VehiclePhysicsVector3 dimensions;

            VehicleMassElement() = default;

            VehicleMassElement( SourceBudgetStdString name, double massKg, VehiclePhysicsVector3 centre,
                                VehiclePhysicsVector3 dimensions ) :
                name( std::move( name ) ),
                massKg( massKg ),
                centre( centre ),
                dimensions( dimensions )
            {
            }

            VehicleMassElement( const VehicleMassElement & ) = default;
            VehicleMassElement( VehicleMassElement && ) noexcept = default;
            VehicleMassElement &operator=( VehicleMassElement && ) noexcept = default;

            VehicleMassElement &operator=( const VehicleMassElement &other )
            {
                if( this != &other )
                {
                    VehicleMassElement candidate( other );
                    swap( candidate );
                }
                return *this;
            }

            void swap( VehicleMassElement &other ) noexcept
            {
                using std::swap;
                name.swap( other.name );
                swap( massKg, other.massKg );
                swap( centre, other.centre );
                swap( dimensions, other.dimensions );
            }
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

        struct VehicleDrivetrainConfig : SourceBudgetObject
        {
            DrivenAxle drivenAxle = DrivenAxle::Rear;
            double peakPowerW = 0.0;
            double peakTorqueNm = 0.0;
            double idleRpm = 0.0;
            double redlineRpm = 0.0;
            double finalDriveRatio = 1.0;
            double transmissionEfficiency = 1.0;
            SourceBudgetVector<double> forwardGearRatios;

            VehicleDrivetrainConfig() = default;

            VehicleDrivetrainConfig( DrivenAxle axle, double power, double torque, double idle,
                                     double redline, double finalRatio, double efficiency,
                                     SourceBudgetVector<double> ratios ) :
                drivenAxle( axle ),
                peakPowerW( power ),
                peakTorqueNm( torque ),
                idleRpm( idle ),
                redlineRpm( redline ),
                finalDriveRatio( finalRatio ),
                transmissionEfficiency( efficiency ),
                forwardGearRatios( std::move( ratios ) )
            {
            }

            VehicleDrivetrainConfig( const VehicleDrivetrainConfig & ) = default;
            VehicleDrivetrainConfig( VehicleDrivetrainConfig && ) noexcept = default;
            VehicleDrivetrainConfig &operator=( VehicleDrivetrainConfig && ) noexcept = default;

            VehicleDrivetrainConfig &operator=( const VehicleDrivetrainConfig &other )
            {
                if( this != &other )
                {
                    VehicleDrivetrainConfig candidate( other );
                    swap( candidate );
                }
                return *this;
            }

            void swap( VehicleDrivetrainConfig &other ) noexcept
            {
                using std::swap;
                swap( drivenAxle, other.drivenAxle );
                swap( peakPowerW, other.peakPowerW );
                swap( peakTorqueNm, other.peakTorqueNm );
                swap( idleRpm, other.idleRpm );
                swap( redlineRpm, other.redlineRpm );
                swap( finalDriveRatio, other.finalDriveRatio );
                swap( transmissionEfficiency, other.transmissionEfficiency );
                forwardGearRatios.swap( other.forwardGearRatios );
            }
        };

        struct VehiclePhysicsConfig : SourceBudgetObject
        {
            std::uint64_t seed = 0;
            VehiclePhysicsPreset preset = VehiclePhysicsPreset::GrandPrix;
            double wheelbaseM = 0.0;
            double frontTrackM = 0.0;
            double rearTrackM = 0.0;
            double bodyLengthM = 0.0;
            double bodyWidthM = 0.0;
            double gravityMps2 = 9.80665;
            SourceBudgetVector<VehicleMassElement> massElements;
            VehicleMassProperties massProperties;
            std::array<VehicleWheelConfig, 4> wheels;
            VehicleAeroConfig aero;
            VehicleDrivetrainConfig drivetrain;

            VehiclePhysicsConfig() = default;
            VehiclePhysicsConfig( const VehiclePhysicsConfig & ) = default;
            VehiclePhysicsConfig( VehiclePhysicsConfig && ) noexcept = default;
            VehiclePhysicsConfig &operator=( VehiclePhysicsConfig && ) noexcept = default;

            VehiclePhysicsConfig &operator=( const VehiclePhysicsConfig &other )
            {
                if( this != &other )
                {
                    VehiclePhysicsConfig candidate( other );
                    swap( candidate );
                }
                return *this;
            }

            void swap( VehiclePhysicsConfig &other ) noexcept
            {
                using std::swap;
                swap( seed, other.seed );
                swap( preset, other.preset );
                swap( wheelbaseM, other.wheelbaseM );
                swap( frontTrackM, other.frontTrackM );
                swap( rearTrackM, other.rearTrackM );
                swap( bodyLengthM, other.bodyLengthM );
                swap( bodyWidthM, other.bodyWidthM );
                swap( gravityMps2, other.gravityMps2 );
                massElements.swap( other.massElements );
                swap( massProperties, other.massProperties );
                wheels.swap( other.wheels );
                swap( aero, other.aero );
                drivetrain.swap( other.drivetrain );
            }
        };

        struct VehiclePhysicsValidation : SourceBudgetObject
        {
            SourceBudgetVector<SourceBudgetStdString> errors;
            SourceBudgetVector<SourceBudgetStdString> warnings;

            VehiclePhysicsValidation() = default;
            VehiclePhysicsValidation( const VehiclePhysicsValidation & ) = default;
            VehiclePhysicsValidation( VehiclePhysicsValidation && ) noexcept = default;
            VehiclePhysicsValidation &operator=( VehiclePhysicsValidation && ) noexcept = default;

            VehiclePhysicsValidation &operator=( const VehiclePhysicsValidation &other )
            {
                if( this != &other )
                {
                    VehiclePhysicsValidation candidate( other );
                    swap( candidate );
                }
                return *this;
            }

            void swap( VehiclePhysicsValidation &other ) noexcept
            {
                errors.swap( other.errors );
                warnings.swap( other.warnings );
            }

            bool isValid() const noexcept
            {
                return errors.empty();
            }
        };

    }  // namespace procedural
}  // namespace workphone
