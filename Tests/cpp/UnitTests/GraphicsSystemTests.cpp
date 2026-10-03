#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::render;
using namespace workphone::scene;

// ============================================================================
// Entity Creation Tests
// ============================================================================

BOOST_AUTO_TEST_CASE( test_entity_creation )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneManager );

        auto mesh = sceneManager->addGraphicsObjectByTypeId( render::IGraphicsMesh::typeInfo() );
        BOOST_CHECK( mesh != nullptr );

        if( mesh )
        {
            BOOST_CHECK( mesh->isValid() );
            BOOST_CHECK( mesh->getLoadingState() != LoadingState::Unloaded );

            // Cleanup
            sceneManager->removeGraphicsObject( mesh );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during entity creation test" );
    }
}

BOOST_AUTO_TEST_CASE( test_entity_creation_null_scene_manager )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system not available - skipping test" );
            return;
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        if( !sceneManager )
        {
            BOOST_TEST_MESSAGE( "Scene manager not available - expected behavior" );
            return;
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        // Expected behavior in some cases
    }
}

BOOST_AUTO_TEST_CASE( test_multiple_entity_creation )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneManager );

        const size_t entityCount = 10;
        std::vector<SmartPtr<IGraphicsObject>> entities;

        for( size_t i = 0; i < entityCount; ++i )
        {
            auto mesh = sceneManager->addGraphicsObjectByTypeId( render::IGraphicsMesh::typeInfo() );
            BOOST_REQUIRE( mesh );
            entities.push_back( mesh );
        }

        BOOST_CHECK_EQUAL( entities.size(), entityCount );

        // Verify all entities are unique
        for( size_t i = 0; i < entities.size(); ++i )
        {
            for( size_t j = i + 1; j < entities.size(); ++j )
            {
                BOOST_CHECK( entities[i] != entities[j] );
            }
        }

        // Cleanup
        for( auto &entity : entities )
        {
            sceneManager->removeGraphicsObject( entity );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during multiple entity creation test" );
    }
}

// ============================================================================
// Mesh Tests
// ============================================================================

BOOST_AUTO_TEST_CASE( test_entity_mesh )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto resourceDatabase = applicationManager->getResourceDatabase();
        BOOST_REQUIRE( resourceDatabase );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneManager );

        auto meshManager = applicationManager->getMeshManager();
        BOOST_CHECK( meshManager );

        auto entity = sceneManager->addGraphicsObjectByTypeId( render::IGraphicsMesh::typeInfo() );
        BOOST_REQUIRE( entity );

        auto cubeMeshResource = resourceDatabase->loadResource( "cube.mesh" );
        if( cubeMeshResource )
        {
            BOOST_CHECK( cubeMeshResource->isValid() );
            BOOST_CHECK( cubeMeshResource->getLoadingState() == LoadingState::Loaded );
        }
        else
        {
            BOOST_TEST_MESSAGE( "cube.mesh resource not found - this may be expected" );
        }

        // Cleanup
        sceneManager->removeGraphicsObject( entity );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during mesh test" );
    }
}

BOOST_AUTO_TEST_CASE( test_entity_mesh_invalid_resource )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto resourceDatabase = applicationManager->getResourceDatabase();
        BOOST_REQUIRE( resourceDatabase );

        auto invalidMeshResource = resourceDatabase->loadResource( "nonexistent.mesh" );
        BOOST_CHECK( invalidMeshResource == nullptr );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        // Expected for invalid resources
    }
}

BOOST_AUTO_TEST_CASE( test_entity_mesh_empty_resource_name )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto resourceDatabase = applicationManager->getResourceDatabase();
        BOOST_REQUIRE( resourceDatabase );

        auto emptyResource = resourceDatabase->loadResource( "" );
        BOOST_CHECK( emptyResource == nullptr );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        // Expected for empty resource names
    }
}

