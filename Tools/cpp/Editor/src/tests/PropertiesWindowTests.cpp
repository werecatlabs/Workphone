#include <EditorPCH.hpp>
#include <Workphone/Workphone.hpp>

#if WP_EDITOR_TESTS
#    include <ui/PropertiesWindow.hpp>
#    include <ui/UIManager.hpp>
#    include <editor/EditorManager.hpp>
#    include <WPImGui/ImGuiPropertyGrid.hpp>
#    include <Workphone/Core/PropertiesBinarySerializer.hpp>
#    include <Workphone/Database/ResourceDatabase.hpp>
#    include <Workphone/Scene/Directors/MeshResourceDirector.hpp>
#    include <Workphone/System/ApplicationManager.hpp>
#    include <Workphone/System/FileSelection.hpp>
#    include <Workphone/System/FactoryManager.hpp>
#    include <Workphone/System/JobQueue.hpp>
#    include <Workphone/System/SelectionManager.hpp>
#    include <boost/test/unit_test.hpp>
#    include <stdexcept>
#    include <utility>

using namespace workphone;
using namespace workphone::editor;

namespace
{
    SmartPtr<Properties> copyInspectorData( SmartPtr<Properties> source )
    {
        if( !source ) return nullptr;
        auto copy = make_ptr<Properties>();
        String error;
        if( !PropertiesBinarySerializer::deserialize(
                PropertiesBinarySerializer::serialize( *source ), *copy, &error ) )
            throw std::runtime_error( error.c_str() );
        return copy;
    }

    SmartPtr<Properties> propertyGroup( const String &name )
    {
        auto group = make_ptr<Properties>();
        group->setName( name );
        return group;
    }

    SmartPtr<Properties> inspectorSchema( const String &owner )
    {
        auto root = propertyGroup( "Inspector settings" );
        root->setProperty( "value", s32( 1 ) );
        root->setProperty( "owner", owner );
        root->setProperty( "enabled", false );
        for( s32 i = 0; i < 2; ++i )
        {
            // Repeated sibling names must be addressed by occurrence as well
            // as name. Every hierarchy level deliberately shares a leaf name.
            auto group = propertyGroup( "group" );
            group->setProperty( "value", s32( 10 + i ) );
            auto details = propertyGroup( "details" );
            details->setProperty( "value", s32( 20 + i ) );
            details->getPropertyObject( "value" ).setAttribute( "units", "metres" );
            details->setProperty( "untouched", owner + StringUtil::toString( i ) );
            details->setButtonPressed( "Apply", false );
            group->addChild( details );
            root->addChild( group );
        }
        return root;
    }

    SmartPtr<Properties> detailGroup( SmartPtr<Properties> root, size_t occurrence = 1 )
    {
        auto groups = root->getChildrenByName( "group" );
        BOOST_REQUIRE_GT( groups.size(), occurrence );
        return groups[occurrence]->getChild( "details" );
    }

    class InspectorObject : public ISharedObject
    {
    public:
        explicit InspectorObject( SmartPtr<Properties> initial ) : data( std::move( initial ) ) {}

        SmartPtr<Properties> getProperties() const override
        {
            // The actual editor targets commonly return fresh trees. Tests
            // must not pass merely because the grid shares live target data.
            return copyInspectorData( data );
        }
        void setProperties( SmartPtr<Properties> properties ) override
        {
            ++sets;
            data = copyInspectorData( properties );
            for( auto group : data->getChildrenByName( "group" ) )
                if( auto details = group->getChild( "details" ); details && details->isButtonPressed( "Apply" ) )
                {
                    ++actions;
                    actionValue = details->getPropertyAsInt( "value" );
                }
        }

        SmartPtr<Properties> data;
        u32 sets = 0, actions = 0;
        s32 actionValue = 0;
    };

