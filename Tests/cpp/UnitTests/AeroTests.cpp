#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include "WPVehiclePhysics/WPVehiclePhysics.hpp"
#include "AeroTestsCallback.hpp"
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;
using namespace workphone::vehicle;

BOOST_AUTO_TEST_CASE( vehicle_aerodynamics_propunit )
{
    auto applicationManager = core::IApplicationManager::instance();

    auto aircraft = workphone::make_ptr<CAircraft>();
    //aircraft->load( nullptr );
    BOOST_CHECK( aircraft );

    auto callback = workphone::make_ptr<DummyAircraftCallback>();
    aircraft->setCallback( callback );

    auto body = workphone::make_ptr<CAircraftBody>();
    body->setParentAircraft( aircraft );
    aircraft->setBody( body );
    BOOST_CHECK( body->isValid() );

    auto esc = workphone::make_ptr<CESController>();
    BOOST_CHECK( esc );
    esc->setParent( body );
    esc->setParentAircraft( aircraft );
    BOOST_CHECK( esc->isValid() );

    auto battery = workphone::make_ptr<CBatteryPackStandard>();
    BOOST_CHECK( battery );
    battery->setParent( body );
    battery->setParentAircraft( aircraft );
    BOOST_CHECK( battery->isValid() );

    auto motor = workphone::make_ptr<CAircraftMotor>();
    BOOST_CHECK( motor );
    motor->setParent( body );
    motor->setParentAircraft( aircraft );
    BOOST_CHECK( motor->isValid() );

    auto prop = workphone::make_ptr<CAircraftPropeller>();
    BOOST_CHECK( prop );
    prop->setParent( body );
    prop->setParentAircraft( aircraft );
    BOOST_CHECK( prop->isValid() );

    auto propUnit = workphone::make_ptr<CAircraftPropellerUnit>();
    BOOST_CHECK( propUnit );
    propUnit->setParent( body );
    propUnit->setParentAircraft( aircraft );
    propUnit->setESC( esc );
    propUnit->setBatteryPack( battery );
    propUnit->setPowerUnit( motor );
    propUnit->setPropeller( prop );
    BOOST_CHECK( propUnit->isValid() );

    aircraft->addPropellerUnit( propUnit );

    auto propUnits = aircraft->getPropellerUnits();
    BOOST_CHECK( propUnits.size() == 1 );

    BOOST_CHECK( aircraft->isValid() );

    auto wheelController = workphone::make_ptr<WheelController>();
    //aircraft->addWheel( wheelController );

    aircraft->setChannel( CAircraft::m_thrChannel, 1.0f );
    BOOST_CHECK( aircraft->getChannel( CAircraft::m_thrChannel ) > 0.9f );

    real_Num fT = 0.0;
    real_Num fDT = 1.0 / 3000.0;

    BOOST_CHECK( aircraft->isValid() );
    if( aircraft->isValid() )
    {
        aircraft->setChannel( CAircraft::m_thrChannel, 0.0f );
        BOOST_CHECK( aircraft->getChannel( CAircraft::m_thrChannel ) <
                     std::numeric_limits<f32>::epsilon() );

        size_t numSteps = 10000;
        for( size_t i = 0; i < numSteps; ++i )
        {
            f32 throttleValue = static_cast<f32>( i ) / static_cast<f32>( numSteps );
            aircraft->setChannel( CAircraft::m_thrChannel, throttleValue );

            aircraft->update( fT, fDT );
            fT += fDT;

            BOOST_CHECK( aircraft->isValid() );
        }

        BOOST_CHECK( aircraft->isValid() );

        // test transforms
        for( size_t i = 0; i < numSteps; ++i )
        {
            callback->position.Y() += 100.0f;

            aircraft->update( fT, fDT );
            fT += fDT;

            BOOST_CHECK( aircraft->isValid() );
        }

        BOOST_CHECK( aircraft->isValid() );
    }

    // auto propUnits = aircraft->getPropellerUnits();
    for( auto propUnit : propUnits )
    {
        BOOST_CHECK( propUnit->isValid() );

        auto puProp = propUnit->getPropeller();
        BOOST_CHECK( puProp->isValid() );
    }

    aircraft->unload( nullptr );

    callback = nullptr;
    body = nullptr;
    esc = nullptr;
    battery = nullptr;
    motor = nullptr;
    prop = nullptr;
    propUnit = nullptr;
    aircraft = nullptr;
}