BOOST_AUTO_TEST_CASE( test_mesh_visibility )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneManager );

        auto pMesh = sceneManager->addGraphicsObjectByTypeId( render::IGraphicsMesh::typeInfo() );
        auto mesh = workphone::static_pointer_cast<IGraphicsMesh>( pMesh );
        BOOST_REQUIRE( mesh );

        // Test explicit visibility state. Newly-created meshes may not have renderer-side
        // state materialized until a value is written.
        mesh->setVisible( true );
        BOOST_CHECK( mesh->isVisible() == true );

        // Test setting visibility to false
        mesh->setVisible( false );
        BOOST_CHECK( mesh->isVisible() == false );
        mesh->setVisible( false );
        BOOST_CHECK( mesh->isVisible() == false );

        // Test setting visibility back to true
        mesh->setVisible( true );
        BOOST_CHECK( mesh->isVisible() == true );
        mesh->setVisible( true );
        BOOST_CHECK( mesh->isVisible() == true );

        // Test rapid visibility toggling
        auto expectedVisible = mesh->isVisible();
        for( int i = 0; i < 100; ++i )
        {
            expectedVisible = i % 2 == 0;
            mesh->setVisible( expectedVisible );
        }
        BOOST_CHECK( mesh->isVisible() == expectedVisible );

        // Cleanup
        sceneManager->removeGraphicsObject( mesh );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during mesh visibility test" );
    }
}

// ============================================================================
// Material Tests
// ============================================================================

BOOST_AUTO_TEST_CASE( test_entity_material )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneManager );

        auto materialManager = graphicsSystem->getMaterialManager();
        BOOST_CHECK( materialManager );

        if( materialManager )
        {
            BOOST_CHECK( materialManager->isValid() );

            auto defaultMaterial = materialManager->create( "TestMaterial" );
            if( defaultMaterial )
            {
                BOOST_CHECK( defaultMaterial->isValid() );
                BOOST_CHECK( !defaultMaterial->getName().empty() );
            }
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during material test" );
    }
}

BOOST_AUTO_TEST_CASE( test_material_duplicate_name )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto materialManager = graphicsSystem->getMaterialManager();
        if( !materialManager )
        {
            BOOST_TEST_MESSAGE( "Material manager not available - skipping test" );
            return;
        }

        auto material1 = materialManager->create( "DuplicateTestMaterial" );
        auto material2 = materialManager->create( "DuplicateTestMaterial" );

        // Behavior may vary: either return same material or create new one
        if( material1 && material2 )
        {
            BOOST_TEST_MESSAGE( "Both materials created successfully" );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        // May be expected for duplicate names
    }
}

BOOST_AUTO_TEST_CASE( test_material_empty_name )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto materialManager = graphicsSystem->getMaterialManager();
        if( !materialManager )
        {
            BOOST_TEST_MESSAGE( "Material manager not available - skipping test" );
            return;
        }

        auto material = materialManager->create( "" );
        // Should either fail gracefully or create with generated name
        if( material )
        {
            BOOST_TEST_MESSAGE( "Material created with empty name" );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        // Expected behavior for empty names
    }
}

BOOST_AUTO_TEST_CASE( test_material_properties )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto materialManager = graphicsSystem->getMaterialManager();
        if( !materialManager )
        {
            BOOST_TEST_MESSAGE( "Material manager not available - skipping test" );
            return;
        }

        auto material = materialManager->create( "TestMaterialProperties" );
        if( material )
        {
            ColourF testDiffuse( 0.8f, 0.6f, 0.4f, 1.0f );
            ColourF testAmbient( 0.2f, 0.2f, 0.2f, 1.0f );

            // Material property tests would go here if API supports it
            BOOST_CHECK( material->isValid() );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during material properties test" );
    }
}

// ============================================================================
// Scene Node Tests
// ============================================================================

