#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>

#include <EditorApplication.hpp>
#include <ui/ScriptWindow.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/PropertiesWindow.hpp>
#include <ui/ObjectWindow.hpp>
#include <ui/UIManager.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <editor/MaterialShaderReloadActions.hpp>
#include <Workphone/Interface/UI/IUITextEntry.hpp>
#include <Workphone/System/FileSelection.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Input/Joystick.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Scene/Components/Cubemap.hpp>
#include <Workphone/Graphics/GraphicsCubemap.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>

#include <algorithm>
#include <functional>
#include <filesystem>

#if WP_EDITOR_TESTS
#    include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::editor;

namespace
{
    class InspectorCubemapTexture : public render::Texture
    {
    public:
        Array<SmartPtr<render::ITexture>> getCubemapFaces() const override
        {
            return faces;
        }

        Array<SmartPtr<render::ITexture>> faces;
    };

    class EditorApplicationGuard
    {
    public:
        void load()
        {
            m_application.load( nullptr );
        }

        void loadSingleThreaded()
        {
            m_application.setActiveThreads( 0 );
            load();
        }

        void updateApplication()
        {
            const auto previousTask = Thread::getCurrentTask();
            Thread::setCurrentTask( TaskId::Application );
            try
            {
                m_application.update();
            }
            catch( ... )
            {
                Thread::setCurrentTask( previousTask );
                throw;
            }
            Thread::setCurrentTask( previousTask );
        }

        ~EditorApplicationGuard()
        {
            if( auto applicationManager = core::IApplicationManager::instancePtr() )
            {
                applicationManager->setQuit( true );
                applicationManager->setRunning( false );
            }

            m_application.unload( nullptr );
        }

    private:
        EditorApplication m_application;
    };
}  // namespace

BOOST_AUTO_TEST_CASE( script_window_class_name_test )
{
    ScriptWindow window;

    BOOST_CHECK_EQUAL( window.getClassName(), String() );

    const String className = "TerrainEditor";
    window.setClassName( className );
    BOOST_CHECK_EQUAL( window.getClassName(), className );

    window.setClassName( String() );
    BOOST_CHECK( StringUtil::isNullOrEmpty( window.getClassName() ) );
}

BOOST_AUTO_TEST_CASE( editor_script_manager_lua_support_test )
{
    EditorApplicationGuard app;
    app.load();

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );

    auto scriptManager = applicationManager->getScriptManager();
    BOOST_REQUIRE( scriptManager );
    BOOST_CHECK( scriptManager->isLoaded() );

    const auto extensions = scriptManager->getSupportedFileExtensions();
    BOOST_CHECK( std::find( extensions.begin(), extensions.end(), String( ".lua" ) ) !=
                 extensions.end() );
}

BOOST_AUTO_TEST_CASE( editor_script_window_owner_binding_test )
{
    EditorApplicationGuard app;
    app.load();

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );
    auto scriptManager = applicationManager->getScriptManager();
    BOOST_REQUIRE( scriptManager );

    scriptManager->loadScriptFromString( R"(
        class 'EditorWindowOwnerProbe'
        function EditorWindowOwnerProbe:__init(window)
            assert(type(window.getParent) == "function")
            assert(type(window.getParentWindow) == "function")
            assert(type(window.getDebugWindow) == "function")
            assert(window:getParent() == nil)
            assert(window:getParentWindow() == nil)
            assert(window:getDebugWindow() == nil)
            window:setParent(nil)
            window:setParentWindow(nil)
            window:setDebugWindow(nil)
            window:setName("Editor owner methods verified")
        end
    )" );

    SmartPtr<ISharedObject> owner = make_ptr<ScriptWindow>();
    scriptManager->createObject( "EditorWindowOwnerProbe", owner );
    BOOST_CHECK_EQUAL( owner->getName(), "Editor owner methods verified" );
    scriptManager->destroyObject( owner );
}

BOOST_AUTO_TEST_CASE( editor_script_window_properties_bridge_test )
{
    EditorApplicationGuard app;
    app.loadSingleThreaded();
    auto manager = core::IApplicationManager::instance()->getScriptManager();
    BOOST_REQUIRE( manager );
    manager->loadScriptFromString( R"(
        class 'EditorPropertiesBridgeProbe'
        function EditorPropertiesBridgeProbe:__init(window) self.window = window end
        function EditorPropertiesBridgeProbe:getProperties(p)
            local properties = p:at(0)
            properties:setProperty('bridgeProbe', 'typed object reached Lua')
        end
        function EditorPropertiesBridgeProbe:setProperties(p)
            local properties = p:at(0)
            self.window:setName(properties:getProperty('bridgeProbe'))
        end
    )" );
    auto window = make_ptr<ScriptWindow>();
    window->setClassName( "EditorPropertiesBridgeProbe" );
    window->load( nullptr );
    window->setWindowVisible( true );
    auto properties = window->getProperties();
    BOOST_REQUIRE( properties );
    BOOST_CHECK_EQUAL( properties->getProperty( "bridgeProbe" ), "typed object reached Lua" );
    properties->setProperty( "bridgeProbe", "setter reached Lua" );
    window->setProperties( properties );
    BOOST_CHECK_EQUAL( window->getName(), "setter reached Lua" );
    window->unload( nullptr );
}