    class InspectorApplication : public core::ApplicationManager
    {
    public:
        Parameter triggerEvent( EventType type, hash_type value, const Array<Parameter> &arguments,
                                SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                SmartPtr<IEvent> event, bool sendNow, u32 taskFlags ) override
        {
            ++events;
            lastEvent = value;
            lastArguments = arguments;
            lastSender = sender;
            return {};
        }
        u32 events = 0;
        hash_type lastEvent = 0;
        Array<Parameter> lastArguments;
        SmartPtr<ISharedObject> lastSender;
    };

    class InspectorWindow : public PropertiesWindow
    {
    public:
        using PropertiesWindow::propertyChange;
    };

    class InspectorMeshDirector : public scene::MeshResourceDirector
    {
    public:
        void setProperties( SmartPtr<Properties> properties ) override
        {
            ++sets;
            lastApplied = copyInspectorData( properties );
            MeshResourceDirector::setProperties( properties );
        }
        void save() override
        {
            ++saves;
            savedEnabled = getStreamingOptions().enabled;
            lastSaved = getProperties();
        }
        void import() override
        {
            ++imports;
            importedEnabled = getStreamingOptions().enabled;
        }

        u32 sets = 0, saves = 0, imports = 0;
        bool savedEnabled = false, importedEnabled = false;
        SmartPtr<Properties> lastApplied, lastSaved;
    };

    class InspectorResources : public ResourceDatabase
    {
    public:
        SmartPtr<IBuildDirector> loadDirectorFromResourcePath( const String &path, u32 type ) override
        {
            ++loads;
            BOOST_CHECK_EQUAL( path, "isolated-city.glb" );
            BOOST_CHECK_EQUAL( type, scene::MeshResourceDirector::typeInfo() );
            return director;
        }
        SmartPtr<InspectorMeshDirector> director;
        u32 loads = 0;
    };

    struct InspectorFixture
    {
        SmartPtr<core::IApplicationManager> previousApplication = core::IApplicationManager::instance();
        SmartPtr<EditorManager> previousEditor = EditorManager::getSingleton();
        SmartPtr<InspectorApplication> application;
        SmartPtr<EditorManager> editor;
        SmartPtr<InspectorWindow> window;
        SmartPtr<ui::ImGuiPropertyGrid> grid;

        InspectorFixture()
        {
            // Editor's Boost entry point already loads TypeManager. Exercise
            // native property routing without starting plugins, render windows,
            // the script runtime or a real user project.
            application = make_ptr<InspectorApplication>();
            core::IApplicationManager::setInstance( application );
            application->setRunning( false );
            // Director::getProperties uses IFactoryManager::make_ptr. An empty
            // real manager uses that API's normal heap fallback for Properties;
            // no plugin/factory registration or render services are required.
            application->setFactoryManager( make_ptr<FactoryManager>() );
            application->setSelectionManager( make_ptr<SelectionManager>() );
            application->setJobQueue( make_ptr<JobQueue>() );
            editor = make_ptr<EditorManager>();
            EditorManager::setSingletonPtr( editor );
            editor->setUI( make_ptr<UIManager>() );
            window = make_ptr<InspectorWindow>();
            grid = make_ptr<ui::ImGuiPropertyGrid>();
            window->setPropertyGrid( grid );
        }
        ~InspectorFixture()
        {
            application->lastArguments.clear();
            application->lastSender = nullptr;
            window->setSelected( nullptr );
            window->setSelection( {} );
            window->setPropertyGrid( nullptr );
            grid = nullptr;
            window = nullptr;
            application->setResourceDatabase( nullptr );
            application->setSelectionManager( nullptr );
            application->setJobQueue( nullptr );
            application->setFactoryManager( nullptr );
            editor->setUI( nullptr );
            editor->unload( nullptr );
            EditorManager::setSingletonPtr( previousEditor );
            editor = nullptr;
            core::IApplicationManager::setInstance( previousApplication );
            application = nullptr;
        }
    };
}

