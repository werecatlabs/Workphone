#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPVehicleDamage.hpp"

#include <algorithm>
#include <cmath>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            double clamp( double v, double lo, double hi )
            {
                return std::max( lo, std::min( v, hi ) );
            }

            bool finite( double v )
            {
                return std::isfinite( v );
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
        }  // namespace

        VehicleDamageState WPVehicleDamage::reset( std::uint64_t seed )
        {
            VehicleDamageState state;
            state.randomState = seed ^ 0x44414d414745ULL;
            return state;
        }

        VehicleDamageValidation WPVehicleDamage::validate( const VehicleDamageConfig &c )
        {
            VehicleDamageValidation result;
            const double values[] = {
                c.minimumSeverity,         c.hitCooldownSeconds,        c.wreckSeverity,
                c.damagePerSeverity,       c.permanentFraction,         c.maximumPermanentDamage,
                c.repairDelaySeconds,      c.repairRatePerSecond,       c.engineCutSeconds,
                c.wreckControlLockSeconds, c.torquePenaltyAtFullDamage, c.gripPenaltyAtFullDamage,
                c.steeringPullAtFullDamage
            };
            for( double value : values )
                if( !finite( value ) )
                {
                    result.errors.push_back( "damage configuration contains a non-finite value" );
                    return result;
                }
            if( c.minimumSeverity < 0.0 || c.minimumSeverity > 1.0 ||
                c.wreckSeverity <= c.minimumSeverity || c.wreckSeverity > 1.0 )
                result.errors.push_back( "damage severity thresholds are inconsistent" );
            if( c.hitCooldownSeconds < 0.0 || c.damagePerSeverity < 0.0 || c.repairDelaySeconds < 0.0 ||
                c.repairRatePerSecond < 0.0 || c.engineCutSeconds < 0.0 ||
                c.wreckControlLockSeconds < 0.0 )
                result.errors.push_back( "damage times and rates cannot be negative" );
            if( c.permanentFraction < 0.0 || c.permanentFraction > 1.0 ||
                c.maximumPermanentDamage < 0.0 || c.maximumPermanentDamage > 1.0 )
                result.errors.push_back( "permanent damage settings must be in [0,1]" );
            if( c.torquePenaltyAtFullDamage < 0.0 || c.torquePenaltyAtFullDamage > 0.95 ||
                c.gripPenaltyAtFullDamage < 0.0 || c.gripPenaltyAtFullDamage > 0.95 ||
                c.steeringPullAtFullDamage < 0.0 || c.steeringPullAtFullDamage > 0.5 )
                result.errors.push_back( "damage penalties exceed safe limits" );
            return result;
        }

        VehicleDamageValidation WPVehicleDamage::validateState( const VehicleDamageState &s )
        {
            VehicleDamageValidation result;
            const double values[] = { s.damage,
                                      s.permanentDamage,
                                      s.hitCooldownRemaining,
                                      s.secondsSinceImpact,
                                      s.engineCutRemaining,
                                      s.controlLockRemaining,
                                      s.trauma,
                                      s.smokeBurst,
                                      s.steeringPullSign,
                                      s.lastSeverity };
            for( double value : values )
                if( !finite( value ) )
                {
                    result.errors.push_back( "damage state contains a non-finite value" );
                    return result;
                }
            if( s.damage < 0.0 || s.damage > 1.0 || s.permanentDamage < 0.0 ||
                s.permanentDamage > s.damage + 1e-9 || s.hitCooldownRemaining < 0.0 ||
                s.engineCutRemaining < 0.0 || s.controlLockRemaining < 0.0 )
                result.errors.push_back( "damage state is outside its permitted range" );
            return result;
        }

        VehicleDamageEvent WPVehicleDamage::registerImpact( const VehicleDamageConfig &config,
                                                            const VehiclePhysicsConfig &physics,
                                                            const VehicleDamageImpact &impact,
                                                            VehicleDamageState &state )
        {
            VehicleDamageEvent event;
            if( !validate( config ).isValid() || !validateState( state ).isValid() ||
                !finite( impact.closingSpeedMps ) || !finite( impact.impulseNs ) ||
                !finite( impact.severity ) || impact.closingSpeedMps < 0.0 || impact.impulseNs < 0.0 ||
                physics.massProperties.massKg <= 0.0 )
                return event;

            const double speedSeverity = impact.closingSpeedMps / 52.0;
            const double impulseSeverity =
                impact.impulseNs / std::max( 1.0, physics.massProperties.massKg * 30.0 );
            const double severity = clamp(
                impact.severity >= 0.0 ? impact.severity : std::max( speedSeverity, impulseSeverity ),
                0.0, 1.0 );
            event.severity = severity;
            if( severity < config.minimumSeverity )
                return event;
            if( state.hitCooldownRemaining > 0.0 && severity <= state.lastSeverity * 1.20 )
                return event;

            event.accepted = true;
            state.hitCooldownRemaining = config.hitCooldownSeconds;
            state.secondsSinceImpact = 0.0;
            state.lastSeverity = severity;
            ++state.hitCount;

            const double added = std::pow( severity, 1.25 ) * config.damagePerSeverity;
            state.damage = clamp( state.damage + added, 0.0, 1.0 );
            state.permanentDamage = clamp( state.permanentDamage + added * config.permanentFraction, 0.0,
                                           config.maximumPermanentDamage );
            state.trauma = clamp( state.trauma + 0.22 + severity, 0.0, 1.5 );
            state.smokeBurst = clamp( state.smokeBurst + severity * 0.85, 0.0, 1.5 );

            double side = impact.localNormal.x;
            if( !finite( side ) || std::abs( side ) < 0.15 )
                side = random01( state.randomState ) < 0.5 ? -1.0 : 1.0;
            state.steeringPullSign = side < 0.0 ? -1.0 : 1.0;

            event.wrecked = severity >= config.wreckSeverity || state.damage >= 0.98;
            if( event.wrecked )
            {
                ++state.wreckCount;
                state.engineCutRemaining = std::max(
                    state.engineCutRemaining, config.engineCutSeconds * ( 0.55 + 0.75 * severity ) );
                state.controlLockRemaining =
                    std::max( state.controlLockRemaining,
                              config.wreckControlLockSeconds * ( 0.55 + 0.75 * severity ) );
            }
            event.damageAdded = added;
            event.recommendedSpeedScale =
                clamp( 1.0 - severity * ( event.wrecked ? 0.68 : 0.38 ), 0.12, 1.0 );
            event.recommendedYawImpulseRadPerSec =
                state.steeringPullSign * severity * ( event.wrecked ? 2.2 : 0.65 );
            return event;
        }

        bool WPVehicleDamage::update( const VehicleDamageConfig &config, double dt,
                                      VehicleDamageState &state )
        {
            if( !validate( config ).isValid() || !validateState( state ).isValid() || !finite( dt ) ||
                dt <= 0.0 || dt > 0.25 )
                return false;
            state.hitCooldownRemaining = std::max( 0.0, state.hitCooldownRemaining - dt );
            state.engineCutRemaining = std::max( 0.0, state.engineCutRemaining - dt );
            state.controlLockRemaining = std::max( 0.0, state.controlLockRemaining - dt );
            state.secondsSinceImpact += dt;
            state.trauma = std::max( 0.0, state.trauma - 1.45 * dt );
            state.smokeBurst = std::max( 0.0, state.smokeBurst - 0.72 * dt );
            if( state.secondsSinceImpact > config.repairDelaySeconds &&
                state.damage > state.permanentDamage )
                state.damage =
                    std::max( state.permanentDamage, state.damage - config.repairRatePerSecond * dt );
            return validateState( state ).isValid();
        }

        VehicleDamageTelemetry WPVehicleDamage::telemetry( const VehicleDamageConfig &config,
                                                           const VehicleDamageState &state )
        {
            VehicleDamageTelemetry out;
            if( !validate( config ).isValid() || !validateState( state ).isValid() )
                return out;
            out.damage = state.damage;
            out.permanentDamage = state.permanentDamage;
            out.torqueScale = 1.0 - state.damage * config.torquePenaltyAtFullDamage;
            out.gripScale = 1.0 - state.damage * config.gripPenaltyAtFullDamage;
            out.steeringBias = state.steeringPullSign * state.damage * config.steeringPullAtFullDamage;
            out.trauma = clamp( state.trauma, 0.0, 1.0 );
            out.smoke = clamp( state.damage * 0.65 + state.smokeBurst, 0.0, 1.0 );
            out.engineCut = state.engineCutRemaining > 0.0;
            out.controlsLocked = state.controlLockRemaining > 0.0;
            out.wrecked = out.controlsLocked && state.damage > 0.5;
            return out;
        }

        VehicleDynamicsModifiers WPVehicleDamage::dynamicsModifiers(
            const VehicleDamageTelemetry &damage )
        {
            VehicleDynamicsModifiers modifiers;
            if( !finite( damage.torqueScale ) || !finite( damage.gripScale ) ||
                !finite( damage.steeringBias ) )
            {
                modifiers.driveTorqueScale = 0.0;
                modifiers.tireGripScale = 0.05;
                modifiers.controlsLocked = true;
                return modifiers;
            }
            modifiers.driveTorqueScale = damage.engineCut ? 0.0 : clamp( damage.torqueScale, 0.0, 1.0 );
            modifiers.tireGripScale = clamp( damage.gripScale, 0.05, 1.0 );
            modifiers.steeringBias = clamp( damage.steeringBias, -0.5, 0.5 );
            modifiers.controlsLocked = damage.controlsLocked;
            return modifiers;
        }

        bool WPVehicleDamage::applyImpactResponse( const VehicleDamageEvent &event,
                                                   VehicleDynamicsState &dynamics )
        {
            if( !event.accepted || !finite( event.recommendedSpeedScale ) ||
                !finite( event.recommendedYawImpulseRadPerSec ) || event.recommendedSpeedScale < 0.0 ||
                event.recommendedSpeedScale > 1.0 )
                return false;
            const VehiclePhysicsVector3 velocity = {
                dynamics.linearVelocity.x * event.recommendedSpeedScale,
                dynamics.linearVelocity.y * event.recommendedSpeedScale,
                dynamics.linearVelocity.z * event.recommendedSpeedScale
            };
            const double yawRate = dynamics.yawRateRadPerSec + event.recommendedYawImpulseRadPerSec;
            if( !finite( velocity.x ) || !finite( velocity.y ) || !finite( velocity.z ) ||
                !finite( yawRate ) )
                return false;
            dynamics.linearVelocity = velocity;
            dynamics.yawRateRadPerSec = yawRate;
            return true;
        }
    }  // namespace procedural
}  // namespace workphone