BOOST_AUTO_TEST_CASE( editor_project_settings_live_bridge_test )
{
    EditorApplicationGuard app;
    app.loadSingleThreaded();
    auto application = core::IApplicationManager::instance();
    BOOST_REQUIRE( application );
    auto editor = EditorManager::getSingletonPtr();
    BOOST_REQUIRE( editor );
    auto scripts = application->getScriptManager();
    BOOST_REQUIRE( scripts );

    struct ProjectGuard
    {
        EditorManager *editor;
        SmartPtr<Project> previous;
        std::filesystem::path directory;

        ~ProjectGuard()
        {
            editor->setProject( previous );
            std::error_code ignored;
            std::filesystem::remove_all( directory, ignored );
        }
    } guard{ editor, editor->getProject(),
             std::filesystem::temp_directory_path() /
                 std::string( ( "lioncat-project-settings-lua-" + StringUtil::getUUID() ).c_str() ) };
    auto project = make_ptr<Project>();
    project->create( guard.directory.u8string() );
    auto initial = project->getProperties();
    initial->setProperty( "project.name", "Lua settings project" );
    initial->setProperty( "project.company", String() );
    initial->setProperty( "sound.masterVolume", s32( 80 ) );
    project->setProperties( initial );
    project->save();
    editor->setProject( project );
    const auto beforeOpen = DataUtil::toString( project->getProperties().get(), true );

    // This uses the real Properties factory, event enum/hash, UI bindings and
    // ScriptWindow action bridge. The separate Lua suite checks every control.
    auto window = make_ptr<ScriptWindow>();
    struct WindowGuard
    {
        SmartPtr<ScriptWindow> window;
        ~WindowGuard() { window->unload( nullptr ); }
    } windowGuard{ window };
    window->setClassName( "ProjectSettings" );
    window->load( nullptr );
    window->setWindowVisible( false );
    scripts->loadScriptFromString( R"(
        function ProjectSettings:projectSettingsIntegrationStatus()
            self.window:setName(self.statusMessage)
        end
    )" );
    BOOST_REQUIRE( window->getInvoker() );
    window->getInvoker()->callObjectMember( "projectSettingsIntegrationStatus" );
    auto snapshot = window->getProperties();
    BOOST_REQUIRE( snapshot );
    BOOST_REQUIRE( snapshot->hasProperty( "graphics.frameRateLimit" ) );
    BOOST_REQUIRE_MESSAGE( snapshot->getProperty( "project.name" ) == "Lua settings project",
                           "Project settings read failed: " << window->getName() );
    BOOST_CHECK_EQUAL( snapshot->getProperty( "project.company" ), "" );
    BOOST_CHECK_EQUAL( DataUtil::toString( project->getProperties().get(), true ), beforeOpen );
    BOOST_CHECK( !project->isDirty() );

    scripts->loadScriptFromString( R"(
        function ProjectSettings:projectSettingsIntegrationProbe()
            assert(#self.controlBindings == 130)
            assert(self.capabilities.cloudSave == false)
            assert(not self.cloudSaveControl:isEnabled())
            assert(not self.reloadRendererButton:isEnabled())
            assert(self.cloudSaveControl:getLabel():find("(", 1, true))
            self.projectNameControl:setText("Applied through Lua controls")
            self.companyControl:setText("")
            self.masterVolumeControl:setValue(25)
            local changed = Parameters()
            changed:add(IEvent.UI)
            changed:add(IEvent.handlePropertyChanged)
            changed:addAsInt(0)
            changed:addAsObject(self.projectNameControl)
            self:handleEvent(changed)
            assert(self.settings.projectName == "Applied through Lua controls")
            local click = Parameters()
            click:add(IEvent.UI)
            click:add(IEvent.handleSelection)
            click:addAsInt(0)
            click:addAsObject(self.applyButton)
            local perform = self.performAction
            local actionSucceeded = false
            self.performAction = function(editor, action, ...)
                actionSucceeded = perform(editor, action, ...)
                return actionSucceeded
            end
            self:handleEvent(click)
            self.performAction = perform
            assert(actionSucceeded, self.statusMessage)
            self.window:setName("Lua apply button dispatched")
        end
        function ProjectSettings:projectSettingsIntegrationSave()
            self.projectNameControl:setText("Saved through Lua controls")
            local click = Parameters()
            click:add(IEvent.UI)
            click:add(IEvent.handleSelection)
            click:addAsInt(0)
            click:addAsObject(self.saveButton)
            local perform = self.performAction
            local actionSucceeded = false
            self.performAction = function(editor, action, ...)
                actionSucceeded = perform(editor, action, ...)
                return actionSucceeded
            end
            self:handleEvent(click)
            self.performAction = perform
            assert(actionSucceeded, self.statusMessage)
            self.window:setName("Lua save button dispatched")
        end
    )" );
    BOOST_REQUIRE( window->getInvoker() );
    window->getInvoker()->callObjectMember( "projectSettingsIntegrationProbe" );
    BOOST_REQUIRE_EQUAL( window->getName(), "Lua apply button dispatched" );
    BOOST_CHECK_EQUAL( project->getLabel(), "Applied through Lua controls" );
    BOOST_CHECK_EQUAL( project->getProperties()->getProperty( "project.company" ), "" );
    BOOST_CHECK_EQUAL( project->getProperties()->getPropertyAsInt( "sound.masterVolume" ), 25 );
    BOOST_CHECK( project->isDirty() );
    BOOST_REQUIRE( application->getSoundManager() );
    BOOST_CHECK_CLOSE( application->getSoundManager()->getVolume(), 0.25f, 0.01f );
    Project beforeSave;
    beforeSave.loadFromFile( project->getFilePath() );
    BOOST_CHECK_EQUAL( beforeSave.getLabel(), "Lua settings project" );

    window->getInvoker()->callObjectMember( "projectSettingsIntegrationSave" );
    BOOST_REQUIRE_EQUAL( window->getName(), "Lua save button dispatched" );
    BOOST_CHECK( !project->isDirty() );
    Project saved;
    saved.loadFromFile( project->getFilePath() );
    BOOST_CHECK_EQUAL( saved.getLabel(), "Saved through Lua controls" );
    BOOST_CHECK_EQUAL( saved.getProperties()->getPropertyAsInt( "sound.masterVolume" ), 25 );

    // Hidden settings windows must accept state restoration without applying
    // edits to the current project or writing its file.
    auto restored = window->getProperties();
    restored->setProperty( "project.company", "Hidden snapshot company" );
    window->setProperties( restored );
    BOOST_CHECK_EQUAL( window->getProperties()->getProperty( "project.company" ),
                       "Hidden snapshot company" );
    BOOST_CHECK_EQUAL( project->getProperties()->getProperty( "project.company" ), "" );
    BOOST_CHECK( !project->isDirty() );
    window->unload( nullptr );
}

BOOST_AUTO_TEST_CASE( editor_input_window_live_values_test )
{
    EditorApplicationGuard app;
    app.loadSingleThreaded();

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );
    BOOST_CHECK( !applicationManager->isPlaying() );
    auto editorUI = EditorManager::getSingletonPtr()->getUI();
    auto window = editorUI->getInputManagerWindow();
    BOOST_REQUIRE( window );
    BOOST_REQUIRE( window->isLoaded() );
    BOOST_REQUIRE( window->getParentWindow() );

    // Supply deterministic device state without depending on connected hardware.
    auto inputManager = applicationManager->getInputDeviceManager();
    BOOST_REQUIRE( inputManager );
    auto originalJoysticks = inputManager->getJoysticks();
    auto joystick = make_ptr<Joystick>();
    joystick->setName( "Input window test joystick" );
    joystick->setNumAxes( 2 );
    joystick->setNumButtons( 2 );
    inputManager->setJoysticks( Array<SmartPtr<IJoystick>>{ joystick } );

    // Tab bars keep their tab items separately from getChildren(). A test-only
    // subclass exposes the real device list through the host's parent pointer.
    applicationManager->getScriptManager()->loadScriptFromString( R"(
        class 'InputWindowLiveValuesProbe' (InputManager)
        function InputWindowLiveValuesProbe:__init(window)
            InputManager.__init(self, window)
        end
        function InputWindowLiveValuesProbe:load()
            InputManager.load(self)
            self.window:setParent(self.deviceListWindow)
        end
    )" );
    window->unload( nullptr );
    window->setClassName( "InputWindowLiveValuesProbe" );
    window->load( nullptr );
    auto deviceWindow = window->getParent();
    BOOST_REQUIRE( deviceWindow );
    window->setParent( nullptr );
    window->setWindowVisible( true );

    std::function<bool( SmartPtr<ui::IUIElement>, const String & )> containsText;
    containsText = [&containsText]( SmartPtr<ui::IUIElement> element, const String &expected ) {
        if( auto text = workphone::dynamic_pointer_cast<ui::IUIText>( element ) )
        {
            if( text->getText() == expected )
            {
                return true;
            }
        }
        for( auto child : element->getChildren() )
        {
            if( containsText( child, expected ) )
            {
                return true;
            }
        }
        return false;
    };
    const auto checkText = [&]( const String &expected ) {
        // Text setters publish their state on the graphics task.
        const auto previousTask = Thread::getCurrentTask();
        Thread::setCurrentTask( applicationManager->getGraphicsSystem()->getStateTask() );
        applicationManager->getStateManager()->update();
        Thread::setCurrentTask( previousTask );
        BOOST_CHECK_MESSAGE( containsText( deviceWindow, expected ),
                             "Missing input window text: " << expected );
    };

    checkText( "X Axis: 0.00" );
    checkText( "Pressed Buttons: none" );

    joystick->updateAxisValue( 0, 0.75f );
    joystick->updateAxisValue( 1, -0.5f );
    joystick->updateButtonState( 1, true );
    app.updateApplication();
    checkText( "X Axis: 0.75" );
    checkText( "Y Axis: -0.50" );
    checkText( String( "Pressed Buttons: " ) + joystick->getButtonName( 1 ) );

    joystick->updateAxisValue( 0, 0.0f );
    joystick->updateButtonState( 1, false );
    app.updateApplication();
    checkText( "X Axis: 0.00" );
    checkText( "Pressed Buttons: none" );

    window->setWindowVisible( false );
    joystick->updateAxisValue( 0, -1.0f );
    app.updateApplication();
    checkText( "X Axis: 0.00" );
    window->setWindowVisible( true );
    checkText( "X Axis: -1.00" );

    inputManager->setJoysticks( Array<SmartPtr<IJoystick>>() );
    app.updateApplication();
    checkText( "Joysticks: none connected" );
    inputManager->setJoysticks( originalJoysticks );
}

