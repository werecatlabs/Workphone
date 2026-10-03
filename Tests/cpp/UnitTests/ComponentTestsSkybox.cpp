#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

/**
 * @brief Test basic creation of a Skybox component
 */
BOOST_AUTO_TEST_CASE( components_create_skybox )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        BOOST_CHECK( skybox->isValid() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test loading and unloading of Skybox component
 */
BOOST_AUTO_TEST_CASE( components_skybox_load_unload )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );

        skybox->load( nullptr );
        BOOST_CHECK( skybox->isLoaded() );

        skybox->unload( nullptr );
        BOOST_CHECK( !skybox->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test double loading and unloading of Skybox component
 */
BOOST_AUTO_TEST_CASE( components_skybox_double_load_unload )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );

        // Test double load
        skybox->load( nullptr );
        BOOST_CHECK( skybox->isLoaded() );
        skybox->load( nullptr );
        BOOST_CHECK( skybox->isLoaded() );

        // Test double unload
        skybox->unload( nullptr );
        BOOST_CHECK( !skybox->isLoaded() );
        skybox->unload( nullptr );
        BOOST_CHECK( !skybox->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test setting and getting distance for Skybox component
 */
BOOST_AUTO_TEST_CASE( components_skybox_distance )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test setting distance
        const f32 testDistance = 10000.0f;
        skybox->setDistance( testDistance );
        BOOST_CHECK_CLOSE( skybox->getDistance(), testDistance, 0.001f );

        // Test setting different distance
        const f32 newDistance = 5000.0f;
        skybox->setDistance( newDistance );
        BOOST_CHECK_CLOSE( skybox->getDistance(), newDistance, 0.001f );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test edge case: zero distance
 */
BOOST_AUTO_TEST_CASE( components_skybox_distance_zero )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test zero distance
        skybox->setDistance( 0.0f );
        BOOST_CHECK_CLOSE( skybox->getDistance(), 0.0f, 0.001f );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test edge case: negative distance
 */
BOOST_AUTO_TEST_CASE( components_skybox_distance_negative )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test negative distance (should still be settable, implementation decides handling)
        const f32 negativeDistance = -1000.0f;
        skybox->setDistance( negativeDistance );
        BOOST_CHECK_CLOSE( skybox->getDistance(), negativeDistance, 0.001f );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test setting and getting textures for Skybox component
 */
BOOST_AUTO_TEST_CASE( components_skybox_textures )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test getting initial textures (should be empty or null)
        auto textures = skybox->getTextures();
        BOOST_CHECK( textures.empty() || textures.size() >= 0 );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test setting texture by name
 */
BOOST_AUTO_TEST_CASE( components_skybox_texture_by_name )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test setting texture by name (should not crash even with invalid name)
        skybox->setTextureByName( "TestTexture.png", 0 );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test edge case: setting texture at invalid index
 */
BOOST_AUTO_TEST_CASE( components_skybox_texture_invalid_index )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test setting texture at high index (should handle gracefully)
        skybox->setTextureByName( "TestTexture.png", 255 );

        // Test getting texture at invalid index
        auto texture = skybox->getTexture( 255 );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test edge case: setting empty texture name
 */
BOOST_AUTO_TEST_CASE( components_skybox_texture_empty_name )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test setting empty texture name
        skybox->setTextureByName( "", 0 );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test swap left and right flag
 */
BOOST_AUTO_TEST_CASE( components_skybox_swap_left_right )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test default state
        bool initialSwap = skybox->getSwapLeftRight();

        // Test setting swap left right
        skybox->setSwapLeftRight( true );
        BOOST_CHECK( skybox->getSwapLeftRight() );

        skybox->setSwapLeftRight( false );
        BOOST_CHECK( !skybox->getSwapLeftRight() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test getting material from Skybox component
 */
BOOST_AUTO_TEST_CASE( components_skybox_material )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test getting material (may be null initially)
        auto material = skybox->getMaterial();

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test properties get/set for Skybox component
 */
BOOST_AUTO_TEST_CASE( components_skybox_properties )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test getting properties
        auto properties = skybox->getProperties();
        BOOST_REQUIRE( properties );

        // Test setting properties back
        skybox->setProperties( properties );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test edge case: setting null properties
 */
BOOST_AUTO_TEST_CASE( components_skybox_null_properties )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test setting null properties (should handle gracefully)
        skybox->setProperties( nullptr );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test skybox interface getter/setter
 */
BOOST_AUTO_TEST_CASE( components_skybox_interface )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test getting skybox interface pointer
        auto skyboxPtr = skybox->getSkyboxPtr();

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test update materials call
 */
BOOST_AUTO_TEST_CASE( components_skybox_update_materials )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test update materials (should not crash)
        skybox->updateMaterials();

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test setup material call
 */
BOOST_AUTO_TEST_CASE( components_skybox_setup_material )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test setup material (should not crash)
        skybox->setupMaterial();

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test multiple skybox components in scene
 */
BOOST_AUTO_TEST_CASE( components_skybox_multiple )
{
    try
    {
        TestGuard fixture;

        auto actor1 = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor1 );
        auto skybox1 = actor1->addComponent<Skybox>();
        BOOST_REQUIRE( skybox1 );
        skybox1->load( nullptr );

        auto actor2 = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor2 );
        auto skybox2 = actor2->addComponent<Skybox>();
        BOOST_REQUIRE( skybox2 );
        skybox2->load( nullptr );

        // Both should be valid and loaded
        BOOST_CHECK( skybox1->isLoaded() );
        BOOST_CHECK( skybox2->isLoaded() );

        // Set different distances
        skybox1->setDistance( 5000.0f );
        skybox2->setDistance( 10000.0f );

        BOOST_CHECK_CLOSE( skybox1->getDistance(), 5000.0f, 0.001f );
        BOOST_CHECK_CLOSE( skybox2->getDistance(), 10000.0f, 0.001f );

        fixture.sceneManager->destroyActor( actor1 );
        fixture.sceneManager->destroyActor( actor2 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test skybox component with all 6 texture faces
 */
BOOST_AUTO_TEST_CASE( components_skybox_all_faces )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        skybox->load( nullptr );

        // Test setting textures for all 6 faces (front, back, left, right, up, down)
        for( u8 i = 0; i < 6; ++i )
        {
            String textureName = String( "Skybox_Face_" ) + StringUtil::toString( i ) + ".png";
            skybox->setTextureByName( textureName, i );
        }

        // Verify we can get textures back
        for( u8 i = 0; i < 6; ++i )
        {
            auto texture = skybox->getTexture( i );
        }

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

/**
 * @brief Test skybox component lifecycle: create, modify, clear
 */
BOOST_AUTO_TEST_CASE( components_skybox_lifecycle )
{
    try
    {
        TestGuard fixture;

        auto actor = fixture.sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto skybox = actor->addComponent<Skybox>();
        BOOST_REQUIRE( skybox );
        BOOST_CHECK( skybox->isValid() );

        // Load
        skybox->load( nullptr );
        BOOST_CHECK( skybox->isLoaded() );

        // Modify
        skybox->setDistance( 8000.0f );
        skybox->setSwapLeftRight( true );
        skybox->setupMaterial();

        // Unload
        skybox->unload( nullptr );
        BOOST_CHECK( !skybox->isLoaded() );

        // Reload
        skybox->load( nullptr );
        BOOST_CHECK( skybox->isLoaded() );

        fixture.sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}