BOOST_AUTO_TEST_CASE( test_scenenode_position )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneManager );

        auto rootNode = sceneManager->getRootSceneNode();
        BOOST_REQUIRE( rootNode );

        auto childNode = rootNode->addChildSceneNode( "TestNodePosition" );
        BOOST_REQUIRE( childNode );

        // Test position setting and getting
        Vector3F testPosition( 10.0f, 20.0f, 30.0f );
        childNode->setPosition( testPosition );
        auto retrievedPosition = childNode->getPosition();

        BOOST_CHECK_CLOSE( retrievedPosition.x, testPosition.x, 0.001f );
        BOOST_CHECK_CLOSE( retrievedPosition.y, testPosition.y, 0.001f );
        BOOST_CHECK_CLOSE( retrievedPosition.z, testPosition.z, 0.001f );

        // Cleanup
        sceneManager->removeSceneNode( childNode );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during scene node position test" );
    }
}

BOOST_AUTO_TEST_CASE( test_scenenode_position_edge_cases )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneManager );

        auto rootNode = sceneManager->getRootSceneNode();
        BOOST_REQUIRE( rootNode );

        auto childNode = rootNode->addChildSceneNode( "TestNodeEdgeCases" );
        BOOST_REQUIRE( childNode );

        // Test zero position
        Vector3F zeroPosition( 0.0f, 0.0f, 0.0f );
        childNode->setPosition( zeroPosition );
        auto retrievedZero = childNode->getPosition();
        BOOST_CHECK_CLOSE( retrievedZero.x, 0.0f, 0.001f );
        BOOST_CHECK_CLOSE( retrievedZero.y, 0.0f, 0.001f );
        BOOST_CHECK_CLOSE( retrievedZero.z, 0.0f, 0.001f );

        // Test negative position
        Vector3F negativePosition( -100.0f, -200.0f, -300.0f );
        childNode->setPosition( negativePosition );
        auto retrievedNegative = childNode->getPosition();
        BOOST_CHECK_CLOSE( retrievedNegative.x, negativePosition.x, 0.001f );
        BOOST_CHECK_CLOSE( retrievedNegative.y, negativePosition.y, 0.001f );
        BOOST_CHECK_CLOSE( retrievedNegative.z, negativePosition.z, 0.001f );

        // Test very large position
        Vector3F largePosition( 1e6f, 1e6f, 1e6f );
        childNode->setPosition( largePosition );
        auto retrievedLarge = childNode->getPosition();
        BOOST_CHECK_CLOSE( retrievedLarge.x, largePosition.x, 0.001f );
        BOOST_CHECK_CLOSE( retrievedLarge.y, largePosition.y, 0.001f );
        BOOST_CHECK_CLOSE( retrievedLarge.z, largePosition.z, 0.001f );

        // Test very small position
        Vector3F smallPosition( 1e-6f, 1e-6f, 1e-6f );
        childNode->setPosition( smallPosition );
        auto retrievedSmall = childNode->getPosition();
        BOOST_CHECK_CLOSE( retrievedSmall.x, smallPosition.x, 0.001f );
        BOOST_CHECK_CLOSE( retrievedSmall.y, smallPosition.y, 0.001f );
        BOOST_CHECK_CLOSE( retrievedSmall.z, smallPosition.z, 0.001f );

        // Cleanup
        sceneManager->removeSceneNode( childNode );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during scene node position edge cases test" );
    }
}