BOOST_AUTO_TEST_CASE( editor_window_callbacks_disconnect_test )
{
    EditorApplicationGuard app;
    app.load();

    auto window = make_ptr<EditorWindow>();
    window->load( nullptr );
    auto listener =
        workphone::dynamic_pointer_cast<EditorWindow::UIListener>( window->getEventListener() );
    auto target =
        workphone::dynamic_pointer_cast<EditorWindow::UIDropTarget>( window->getWindowDropTarget() );
    BOOST_REQUIRE( listener );
    BOOST_REQUIRE( target );
    BOOST_CHECK( listener->getOwner() == window );
    BOOST_CHECK( target->getOwner() == window );

    window->unload( nullptr );
    window = nullptr;
    // Controls may retain callbacks beyond the lifetime of their window.
    BOOST_CHECK( !listener->getOwner() );
    BOOST_CHECK( !target->getOwner() );
}

BOOST_AUTO_TEST_CASE( editor_ui_control_unload_test )
{
    EditorApplicationGuard app;
    app.loadSingleThreaded();
    auto manager = core::IApplicationManager::instance();
    BOOST_REQUIRE( manager );
    auto ui = manager->getUI();
    BOOST_REQUIRE( ui );
    auto scripts = manager->getScriptManager();
    BOOST_REQUIRE( scripts );
    // Keep native UI owners in Lua globals until the VM closes. Shutdown must
    // collect these before freeing the UI factory pools.
    scripts->loadScriptFromString( R"(
        editorShutdownWindow = IApplicationManager.instance():getUI():addElement(IUIWindow.typeInfo())
        assert(editorShutdownWindow ~= nil)
        editorShutdownButton = IApplicationManager.instance():getUI():addElement(IUIButton.typeInfo())
        assert(editorShutdownButton ~= nil)
        editorShutdownWindow:addChild(editorShutdownButton)
        IApplicationManager.instance():getUI():getApplication():setName("Lua shutdown owners retained")
    )" );
    BOOST_REQUIRE( ui->getApplication() );
    BOOST_CHECK_EQUAL( ui->getApplication()->getName(), "Lua shutdown owners retained" );
    for( const auto type : { ui::IUIButton::typeInfo(), ui::IUILabelTogglePair::typeInfo(),
                             ui::IUIProfileWindow::typeInfo(), ui::IUIProfilerWindow::typeInfo() } )
    {
        BOOST_TEST_CONTEXT( "UI control type " << type )
        {
            auto parent = ui->addElementByType<ui::IUIWindow>();
            auto control = ui->addElement( type );
            auto child = ui->addElementByType<ui::IUIText>();
            BOOST_REQUIRE( parent );
            BOOST_REQUIRE( control );
            BOOST_REQUIRE( child );
            parent->addChild( control );
            control->addChild( child );
            control->setScriptData( make_ptr<ISharedObject>() );
            BOOST_REQUIRE( control->getParent() == parent );
            control->unload( nullptr );
            BOOST_CHECK( !control->getParent() );
            BOOST_CHECK( control->getChildren().empty() );
            BOOST_CHECK( !control->getScriptData() );
            ui->removeElement( control );
            ui->removeElement( child );
            ui->removeElement( parent );
        }
    }
}

