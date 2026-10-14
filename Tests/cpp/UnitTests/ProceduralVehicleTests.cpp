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

BOOST_AUTO_TEST_CASE( gt_body_is_valid_deterministic_and_has_an_enclosed_cabin )
{
    VehicleGenerationConfig request;
    request.appearance.quality = VehicleAppearanceQuality::Preview;
    request.physicsPreset = VehiclePhysicsPreset::GT;

    const GeneratedVehicle vehicle = WPVehicleGenerator::generate( request );
    BOOST_REQUIRE( vehicle.isValid() );
    BOOST_CHECK_EQUAL( vehicle.contentHash, WPVehicleGenerator::generate( request ).contentHash );
    BOOST_CHECK_EQUAL( vehicle.geometry.lods.size(), 3u );
    BOOST_CHECK_CLOSE( vehicle.physics.wheelbaseM, 2.78, .01 );
    for( const auto &lod : vehicle.geometry.lods )
    {
        bool cabin = false, lights = false;
        for( const auto &section : lod.sections )
        {
            cabin |= section.name == "gt_cabin_glass" && section.hasGeometry();
            lights |= section.material == VehicleMaterial::Headlight && section.hasGeometry();
        }
        BOOST_CHECK( cabin );
        BOOST_CHECK( lights );
    }
}

BOOST_AUTO_TEST_CASE( gt_preview_lod_has_valid_bounds_without_fine_details )
{
    VehicleGenerationConfig request;
    request.physicsPreset = VehiclePhysicsPreset::GT;
    request.appearance.quality = VehicleAppearanceQuality::Preview;
    request.geometry.generateThreeLODs = false;
    request.geometry.includeFineDetails = false;
    const auto vehicle = WPVehicleGenerator::generate( request );
    for( const auto &issue : vehicle.issues )
        BOOST_CHECK_MESSAGE( !issue.error, issue.component + ": " + issue.message );
    BOOST_REQUIRE( vehicle.isValid() );
    BOOST_REQUIRE_EQUAL( vehicle.geometry.lods.size(), 1u );
    for( const auto &section : vehicle.geometry.lods.front().sections )
        for( const auto &vertex : section.vertices )
        {
            BOOST_CHECK_GE( vertex.position.x, vehicle.geometry.boundsMin.x );
            BOOST_CHECK_LE( vertex.position.x, vehicle.geometry.boundsMax.x );
            BOOST_CHECK_GE( vertex.position.y, vehicle.geometry.boundsMin.y );
            BOOST_CHECK_LE( vertex.position.y, vehicle.geometry.boundsMax.y );
            BOOST_CHECK_GE( vertex.position.z, vehicle.geometry.boundsMin.z );
            BOOST_CHECK_LE( vertex.position.z, vehicle.geometry.boundsMax.z );
        }
}

BOOST_AUTO_TEST_CASE( road_sports_car_has_a_lower_cabin_and_no_racing_wing )
{
    VehicleGenerationConfig request;
    request.appearance.quality = VehicleAppearanceQuality::Preview;
    request.physicsPreset = VehiclePhysicsPreset::GT;
    const auto gt = WPVehicleGenerator::generate( request );
    request.physicsPreset = VehiclePhysicsPreset::RoadSport;
    for( bool details : { false, true } )
    {
        request.geometry.includeFineDetails = details;
        request.geometry.generateThreeLODs = details;
        const auto sport = WPVehicleGenerator::generate( request );
        for( const auto &issue : sport.issues )
            BOOST_CHECK_MESSAGE( !issue.error, issue.component + ": " + issue.message );
        BOOST_REQUIRE( sport.isValid() );
        BOOST_CHECK_EQUAL( sport.contentHash, WPVehicleGenerator::generate( request ).contentHash );
        BOOST_CHECK_CLOSE( sport.physics.wheelbaseM, 2.64, .01 );
        BOOST_CHECK_LT( sport.geometry.boundsMax.y, gt.geometry.boundsMax.y );
        BOOST_CHECK_EQUAL( sport.geometry.lods.size(), details ? 3u : 1u );
        for( const auto &lod : sport.geometry.lods )
        {
            bool cabin = false;
            for( const auto &section : lod.sections )
                cabin |= section.name == "sports_cabin_glass" && section.hasGeometry();
            BOOST_CHECK( cabin );
        }
    }
}

