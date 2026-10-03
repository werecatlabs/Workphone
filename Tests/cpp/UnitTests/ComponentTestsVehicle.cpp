#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <WPVehiclePhysics/CAircraft.hpp>
#include <WPVehiclePhysics/CCarController.hpp>
#include <WPVehiclePhysics/WheelControllerBrush.hpp>
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <array>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    constexpr real_Num Tolerance = static_cast<real_Num>( 0.001 );

    void checkClose( real_Num actual, real_Num expected )
    {
        BOOST_CHECK_MESSAGE( Math<real_Num>::equals( actual, expected, Tolerance ),
                             "actual=" << actual << " expected=" << expected );
    }

    void checkVectorClose( const Vector3<real_Num> &actual, const Vector3<real_Num> &expected )
    {
        BOOST_CHECK_MESSAGE( MathUtil<real_Num>::equals( actual, expected, Tolerance ),
                             "actual=(" << actual.X() << ", " << actual.Y() << ", " << actual.Z()
                                        << ") expected=(" << expected.X() << ", " << expected.Y() << ", "
                                        << expected.Z() << ")" );
    }

    void checkQuaternionClose( const Quaternion<real_Num> &actual, const Quaternion<real_Num> &expected )
    {
        checkClose( actual.w, expected.w );
        checkClose( actual.x, expected.x );
        checkClose( actual.y, expected.y );
        checkClose( actual.z, expected.z );
    }

    template <class T>
    void checkScalarProperty( const SmartPtr<Properties> &properties, const String &name, T expected )
    {
        T actual = T();
        BOOST_REQUIRE( properties );
        BOOST_REQUIRE_MESSAGE( properties->hasProperty( name ), "Missing property: " << name );
        BOOST_REQUIRE_MESSAGE( properties->getPropertyValue( name, actual ),
                               "Could not read property: " << name );
        checkClose( static_cast<real_Num>( actual ), static_cast<real_Num>( expected ) );
    }

    void checkEnumProperty( const SmartPtr<Properties> &properties, const String &name, s32 expected )
    {
        s32 actual = -1;
        BOOST_REQUIRE( properties );
        BOOST_REQUIRE_MESSAGE( properties->hasProperty( name ), "Missing property: " << name );
        BOOST_REQUIRE_MESSAGE( properties->getPropertyValue( name, actual ),
                               "Could not read property: " << name );
        BOOST_CHECK_EQUAL( actual, expected );
    }

    void checkVectorProperty( const SmartPtr<Properties> &properties, const String &name,
                              const Vector3<real_Num> &expected )
    {
        Vector3<real_Num> actual;
        BOOST_REQUIRE( properties );
        BOOST_REQUIRE_MESSAGE( properties->hasProperty( name ), "Missing property: " << name );
        BOOST_REQUIRE_MESSAGE( properties->getPropertyValue( name, actual ),
                               "Could not read property: " << name );
        checkVectorClose( actual, expected );
    }

    bool containsObject( const Array<SmartPtr<ISharedObject>> &objects, const ISharedObject *expected )
    {
        for( const auto &object : objects )
        {
            if( object.get() == expected )
            {
                return true;
            }
        }

        return false;
    }

    struct VehicleFactoryGuard
    {
        VehicleFactoryGuard()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto factoryManager =
                applicationManager ? applicationManager->getFactoryManagerPtr() : nullptr;
            if( !factoryManager )
            {
                return;
            }

            if( !factoryManager->hasFactoryById( CCarController::typeInfo() ) )
            {
                FactoryUtil::addFactory<CCarController>();
                addedCar = true;
            }
            if( !factoryManager->hasFactoryById( vehicle::CAircraft::typeInfo() ) )
            {
                FactoryUtil::addFactory<vehicle::CAircraft>();
                addedAircraft = true;
            }
            if( !factoryManager->hasFactoryById( WheelControllerBrush::typeInfo() ) )
            {
                FactoryUtil::addFactory<WheelControllerBrush>();
                addedWheel = true;
            }
        }

        ~VehicleFactoryGuard()
        {
            if( addedWheel )
            {
                FactoryUtil::removeFactory<WheelControllerBrush>();
            }
            if( addedAircraft )
            {
                FactoryUtil::removeFactory<vehicle::CAircraft>();
            }
            if( addedCar )
            {
                FactoryUtil::removeFactory<CCarController>();
            }
        }

        bool addedCar = false;
        bool addedAircraft = false;
        bool addedWheel = false;
    };
}  // namespace