BOOST_AUTO_TEST_CASE( vehicle_aerodynamics_propunit_simple )
{
    auto applicationManager = core::IApplicationManager::instance();

    auto aircraft = workphone::make_ptr<CAircraft>();
    //aircraft->load( "" );
    BOOST_CHECK( aircraft );

    auto callback = workphone::make_ptr<DummyAircraftCallback>();
    aircraft->setCallback( callback );

    auto body = workphone::make_ptr<CAircraftBody>();
    body->setParentAircraft( aircraft );
    aircraft->setBody( body );
    BOOST_CHECK( body->isValid() );

    auto esc = workphone::make_ptr<CESController>();
    BOOST_CHECK( esc );
    esc->setParent( body );
    esc->setParentAircraft( aircraft );
    BOOST_CHECK( esc->isValid() );

    auto battery = workphone::make_ptr<CBatteryPackStandard>();
    BOOST_CHECK( battery );
    battery->setParent( body );
    battery->setParentAircraft( aircraft );
    BOOST_CHECK( battery->isValid() );

    auto motor = workphone::make_ptr<CAircraftMotor>();
    BOOST_CHECK( motor );
    motor->setParent( body );
    motor->setParentAircraft( aircraft );
    BOOST_CHECK( motor->isValid() );

    auto prop = workphone::make_ptr<CAircraftPropeller>();
    BOOST_CHECK( prop );
    prop->setParent( body );
    prop->setParentAircraft( aircraft );
    BOOST_CHECK( prop->isValid() );

    auto propUnit = workphone::make_ptr<CAircraftPropellerUnitSimple>();
    BOOST_CHECK( propUnit );
    propUnit->setParent( body );
    propUnit->setParentAircraft( aircraft );
    propUnit->setESC( esc );
    propUnit->setBatteryPack( battery );
    propUnit->setPowerUnit( motor );
    propUnit->setPropeller( prop );
    BOOST_CHECK( propUnit->isValid() );

    aircraft->addPropellerUnit( propUnit );

    auto propUnits = aircraft->getPropellerUnits();
    BOOST_CHECK( propUnits.size() == 1 );

    BOOST_CHECK( aircraft->isValid() );

    auto wheelController = workphone::make_ptr<WheelController>();
    //aircraft->addWheel( wheelController );

    aircraft->setChannel( CAircraft::m_thrChannel, 1.0f );
    BOOST_CHECK( aircraft->getChannel( CAircraft::m_thrChannel ) > 0.9f );

    real_Num fT = 0.0;
    real_Num fDT = 1.0 / 3000.0;

    BOOST_CHECK( aircraft->isValid() );
    if( aircraft->isValid() )
    {
        aircraft->setChannel( CAircraft::m_thrChannel, 0.0f );
        BOOST_CHECK( aircraft->getChannel( CAircraft::m_thrChannel ) <
                     std::numeric_limits<f32>::epsilon() );

        size_t numSteps = 10000;
        for( size_t i = 0; i < numSteps; ++i )
        {
            f32 throttleValue = static_cast<f32>( i ) / static_cast<f32>( numSteps );
            aircraft->setChannel( CAircraft::m_thrChannel, throttleValue );

            aircraft->update( fT, fDT );
            fT += fDT;

            BOOST_CHECK( aircraft->isValid() );
        }

        BOOST_CHECK( aircraft->isValid() );

        // test transforms
        for( size_t i = 0; i < numSteps; ++i )
        {
            callback->position.Y() += 100.0f;

            aircraft->update( fT, fDT );
            fT += fDT;

            BOOST_CHECK( aircraft->isValid() );
        }

        BOOST_CHECK( aircraft->isValid() );
    }

    // auto propUnits = aircraft->getPropellerUnits();
    for( auto propUnit : propUnits )
    {
        BOOST_CHECK( propUnit->isValid() );

        auto puProp = propUnit->getPropeller();
        BOOST_CHECK( puProp->isValid() );
    }

    aircraft->unload( nullptr );

    callback = nullptr;
    body = nullptr;
    esc = nullptr;
    battery = nullptr;
    motor = nullptr;
    prop = nullptr;
    propUnit = nullptr;
    aircraft = nullptr;
}