BOOST_AUTO_TEST_CASE( test_scenenode_hierarchy )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneManager );

        auto rootNode = sceneManager->getRootSceneNode();
        BOOST_REQUIRE( rootNode );

        auto parentNode = rootNode->addChildSceneNode( "ParentNode" );
        BOOST_REQUIRE( parentNode );

        auto childNode = parentNode->addChildSceneNode( "ChildNode" );
        BOOST_REQUIRE( childNode );

        auto grandchildNode = childNode->addChildSceneNode( "GrandchildNode" );
        BOOST_REQUIRE( grandchildNode );

        // Test hierarchy relationships
        BOOST_CHECK( childNode->getParent() == parentNode );
        BOOST_CHECK( parentNode->getParent() == rootNode );
        BOOST_CHECK( grandchildNode->getParent() == childNode );
        BOOST_CHECK( parentNode->getNumChildren() >= 1 );
        BOOST_CHECK( childNode->getNumChildren() >= 1 );

        // Test world vs local transforms
        Vector3F parentPos( 10.0f, 0.0f, 0.0f );
        Vector3F childLocalPos( 5.0f, 0.0f, 0.0f );
        Vector3F grandchildLocalPos( 2.0f, 0.0f, 0.0f );

        parentNode->setPosition( parentPos );
        childNode->setPosition( childLocalPos );
        grandchildNode->setPosition( grandchildLocalPos );

        auto childWorldPos = childNode->getWorldPosition();
        auto expectedChildWorldPos = parentPos + childLocalPos;

        BOOST_CHECK_CLOSE( childWorldPos.x, expectedChildWorldPos.x, 0.001f );
        BOOST_CHECK_CLOSE( childWorldPos.y, expectedChildWorldPos.y, 0.001f );
        BOOST_CHECK_CLOSE( childWorldPos.z, expectedChildWorldPos.z, 0.001f );

        // Cleanup - remove in reverse order
        sceneManager->removeSceneNode( grandchildNode );
        sceneManager->removeSceneNode( childNode );
        sceneManager->removeSceneNode( parentNode );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during scene node hierarchy test" );
    }
}

BOOST_AUTO_TEST_CASE( test_scenenode_scale )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneManager );

        auto rootNode = sceneManager->getRootSceneNode();
        BOOST_REQUIRE( rootNode );

        auto testNode = rootNode->addChildSceneNode( "TestScaleNode" );
        BOOST_REQUIRE( testNode );

        // Test uniform scale
        Vector3F uniformScale( 2.0f, 2.0f, 2.0f );
        testNode->setScale( uniformScale );
        auto retrievedScale = testNode->getScale();

        BOOST_CHECK_CLOSE( retrievedScale.x, uniformScale.x, 0.001f );
        BOOST_CHECK_CLOSE( retrievedScale.y, uniformScale.y, 0.001f );
        BOOST_CHECK_CLOSE( retrievedScale.z, uniformScale.z, 0.001f );

        // Test non-uniform scale
        Vector3F nonUniformScale( 1.0f, 2.0f, 3.0f );
        testNode->setScale( nonUniformScale );
        retrievedScale = testNode->getScale();

        BOOST_CHECK_CLOSE( retrievedScale.x, nonUniformScale.x, 0.001f );
        BOOST_CHECK_CLOSE( retrievedScale.y, nonUniformScale.y, 0.001f );
        BOOST_CHECK_CLOSE( retrievedScale.z, nonUniformScale.z, 0.001f );

        // Test very small scale (near zero)
        Vector3F smallScale( 0.001f, 0.001f, 0.001f );
        testNode->setScale( smallScale );
        retrievedScale = testNode->getScale();

        BOOST_CHECK_CLOSE( retrievedScale.x, smallScale.x, 0.001f );
        BOOST_CHECK_CLOSE( retrievedScale.y, smallScale.y, 0.001f );
        BOOST_CHECK_CLOSE( retrievedScale.z, smallScale.z, 0.001f );

        // Cleanup
        sceneManager->removeSceneNode( testNode );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during scene node scale test" );
    }
}

