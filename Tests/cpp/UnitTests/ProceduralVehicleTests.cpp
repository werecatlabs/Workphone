#include "UnitTests.hpp"

#include <WPProcedural/WPVehicleGenerator.hpp>
#include <WPProcedural/WPVehicleDamage.hpp>
#include <WPProcedural/WPVehicleDynamics.hpp>
#include <WPProcedural/WPVehicleEffects.hpp>
#include <WPProcedural/WPVehiclePresentation.hpp>
#include <boost/test/unit_test.hpp>

#include <cmath>

using namespace workphone::procedural;

BOOST_AUTO_TEST_SUITE( procedural_vehicle )

BOOST_AUTO_TEST_CASE( grand_prix_asset_is_deterministic_and_cross_domain_valid )
{
    VehicleGenerationConfig request;
    request.seed = 0xA93F142Bu;
    request.appearance.quality = VehicleAppearanceQuality::Preview;

    const GeneratedVehicle first = WPVehicleGenerator::generate( request );
    const GeneratedVehicle second = WPVehicleGenerator::generate( request );

    BOOST_REQUIRE_MESSAGE( first.isValid(), "default generated vehicle failed validation" );
    BOOST_CHECK_EQUAL( first.contentHash, second.contentHash );
    BOOST_CHECK_EQUAL( first.appearance.contentHash, second.appearance.contentHash );
    BOOST_CHECK_EQUAL( first.geometry.lods.size(), 3u );
    BOOST_CHECK_CLOSE( first.physics.wheelbaseM, 3.60, 0.01 );
    BOOST_CHECK_SMALL(
        static_cast<double>( first.geometry.frontAxle.z ) - first.physics.wheels[0].hubPosition.z,
        0.001 );
    BOOST_CHECK_SMALL( static_cast<double>( first.geometry.centreOfMass.y ) -
                           first.physics.massProperties.centreOfMass.y,
                       0.001 );
}

BOOST_AUTO_TEST_CASE( invalid_appearance_request_is_not_hidden_by_builder_clamping )
{
    VehicleGenerationConfig request;
    request.appearance.quality = VehicleAppearanceQuality::Preview;
    request.appearance.wear = 2.0f;

    const GeneratedVehicle vehicle = WPVehicleGenerator::generate( request );
    BOOST_CHECK( !vehicle.isValid() );
    BOOST_CHECK( !vehicle.issues.empty() );
}

BOOST_AUTO_TEST_CASE( declared_bounds_contain_every_generated_lod_vertex )
{
    const VehicleGeometry geometry = WPVehicleGeometry::generate();
    BOOST_REQUIRE( geometry.hasGeometry() );
    for( const auto &lod : geometry.lods )
        for( const auto &section : lod.sections )
            for( const auto &vertex : section.vertices )
            {
                BOOST_CHECK_GE( vertex.position.x, geometry.boundsMin.x );
                BOOST_CHECK_GE( vertex.position.y, geometry.boundsMin.y );
                BOOST_CHECK_GE( vertex.position.z, geometry.boundsMin.z );
                BOOST_CHECK_LE( vertex.position.x, geometry.boundsMax.x );
                BOOST_CHECK_LE( vertex.position.y, geometry.boundsMax.y );
                BOOST_CHECK_LE( vertex.position.z, geometry.boundsMax.z );
            }
}

BOOST_AUTO_TEST_CASE( unsupported_closed_body_presets_fail_explicitly )
{
    VehicleGenerationConfig request;
    request.appearance.quality = VehicleAppearanceQuality::Preview;
    request.physicsPreset = VehiclePhysicsPreset::GT;

    const GeneratedVehicle vehicle = WPVehicleGenerator::generate( request );
    BOOST_CHECK( !vehicle.isValid() );
}

