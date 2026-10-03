#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <WPVehiclePhysics/WheelControllerArcade.hpp>
#include <Workphone/Scene/Components/WheelController.hpp>
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    SmartPtr<WheelController> makeWheelController()
    {
        auto controller = make_ptr<WheelController>();
        BOOST_REQUIRE( controller );
        return controller;
    }

    void checkProperty( const SmartPtr<Properties> &properties, const String &name, f32 expected )
    {
        f32 value = 0.0f;
        BOOST_REQUIRE( properties->hasProperty( name ) );
        BOOST_REQUIRE( properties->getPropertyValue( name, value ) );
        BOOST_CHECK_CLOSE( value, expected, 0.001f );
    }

    void checkProperty( const SmartPtr<Properties> &properties, const String &name, bool expected )
    {
        bool value = false;
        BOOST_REQUIRE( properties->hasProperty( name ) );
        BOOST_REQUIRE( properties->getPropertyValue( name, value ) );
        BOOST_CHECK_EQUAL( value, expected );
    }
}  // namespace

BOOST_AUTO_TEST_CASE( wheel_controller_default_state )
{
    auto controller = makeWheelController();

    BOOST_CHECK( !controller->isLoaded() );
    BOOST_CHECK( !controller->getWheelController() );
    BOOST_CHECK_CLOSE( controller->getMassFraction(), 0.05f, 0.001f );
    BOOST_CHECK_CLOSE( controller->getRadius(), 0.5f, 0.001f );
    BOOST_CHECK_CLOSE( controller->getWheelDamping(), 1.0f, 0.001f );
    BOOST_CHECK_CLOSE( controller->getSuspensionDistance(), 0.3f, 0.001f );
    BOOST_CHECK( controller->getTireModel() == TireModel::Simple );
}

BOOST_AUTO_TEST_CASE( wheel_controller_public_setters_round_trip )
{
    auto controller = makeWheelController();

    controller->setMassFraction( 0.25f );
    controller->setRadius( 0.42f );
    controller->setWheelDamping( 125.0f );
    controller->setSuspensionDistance( 0.18f );
    controller->setTireModel( TireModel::Brush );

    BOOST_CHECK_CLOSE( controller->getMassFraction(), 0.25f, 0.001f );
    BOOST_CHECK_CLOSE( controller->getRadius(), 0.42f, 0.001f );
    BOOST_CHECK_CLOSE( controller->getWheelDamping(), 125.0f, 0.001f );
    BOOST_CHECK_CLOSE( controller->getSuspensionDistance(), 0.18f, 0.001f );
    BOOST_CHECK( controller->getTireModel() == TireModel::Brush );
}

BOOST_AUTO_TEST_CASE( wheel_controller_serializes_complete_property_set )
{
    auto controller = makeWheelController();
    auto properties = controller->getProperties();
    BOOST_REQUIRE( properties );

    checkProperty( properties, WheelController::massFractionStr, 0.05f );
    checkProperty( properties, WheelController::radiusStr, 0.5f );
    checkProperty( properties, WheelController::wheelDampingStr, 1.0f );
    checkProperty( properties, WheelController::suspensionDistanceStr, 0.3f );
    checkProperty( properties, WheelController::springRateStr, 1.0f );
    checkProperty( properties, WheelController::suspensionDamperStr, 1.0f );
    checkProperty( properties, WheelController::targetPositionStr, 0.0f );
    checkProperty( properties, WheelController::forwardExtremumSlipStr, 10.0f );
    checkProperty( properties, WheelController::forwardExtrememValueStr, 10.0f );
    checkProperty( properties, WheelController::forwardAsymptoteSlipStr, 10.0f );
    checkProperty( properties, WheelController::forwardAsymptoteValueStr, 10.0f );
    checkProperty( properties, WheelController::forwardStiffnessStr, 10.0f );
    checkProperty( properties, WheelController::sidewaysExtremumSlipStr, 1.0f );
    checkProperty( properties, WheelController::sidewaysExtrememValueStr, 1.0f );
    checkProperty( properties, WheelController::sidewaysAsymptoteSlipStr, 1.0f );
    checkProperty( properties, WheelController::sidewaysAsymptoteValueStr, 1.0f );
    checkProperty( properties, WheelController::sidewaysStiffnessStr, 1.0f );
    checkProperty( properties, WheelController::isSteeringWheelStr, false );

    s32 tireModel = -1;
    BOOST_REQUIRE( properties->getPropertyValue( WheelController::tireModelStr, tireModel ) );
    BOOST_CHECK_EQUAL( tireModel, static_cast<s32>( TireModel::Simple ) );
    BOOST_CHECK( !properties->isButtonPressed( WheelController::resetStr ) );
}