BOOST_AUTO_TEST_CASE( editor_property_nested_mesh_import_settings_test )
{
    InspectorFixture fixture;
    auto resources = make_ptr<InspectorResources>();
    auto director = make_ptr<InspectorMeshDirector>();
    resources->director = director;
    fixture.application->setResourceDatabase( resources );
    auto progressive = director->getProgressiveMeshOptions();
    progressive.generationMode = MeshLodGenerationMode::Custom;
    director->setProgressiveMeshOptions( progressive );
    auto file = make_ptr<FileSelection>();
    file->setFilePath( "isolated-city.glb" );
    auto displayed = director->getProperties();
    fixture.grid->setProperties( displayed );
    auto streaming = displayed->getChild( "streamingOptions" );
    BOOST_REQUIRE( streaming );

    // Preserve changes made outside this displayed snapshot, too.
    director->setScale( 37.5f );
    fixture.window->propertyChange( streaming, file, "enabled", "true", false );
    BOOST_CHECK( director->getStreamingOptions().enabled );
    BOOST_CHECK_EQUAL( director->sets, 1u );
    BOOST_CHECK_EQUAL( director->saves, 1u );
    BOOST_CHECK( director->savedEnabled );
    BOOST_REQUIRE( director->lastApplied );
    BOOST_CHECK( !director->lastApplied->hasProperty( "enabled" ) );
    BOOST_CHECK_CLOSE( director->getScale(), 37.5f, 0.001f );
    BOOST_CHECK( director->getProgressiveMeshOptions() == progressive );

    fixture.window->propertyChange( streaming, file, "loadRadius", "2048", false );
    BOOST_CHECK_CLOSE( director->getStreamingOptions().loadRadius, 2048.0f, 0.001f );
    BOOST_CHECK_GE( director->getStreamingOptions().retainRadius, 2048.0f );
    scene::MeshResourceDirector restored;
    restored.setProperties( copyInspectorData( director->lastSaved ) );
    BOOST_CHECK( restored.getStreamingOptions().enabled );
    BOOST_CHECK_CLOSE( restored.getStreamingOptions().loadRadius, 2048.0f, 0.001f );
    BOOST_CHECK( restored.getProgressiveMeshOptions() == progressive );

    // Ordinary Save dispatches once, and Import sees the previously applied
    // nested values. Neither action may reset options from the stale grid root.
    const auto savesBefore = director->saves;
    fixture.window->propertyChange( displayed, file, "Save", "true", true );
    BOOST_CHECK_EQUAL( director->saves, savesBefore + 1 );
    fixture.window->propertyChange( displayed, file, "Import", "true", true );
    BOOST_CHECK_EQUAL( director->imports, 1u );
    BOOST_CHECK( director->importedEnabled );
    BOOST_CHECK( director->getProgressiveMeshOptions() == progressive );
}