BOOST_AUTO_TEST_CASE( runtime_dynamics_accelerates_brakes_and_replays_deterministically )
{
    const VehiclePhysicsConfig physics =
        WPVehiclePhysics::generate( VehiclePhysicsPreset::GrandPrix, 42 );
    VehicleDynamicsTuning tuning;
    std::array<VehicleSurfaceSample, 4> surfaces;
    VehicleControlInput input;
    input.throttle = 0.72;

    VehicleDynamicsState first = WPVehicleDynamics::reset( physics );
    VehicleDynamicsState second = first;
    VehicleDynamicsTelemetry firstTelemetry;
    VehicleDynamicsTelemetry secondTelemetry;
    for( int i = 0; i < 480; ++i )
    {
        BOOST_REQUIRE( WPVehicleDynamics::stepFixed( physics, tuning, input, surfaces, 1.0 / 120.0,
                                                     first, firstTelemetry ) );
        BOOST_REQUIRE( WPVehicleDynamics::stepFixed( physics, tuning, input, surfaces, 1.0 / 120.0,
                                                     second, secondTelemetry ) );
    }
    BOOST_CHECK_GT( firstTelemetry.speedMps, 8.0 );
    BOOST_CHECK_LT( first.position.z, -1.0 );
    BOOST_CHECK_EQUAL( first.position.x, second.position.x );
    BOOST_CHECK_EQUAL( first.position.z, second.position.z );
    BOOST_CHECK_EQUAL( first.engineRpm, second.engineRpm );

    const double speedBeforeBraking = firstTelemetry.speedMps;
    input.throttle = 0.0;
    input.brake = 1.0;
    for( int i = 0; i < 180; ++i )
        BOOST_REQUIRE( WPVehicleDynamics::stepFixed( physics, tuning, input, surfaces, 1.0 / 120.0,
                                                     first, firstTelemetry ) );
    BOOST_CHECK_LT( firstTelemetry.speedMps, speedBeforeBraking * 0.65 );
}

BOOST_AUTO_TEST_CASE( runtime_dynamics_produces_yaw_and_combined_slip_telemetry )
{
    const VehiclePhysicsConfig physics =
        WPVehiclePhysics::generate( VehiclePhysicsPreset::RoadSport, 7 );
    VehicleDynamicsState state = WPVehicleDynamics::reset( physics );
    state.linearVelocity.z = -25.0;
    for( std::size_t i = 0; i < 4; ++i )
        state.wheels[i].angularVelocityRadPerSec = 25.0 / physics.wheels[i].tire.radiusM;

    VehicleDynamicsTuning tuning;
    VehicleControlInput input;
    input.steering = 0.35;
    input.throttle = 0.2;
    std::array<VehicleSurfaceSample, 4> surfaces;
    VehicleDynamicsTelemetry telemetry;
    for( int i = 0; i < 120; ++i )
        BOOST_REQUIRE( WPVehicleDynamics::stepFixed( physics, tuning, input, surfaces, 1.0 / 120.0,
                                                     state, telemetry ) );

    BOOST_CHECK_GT( state.yawRad, 0.01 );
    BOOST_CHECK_GT( std::abs( telemetry.lateralAccelerationMps2 ), 0.1 );
    BOOST_CHECK_GE( telemetry.wheels[0].combinedSlip, 0.0 );
    BOOST_CHECK_LE( telemetry.wheels[0].combinedSlip, 2.0 );
}