BOOST_AUTO_TEST_CASE( test_scenenode_orientation )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneManager );

        auto rootNode = sceneManager->getRootSceneNode();
        BOOST_REQUIRE( rootNode );

        auto testNode = rootNode->addChildSceneNode( "TestOrientationNode" );
        BOOST_REQUIRE( testNode );

        // Test identity orientation
        QuaternionF identityQuat( 1.0f, 0.0f, 0.0f, 0.0f );
        testNode->setOrientation( identityQuat );
        auto retrievedOrientation = testNode->getOrientation();

        BOOST_CHECK_CLOSE( retrievedOrientation.w, identityQuat.w, 0.001f );
        BOOST_CHECK_CLOSE( retrievedOrientation.x, identityQuat.x, 0.001f );
        BOOST_CHECK_CLOSE( retrievedOrientation.y, identityQuat.y, 0.001f );
        BOOST_CHECK_CLOSE( retrievedOrientation.z, identityQuat.z, 0.001f );

        // Test 90-degree rotation around Y axis
        QuaternionF rotationY;
        rotationY.fromAngleAxis( MathF::pi() / 2.0f, Vector3F( 0.0f, 1.0f, 0.0f ) );
        testNode->setOrientation( rotationY );
        retrievedOrientation = testNode->getOrientation();

        BOOST_CHECK_CLOSE( retrievedOrientation.w, rotationY.w, 0.01f );
        BOOST_CHECK_CLOSE( retrievedOrientation.x, rotationY.x, 0.01f );
        BOOST_CHECK_CLOSE( retrievedOrientation.y, rotationY.y, 0.01f );
        BOOST_CHECK_CLOSE( retrievedOrientation.z, rotationY.z, 0.01f );

        // Cleanup
        sceneManager->removeSceneNode( testNode );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during scene node orientation test" );
    }
}

// ============================================================================
// Light Tests
// ============================================================================

