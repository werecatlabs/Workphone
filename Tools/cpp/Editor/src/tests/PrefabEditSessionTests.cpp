#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>

#if WP_EDITOR_TESTS
#    include <EditorApplication.hpp>
#    include <commands/Command.hpp>
#    include <editor/EditorManager.hpp>
#    include <editor/Project.hpp>
#    include <Workphone/Scene/Components/PrefabInstance.hpp>
#    include <Workphone/Scene/Components/Camera/CameraTarget.hpp>
#    include <boost/test/unit_test.hpp>
#    include <filesystem>
#    include <fstream>

using namespace workphone;
using namespace workphone::editor;

namespace
{
    // Preferences are read during Application::load, before loadProject. Keep
    // the entire application lifetime in a private working directory, including
    // constructor-failure cleanup and the destructor's settings write.
    struct PrefabTestDirectory
    {
        std::filesystem::path previous = std::filesystem::current_path();
        std::filesystem::path path;

        explicit PrefabTestDirectory( const std::filesystem::path &directory ) : path( directory )
        {
            std::filesystem::create_directories( path );
            std::filesystem::current_path( path );
        }

        ~PrefabTestDirectory()
        {
            std::error_code ignored;
            std::filesystem::current_path( previous, ignored );
            if( !ignored ) std::filesystem::remove_all( path, ignored );
        }
    };

    class PrefabTestApplication : public EditorApplication
    {
    public:
        String pluginConfiguration, mediaDirectory, mediaCatalog;
        String getMediaPath() const override { return mediaDirectory; }
        String getMediaCatalogPath( const String & ) const override { return mediaCatalog; }

    private:
        void createPlugins() override
        {
            setPluginsConfigFilePath( pluginConfiguration );
            core::Application::createPlugins();
        }
    };

    struct PrefabEditorFixture
    {
        std::filesystem::path directory = std::filesystem::temp_directory_path() /
            std::string( ( "workphone-prefab-editor-" + StringUtil::getUUID() ).c_str() );
        PrefabTestDirectory isolatedDirectory{ directory };
        PrefabTestApplication application;
        TaskId previousTask = Thread::getCurrentTask();

        PrefabEditorFixture()
        {
            const auto configPath = directory / "plugins.cfg";
            std::ofstream config( configPath );
            config << "PluginFolder=" << isolatedDirectory.previous.generic_string() << '\n';
#    if WP_GRAPHICS_SYSTEM_OGRENEXT
            config << "Plugin=WPGraphicsOgreNext\n";
#    elif WP_GRAPHICS_SYSTEM_OGRE
            config << "Plugin=WPGraphicsOgre\n";
#    else
            config << "Plugin=WPGraphics\n";
#    endif
            config << "Plugin=WPSQLite\nPlugin=WPPhysics\nPlugin=WPAssimp\n";
#    if WP_BUILD_AUDIO
            config << "Plugin=WPAudio\n";
#    endif
            config.close();
            BOOST_REQUIRE( config.good() );
            auto media = isolatedDirectory.previous;
            while( !std::filesystem::is_directory( media / "Media" ) && media != media.root_path() )
                media = media.parent_path();
            BOOST_REQUIRE( std::filesystem::is_directory( media / "Media" ) );
            application.pluginConfiguration = configPath.generic_u8string().c_str();
            application.mediaDirectory = ( media / "Media" ).generic_u8string().c_str();
            application.mediaCatalog = ( directory / "media-assets.db" ).generic_u8string().c_str();
            application.setApplicationFlags( application.getApplicationFlags() &
                                             ~core::IApplication::developerModeFlag );
            Project project;
            project.create( directory.generic_u8string() );
            project.setMediaPaths( {} );
            project.save();
            application.setDebugMode( true );
            application.setActiveThreads( 0 );
            application.load( nullptr );
            BOOST_REQUIRE( application.isLoaded() );
            auto editor = EditorManager::getSingletonPtr();
            BOOST_REQUIRE( editor );
            editor->loadProject( project.getFilePath() );
            Thread::setCurrentTask( TaskId::Application );
            auto app = core::IApplicationManager::instance();
            BOOST_REQUIRE( app );
            app->setEditor( true );
            app->setPlaying( false );
            auto jobs = app->getJobQueue();
            for( unsigned i = 0; jobs && jobs->hasJobs() && i < 100; ++i ) jobs->update();
            BOOST_REQUIRE( !jobs || !jobs->hasJobs() );
            app->getCommandManager()->clearAll();
        }

