#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CVehicleDamage.hpp>
#include <WPProcedural/WPVehicleDamage.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, CVehicleDamage, IVehicleDamage );

    VehicleDamageState CVehicleDamage::reset( std::uint64_t seed )
    {
        return WPVehicleDamage::reset( seed );
    }

    VehicleDamageEvent CVehicleDamage::registerImpact( const VehicleDamageConfig &config,
                                                       const VehiclePhysicsConfig &physics,
                                                       const VehicleDamageImpact &impact,
                                                       VehicleDamageState &state )
    {
        return WPVehicleDamage::registerImpact( config, physics, impact, state );
    }

    bool CVehicleDamage::update( const VehicleDamageConfig &config, double fixedDeltaSeconds,
                                 VehicleDamageState &state )
    {
        return WPVehicleDamage::update( config, fixedDeltaSeconds, state );
    }

    VehicleDamageTelemetry CVehicleDamage::telemetry( const VehicleDamageConfig &config,
                                                      const VehicleDamageState &state )
    {
        return WPVehicleDamage::telemetry( config, state );
    }

    VehicleDynamicsModifiers CVehicleDamage::dynamicsModifiers( const VehicleDamageTelemetry &telemetry )
    {
        return WPVehicleDamage::dynamicsModifiers( telemetry );
    }

    bool CVehicleDamage::applyImpactResponse( const VehicleDamageEvent &event,
                                              VehicleDynamicsState &dynamics )
    {
        return WPVehicleDamage::applyImpactResponse( event, dynamics );
    }

    VehicleDamageValidation CVehicleDamage::validate( const VehicleDamageConfig &config )
    {
        return WPVehicleDamage::validate( config );
    }

    VehicleDamageValidation CVehicleDamage::validateState( const VehicleDamageState &state )
    {
        return WPVehicleDamage::validateState( state );
    }
}  // namespace workphone::procedural
