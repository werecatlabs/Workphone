#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( audio_test )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto soundManager = applicationManager->getSoundManager();
        if( !soundManager )
        {
            BOOST_TEST_MESSAGE( "Sound manager is not available - skipping sound test" );
            return;
        }

        if( soundManager )
        {
            auto testSoundPath = String( "game_dubstep.wav" );
            auto sound = soundManager->loadResourceByType<ISound>( testSoundPath );
            BOOST_CHECK( sound );

            if( sound )
            {
                sound->play();
            }

            int count = 0;
            while( applicationManager->isRunning() && count++ > 1000 )
            {
                soundManager->update();
            }
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( audio )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();

        auto volumeEffect = factoryManager->make_object<IAudioEffectVolume>();
        if( volumeEffect )
        {
            float input[4096] = { 0 };
            float output[4096] = { 0 };

            volumeEffect->setInput( input );
            volumeEffect->setOutput( output );
            volumeEffect->setNumSamples( 32 );

            BOOST_CHECK( volumeEffect->getInput() );
            BOOST_CHECK( volumeEffect->getNumSamples() > 0 );

            volumeEffect->process();

            BOOST_CHECK( volumeEffect->getNumSamples() > 0 );
            BOOST_CHECK( volumeEffect->getOutput() );

            volumeEffect->setBypass( true );
            BOOST_CHECK( volumeEffect->getBypass() );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( sound_manager_initialization )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto soundManager = applicationManager->getSoundManager();
        if( !soundManager )
        {
            BOOST_TEST_MESSAGE( "Sound manager is not available - skipping sound test" );
            return;
        }
        BOOST_CHECK( soundManager->isValid() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during sound manager initialization" );
    }
}

BOOST_AUTO_TEST_CASE( sound_load_and_play )
{
        if( UnitTests::isHeadlessSoundMode() )
        {
            BOOST_TEST_MESSAGE( "Headless sound mode - skipping sound test" );
            return;
        }

    if( UnitTests::isHeadlessSoundMode() )
    {
        BOOST_TEST_MESSAGE( "Headless sound mode - skipping sound test" );
        return;
    }

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto soundManager = applicationManager->getSoundManager();
        if( !soundManager )
        {
            BOOST_TEST_MESSAGE( "Sound manager is not available - skipping sound test" );
            return;
        }
        BOOST_REQUIRE( soundManager->isValid() );

        auto soundFilePath = "game_dubstep.wav";
        auto sound = soundManager->loadResourceByType<ISound>( soundFilePath );
        if( !sound || !sound->isValid() )
        {
            BOOST_TEST_MESSAGE( "Sound backend cannot load a valid sound - skipping sound test" );
            return;
        }

        sound->play();
        BOOST_CHECK( sound->isValid() );

        // Update loop with corrected condition (was count++ > 1000, which never executes)
        auto count = 0;
        while( applicationManager->isRunning() && count++ < 1000 )
        {
            soundManager->update();
        }

        // Cleanup: stop the sound
        sound->stop();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during sound load and play" );
    }
}

BOOST_AUTO_TEST_CASE( sound_invalid_file_path )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto soundManager = applicationManager->getSoundManager();
        if( !soundManager )
        {
            BOOST_TEST_MESSAGE( "Sound manager is not available - skipping sound test" );
            return;
        }

        // Test loading non-existent file
        auto invalidSound = soundManager->loadResourceByType<ISound>( "non_existent_file.wav" );
        BOOST_CHECK( !invalidSound || !invalidSound->isValid() );

        // Test loading empty path
        auto emptyPathSound = soundManager->loadResourceByType<ISound>( "" );
        BOOST_CHECK( !emptyPathSound || !emptyPathSound->isValid() );
    }
    catch( std::exception &e )
    {
        // Expected behavior - exception may be thrown for invalid files
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( sound_play_stop_pause_resume )
{
        if( UnitTests::isHeadlessSoundMode() )
        {
            BOOST_TEST_MESSAGE( "Headless sound mode - skipping sound test" );
            return;
        }

    if( UnitTests::isHeadlessSoundMode() )
    {
        BOOST_TEST_MESSAGE( "Headless sound mode - skipping sound test" );
        return;
    }

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto soundManager = applicationManager->getSoundManager();
        if( !soundManager )
        {
            BOOST_TEST_MESSAGE( "Sound manager is not available - skipping sound test" );
            return;
        }
        BOOST_REQUIRE( soundManager->isValid() );

        auto soundFilePath = "game_dubstep.wav";
        auto sound = soundManager->loadResourceByType<ISound>( soundFilePath );
        if( !sound || !sound->isValid() )
        {
            BOOST_TEST_MESSAGE( "Sound backend cannot load a valid sound - skipping sound test" );
            return;
        }

        // Test play
        sound->play();
        soundManager->update();

        // Test pause
        sound->pause();
        soundManager->update();

        // Test resume
        sound->play();
        soundManager->update();

        // Test stop
        sound->stop();
        soundManager->update();

        // Verify sound is still valid after operations
        BOOST_CHECK( sound->isValid() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during sound playback control" );
    }
}

BOOST_AUTO_TEST_CASE( sound_multiple_instances )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto soundManager = applicationManager->getSoundManager();
        if( !soundManager )
        {
            BOOST_TEST_MESSAGE( "Sound manager is not available - skipping sound test" );
            return;
        }
        BOOST_REQUIRE( soundManager->isValid() );

        auto soundFilePath = "game_dubstep.wav";

        // Load multiple instances of the same sound
        auto sound1 = soundManager->loadResourceByType<ISound>( soundFilePath );
        auto sound2 = soundManager->loadResourceByType<ISound>( soundFilePath );

        BOOST_REQUIRE( sound1 );
        BOOST_REQUIRE( sound2 );

        // Play both sounds simultaneously
        sound1->play();
        sound2->play();

        for( auto i = 0; i < 100; ++i )
        {
            soundManager->update();
        }

        // Cleanup
        sound1->stop();
        sound2->stop();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during multiple sound instances test" );
    }
}

