#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>

#include <EditorApplication.hpp>
#include <ui/ScriptWindow.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/ObjectWindow.hpp>
#include <ui/UIManager.hpp>
#include <editor/EditorManager.hpp>
#include <Workphone/System/FileSelection.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Input/Joystick.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>

#include <algorithm>
#include <functional>

#if WP_EDITOR_TESTS
#    include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::editor;

namespace
{
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
            window:setName("Editor owner methods verified")
        end
    )" );

    SmartPtr<ISharedObject> owner = make_ptr<ScriptWindow>();
    scriptManager->createObject( "EditorWindowOwnerProbe", owner );
    BOOST_CHECK_EQUAL( owner->getName(), "Editor owner methods verified" );
    scriptManager->destroyObject( owner );
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
            if( text->getText() == expected )
                return true;
        for( auto child : element->getChildren() )
            if( containsText( child, expected ) )
                return true;
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
    auto listener = workphone::dynamic_pointer_cast<EditorWindow::UIListener>( window->getEventListener() );
    auto target = workphone::dynamic_pointer_cast<EditorWindow::UIDropTarget>( window->getWindowDropTarget() );
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
    EditorApplicationGuard app;
    app.loadSingleThreaded();
    auto application = core::IApplicationManager::instance();
    const String materialPath = "editor-material-control-events-test.mat";
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
        listener->handleEvent( EventType::UI, IEvent::handleSelection,
            Array<Parameter>{ Parameter( 3u ) }, projection, projection, nullptr );
    BOOST_CHECK_EQUAL( material->getUVProjection(), 3u );
    auto slider = workphone::dynamic_pointer_cast<ui::IUILabelSliderPair>( controls[0] );
    BOOST_REQUIRE( slider );
    slider->setValue( 0.73f );
    for( auto listener : slider->getObjectListeners() )
        listener->handleEvent( EventType::UI, IEvent::handleValueChanged,
                              Array<Parameter>{ Parameter( 0.73f ) }, slider, slider, nullptr );
    BOOST_CHECK_CLOSE( material->getRoughness(), 0.73f, 0.01f );
    auto colour = workphone::dynamic_pointer_cast<ui::IUIColourPicker>( controls[1] );
    BOOST_REQUIRE( colour );
    colour->setColour( ColourF( 0.2f, 0.4f, 0.6f, 1.0f ) );
    for( auto listener : colour->getObjectListeners() )
        listener->handleEvent( EventType::UI, IEvent::handleValueChanged,
                              Array<Parameter>{ Parameter( 0.2f ), Parameter( 0.4f ), Parameter( 0.6f ) },
                              colour, colour, nullptr );
    BOOST_CHECK_CLOSE( material->getDiffuse().r, 0.2f, 0.01f );
    BOOST_CHECK_CLOSE( material->getDiffuse().g, 0.4f, 0.01f );
    BOOST_CHECK_CLOSE( material->getDiffuse().b, 0.6f, 0.01f );
    auto metallicColour = workphone::dynamic_pointer_cast<ui::IUIColourPicker>( controls[3] );
    BOOST_REQUIRE( metallicColour );
    BOOST_CHECK_CLOSE( metallicColour->getColour().r, 0.2f, 0.01f );
    // Apply must dispatch its action, including values without a change event.
    slider->setValue( 0.61f );
    auto apply = controls[2];
    application->triggerEvent( EventType::UI, IEvent::handleSelection, {}, apply, nullptr,
                               nullptr, false, Thread::Application_Flag );
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
    auto mesh = application->getGraphicsSystem()->getGraphicsScene()
                    ->addGraphicsObjectByType<render::IGraphicsMesh>();
    renderer->setGraphicsObject( mesh );
    renderer->updateMaterials();
    BOOST_CHECK( mesh->getMaterial( 0 ) == material );
    selection->clearSelection();
    selection->addSelectedObject( component );
    window->updateSelection();
    slider->setValue( 0.42f );
    for( auto listener : slider->getObjectListeners() )
        listener->handleEvent( EventType::UI, IEvent::handleValueChanged,
            Array<Parameter>{ Parameter( 0.42f ) }, slider, slider, nullptr );
    BOOST_CHECK_CLOSE( mesh->getMaterial( 0 )->getRoughness(), 0.42f, 0.01f );
    slider->setValue( 0.36f );
    application->triggerEvent( EventType::UI, IEvent::handleSelection, {}, controls[4], nullptr,
                               nullptr, false, Thread::Application_Flag );
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

#endif