BOOST_AUTO_TEST_CASE( editor_property_import_status_refresh_test )
{
    InspectorFixture fixture;
    auto resources = make_ptr<InspectorResources>();
    auto director = make_ptr<InspectorMeshDirector>();
    director->setResourcePath( "isolated-city.glb" );
    resources->director = director;
    fixture.application->setResourceDatabase( resources );
    auto file = make_ptr<FileSelection>();
    file->setFilePath( "isolated-city.glb" );
    fixture.window->setSelected( file );
    fixture.window->setDirty( false );
    auto displayed = director->getProperties();
    fixture.grid->setProperties( displayed );
    auto streaming = displayed->getChild( "streamingOptions" );
    BOOST_REQUIRE( streaming );
    streaming->setProperty( "loadRadius", 12345.0f );

    const auto request = director->beginImport();
    fixture.window->update();
    BOOST_CHECK_EQUAL( displayed->getProperty( scene::ResourceDirector::importStatusStr ),
                       director->getImportStatus() );
    BOOST_CHECK( displayed->getPropertyObject( scene::ResourceDirector::importStatusStr ).isReadOnly() );

    const auto longFailure = String( "Import failed: persistent group has 32 actors; limit is 20. " ) +
                             String( 512, 'x' );
    director->setImportStatus( request, longFailure );
    fixture.window->update();
    BOOST_CHECK_EQUAL( displayed->getProperty( scene::ResourceDirector::importStatusStr ),
                       director->getImportStatus() );
    BOOST_CHECK_EQUAL( displayed->getPropertyObject( scene::ResourceDirector::importStatusStr )
                           .getAttribute( "valueTooltip" ), "true" );
    BOOST_CHECK_LT( displayed->getPropertyObject( scene::ResourceDirector::importStatusStr )
                        .getAttribute( "description" ).size(), 256u );
    BOOST_CHECK( fixture.grid->getProperties() == displayed );
    BOOST_CHECK( displayed->getChild( "streamingOptions" ) == streaming );
    BOOST_CHECK_CLOSE( streaming->getPropertyAsFloat( "loadRadius" ), 12345.0f, 0.001f );
    BOOST_CHECK_EQUAL( director->sets, 0u );
    BOOST_CHECK_EQUAL( director->saves, 0u );
    BOOST_CHECK_EQUAL( director->imports, 0u );

    // A pending selection refresh must not copy a different resource's status
    // into the still-displayed snapshot.
    displayed->setProperty( scene::ResourceDirector::resourcePathStr, "another-city.glb", true );
    director->setImportStatus( request, "Import complete." );
    fixture.window->update();
    BOOST_CHECK_NE( displayed->getProperty( scene::ResourceDirector::importStatusStr ),
                    director->getImportStatus() );
}

BOOST_AUTO_TEST_CASE( editor_property_deep_groups_and_multiselection_test )
{
    InspectorFixture fixture;
    auto first = make_ptr<InspectorObject>( inspectorSchema( "first" ) );
    auto second = make_ptr<InspectorObject>( inspectorSchema( "second" ) );
    detailGroup( second->data )->setProperty( "value", s32( 99 ) );
    auto displayed = first->getProperties();
    fixture.grid->setProperties( displayed );
    auto owner = detailGroup( displayed );
    const auto type = owner->getPropertyObject( "value" ).getTypeName();
    // Metadata comes from each target, not from the incoming UI snapshot.
    owner->getPropertyObject( "value" ).setAttribute( "units", "stale units" );
    first->data->setProperty( "owner", "fresh first" );
    fixture.window->setSelection( { first, second } );
    fixture.window->propertyChange( owner, "value", "314", false );
    BOOST_CHECK_EQUAL( first->sets, 1u );
    BOOST_CHECK_EQUAL( second->sets, 1u );
    BOOST_CHECK_EQUAL( first->data->getProperty( "owner" ), "fresh first" );
    BOOST_CHECK_EQUAL( second->data->getProperty( "owner" ), "second" );
    for( auto target : { first, second } )
    {
        BOOST_CHECK_EQUAL( target->data->getPropertyAsInt( "value" ), 1 );
        BOOST_CHECK_EQUAL( detailGroup( target->data, 0 )->getPropertyAsInt( "value" ), 20 );
        auto edited = detailGroup( target->data );
        BOOST_CHECK_EQUAL( edited->getPropertyAsInt( "value" ), 314 );
        BOOST_CHECK_EQUAL( edited->getPropertyObject( "value" ).getTypeName(), type );
        BOOST_CHECK_EQUAL( edited->getPropertyObject( "value" ).getAttribute( "units" ), "metres" );
    }
    BOOST_CHECK_EQUAL( detailGroup( second->data )->getProperty( "untouched" ), "second1" );
    BOOST_CHECK_EQUAL( owner->getPropertyAsInt( "value" ), 21 );
}