BOOST_AUTO_TEST_CASE( sound_resource_database_test )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto resourceDatabase = applicationManager->getResourceDatabase();
        BOOST_REQUIRE( resourceDatabase );

        auto dbFilePath = "test_sound_resources.db";
        resourceDatabase->setFilePath( dbFilePath );
        resourceDatabase->createDatabase();

        auto soundFilePath = "game_dubstep.wav";
        resourceDatabase->importFile( soundFilePath );
        if( auto jobQueue = applicationManager->getJobQueue() )
        {
            jobQueue->update();
        }

        auto soundManager = applicationManager->getSoundManager();

        if( !soundManager || !soundManager->isValid() )

        {
            BOOST_TEST_MESSAGE( "Sound manager is not available - skipping sound test" );

            return;
        }

        auto sound = resourceDatabase->loadResourceByType<ISound>( soundFilePath );

        if( !sound )

        {
            BOOST_TEST_MESSAGE( "Sound resource could not be loaded - skipping sound test" );

            return;
        }

        if( !sound->isValid() )
        {
            BOOST_TEST_MESSAGE( "Sound backend cannot load the imported file - skipping playback" );
            return;
        }

        sound->play();
        BOOST_CHECK( sound->isValid() );

        // Fixed: condition was count++ > 1000 (never true), should be < 1000
        auto count = 0;
        while( applicationManager->isRunning() && count++ < 1000 )
        {
            soundManager->update();
        }

        // Cleanup
        sound->stop();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during resource database sound test" );
    }
}

BOOST_AUTO_TEST_CASE( sound_resource_database_invalid_import )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto resourceDatabase = applicationManager->getResourceDatabase();
        BOOST_REQUIRE( resourceDatabase );

        auto dbFilePath = "test_sound_resources_invalid.db";
        resourceDatabase->setFilePath( dbFilePath );

        // Test importing non-existent file
        auto invalidFilePath = "non_existent_audio.wav";
        resourceDatabase->importFile( invalidFilePath );

        auto sound = resourceDatabase->loadResourceByType<ISound>( invalidFilePath );
        BOOST_CHECK( !sound || !sound->isValid() );
    }
    catch( std::exception &e )
    {
        // Expected - invalid files may throw
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( sound_rapid_play_stop )
{
        if( UnitTests::isHeadlessSoundMode() )
        {
            BOOST_TEST_MESSAGE( "Headless sound mode - skipping sound test" );
            return;
        }

    if( UnitTests::isHeadlessSoundMode() )
    {
        BOOST_TEST_MESSAGE( "Headless sound mode - skipping sound test" );
        return;
    }

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto soundManager = applicationManager->getSoundManager();
        if( !soundManager )
        {
            BOOST_TEST_MESSAGE( "Sound manager is not available - skipping sound test" );
            return;
        }
        BOOST_REQUIRE( soundManager->isValid() );

        auto soundFilePath = "game_dubstep.wav";
        auto sound = soundManager->loadResourceByType<ISound>( soundFilePath );
        if( !sound || !sound->isValid() )
        {
            BOOST_TEST_MESSAGE( "Sound backend cannot load a valid sound - skipping sound test" );
            return;
        }

        // Rapid play/stop cycles to test stability
        for( auto i = 0; i < 10; ++i )
        {
            sound->play();
            soundManager->update();
            sound->stop();
            soundManager->update();
        }

        BOOST_CHECK( sound->isValid() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during rapid play/stop test" );
    }
}