BOOST_AUTO_TEST_CASE( components_vehicle_controller_default_contract )
{
    TestGuard fixture;
    BOOST_REQUIRE( fixture.isAvailable );

    auto controller = make_ptr<VehicleController>();
    BOOST_REQUIRE( controller );

    BOOST_CHECK( !controller->isLoaded() );
    BOOST_CHECK( !controller->getChassis() );
    BOOST_CHECK( !controller->getCollision() );
    BOOST_CHECK( !controller->getAircraftController() );
    BOOST_CHECK( !controller->isAircraftPhysicsEnabled() );

    checkClose( controller->getMass(), 1370.0f );
    checkVectorClose( controller->getMOI(), Vector3<real_Num>::unit() );
    checkVectorClose( controller->getCg(), Vector3<real_Num>::zero() );
    BOOST_CHECK( controller->getTireModel() == TireModel::Simple );
    BOOST_CHECK_EQUAL( controller->getVehicleType(),
                       static_cast<s32>( VehicleController::VehicleType::Car ) );

    checkClose( controller->getAircraftThrottle(), 0.0f );
    checkClose( controller->getAircraftPitch(), 0.0f );
    checkClose( controller->getAircraftRoll(), 0.0f );
    checkClose( controller->getAircraftYaw(), 0.0f );
    checkClose( controller->getAirDensity(), static_cast<real_Num>( 1.225 ) );
    checkClose( controller->getAerodynamicSectionMultiplier(), 16.0f );
    checkClose( controller->getRollwiseDamping(), 1.0f );
}

BOOST_AUTO_TEST_CASE( components_vehicle_controller_setters_enforce_documented_bounds )
{
    TestGuard fixture;
    BOOST_REQUIRE( fixture.isAvailable );

    auto controller = make_ptr<VehicleController>();
    BOOST_REQUIRE( controller );

    controller->setMass( -25.0f );
    checkClose( controller->getMass(), 0.1f );
    controller->setMass( 250000.0f );
    checkClose( controller->getMass(), 100000.0f );
    controller->setMass( 1825.0f );
    checkClose( controller->getMass(), 1825.0f );

    const auto moi = Vector3<real_Num>( 5.0f, 6.0f, 7.0f );
    const auto cg = Vector3<real_Num>( 0.25f, -0.5f, 1.5f );
    controller->setMOI( moi );
    controller->setCg( cg );
    controller->setTireModel( TireModel::Brush );
    checkVectorClose( controller->getMOI(), moi );
    checkVectorClose( controller->getCg(), cg );
    BOOST_CHECK( controller->getTireModel() == TireModel::Brush );

    controller->setAircraftControls( 2.0f, -2.0f, 3.0f, -3.0f );
    checkClose( controller->getAircraftThrottle(), 1.0f );
    checkClose( controller->getAircraftPitch(), -1.0f );
    checkClose( controller->getAircraftRoll(), 1.0f );
    checkClose( controller->getAircraftYaw(), -1.0f );

    controller->setAirDensity( 0.0f );
    controller->setAerodynamicSectionMultiplier( 2000.0f );
    controller->setRollwiseDamping( -1.0f );
    checkClose( controller->getAirDensity(), 0.05f );
    checkClose( controller->getAerodynamicSectionMultiplier(), 1000.0f );
    checkClose( controller->getRollwiseDamping(), 0.0f );

    controller->setVehicleType( -1 );
    BOOST_CHECK_EQUAL( controller->getVehicleType(),
                       static_cast<s32>( VehicleController::VehicleType::Car ) );
    controller->setVehicleType( 99 );
    BOOST_CHECK_EQUAL( controller->getVehicleType(),
                       static_cast<s32>( VehicleController::VehicleType::Car ) );
    controller->setVehicleType( static_cast<s32>( VehicleController::VehicleType::Aircraft ) );
    BOOST_CHECK_EQUAL( controller->getVehicleType(),
                       static_cast<s32>( VehicleController::VehicleType::Aircraft ) );
    BOOST_CHECK( !controller->isAircraftPhysicsEnabled() );
}

