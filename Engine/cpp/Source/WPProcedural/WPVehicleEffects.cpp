#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPVehicleEffects.hpp"

#include <algorithm>
#include <cmath>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            double clamp( double value, double lo, double hi )
            {
                return std::max( lo, std::min( value, hi ) );
            }

            double saturate( double value )
            {
                return clamp( value, 0.0, 1.0 );
            }

            double random01( std::uint64_t &state )
            {
                state += 0x9e3779b97f4a7c15ULL;
                std::uint64_t z = state;
                z = ( z ^ ( z >> 30U ) ) * 0xbf58476d1ce4e5b9ULL;
                z = ( z ^ ( z >> 27U ) ) * 0x94d049bb133111ebULL;
                z ^= z >> 31U;
                return static_cast<double>( z >> 11U ) * ( 1.0 / 9007199254740992.0 );
            }

            VehiclePhysicsVector3 add( const VehiclePhysicsVector3 &a, const VehiclePhysicsVector3 &b )
            {
                return { a.x + b.x, a.y + b.y, a.z + b.z };
            }

            VehiclePhysicsVector3 mul( const VehiclePhysicsVector3 &v, double s )
            {
                return { v.x * s, v.y * s, v.z * s };
            }

            VehiclePhysicsVector3 worldPoint( const VehicleDynamicsState &state,
                                              const VehiclePhysicsVector3 &local )
            {
                const double c = std::cos( state.yawRad );
                const double s = std::sin( state.yawRad );
                return { state.position.x + local.x * c + local.z * s, state.position.y + local.y,
                         state.position.z - local.x * s + local.z * c };
            }

            VehiclePhysicsVector3 forward( const VehicleDynamicsState &state )
            {
                return { -std::sin( state.yawRad ), 0.0, -std::cos( state.yawRad ) };
            }

            VehiclePhysicsVector3 right( const VehicleDynamicsState &state )
            {
                return { std::cos( state.yawRad ), 0.0, -std::sin( state.yawRad ) };
            }

            bool leftCorner( std::size_t i )
            {
                return i == 0 || i == 2;
            }

            bool canAppend( const VehicleEffectConfig &config,
                            const std::vector<VehicleEffectEvent> &events, std::size_t startSize )
            {
                return events.size() - startSize < config.maximumEventsPerStep;
            }

            void jitterVelocity( VehiclePhysicsVector3 &velocity, VehicleEffectsState &state,
                                 double spread )
            {
                velocity.x += ( random01( state.randomState ) * 2.0 - 1.0 ) * spread;
                velocity.y += random01( state.randomState ) * spread;
                velocity.z += ( random01( state.randomState ) * 2.0 - 1.0 ) * spread;
            }
        }  // namespace

        VehicleEffectsState WPVehicleEffects::reset( std::uint64_t seed )
        {
            VehicleEffectsState state;
            state.randomState = seed ^ 0x46584556454e5453ULL;
            return state;
        }

        bool WPVehicleEffects::emit(
            const VehicleEffectConfig &config, const VehiclePhysicsConfig &physics,
            const VehicleDynamicsState &dynamics, const VehicleDynamicsTelemetry &telemetry,
            const VehiclePresentationState &presentation, const VehicleDamageTelemetry &damage,
            const VehicleEffectEnvironment &environment, double dt, VehicleEffectsState &state,
            std::vector<VehicleEffectEvent> &events )
        {
            if( !std::isfinite( dt ) || dt <= 0.0 || dt > 0.25 ||
                !std::isfinite( environment.wetness ) ||
                !std::isfinite( environment.ambientTemperatureC ) ||
                !std::isfinite( config.minimumSkidSpeedMps ) ||
                !std::isfinite( config.skidSlipThreshold ) ||
                !std::isfinite( config.smokeEventsPerSecond ) ||
                !std::isfinite( config.sprayEventsPerSecond ) ||
                !std::isfinite( config.exhaustEventsPerSecond ) ||
                !std::isfinite( config.damageSmokeEventsPerSecond ) ||
                !std::isfinite( config.skidWidthScale ) || config.minimumSkidSpeedMps < 0.0 ||
                config.skidSlipThreshold < 0.0 || config.smokeEventsPerSecond < 0.0 ||
                config.sprayEventsPerSecond < 0.0 || config.exhaustEventsPerSecond < 0.0 ||
                config.damageSmokeEventsPerSecond < 0.0 || config.skidWidthScale <= 0.0 ||
                config.maximumEventsPerStep == 0 )
                return false;

            const std::size_t startSize = events.size();
            if( !std::isfinite( dynamics.position.x ) || !std::isfinite( dynamics.position.y ) ||
                !std::isfinite( dynamics.position.z ) || !std::isfinite( dynamics.yawRad ) ||
                !std::isfinite( telemetry.speedMps ) || !std::isfinite( telemetry.wheelLock ) ||
                !std::isfinite( presentation.exhaustIntensity ) || !std::isfinite( damage.smoke ) ||
                !std::isfinite( state.exhaustAccumulator ) ||
                !std::isfinite( state.damageSmokeAccumulator ) || state.exhaustAccumulator < 0.0 ||
                state.damageSmokeAccumulator < 0.0 )
                return false;
            for( std::size_t i = 0; i < 4; ++i )
                if( !std::isfinite( telemetry.wheels[i].longitudinalSlipRatio ) ||
                    !std::isfinite( telemetry.wheels[i].slipAngleRad ) ||
                    !std::isfinite( state.smokeAccumulator[i] ) ||
                    !std::isfinite( state.sprayAccumulator[i] ) || state.smokeAccumulator[i] < 0.0 ||
                    state.sprayAccumulator[i] < 0.0 )
                    return false;
                else if( state.hasPreviousContact[i] &&
                         ( !std::isfinite( state.previousContact[i].x ) ||
                           !std::isfinite( state.previousContact[i].y ) ||
                           !std::isfinite( state.previousContact[i].z ) ) )
                    return false;
            const auto fwd = forward( dynamics );
            const auto rgt = right( dynamics );
            for( std::size_t i = 0; i < 4; ++i )
            {
                const auto &wheel = physics.wheels[i];
                const auto &t = telemetry.wheels[i];
                auto local = wheel.hubPosition;
                local.y = wheel.tire.radiusM * 0.04;
                const auto contact = worldPoint( dynamics, local );
                const double slip =
                    std::max( std::abs( t.longitudinalSlipRatio ), std::abs( t.slipAngleRad ) / 0.35 );
                const double slipIntensity =
                    saturate( ( slip - config.skidSlipThreshold ) /
                              std::max( 0.05, 1.0 - config.skidSlipThreshold ) );
                const bool skidding = t.grounded && telemetry.speedMps >= config.minimumSkidSpeedMps &&
                                      slipIntensity > 0.0;

                if( skidding && state.hasPreviousContact[i] && canAppend( config, events, startSize ) )
                {
                    VehicleEffectEvent event;
                    event.type = VehicleEffectType::SkidmarkSegment;
                    event.wheel = static_cast<WheelCorner>( i );
                    event.position = state.previousContact[i];
                    event.endPosition = contact;
                    event.linearColor = { 0.025, 0.025, 0.028 };
                    event.intensity = slipIntensity;
                    event.sizeM = wheel.tire.widthM * config.skidWidthScale;
                    event.lifetimeSeconds = 28.0;
                    events.push_back( event );
                }
                state.previousContact[i] = contact;
                state.hasPreviousContact[i] = t.grounded;

                const double drySmoke = slipIntensity * ( 1.0 - saturate( environment.wetness ) );
                state.smokeAccumulator[i] += drySmoke * config.smokeEventsPerSecond * dt;
                while( state.smokeAccumulator[i] >= 1.0 && canAppend( config, events, startSize ) )
                {
                    state.smokeAccumulator[i] -= 1.0;
                    VehicleEffectEvent event;
                    event.type = VehicleEffectType::TireSmoke;
                    event.wheel = static_cast<WheelCorner>( i );
                    event.position = contact;
                    event.velocity = add( mul( dynamics.linearVelocity, 0.12 ),
                                          VehiclePhysicsVector3{ 0.0, 0.8, 0.0 } );
                    jitterVelocity( event.velocity, state, 0.55 );
                    event.linearColor = { 0.34, 0.35, 0.37 };
                    event.intensity = drySmoke;
                    event.sizeM = wheel.tire.widthM * 1.6;
                    event.lifetimeSeconds = 0.75 + random01( state.randomState ) * 0.45;
                    events.push_back( event );
                }
                if( !canAppend( config, events, startSize ) )
                    state.smokeAccumulator[i] = std::fmod( state.smokeAccumulator[i], 1.0 );

                const double spray = saturate( environment.wetness ) *
                                     saturate( ( telemetry.speedMps - 5.0 ) / 30.0 ) *
                                     ( t.grounded ? 1.0 : 0.0 );
                state.sprayAccumulator[i] += spray * config.sprayEventsPerSecond * dt;
                while( state.sprayAccumulator[i] >= 1.0 && canAppend( config, events, startSize ) )
                {
                    state.sprayAccumulator[i] -= 1.0;
                    VehicleEffectEvent event;
                    event.type = VehicleEffectType::WaterSpray;
                    event.wheel = static_cast<WheelCorner>( i );
                    event.position = contact;
                    event.velocity = add( mul( fwd, telemetry.speedMps * -0.18 ),
                                          VehiclePhysicsVector3{ 0.0, 1.25, 0.0 } );
                    event.velocity = add( event.velocity, mul( rgt, leftCorner( i ) ? -0.65 : 0.65 ) );
                    jitterVelocity( event.velocity, state, 0.45 );
                    event.linearColor = { 0.48, 0.54, 0.64 };
                    event.intensity = spray;
                    event.sizeM = wheel.tire.widthM * 1.2;
                    event.lifetimeSeconds = 0.32 + random01( state.randomState ) * 0.22;
                    events.push_back( event );
                }
                if( !canAppend( config, events, startSize ) )
                    state.sprayAccumulator[i] = std::fmod( state.sprayAccumulator[i], 1.0 );

                if( telemetry.wheelLock > 0.82 && telemetry.speedMps > 18.0 &&
                    random01( state.randomState ) < dt * 2.5 && canAppend( config, events, startSize ) )
                {
                    VehicleEffectEvent event;
                    event.type = VehicleEffectType::BrakeSpark;
                    event.wheel = static_cast<WheelCorner>( i );
                    event.position = contact;
                    event.velocity = add( mul( dynamics.linearVelocity, 0.16 ),
                                          VehiclePhysicsVector3{ 0.0, 1.4, 0.0 } );
                    jitterVelocity( event.velocity, state, 1.4 );
                    event.linearColor = { 1.0, 0.34, 0.045 };
                    event.intensity = telemetry.wheelLock;
                    event.sizeM = 0.025;
                    event.lifetimeSeconds = 0.18;
                    events.push_back( event );
                }
            }

            const double rearZ =
                std::max( physics.wheels[2].hubPosition.z, physics.wheels[3].hubPosition.z ) +
                std::max( 0.15, ( physics.bodyLengthM - physics.wheelbaseM ) * 0.38 );
            state.exhaustAccumulator +=
                presentation.exhaustIntensity * config.exhaustEventsPerSecond * dt;
            while( state.exhaustAccumulator >= 1.0 && canAppend( config, events, startSize ) )
            {
                state.exhaustAccumulator -= 1.0;
                const double side = random01( state.randomState ) < 0.5 ? -1.0 : 1.0;
                VehicleEffectEvent event;
                event.type = presentation.exhaustIntensity > 0.72 ? VehicleEffectType::ExhaustFlame
                                                                  : VehicleEffectType::ExhaustHeatHaze;
                event.position =
                    worldPoint( dynamics, { side * physics.bodyWidthM * 0.16,
                                            physics.massProperties.centreOfMass.y * 0.72, rearZ } );
                event.velocity = add( mul( fwd, -2.5 - 3.0 * presentation.exhaustIntensity ),
                                      mul( dynamics.linearVelocity, 0.08 ) );
                jitterVelocity( event.velocity, state, 0.3 );
                event.linearColor = event.type == VehicleEffectType::ExhaustFlame
                                        ? VehiclePhysicsVector3{ 0.35, 0.62, 1.0 }
                                        : VehiclePhysicsVector3{ 0.7, 0.7, 0.7 };
                event.intensity = presentation.exhaustIntensity;
                event.sizeM = 0.14 + 0.20 * presentation.exhaustIntensity;
                event.lifetimeSeconds = event.type == VehicleEffectType::ExhaustFlame ? 0.09 : 0.22;
                events.push_back( event );
            }
            if( !canAppend( config, events, startSize ) )
                state.exhaustAccumulator = std::fmod( state.exhaustAccumulator, 1.0 );

            state.damageSmokeAccumulator += damage.smoke * config.damageSmokeEventsPerSecond * dt;
            while( state.damageSmokeAccumulator >= 1.0 && canAppend( config, events, startSize ) )
            {
                state.damageSmokeAccumulator -= 1.0;
                VehicleEffectEvent event;
                event.type = VehicleEffectType::DamageSmoke;
                event.position =
                    worldPoint( dynamics, { 0.0, physics.massProperties.centreOfMass.y + 0.12,
                                            physics.wheels[0].hubPosition.z * 0.52 } );
                event.velocity =
                    add( mul( dynamics.linearVelocity, 0.05 ), VehiclePhysicsVector3{ 0.0, 1.15, 0.0 } );
                jitterVelocity( event.velocity, state, 0.35 );
                event.linearColor = { 0.22, 0.23, 0.24 };
                event.intensity = damage.smoke;
                event.sizeM = 0.28 + damage.smoke * 0.38;
                event.lifetimeSeconds = 1.0 + random01( state.randomState ) * 0.7;
                events.push_back( event );
            }
            if( !canAppend( config, events, startSize ) )
                state.damageSmokeAccumulator = std::fmod( state.damageSmokeAccumulator, 1.0 );
            return true;
        }
    }  // namespace procedural
}  // namespace workphone
