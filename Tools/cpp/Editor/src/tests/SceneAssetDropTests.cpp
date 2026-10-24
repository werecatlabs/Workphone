#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Components/Component.hpp>

#include <EditorApplication.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/ProjectAssetsWindow.hpp>
#include <ui/ProjectTreeData.hpp>
#include <ui/TransformWindow.hpp>
#include <ui/UIManager.hpp>

#if WP_EDITOR_TESTS
#    include <boost/test/unit_test.hpp>
#    include <filesystem>
#    include <fstream>
#    include <limits>
#    include <set>

using namespace workphone;
using namespace workphone::editor;

namespace
{
    struct SceneAssetDropDirectory
    {
        std::filesystem::path path = std::filesystem::temp_directory_path() /
                                     std::string( ( "workphone-scene-drop-" +
                                                    StringUtil::getUUID() ).c_str() );

        ~SceneAssetDropDirectory()
        {
            std::error_code ignored;
            std::filesystem::remove_all( path, ignored );
        }
    };

    struct SceneAssetDropEditor
    {
        EditorApplication app;

        ~SceneAssetDropEditor()
        {
            if( auto application = core::IApplicationManager::instance() )
            {
                application->setQuit( true );
                application->setRunning( false );
            }
            app.unload( nullptr );
        }
    };

    struct SceneAssetDropTask
    {
        TaskId previous = Thread::getCurrentTask();

        SceneAssetDropTask()
        {
            Thread::setCurrentTask( TaskId::Application );
        }

        ~SceneAssetDropTask()
        {
            Thread::setCurrentTask( previous );
        }
    };

    void checkDropPosition( SmartPtr<scene::IGameActor> actor,
                            const Vector3<real_Num> &expected )
    {
        const auto actual = actor->getPosition();
        BOOST_CHECK_SMALL( actual.x - expected.x, static_cast<real_Num>( 0.0001 ) );
        BOOST_CHECK_SMALL( actual.y - expected.y, static_cast<real_Num>( 0.0001 ) );
        BOOST_CHECK_SMALL( actual.z - expected.z, static_cast<real_Num>( 0.0001 ) );
    }
}  // namespace