BOOST_AUTO_TEST_CASE( damage_is_seeded_bounded_and_separates_gameplay_multipliers )
{
    const VehiclePhysicsConfig physics =
        WPVehiclePhysics::generate( VehiclePhysicsPreset::GrandPrix, 9 );
    VehicleDamageConfig config;
    VehicleDamageState first = WPVehicleDamage::reset( 123 );
    VehicleDamageState second = WPVehicleDamage::reset( 123 );
    VehicleDamageImpact impact;
    impact.severity = 0.8;

    const VehicleDamageEvent a = WPVehicleDamage::registerImpact( config, physics, impact, first );
    const VehicleDamageEvent b = WPVehicleDamage::registerImpact( config, physics, impact, second );
    BOOST_REQUIRE( a.accepted );
    BOOST_CHECK( a.wrecked );
    BOOST_CHECK_EQUAL( a.recommendedYawImpulseRadPerSec, b.recommendedYawImpulseRadPerSec );
    const VehicleDamageTelemetry result = WPVehicleDamage::telemetry( config, first );
    BOOST_CHECK_LT( result.torqueScale, 1.0 );
    BOOST_CHECK_LT( result.gripScale, 1.0 );
    BOOST_CHECK( result.engineCut );
    BOOST_CHECK( result.controlsLocked );
    const VehicleDynamicsModifiers modifiers = WPVehicleDamage::dynamicsModifiers( result );
    BOOST_CHECK_EQUAL( modifiers.driveTorqueScale, 0.0 );
    BOOST_CHECK( modifiers.controlsLocked );
    VehicleDynamicsState dynamics = WPVehicleDynamics::reset( physics );
    dynamics.linearVelocity.z = -20.0;
    BOOST_REQUIRE( WPVehicleDamage::applyImpactResponse( a, dynamics ) );
    BOOST_CHECK_LT( std::abs( dynamics.linearVelocity.z ), 20.0 );
    BOOST_CHECK_NE( dynamics.yawRateRadPerSec, 0.0 );
}

BOOST_AUTO_TEST_CASE( presentation_and_effect_events_are_renderer_independent_and_bounded )
{
    const VehiclePhysicsConfig physics =
        WPVehiclePhysics::generate( VehiclePhysicsPreset::GrandPrix, 55 );
    VehicleDynamicsState dynamics = WPVehicleDynamics::reset( physics );
    VehicleDynamicsTelemetry telemetry;
    dynamics.linearVelocity.z = -35.0;
    telemetry.speedMps = 35.0;
    telemetry.brakeLight = 1.0;
    telemetry.longitudinalAccelerationMps2 = -8.0;
    for( std::size_t i = 0; i < 4; ++i )
    {
        dynamics.wheels[i].angularVelocityRadPerSec = 150.0;
        telemetry.wheels[i].grounded = true;
        telemetry.wheels[i].normalLoadN = 2200.0;
        telemetry.wheels[i].longitudinalSlipRatio = 0.7;
        telemetry.wheels[i].combinedSlip = 0.8;
    }

    VehiclePresentationState presentation = WPVehiclePresentation::reset( physics );
    VehiclePresentationInput presentationInput;
    presentationInput.throttle = 1.0;
    presentationInput.backfire = 0.8;
    VehicleDamageTelemetry damage;
    damage.smoke = 0.7;
    BOOST_REQUIRE( WPVehiclePresentation::update( {}, physics, dynamics, telemetry, damage,
                                                  presentationInput, 1.0 / 60.0, presentation ) );
    BOOST_CHECK_GT( presentation.brakeLightIntensity, 0.0 );
    BOOST_CHECK_GT( presentation.wheels[0].rotationalBlur, 0.0 );

    VehicleEffectConfig effectConfig;
    effectConfig.maximumEventsPerStep = 12;
    VehicleEffectsState effectState = WPVehicleEffects::reset( 88 );
    VehicleEffectEnvironment environment;
    environment.wetness = 0.6;
    std::vector<VehicleEffectEvent> events;
    for( int i = 0; i < 20; ++i )
    {
        const std::size_t before = events.size();
        dynamics.position.z -= 35.0 / 60.0;
        BOOST_REQUIRE( WPVehicleEffects::emit( effectConfig, physics, dynamics, telemetry, presentation,
                                               damage, environment, 1.0 / 60.0, effectState, events ) );
        BOOST_CHECK_LE( events.size() - before, effectConfig.maximumEventsPerStep );
    }
    BOOST_CHECK( !events.empty() );
}

BOOST_AUTO_TEST_SUITE_END()