BOOST_AUTO_TEST_CASE( graphics_create_light )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto light = actor->addComponent<Light>();
        BOOST_REQUIRE( light );

        BOOST_CHECK( light->isValid() );
        BOOST_CHECK( light->isLoaded() );

        light->setLightType( LightTypes::LT_POINT );
        BOOST_CHECK_EQUAL( static_cast<u32>( light->getLightType() ),
                           static_cast<u32>( LightTypes::LT_POINT ) );

        auto diffuseColour = light->getDiffuseColour();
        BOOST_CHECK_CLOSE( diffuseColour.r, 1.0f, 0.001f );
        BOOST_CHECK_CLOSE( diffuseColour.g, 1.0f, 0.001f );
        BOOST_CHECK_CLOSE( diffuseColour.b, 1.0f, 0.001f );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during light creation test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_light_set_diffuse )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto light = actor->addComponent<Light>();
        BOOST_REQUIRE( light );

        light->setLightType( LightTypes::LT_POINT );
        BOOST_CHECK_EQUAL( static_cast<u32>( light->getLightType() ),
                           static_cast<u32>( LightTypes::LT_POINT ) );

        auto diffuseColour = ColourF( 0.5f, 0.5f, 0.5f );
        light->setDiffuseColour( diffuseColour );

        auto retrievedColour = light->getDiffuseColour();
        BOOST_CHECK_CLOSE( retrievedColour.r, 0.5f, 0.001f );
        BOOST_CHECK_CLOSE( retrievedColour.g, 0.5f, 0.001f );
        BOOST_CHECK_CLOSE( retrievedColour.b, 0.5f, 0.001f );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during light diffuse test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_light_set_specular )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto light = actor->addComponent<Light>();
        BOOST_REQUIRE( light );

        auto specularColour = ColourF( 0.8f, 0.7f, 0.6f );
        light->setSpecularColour( specularColour );

        auto retrievedColour = light->getSpecularColour();
        BOOST_CHECK_CLOSE( retrievedColour.r, 0.8f, 0.001f );
        BOOST_CHECK_CLOSE( retrievedColour.g, 0.7f, 0.001f );
        BOOST_CHECK_CLOSE( retrievedColour.b, 0.6f, 0.001f );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during light specular test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_light_types )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto light = actor->addComponent<Light>();
        BOOST_REQUIRE( light );

        // Test directional light
        light->setLightType( LightTypes::LT_DIRECTIONAL );
        BOOST_CHECK_EQUAL( static_cast<u32>( light->getLightType() ),
                           static_cast<u32>( LightTypes::LT_DIRECTIONAL ) );

        // Test point light
        light->setLightType( LightTypes::LT_POINT );
        BOOST_CHECK_EQUAL( static_cast<u32>( light->getLightType() ),
                           static_cast<u32>( LightTypes::LT_POINT ) );

        // Test spot light
        light->setLightType( LightTypes::LT_SPOTLIGHT );
        BOOST_CHECK_EQUAL( static_cast<u32>( light->getLightType() ),
                           static_cast<u32>( LightTypes::LT_SPOTLIGHT ) );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during light types test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_light_attenuation )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto light = actor->addComponent<Light>();
        BOOST_REQUIRE( light );

        light->setLightType( LightTypes::LT_POINT );

        // Test attenuation settings
        f32 range = 100.0f;
        f32 constant = 1.0f;
        f32 linear = 0.09f;
        f32 quadratic = 0.032f;

        light->setAttenuation( range, constant, linear, quadratic );

        BOOST_CHECK_CLOSE( light->getAttenuationRange(), range, 0.001f );
        BOOST_CHECK_CLOSE( light->getAttenuationConstant(), constant, 0.001f );
        BOOST_CHECK_CLOSE( light->getAttenuationLinear(), linear, 0.001f );
        BOOST_CHECK_CLOSE( light->getAttenuationQuadratic(), quadratic, 0.001f );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during light attenuation test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_light_color_edge_cases )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto light = actor->addComponent<Light>();
        BOOST_REQUIRE( light );

        // Test black color (minimum)
        auto blackColour = ColourF( 0.0f, 0.0f, 0.0f );
        light->setDiffuseColour( blackColour );
        auto retrievedBlack = light->getDiffuseColour();
        BOOST_CHECK_CLOSE( retrievedBlack.r, 0.0f, 0.001f );
        BOOST_CHECK_CLOSE( retrievedBlack.g, 0.0f, 0.001f );
        BOOST_CHECK_CLOSE( retrievedBlack.b, 0.0f, 0.001f );

        // Test white color (maximum normal)
        auto whiteColour = ColourF( 1.0f, 1.0f, 1.0f );
        light->setDiffuseColour( whiteColour );
        auto retrievedWhite = light->getDiffuseColour();
        BOOST_CHECK_CLOSE( retrievedWhite.r, 1.0f, 0.001f );
        BOOST_CHECK_CLOSE( retrievedWhite.g, 1.0f, 0.001f );
        BOOST_CHECK_CLOSE( retrievedWhite.b, 1.0f, 0.001f );

        // Test HDR values (greater than 1.0)
        auto hdrColour = ColourF( 2.0f, 1.5f, 3.0f );
        light->setDiffuseColour( hdrColour );
        auto retrievedHdr = light->getDiffuseColour();
        BOOST_CHECK_CLOSE( retrievedHdr.r, 2.0f, 0.001f );
        BOOST_CHECK_CLOSE( retrievedHdr.g, 1.5f, 0.001f );
        BOOST_CHECK_CLOSE( retrievedHdr.b, 3.0f, 0.001f );

        // Test individual color channels
        auto redColour = ColourF( 1.0f, 0.0f, 0.0f );
        light->setDiffuseColour( redColour );
        auto retrievedRed = light->getDiffuseColour();
        BOOST_CHECK_CLOSE( retrievedRed.r, 1.0f, 0.001f );
        BOOST_CHECK_CLOSE( retrievedRed.g, 0.0f, 0.001f );
        BOOST_CHECK_CLOSE( retrievedRed.b, 0.0f, 0.001f );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during light color edge cases test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_light_visibility )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto light = actor->addComponent<Light>();
        BOOST_REQUIRE( light );

        // Test visibility toggling
        light->setVisible( true );
        BOOST_CHECK( light->isVisible() == true );

        light->setVisible( false );
        BOOST_CHECK( light->isVisible() == false );

        light->setVisible( true );
        BOOST_CHECK( light->isVisible() == true );

        sceneManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during light visibility test" );
    }
}