BOOST_AUTO_TEST_CASE( scene_render_asset_drop_test )
{
    SceneAssetDropDirectory temporary;
    const auto directory = temporary.path / "Scene Drop Project with spaces";
    Project project;
    project.create( directory.generic_u8string() );
    project.setMediaPaths( {} );
    project.save();
    std::ofstream( directory / "Assets/Triangle.obj" )
        << "o Triangle\nv 0 0 0\nv 1 0 0\nv 0 1 0\n"
           "vt 0 0\nvt 1 0\nvt 0 1\nvn 0 0 1\nf 1/1/1 2/2/1 3/3/1\n";
    std::ofstream( directory / "Assets/Unsupported.txt" ) << "Not a scene asset";

    SceneAssetDropEditor guard;
    guard.app.setDebugMode( true );
    guard.app.setActiveThreads( 0 );
    guard.app.load( nullptr );
    BOOST_REQUIRE( guard.app.isLoaded() );
    auto app = core::IApplicationManager::instance();
    BOOST_REQUIRE( app );
    auto editor = EditorManager::getSingletonPtr();
    BOOST_REQUIRE( editor );
    editor->loadProject( project.getFilePath() );
    SceneAssetDropTask applicationTask;

    auto gameManager = app->getGameManager();
    auto scene = gameManager ? gameManager->getCurrentScene() : nullptr;
    auto commands = app->getCommandManager();
    auto selection = app->getSelectionManager();
    auto viewport = app->getSceneRenderWindow();
    auto ui = app->getUI();
    BOOST_REQUIRE( scene );
    BOOST_REQUIRE( commands );
    BOOST_REQUIRE( selection );
    BOOST_REQUIRE( viewport );
    BOOST_REQUIRE( ui );
    auto target = viewport->getDropTarget();
    BOOST_REQUIRE_MESSAGE( target, "The scene render viewport must accept asset drops" );
    auto editorUI = editor->getUI();
    BOOST_REQUIRE( editorUI );
    auto actorWindow = editorUI->getActorWindow();
    BOOST_REQUIRE( actorWindow );
    auto transformWindow = actorWindow->getTransformWindow();
    BOOST_REQUIRE( transformWindow );

    auto flushJobs = [&]() {
        auto jobs = app->getJobQueue();
        BOOST_REQUIRE( jobs );
        for( int i = 0; i < 100 && jobs->hasJobs(); ++i )
        {
            jobs->update();
        }
    };
    flushJobs();
    commands->clearAll();

    // Serialize a real prefab hierarchy, including a nonzero saved position that a viewport
    // drop must replace. Each instance must receive its own actor identifiers.
    auto prefabActor = gameManager->createActor();
    auto prefabChild = gameManager->createActor();
    BOOST_REQUIRE( prefabActor );
    BOOST_REQUIRE( prefabChild );
    prefabActor->setName( "Viewport Prefab" );
    prefabActor->setPosition( Vector3<real_Num>( 50, 60, 70 ) );
    prefabChild->setName( "Prefab Child" );
    BOOST_REQUIRE( prefabChild->addComponent<scene::Component>() );
    prefabActor->addChild( prefabChild );
    const auto originalRootId = prefabActor->getHandle()->getUUIDAsString();
    const auto originalChildId = prefabChild->getHandle()->getUUIDAsString();
    auto prefabs = app->getPrefabManager();
    BOOST_REQUIRE( prefabs );
    prefabs->savePrefab( "Assets/Viewport.prefab", prefabActor );
    BOOST_REQUIRE( std::filesystem::is_regular_file( directory / "Assets/Viewport.prefab" ) );
    gameManager->destroyActor( prefabActor );
    prefabChild = nullptr;
    prefabActor = nullptr;

    auto selectedParent = gameManager->createActor();
    BOOST_REQUIRE( selectedParent );
    selectedParent->setName( "Selected Parent" );
    scene->addActor( selectedParent );
    selection->clearSelection();
    selection->addSelectedObject( selectedParent );

    // Exercise the project asset drag serializer, rather than constructing a parallel payload.
    auto sourceTree = ui->addElementByType<ui::IUITreeCtrl>();
    BOOST_REQUIRE( sourceTree );
    auto sourceNode = sourceTree->addRoot();
    BOOST_REQUIRE( sourceNode );
    auto sourceData = workphone::make_ptr<ProjectTreeData>();
    sourceNode->setNodeUserData( sourceData );
    ProjectAssetsWindow::DragSource dragSource;
    auto payloadFor = [&]( const String &path, const String &kind ) {
        sourceData->setOwnerType( kind );
        sourceData->setObjectType( path );
        return dragSource.handleDrag( Vector2I::zero(), sourceNode );
    };
    const auto prefabPayload = payloadFor( "Assets/Viewport.prefab", "file" );
    const auto meshPayload = payloadFor( "Assets/Triangle.obj", "file" );
    BOOST_REQUIRE( !prefabPayload.empty() );
    BOOST_REQUIRE( !meshPayload.empty() );

    auto deliver = [&]( const String &payload, const Vector3<real_Num> &origin,
                        const Vector3<real_Num> &direction ) {
        Array<Parameter> arguments( 4 );
        arguments[0].setStr( payload );
        arguments[1].setS32( -1 );
        arguments[2].setVector3( origin );
        arguments[3].setVector3( direction );
        target->handleEvent( EventType::UI, IEvent::handleDrop, arguments, sourceNode, viewport,
                             nullptr );
        flushJobs();
    };
    auto dropAndCheck = [&]( const String &payload, const String &name,
                             const Vector3<real_Num> &origin,
                             const Vector3<real_Num> &direction,
                             const Vector3<real_Num> &expected ) {
        selection->clearSelection();
        selection->addSelectedObject( selectedParent );
        const auto before = scene->getActors().size();
        deliver( payload, origin, direction );
        BOOST_REQUIRE_EQUAL( scene->getActors().size(), before + 1 );
        auto actor = scene->getActors().back();
        BOOST_REQUIRE( actor );
        BOOST_CHECK_EQUAL( actor->getName(), name );
        BOOST_CHECK( !actor->getParent() );
        BOOST_CHECK( selectedParent->getChildren().empty() );
        checkDropPosition( actor, expected );

        // Undo must also clear selected prefab descendants and owned components while
        // retaining other selected actors. Redo must not revive the stale selection.
        selection->clearSelection();
        selection->addSelectedObject( selectedParent );
        selection->addSelectedObject( actor );
        for( auto child : actor->getChildren() )
        {
            selection->addSelectedObject( child );
        }
        auto component = actor->getComponentInThisAndChildren<scene::IComponent>();
        BOOST_REQUIRE( component );
        selection->addSelectedObject( component );
        actorWindow->updateSelection();

        auto undo = commands->getPreviousCommand();
        BOOST_REQUIRE( undo );
        undo->undo();
        BOOST_CHECK_EQUAL( scene->getActors().size(), before );
        auto remainingSelection = selection->getSelection();
        BOOST_REQUIRE_EQUAL( remainingSelection.size(), 1u );
        BOOST_CHECK( remainingSelection.front() == selectedParent );
        BOOST_CHECK( transformWindow->getTransform() == selectedParent->getTransform() );
        BOOST_CHECK( transformWindow->isWindowVisible() );
        auto redo = commands->getNextCommand();
        BOOST_REQUIRE( redo );
        BOOST_CHECK( redo == undo );
        redo->redo();
        BOOST_REQUIRE_EQUAL( scene->getActors().size(), before + 1 );
        BOOST_CHECK( scene->getActors().back() == actor );
        BOOST_CHECK( !actor->getParent() );
        checkDropPosition( actor, expected );
        remainingSelection = selection->getSelection();
        BOOST_REQUIRE_EQUAL( remainingSelection.size(), 1u );
        BOOST_CHECK( remainingSelection.front() == selectedParent );
        BOOST_CHECK( transformWindow->getTransform() == selectedParent->getTransform() );

        // With only the dropped actor selected, undo must release the Inspector's
        // transform facade as well as its selection. Redo preserves placement without
        // reviving that binding until the actor is explicitly selected again.
        selection->clearSelection();
        selection->addSelectedObject( actor );
        actorWindow->updateSelection();
        BOOST_CHECK( transformWindow->getTransform() == actor->getTransform() );
        BOOST_CHECK( transformWindow->isWindowVisible() );
        undo = commands->getPreviousCommand();
        BOOST_REQUIRE( undo );
        undo->undo();
        BOOST_CHECK_EQUAL( scene->getActors().size(), before );
        BOOST_CHECK( selection->getSelection().empty() );
        BOOST_CHECK( !transformWindow->getTransform() );
        BOOST_CHECK( !transformWindow->isWindowVisible() );
        redo = commands->getNextCommand();
        BOOST_REQUIRE( redo );
        redo->redo();
        BOOST_REQUIRE_EQUAL( scene->getActors().size(), before + 1 );
        BOOST_CHECK( scene->getActors().back() == actor );
        checkDropPosition( actor, expected );
        BOOST_CHECK( selection->getSelection().empty() );
        BOOST_CHECK( !transformWindow->getTransform() );
        BOOST_CHECK( !transformWindow->isWindowVisible() );
        return actor;
    };

    // A non-unit ray verifies normalization before ground-plane placement.
    auto first = dropAndCheck( prefabPayload, "Viewport", Vector3<real_Num>( 3, 8, 5 ),
                               Vector3<real_Num>( 0, -2, 0 ), Vector3<real_Num>( 3, 0, 5 ) );
    auto mesh = dropAndCheck( meshPayload, "Triangle", Vector3<real_Num>( -2, 6, 4 ),
                              Vector3<real_Num>( 0, -1, 0 ), Vector3<real_Num>( -2, 0, 4 ) );
    BOOST_CHECK( mesh->getComponentInThisAndChildren<scene::Mesh>() );
    auto second = dropAndCheck( prefabPayload, "Viewport", Vector3<real_Num>( 2, 3, 4 ),
                                Vector3<real_Num>( 2, 0, 0 ), Vector3<real_Num>( 12, 3, 4 ) );
    BOOST_REQUIRE_EQUAL( first->getChildren().size(), 1u );
    BOOST_REQUIRE_EQUAL( second->getChildren().size(), 1u );
    std::set<String> actorIds = { originalRootId, originalChildId,
                                  first->getHandle()->getUUIDAsString(),
                                  first->getChildren().front()->getHandle()->getUUIDAsString(),
                                  second->getHandle()->getUUIDAsString(),
                                  second->getChildren().front()->getHandle()->getUUIDAsString() };
    BOOST_CHECK_EQUAL( actorIds.size(), 6u );

    commands->clearAll();
    const auto beforeInvalid = scene->getActors().size();
    const auto origin = Vector3<real_Num>( 0, 5, 0 );
    const auto down = Vector3<real_Num>( 0, -1, 0 );
    deliver( payloadFor( "Assets/Unsupported.txt", "file" ), origin, down );
    deliver( payloadFor( "Assets", "folder" ), origin, down );
    deliver( "", origin, down );
    deliver( prefabPayload, origin, Vector3<real_Num>::zero() );
    deliver( prefabPayload,
              Vector3<real_Num>( std::numeric_limits<real_Num>::quiet_NaN(), 5, 0 ), down );
    // Viewport drops without a cursor ray must never instantiate assets at an arbitrary origin.
    Array<Parameter> missingRay;
    missingRay.emplace_back( prefabPayload );
    target->handleEvent( EventType::UI, IEvent::handleDrop, missingRay, sourceNode, viewport,
                         nullptr );
    flushJobs();
    BOOST_CHECK_EQUAL( scene->getActors().size(), beforeInvalid );
    BOOST_CHECK( !commands->getPreviousCommand() );

    selection->clearSelection();
    ui->removeElement( sourceTree );
    gameManager->destroyActor( first );
    gameManager->destroyActor( mesh );
    gameManager->destroyActor( second );
    gameManager->destroyActor( selectedParent );
}

#endif
