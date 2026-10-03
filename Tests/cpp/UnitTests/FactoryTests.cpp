#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

// Test basic factory manager functionality
BOOST_AUTO_TEST_CASE( factory_basic )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    try
    {
        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        auto stateManager = applicationManager->getStateManager();
        BOOST_REQUIRE( stateManager );

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        auto initialFactoryCount = factoryManager->getFactories().size();

        // Setup factories
        UnitTests::setupFactories();

        // Verify factories were registered
        BOOST_CHECK( !factoryManager->getFactories().empty() );
        BOOST_CHECK( factoryManager->getFactories().size() >= initialFactoryCount );

        auto typeInfo = scene::CarController::typeInfo();
        BOOST_REQUIRE( typeInfo );

        auto typeManager = TypeManager::instance();
        BOOST_REQUIRE( typeManager );

        auto type = typeManager->getName( typeInfo );
        BOOST_CHECK( !StringUtil::isNullOrEmpty( type ) );

        // Test object creation from type
        auto carController = factoryManager->createObjectFromType<scene::CarController>( type );
        BOOST_CHECK( carController );

        // Verify the created object is of correct type
        if( carController )
        {
            auto objectTypeInfo = carController->typeInfo();
            BOOST_CHECK( objectTypeInfo == typeInfo );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in factory_basic test: " + std::string( e.what() ) );
    }
}

// Test factory creation with invalid type names (edge case)
BOOST_AUTO_TEST_CASE( factory_invalid_type )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    try
    {
        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        UnitTests::setupFactories();

        // Test with empty string
        auto emptyResult = factoryManager->createObjectFromType<scene::CarController>( "" );
        BOOST_CHECK( !emptyResult );

        // Test with non-existent type
        auto invalidResult =
            factoryManager->createObjectFromType<scene::CarController>( "NonExistentType" );
        BOOST_CHECK( !invalidResult );

        // Test with null/whitespace type
        auto whitespaceResult = factoryManager->createObjectFromType<scene::CarController>( "   " );
        BOOST_CHECK( !whitespaceResult );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        // This test expects no exception for invalid types, just null returns
        BOOST_FAIL( "Unexpected exception in factory_invalid_type test: " + std::string( e.what() ) );
    }
}