        ~PrefabEditorFixture()
        {
            if( auto editor = EditorManager::getSingletonPtr(); editor && editor->isEditingPrefab() )
            {
                String error;
                editor->closePrefab( error );
            }
            if( auto app = core::IApplicationManager::instance() )
            {
                app->setQuit( true );
                app->setRunning( false );
            }
            application.unload( nullptr );
            Thread::setCurrentTask( previousTask );
        }

        String prefab( const char *name = "Editable.prefab" )
        {
            auto app = core::IApplicationManager::instance();
            auto manager = app->getGameManager();
            auto source = manager->createActor();
            source->setName( "PrefabSource" );
            source->setLocalPosition( Vector3F( 4, 5, 6 ) );
            source->setLocalOrientation( QuaternionF::eulerDegrees( 0, 15, 0 ) );
            source->setLocalScale( Vector3F( 2, 3, 4 ) );
            auto child = manager->createActor();
            child->setName( "SourceChild" );
            child->addComponent<scene::CameraTarget>();
            source->addChild( child );
            source->updateTransform();
            const auto path = String( ( directory / "Assets" / name ).generic_u8string().c_str() );
            app->getPrefabManager()->savePrefab( path, source );
            manager->destroyActor( source, true );
            BOOST_REQUIRE( app->getFileSystem()->isExistingFile( path, false, false ) );
            return path;
        }

        SmartPtr<scene::IGameActor> instantiate( const String &path )
        {
            auto app = core::IApplicationManager::instance();
            auto prefab = app->getPrefabManager()->loadPrefab( path );
            BOOST_REQUIRE( prefab );
            auto actor = prefab->createActor();
            BOOST_REQUIRE( actor );
            app->getGameManager()->getCurrentScene()->addActor( actor );
            BOOST_REQUIRE( actor->getComponent<scene::PrefabInstance>() );
            return actor;
        }
    };

    class PrefabRenameCommand : public Command
    {
    public:
        SmartPtr<scene::IGameActor> actor;
        String before, after;
        void execute() override { actor->setName( after ); setState( State::Finished ); }
        void undo() override { actor->setName( before ); }
        void redo() override { actor->setName( after ); }
    };

    SmartPtr<PrefabRenameCommand> renameCommand( SmartPtr<scene::IGameActor> actor,
                                                const String &name )
    {
        auto command = make_ptr<PrefabRenameCommand>();
        command->actor = actor;
        command->before = actor->getName();
        command->after = name;
        // Match owner-thread Inspector edits: mutate once, then register the
        // completed command without scheduling a background history job.
        command->execute();
        core::IApplicationManager::instance()->getCommandManager()->addCommand( command );
        return command;
    }

    void checkVector( const Vector3F &actual, const Vector3F &expected )
    {
        BOOST_CHECK_SMALL( actual.x - expected.x, 0.0002f );
        BOOST_CHECK_SMALL( actual.y - expected.y, 0.0002f );
        BOOST_CHECK_SMALL( actual.z - expected.z, 0.0002f );
    }
}