BOOST_AUTO_TEST_CASE( editor_material_selection_visibility_test )
{
    EditorApplicationGuard app;
    app.load();

    auto applicationManager = core::IApplicationManager::instance();
    BOOST_REQUIRE( applicationManager );
    auto editorUI = EditorManager::getSingletonPtr()->getUI();
    auto objectWindow = editorUI->getObjectWindow();
    auto actorWindow = editorUI->getActorWindow();
    BOOST_REQUIRE( objectWindow );
    BOOST_REQUIRE( actorWindow );

    auto gameManager = applicationManager->getGameManager();
    auto actor = gameManager->createActor();
    auto material = actor->addComponent<scene::Material>();
    auto selectionManager = applicationManager->getSelectionManager();
    selectionManager->clearSelection();
    selectionManager->addSelectedObject( actor );
    objectWindow->updateSelection();

    selectionManager->clearSelection();
    selectionManager->addSelectedObject( material );
    objectWindow->updateSelection();
    BOOST_CHECK( actorWindow->isWindowVisible() );
    BOOST_REQUIRE( actorWindow->getParentWindow() );
    BOOST_CHECK( actorWindow->getParentWindow()->isVisible() );

    auto children = actorWindow->getParentWindow()->getChildren();
    auto foundMaterialEditor = false;
    for( auto child : children )
    {
        for( auto content : child->getChildren() )
        {
            auto window = workphone::dynamic_pointer_cast<ui::IUIWindow>( content );
            if( window && window->getLabel() == "Material Editor" )
            {
                foundMaterialEditor = true;
                BOOST_CHECK( child->isVisible() );
                BOOST_CHECK( window->isVisible() );
                BOOST_CHECK( !window->getChildren().empty() );
            }
        }
    }
    BOOST_CHECK( foundMaterialEditor );

    // A material file uses the separate Lua panel hosted directly by ObjectWindow.
    // The panel should remain visible even when its resource has not loaded yet.
    auto file = make_ptr<FileSelection>();
    file->setFilePath( "material-visibility-test.mat" );
    selectionManager->clearSelection();
    selectionManager->addSelectedObject( file );
    objectWindow->updateSelection();
    BOOST_CHECK( !actorWindow->isWindowVisible() );
    auto visibleFileEditor = false;
    for( auto child : objectWindow->getParentWindow()->getChildren() )
    {
        for( auto content : child->getChildren() )
        {
            auto window = workphone::dynamic_pointer_cast<ui::IUIWindow>( content );
            if( window && window->getLabel() == "Material Editor" )
            {
                visibleFileEditor = child->isVisible() && window->isVisible();
            }
        }
    }
    BOOST_CHECK( visibleFileEditor );

    auto resource = make_ptr<render::Material>();
    BOOST_CHECK( resource->getTechniques().empty() );
    BOOST_CHECK_EQUAL( resource->getBlendMode(), 0u );
    BOOST_CHECK_EQUAL( resource->getCullMode(), 2u );
    selectionManager->clearSelection();
    selectionManager->addSelectedObject( resource );
    applicationManager->getScriptManager()->loadScriptFromString( R"(
        local selection = IApplicationManager.instance():getSelectionManager():getSelection()
        local material = selection:at(0)
        assert(type(material.getTexture) == "function")
        material:setName("Material resource methods verified")
    )" );
    BOOST_CHECK_EQUAL( resource->getName(), "Material resource methods verified" );
    objectWindow->updateSelection();
    BOOST_CHECK( !actorWindow->isWindowVisible() );
    auto visibleResourceEditor = false;
    for( auto child : objectWindow->getParentWindow()->getChildren() )
    {
        for( auto content : child->getChildren() )
        {
            auto window = workphone::dynamic_pointer_cast<ui::IUIWindow>( content );
            if( window && window->getLabel() == "Material Editor" )
            {
                visibleResourceEditor = child->isVisible() && window->isVisible();
            }
        }
    }
    BOOST_CHECK( visibleResourceEditor );
    selectionManager->clearSelection();
    gameManager->destroyActor( actor );
}

