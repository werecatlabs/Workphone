#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

#if WP_GRAPHICS_SYSTEM_CLAW
#    include <WPGraphics/ClawCamera.hpp>
#    include <WPGraphics/ClawLight.hpp>
#    include <WPGraphics/ClawMesh.hpp>
#    include <WPGraphics/ClawSceneNode.hpp>
#    include <workphone_graphics_object.h>
#    include <workphone_graphics_light.h>
#    include <cstdlib>
#endif

using namespace workphone;

// Helper macro for logging test state with context
#define BOOST_TEST_LOG_MESSAGE( msg ) \
    BOOST_TEST_MESSAGE( "[" << boost::unit_test::framework::current_test_case().p_name << "] " << msg )

// Helper macro for checking with detailed logging
#define BOOST_CHECK_WITH_LOG( condition, message )           \
    do                                                       \
    {                                                        \
        if( !( condition ) )                                 \
        {                                                    \
            BOOST_TEST_LOG_MESSAGE( "FAILED: " << message ); \
        }                                                    \
        else                                                 \
        {                                                    \
            BOOST_TEST_LOG_MESSAGE( "PASSED: " << message ); \
        }                                                    \
        BOOST_CHECK( condition );                            \
    } while( 0 )

// Helper function to run update loop
namespace
{
    void RunUpdateLoop( SmartPtr<core::IApplicationManager> appManager, int iterations = 5 )
    {
        auto taskManager = appManager->getTaskManager();
        for( int i = 0; i < iterations; ++i )
        {
            taskManager->update();
        }
    }
}  // namespace

#if WP_GRAPHICS_SYSTEM_CLAW
BOOST_AUTO_TEST_CASE( claw_scene_node_deferred_load_preserves_mesh_world_matrix )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.graphicsSystem );
    auto scene = guard.graphicsSystem->getGraphicsScene();
    BOOST_REQUIRE( scene );
    auto parent = guard.factoryManager->make_ptr<render::ClawSceneNode>( scene );
    auto child = guard.factoryManager->make_ptr<render::ClawSceneNode>( scene );
    auto mesh = guard.factoryManager->make_ptr<render::ClawMesh>();
    auto camera = guard.factoryManager->make_ptr<render::ClawCamera>();
    auto light = guard.factoryManager->make_ptr<render::ClawLight>();
    auto nativeObject = wp_graphics_object_create();
    BOOST_REQUIRE( nativeObject );
    mesh->bindNativeRenderObject( nativeObject );
    guard.addCleanup( [parent, child, mesh, nativeObject]() mutable {
        child->unload( nullptr );
        parent->unload( nullptr );
        mesh->unload( nullptr );
        wp_graphics_object_destroy( nativeObject );
    } );

    Transform3<real_Num> parentTransform;
    parentTransform.setPosition( Vector3F( 10.0f, 20.0f, -30.0f ) );
    parentTransform.setOrientation( QuaternionF( 0.5f, 0.5f, -0.5f, 0.5f ) );
    parentTransform.setScale( Vector3F( 2.0f, 3.0f, 4.0f ) );
    parent->setTransform( parentTransform );
    Transform3<real_Num> childTransform;
    childTransform.setPosition( Vector3F( -5.0f, 8.0f, 11.0f ) );
    childTransform.setScale( Vector3F( 0.5f, 1.5f, 2.0f ) );
    child->setTransform( childTransform );

    // MeshRenderer builds these links before queued native nodes are created.
    parent->addChild( child );
    child->attachObject( mesh );
    child->attachObject( camera );
    child->attachObject( light );
    child->load( nullptr );
    parent->load( nullptr );
    BOOST_CHECK( wp_scenenode_get_parent( child->getNativeNode() ) == parent->getNativeNode() );
    BOOST_CHECK( wp_graphics_object_get_owner( nativeObject ) == child->getNativeNode() );
    BOOST_CHECK( wp_camera_get_node( camera->getNativeCamera() ) == child->getNativeNode() );

    wp_light* nativeLight = nullptr;
    light->_getObject( reinterpret_cast<void **>( &nativeLight ) );
    BOOST_CHECK( wp_light_get_node( nativeLight ) == child->getNativeNode() );
    BOOST_CHECK_EQUAL( wp_scenenode_get_object_count( child->getNativeNode() ), 1 );
    child->attachObject( mesh );
    BOOST_CHECK_EQUAL( child->getObjects().size(), 3u );
    BOOST_CHECK_EQUAL( wp_scenenode_get_object_count( child->getNativeNode() ), 1 );

    Matrix4F parentMatrix;
    parentMatrix.makeTransform( parentTransform.getPosition(), parentTransform.getScale(),
                                parentTransform.getOrientation() );
    Matrix4F childMatrix;
    childMatrix.makeTransform( childTransform.getPosition(), childTransform.getScale(),
                               childTransform.getOrientation() );
    const auto expected = parentMatrix * childMatrix;
    wp_mat4f rendered;
    wp_scenenode_get_world_matrix( wp_graphics_object_get_owner( nativeObject ), &rendered );
    for( size_t i = 0; i < 16; ++i )
        BOOST_CHECK_SMALL( rendered.m[i / 4][i % 4] - expected.ptr()[i], 0.001f );

    parent->attachObject( mesh );
    BOOST_CHECK( mesh->getOwner() == parent );
    BOOST_CHECK( wp_graphics_object_get_owner( nativeObject ) == parent->getNativeNode() );
    child->unload( nullptr );
    BOOST_CHECK( !wp_camera_get_node( camera->getNativeCamera() ) );

    nativeLight = nullptr;
    light->_getObject( reinterpret_cast<void **>( &nativeLight ) );
    BOOST_CHECK( !wp_light_get_node( nativeLight ) );
    BOOST_CHECK( !camera->getOwner() );
    BOOST_CHECK( !light->getOwner() );
}