BOOST_AUTO_TEST_CASE( prefab_edit_open_discard_and_undo_preserve_authoring_objects )
{
    PrefabEditorFixture fixture;
    auto app = core::IApplicationManager::instance();
    auto editor = EditorManager::getSingletonPtr();
    const auto path = fixture.prefab();
    const auto originalBytes = app->getFileSystem()->readAllText( path );
    auto original = fixture.instantiate( path );
    auto originalChild = original->findChildByName( "SourceChild" );
    auto originalComponent = originalChild->getComponent<scene::CameraTarget>();
    auto scene = app->getGameManager()->getCurrentScene();
    auto commands = app->getCommandManager();
    auto command = renameCommand( original, "UnsavedSceneName" );
    original->setPosition( Vector3F( 12, 34, 56 ) );
    original->updateTransform();
    const auto position = original->getPosition();
    auto selection = app->getSelectionManager();
    selection->clearSelection();
    selection->addSelectedObject( original );
    editor->setFileSaved( false );

    String error;
    BOOST_REQUIRE_MESSAGE( editor->openPrefab( path, error ), error.c_str() );
    BOOST_CHECK( editor->isEditingPrefab() );
    BOOST_CHECK( editor->getAuthoringScene() == scene );
    BOOST_CHECK( app->getGameManager()->getCurrentScene() != scene );
    BOOST_CHECK( app->getCommandManager() != commands );
    BOOST_CHECK( commands->hasCommand( command ) );
    auto staged = editor->getPrefabEditRoot();
    BOOST_REQUIRE( staged );
    BOOST_CHECK( staged != original );
    auto stagedCommand = renameCommand( staged, "UnsavedPrefabName" );
    auto undo = app->getCommandManager()->getPreviousCommand();
    BOOST_REQUIRE( undo );
    BOOST_CHECK( undo == stagedCommand );
    undo->undo();
    BOOST_CHECK_EQUAL( staged->getName(), "PrefabSource" );
    staged->setName( "DiscardThisName" );
    BOOST_CHECK_EQUAL( original->getName(), "UnsavedSceneName" );
    BOOST_CHECK( original->findChildByName( "SourceChild" ) == originalChild );
    BOOST_CHECK( originalChild->getComponent<scene::CameraTarget>() == originalComponent );

    BOOST_REQUIRE_MESSAGE( editor->closePrefab( error ), error.c_str() );
    BOOST_CHECK( !editor->isEditingPrefab() );
    BOOST_CHECK( app->getGameManager()->getCurrentScene() == scene );
    BOOST_CHECK( app->getCommandManager() == commands );
    BOOST_CHECK( original->isEnabled() && original->isVisible() );
    BOOST_CHECK( !editor->getFileSaved() );
    checkVector( original->getPosition(), position );
    BOOST_CHECK_EQUAL( app->getFileSystem()->readAllText( path ), originalBytes );
    const auto restoredSelection = selection->getSelection();
    BOOST_REQUIRE_EQUAL( restoredSelection.size(), 1u );
    BOOST_CHECK( restoredSelection.front() == original );
    undo = commands->getPreviousCommand();
    BOOST_REQUIRE( undo );
    BOOST_CHECK( undo == command );
    undo->undo();
    BOOST_CHECK_EQUAL( original->getName(), "PrefabSource" );
    BOOST_CHECK( original->findChildByName( "SourceChild" ) == originalChild );
}