BOOST_AUTO_TEST_CASE( editor_material_parameters_test )
{
    EditorApplicationGuard app;
    app.loadSingleThreaded();
    auto applicationManager = core::IApplicationManager::instance();
    auto material = workphone::dynamic_pointer_cast<render::IMaterial>(
        applicationManager->getGraphicsSystem()->getMaterialManager()->create(
            "EditorMaterialParametersTest" ) );
    BOOST_REQUIRE( material );
    material->load( nullptr );
    auto selection = applicationManager->getSelectionManager();
    selection->clearSelection();
    selection->addSelectedObject( material );
    auto scripts = applicationManager->getScriptManager();
    scripts->loadScriptFromString( R"(
        class 'EditorMaterialParametersProbe' (MaterialEditor)
        function EditorMaterialParametersProbe:__init(window)
            MaterialEditor.__init(self, window)
            self.material = IApplicationManager.instance():getSelectionManager():getSelection():at(0)
            self:applySettingToMaterial({key="roughness", valueType="float", setterName="setRoughness"}, 0.73)
            assert(math.abs(self.material:getRoughness() - 0.73) < 0.0001)
            self:applySettingToMaterial({key="normalStrength", valueType="float", setterName="setNormalStrength"}, 0.4)
            assert(math.abs(self.material:getNormalStrength() - 0.4) < 0.0001)
            local cullBinding = {key="cullMode", valueType="uint", setterName="setCullMode"}
            for option = 0, 2 do
                self:applySettingToMaterial(cullBinding, option)
                assert(self.material:getCullMode() == ({2, 3, 1})[option + 1])
                self:syncDerivedSettingsFromMaterial()
                assert(self.materialSettings.cullMode == option)
            end
            self:applySettingToMaterial({key="doubleSided", valueType="bool"}, false)
            assert(self.material:getCullMode() == 2)
            self:applySettingToMaterial({key="doubleSided", valueType="bool"}, true)
            assert(self.material:getCullMode() == 1)
            window:setName("Material parameters verified")
        end
    )" );
    SmartPtr<ISharedObject> owner = make_ptr<ScriptWindow>();
    scripts->createObject( "EditorMaterialParametersProbe", owner );
    BOOST_CHECK_EQUAL( owner->getName(), "Material parameters verified" );
    scripts->destroyObject( owner );
    selection->clearSelection();
}

