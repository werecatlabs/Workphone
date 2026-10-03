#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPVehicleDynamics.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            constexpr double Pi = 3.1415926535897932384626433832795;
            constexpr double RadPerRpm = 2.0 * Pi / 60.0;
            constexpr double RpmPerRad = 60.0 / ( 2.0 * Pi );

            double clamp( double v, double lo, double hi )
            {
                return std::max( lo, std::min( v, hi ) );
            }

            double saturate( double v )
            {
                return clamp( v, 0.0, 1.0 );
            }

            double signNonZero( double v, double fallback )
            {
                return v > 0.0 ? 1.0 : ( v < 0.0 ? -1.0 : fallback );
            }

            bool finite( double v )
            {
                return std::isfinite( v );
            }

            bool finite( const VehiclePhysicsVector3 &v )
            {
                return finite( v.x ) && finite( v.y ) && finite( v.z );
            }

            VehiclePhysicsVector3 add( const VehiclePhysicsVector3 &a, const VehiclePhysicsVector3 &b )
            {
                return { a.x + b.x, a.y + b.y, a.z + b.z };
            }

            VehiclePhysicsVector3 mul( const VehiclePhysicsVector3 &v, double s )
            {
                return { v.x * s, v.y * s, v.z * s };
            }

            double dot( const VehiclePhysicsVector3 &a, const VehiclePhysicsVector3 &b )
            {
                return a.x * b.x + a.y * b.y + a.z * b.z;
            }

            VehiclePhysicsVector3 forwardForYaw( double yaw )
            {
                // Conventional right-handed +Y rotation applied to local -Z.
                return { -std::sin( yaw ), 0.0, -std::cos( yaw ) };
            }

            VehiclePhysicsVector3 rightForYaw( double yaw )
            {
                return { std::cos( yaw ), 0.0, -std::sin( yaw ) };
            }

            double smoothToward( double current, double target, double rate, double dt )
            {
                return current + ( target - current ) * ( 1.0 - std::exp( -rate * dt ) );
            }

            bool frontCorner( std::size_t i )
            {
                return i < 2;
            }

            bool leftCorner( std::size_t i )
            {
                return i == 0 || i == 2;
            }

            double staticAxleFraction( const VehiclePhysicsConfig &p, bool front )
            {
                return front ? p.massProperties.frontStaticWeightFraction
                             : 1.0 - p.massProperties.frontStaticWeightFraction;
            }

            double effectiveFriction( const VehicleTireConfig &tire, double normalLoad,
                                      double staticLoad, double temperatureC, double surfaceScale )
            {
                const double loadRatio = normalLoad / std::max( 1.0, staticLoad );
                const double loadScale =
                    clamp( 1.0 - tire.loadSensitivity * std::max( 0.0, loadRatio - 1.0 ), 0.68, 1.08 );
                const double optimum = std::max( 1.0, tire.optimalTemperatureC );
                const double warm =
                    saturate( ( temperatureC - 20.0 ) / std::max( 1.0, optimum - 20.0 ) );
                const double thermalScale =
                    tire.coldFrictionScale + ( 1.0 - tire.coldFrictionScale ) * warm;
                return clamp( surfaceScale, 0.05, 2.0 ) * loadScale * thermalScale;
            }

            double engineTorque( const VehicleDrivetrainConfig &d, double rpm )
            {
                const double span = std::max( 1.0, d.redlineRpm - d.idleRpm );
                const double x = saturate( ( rpm - d.idleRpm ) / span );
                // Broad motorsport torque curve with a controlled fall near redline.
                const double shape = 0.72 + 0.28 * std::sin( Pi * clamp( x * 0.88, 0.0, 0.88 ) );
                double torque = d.peakTorqueNm * shape;
                const double omega = std::max( d.idleRpm * RadPerRpm, rpm * RadPerRpm );
                torque = std::min( torque, d.peakPowerW / omega );
                if( rpm > d.redlineRpm )
                    torque *= clamp( 1.0 - ( rpm - d.redlineRpm ) / 600.0, 0.0, 1.0 );
                return std::max( 0.0, torque );
            }

            void addError( VehicleDynamicsValidation &v, const std::string &text )
            {
                v.errors.push_back( text );
            }

            bool stateIsFinite( const VehicleDynamicsState &state )
            {
                if( !finite( state.position ) || !finite( state.linearVelocity ) ||
                    !finite( state.yawRad ) || !finite( state.yawRateRadPerSec ) ||
                    !finite( state.engineRpm ) || !finite( state.shiftTimeRemaining ) ||
                    !finite( state.smoothedSteering ) ||
                    !finite( state.previousLongitudinalAccelerationMps2 ) ||
                    !finite( state.previousLateralAccelerationMps2 ) )
                    return false;
                for( const auto &wheel : state.wheels )
                    if( !finite( wheel.angularVelocityRadPerSec ) || !finite( wheel.steerAngleRad ) ||
                        !finite( wheel.suspensionCompressionM ) || !finite( wheel.tireTemperatureC ) )
                        return false;
                return true;
            }

            bool fastConfigCheck( const VehiclePhysicsConfig &physics,
                                  const VehicleDynamicsTuning &tuning )
            {
                return finite( physics.massProperties.massKg ) && physics.massProperties.massKg > 0.0 &&
                       finite( physics.massProperties.inertia.yy ) &&
                       physics.massProperties.inertia.yy > 0.0 &&
                       !physics.drivetrain.forwardGearRatios.empty() &&
                       finite( physics.drivetrain.finalDriveRatio ) &&
                       physics.drivetrain.finalDriveRatio > 0.0 &&
                       finite( tuning.steeringRatePerSecond ) && tuning.steeringRatePerSecond > 0.0 &&
                       finite( tuning.highSpeedSteerFraction ) && tuning.highSpeedSteerFraction > 0.0 &&
                       tuning.highSpeedSteerFraction <= 1.0 &&
                       finite( tuning.highSpeedSteerReferenceMps ) &&
                       tuning.highSpeedSteerReferenceMps > 0.0 &&
                       finite( tuning.clutchResponsePerSecond ) &&
                       tuning.clutchResponsePerSecond > 0.0 &&
                       finite( tuning.tireRelaxationPerSecond ) && tuning.tireRelaxationPerSecond > 0.0;
            }
        }  // namespace

        VehicleDynamicsState WPVehicleDynamics::reset( const VehiclePhysicsConfig &physics,
                                                       const VehiclePhysicsVector3 &position,
                                                       double yawRad )
        {
            VehicleDynamicsState state;
            state.position = position;
            state.yawRad = finite( yawRad ) ? yawRad : 0.0;
            state.engineRpm = physics.drivetrain.idleRpm;
            state.currentGear = physics.drivetrain.forwardGearRatios.empty() ? 0U : 1U;
            for( std::size_t i = 0; i < state.wheels.size(); ++i )
            {
                const auto &wheel = physics.wheels[i];
                const double load =
                    WPVehiclePhysics::staticWheelLoadN( physics, static_cast<WheelCorner>( i ) );
                state.wheels[i].suspensionCompressionM =
                    clamp( load / std::max( 1.0, wheel.tire.verticalStiffnessNPerM ), 0.0,
                           wheel.suspension.bumpTravelM );
                state.wheels[i].tireTemperatureC = 25.0;
            }
            return state;
        }

        VehicleDynamicsValidation WPVehicleDynamics::validate( const VehiclePhysicsConfig &physics,
                                                               const VehicleDynamicsTuning &tuning )
        {
            VehicleDynamicsValidation result;
            const auto physicsValidation = WPVehiclePhysics::validate( physics );
            for( const auto &error : physicsValidation.errors )
                addError( result, std::string( "physics: " ) + error );

            const double values[] = { tuning.steeringRatePerSecond,
                                      tuning.highSpeedSteerFraction,
                                      tuning.highSpeedSteerReferenceMps,
                                      tuning.shiftDurationSeconds,
                                      tuning.upshiftRpmFraction,
                                      tuning.downshiftRpmFraction,
                                      tuning.clutchResponsePerSecond,
                                      tuning.tireRelaxationPerSecond,
                                      tuning.sportYawDamping,
                                      tuning.arcadeYawDamping,
                                      tuning.arcadeLateralDamping };
            for( double value : values )
                if( !finite( value ) )
                {
                    addError( result, "dynamics tuning contains a non-finite value" );
                    break;
                }
            if( tuning.steeringRatePerSecond <= 0.0 )
                addError( result, "steering rate must be positive" );
            if( tuning.highSpeedSteerFraction <= 0.0 || tuning.highSpeedSteerFraction > 1.0 )
                addError( result, "high-speed steering fraction must be in (0,1]" );
            if( tuning.highSpeedSteerReferenceMps <= 0.0 )
                addError( result, "high-speed steering reference must be positive" );
            if( tuning.shiftDurationSeconds < 0.0 || tuning.shiftDurationSeconds > 1.0 )
                addError( result, "shift duration must be in [0,1]" );
            if( tuning.downshiftRpmFraction <= 0.0 ||
                tuning.upshiftRpmFraction <= tuning.downshiftRpmFraction ||
                tuning.upshiftRpmFraction > 1.1 )
                addError( result, "automatic shift fractions are inconsistent" );
            if( tuning.clutchResponsePerSecond <= 0.0 || tuning.tireRelaxationPerSecond <= 0.0 )
                addError( result, "clutch and tire relaxation rates must be positive" );
            if( tuning.sportYawDamping < 0.0 || tuning.arcadeYawDamping < 0.0 ||
                tuning.arcadeLateralDamping < 0.0 )
                addError( result, "assist damping cannot be negative" );
            return result;
        }

        VehicleDynamicsValidation WPVehicleDynamics::validateState( const VehiclePhysicsConfig &physics,
                                                                    const VehicleDynamicsState &state )
        {
            VehicleDynamicsValidation result;
            if( !finite( state.position ) || !finite( state.linearVelocity ) ||
                !finite( state.yawRad ) || !finite( state.yawRateRadPerSec ) ||
                !finite( state.engineRpm ) || !finite( state.shiftTimeRemaining ) ||
                !finite( state.smoothedSteering ) ||
                !finite( state.previousLongitudinalAccelerationMps2 ) ||
                !finite( state.previousLateralAccelerationMps2 ) )
                addError( result, "dynamics state contains a non-finite scalar" );
            if( state.currentGear == 0 ||
                state.currentGear > physics.drivetrain.forwardGearRatios.size() )
                addError( result, "current gear exceeds the configured gearbox" );
            if( state.engineRpm < 0.0 || state.engineRpm > physics.drivetrain.redlineRpm * 1.25 )
                addError( result, "engine speed is outside the permitted range" );
            for( const auto &wheel : state.wheels )
                if( !finite( wheel.angularVelocityRadPerSec ) || !finite( wheel.steerAngleRad ) ||
                    !finite( wheel.suspensionCompressionM ) || !finite( wheel.tireTemperatureC ) )
                {
                    addError( result, "wheel state contains a non-finite scalar" );
                    break;
                }
            return result;
        }

        bool WPVehicleDynamics::stepFixed( const VehiclePhysicsConfig &physics,
                                           const VehicleDynamicsTuning &tuning,
                                           const VehicleControlInput &input,
                                           const std::array<VehicleSurfaceSample, 4> &surfaces,
                                           double dt, VehicleDynamicsState &state,
                                           VehicleDynamicsTelemetry &telemetry )
        {
            return stepFixed( physics, tuning, input, surfaces, {}, dt, state, telemetry );
        }

        bool WPVehicleDynamics::stepFixed( const VehiclePhysicsConfig &physics,
                                           const VehicleDynamicsTuning &tuning,
                                           const VehicleControlInput &input,
                                           const std::array<VehicleSurfaceSample, 4> &surfaces,
                                           const VehicleDynamicsModifiers &modifiers, double dt,
                                           VehicleDynamicsState &state,
                                           VehicleDynamicsTelemetry &telemetry )
        {
            telemetry = {};
            if( dt <= 0.0 || dt > 0.025 || !finite( dt ) || !fastConfigCheck( physics, tuning ) ||
                !stateIsFinite( state ) || state.currentGear == 0 ||
                state.currentGear > physics.drivetrain.forwardGearRatios.size() ||
                !finite( modifiers.driveTorqueScale ) || !finite( modifiers.tireGripScale ) ||
                !finite( modifiers.steeringBias ) || modifiers.driveTorqueScale < 0.0 ||
                modifiers.driveTorqueScale > 2.0 || modifiers.tireGripScale <= 0.0 ||
                modifiers.tireGripScale > 2.0 )
                return false;

            for( const auto &surface : surfaces )
                if( !finite( surface.frictionScale ) || !finite( surface.rollingResistanceScale ) ||
                    !finite( surface.wetness ) || !finite( surface.groundHeightM ) ||
                    surface.frictionScale < 0.0 || surface.rollingResistanceScale < 0.0 )
                    return false;

            const double throttle =
                modifiers.controlsLocked ? 0.0 : saturate( input.throttle ) * modifiers.driveTorqueScale;
            const double brake = modifiers.controlsLocked ? 0.0 : saturate( input.brake );
            const double handbrake = modifiers.controlsLocked ? 0.0 : saturate( input.handbrake );
            const double steeringInput =
                clamp( ( modifiers.controlsLocked ? 0.0 : input.steering ) + modifiers.steeringBias,
                       -1.0, 1.0 );
            const double mass = physics.massProperties.massKg;
            const auto forward = forwardForYaw( state.yawRad );
            const auto right = rightForYaw( state.yawRad );
            double vLong = dot( state.linearVelocity, forward );
            double vLat = dot( state.linearVelocity, right );
            const double speed = std::sqrt( vLong * vLong + vLat * vLat );

            const double speedSteer =
                tuning.highSpeedSteerFraction + ( 1.0 - tuning.highSpeedSteerFraction ) /
                                                    ( 1.0 + speed / tuning.highSpeedSteerReferenceMps );
            state.smoothedSteering =
                smoothToward( state.smoothedSteering, steeringInput, tuning.steeringRatePerSecond, dt );
            double maxSteer = 0.0;
            for( const auto &wheel : physics.wheels )
                if( wheel.steerable )
                    maxSteer = std::max( maxSteer, wheel.maxSteerRad );
            const double centreSteer = state.smoothedSteering * maxSteer * speedSteer;
            telemetry.steeringAngleRad = centreSteer;

            // Ackermann steering using the narrowest track as the steering axle width.
            const double turnRadius = std::abs( centreSteer ) > 1e-5
                                          ? physics.wheelbaseM / std::tan( std::abs( centreSteer ) )
                                          : std::numeric_limits<double>::infinity();
            for( std::size_t i = 0; i < 4; ++i )
            {
                double steer = 0.0;
                if( physics.wheels[i].steerable )
                {
                    const bool inner = centreSteer > 0.0 ? leftCorner( i ) : !leftCorner( i );
                    const double halfTrack = 0.5 * physics.frontTrackM;
                    const double radius =
                        inner ? std::max( 0.2, turnRadius - halfTrack ) : turnRadius + halfTrack;
                    steer = std::copysign( std::atan2( physics.wheelbaseM, radius ), centreSteer );
                }
                state.wheels[i].steerAngleRad = steer;
            }

            auto &drivetrain = physics.drivetrain;
            const std::size_t gearCount = drivetrain.forwardGearRatios.size();
            if( gearCount == 0 )
                return false;
            state.shiftTimeRemaining = std::max( 0.0, state.shiftTimeRemaining - dt );

            double drivenOmega = 0.0;
            std::size_t drivenCount = 0;
            for( std::size_t i = 0; i < 4; ++i )
                if( physics.wheels[i].driven )
                {
                    drivenOmega += std::abs( state.wheels[i].angularVelocityRadPerSec );
                    ++drivenCount;
                }
            drivenOmega /= std::max<std::size_t>( 1, drivenCount );
            double ratio =
                drivetrain.forwardGearRatios[state.currentGear - 1] * drivetrain.finalDriveRatio;
            const double coupledRpm = drivenOmega * ratio * RpmPerRad;
            const double freeRevRpm =
                drivetrain.idleRpm + throttle * ( drivetrain.redlineRpm * 0.72 - drivetrain.idleRpm );
            const double targetRpm =
                std::max( drivetrain.idleRpm, std::max( coupledRpm, speed < 1.0 ? freeRevRpm : 0.0 ) );
            state.engineRpm =
                smoothToward( state.engineRpm, targetRpm, tuning.clutchResponsePerSecond, dt );
            state.engineRpm = clamp( state.engineRpm, drivetrain.idleRpm, drivetrain.redlineRpm * 1.1 );

            bool requestUp = input.shiftUp;
            bool requestDown = input.shiftDown;
            if( input.automaticTransmission && state.shiftTimeRemaining <= 0.0 )
            {
                requestUp = state.engineRpm >= drivetrain.redlineRpm * tuning.upshiftRpmFraction;
                requestDown = state.engineRpm <= drivetrain.redlineRpm * tuning.downshiftRpmFraction &&
                              state.currentGear > 1 && throttle < 0.92;
            }
            if( state.shiftTimeRemaining <= 0.0 && requestUp && state.currentGear < gearCount )
            {
                ++state.currentGear;
                state.shiftTimeRemaining = tuning.shiftDurationSeconds;
            }
            else if( state.shiftTimeRemaining <= 0.0 && requestDown && state.currentGear > 1 )
            {
                --state.currentGear;
                state.shiftTimeRemaining = tuning.shiftDurationSeconds;
            }
            ratio = drivetrain.forwardGearRatios[state.currentGear - 1] * drivetrain.finalDriveRatio;
            const double clutch = state.shiftTimeRemaining > 0.0 ? 0.0 : 1.0;
            const double crankTorque = engineTorque( drivetrain, state.engineRpm ) * throttle;
            const double totalDriveTorque =
                crankTorque * ratio * drivetrain.transmissionEfficiency * clutch;
            const double driveTorquePerWheel =
                totalDriveTorque / std::max<std::size_t>( 1, drivenCount );
            telemetry.drivelineTorqueNm = totalDriveTorque;
            telemetry.engineLoad = saturate( throttle * clutch );

            double compressionDelta = 0.0;
            for( std::size_t i = 0; i < 4; ++i )
            {
                const double staticCompression =
                    WPVehiclePhysics::staticWheelLoadN( physics, static_cast<WheelCorner>( i ) ) /
                    std::max( 1.0, physics.wheels[i].tire.verticalStiffnessNPerM );
                compressionDelta += state.wheels[i].suspensionCompressionM - staticCompression;
            }
            const double rideHeight =
                std::max( 0.015, physics.aero.groundEffectRideHeightM - 0.25 * compressionDelta );
            const double downforce =
                WPVehiclePhysics::aerodynamicDownforceN( physics, speed, rideHeight );
            const double dragMagnitude = WPVehiclePhysics::aerodynamicDragN( physics, speed );
            telemetry.aerodynamicDownforceN = downforce;
            telemetry.aerodynamicDragN = dragMagnitude;

            std::array<double, 4> loads{};
            for( std::size_t i = 0; i < 4; ++i )
                loads[i] = WPVehiclePhysics::staticWheelLoadN( physics, static_cast<WheelCorner>( i ) );
            loads[0] += downforce * physics.aero.frontDownforceFraction * 0.5;
            loads[1] += downforce * physics.aero.frontDownforceFraction * 0.5;
            loads[2] += downforce * ( 1.0 - physics.aero.frontDownforceFraction ) * 0.5;
            loads[3] += downforce * ( 1.0 - physics.aero.frontDownforceFraction ) * 0.5;

            const double cgHeight = std::max( 0.05, physics.massProperties.centreOfMass.y );
            const double longitudinalTransfer = mass * state.previousLongitudinalAccelerationMps2 *
                                                cgHeight / std::max( 0.1, physics.wheelbaseM );
            const double frontLoad = loads[0] + loads[1];
            const double rearLoad = loads[2] + loads[3];
            const double boundedLongitudinalTransfer =
                clamp( longitudinalTransfer, -0.95 * rearLoad, 0.95 * frontLoad );
            loads[0] -= boundedLongitudinalTransfer * 0.5;
            loads[1] -= boundedLongitudinalTransfer * 0.5;
            loads[2] += boundedLongitudinalTransfer * 0.5;
            loads[3] += boundedLongitudinalTransfer * 0.5;
            for( int axle = 0; axle < 2; ++axle )
            {
                const bool front = axle == 0;
                const double axleMass = mass * staticAxleFraction( physics, front );
                const double track = front ? physics.frontTrackM : physics.rearTrackM;
                const std::size_t left = front ? 0 : 2;
                const double axleLoad = loads[left] + loads[left + 1];
                const double transfer = clamp(
                    axleMass * state.previousLateralAccelerationMps2 * cgHeight / std::max( 0.1, track ),
                    -0.95 * axleLoad, 0.95 * axleLoad );
                loads[left] += transfer * 0.5;
                loads[left + 1] -= transfer * 0.5;
            }

            double totalLongForce = 0.0;
            double totalLatForce = 0.0;
            double yawMoment = 0.0;
            double maxSpin = 0.0;
            double maxLock = 0.0;
            for( std::size_t i = 0; i < 4; ++i )
            {
                const auto &wheel = physics.wheels[i];
                const auto &tire = wheel.tire;
                auto &wheelState = state.wheels[i];
                auto &out = telemetry.wheels[i];
                const auto &surface = surfaces[i];
                const double load = surface.grounded ? std::max( 0.0, loads[i] ) : 0.0;
                out.normalLoadN = load;
                out.grounded = surface.grounded;

                const double x = wheel.hubPosition.x - physics.massProperties.centreOfMass.x;
                const double z = wheel.hubPosition.z - physics.massProperties.centreOfMass.z;
                const double hubLong = vLong + state.yawRateRadPerSec * x;
                const double hubLat = vLat + state.yawRateRadPerSec * z;
                const double steer = wheelState.steerAngleRad;
                const double cs = std::cos( steer );
                const double sn = std::sin( steer );
                const double patchLong = hubLong * cs - hubLat * sn;
                const double patchLat = hubLat * cs + hubLong * sn;
                out.contactPatchSpeedMps = patchLong;

                const double speedFloor = 1.5;
                const double slipRatio =
                    ( wheelState.angularVelocityRadPerSec * tire.radiusM - patchLong ) /
                    std::max( speedFloor, std::abs( patchLong ) );
                const double slipAngle =
                    std::atan2( patchLat, std::max( speedFloor, std::abs( patchLong ) ) );
                out.longitudinalSlipRatio = slipRatio;
                out.slipAngleRad = slipAngle;

                const double staticLoad =
                    WPVehiclePhysics::staticWheelLoadN( physics, static_cast<WheelCorner>( i ) );
                const double muScale =
                    effectiveFriction( tire, load, staticLoad, wheelState.tireTemperatureC,
                                       surface.frictionScale * modifiers.tireGripScale *
                                           ( 1.0 - 0.32 * saturate( surface.wetness ) ) );
                const double longCap = tire.peakLongitudinalFriction * muScale * load;
                const double latCap = tire.peakLateralFriction * muScale * load;
                double fx = longCap > 0.0 ? longCap * std::tanh( tire.longitudinalStiffnessNPerSlip *
                                                                 slipRatio / std::max( 1.0, longCap ) )
                                          : 0.0;
                double fy = latCap > 0.0 ? -latCap * std::tanh( tire.corneringStiffnessNPerRad *
                                                                slipAngle / std::max( 1.0, latCap ) )
                                         : 0.0;
                const double ellipse = std::sqrt( ( fx * fx ) / std::max( 1.0, longCap * longCap ) +
                                                  ( fy * fy ) / std::max( 1.0, latCap * latCap ) );
                if( ellipse > 1.0 )
                {
                    fx /= ellipse;
                    fy /= ellipse;
                }
                out.longitudinalForceN = fx;
                out.lateralForceN = fy;
                out.combinedSlip = std::min( 2.0, ellipse );

                const double driveTorque = wheel.driven ? driveTorquePerWheel : 0.0;
                const double brakeTorque =
                    brake * wheel.brakeTorqueNm + handbrake * wheel.handbrakeTorqueNm;
                const double brakeDirection =
                    signNonZero( wheelState.angularVelocityRadPerSec, signNonZero( patchLong, 1.0 ) );
                double netWheelTorque = driveTorque - fx * tire.radiusM - brakeDirection * brakeTorque;
                if( !surface.grounded )
                    netWheelTorque = driveTorque - brakeDirection * brakeTorque;
                const double oldOmega = wheelState.angularVelocityRadPerSec;
                wheelState.angularVelocityRadPerSec +=
                    netWheelTorque / std::max( 0.01, tire.wheelInertiaKgM2 ) * dt;
                if( brakeTorque > 0.0 && oldOmega * wheelState.angularVelocityRadPerSec < 0.0 &&
                    std::abs( driveTorque ) < brakeTorque )
                    wheelState.angularVelocityRadPerSec = 0.0;
                if( surface.grounded && throttle < 0.01 && brake < 0.01 && handbrake < 0.01 &&
                    std::abs( slipRatio ) < 0.25 )
                    wheelState.angularVelocityRadPerSec = smoothToward(
                        wheelState.angularVelocityRadPerSec, patchLong / std::max( 0.01, tire.radiusM ),
                        tuning.tireRelaxationPerSecond, dt );

                const double suspensionTarget =
                    clamp( load / std::max( 1.0, tire.verticalStiffnessNPerM ),
                           -wheel.suspension.reboundTravelM, wheel.suspension.bumpTravelM );
                wheelState.suspensionCompressionM =
                    smoothToward( wheelState.suspensionCompressionM, suspensionTarget,
                                  0.5 * tuning.tireRelaxationPerSecond, dt );
                const double heat =
                    ( std::abs( fx * slipRatio ) + std::abs( fy * slipAngle ) ) * 0.00011;
                wheelState.tireTemperatureC +=
                    ( heat - ( wheelState.tireTemperatureC - 25.0 ) * 0.018 ) * dt;
                wheelState.tireTemperatureC = clamp( wheelState.tireTemperatureC, 0.0, 180.0 );

                const double bodyLong = fx * cs + fy * sn;
                const double bodyLat = -fx * sn + fy * cs;
                totalLongForce += bodyLong;
                totalLatForce += bodyLat;
                yawMoment += z * bodyLat + x * bodyLong;
                const double spin = std::max( 0.0, slipRatio );
                const double lock = std::max( 0.0, -slipRatio );
                maxSpin = std::max( maxSpin, spin );
                maxLock = std::max( maxLock, lock );
                const double rr = tire.rollingResistance * surface.rollingResistanceScale * load;
                totalLongForce -= signNonZero( patchLong, 0.0 ) * rr;
            }

            totalLongForce -= signNonZero( vLong, 0.0 ) * dragMagnitude;
            const double beta = std::atan2( vLat, std::max( 1.0, std::abs( vLong ) ) );
            const double dynamicPressure = 0.5 * physics.aero.airDensityKgPerM3 * speed * speed;
            yawMoment -= beta * dynamicPressure * physics.aero.referenceAreaM2 *
                         physics.aero.yawStabilityCoefficient * physics.wheelbaseM;

            if( tuning.assistMode != VehicleAssistMode::None )
            {
                const double yawDamping = tuning.assistMode == VehicleAssistMode::Sport
                                              ? tuning.sportYawDamping
                                              : tuning.arcadeYawDamping;
                yawMoment -= state.yawRateRadPerSec * physics.massProperties.inertia.yy * yawDamping;
                if( tuning.assistMode == VehicleAssistMode::Arcade && handbrake < 0.2 )
                    totalLatForce -= vLat * mass * tuning.arcadeLateralDamping;
            }

            const double accelLong = totalLongForce / mass;
            const double accelLat = totalLatForce / mass;
            const double yawAccel = yawMoment / std::max( 1.0, physics.massProperties.inertia.yy );
            const auto worldAcceleration = add( mul( forward, accelLong ), mul( right, accelLat ) );
            state.linearVelocity = add( state.linearVelocity, mul( worldAcceleration, dt ) );
            state.position = add( state.position, mul( state.linearVelocity, dt ) );
            state.yawRateRadPerSec += yawAccel * dt;
            state.yawRad += state.yawRateRadPerSec * dt;
            if( state.yawRad > Pi || state.yawRad < -Pi )
                state.yawRad = std::remainder( state.yawRad, 2.0 * Pi );
            state.previousLongitudinalAccelerationMps2 = accelLong;
            state.previousLateralAccelerationMps2 = accelLat;
            ++state.simulationTick;

            telemetry.speedMps = std::sqrt( dot( state.linearVelocity, state.linearVelocity ) );
            telemetry.longitudinalSpeedMps = vLong;
            telemetry.lateralSpeedMps = vLat;
            telemetry.longitudinalAccelerationMps2 = accelLong;
            telemetry.lateralAccelerationMps2 = accelLat;
            telemetry.wheelSpin = saturate( maxSpin / 0.35 );
            telemetry.wheelLock = saturate( maxLock / 0.35 );
            telemetry.driftFactor =
                saturate( ( std::abs( beta ) - 0.05 ) / 0.45 ) * saturate( speed / 12.0 );
            telemetry.brakeLight = saturate( brake + 0.75 * handbrake );
            return stateIsFinite( state ) && state.currentGear <= gearCount && state.engineRpm >= 0.0 &&
                   state.engineRpm <= physics.drivetrain.redlineRpm * 1.25;
        }
    }  // namespace procedural
}  // namespace workphone