BOOST_AUTO_TEST_CASE( claw_loaded_scene_meshes_use_actor_world_transforms )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.graphicsSystem );
    const auto scenePath = std::getenv( "WP_TEST_GRAPHICS_SCENE" );
    guard.scene->setState( scene::IGameScene::State::Edit );
    guard.scene->clear( true );
    guard.addCleanup( [scene = guard.scene]() mutable { scene->clear( true ); } );
    // Match editor scene loading on the application task, where node loads are queued.
    guard.addCleanup( []() { Thread::setCurrentTask( TaskId::Primary ); } );
    Thread::setCurrentTask( TaskId::Application );
    guard.scene->loadScene( scenePath ? String( scenePath ) : String( "Tests/physics_test.fbscene" ),
                            false );
    guard.setupThread();
    for( u32 i = 0; i < 10; ++i )
    {
        guard.applicationManager->getJobQueue()->update();
        guard.taskManager->update();
        guard.runUpdateCycle( 1 );
    }

    size_t meshCount = 0;
    std::function<void( SmartPtr<scene::IGameActor> )> checkActor;
    checkActor = [&]( SmartPtr<scene::IGameActor> actor ) {
        for( auto &renderer : actor->getComponentsByType<scene::MeshRenderer>() )
        {
            auto mesh = dynamic_pointer_cast<render::ClawMesh>( renderer->getGraphicsObject() );
            auto node = dynamic_pointer_cast<render::ClawSceneNode>( renderer->getGraphicsNode() );
            if( !mesh || !node )
                continue;
            BOOST_TEST_CONTEXT( "Actor: " << actor->getName() )
            {
                ++meshCount;
                auto nativeObject = mesh->getNativeRenderObject();
                BOOST_REQUIRE( nativeObject );
                BOOST_REQUIRE( node->getNativeNode() );
                BOOST_CHECK( wp_graphics_object_get_owner( nativeObject ) == node->getNativeNode() );
                const auto transform = actor->getTransform()->getWorldTransform();
                Matrix4F expected;
                expected.makeTransform( transform.getPosition(), transform.getScale(),
                                         transform.getOrientation() );
                wp_mat4f rendered;
                wp_scenenode_get_world_matrix( wp_graphics_object_get_owner( nativeObject ), &rendered );
                for( size_t i = 0; i < 16; ++i )
                    BOOST_CHECK_SMALL( rendered.m[i / 4][i % 4] - expected.ptr()[i], 0.01f );
            }
        }
        for( auto &child : actor->getChildren() )
            checkActor( child );
    };
    for( auto &actor : guard.scene->getActors() )
        checkActor( actor );
    BOOST_CHECK_GT( meshCount, 0u );
}