BOOST_AUTO_TEST_CASE( editor_material_control_events_test )
{
    struct TestProject
    {
        std::filesystem::path path =
            std::filesystem::temp_directory_path() /
            ( "lioncat-material-" + std::string( StringUtil::getUUID().c_str() ) );

        TestProject()
        {
            std::filesystem::create_directories( path / "Cache" );
        }

        ~TestProject()
        {
            std::error_code ignored;
            std::filesystem::remove_all( path, ignored );
        }
    } project;

    EditorApplicationGuard app;
    app.loadSingleThreaded();
    auto application = core::IApplicationManager::instance();
    const String projectRoot( project.path.generic_string().c_str() );
    application->setProjectPath( projectRoot );
    application->setCachePath( projectRoot + "/Cache" );
    auto resources = application->getResourceDatabase();
    auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() );
    BOOST_REQUIRE( catalog );
    catalog->unload( nullptr );
    BOOST_REQUIRE( catalog->setProjectRoot( projectRoot ) );
    catalog->loadFromFile( projectRoot + "/asset.db" );
    const String materialPath = projectRoot + "/editor-material-control-events-test.mat";
    auto fileSystem = application->getFileSystem();
    fileSystem->writeAllText( materialPath, R"({
        "materialType": 0, "schemes": [{"passes": [{
            "roughness": 0.21, "metalness": 0.1,
            "diffuse": {"x":0.3,"y":0.5,"z":0.7,"w":1},
            "specular": {"x":0.04,"y":0.04,"z":0.04,"w":1}
        }]}]
    })" );
    auto material = workphone::dynamic_pointer_cast<render::IMaterial>(
        application->getGraphicsSystem()->getMaterialManager()->create( materialPath ) );
    BOOST_REQUIRE( material );
    resources->addResource( material );
    BOOST_REQUIRE( resources->hasResource( material ) );
    // ResourceDatabase::loadResource may return a newly allocated material.
    // The real panel must prepare it before accepting edits.
    BOOST_CHECK( !material->isLoaded() );
    auto selection = application->getSelectionManager();
    selection->clearSelection();
    auto file = make_ptr<FileSelection>();
    file->setFilePath( materialPath );
    selection->addSelectedObject( file );
    application->getGraphicsSystem()->loadObject( material, true );
    application->getScriptManager()->loadScriptFromString( R"(
        class 'MaterialControlEventsProbe' (MaterialEditor)
        function MaterialControlEventsProbe:__init(window)
            MaterialEditor.__init(self, window)
        end
        function MaterialControlEventsProbe:load()
            MaterialEditor.load(self)
            self.probeWindow = IApplicationManager.instance():getUI():addElement(IUIWindow.typeInfo())
            self.probeWindow:addChild(self.roughnessSlider)
            self.probeWindow:addChild(self.albedoColour)
            self.probeWindow:addChild(self.applyButton)
            self.probeWindow:addChild(self.metallicColour)
            self.probeWindow:addChild(self.saveButton)
            self.probeWindow:addChild(self.uvProjectionDropdown)
            self.window:setParent(self.probeWindow)
        end
    )" );
    auto window = make_ptr<ScriptWindow>();
    window->setClassName( "MaterialControlEventsProbe" );
    window->load( nullptr );
    window->setWindowVisible( true );
    window->updateSelection();
    BOOST_CHECK( material->isLoaded() );
    BOOST_CHECK_CLOSE( material->getRoughness(), 0.21f, 0.01f );
    BOOST_REQUIRE( window->getParent() );
    auto controls = window->getParent()->getChildren();
    BOOST_REQUIRE_EQUAL( controls.size(), 6u );
    auto projection = workphone::dynamic_pointer_cast<ui::IUIDropdown>( controls[5] );
    BOOST_REQUIRE( projection );
    projection->setSelectedOption( 3u );
    // ImGuiApplication sends dropdown selections directly to control listeners.
    for( auto listener : projection->getObjectListeners() )
    {
        listener->handleEvent( EventType::UI, IEvent::handleSelection,
                               Array<Parameter>{ Parameter( 3u ) }, projection, projection, nullptr );
    }
    BOOST_CHECK_EQUAL( material->getUVProjection(), 3u );
    auto slider = workphone::dynamic_pointer_cast<ui::IUILabelSliderPair>( controls[0] );
    BOOST_REQUIRE( slider );
    slider->setValue( 0.73f );
    for( auto listener : slider->getObjectListeners() )
    {
        listener->handleEvent( EventType::UI, IEvent::handleValueChanged,
                               Array<Parameter>{ Parameter( 0.73f ) }, slider, slider, nullptr );
    }
    BOOST_CHECK_CLOSE( material->getRoughness(), 0.73f, 0.01f );
    auto colour = workphone::dynamic_pointer_cast<ui::IUIColourPicker>( controls[1] );
    BOOST_REQUIRE( colour );
    colour->setColour( ColourF( 0.2f, 0.4f, 0.6f, 1.0f ) );
    for( auto listener : colour->getObjectListeners() )
    {
        listener->handleEvent(
            EventType::UI, IEvent::handleValueChanged,
            Array<Parameter>{ Parameter( 0.2f ), Parameter( 0.4f ), Parameter( 0.6f ) }, colour, colour,
            nullptr );
    }
    BOOST_CHECK_CLOSE( material->getDiffuse().r, 0.2f, 0.01f );
    BOOST_CHECK_CLOSE( material->getDiffuse().g, 0.4f, 0.01f );
    BOOST_CHECK_CLOSE( material->getDiffuse().b, 0.6f, 0.01f );
    auto metallicColour = workphone::dynamic_pointer_cast<ui::IUIColourPicker>( controls[3] );
    BOOST_REQUIRE( metallicColour );
    BOOST_CHECK_CLOSE( metallicColour->getColour().r, 0.2f, 0.01f );
    // Apply must dispatch its action, including values without a change event.
    slider->setValue( 0.61f );
    auto apply = controls[2];
    application->triggerEvent( EventType::UI, IEvent::handleSelection, {}, apply, nullptr, nullptr,
                               false, Thread::Application_Flag );
    BOOST_CHECK_CLOSE( material->getRoughness(), 0.61f, 0.01f );
    BOOST_CHECK_CLOSE( material->getDiffuse().r, 0.2f, 0.01f );
    BOOST_CHECK_CLOSE( material->getDiffuse().g, 0.4f, 0.01f );
    BOOST_CHECK_CLOSE( material->getDiffuse().b, 0.6f, 0.01f );
    // A later queue entry must not reload the saved file over unsaved edits.
    material->setRoughness( 0.84f );
    application->getGraphicsSystem()->update();
    BOOST_CHECK_CLOSE( material->getRoughness(), 0.84f, 0.01f );

    // The actor's material component must edit the resource assigned to its mesh.
    auto actor = application->getGameManager()->createActor();
    auto component = actor->addComponent<scene::Material>();
    component->setMaterial( material );
    auto renderer = actor->addComponent<scene::MeshRenderer>();
    auto mesh = application->getGraphicsSystem()
                    ->getGraphicsScene()
                    ->addGraphicsObjectByType<render::IGraphicsMesh>();
    renderer->setGraphicsObject( mesh );
    renderer->updateMaterials();
    BOOST_CHECK( mesh->getMaterial( 0 ) == material );
    selection->clearSelection();
    selection->addSelectedObject( component );
    window->updateSelection();
    slider->setValue( 0.42f );
    for( auto listener : slider->getObjectListeners() )
    {
        listener->handleEvent( EventType::UI, IEvent::handleValueChanged,
                               Array<Parameter>{ Parameter( 0.42f ) }, slider, slider, nullptr );
    }
    BOOST_CHECK_CLOSE( mesh->getMaterial( 0 )->getRoughness(), 0.42f, 0.01f );
    slider->setValue( 0.36f );
    application->triggerEvent( EventType::UI, IEvent::handleSelection, {}, controls[4], nullptr, nullptr,
                               false, Thread::Application_Flag );
    BOOST_CHECK_CLOSE( mesh->getMaterial( 0 )->getRoughness(), 0.36f, 0.01f );
    window->setParent( nullptr );
    window->unload( nullptr );
    selection->clearSelection();
    application->getGameManager()->destroyActor( actor );
    material->reload( nullptr );
    BOOST_CHECK_CLOSE( material->getRoughness(), 0.36f, 0.01f );
    BOOST_CHECK_CLOSE( material->getDiffuse().r, 0.2f, 0.01f );
    BOOST_CHECK_EQUAL( material->getUVProjection(), 3u );
    fileSystem->deleteFile( materialPath );
}

