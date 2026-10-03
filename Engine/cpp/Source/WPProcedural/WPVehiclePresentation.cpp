#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPVehiclePresentation.hpp"

#include <algorithm>
#include <cmath>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            constexpr double TwoPi = 6.283185307179586476925286766559;

            double clamp( double value, double lo, double hi )
            {
                return std::max( lo, std::min( value, hi ) );
            }

            double toward( double value, double target, double rate, double dt )
            {
                return value + ( target - value ) * ( 1.0 - std::exp( -rate * dt ) );
            }
        }  // namespace

        VehiclePresentationState WPVehiclePresentation::reset( const VehiclePhysicsConfig &physics )
        {
            VehiclePresentationState state;
            for( std::size_t i = 0; i < 4; ++i )
                state.wheels[i].hubPosition = physics.wheels[i].hubPosition;
            return state;
        }

        bool WPVehiclePresentation::update( const VehiclePresentationConfig &config,
                                            const VehiclePhysicsConfig &physics,
                                            const VehicleDynamicsState &dynamics,
                                            const VehicleDynamicsTelemetry &telemetry,
                                            const VehicleDamageTelemetry &damage,
                                            const VehiclePresentationInput &input, double dt,
                                            VehiclePresentationState &state )
        {
            const double configValues[] = {
                config.pitchRadiansPerMps2,     config.rollRadiansPerMps2,
                config.maximumPitchRad,         config.maximumRollRad,
                config.bodyResponsePerSecond,   config.lightResponsePerSecond,
                config.wheelBlurStartRadPerSec, config.wheelBlurFullRadPerSec
            };
            for( double value : configValues )
                if( !std::isfinite( value ) )
                    return false;
            if( !std::isfinite( dt ) || dt <= 0.0 || dt > 0.25 ||
                !std::isfinite( config.bodyResponsePerSecond ) ||
                !std::isfinite( config.lightResponsePerSecond ) || config.bodyResponsePerSecond <= 0.0 ||
                config.lightResponsePerSecond <= 0.0 || config.maximumPitchRad < 0.0 ||
                config.maximumRollRad < 0.0 ||
                config.wheelBlurFullRadPerSec <= config.wheelBlurStartRadPerSec )
                return false;
            const double inputValues[] = { input.throttle,
                                           input.backfire,
                                           telemetry.longitudinalAccelerationMps2,
                                           telemetry.lateralAccelerationMps2,
                                           telemetry.brakeLight,
                                           damage.smoke,
                                           state.bodyPitchRad,
                                           state.bodyRollRad,
                                           state.bodyHeaveM,
                                           state.brakeLightIntensity,
                                           state.headlightIntensity,
                                           state.rainLightIntensity,
                                           state.exhaustIntensity,
                                           state.damageSmokeIntensity };
            for( double value : inputValues )
                if( !std::isfinite( value ) )
                    return false;
            for( const auto &wheel : dynamics.wheels )
                if( !std::isfinite( wheel.angularVelocityRadPerSec ) ||
                    !std::isfinite( wheel.steerAngleRad ) ||
                    !std::isfinite( wheel.suspensionCompressionM ) )
                    return false;

            const double pitchTarget =
                clamp( -telemetry.longitudinalAccelerationMps2 * config.pitchRadiansPerMps2,
                       -config.maximumPitchRad, config.maximumPitchRad );
            const double rollTarget =
                clamp( telemetry.lateralAccelerationMps2 * config.rollRadiansPerMps2,
                       -config.maximumRollRad, config.maximumRollRad );
            double compressionDelta = 0.0;
            for( std::size_t i = 0; i < 4; ++i )
            {
                const double staticCompression =
                    WPVehiclePhysics::staticWheelLoadN( physics, static_cast<WheelCorner>( i ) ) /
                    std::max( 1.0, physics.wheels[i].tire.verticalStiffnessNPerM );
                compressionDelta += dynamics.wheels[i].suspensionCompressionM - staticCompression;
            }
            const double heaveTarget = -0.25 * compressionDelta;
            state.bodyPitchRad =
                toward( state.bodyPitchRad, pitchTarget, config.bodyResponsePerSecond, dt );
            state.bodyRollRad =
                toward( state.bodyRollRad, rollTarget, config.bodyResponsePerSecond, dt );
            state.bodyHeaveM = toward( state.bodyHeaveM, heaveTarget, config.bodyResponsePerSecond, dt );

            const double lightRate = config.lightResponsePerSecond;
            state.brakeLightIntensity = toward( state.brakeLightIntensity,
                                                clamp( telemetry.brakeLight, 0.0, 1.0 ), lightRate, dt );
            state.headlightIntensity =
                toward( state.headlightIntensity, input.headlightsEnabled ? 1.0 : 0.0, lightRate, dt );
            state.rainLightIntensity =
                toward( state.rainLightIntensity, input.rainLightEnabled ? 1.0 : 0.0, lightRate, dt );
            const double exhaustTarget =
                clamp( 0.12 + 0.58 * input.throttle + input.backfire, 0.0, 1.0 );
            state.exhaustIntensity = toward( state.exhaustIntensity, exhaustTarget, lightRate, dt );
            state.damageSmokeIntensity = toward( state.damageSmokeIntensity,
                                                 clamp( damage.smoke, 0.0, 1.0 ), 0.45 * lightRate, dt );

            for( std::size_t i = 0; i < 4; ++i )
            {
                auto &pose = state.wheels[i];
                pose.hubPosition = physics.wheels[i].hubPosition;
                pose.steerAngleRad = dynamics.wheels[i].steerAngleRad;
                pose.rotationAngleRad = std::remainder(
                    pose.rotationAngleRad + dynamics.wheels[i].angularVelocityRadPerSec * dt, TwoPi );
                const double staticCompression =
                    WPVehiclePhysics::staticWheelLoadN( physics, static_cast<WheelCorner>( i ) ) /
                    std::max( 1.0, physics.wheels[i].tire.verticalStiffnessNPerM );
                pose.suspensionOffsetM = dynamics.wheels[i].suspensionCompressionM - staticCompression;
                const double omega = std::abs( dynamics.wheels[i].angularVelocityRadPerSec );
                pose.rotationalBlur =
                    clamp( ( omega - config.wheelBlurStartRadPerSec ) /
                               ( config.wheelBlurFullRadPerSec - config.wheelBlurStartRadPerSec ),
                           0.0, 1.0 );
            }
            return true;
        }
    }  // namespace procedural
}  // namespace workphone