BOOST_AUTO_TEST_CASE( editor_property_invalid_and_readonly_context_test )
{
    InspectorFixture fixture;
    auto target = make_ptr<InspectorObject>( inspectorSchema( "target" ) );
    auto displayed = target->getProperties();
    fixture.grid->setProperties( displayed );
    auto owner = detailGroup( displayed );
    fixture.window->propertyChange( copyInspectorData( owner ), target, "value", "99", false );
    fixture.window->propertyChange( owner, target, "missing", "99", false );
    BOOST_CHECK_EQUAL( target->sets, 0u );
    BOOST_CHECK( !detailGroup( target->data )->hasProperty( "missing" ) );

    detailGroup( target->data )->getPropertyObject( "value" ).setReadOnly( true );
    fixture.window->propertyChange( owner, target, "value", "99", false );
    BOOST_CHECK_EQUAL( target->sets, 0u );
    BOOST_CHECK_EQUAL( detailGroup( target->data )->getPropertyAsInt( "value" ), 21 );

    auto missingGroup = make_ptr<InspectorObject>( propertyGroup( "Inspector settings" ) );
    fixture.window->propertyChange( owner, missingGroup, "value", "99", false );
    BOOST_CHECK_EQUAL( missingGroup->sets, 0u );
    BOOST_CHECK( missingGroup->data->getChildren().empty() );

    // No context means the displayed root, and must still use the explicit
    // incoming value rather than its unchanged value in the grid snapshot.
    fixture.window->propertyChange( nullptr, target, "value", "77", false );
    BOOST_CHECK_EQUAL( target->sets, 1u );
    BOOST_CHECK_EQUAL( target->data->getPropertyAsInt( "value" ), 77 );
    BOOST_CHECK_EQUAL( detailGroup( target->data )->getPropertyAsInt( "value" ), 21 );
}