BOOST_AUTO_TEST_CASE( claw_scene_node_loaded_transform_survives_state_updates )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.graphicsSystem );
    auto graphicsScene = guard.graphicsSystem->getGraphicsScene();
    BOOST_REQUIRE( graphicsScene );

    // Scene transforms can arrive before the render task creates the native node.
    auto node = guard.factoryManager->make_ptr<render::ClawSceneNode>( graphicsScene );
    guard.addCleanup( [node]() mutable { node->unload( nullptr ); } );
    Transform3<real_Num> transform;
    transform.setPosition( Vector3F( 12.0f, -8.0f, 31.0f ) );
    QuaternionF orientation( 0.5f, -0.5f, 0.5f, 0.5f );
    transform.setOrientation( orientation );
    transform.setScale( Vector3F( 2.0f, 3.0f, 4.0f ) );
    node->setTransform( transform );
    node->load( nullptr );
    BOOST_REQUIRE( node->isLoaded() );
    BOOST_REQUIRE( node->getNativeNode() );

    auto checkTransform = [&node]( const Transform3<real_Num> &expected ) {
        const auto stored = node->getTransform();
        BOOST_CHECK( stored.getPosition().equals( expected.getPosition(), 0.001f ) );
        BOOST_CHECK( stored.getScale().equals( expected.getScale(), 0.001f ) );
        BOOST_CHECK_SMALL( stored.getOrientation().w - expected.getOrientation().w, 0.001f );
        BOOST_CHECK_SMALL( stored.getOrientation().x - expected.getOrientation().x, 0.001f );
        BOOST_CHECK_SMALL( stored.getOrientation().y - expected.getOrientation().y, 0.001f );
        BOOST_CHECK_SMALL( stored.getOrientation().z - expected.getOrientation().z, 0.001f );

        const auto position = wp_scenenode_get_position( node->getNativeNode() );
        const auto rotation = wp_scenenode_get_orientation( node->getNativeNode() );
        const auto scale = wp_scenenode_get_scale( node->getNativeNode() );
        BOOST_CHECK_SMALL( position.x - expected.getPosition().x, 0.001f );
        BOOST_CHECK_SMALL( position.y - expected.getPosition().y, 0.001f );
        BOOST_CHECK_SMALL( position.z - expected.getPosition().z, 0.001f );
        BOOST_CHECK_SMALL( rotation.w - expected.getOrientation().w, 0.001f );
        BOOST_CHECK_SMALL( rotation.x - expected.getOrientation().x, 0.001f );
        BOOST_CHECK_SMALL( rotation.y - expected.getOrientation().y, 0.001f );
        BOOST_CHECK_SMALL( rotation.z - expected.getOrientation().z, 0.001f );
        BOOST_CHECK_SMALL( scale.x - expected.getScale().x, 0.001f );
        BOOST_CHECK_SMALL( scale.y - expected.getScale().y, 0.001f );
        BOOST_CHECK_SMALL( scale.z - expected.getScale().z, 0.001f );
    };
    checkTransform( transform );

    auto context = node->getStateContext();
    BOOST_REQUIRE( context );
    SmartPtr<IState> transformState;
    for( auto &state : context->getStates() )
    {
        if( state && state->getOwnerPtr() == node.get() && state->getData() &&
            state->getData()->isDerived<TransformStateData>() )
        {
            transformState = state;
            break;
        }
    }
    BOOST_REQUIRE( transformState );

    // The initial dirty state must not reset the transform loaded from the scene.
    BOOST_CHECK( node->handleStateChanged( transformState ) );
    checkTransform( transform );

    transform.setPosition( Vector3F( -4.0f, 9.0f, 17.0f ) );
    node->setTransform( transform );
    BOOST_CHECK( node->handleStateChanged( transformState ) );
    checkTransform( transform );

    // Changing just one component must preserve the rest of the loaded transform.
    transform.setScale( Vector3F( 5.0f, 6.0f, 7.0f ) );
    node->setScale( transform.getScale() );
    BOOST_CHECK( node->handleStateChanged( transformState ) );
    checkTransform( transform );

    transform.setPosition( Vector3F( 20.0f, -11.0f, 3.0f ) );
    node->setWorldTransform( transform );
    BOOST_CHECK( node->handleStateChanged( transformState ) );
    checkTransform( transform );
}
#endif