BOOST_AUTO_TEST_CASE( editor_cubemap_face_preview_test )
{
    EditorApplicationGuard app;
    app.loadSingleThreaded();
    auto application = core::IApplicationManager::instance();
    auto window = make_ptr<PropertiesWindow>();
    window->load( nullptr );
    auto actor = application->getGameManager()->createActor();
    auto component = actor->addComponent<scene::Cubemap>();
    auto probe = make_ptr<render::GraphicsCubemap>();
    component->setRenderCubemap( probe );

    SmartPtr<ui::IUIWindow> preview;
    for( auto child : window->getParentWindow()->getChildren() )
    {
        auto childWindow = workphone::dynamic_pointer_cast<ui::IUIWindow>( child );
        if( childWindow && childWindow->getLabel() == "Cubemap Texture Preview" )
        {
            preview = childWindow;
        }
    }
    BOOST_REQUIRE( preview );
    window->updateSelection( component );
    BOOST_CHECK( preview->isVisible() );
    auto status = workphone::dynamic_pointer_cast<ui::IUIText>( preview->getChildren()[0] );
    BOOST_REQUIRE( status );
    BOOST_CHECK( status->isVisible() );
    BOOST_CHECK( status->getText().find( "No cubemap texture" ) != String::npos );

    auto texture = make_ptr<InspectorCubemapTexture>();
    texture->setName( "Inspector Test Cubemap" );
    Array<SmartPtr<render::ITexture>> faces;
    for( size_t i = 0; i < 6; ++i )
    {
        faces.push_back( make_ptr<render::Texture>() );
    }
    texture->faces = faces;
    probe->setTexture( texture );
    auto selection = application->getSelectionManager();
    selection->clearSelection();
    selection->addSelectedObject( component );
    // The texture can become available after selection (e.g. when baking completes).
    window->update();
    BOOST_CHECK( status->getText().find( "Inspector Test Cubemap" ) != String::npos );
    const char *labels[] = {
        "Front (-Z)", "Back (+Z)", "Left (-X)", "Right (+X)", "Up (+Y)", "Down (-Y)"
    };
    Array<SmartPtr<ui::IUIImage>> images;
    for( size_t i = 0; i < 6; ++i )
    {
        auto panel = preview->getChildren()[i + 1];
        BOOST_CHECK( panel->isVisible() );
        auto label = workphone::dynamic_pointer_cast<ui::IUIText>( panel->getChildren()[0] );
        auto image = workphone::dynamic_pointer_cast<ui::IUIImage>( panel->getChildren()[1] );
        BOOST_REQUIRE( label );
        BOOST_REQUIRE( image );
        BOOST_CHECK( label->isVisible() );
        BOOST_CHECK( image->isVisible() );
        BOOST_CHECK_EQUAL( label->getText(), labels[i] );
        BOOST_CHECK( image->getTexture() == faces[i] );
        images.push_back( image );
    }
    window->updateSelection( actor );
    BOOST_CHECK( preview->isVisible() );

    probe->setTexture( make_ptr<render::Texture>() );
    window->update();
    BOOST_CHECK( status->getText().find( "unavailable" ) != String::npos );
    for( auto image : images )
    {
        BOOST_CHECK( !image->getTexture() );
    }
    probe->setTexture( nullptr );
    window->update();
    BOOST_CHECK( status->getText().find( "No cubemap texture" ) != String::npos );

    probe->setTexture( texture );
    window->update();
    selection->clearSelection();
    window->update();
    BOOST_CHECK( !preview->isVisible() );
    for( auto image : images )
    {
        BOOST_CHECK( !image->getTexture() );
    }
    window->updateSelection( make_ptr<Properties>() );
    BOOST_CHECK( !preview->isVisible() );
    window->updateSelection( component );
    window->unload( nullptr );
    for( auto image : images )
    {
        BOOST_CHECK( !image->getTexture() );
    }
    application->getGameManager()->destroyActor( actor );
}

BOOST_AUTO_TEST_CASE( editor_material_shader_reload_actions_test )
{
    EditorApplicationGuard app;
    app.loadSingleThreaded();
    auto application = core::IApplicationManager::instance();
    auto material = workphone::dynamic_pointer_cast<render::IMaterial>(
        application->getGraphicsSystem()->getMaterialManager()->create("EditorShaderReloadActions"));
    BOOST_REQUIRE(material);
    const auto requestFor = [](const String &action) {
        auto request = make_ptr<Properties>(); request->setProperty("action",action); return request;
    };
    auto missingMaterial = requestFor("read");
    processMaterialShaderReloadAction(nullptr,missingMaterial);
    BOOST_CHECK(!missingMaterial->getPropertyAsBool("success"));
    BOOST_CHECK(missingMaterial->getProperty("status").find("Select a material") != String::npos);
    auto unavailable = requestFor("read");
    ScriptWindow bridge;
    bridge.handleEvent(EventType::UI, StringUtil::getHash32("materialShaderReloadAction"),{},material,unavailable,nullptr);
    BOOST_CHECK(!unavailable->getPropertyAsBool("success"));
    BOOST_CHECK(!unavailable->getPropertyAsBool("available"));
    BOOST_CHECK(!unavailable->getProperty("status").empty());
    auto malformed = requestFor("configureFiles");
    processMaterialShaderReloadAction(material,malformed);
    BOOST_CHECK(!malformed->getPropertyAsBool("success"));
    BOOST_CHECK(!malformed->getPropertyAsBool("queued"));
    BOOST_CHECK(malformed->getProperty("status").find("pixel shader") != String::npos);
    auto unknown = requestFor("pretendSuccess");
    processMaterialShaderReloadAction(material,unknown);
    BOOST_CHECK(!unknown->getPropertyAsBool("success"));
    BOOST_CHECK(unknown->getProperty("status").find("Unknown") != String::npos);
#if WP_GRAPHICS_SYSTEM_CLAW && WP_BUILD_RENDERER_DX11
    auto reloadUnavailable = requestFor("reload");
    processMaterialShaderReloadAction(material,reloadUnavailable);
    BOOST_CHECK(!reloadUnavailable->getPropertyAsBool("success"));
    BOOST_CHECK(!reloadUnavailable->getPropertyAsBool("queued"));
    const auto missingPath = (std::filesystem::temp_directory_path() /
        ("missing-editor-shader-"+std::string(StringUtil::getUUID().c_str())+".hlsl")).generic_string();
    auto configure = requestFor("configureFiles");
    configure->setProperty("pixelFile",String(missingPath.c_str()));
    processMaterialShaderReloadAction(material,configure);
    BOOST_REQUIRE_MESSAGE(configure->getPropertyAsBool("success"),configure->getProperty("status"));
    BOOST_CHECK(configure->getPropertyAsBool("queued"));
    BOOST_CHECK(configure->getPropertyAsBool("pending"));
    BOOST_CHECK(!configure->getPropertyAsBool("runtimeSuccess"));
    // Queueing must not compile on the UI thread. Owner update publishes the
    // file failure, and the user can still request another reload afterwards.
    const auto ownerUpdate = [&] {
        struct RestoreTask
        {
            decltype(Thread::getCurrentTask()) previous = Thread::getCurrentTask();
            ~RestoreTask() { Thread::setCurrentTask(previous); }
        } restoreTask;
        Thread::setCurrentTask(TaskId::Render);
        application->getGraphicsSystem()->update();
    };
    ownerUpdate();
    auto failed = requestFor("read");
    processMaterialShaderReloadAction(material,failed);
    BOOST_REQUIRE_MESSAGE(failed->getPropertyAsBool("success"),failed->getProperty("status"));
    BOOST_CHECK(failed->getPropertyAsBool("available"));
    BOOST_CHECK(!failed->getPropertyAsBool("pending"));
    BOOST_CHECK(!failed->getPropertyAsBool("runtimeSuccess"));
    BOOST_CHECK(!failed->getProperty("diagnostic").empty() || !failed->getProperty("status").empty());
    auto retry = requestFor("reload");
    processMaterialShaderReloadAction(material,retry);
    BOOST_CHECK(retry->getPropertyAsBool("success"));
    BOOST_CHECK(retry->getPropertyAsBool("queued"));
    ownerUpdate();
#endif
}