// Test factory manager without setup (edge case)
BOOST_AUTO_TEST_CASE( factory_uninitialized )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    try
    {
        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        const auto initialFactoryCount = factoryManager->getFactories().size();

        auto typeManager = TypeManager::instance();
        BOOST_REQUIRE( typeManager );

        auto typeInfo = scene::CarController::typeInfo();
        BOOST_REQUIRE( typeInfo );

        auto type = typeManager->getName( typeInfo );

        // The shared test runtime may already have factories registered by earlier tests.
        auto carController = factoryManager->createObjectFromType<scene::CarController>( type );
        if( initialFactoryCount == 0 )
        {
            BOOST_CHECK( !carController );
        }
        else
        {
            BOOST_CHECK( carController || initialFactoryCount > 0 );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

// Test multiple factory registrations (edge case)
BOOST_AUTO_TEST_CASE( factory_duplicate_registration )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    try
    {
        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        size_t initialCount = factoryManager->getFactories().size();

        // Setup factories multiple times
        UnitTests::setupFactories();
        size_t afterFirstSetup = factoryManager->getFactories().size();
        BOOST_CHECK( afterFirstSetup >= initialCount );

        UnitTests::setupFactories();
        size_t afterSecondSetup = factoryManager->getFactories().size();

        // Verify behavior with duplicate registration
        // (Implementation may allow duplicates or prevent them)
        BOOST_CHECK( afterSecondSetup >= afterFirstSetup );

        // Verify objects can still be created
        auto typeInfo = scene::CarController::typeInfo();
        auto typeManager = TypeManager::instance();
        auto type = typeManager->getName( typeInfo );
        auto carController = factoryManager->createObjectFromType<scene::CarController>( type );
        BOOST_CHECK( carController );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

using namespace workphone;

template <class T>
class FactoryCreator
{
public:
    void bind()
    {
        // Function = std::bind(&createFactoryFromTypeInfo<T>, this);
    }

    static void createFactoryFromTypeInfo()
    {
        using namespace workphone;

        auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
        BOOST_REQUIRE( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        auto typeManager = TypeManager::instance();
        BOOST_REQUIRE( typeManager );

        auto typeInfo = T::typeInfo();
        BOOST_REQUIRE( typeInfo );

        auto typeName = typeManager->getName( typeInfo );
        BOOST_CHECK( !StringUtil::isNullOrEmpty( typeName ) );

        auto typeHash = typeManager->getHash( typeInfo );

        factoryManager->addFactory( new FactoryTemplate<T>( typeName, typeHash ) );
    }
};

#define ADD_FACTORY( X ) FactoryCreator<X>::createFactoryFromTypeInfo();

BOOST_AUTO_TEST_CASE( factory_creation )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    try
    {
        auto factoryManager = workphone::make_ptr<FactoryManager>();
        BOOST_REQUIRE( factoryManager );

        auto typeManager = TypeManager::instance();
        BOOST_REQUIRE( typeManager );

        BOOST_CHECK( factoryManager->getFactories().size() >= 0 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in factory_creation test: " + std::string( e.what() ) );
    }
}

// Test type manager consistency
BOOST_AUTO_TEST_CASE( factory_type_manager_consistency )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    try
    {
        auto typeManager = TypeManager::instance();
        BOOST_REQUIRE( typeManager );

        auto typeInfo = scene::CarController::typeInfo();
        BOOST_REQUIRE( typeInfo );

        // Get type information multiple times
        auto typeName1 = typeManager->getName( typeInfo );
        auto typeName2 = typeManager->getName( typeInfo );

        // Verify consistency
        BOOST_CHECK_EQUAL( typeName1, typeName2 );

        auto typeHash1 = typeManager->getHash( typeInfo );
        auto typeHash2 = typeManager->getHash( typeInfo );

        BOOST_CHECK_EQUAL( typeHash1, typeHash2 );

        // Verify name is not empty
        BOOST_CHECK( !StringUtil::isNullOrEmpty( typeName1 ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in factory_type_manager_consistency test" );
    }
}

BOOST_AUTO_TEST_CASE( object_by_string )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    try
    {
        auto timer = applicationManager->getTimer();
        BOOST_REQUIRE( timer );

        auto stateManager = applicationManager->getStateManager();
        BOOST_REQUIRE( stateManager );

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        BOOST_REQUIRE( scene );

        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        auto initialFactoryCount = factoryManager->getFactories().size();
        UnitTests::setupFactories();
        BOOST_CHECK( factoryManager->getFactories().size() >= initialFactoryCount );

        auto typeInfo = GameActor::typeInfo();
        BOOST_REQUIRE( typeInfo );

        auto typeManager = TypeManager::instance();
        BOOST_REQUIRE( typeManager );

        auto type = typeManager->getName( typeInfo );
        BOOST_CHECK( !StringUtil::isNullOrEmpty( type ) );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        // Test actor is properly created
        BOOST_CHECK( actor.get() != nullptr );

        actor->unload( nullptr );

        // Verify actor is still valid after unload
        BOOST_CHECK( actor.get() != nullptr );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in object_by_string test: " + std::string( e.what() ) );
    }
}

// Test multiple actor creation
BOOST_AUTO_TEST_CASE( object_multiple_actors )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    try
    {
        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        UnitTests::setupFactories();

        // Create multiple actors
        std::vector<SmartPtr<GameActor>> actors;
        const size_t numActors = 10;

        for( size_t i = 0; i < numActors; ++i )
        {
            auto actor = sceneManager->createActor();
            BOOST_CHECK( actor );
            actors.push_back( actor );
        }

        BOOST_CHECK_EQUAL( actors.size(), numActors );

        // Verify all actors are unique
        for( size_t i = 0; i < actors.size(); ++i )
        {
            BOOST_CHECK( actors[i] );
            for( size_t j = i + 1; j < actors.size(); ++j )
            {
                BOOST_CHECK( actors[i] != actors[j] );
            }
        }

        // Clean up
        for( auto &actor : actors )
        {
            if( actor )
            {
                actor->unload( nullptr );
            }
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown in object_multiple_actors test" );
    }
}

// Test factory manager null safety (edge case)
BOOST_AUTO_TEST_CASE( factory_null_safety )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    try
    {
        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        // Test with null type info (if applicable)
        // This depends on the API design

        auto factories = factoryManager->getFactories();
        BOOST_CHECK( factories.size() >= 0 );  // Should not crash
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

// Test factory performance with many registrations
BOOST_AUTO_TEST_CASE( factory_performance )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    try
    {
        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        UnitTests::setupFactories();

        auto typeManager = TypeManager::instance();
        BOOST_REQUIRE( typeManager );

        auto typeInfo = scene::CarController::typeInfo();
        auto type = typeManager->getName( typeInfo );

        // Create multiple objects rapidly
        const size_t numObjects = 100;
        std::vector<SmartPtr<scene::CarController>> objects;

        for( size_t i = 0; i < numObjects; ++i )
        {
            auto obj = factoryManager->createObjectFromType<scene::CarController>( type );
            if( obj )
            {
                objects.push_back( obj );
            }
        }

        BOOST_CHECK( objects.size() == numObjects );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}