BOOST_AUTO_TEST_CASE( prefab_edit_save_updates_instances_and_marks_original_scene_unsaved )
{
    PrefabEditorFixture fixture;
    auto app = core::IApplicationManager::instance();
    auto editor = EditorManager::getSingletonPtr();
    const auto path = fixture.prefab();
    const auto otherPath = fixture.prefab( "Other.prefab" );
    const auto before = app->getFileSystem()->readAllText( path );
    auto first = fixture.instantiate( path );
    auto second = fixture.instantiate( path );
    auto other = fixture.instantiate( otherPath );
    other->setName( "OtherInstance" );
    const auto otherBefore = DataUtil::toString( other->toData().get() );
    first->setPosition( Vector3F( 10, 20, 30 ) );
    second->setPosition( Vector3F( -15, 5, 80 ) );
    first->setLocalScale( Vector3F( 20, 20, 20 ) );
    first->setLocalOrientation( QuaternionF::eulerDegrees( 20, 30, 40 ) );
    first->updateTransform();second->updateTransform();
    const auto firstPosition = first->getPosition(), secondPosition = second->getPosition();
    auto child = first->findChildByName( "SourceChild" );
    auto component = child->getComponent<scene::CameraTarget>();
    const auto scene = app->getGameManager()->getCurrentScene();
    editor->setFileSaved( true );
    String error;
    BOOST_REQUIRE_MESSAGE( editor->openPrefab( path, error ), error.c_str() );
    auto staged = editor->getPrefabEditRoot();
    BOOST_REQUIRE( staged );
    staged->setName( "EditedSource" );
    staged->setLocalOrientation( QuaternionF::eulerDegrees( 0, 75, 0 ) );
    staged->setLocalScale( Vector3F( 3, 4, 5 ) );
    staged->findChildByName( "SourceChild" )->setName( "EditedChild" );
    staged->updateTransform();
    size_t updated = 0;
    BOOST_REQUIRE_MESSAGE( editor->savePrefab( error, &updated ), error.c_str() );
    BOOST_CHECK_EQUAL( updated, 2u );
    BOOST_CHECK( editor->isEditingPrefab() );
    BOOST_CHECK( editor->getAuthoringScene() == scene );
    BOOST_CHECK( app->getFileSystem()->readAllText( path ) != before );
    // Discard only edits made after the successful save.
    staged->setName( "DiscardAfterSave" );
    BOOST_REQUIRE_MESSAGE( editor->closePrefab( error ), error.c_str() );
    BOOST_CHECK( app->getGameManager()->getCurrentScene() == scene );
    BOOST_CHECK( !editor->getFileSaved() );
    BOOST_CHECK_EQUAL( first->getName(), "EditedSource" );
    BOOST_CHECK_EQUAL( second->getName(), "EditedSource" );
    BOOST_CHECK( first->findChildByName( "EditedChild" ) == child );
    BOOST_CHECK( child->getComponent<scene::CameraTarget>() == component );
    checkVector( first->getPosition(), firstPosition );
    checkVector( second->getPosition(), secondPosition );
    checkVector( first->getLocalScale(), Vector3F( 3, 4, 5 ) );
    BOOST_CHECK( first->isEnabled() && first->isVisible() );
    BOOST_CHECK_EQUAL( DataUtil::toString( other->toData().get() ), otherBefore );
    BOOST_REQUIRE_MESSAGE( editor->openPrefab( path, error ), error.c_str() );
    BOOST_CHECK_EQUAL( editor->getPrefabEditRoot()->getName(), "EditedSource" );
    BOOST_REQUIRE_MESSAGE( editor->closePrefab( error ), error.c_str() );
}

BOOST_AUTO_TEST_CASE( prefab_edit_failed_open_preserves_scene_selection_and_history )
{
    PrefabEditorFixture fixture;
    auto app = core::IApplicationManager::instance();
    auto editor = EditorManager::getSingletonPtr();
    auto actor = fixture.instantiate( fixture.prefab() );
    auto scene = app->getGameManager()->getCurrentScene();
    auto commands = app->getCommandManager();
    auto command = renameCommand( actor, "AuthoredName" );
    app->getSelectionManager()->addSelectedObject( actor );
    editor->setFileSaved( false );
    String error;
    BOOST_CHECK( !editor->openPrefab( String( ( fixture.directory / "Assets/Missing.prefab" ).generic_u8string().c_str() ), error ) );
    BOOST_CHECK( !error.empty() );
    BOOST_CHECK( !editor->isEditingPrefab() );
    BOOST_CHECK( app->getGameManager()->getCurrentScene() == scene );
    BOOST_CHECK( app->getCommandManager() == commands );
    BOOST_CHECK( commands->hasCommand( command ) );
    BOOST_CHECK_EQUAL( actor->getName(), "AuthoredName" );
    BOOST_CHECK( actor->isEnabled() && actor->isVisible() );
    BOOST_CHECK( !editor->getFileSaved() );
}
#endif