BOOST_AUTO_TEST_CASE( components_vehicle_controller_serializes_complete_property_contract )
{
    TestGuard fixture;
    BOOST_REQUIRE( fixture.isAvailable );

    auto controller = make_ptr<VehicleController>();
    BOOST_REQUIRE( controller );
    auto properties = controller->getProperties();
    BOOST_REQUIRE( properties );

    checkVectorProperty( properties, VehicleController::cgStr, Vector3<real_Num>::zero() );
    checkScalarProperty( properties, VehicleController::massStr, 1370.0f );
    checkVectorProperty( properties, VehicleController::moiStr, Vector3<real_Num>::unit() );
    checkEnumProperty( properties, VehicleController::tireModelStr,
                       static_cast<s32>( TireModel::Simple ) );
    checkEnumProperty( properties, VehicleController::vehicleTypeStr,
                       static_cast<s32>( VehicleController::VehicleType::Car ) );
    checkScalarProperty( properties, VehicleController::airDensityStr, static_cast<real_Num>( 1.225 ) );
    checkScalarProperty( properties, VehicleController::sectionMultiplierStr,
                         static_cast<real_Num>( 16.0 ) );
    checkScalarProperty( properties, VehicleController::rollwiseDampingStr,
                         static_cast<real_Num>( 1.0 ) );

    BOOST_CHECK( properties->hasProperty( VehicleController::collisionStr ) );
    BOOST_CHECK( properties->hasProperty( VehicleController::chassisStr ) );
    BOOST_CHECK( !properties->isButtonPressed( VehicleController::resetStr ) );
    BOOST_CHECK( !properties->isButtonPressed( VehicleController::resetTransformStr ) );
}

BOOST_AUTO_TEST_CASE( components_vehicle_controller_properties_round_trip_and_clamp )
{
    TestGuard fixture;
    BOOST_REQUIRE( fixture.isAvailable );

    auto source = make_ptr<VehicleController>();
    BOOST_REQUIRE( source );
    auto properties = source->getProperties();
    BOOST_REQUIRE( properties );

    const auto cg = Vector3<real_Num>( 0.2f, -0.4f, 0.8f );
    const auto moi = Vector3<real_Num>( 1500.0f, 2200.0f, 3100.0f );
    properties->setProperty( VehicleController::cgStr, cg );
    properties->setProperty( VehicleController::massStr, -100.0f );
    properties->setProperty( VehicleController::moiStr, moi );
    properties->setProperty( VehicleController::tireModelStr, static_cast<s32>( TireModel::Pacejka ) );
    properties->setProperty( VehicleController::vehicleTypeStr,
                             static_cast<s32>( VehicleController::VehicleType::Aircraft ) );
    properties->setProperty( VehicleController::airDensityStr, static_cast<real_Num>( 10.0 ) );
    properties->setProperty( VehicleController::sectionMultiplierStr, static_cast<real_Num>( -4.0 ) );
    properties->setProperty( VehicleController::rollwiseDampingStr, static_cast<real_Num>( 250.0 ) );

    auto restored = make_ptr<VehicleController>();
    BOOST_REQUIRE( restored );
    restored->setProperties( properties );

    checkVectorClose( restored->getCg(), cg );
    checkClose( restored->getMass(), 0.1f );
    checkVectorClose( restored->getMOI(), moi );
    BOOST_CHECK( restored->getTireModel() == TireModel::Pacejka );
    BOOST_CHECK_EQUAL( restored->getVehicleType(),
                       static_cast<s32>( VehicleController::VehicleType::Aircraft ) );
    BOOST_CHECK( !restored->isAircraftPhysicsEnabled() );
    checkClose( restored->getAirDensity(), 5.0f );
    checkClose( restored->getAerodynamicSectionMultiplier(), 0.01f );
    checkClose( restored->getRollwiseDamping(), 100.0f );

    auto roundTripped = restored->getProperties();
    checkVectorProperty( roundTripped, VehicleController::cgStr, cg );
    checkScalarProperty( roundTripped, VehicleController::massStr, 0.1f );
    checkVectorProperty( roundTripped, VehicleController::moiStr, moi );
    checkEnumProperty( roundTripped, VehicleController::tireModelStr,
                       static_cast<s32>( TireModel::Pacejka ) );
}

