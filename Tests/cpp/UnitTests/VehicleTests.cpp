#include "UnitTests.hpp"
#include <WPVehiclePhysics/CAircraft.hpp>
#include <WPVehiclePhysics/CCarController.hpp>
#include <WPVehiclePhysics/CDriveTrain.hpp>
#include <WPVehiclePhysics/WheelControllerArcade.hpp>
#include <WPVehiclePhysics/WheelControllerBrush.hpp>
#include <WPVehiclePhysics/WheelControllerPacejka.hpp>
#include <WPVehiclePhysics/WPVehiclePhysics.hpp>
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

namespace
{
    constexpr auto Tolerance = 0.001f;

    void checkFinite( physics_Num value )
    {
        BOOST_CHECK( Math<physics_Num>::isFinite( value ) );
    }

    void checkFinite( const Vector3<physics_Num> &value )
    {
        BOOST_CHECK( MathUtil<physics_Num>::isFinite( value ) );
    }

    template <class T>
    int enumValue( T value )
    {
        return static_cast<int>( value );
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

BOOST_AUTO_TEST_CASE( vehicle_load_from_db )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        //auto vehicleActor = ApplicationUtil::loadVehicleFromDB( 3 );
        //BOOST_CHECK( vehicleActor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( vehicle_factory_keeps_car_wheel_controller_selection_unambiguous )
{
    auto applicationManager = core::IApplicationManager::instancePtr();
    BOOST_REQUIRE( applicationManager );

    auto factoryManager = applicationManager->getFactoryManagerPtr();
    BOOST_REQUIRE( factoryManager );
    VehicleFactoryGuard factoryGuard;

    // Both factories intentionally coexist. The car hint must resolve IVehicle to
    // CCarController, while IAircraft continues to resolve independently.
    auto vehicle = factoryManager->make_object<vehicle::IVehicle>( "CCarController" );
    BOOST_REQUIRE( vehicle );
    auto carController = workphone::dynamic_pointer_cast<CCarController>( vehicle );
    BOOST_REQUIRE( carController );

    carController->load( nullptr );
    BOOST_REQUIRE( carController->isLoaded() );
    for( u32 wheelIndex = 0; wheelIndex < 4; ++wheelIndex )
    {
        auto wheel = carController->getWheelController( wheelIndex );
        BOOST_REQUIRE_MESSAGE( wheel, "Missing car wheel controller " << wheelIndex );
        BOOST_CHECK_GT( wheel->getRadius(), static_cast<physics_Num>( 0.0 ) );
        BOOST_CHECK_GT( wheel->getSuspensionTravel(), static_cast<physics_Num>( 0.0 ) );
    }
    carController->unload( nullptr );

    auto aircraft = factoryManager->make_object<vehicle::IAircraft>();
    BOOST_REQUIRE( aircraft );
    BOOST_CHECK( workphone::dynamic_pointer_cast<vehicle::CAircraft>( aircraft ) );
}

BOOST_AUTO_TEST_CASE( vehicle_drive_train_defaults_and_engine_torque )
{
    try
    {
        CDriveTrain driveTrain;

        BOOST_CHECK( !driveTrain.isValid() );
        BOOST_CHECK_EQUAL( enumValue( driveTrain.getState() ),
                           enumValue( IVehicleComponent::State::AWAKE ) );
        BOOST_CHECK( driveTrain.isAutomatic() );
        BOOST_CHECK_EQUAL( driveTrain.getGear(), 2 );
        BOOST_CHECK_CLOSE( driveTrain.getThrottle(), 0.0f, Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getThrottleInput(), 0.0f, Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getFinalDriveRatio(), static_cast<physics_Num>( 3.23 ),
                           Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getMinRPM(), static_cast<physics_Num>( 800.0 ), Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getMaxRPM(), static_cast<physics_Num>( 6400.0 ), Tolerance );

        auto gearRatios = driveTrain.getGearRatios();
        BOOST_REQUIRE_EQUAL( gearRatios.size(), 7u );
        BOOST_CHECK_CLOSE( gearRatios[0], static_cast<physics_Num>( 0.0 ), Tolerance );
        BOOST_CHECK_LT( gearRatios[1], static_cast<physics_Num>( 0.0 ) );
        BOOST_CHECK_GT( gearRatios[2], static_cast<physics_Num>( 0.0 ) );

        driveTrain.setRPM( static_cast<physics_Num>( 0.0 ) );
        BOOST_CHECK_CLOSE( driveTrain.calcEngineTorque(), static_cast<physics_Num>( 0.0 ), Tolerance );

        driveTrain.setRPM( driveTrain.getTorqueRPM() );
        BOOST_CHECK_CLOSE( driveTrain.calcEngineTorque(), driveTrain.getMaxTorque(), Tolerance );

        driveTrain.setRPM( driveTrain.getPowerRPM() );
        auto torqueAtPowerRPM = driveTrain.calcEngineTorque();
        BOOST_CHECK_GT( torqueAtPowerRPM, static_cast<physics_Num>( 0.0 ) );
        BOOST_CHECK_LE( torqueAtPowerRPM, driveTrain.getMaxTorque() );
        checkFinite( torqueAtPowerRPM );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( vehicle_drive_train_setters_and_gear_selection )
{
    try
    {
        CDriveTrain driveTrain;

        driveTrain.setThrottle( 0.75f );
        driveTrain.setThrottleInput( static_cast<physics_Num>( -0.25 ) );
        driveTrain.setAutomatic( false );
        driveTrain.setFinalDriveRatio( static_cast<physics_Num>( 4.1 ) );
        driveTrain.setMinRPM( static_cast<physics_Num>( 900.0 ) );
        driveTrain.setMaxRPM( static_cast<physics_Num>( 7000.0 ) );
        driveTrain.setMaxTorque( static_cast<physics_Num>( 500.0 ) );
        driveTrain.setTorqueRPM( static_cast<physics_Num>( 3200.0 ) );
        driveTrain.setMaxPower( static_cast<physics_Num>( 220000.0 ) );
        driveTrain.setPowerRPM( static_cast<physics_Num>( 6100.0 ) );
        driveTrain.setEngineInertia( static_cast<physics_Num>( 0.45 ) );
        driveTrain.setEngineBaseFriction( static_cast<physics_Num>( 30.0 ) );
        driveTrain.setEngineRPMFriction( static_cast<physics_Num>( 0.03 ) );
        driveTrain.setDifferentialLockCoefficient( static_cast<physics_Num>( 0.5 ) );
        driveTrain.setStarterGearRatio( static_cast<physics_Num>( 2.7 ) );
        driveTrain.setClutchThrottleRPMBoost( static_cast<physics_Num>( 2500.0 ) );
        driveTrain.setUpShiftBaseRPMScale( static_cast<physics_Num>( 0.6 ) );
        driveTrain.setUpShiftThrottleRPMScale( static_cast<physics_Num>( 0.3 ) );
        driveTrain.setDownShiftBaseRPMScale( static_cast<physics_Num>( 0.2 ) );
        driveTrain.setDownShiftThrottleRPMScale( static_cast<physics_Num>( 0.35 ) );
        driveTrain.setOverRevTorqueFalloff( static_cast<physics_Num>( 0.004 ) );

        BOOST_CHECK_CLOSE( driveTrain.getThrottle(), 0.75f, Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getThrottleInput(), static_cast<physics_Num>( -0.25 ), Tolerance );
        BOOST_CHECK( !driveTrain.isAutomatic() );
        BOOST_CHECK_CLOSE( driveTrain.getFinalDriveRatio(), static_cast<physics_Num>( 4.1 ), Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getMinRPM(), static_cast<physics_Num>( 900.0 ), Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getMaxRPM(), static_cast<physics_Num>( 7000.0 ), Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getMaxTorque(), static_cast<physics_Num>( 500.0 ), Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getTorqueRPM(), static_cast<physics_Num>( 3200.0 ), Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getMaxPower(), static_cast<physics_Num>( 220000.0 ), Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getPowerRPM(), static_cast<physics_Num>( 6100.0 ), Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getEngineInertia(), static_cast<physics_Num>( 0.45 ), Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getEngineBaseFriction(), static_cast<physics_Num>( 30.0 ),
                           Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getEngineRPMFriction(), static_cast<physics_Num>( 0.03 ),
                           Tolerance );
        BOOST_CHECK_CLOSE( driveTrain.getDifferentialLockCoefficient(), static_cast<physics_Num>( 0.5 ),
                           Tolerance );

        Array<physics_Num> ratios = { static_cast<physics_Num>( 0.0 ), static_cast<physics_Num>( -3.0 ),
                                      static_cast<physics_Num>( 3.8 ), static_cast<physics_Num>( 2.1 ) };
        driveTrain.setGearRatios( ratios );
        driveTrain.setGear( 2 );
        driveTrain.shiftUp();
        BOOST_CHECK_EQUAL( driveTrain.getGear(), 3 );
        driveTrain.shiftUp();
        BOOST_CHECK_EQUAL( driveTrain.getGear(), 3 );
        driveTrain.shiftDown();
        BOOST_CHECK_EQUAL( driveTrain.getGear(), 2 );
        driveTrain.shiftDown();
        BOOST_CHECK_EQUAL( driveTrain.getGear(), 1 );
        driveTrain.shiftDown();
        BOOST_CHECK_EQUAL( driveTrain.getGear(), 0 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( vehicle_wheel_brush_state_and_configuration )
{
    try
    {
        WheelControllerBrush wheel;

        BOOST_CHECK_EQUAL( enumValue( wheel.getTireModel() ), enumValue( TireModel::Brush ) );
        BOOST_CHECK( !wheel.isValid() );
        BOOST_CHECK( !wheel.isSteeringWheel() );
        BOOST_CHECK( !wheel.isPoweredWheel() );
        BOOST_CHECK( !wheel.isGrounded() );

        wheel.setRadius( static_cast<physics_Num>( 0.42 ) );
        wheel.setSuspensionTravel( static_cast<physics_Num>( 0.65 ) );
        wheel.setSpringRate( static_cast<physics_Num>( 8000.0 ) );
        wheel.setDamping( static_cast<physics_Num>( 1500.0 ) );
        wheel.setMass( static_cast<physics_Num>( 90.0 ) );
        wheel.setMassFraction( static_cast<physics_Num>( 0.2 ) );
        wheel.setInertia( static_cast<physics_Num>( 3.0 ) );
        wheel.setGrip( static_cast<physics_Num>( 1.2 ) );
        wheel.setStaticFrictionCoefficient( static_cast<physics_Num>( 1.15 ) );
        wheel.setSlidingFrictionCoefficient( static_cast<physics_Num>( 0.95 ) );
        wheel.setLongitudinalStiffness( static_cast<physics_Num>( 70000.0 ) );
        wheel.setLateralStiffness( static_cast<physics_Num>( 55000.0 ) );
        wheel.setBrakeFrictionTorque( static_cast<physics_Num>( 6500.0 ) );
        wheel.setHandbrakeFrictionTorque( static_cast<physics_Num>( 7000.0 ) );
        wheel.setRollingResistanceTorque( static_cast<physics_Num>( 12.0 ) );
        wheel.setSteeringAngle( static_cast<physics_Num>( 15.0 ) );
        wheel.setBrake( static_cast<physics_Num>( 0.25 ) );
        wheel.setHandbrake( static_cast<physics_Num>( 0.5 ) );
        wheel.setAngularVelocity( static_cast<physics_Num>( 32.0 ) );
        wheel.setSteeringWheel( true );
        wheel.setPoweredWheel( true );
        wheel.setTorque( static_cast<physics_Num>( 120.0 ) );
        wheel.addTorque( static_cast<physics_Num>( 30.0 ) );

        BOOST_CHECK_CLOSE( wheel.getRadius(), static_cast<physics_Num>( 0.42 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getSuspensionTravel(), static_cast<physics_Num>( 0.65 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getMass(), static_cast<physics_Num>( 90.0 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getMassFraction(), static_cast<physics_Num>( 0.2 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getTorque(), static_cast<physics_Num>( 150.0 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getBrake(), static_cast<physics_Num>( 0.25 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getHandbrake(), static_cast<physics_Num>( 0.5 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getAngularVelocity(), static_cast<physics_Num>( 32.0 ), Tolerance );
        BOOST_CHECK( wheel.isSteeringWheel() );
        BOOST_CHECK( wheel.isPoweredWheel() );

        wheel.setPoweredWheel( false );
        BOOST_CHECK( !wheel.isPoweredWheel() );
        BOOST_CHECK_CLOSE( wheel.getTorque(), static_cast<physics_Num>( 0.0 ), Tolerance );

        checkFinite( wheel.getWheelVelo() );
        checkFinite( wheel.getLocalVelo() );
        checkFinite( wheel.getSuspensionForceVector() );
        checkFinite( wheel.getRoadForceVector() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( vehicle_wheel_arcade_torque_and_friction_state )
{
    try
    {
        WheelControllerArcade wheel;

        BOOST_CHECK_EQUAL( enumValue( wheel.getTireModel() ), enumValue( TireModel::Simple ) );
        BOOST_CHECK( !wheel.isGrounded() );
        BOOST_CHECK( !wheel.isSteeringWheel() );
        BOOST_CHECK( !wheel.isPoweredWheel() );

        wheel.setRadius( static_cast<physics_Num>( 0.5 ) );
        wheel.setSuspensionDistance( static_cast<physics_Num>( 0.7 ) );
        wheel.setSpringForce( static_cast<physics_Num>( 7000.0 ) );
        wheel.setDamping( static_cast<physics_Num>( 1200.0 ) );
        wheel.setMassFraction( static_cast<physics_Num>( 0.3 ) );
        wheel.setAngularVelocity( static_cast<physics_Num>( 20.0 ) );
        wheel.setSteeringAngle( static_cast<physics_Num>( 25.0 ) );
        wheel.setSteeringWheel( true );
        wheel.setPoweredWheel( true );
        wheel.setLateralFrictionCoefficient( static_cast<physics_Num>( 6.0 ) );
        wheel.setRollingResistanceCoefficient( static_cast<physics_Num>( 0.02 ) );
        wheel.setTorque( static_cast<physics_Num>( 400.0 ) );
        wheel.addTorque( static_cast<physics_Num>( 200.0 ) );

        BOOST_CHECK_CLOSE( wheel.getRadius(), static_cast<physics_Num>( 0.5 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getSuspensionTravel(), static_cast<physics_Num>( 0.7 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getSpringRate(), static_cast<physics_Num>( 7000.0 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getDamping(), static_cast<physics_Num>( 1200.0 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getMassFraction(), static_cast<physics_Num>( 0.3 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getAngularVelocity(), static_cast<physics_Num>( 20.0 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getSteeringAngle(), static_cast<physics_Num>( 25.0 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getLateralFrictionCoefficient(), static_cast<physics_Num>( 6.0 ),
                           Tolerance );
        BOOST_CHECK_CLOSE( wheel.getRollingResistanceCoefficient(), static_cast<physics_Num>( 0.02 ),
                           Tolerance );
        BOOST_CHECK_CLOSE( wheel.getTorque(), static_cast<physics_Num>( 150.0 ), Tolerance );
        BOOST_CHECK( wheel.isSteeringWheel() );
        BOOST_CHECK( wheel.isPoweredWheel() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( vehicle_wheel_pacejka_force_model_and_state )
{
    try
    {
        WheelControllerPacejka wheel;

        BOOST_CHECK_EQUAL( enumValue( wheel.getTireModel() ), enumValue( TireModel::Pacejka ) );
        BOOST_CHECK( !wheel.isPoweredWheel() );
        BOOST_CHECK( !wheel.isSteeringWheel() );
        BOOST_CHECK( !wheel.isOnGround() );
        BOOST_CHECK_GT( wheel.getPacejkaA().size(), 0u );
        BOOST_CHECK_GT( wheel.getPacejkaB().size(), 0u );
        BOOST_CHECK_GT( wheel.getMaxSlip(), static_cast<physics_Num>( 0.0 ) );
        BOOST_CHECK_GT( wheel.getMaxAngle(), static_cast<physics_Num>( 0.0 ) );

        wheel.setPoweredWheel( true );
        wheel.setSteeringWheel( true );
        wheel.setRadius( static_cast<physics_Num>( 0.33 ) );
        wheel.setSuspensionTravel( static_cast<physics_Num>( 0.55 ) );
        wheel.setDamping( static_cast<physics_Num>( 2500.0 ) );
        wheel.setSpringRate( static_cast<physics_Num>( 6000.0 ) );
        wheel.setSuspensionDistance( static_cast<physics_Num>( 0.45 ) );
        wheel.setInertia( static_cast<physics_Num>( 2.8 ) );
        wheel.setGrip( static_cast<physics_Num>( 0.8 ) );
        wheel.setBrakeFrictionTorque( static_cast<physics_Num>( 6000.0 ) );
        wheel.setHandbrakeFrictionTorque( static_cast<physics_Num>( 3000.0 ) );
        wheel.setFrictionTorque( static_cast<physics_Num>( 0.03 ) );
        wheel.setMaxSteeringAngle( static_cast<physics_Num>( 45.0 ) );
        wheel.setMassFraction( static_cast<physics_Num>( 0.25 ) );
        wheel.setDriveTorque( static_cast<physics_Num>( 300.0 ) );
        wheel.setDriveFrictionTorque( static_cast<physics_Num>( 25.0 ) );
        wheel.setBrake( static_cast<physics_Num>( 0.4 ) );
        wheel.setHandbrake( static_cast<physics_Num>( 0.2 ) );
        wheel.setDrivetrainInertia( static_cast<physics_Num>( 1.1 ) );
        wheel.setSuspensionForceInput( static_cast<physics_Num>( 100.0 ) );
        wheel.setAngularVelocity( static_cast<physics_Num>( 12.0 ) );
        wheel.setSlipRatio( static_cast<physics_Num>( 0.1 ) );
        wheel.setSlipVelo( static_cast<physics_Num>( 3.0 ) );
        wheel.setCompression( static_cast<physics_Num>( 0.5 ) );
        wheel.setFullCompressionSpringForce( static_cast<physics_Num>( 12000.0 ) );
        wheel.setNormalForce( static_cast<physics_Num>( 4000.0 ) );
        wheel.setSlipAngle( static_cast<physics_Num>( 0.05 ) );

        BOOST_CHECK( wheel.isPoweredWheel() );
        BOOST_CHECK( wheel.isSteeringWheel() );
        BOOST_CHECK_CLOSE( wheel.getRadius(), static_cast<physics_Num>( 0.33 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getSuspensionTravel(), static_cast<physics_Num>( 0.55 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getSuspensionDistance(), static_cast<physics_Num>( 0.45 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getDriveTorque(), static_cast<physics_Num>( 300.0 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getDriveFrictionTorque(), static_cast<physics_Num>( 25.0 ), Tolerance );
        BOOST_CHECK_CLOSE( wheel.getDrivetrainInertia(), static_cast<physics_Num>( 1.1 ), Tolerance );

        BOOST_CHECK_CLOSE( wheel.calcLongitudinalForceUnit( 0.0f, 0.5f ), 0.0f, Tolerance );
        BOOST_CHECK_CLOSE( wheel.calcLateralForceUnit( 0.0f, 0.5f ), 0.0f, Tolerance );

        auto noLoadForce = wheel.combinedForce( 0.0f, 0.5f, 0.5f );
        BOOST_CHECK_SMALL( noLoadForce.length(), static_cast<physics_Num>( 0.001 ) );

        auto loadedForce = wheel.combinedForce( 4000.0f, 0.1f, 0.05f );
        checkFinite( loadedForce );
        BOOST_CHECK_GT( loadedForce.length(), static_cast<physics_Num>( 0.0 ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( vehicle_car_controller_configuration )
{
    try
    {
        CCarController controller;

        BOOST_CHECK_EQUAL( enumValue( controller.getState() ), enumValue( IVehicle::State::AWAKE ) );
        BOOST_CHECK_EQUAL( enumValue( controller.getDriveType() ),
                           enumValue( VehicleDriveType::RearWheelDrive ) );
        BOOST_CHECK_CLOSE( controller.getEditSteeringScale(), static_cast<physics_Num>( 70.0 ),
                           Tolerance );
        BOOST_CHECK_CLOSE( controller.getPlaySteeringScale(), static_cast<physics_Num>( 50.0 ),
                           Tolerance );
        BOOST_CHECK_CLOSE( controller.getDefaultMass(), static_cast<physics_Num>( 1000.0 ), Tolerance );

        controller.setEditSteeringScale( static_cast<physics_Num>( 45.0 ) );
        controller.setPlaySteeringScale( static_cast<physics_Num>( 30.0 ) );
        controller.setDefaultMass( static_cast<physics_Num>( 1250.0 ) );
        controller.setDriveType( VehicleDriveType::AllWheelDrive );
        controller.setState( IVehicle::State::EDIT );

        BOOST_CHECK_EQUAL( enumValue( controller.getState() ), enumValue( IVehicle::State::EDIT ) );
        BOOST_CHECK_EQUAL( enumValue( controller.getDriveType() ),
                           enumValue( VehicleDriveType::AllWheelDrive ) );
        BOOST_CHECK_CLOSE( controller.getEditSteeringScale(), static_cast<physics_Num>( 45.0 ),
                           Tolerance );
        BOOST_CHECK_CLOSE( controller.getPlaySteeringScale(), static_cast<physics_Num>( 30.0 ),
                           Tolerance );
        BOOST_CHECK_CLOSE( controller.getDefaultMass(), static_cast<physics_Num>( 1250.0 ), Tolerance );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " << e.what() );
    }
}

BOOST_AUTO_TEST_CASE( vehicle_car_reset_clears_controls_forces_and_wheel_spin )
{
    struct ResetTestCar : CCarController
    {
        Vector3<physics_Num> pendingForce() const { return m_force; }
        Vector3<physics_Num> pendingTorque() const { return m_torque; }
    };
    ResetTestCar car;
    car.setChannel( 0, 1.0f );
    car.setChannel( 1, 0.5f );
    car.setChannel( 2, -1.0f );
    car.addForce( 0, Vector3<physics_Num>( 10.0f, 0.0f, 0.0f ), Vector3<physics_Num>::zero() );
    car.addTorque( 0, Vector3<physics_Num>( 0.0f, 20.0f, 0.0f ) );
    BOOST_CHECK_GT( car.pendingForce().length(), 0.0f );
    BOOST_CHECK_GT( car.pendingTorque().length(), 0.0f );
    car.reset();
    BOOST_CHECK_SMALL( car.pendingForce().length(), Tolerance );
    BOOST_CHECK_SMALL( car.pendingTorque().length(), Tolerance );
    for( s32 i = 0; i < 3; ++i )
        BOOST_CHECK_SMALL( car.getChannel( i ), Tolerance );

    WheelControllerBrush wheel;
    wheel.setDamping( 4000.0f );
    wheel.setAngularVelocity( 80.0f );
    wheel.setTorque( 200.0f );
    wheel.setBrake( 1.0f );
    wheel.setHandbrake( 1.0f );
    wheel.setSteeringAngle( 15.0f );
    wheel.reset();
    BOOST_CHECK_SMALL( wheel.getAngularVelocity(), Tolerance );
    BOOST_CHECK_SMALL( wheel.getTorque(), Tolerance );
    BOOST_CHECK_SMALL( wheel.getBrake(), Tolerance );
    BOOST_CHECK_SMALL( wheel.getHandbrake(), Tolerance );
    BOOST_CHECK_SMALL( wheel.getSteeringAngle(), Tolerance );
    BOOST_CHECK_CLOSE( wheel.getDamping(), 4000.0f, Tolerance );
}