BOOST_AUTO_TEST_CASE( editor_material_shader_reload_controls_test )
{
    EditorApplicationGuard app;
    app.loadSingleThreaded();
    auto application = core::IApplicationManager::instance();
    auto material = workphone::dynamic_pointer_cast<render::IMaterial>(
        application->getGraphicsSystem()->getMaterialManager()->create("EditorShaderReloadControls"));
    BOOST_REQUIRE(material);
    application->getGraphicsSystem()->loadObject(material,true);
    auto selection = application->getSelectionManager();
    selection->clearSelection(); selection->addSelectedObject(material);
    application->getScriptManager()->loadScriptFromString(R"(
        class 'MaterialShaderReloadControlsProbe' (MaterialEditor)
        function MaterialShaderReloadControlsProbe:__init(window)
            MaterialEditor.__init(self, window)
        end
        function MaterialShaderReloadControlsProbe:load()
            MaterialEditor.load(self)
            self.probeWindow = IApplicationManager.instance():getUI():addElement(IUIWindow.typeInfo())
            self.probeWindow:addChild(self.reloadShaderButton)
            self.probeWindow:addChild(self.shaderWatchToggle)
            self.probeWindow:addChild(self.shaderReloadStatusText)
            self.probeWindow:addChild(self.configureShaderFilesButton)
            self.probeWindow:addChild(self.shaderPathEntry)
            self.probeWindow:addChild(self.vertexShaderPathEntry)
            self.probeWindow:addChild(self.shaderReloadStatusButton)
            self.window:setParent(self.probeWindow)
        end
    )");
    auto window = make_ptr<ScriptWindow>();
    window->setClassName("MaterialShaderReloadControlsProbe");
    window->load(nullptr); window->setWindowVisible(true); window->updateSelection();
    BOOST_REQUIRE(window->getParent());
    const auto controls = window->getParent()->getChildren();
    BOOST_REQUIRE_EQUAL(controls.size(),7u);
    auto status = workphone::dynamic_pointer_cast<ui::IUIText>(controls[2]);
    auto pixel = workphone::dynamic_pointer_cast<ui::IUITextEntry>(controls[4]);
    BOOST_REQUIRE(status); BOOST_REQUIRE(pixel);
    BOOST_CHECK(!controls[0]->isEnabled());
    BOOST_CHECK(!controls[1]->isEnabled());
    BOOST_CHECK(!status->getText().empty());
    BOOST_CHECK(status->getText().find("requested") == String::npos);
    pixel->setText("");
    application->triggerEvent(EventType::UI,IEvent::handleSelection,{},controls[3],nullptr,nullptr,
        false,Thread::Application_Flag);
    BOOST_CHECK(status->getText().find("pixel shader") != String::npos);
    BOOST_CHECK(status->getText().find("requested") == String::npos);
    // Editing the path alone is a draft; the Use Shader Files control is the
    // explicit queue boundary and must read current text without a change event.
#if WP_GRAPHICS_SYSTEM_CLAW && WP_BUILD_RENDERER_DX11
    const String missing((std::filesystem::temp_directory_path() /
        ("missing-editor-ui-shader-"+std::string(StringUtil::getUUID().c_str())+".hlsl")).generic_string().c_str());
    pixel->setText(missing);
    application->triggerEvent(EventType::UI,IEvent::handleSelection,{},controls[3],nullptr,nullptr,
        false,Thread::Application_Flag);
    BOOST_CHECK(status->getText().find("pending") != String::npos);
#endif
    selection->clearSelection(); window->updateSelection();
    BOOST_CHECK(status->getText().find("Select a material") != String::npos);
    BOOST_CHECK(!controls[0]->isEnabled());
    BOOST_CHECK(!controls[3]->isEnabled());
    window->unload(nullptr);
}

#endif