BOOST_AUTO_TEST_CASE( components_vehicle_controller_associations_are_child_objects )
{
    TestGuard fixture;
    BOOST_REQUIRE( fixture.isAvailable );

    auto controller = make_ptr<VehicleController>();
    auto chassis = make_ptr<Rigidbody>();
    auto collision = make_ptr<CollisionBox>();
    BOOST_REQUIRE( controller );
    BOOST_REQUIRE( chassis );
    BOOST_REQUIRE( collision );

    controller->setChassis( chassis );
    controller->setCollision( collision );

    BOOST_CHECK( controller->getChassis().get() == chassis.get() );
    BOOST_CHECK( controller->getCollision().get() == collision.get() );
    checkClose( chassis->getMass(), controller->getMass() );
    checkVectorClose( chassis->getMassSpaceInertiaTensor(), controller->getMOI() );

    auto children = controller->getChildObjects();
    BOOST_REQUIRE_EQUAL( children.size(), 5u );
    BOOST_CHECK( containsObject( children, chassis.get() ) );
    BOOST_CHECK( containsObject( children, collision.get() ) );

    controller->setChassis( nullptr );
    controller->setCollision( nullptr );
    BOOST_CHECK( !controller->getChassis() );
    BOOST_CHECK( !controller->getCollision() );
}

BOOST_AUTO_TEST_CASE( components_vehicle_controller_binds_actor_dependencies_and_propagates_tires )
{
    TestGuard fixture;
    BOOST_REQUIRE( fixture.isAvailable );

    auto actor = fixture.sceneManager->createActor();
    BOOST_REQUIRE( actor );
    auto collision = actor->addComponent<CollisionBox>();
    auto chassis = actor->addComponent<Rigidbody>();
    auto controller = actor->addComponent<VehicleController>();
    BOOST_REQUIRE( collision );
    BOOST_REQUIRE( chassis );
    BOOST_REQUIRE( controller );

    BOOST_CHECK( controller->isLoaded() );
    BOOST_CHECK( controller->isValid() );
    BOOST_CHECK( controller->getActor().get() == actor.get() );
    BOOST_CHECK( actor->getComponent<VehicleController>().get() == controller.get() );
    BOOST_CHECK( controller->getCollision().get() == collision.get() );
    BOOST_CHECK( controller->getChassis().get() == chassis.get() );

    const auto moi = Vector3<real_Num>( 900.0f, 1200.0f, 1500.0f );
    controller->setMass( 1650.0f );
    controller->setMOI( moi );
    checkClose( chassis->getMass(), 1650.0f );
    checkVectorClose( chassis->getMassSpaceInertiaTensor(), moi );

    auto wheelActor = fixture.sceneManager->createActor();
    BOOST_REQUIRE( wheelActor );
    actor->addChild( wheelActor );
    auto wheel = wheelActor->addComponent<WheelController>();
    BOOST_REQUIRE( wheel );

    controller->setTireModel( TireModel::Pacejka );
    BOOST_CHECK( wheel->getTireModel() == TireModel::Pacejka );

    auto properties = controller->getProperties();
    BOOST_REQUIRE( properties );
    properties->setProperty( VehicleController::tireModelStr, static_cast<s32>( TireModel::Brush ) );
    controller->setProperties( properties );
    BOOST_CHECK( wheel->getTireModel() == TireModel::Brush );

    fixture.sceneManager->destroyActor( actor );
}