BOOST_AUTO_TEST_CASE( editor_property_nested_buttons_and_resource_metadata_test )
{
    InspectorFixture fixture;
    auto target = make_ptr<InspectorObject>( inspectorSchema( "target" ) );
    detailGroup( target->data )->setProperty( "asset", "original-uuid", "resource", false );
    detailGroup( target->data )->getPropertyObject( "asset" ).setAttribute( "resourceType", "Material" );
    auto displayed = target->getProperties();
    fixture.grid->setProperties( displayed );
    auto owner = detailGroup( displayed );
    const auto type = fixture.window->propertyChange( owner, target, "asset", "", true );
    BOOST_CHECK_EQUAL( type, render::IMaterial::typeInfo() );
    BOOST_CHECK_EQUAL( target->sets, 0u );
    BOOST_CHECK_EQUAL( detailGroup( target->data )->getProperty( "asset" ), "original-uuid" );

    detailGroup( target->data )->getPropertyObject( "asset" ).setReadOnly( true );
    const auto rejected = fixture.window->propertyChange( owner, target, "asset", "", true );
    BOOST_CHECK_EQUAL( rejected, 0u );
    BOOST_CHECK_EQUAL( target->sets, 0u );

    fixture.window->propertyChange( owner, target, "value", "42", false );
    fixture.window->propertyChange( owner, target, "Apply", "true", true );
    BOOST_CHECK_EQUAL( target->sets, 2u );
    BOOST_CHECK_EQUAL( target->actions, 1u );
    BOOST_CHECK_EQUAL( target->actionValue, 42 );
    BOOST_CHECK( !target->data->hasProperty( "Apply" ) );
    BOOST_CHECK( !detailGroup( target->data, 0 )->isButtonPressed( "Apply" ) );

    // The real drop handler must keep the dragged metadata separate from the
    // displayed group and route the UUID to that group's existing leaf.
    auto dropData = inspectorSchema( "drop target" );
    dropData->setProperty( "asset", "root-reference" );
    detailGroup( dropData, 0 )->setProperty( "asset", "first-reference" );
    detailGroup( dropData )->setProperty( "asset", "old-nested-reference" );
    auto dropObject = make_ptr<InspectorObject>( dropData );
    auto dropDisplay = dropObject->getProperties();
    auto dropOwner = detailGroup( dropDisplay );
    fixture.grid->setProperties( dropDisplay );
    fixture.window->setSelected( dropObject );
    auto drop = make_ptr<PropertiesWindow::PropertiesDropTarget>();
    drop->setOwner( fixture.window );
    auto dragged = make_ptr<Properties>();
    const auto actorUUID = StringUtil::getUUID();
    dragged->setProperty( "actorUUID", actorUUID );
    dragged->setProperty( "drag-only", "must remain in payload" );
    Array<Parameter> arguments( 5 );
    arguments[0].setStr( DataUtil::toString( dragged.get(), true, DataFormat::JSON ) );
    arguments[1].setStr( "asset" );
    arguments[2].setStr( "old-nested-reference" );
    arguments[3].setObject( dropOwner );
    arguments[4].setObject( dropDisplay->getChildrenByName( "group" )[1] );
    drop->handleEvent( EventType::UI, IEvent::handleDrop, arguments,
                       fixture.grid, fixture.grid, nullptr );
    BOOST_CHECK_EQUAL( dropObject->sets, 1u );
    BOOST_CHECK_EQUAL( detailGroup( dropObject->data )->getProperty( "asset" ), actorUUID );
    BOOST_CHECK_EQUAL( dropObject->data->getProperty( "asset" ), "root-reference" );
    BOOST_CHECK_EQUAL( detailGroup( dropObject->data, 0 )->getProperty( "asset" ), "first-reference" );
    BOOST_CHECK( !dropOwner->hasProperty( "actorUUID" ) );
    BOOST_CHECK( !dropOwner->hasProperty( "drag-only" ) );
    BOOST_CHECK( !detailGroup( dropObject->data )->hasProperty( "actorUUID" ) );
    BOOST_CHECK( !detailGroup( dropObject->data )->hasProperty( "drag-only" ) );
    arguments[0].setStr( String() );
    drop->handleEvent( EventType::UI, IEvent::handleDrop, arguments,
                       fixture.grid, fixture.grid, nullptr );
    BOOST_CHECK_EQUAL( dropObject->sets, 1u );
    BOOST_CHECK_EQUAL( detailGroup( dropObject->data )->getProperty( "asset" ), actorUUID );
}

BOOST_AUTO_TEST_CASE( editor_property_grid_nested_button_owner_event_test )
{
    InspectorFixture fixture;
    auto displayed = inspectorSchema( "displayed" );
    auto owner = detailGroup( displayed );
    fixture.grid->setProperties( displayed );
    ui::ImGuiPropertyGrid::handlePropertyButtonClicked( fixture.grid, owner, "Apply", "true" );
    BOOST_CHECK_EQUAL( fixture.application->events, 1u );
    BOOST_CHECK_EQUAL( fixture.application->lastEvent, IEvent::handlePropertyButtonClick );
    BOOST_REQUIRE_EQUAL( fixture.application->lastArguments.size(), 3u );
    BOOST_CHECK( fixture.application->lastArguments[2].getObject() == owner );
    BOOST_CHECK( fixture.application->lastSender == fixture.grid );

    // Existing root-only callers keep their overload and payload contract.
    ui::ImGuiPropertyGrid::handlePropertyButtonClicked( fixture.grid, "Apply", "true" );
    BOOST_CHECK( fixture.application->lastArguments[2].getObject() == displayed );
    ui::ImGuiPropertyGrid::handlePropertyChanged( fixture.grid, owner, "value", "55" );
    BOOST_CHECK_EQUAL( fixture.application->lastEvent, IEvent::handlePropertyChanged );
    BOOST_CHECK( fixture.application->lastArguments[2].getObject() == owner );
}

#endif