BOOST_AUTO_TEST_CASE( little_bird_helicopter_has_valid_lods_rotors_and_no_car_running_gear )
{
    VehicleGenerationConfig request;
    request.physicsPreset = VehiclePhysicsPreset::HelicopterLittleBird;
    request.appearance.quality = VehicleAppearanceQuality::Preview;
    for( bool details : { false, true } )
    {
        request.geometry.includeFineDetails = details;
        request.geometry.generateThreeLODs = details;
        const auto vehicle = WPVehicleGenerator::generate( request );
        for( const auto &issue : vehicle.issues )
            BOOST_CHECK_MESSAGE( !issue.error, issue.component + ": " + issue.message );
        BOOST_REQUIRE( vehicle.isValid() );
        BOOST_CHECK_EQUAL( vehicle.contentHash, WPVehicleGenerator::generate( request ).contentHash );
        BOOST_CHECK_EQUAL( vehicle.geometry.lods.size(), details ? 3u : 1u );
        BOOST_CHECK_GT( vehicle.geometry.boundsMax.x - vehicle.geometry.boundsMin.x, 7.0f );
        for( const auto &lod : vehicle.geometry.lods )
        {
            bool bubble = false, mainRotor = false, tailRotor = false, skids = false;
            for( const auto &section : lod.sections )
            {
                bubble |= section.name == "helicopter_bubble_cockpit" && section.hasGeometry();
                mainRotor |= section.name == "helicopter_main_rotor" && section.hasGeometry();
                tailRotor |= section.name == "helicopter_tail_rotor" && section.hasGeometry();
                skids |= section.name == "helicopter_skids" && section.hasGeometry();
                BOOST_CHECK( section.material != VehicleMaterial::Tyre );
                BOOST_CHECK( section.material != VehicleMaterial::Rim );
            }
            BOOST_CHECK( bubble && mainRotor && tailRotor && skids );
        }
        auto state = WPVehicleDynamics::reset( vehicle.physics );
        VehicleDynamicsTelemetry telemetry;
        BOOST_CHECK( !WPVehicleDynamics::stepFixed( vehicle.physics, {}, {}, {}, 1.0 / 120, state, telemetry ) );
        BOOST_CHECK_EQUAL( WPVehiclePhysics::staticWheelLoadN( vehicle.physics, WheelCorner::FrontLeft ), 0.0 );
    }
    request.geometry.mainRotorBlades = 6;
    request.geometry.tailRotorBlades = 4;
    const auto variant = WPVehicleGenerator::generate( request );
    BOOST_REQUIRE( variant.isValid() );
    for( const auto &section : variant.geometry.lods.front().sections )
    {
        if( section.name == "helicopter_main_rotor" ) BOOST_CHECK_EQUAL( section.vertices.size(), 6u * 24u );
        if( section.name == "helicopter_tail_rotor" ) BOOST_CHECK_EQUAL( section.vertices.size(), 4u * 24u );
    }
}

BOOST_AUTO_TEST_CASE( warbird_preset_has_valid_wings_canopy_propeller_and_retractable_gear )
{
    VehicleGenerationConfig request;
    request.physicsPreset = VehiclePhysicsPreset::Warbird;
    request.appearance.quality = VehicleAppearanceQuality::Preview;
    for( bool details : { false, true } )
        for( bool gearDown : { false, true } )
        {
            request.geometry.includeFineDetails = details;
            request.geometry.generateThreeLODs = details;
            request.geometry.landingGearDown = gearDown;
            const auto plane = WPVehicleGenerator::generate( request );
            for( const auto &issue : plane.issues )
                BOOST_CHECK_MESSAGE( !issue.error, issue.component + ": " + issue.message );
            BOOST_REQUIRE( plane.isValid() );
            BOOST_CHECK_EQUAL( plane.contentHash, WPVehicleGenerator::generate( request ).contentHash );
            BOOST_CHECK_EQUAL( plane.geometry.lods.size(), details ? 3u : 1u );
            BOOST_CHECK_GT( plane.geometry.boundsMax.x - plane.geometry.boundsMin.x, 11.0f );
            for( const auto &lod : plane.geometry.lods )
            {
                bool wings = false, canopy = false, propeller = false, gear = false;
                for( const auto &section : lod.sections )
                {
                    wings |= section.name == "warbird_wings" && section.hasGeometry();
                    canopy |= section.name == "warbird_canopy" && section.hasGeometry();
                    propeller |= section.name == "warbird_propeller" && section.vertices.size() == 4u * 24u;
                    gear |= section.name == "warbird_landing_gear" && section.hasGeometry();
                }
                BOOST_CHECK( wings && canopy && propeller );
                BOOST_CHECK_EQUAL( gear, gearDown );
            }
            auto state = WPVehicleDynamics::reset( plane.physics );
            VehicleDynamicsTelemetry telemetry;
            BOOST_CHECK( !WPVehicleDynamics::stepFixed( plane.physics, {}, {}, {}, 1.0 / 120, state, telemetry ) );
        }
    request.geometry.propellerBlades = 3;
    const auto variant = WPVehicleGenerator::generate( request );
    BOOST_REQUIRE( variant.isValid() );
    for( const auto &section : variant.geometry.lods.front().sections )
        if( section.name == "warbird_propeller" ) BOOST_CHECK_EQUAL( section.vertices.size(), 72u );
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