BOOST_AUTO_TEST_CASE( components_vehicle_controller_reset_actions_restore_actor_and_wheels )
{
    TestGuard fixture;
    BOOST_REQUIRE( fixture.isAvailable );

    auto actor = fixture.sceneManager->createActor();
    BOOST_REQUIRE( actor );
    auto chassis = actor->addComponent<Rigidbody>();
    auto controller = actor->addComponent<VehicleController>();
    BOOST_REQUIRE( chassis );
    BOOST_REQUIRE( controller );

    auto wheelActor = fixture.sceneManager->createActor();
    BOOST_REQUIRE( wheelActor );
    actor->addChild( wheelActor );
    auto wheel = wheelActor->addComponent<WheelController>();
    BOOST_REQUIRE( wheel );
    wheel->setMassFraction( 0.5f );
    wheel->setRadius( 1.2f );
    wheel->setWheelDamping( 25.0f );
    wheel->setSuspensionDistance( 0.8f );

    const auto resetPosition = Vector3<real_Num>( 12.0f, 3.0f, -7.0f );
    const auto resetOrientation = Quaternion<real_Num>::angleAxis(
        Math<real_Num>::pi() / static_cast<real_Num>( 3.0 ), Vector3<real_Num>::unitY() );
    const auto resetTransform =
        Transform3<real_Num>( resetPosition, resetOrientation, Vector3<real_Num>::unit() );
    controller->setResetTransform( resetTransform );

    actor->setLocalPosition( Vector3<real_Num>( -4.0f, 8.0f, 20.0f ) );
    actor->setLocalOrientation( Quaternion<real_Num>::identity() );
    chassis->setLinearVelocity( Vector3<real_Num>( 5.0f, 6.0f, 7.0f ) );
    chassis->setAngularVelocity( Vector3<real_Num>( -2.0f, 3.0f, 4.0f ) );

    auto properties = controller->getProperties();
    BOOST_REQUIRE( properties );
    properties->setButtonPressed( VehicleController::resetStr, true );
    properties->setButtonPressed( VehicleController::resetTransformStr, true );
    controller->setProperties( properties );

    checkVectorClose( actor->getLocalPosition(), resetPosition );
    checkQuaternionClose( actor->getLocalOrientation(), resetOrientation );
    checkVectorClose( chassis->getLinearVelocity(), Vector3<real_Num>::zero() );
    checkVectorClose( chassis->getAngularVelocity(), Vector3<real_Num>::zero() );
    checkClose( wheel->getMassFraction(), 0.05f );
    checkClose( wheel->getRadius(), 0.29f );
    checkClose( wheel->getWheelDamping(), 9000.0f );
    checkClose( wheel->getSuspensionDistance(), 0.1f );

    fixture.sceneManager->destroyActor( actor );
}

BOOST_AUTO_TEST_CASE( components_vehicle_controller_aircraft_lifecycle_and_configuration )
{
    TestGuard fixture;
    BOOST_REQUIRE( fixture.isAvailable );
    VehicleFactoryGuard factoryGuard;

    auto actor = fixture.sceneManager->createActor();
    BOOST_REQUIRE( actor );
    auto controller = actor->addComponent<VehicleController>();
    BOOST_REQUIRE( controller );
    BOOST_REQUIRE( controller->isLoaded() );

    controller->setMass( 2100.0f );
    controller->setAirDensity( static_cast<real_Num>( 0.9 ) );
    controller->setAerodynamicSectionMultiplier( static_cast<real_Num>( 24.0 ) );
    controller->setRollwiseDamping( static_cast<real_Num>( 2.5 ) );
    controller->setAircraftControls( 0.75f, -0.25f, 0.5f, -0.75f );
    controller->setVehicleType( static_cast<s32>( VehicleController::VehicleType::Aircraft ) );

    BOOST_CHECK( controller->isAircraftPhysicsEnabled() );
    auto aircraft = controller->getAircraftController();
    BOOST_REQUIRE( aircraft );
    BOOST_CHECK( aircraft->isLoaded() );
    checkClose( aircraft->getMass(), 2100.0f );
    checkClose( aircraft->getAirDensity(), 0.9f );
    checkClose( aircraft->getSectionMultiplier(), 24.0f );
    checkClose( aircraft->getRollwiseDamping(), 2.5f );
    checkClose( aircraft->getChannel( 0 ), 0.75f );
    checkClose( aircraft->getChannel( 1 ), 0.5f );
    checkClose( aircraft->getChannel( 2 ), -0.25f );
    checkClose( aircraft->getChannel( 3 ), -0.75f );

    auto children = controller->getChildObjects();
    BOOST_CHECK( containsObject( children, aircraft.get() ) );

    controller->setVehicleType( static_cast<s32>( VehicleController::VehicleType::Car ) );
    BOOST_CHECK( !controller->isAircraftPhysicsEnabled() );
    BOOST_CHECK( !controller->getAircraftController() );
    BOOST_CHECK( !aircraft->isLoaded() );

    controller->setVehicleType( static_cast<s32>( VehicleController::VehicleType::Aircraft ) );
    auto replacementAircraft = controller->getAircraftController();
    BOOST_REQUIRE( replacementAircraft );
    BOOST_CHECK( replacementAircraft.get() != aircraft.get() );
    controller->unload( nullptr );
    BOOST_CHECK( !controller->isLoaded() );
    BOOST_CHECK( !controller->getAircraftController() );
    BOOST_CHECK( !replacementAircraft->isLoaded() );
    controller->unload( nullptr );

    fixture.sceneManager->destroyActor( actor );
}