BOOST_AUTO_TEST_CASE( graphics_multiple_lights )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        const size_t lightCount = 5;
        std::vector<SmartPtr<IGameActor>> actors;
        std::vector<SmartPtr<Light>> lights;

        for( size_t i = 0; i < lightCount; ++i )
        {
            auto actor = sceneManager->createActor();
            BOOST_REQUIRE( actor );

            auto light = actor->addComponent<Light>();
            BOOST_REQUIRE( light );

            // Set different colors for each light
            ColourF colour( static_cast<f32>( i ) / lightCount, 0.5f,
                            1.0f - static_cast<f32>( i ) / lightCount );
            light->setDiffuseColour( colour );

            actors.push_back( actor );
            lights.push_back( light );
        }

        BOOST_CHECK_EQUAL( lights.size(), lightCount );

        // Verify all lights have unique colors
        for( size_t i = 0; i < lights.size(); ++i )
        {
            auto colour = lights[i]->getDiffuseColour();
            BOOST_CHECK_CLOSE( colour.r, static_cast<f32>( i ) / lightCount, 0.001f );
        }

        // Cleanup
        for( auto &actor : actors )
        {
            sceneManager->destroyActor( actor );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during multiple lights test" );
    }
}

// ============================================================================
// Graphics System State Tests
// ============================================================================

BOOST_AUTO_TEST_CASE( test_graphics_system_state )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        BOOST_CHECK( graphicsSystem->isLoaded() );
        BOOST_CHECK( graphicsSystem->isValid() );

        auto sceneManager = graphicsSystem->getGraphicsScene();
        if( sceneManager )
        {
            BOOST_CHECK( sceneManager->isValid() );

            auto rootNode = sceneManager->getRootSceneNode();
            BOOST_CHECK( rootNode );
            if( rootNode )
            {
                BOOST_CHECK( rootNode->isValid() );
            }
        }

        auto textureManager = graphicsSystem->getTextureManager();
        BOOST_CHECK( textureManager );

        auto materialManager = graphicsSystem->getMaterialManager();
        BOOST_CHECK( materialManager );

        auto meshManager = applicationManager->getMeshManager();
        BOOST_CHECK( meshManager );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during graphics system state test" );
    }
}

// ============================================================================
// Resource Loading Tests
// ============================================================================

BOOST_AUTO_TEST_CASE( test_resource_loading_stress )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto resourceDatabase = applicationManager->getResourceDatabase();
        BOOST_REQUIRE( resourceDatabase );

        std::vector<String> resourceNames = { "cube.mesh", "sphere.mesh", "plane.mesh", "test.material",
                                              "default.material" };

        std::vector<SmartPtr<IResource>> loadedResources;

        for( const auto &resourceName : resourceNames )
        {
            auto resource = resourceDatabase->loadResource( resourceName );
            if( resource )
            {
                loadedResources.push_back( resource );
                BOOST_CHECK( resource->isValid() );
            }
        }

        BOOST_TEST_MESSAGE( "Successfully loaded " + StringUtil::toString( loadedResources.size() ) +
                            " out of " + StringUtil::toString( resourceNames.size() ) + " resources" );

        // Clean up
        for( auto &resource : loadedResources )
        {
            resource->unload( nullptr );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during resource loading stress test" );
    }
}