BOOST_AUTO_TEST_CASE( vehicle_aerodynamics_liftline )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto aircraft = workphone::make_ptr<CAircraft>();
        //aircraft->load( "" );
        BOOST_CHECK( aircraft );

        auto callback = workphone::make_ptr<DummyAircraftCallback>();
        aircraft->setCallback( callback );

        auto body = workphone::make_ptr<CAircraftBody>();
        body->setParentAircraft( aircraft );
        aircraft->setBody( body );
        BOOST_CHECK( body->isValid() );

        aircraft->unload( nullptr );
        aircraft = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( vehicle_aerodynamics_airfoil )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto aircraft = workphone::make_ptr<CAircraft>();
        //aircraft->load( "" );
        BOOST_CHECK( aircraft );

        auto callback = workphone::make_ptr<DummyAircraftCallback>();
        aircraft->setCallback( callback );

        auto body = workphone::make_ptr<CAircraftBody>();
        body->setParentAircraft( aircraft );
        aircraft->setBody( body );
        BOOST_CHECK( body->isValid() );

        auto esc = workphone::make_ptr<CESController>();
        BOOST_CHECK( esc );
        esc->setParent( body );
        esc->setParentAircraft( aircraft );
        BOOST_CHECK( esc->isValid() );

        auto battery = workphone::make_ptr<CBatteryPackStandard>();
        BOOST_CHECK( battery );
        battery->setParent( body );
        battery->setParentAircraft( aircraft );
        BOOST_CHECK( battery->isValid() );

        auto motor = workphone::make_ptr<CAircraftMotor>();
        BOOST_CHECK( motor );
        motor->setParent( body );
        motor->setParentAircraft( aircraft );
        BOOST_CHECK( motor->isValid() );

        auto prop = workphone::make_ptr<CAircraftPropeller>();
        BOOST_CHECK( prop );
        prop->setParent( body );
        prop->setParentAircraft( aircraft );
        BOOST_CHECK( prop->isValid() );

        auto propUnit = workphone::make_ptr<CAircraftPropellerUnitSimple>();
        BOOST_CHECK( propUnit );
        propUnit->setParent( body );
        propUnit->setParentAircraft( aircraft );
        propUnit->setESC( esc );
        propUnit->setBatteryPack( battery );
        propUnit->setPowerUnit( motor );
        propUnit->setPropeller( prop );
        BOOST_CHECK( propUnit->isValid() );

        aircraft->addPropellerUnit( propUnit );

        auto propUnits = aircraft->getPropellerUnits();
        BOOST_CHECK( propUnits.size() == 1 );

        BOOST_CHECK( aircraft->isValid() );

        auto wheelController = workphone::make_ptr<WheelController>();
        //aircraft->addWheel( wheelController );

        auto angle = static_cast<real_Num>( 14.0 );

        auto airfoil1 = workphone::make_ptr<CAerofoil>();
        airfoil1->setAircraft( aircraft );
        airfoil1->load( "CLARKY.airfoil" );

        auto clLookup1 = airfoil1->getCL();
        auto firstAngle = clLookup1->interpolate( angle );

        auto airfoil2 = workphone::make_ptr<CAerofoil>();
        airfoil2->setAircraft( aircraft );
        airfoil2->setReverseValues( true );
        airfoil2->load( "CLARKY.airfoil" );

        auto clLookup2 = airfoil2->getCL();
        auto secondAngle = clLookup2->interpolate( -angle );

        BOOST_CHECK( Math<real_Num>::equals( firstAngle, secondAngle ) );

        aircraft->setChannel( CAircraft::m_thrChannel, 1.0f );
        BOOST_CHECK( aircraft->getChannel( CAircraft::m_thrChannel ) > 0.9f );

        real_Num fT = 0.0;
        real_Num fDT = 1.0 / 3000.0;

        BOOST_CHECK( aircraft->isValid() );
        if( aircraft->isValid() )
        {
            aircraft->setChannel( CAircraft::m_thrChannel, 0.0f );
            BOOST_CHECK( aircraft->getChannel( CAircraft::m_thrChannel ) <
                         std::numeric_limits<f32>::epsilon() );

            size_t numSteps = 10;
            for( size_t i = 0; i < numSteps; ++i )
            {
                f32 throttleValue = static_cast<f32>( i ) / static_cast<f32>( numSteps );
                aircraft->setChannel( CAircraft::m_thrChannel, throttleValue );

                aircraft->update( fT, fDT );
                fT += fDT;

                BOOST_CHECK( aircraft->isValid() );
            }

            BOOST_CHECK( aircraft->isValid() );

            // test transforms
            for( size_t i = 0; i < numSteps; ++i )
            {
                callback->position.Y() += 100.0f;

                aircraft->update( fT, fDT );
                fT += fDT;

                BOOST_CHECK( aircraft->isValid() );
            }

            BOOST_CHECK( aircraft->isValid() );
        }

        // auto propUnits = aircraft->getPropellerUnits();
        for( auto propUnit : propUnits )
        {
            BOOST_CHECK( propUnit->isValid() );

            auto puProp = propUnit->getPropeller();
            BOOST_CHECK( puProp->isValid() );
        }

        aircraft->unload( nullptr );

        callback = nullptr;
        body = nullptr;
        esc = nullptr;
        battery = nullptr;
        motor = nullptr;
        prop = nullptr;
        propUnit = nullptr;
        aircraft = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( vehicle_aerodynamics_throttle )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto aircraft = workphone::make_ptr<CAircraft>();
        //aircraft->load( "" );
        BOOST_CHECK( aircraft );

        auto callback = workphone::make_ptr<DummyAircraftCallback>();
        aircraft->setCallback( callback );

        auto body = workphone::make_ptr<CAircraftBody>();
        body->setParentAircraft( aircraft );
        aircraft->setBody( body );
        BOOST_CHECK( body->isValid() );

        auto esc = workphone::make_ptr<CESController>();
        BOOST_CHECK( esc );
        esc->setParent( body );
        esc->setParentAircraft( aircraft );
        BOOST_CHECK( esc->isValid() );

        auto battery = workphone::make_ptr<CBatteryPackStandard>();
        BOOST_CHECK( battery );
        battery->setParent( body );
        battery->setParentAircraft( aircraft );
        BOOST_CHECK( battery->isValid() );

        auto motor = workphone::make_ptr<CAircraftMotor>();
        BOOST_CHECK( motor );
        motor->setParent( body );
        motor->setParentAircraft( aircraft );
        BOOST_CHECK( motor->isValid() );

        auto prop = workphone::make_ptr<CAircraftPropeller>();
        BOOST_CHECK( prop );
        prop->setParent( body );
        prop->setParentAircraft( aircraft );
        BOOST_CHECK( prop->isValid() );

        auto propUnit = workphone::make_ptr<CAircraftPropellerUnit>();
        BOOST_CHECK( propUnit );
        propUnit->setParent( body );
        propUnit->setParentAircraft( aircraft );
        propUnit->setESC( esc );
        propUnit->setBatteryPack( battery );
        propUnit->setPowerUnit( motor );
        propUnit->setPropeller( prop );
        BOOST_CHECK( propUnit->isValid() );

        aircraft->addPropellerUnit( propUnit );

        auto propUnits = aircraft->getPropellerUnits();
        BOOST_CHECK( propUnits.size() == 1 );

        BOOST_CHECK( aircraft->isValid() );

        auto wheelController = workphone::make_ptr<WheelController>();
        //aircraft->addWheel( wheelController );

        aircraft->setChannel( CAircraft::m_thrChannel, 1.0f );
        BOOST_CHECK( aircraft->getChannel( CAircraft::m_thrChannel ) > 0.9f );

        real_Num fT = 0.0;
        real_Num fDT = 1.0 / 3000.0;

        BOOST_CHECK( aircraft->isValid() );
        if( aircraft->isValid() )
        {
            aircraft->setChannel( CAircraft::m_thrChannel, 0.0f );
            BOOST_CHECK( aircraft->getChannel( CAircraft::m_thrChannel ) <
                         std::numeric_limits<f32>::epsilon() );

            size_t numSteps = 10;
            for( size_t i = 0; i < numSteps; ++i )
            {
                f32 throttleValue = static_cast<f32>( i ) / static_cast<f32>( numSteps );
                aircraft->setChannel( CAircraft::m_thrChannel, throttleValue );

                aircraft->update( fT, fDT );
                fT += fDT;

                auto thrustValue = aircraft->getThrust( 0 );
                BOOST_CHECK( Math<real_Num>::isFinite( thrustValue ) );
                BOOST_CHECK( aircraft->isValid() );
            }

            BOOST_CHECK( aircraft->isValid() );

            // test transforms
            for( size_t i = 0; i < numSteps; ++i )
            {
                callback->position.Y() += 100.0f;

                aircraft->update( fT, fDT );
                fT += fDT;

                BOOST_CHECK( aircraft->isValid() );
            }

            BOOST_CHECK( aircraft->isValid() );
        }

        // auto propUnits = aircraft->getPropellerUnits();
        for( auto propUnit : propUnits )
        {
            BOOST_CHECK( propUnit->isValid() );

            auto puProp = propUnit->getPropeller();
            BOOST_CHECK( puProp->isValid() );
        }

        aircraft->unload( nullptr );

        callback = nullptr;
        body = nullptr;
        esc = nullptr;
        battery = nullptr;
        motor = nullptr;
        prop = nullptr;
        propUnit = nullptr;
        aircraft = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( vehicle_aerodynamics_motor )
{
    using namespace workphone;

    auto motor = workphone::make_ptr<CAircraftMotor>();
    BOOST_CHECK( motor );
}

BOOST_AUTO_TEST_CASE( vehicle_aerodynamics_battery )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
        auto battery = workphone::make_ptr<CBatteryPackStandard>();
        BOOST_CHECK( battery );

        battery->setEmulateBattery( true );
        BOOST_CHECK( battery->getEmulateBattery() == true );

        battery->charge();
        BOOST_CHECK( Math<real_Num>::equals( battery->getCharge(), static_cast<real_Num>( 1.0 ) ) );

        auto fDT = static_cast<real_Num>( 1.0 ) / static_cast<real_Num>( 60.0 );
        auto fCurrent = static_cast<real_Num>( 1.0 );
        auto dischargeRate = ( fCurrent * fDT ) / static_cast<real_Num>( 3600.0 );
        battery->setDischargeRate( dischargeRate );

        auto dt = 1.0 / 60.0;
        for( size_t i = 0; i < 1000; ++i )
        {
            battery->discharge( 1.0, dt );
        }

        BOOST_CHECK( battery->getCharge() < 1.0 );

        battery->charge();
        BOOST_CHECK( Math<real_Num>::equals( battery->getCharge(), static_cast<real_Num>( 1.0 ) ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}