BOOST_AUTO_TEST_CASE( components_car_controller_default_contract_and_inputs )
{
    TestGuard fixture;
    BOOST_REQUIRE( fixture.isAvailable );

    auto controller = make_ptr<CarController>();
    BOOST_REQUIRE( controller );

    BOOST_CHECK( !controller->isLoaded() );
    BOOST_CHECK( !controller->getVehicleController() );
    BOOST_CHECK( controller->getDriveType() == VehicleDriveType::AllWheelDrive );
    checkClose( controller->getThrottle(), 0.0f );
    checkClose( controller->getBrake(), 0.0f );
    checkClose( controller->getSteering(), 0.0f );
    checkClose( controller->getMass(), 1370.0f );

    controller->setThrottle( 0.8f );
    controller->setBrake( 0.35f );
    controller->setSteering( -0.6f );
    controller->setDriveType( VehicleDriveType::RearWheelDrive );
    checkClose( controller->getThrottle(), 0.8f );
    checkClose( controller->getBrake(), 0.35f );
    checkClose( controller->getSteering(), -0.6f );
    BOOST_CHECK( controller->getDriveType() == VehicleDriveType::RearWheelDrive );
}

BOOST_AUTO_TEST_CASE( components_car_controller_properties_round_trip )
{
    TestGuard fixture;
    BOOST_REQUIRE( fixture.isAvailable );

    auto source = make_ptr<CarController>();
    BOOST_REQUIRE( source );
    auto properties = source->getProperties();
    BOOST_REQUIRE( properties );

    checkScalarProperty( properties, CarController::radiusStr, 0.35f );
    checkScalarProperty( properties, CarController::suspensionTravelStr, 0.3f );
    checkScalarProperty( properties, CarController::dampingStr, 1000.0f );
    checkEnumProperty( properties, CarController::driveTypeStr,
                       static_cast<s32>( VehicleDriveType::AllWheelDrive ) );

    properties->setProperty( CarController::radiusStr, 0.42f );
    properties->setProperty( CarController::suspensionTravelStr, 0.18f );
    properties->setProperty( CarController::dampingStr, 2400.0f );
    properties->setProperty( CarController::driveTypeStr,
                             static_cast<s32>( VehicleDriveType::FrontWheelDrive ) );
    properties->setProperty( VehicleController::massStr, 1550.0f );
    properties->setProperty( VehicleController::tireModelStr, static_cast<s32>( TireModel::Brush ) );

    auto restored = make_ptr<CarController>();
    BOOST_REQUIRE( restored );
    restored->setProperties( properties );
    auto restoredProperties = restored->getProperties();

    checkScalarProperty( restoredProperties, CarController::radiusStr, 0.42f );
    checkScalarProperty( restoredProperties, CarController::suspensionTravelStr, 0.18f );
    checkScalarProperty( restoredProperties, CarController::dampingStr, 2400.0f );
    checkEnumProperty( restoredProperties, CarController::driveTypeStr,
                       static_cast<s32>( VehicleDriveType::FrontWheelDrive ) );
    checkClose( restored->getMass(), 1550.0f );
    BOOST_CHECK( restored->getTireModel() == TireModel::Brush );
}

BOOST_AUTO_TEST_CASE( components_car_controller_vehicle_association_is_a_child_object )
{
    TestGuard fixture;
    BOOST_REQUIRE( fixture.isAvailable );

    auto controller = make_ptr<CarController>();
    auto physicsVehicle = make_ptr<CCarController>();
    BOOST_REQUIRE( controller );
    BOOST_REQUIRE( physicsVehicle );

    controller->setVehicleController( physicsVehicle );
    BOOST_CHECK( controller->getVehicleController().get() == physicsVehicle.get() );
    auto children = controller->getChildObjects();
    BOOST_REQUIRE_EQUAL( children.size(), 7u );
    BOOST_CHECK( containsObject( children, physicsVehicle.get() ) );

    controller->setVehicleController( nullptr );
    BOOST_CHECK( !controller->getVehicleController() );
}