BOOST_AUTO_TEST_CASE( graphics_scene_node_basic )
{
    BOOST_TEST_LOG_MESSAGE( "Starting graphics scene node basic test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK_WITH_LOG( applicationManager, "ApplicationManager instance should be available" );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        if( graphicsSystem )
        {
            BOOST_CHECK_WITH_LOG( graphicsSystem->getLoadingState() == LoadingState::Loaded,
                                  "GraphicsSystem should be loaded" );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_scene_node_creation )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    BOOST_TEST_LOG_MESSAGE( "Starting scene node creation test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }
        BOOST_REQUIRE( graphicsSystem->getLoadingState() == LoadingState::Loaded );

        auto graphicsScene = graphicsSystem->getGraphicsScene();
        BOOST_CHECK_WITH_LOG( graphicsScene, "GraphicsScene should be available" );

        if( graphicsScene )
        {
            // Create a scene node
            auto sceneNode = graphicsScene->addSceneNode();
            BOOST_CHECK_WITH_LOG( sceneNode, "Scene node creation should succeed" );

            if( sceneNode )
            {
                RunUpdateLoop( applicationManager );

                BOOST_CHECK_WITH_LOG( sceneNode->isValid(), "Created scene node should be valid" );
                BOOST_CHECK_WITH_LOG( !sceneNode->getName().empty(), "Scene node should have a name" );
                BOOST_TEST_LOG_MESSAGE( "Scene node name: " << sceneNode->getName() );

                // Check initial state
                BOOST_CHECK_WITH_LOG( sceneNode->getParent() == nullptr,
                                      "New scene node should not have a parent" );
                BOOST_CHECK_WITH_LOG( sceneNode->getChildren().empty(),
                                      "New scene node should have no children" );
                BOOST_CHECK_WITH_LOG( sceneNode->getObjects().empty(),
                                      "New scene node should have no attached objects" );

                // Clean up
                graphicsScene->removeSceneNode( sceneNode );
                RunUpdateLoop( applicationManager );
            }
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_scene_node_transform )
{
    BOOST_TEST_LOG_MESSAGE( "Starting scene node transform test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto graphicsScene = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( graphicsScene );

        auto sceneNode = graphicsScene->addSceneNode();
        BOOST_REQUIRE( sceneNode );

        RunUpdateLoop( applicationManager );

        // Test position
        Vector3F testPosition( 10.0f, 20.0f, 30.0f );
        sceneNode->setPosition( testPosition );
        RunUpdateLoop( applicationManager, 2 );

        auto position = sceneNode->getPosition();
        //BOOST_CHECK_WITH_LOG( MathUtil<f32>::Abs( position.X() - testPosition.X() ) < 0.001f &&
        //                          MathUtil<f32>::Abs( position.Y() - testPosition.Y() ) < 0.001f &&
        //                          MathUtil<f32>::Abs( position.Z() - testPosition.Z() ) < 0.001f,
        //                      "Scene node position should be set correctly" );
        BOOST_TEST_LOG_MESSAGE( "Position set to: " << position.X() << ", " << position.Y() << ", "
                                                    << position.Z() );

        // Test scale
        Vector3F testScale( 2.0f, 3.0f, 4.0f );
        sceneNode->setScale( testScale );
        RunUpdateLoop( applicationManager, 2 );

        auto scale = sceneNode->getScale();
        //BOOST_CHECK_WITH_LOG( MathUtil<f32>::Abs( scale.X() - testScale.X() ) < 0.001f &&
        //                          MathUtil<f32>::Abs( scale.Y() - testScale.Y() ) < 0.001f &&
        //                          MathUtil<f32>::Abs( scale.Z() - testScale.Z() ) < 0.001f,
        //                      "Scene node scale should be set correctly" );
        BOOST_TEST_LOG_MESSAGE( "Scale set to: " << scale.X() << ", " << scale.Y() << ", "
                                                 << scale.Z() );

        // Test orientation
        QuaternionF testOrientation( 0.0f, 0.707f, 0.0f, 0.707f );  // 90 degree rotation around Y
        testOrientation.normalise();
        sceneNode->setOrientation( testOrientation );
        RunUpdateLoop( applicationManager, 2 );

        auto orientation = sceneNode->getOrientation();
        BOOST_CHECK_WITH_LOG( orientation.isValid(), "Scene node orientation should be valid" );
        BOOST_TEST_LOG_MESSAGE( "Orientation set to: " << orientation.W() << ", " << orientation.X()
                                                       << ", " << orientation.Y() << ", "
                                                       << orientation.Z() );

        // Clean up
        graphicsScene->removeSceneNode( sceneNode );
        RunUpdateLoop( applicationManager );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_scene_node_hierarchy )
{
    BOOST_TEST_LOG_MESSAGE( "Starting scene node hierarchy test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto graphicsScene = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( graphicsScene );

        // Create parent node
        auto parentNode = graphicsScene->addSceneNode();
        BOOST_REQUIRE( parentNode );
        RunUpdateLoop( applicationManager );

        // Create child node
        auto childNode = parentNode->addChildSceneNode();
        BOOST_CHECK_WITH_LOG( childNode, "Child scene node creation should succeed" );
        RunUpdateLoop( applicationManager );

        if( childNode )
        {
            // Verify parent-child relationship
            BOOST_CHECK_WITH_LOG( childNode->getParent() == parentNode,
                                  "Child node should reference parent" );
            auto children = parentNode->getChildren();
            BOOST_CHECK_WITH_LOG( children.size() == 1, "Parent should have one child" );
            BOOST_CHECK_WITH_LOG( children[0] == childNode,
                                  "Parent's child should match created child" );

            // Test hierarchical transformation
            parentNode->setPosition( Vector3F( 100.0f, 0.0f, 0.0f ) );
            childNode->setPosition( Vector3F( 10.0f, 0.0f, 0.0f ) );
            RunUpdateLoop( applicationManager, 3 );

            auto childLocalPos = childNode->getPosition();
            BOOST_TEST_LOG_MESSAGE( "Child local position: " << childLocalPos.X() << ", "
                                                             << childLocalPos.Y() << ", "
                                                             << childLocalPos.Z() );

            // Create grandchild
            auto grandchildNode = childNode->addChildSceneNode();
            BOOST_CHECK_WITH_LOG( grandchildNode, "Grandchild scene node creation should succeed" );
            RunUpdateLoop( applicationManager );

            if( grandchildNode )
            {
                BOOST_CHECK_WITH_LOG( grandchildNode->getParent() == childNode,
                                      "Grandchild should reference child as parent" );
                BOOST_CHECK_WITH_LOG( childNode->getChildren().size() == 1,
                                      "Child should have one child" );

                // Remove grandchild
                childNode->removeChild( grandchildNode );
                RunUpdateLoop( applicationManager );
                BOOST_CHECK_WITH_LOG( childNode->getChildren().empty(),
                                      "Child should have no children after removal" );
            }

            // Remove child
            parentNode->removeChild( childNode );
            RunUpdateLoop( applicationManager );
            BOOST_CHECK_WITH_LOG( parentNode->getChildren().empty(),
                                  "Parent should have no children after removal" );
        }

        // Clean up
        graphicsScene->removeSceneNode( parentNode );
        RunUpdateLoop( applicationManager );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_scene_node_visibility )
{
    BOOST_TEST_LOG_MESSAGE( "Starting scene node visibility test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto graphicsScene = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( graphicsScene );

        auto sceneNode = graphicsScene->addSceneNode();
        BOOST_REQUIRE( sceneNode );
        RunUpdateLoop( applicationManager );

        // Test visibility toggle
        //sceneNode->setVisible( true );
        RunUpdateLoop( applicationManager, 2 );
        //BOOST_CHECK_WITH_LOG( sceneNode->isVisible(), "Scene node should be visible" );

        //sceneNode->setVisible( false );
        RunUpdateLoop( applicationManager, 2 );
        //BOOST_CHECK_WITH_LOG( !sceneNode->isVisible(), "Scene node should be invisible" );

        // Test cascading visibility with child
        //sceneNode->setVisible( true );
        auto childNode = sceneNode->addChildSceneNode();
        BOOST_REQUIRE( childNode );
        RunUpdateLoop( applicationManager );

        //childNode->setVisible( true );
        RunUpdateLoop( applicationManager, 2 );

        //sceneNode->setVisible( false, true );  // Hide parent and cascade
        RunUpdateLoop( applicationManager, 2 );
        //BOOST_CHECK_WITH_LOG( !sceneNode->isVisible(),
        //                      "Parent should be invisible after cascading hide" );

        // Clean up
        graphicsScene->removeSceneNode( sceneNode );
        RunUpdateLoop( applicationManager );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_scene_node_multiple_children )
{
    BOOST_TEST_LOG_MESSAGE( "Starting scene node multiple children test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto graphicsScene = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( graphicsScene );

        auto parentNode = graphicsScene->addSceneNode();
        BOOST_REQUIRE( parentNode );
        RunUpdateLoop( applicationManager );

        // Add multiple children
        const size_t numChildren = 5;
        Array<SmartPtr<render::IGraphicsSceneNode>> children;

        for( size_t i = 0; i < numChildren; ++i )
        {
            auto child = parentNode->addChildSceneNode();
            BOOST_CHECK_WITH_LOG( child, "Child creation should succeed" );
            children.push_back( child );
        }

        RunUpdateLoop( applicationManager );

        auto parentChildren = parentNode->getChildren();
        BOOST_CHECK_WITH_LOG( parentChildren.size() == numChildren,
                              "Parent should have correct number of children" );
        BOOST_TEST_LOG_MESSAGE( "Parent has " << parentChildren.size() << " children" );

        // Verify each child
        for( size_t i = 0; i < numChildren; ++i )
        {
            BOOST_CHECK_WITH_LOG( children[i]->getParent() == parentNode,
                                  "Child " + StringUtil::toString( i ) + " should reference parent" );
        }

        // Remove children one by one
        for( size_t i = 0; i < numChildren; ++i )
        {
            parentNode->removeChild( children[i] );
            RunUpdateLoop( applicationManager, 2 );

            auto remainingChildren = parentNode->getChildren();
            BOOST_CHECK_WITH_LOG( remainingChildren.size() == numChildren - i - 1,
                                  "Parent should have correct number of remaining children" );
        }

        BOOST_CHECK_WITH_LOG( parentNode->getChildren().empty(),
                              "Parent should have no children after all removals" );

        // Clean up
        graphicsScene->removeSceneNode( parentNode );
        RunUpdateLoop( applicationManager );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_scene_node_edge_cases )
{
    BOOST_TEST_LOG_MESSAGE( "Starting scene node edge cases test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto graphicsScene = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( graphicsScene );

        auto sceneNode = graphicsScene->addSceneNode();
        BOOST_REQUIRE( sceneNode );
        RunUpdateLoop( applicationManager );

        // Test extreme position values
        Vector3F extremePosition( 1000000.0f, -1000000.0f, 500000.0f );
        sceneNode->setPosition( extremePosition );
        RunUpdateLoop( applicationManager, 2 );

        auto pos = sceneNode->getPosition();
        BOOST_CHECK_WITH_LOG( !std::isnan( pos.X() ) && !std::isnan( pos.Y() ) && !std::isnan( pos.Z() ),
                              "Position should not contain NaN values" );

        // Test zero scale
        Vector3F zeroScale( 0.0f, 0.0f, 0.0f );
        sceneNode->setScale( zeroScale );
        RunUpdateLoop( applicationManager, 2 );

        auto scale = sceneNode->getScale();
        BOOST_TEST_LOG_MESSAGE( "Zero scale set: " << scale.X() << ", " << scale.Y() << ", "
                                                   << scale.Z() );

        // Test very small scale
        Vector3F tinyScale( 0.001f, 0.001f, 0.001f );
        sceneNode->setScale( tinyScale );
        RunUpdateLoop( applicationManager, 2 );

        // Test identity orientation
        QuaternionF identityQuat( 1.0f, 0.0f, 0.0f, 0.0f );
        sceneNode->setOrientation( identityQuat );
        RunUpdateLoop( applicationManager, 2 );

        auto orientation = sceneNode->getOrientation();
        BOOST_CHECK_WITH_LOG( orientation.isValid(), "Identity orientation should be valid" );

        // Test removing non-existent child (should not crash)
        auto otherNode = graphicsScene->addSceneNode();
        BOOST_REQUIRE( otherNode );
        RunUpdateLoop( applicationManager );

        auto result = sceneNode->removeChild( otherNode );
        BOOST_CHECK_WITH_LOG( !result, "Removing non-child should return false" );

        // Test double removal (should not crash)
        auto child = sceneNode->addChildSceneNode();
        BOOST_REQUIRE( child );
        RunUpdateLoop( applicationManager );

        sceneNode->removeChild( child );
        RunUpdateLoop( applicationManager );

        auto secondRemoval = sceneNode->removeChild( child );
        BOOST_CHECK_WITH_LOG( !secondRemoval, "Second removal should return false" );

        // Clean up
        graphicsScene->removeSceneNode( otherNode );
        graphicsScene->removeSceneNode( sceneNode );
        RunUpdateLoop( applicationManager );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_scene_node_remove_all_children )
{
    BOOST_TEST_LOG_MESSAGE( "Starting scene node remove all children test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto graphicsScene = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( graphicsScene );

        auto parentNode = graphicsScene->addSceneNode();
        BOOST_REQUIRE( parentNode );
        RunUpdateLoop( applicationManager );

        // Add multiple children
        for( size_t i = 0; i < 10; ++i )
        {
            auto child = parentNode->addChildSceneNode();
            BOOST_REQUIRE( child );
        }

        RunUpdateLoop( applicationManager );

        auto children = parentNode->getChildren();
        BOOST_CHECK_WITH_LOG( children.size() == 10, "Parent should have 10 children" );

        // Remove all children at once
        parentNode->removeChildren();
        RunUpdateLoop( applicationManager );

        BOOST_CHECK_WITH_LOG( parentNode->getChildren().empty(),
                              "Parent should have no children after removeChildren()" );

        // Clean up
        graphicsScene->removeSceneNode( parentNode );
        RunUpdateLoop( applicationManager );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_scene_node_named_creation )
{
    BOOST_TEST_LOG_MESSAGE( "Starting scene node named creation test" );

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto graphicsScene = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( graphicsScene );

        // Create node with specific name
        String testName = "TestSceneNode_" + StringUtil::toString( rand() );
        auto sceneNode = graphicsScene->addSceneNode( testName );
        BOOST_REQUIRE( sceneNode );
        RunUpdateLoop( applicationManager );

        auto nodeName = sceneNode->getName();
        BOOST_CHECK_WITH_LOG( nodeName == testName || nodeName.find( testName ) != String::npos,
                              "Scene node should have assigned name or contain it" );
        BOOST_TEST_LOG_MESSAGE( "Node name: " << nodeName );

        // Clean up
        graphicsScene->removeSceneNode( sceneNode );
        RunUpdateLoop( applicationManager );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}

BOOST_AUTO_TEST_CASE( graphics_scene_node_accessors )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping graphics test" );
        return;
    }

    BOOST_TEST_LOG_MESSAGE( "Starting scene node accessor methods test" );

    try
    {
        TestGuard guard;

        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            BOOST_TEST_MESSAGE( "Graphics system is not available - skipping graphics test" );
            return;
        }

        auto graphicsScene = graphicsSystem->getGraphicsScene();
        BOOST_REQUIRE( graphicsScene );

        auto sceneNode = graphicsScene->addSceneNode();
        BOOST_REQUIRE( sceneNode );
        RunUpdateLoop( applicationManager );

        // Test name accessor
        String testName = "AccessorTestNode";
        sceneNode->setName( testName );
        RunUpdateLoop( applicationManager );
        BOOST_CHECK_WITH_LOG(
            sceneNode->getName() == testName || sceneNode->getName().find( testName ) != String::npos,
            "getName() should return the set name" );
        BOOST_TEST_LOG_MESSAGE( "Name accessor: " << sceneNode->getName() );

        // Test position accessor (set and get)
        Vector3F testPosition( 5.0f, 10.0f, 15.0f );
        sceneNode->setPosition( testPosition );
        RunUpdateLoop( applicationManager, 2 );
        auto retrievedPosition = sceneNode->getPosition();
        BOOST_TEST_LOG_MESSAGE( "Position accessor - Set: ("
                                << testPosition.X() << ", " << testPosition.Y() << ", "
                                << testPosition.Z() << ") Get: (" << retrievedPosition.X() << ", "
                                << retrievedPosition.Y() << ", " << retrievedPosition.Z() << ")" );

        // Test scale accessor (set and get)
        Vector3F testScale( 1.5f, 2.5f, 3.5f );
        sceneNode->setScale( testScale );
        RunUpdateLoop( applicationManager, 2 );
        auto retrievedScale = sceneNode->getScale();
        BOOST_TEST_LOG_MESSAGE( "Scale accessor - Set: ("
                                << testScale.X() << ", " << testScale.Y() << ", " << testScale.Z()
                                << ") Get: (" << retrievedScale.X() << ", " << retrievedScale.Y() << ", "
                                << retrievedScale.Z() << ")" );

        // Test orientation accessor (set and get)
        QuaternionF testOrientation( 1.0f, 0.0f, 0.0f, 0.0f );  // Identity quaternion
        sceneNode->setOrientation( testOrientation );
        RunUpdateLoop( applicationManager, 2 );
        auto retrievedOrientation = sceneNode->getOrientation();
        BOOST_CHECK_WITH_LOG( retrievedOrientation.isValid(),
                              "getOrientation() should return a valid quaternion" );
        BOOST_TEST_LOG_MESSAGE( "Orientation accessor - Get: ("
                                << retrievedOrientation.W() << ", " << retrievedOrientation.X() << ", "
                                << retrievedOrientation.Y() << ", " << retrievedOrientation.Z() << ")" );

        // Test parent accessor (should be null for root node)
        BOOST_CHECK_WITH_LOG( sceneNode->getParent() == nullptr,
                              "getParent() should return nullptr for root scene node" );

        // Test children accessor (should be empty initially)
        auto children = sceneNode->getChildren();
        BOOST_CHECK_WITH_LOG( children.empty(),
                              "getChildren() should return empty array for node without children" );

        // Add a child and verify children accessor
        auto childNode = sceneNode->addChildSceneNode();
        BOOST_REQUIRE( childNode );
        RunUpdateLoop( applicationManager );

        children = sceneNode->getChildren();
        BOOST_CHECK_WITH_LOG( children.size() == 1,
                              "getChildren() should return one child after adding" );
        BOOST_CHECK_WITH_LOG( children[0] == childNode, "getChildren() should contain the added child" );

        // Test parent accessor on child
        BOOST_CHECK_WITH_LOG( childNode->getParent() == sceneNode,
                              "getParent() should return parent node for child" );

        // Test objects accessor (should be empty initially)
        auto objects = sceneNode->getObjects();
        BOOST_CHECK_WITH_LOG(
            objects.empty(),
            "getObjects() should return empty array for node without attached objects" );

        // Test isValid accessor
        BOOST_CHECK_WITH_LOG( sceneNode->isValid(),
                              "isValid() should return true for valid scene node" );
        BOOST_CHECK_WITH_LOG( childNode->isValid(),
                              "isValid() should return true for valid child node" );

        // Clean up
        graphicsScene->removeSceneNode( sceneNode );
        RunUpdateLoop( applicationManager );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( "Exception thrown: " + String( e.what() ) );
    }
}