BOOST_AUTO_TEST_CASE( wheel_controller_properties_round_trip )
{
    auto source = makeWheelController();
    auto properties = source->getProperties();
    BOOST_REQUIRE( properties );

    properties->setProperty( WheelController::massFractionStr, 0.2f );
    properties->setProperty( WheelController::radiusStr, 0.35f );
    properties->setProperty( WheelController::wheelDampingStr, 800.0f );
    properties->setProperty( WheelController::suspensionDistanceStr, 0.12f );
    properties->setProperty( WheelController::springRateStr, 4.0f );
    properties->setProperty( WheelController::suspensionDamperStr, 7.0f );
    properties->setProperty( WheelController::targetPositionStr, -0.1f );
    properties->setProperty( WheelController::forwardExtremumSlipStr, 2.0f );
    properties->setProperty( WheelController::forwardExtrememValueStr, 3.0f );
    properties->setProperty( WheelController::forwardAsymptoteSlipStr, 4.0f );
    properties->setProperty( WheelController::forwardAsymptoteValueStr, 5.0f );
    properties->setProperty( WheelController::forwardStiffnessStr, 6.0f );
    properties->setProperty( WheelController::sidewaysExtremumSlipStr, 0.7f );
    properties->setProperty( WheelController::sidewaysExtrememValueStr, 0.8f );
    properties->setProperty( WheelController::sidewaysAsymptoteSlipStr, 0.9f );
    properties->setProperty( WheelController::sidewaysAsymptoteValueStr, 1.1f );
    properties->setProperty( WheelController::sidewaysStiffnessStr, 1.2f );
    properties->setProperty( WheelController::isSteeringWheelStr, true );
    properties->setProperty( WheelController::tireModelStr, static_cast<s32>( TireModel::Pacejka ) );

    auto restored = makeWheelController();
    restored->setProperties( properties );

    auto restoredProperties = restored->getProperties();
    BOOST_REQUIRE( restoredProperties );
    checkProperty( restoredProperties, WheelController::massFractionStr, 0.2f );
    checkProperty( restoredProperties, WheelController::radiusStr, 0.35f );
    checkProperty( restoredProperties, WheelController::wheelDampingStr, 800.0f );
    checkProperty( restoredProperties, WheelController::suspensionDistanceStr, 0.12f );
    checkProperty( restoredProperties, WheelController::springRateStr, 4.0f );
    checkProperty( restoredProperties, WheelController::suspensionDamperStr, 7.0f );
    checkProperty( restoredProperties, WheelController::targetPositionStr, -0.1f );
    checkProperty( restoredProperties, WheelController::forwardStiffnessStr, 6.0f );
    checkProperty( restoredProperties, WheelController::sidewaysStiffnessStr, 1.2f );
    checkProperty( restoredProperties, WheelController::isSteeringWheelStr, true );
    BOOST_CHECK( restored->getTireModel() == TireModel::Pacejka );
}

BOOST_AUTO_TEST_CASE( wheel_controller_reset_restores_runtime_defaults )
{
    auto controller = makeWheelController();
    auto properties = controller->getProperties();
    BOOST_REQUIRE( properties );
    properties->setProperty( WheelController::massFractionStr, 0.3f );
    properties->setProperty( WheelController::radiusStr, 1.2f );
    properties->setProperty( WheelController::wheelDampingStr, 55.0f );
    properties->setProperty( WheelController::suspensionDistanceStr, 0.8f );
    controller->setProperties( properties );

    controller->reset();

    BOOST_CHECK_CLOSE( controller->getMassFraction(), 0.05f, 0.001f );
    BOOST_CHECK_CLOSE( controller->getRadius(), 0.29f, 0.001f );
    BOOST_CHECK_CLOSE( controller->getWheelDamping(), 9000.0f, 0.001f );
    BOOST_CHECK_CLOSE( controller->getSuspensionDistance(), 0.1f, 0.001f );
    auto resetProperties = controller->getProperties();
    BOOST_REQUIRE( resetProperties );
    checkProperty( resetProperties, WheelController::springRateStr, 1.0f );
    checkProperty( resetProperties, WheelController::forwardStiffnessStr, 10.0f );
    checkProperty( resetProperties, WheelController::sidewaysStiffnessStr, 1.0f );
}

BOOST_AUTO_TEST_CASE( wheel_controller_reset_property_applies_editor_defaults )
{
    auto controller = makeWheelController();
    auto properties = controller->getProperties();
    BOOST_REQUIRE( properties );
    properties->setProperty( WheelController::radiusStr, 1.2f );
    properties->setProperty( WheelController::wheelDampingStr, 55.0f );
    properties->setProperty( WheelController::suspensionDistanceStr, 0.8f );
    properties->setButtonPressed( WheelController::resetStr, true );

    controller->setProperties( properties );

    BOOST_CHECK_CLOSE( controller->getRadius(), 0.29f, 0.001f );
    BOOST_CHECK_CLOSE( controller->getWheelDamping(), 12000.0f, 0.001f );
    BOOST_CHECK_CLOSE( controller->getSuspensionDistance(), 0.1f, 0.001f );
}

BOOST_AUTO_TEST_CASE( wheel_controller_lifecycle_is_idempotent )
{
    auto controller = makeWheelController();
    BOOST_CHECK( !controller->isLoaded() );

    controller->unload( nullptr );
    BOOST_CHECK( !controller->isLoaded() );
    BOOST_CHECK( !controller->getWheelController() );
    controller->unload( nullptr );
    BOOST_CHECK( !controller->isLoaded() );
}

BOOST_AUTO_TEST_CASE( wheel_controller_underlying_controller_and_children )
{
    auto controller = makeWheelController();
    auto wheel = make_ptr<WheelControllerArcade>();
    BOOST_REQUIRE( wheel );

    controller->setWheelController( wheel );
    BOOST_CHECK( controller->getWheelController().get() == wheel.get() );
    BOOST_CHECK( controller->getTireModel() == wheel->getTireModel() );

    auto children = controller->getChildObjects();
    BOOST_REQUIRE_EQUAL( children.size(), 1u );
    BOOST_CHECK( children[0].get() == wheel.get() );

    controller->setWheelController( nullptr );
    BOOST_CHECK( !controller->getWheelController() );
    BOOST_CHECK( controller->getTireModel() == TireModel::Simple );
}

BOOST_AUTO_TEST_CASE( wheel_controller_actor_attachment )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.sceneManager );
    auto actor = guard.sceneManager->createActor();
    BOOST_REQUIRE( actor );
    auto controller = actor->addComponent<WheelController>();
    BOOST_REQUIRE( controller );
    BOOST_CHECK( actor->getComponent<WheelController>().get() == controller.get() );
    BOOST_CHECK( controller->getActor().get() == actor.get() );
    guard.sceneManager->destroyActor( actor );
}