BOOST_AUTO_TEST_CASE( components_car_controller_loads_concrete_vehicle_and_releases_resources )
{
    TestGuard fixture;
    BOOST_REQUIRE( fixture.isAvailable );
    VehicleFactoryGuard factoryGuard;

    auto actor = fixture.sceneManager->createActor();
    BOOST_REQUIRE( actor );
    auto collision = actor->addComponent<CollisionBox>();
    auto chassis = actor->addComponent<Rigidbody>();
    auto controller = actor->addComponent<CarController>();
    BOOST_REQUIRE( collision );
    BOOST_REQUIRE( chassis );
    BOOST_REQUIRE( controller );

    BOOST_CHECK( controller->isLoaded() );
    BOOST_CHECK( controller->isValid() );
    BOOST_CHECK( controller->getActor().get() == actor.get() );
    BOOST_CHECK( controller->getCollision().get() == collision.get() );
    BOOST_CHECK( controller->getChassis().get() == chassis.get() );

    auto physicsVehicle = controller->getVehicleController();
    BOOST_REQUIRE( physicsVehicle );
    BOOST_CHECK( physicsVehicle->isLoaded() );
    BOOST_CHECK( dynamic_pointer_cast<CCarController>( physicsVehicle ) );
    checkClose( physicsVehicle->getMass(), 1370.0f );
    checkClose( chassis->getMass(), 1370.0f );
    checkVectorClose( chassis->getMassSpaceInertiaTensor(), Vector3<real_Num>::unit() * 1000.0f );
    checkVectorClose( collision->getExtents(), Vector3<real_Num>( 0.905f, 0.585f, 2.2025f ) );

    auto children = controller->getChildObjects();
    BOOST_REQUIRE_EQUAL( children.size(), 7u );
    BOOST_CHECK( containsObject( children, physicsVehicle.get() ) );

    controller->unload( nullptr );
    BOOST_CHECK( !controller->isLoaded() );
    BOOST_CHECK( !controller->getVehicleController() );
    BOOST_CHECK( !controller->getChassis() );
    BOOST_CHECK( !controller->getCollision() );
    BOOST_CHECK( !physicsVehicle->isLoaded() );
    controller->unload( nullptr );

    fixture.sceneManager->destroyActor( actor );
}

BOOST_AUTO_TEST_CASE( components_car_controller_wires_four_wheels_by_drive_type )
{
    TestGuard fixture;
    BOOST_REQUIRE( fixture.isAvailable );
    VehicleFactoryGuard factoryGuard;

    auto actor = fixture.sceneManager->createActor();
    BOOST_REQUIRE( actor );

    std::array<SmartPtr<IGameActor>, 4> wheelActors;
    std::array<SmartPtr<WheelController>, 4> wheels;
    for( u32 i = 0; i < wheels.size(); ++i )
    {
        wheelActors[i] = fixture.sceneManager->createActor();
        BOOST_REQUIRE( wheelActors[i] );
        actor->addChild( wheelActors[i] );
        wheels[i] = wheelActors[i]->addComponent<WheelController>();
        BOOST_REQUIRE( wheels[i] );
    }

    auto controller = actor->addComponent<CarController>();
    BOOST_REQUIRE( controller );
    BOOST_REQUIRE( controller->getVehicleController() );
    controller->setDriveType( VehicleDriveType::RearWheelDrive );
    controller->update();

    const std::array<Vector3<real_Num>, 4> expectedPositions = {
        Vector3<real_Num>( -0.905f, 0.0f, -1.265f ), Vector3<real_Num>( 0.905f, 0.0f, -1.265f ),
        Vector3<real_Num>( -0.905f, 0.0f, 1.265f ), Vector3<real_Num>( 0.905f, 0.0f, 1.265f )
    };

    for( u32 i = 0; i < wheels.size(); ++i )
    {
        auto wheel = wheels[i]->getWheelController();
        BOOST_REQUIRE_MESSAGE( wheel, "Wheel " << i << " was not connected" );
        BOOST_CHECK_EQUAL( wheel->isSteeringWheel(), i < 2 );
        BOOST_CHECK_EQUAL( wheel->isPoweredWheel(), i >= 2 );
        checkClose( wheel->getRadius(), 0.35f );
        checkClose( wheel->getSuspensionTravel(), 0.3f );
        checkClose( wheel->getDamping(), 1000.0f );
        checkVectorClose( wheelActors[i]->getLocalPosition(), expectedPositions[i] );
    }

    fixture.sceneManager->destroyActor( actor );
}