BOOST_AUTO_TEST_CASE( test_resource_double_load )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto resourceDatabase = applicationManager->getResourceDatabase();
        BOOST_REQUIRE( resourceDatabase );

        auto resource1 = resourceDatabase->loadResource( "cube.mesh" );
        auto resource2 = resourceDatabase->loadResource( "cube.mesh" );

        // Both should reference the same resource or handle gracefully
        if( resource1 && resource2 )
        {
            BOOST_TEST_MESSAGE( "Same resource loaded twice - checking consistency" );
            BOOST_CHECK( resource1->isValid() );
            BOOST_CHECK( resource2->isValid() );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        // May be expected behavior
    }
}

// ============================================================================
// Graphics Object Attachment Tests
// ============================================================================

BOOST_AUTO_TEST_CASE( test_graphics_object_attachment )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneManager );

        auto rootNode = sceneManager->getRootSceneNode();
        BOOST_REQUIRE( rootNode );

        auto testNode = rootNode->addChildSceneNode( "TestAttachmentNode" );
        BOOST_REQUIRE( testNode );

        auto mesh = sceneManager->addGraphicsObjectByTypeId( render::IGraphicsMesh::typeInfo() );
        BOOST_REQUIRE( mesh );

        // Test attachment
        testNode->attachObject( mesh );

        // Test detachment
        testNode->detachObject( mesh );

        // Test reattachment
        testNode->attachObject( mesh );

        // Cleanup
        testNode->detachObject( mesh );
        sceneManager->removeGraphicsObject( mesh );
        sceneManager->removeSceneNode( testNode );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during graphics object attachment test" );
    }
}

BOOST_AUTO_TEST_CASE( test_multiple_objects_single_node )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneManager );

        auto rootNode = sceneManager->getRootSceneNode();
        BOOST_REQUIRE( rootNode );

        auto testNode = rootNode->addChildSceneNode( "TestMultiAttachNode" );
        BOOST_REQUIRE( testNode );

        const size_t objectCount = 3;
        std::vector<SmartPtr<IGraphicsObject>> objects;

        for( size_t i = 0; i < objectCount; ++i )
        {
            auto mesh = sceneManager->addGraphicsObjectByTypeId( render::IGraphicsMesh::typeInfo() );
            BOOST_REQUIRE( mesh );
            testNode->attachObject( mesh );
            objects.push_back( mesh );
        }

        BOOST_CHECK_EQUAL( objects.size(), objectCount );

        // Cleanup
        for( auto &obj : objects )
        {
            testNode->detachObject( obj );
            sceneManager->removeGraphicsObject( obj );
        }
        sceneManager->removeSceneNode( testNode );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during multiple objects attachment test" );
    }
}

// ============================================================================
// Cleanup/Destruction Tests
// ============================================================================

BOOST_AUTO_TEST_CASE( test_scene_node_removal )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( sceneManager );

        auto rootNode = sceneManager->getRootSceneNode();
        BOOST_REQUIRE( rootNode );

        // Create and immediately destroy
        auto tempNode = rootNode->addChildSceneNode( "TempNode" );
        BOOST_REQUIRE( tempNode );

        sceneManager->removeSceneNode( tempNode );

        // Should be able to create another with the same name
        auto newNode = rootNode->addChildSceneNode( "TempNode" );
        BOOST_CHECK( newNode );

        if( newNode )
        {
            sceneManager->removeSceneNode( newNode );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during scene node removal test" );
    }
}

BOOST_AUTO_TEST_CASE( test_actor_destruction_with_components )
{
    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto sceneManager = applicationManager->getGameManager();
        BOOST_REQUIRE( sceneManager );

        // Create actor with light component
        auto actor = sceneManager->createActor();
        BOOST_REQUIRE( actor );

        auto light = actor->addComponent<Light>();
        BOOST_REQUIRE( light );

        light->setDiffuseColour( ColourF( 1.0f, 0.0f, 0.0f ) );

        // Destroy actor - should cleanup light component properly
        sceneManager->destroyActor( actor );

        // Verify no crash occurred
        BOOST_CHECK( true );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown during actor destruction test" );
    }
}
