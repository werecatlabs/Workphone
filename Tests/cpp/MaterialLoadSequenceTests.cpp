#include <Workphone/Workphone.hpp>
#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawMaterialManager.hpp>
#include <WPGraphics/ClawMaterialTechnique.hpp>
#include <WPGraphics/ClawMaterialPass.hpp>
#include <WPGraphics/ClawMaterialTexture.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/System/TimerMT.hpp>
#include <Workphone/Jobs/EventJob.hpp>
#include <Workphone/Memory/FactoryUtil.hpp>
#include <WPSQLite/WPSQLite.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{
    using namespace workphone;
    namespace fs = std::filesystem;

    void require( bool value, const char *message )
    {
        if( !value )
            throw std::runtime_error( message );
    }

    class DeferredGraphics : public render::ClawHammerSystem
    {
    public:
        Array<SmartPtr<ISharedObject>> pending;
        void loadObject( SmartPtr<ISharedObject> object, bool = false ) override
        {
            if( object )
                pending.push_back( object );
        }
        void setMaterials( SmartPtr<render::IMaterialManager> manager )
        {
            m_materialManager = manager;
        }
    };

    class MaterialProbe : public scene::Material
    {
    public:
        unsigned updates = 0;
        void updateDependentComponents() override
        {
            ++updates;
            scene::Material::updateDependentComponents();
        }
    };

    struct Fixture
    {
        TypeManager types;
        TaskId previousTask = Thread::getCurrentTask();
        SmartPtr<core::ApplicationManager> app;
        SmartPtr<DeferredGraphics> graphics;
        SmartPtr<StateManager> states;
        SmartPtr<TimerMT> timer;
        SmartPtr<SQLitePlugin> sqlite;
        SmartPtr<AssetDatabaseManager> catalog;
        SmartPtr<ResourceDatabase> resources;
        SmartPtr<render::ClawMaterialManager> materials;
        SmartPtr<scene::GameManager> game;
        fs::path root;

        Fixture()
        {
            types.load();
            TypeManager::setInstance( &types );
            app = make_ptr<core::ApplicationManager>();
            core::IApplicationManager::setInstance( app );
            auto factories = make_ptr<FactoryManager>();
            factories->load( nullptr );
            app->setFactoryManager( factories );
            app->setFileSystem( make_ptr<FileSystem>() );
            timer = make_ptr<TimerMT>();
            timer->load( nullptr );
            app->setTimer( timer );
            states = make_ptr<StateManager>();
            app->setStateManager( states );
            states->load( nullptr );
            graphics = make_ptr<DeferredGraphics>();
            app->setGraphicsSystem( graphics );
            auto graphicsFactories = make_ptr<FactoryManager>();
            graphicsFactories->load( nullptr );
            FactoryUtil::addFactory<render::ClawMaterialTechnique>( graphicsFactories );
            FactoryUtil::addFactory<render::ClawMaterialPass>( graphicsFactories );
            FactoryUtil::addFactory<render::ClawMaterialTexture>( graphicsFactories );
            graphics->setFactoryManager( graphicsFactories );
            materials = make_ptr<render::ClawMaterialManager>();
            graphics->setMaterials( materials );
            game = make_ptr<scene::GameManager>();
            app->setGameManager( game );
            sqlite = make_ptr<SQLitePlugin>();
            sqlite->load( nullptr );
            root = fs::temp_directory_path() /
                   ( std::string( "workphone_material_sequence_" ) + StringUtil::getUUID().c_str() );
            fs::create_directories( root );
            catalog = make_ptr<AssetDatabaseManager>();
            require( catalog->setProjectRoot( root.generic_string().c_str() ), "catalog root" );
            catalog->loadFromFile( ( root / "assets.db" ).generic_string().c_str() );
            require( catalog->getDatabase() && catalog->getDatabase()->isLoaded(), "SQLite fixture" );
            resources = make_ptr<ResourceDatabase>();
            resources->setDatabaseManager( catalog );
            app->setResourceDatabase( resources );
            Thread::setCurrentTask( TaskId::Primary );
            app->setLoadingState( LoadingState::Loaded );
        }

        ~Fixture()
        {
            app->setLoadingState( LoadingState::Unloaded );
            graphics->pending.clear();
            materials->unload( nullptr );
            graphics->setMaterials( nullptr );
            materials = nullptr;
            app->setGameManager( nullptr );
            game = nullptr;
            app->setResourceDatabase( nullptr );
            resources = nullptr;
            catalog->unload( nullptr );
            catalog = nullptr;
            sqlite->unload( nullptr );
            sqlite = nullptr;
            states->unload( nullptr );
            app->setStateManager( nullptr );
            app->setTimer( nullptr );
            app->setGraphicsSystem( nullptr );
            graphics = nullptr;
            app->setFileSystem( nullptr );
            app->setFactoryManager( nullptr );
            core::IApplicationManager::setInstance( nullptr );
            app = nullptr;
            timer = nullptr;
            states = nullptr;
            Thread::setCurrentTask( previousTask );
            TypeManager::setInstance( nullptr );
            types.unload();
            if( !root.empty() && root.parent_path() == fs::temp_directory_path() &&
                root.filename().string().find( "workphone_material_sequence_" ) == 0 )
                fs::remove_all( root );
        }
    };

    void run( Fixture &fixture )
    {
        // The process working directory deliberately differs from the catalog source root.
        std::ofstream( fixture.root / "surface.mat" )
            << R"({"materialType":0,"editorSettings":{"floats":{"roughness":0.23}}})";
        auto entry = make_ptr<render::ClawMaterial>();
        entry->getHandle()->setUUID( StringUtil::getUUID() );
        entry->setFilePath( "surface.mat" );
        fixture.catalog->addResourceEntry( entry );

        auto actor = make_ptr<scene::GameActor>();
        auto component = make_ptr<MaterialProbe>();
        auto renderer = make_ptr<scene::MeshRenderer>();
        auto mesh = make_ptr<render::ClawMesh>();
        actor->addComponentInstance( component );
        actor->addComponentInstance( renderer );
        renderer->setGraphicsObject( mesh );
        auto properties = make_ptr<Properties>();
        properties->setProperty( component->getMaterialPathStr(), String( "surface.mat" ) );
        properties->setProperty( component->getMaterialStr(), entry->getHandle()->getUUIDAsString() );
        properties->setProperty( component->getIndexStr(), 3u );
        component->fromData( properties );
        auto material = component->getMaterial();
        require( material && !material->isLoaded(), "scene restore must retain the pending material" );
        const fs::path resolvedPath( material->getFilePath().c_str() );
        require(
            resolvedPath.is_absolute() && fs::equivalent( resolvedPath, fixture.root / "surface.mat" ),
            "catalog material must resolve against its source root" );
        require( mesh->getMaterial( 3 ) == material, "restore must bind the final submesh index" );
        auto listener = component->getMaterialObjectListener();
        require( listener && material->getObjectListeners().size() == 1,
                 "restore before component load must subscribe exactly once" );
        component->load( nullptr );
        component->load( nullptr );
        require( component->isLoaded() && component->getMaterialObjectListener() == listener &&
                     material->getObjectListeners().size() == 1,
                 "component load must preserve the existing subscription" );

        auto updates = component->updates;
        material->load( nullptr );  // Real Claw material load and real EventJob delivery on Primary.
        require( material->isLoaded() && component->updates > updates,
                 "inline material completion must refresh dependents" );
        require( std::abs( material->getRoughness() - 0.23f ) < 0.001f,
                 "completion must include the actual serialized material settings" );
        {
            auto lateComponent = make_ptr<MaterialProbe>();
            auto legacyProperties = make_ptr<Properties>();
            legacyProperties->setProperty( lateComponent->getMaterialPathStr(),
                                           String( "surface.mat" ) );
            legacyProperties->setProperty( lateComponent->getMaterialStr(),
                                           SmartPtr<render::IMaterial>() );
            lateComponent->fromData( legacyProperties );
            lateComponent->load( nullptr );
            require(
                lateComponent->isLoaded() && lateComponent->getMaterial() == material &&
                    material->getObjectListeners().size() == 2,
                "path-only scenes and materials loaded before their component must bind correctly" );
            lateComponent->unload( nullptr );
        }
        component->setProperties( make_ptr<Properties>() );
        require( component->getMaterial() == material && component->getIndex() == 3,
                 "partial properties must preserve the material and section index" );

        std::ofstream( fixture.root / "replacement.mat" ) << R"({"materialType":0})";
        auto replacementEntry = make_ptr<render::ClawMaterial>();
        replacementEntry->getHandle()->setUUID( StringUtil::getUUID() );
        replacementEntry->setFilePath( "replacement.mat" );
        fixture.catalog->addResourceEntry( replacementEntry );
        auto replacement = fixture.resources->loadResourceByType<render::IMaterial>( "replacement.mat" );
        require( replacement != nullptr, "replacement must resolve through the real catalog" );
        properties = make_ptr<Properties>();
        properties->setProperty( component->getMaterialStr(),
                                 SmartPtr<render::IMaterial>( replacement ) );
        component->setProperties( properties );
        require( material->getObjectListeners().empty() && replacement->getObjectListeners().size() == 1,
                 "resource replacement must move the subscription" );

        auto completion = make_ptr<EventJob>();
        completion->setEventType( EventType::Loading );
        completion->setEventValue( IEvent::loadingStateChanged );
        completion->setArguments( { Parameter( static_cast<s32>( LoadingState::Loading ) ),
                                    Parameter( static_cast<s32>( LoadingState::Loaded ) ) } );
        completion->setSender( material );
        updates = component->updates;
        Thread::setCurrentTask( TaskId::Application );
        listener->handleEvent( EventType::Loading, IEvent::loadingStateChanged,
                               completion->getArguments(), material, material, nullptr );
        require( component->updates == updates, "stale material completion must be ignored" );
        listener->handleEvent( EventType::Loading, IEvent::loadingStateChanged, {}, replacement,
                               replacement, nullptr );
        require( component->updates == updates, "malformed completion must be ignored" );
        replacement->load( nullptr );
        require( component->updates > updates && mesh->getMaterial( 3 ) == replacement,
                 "application-task completion must refresh the replacement" );

        // Material first, renderer second: its initial binding must use the ready resource.
        auto laterMesh = make_ptr<render::ClawMesh>();
        renderer->setGraphicsObject( laterMesh );
        renderer->updateMaterials();
        require( laterMesh->getMaterial( 3 ) == replacement, "late mesh must bind the loaded material" );

        // Draws read the texture through the retained material, so delayed texture
        // readiness must remain observable without replacing the actor/material.
        auto texture = make_ptr<render::ClawTexture>();
        replacement->setTexture( texture, 0 );
        require( replacement->getTexture( 0 ) == texture && !texture->isLoaded(),
                 "pending texture binding" );
        texture->setLoadingState( LoadingState::Loaded );
        require( laterMesh->getMaterial( 3 )->getTexture( 0 )->isLoaded(), "late texture readiness" );

        component->unload( nullptr );
        require( replacement->getObjectListeners().empty(), "unload must detach resource completion" );
        updates = component->updates;
        listener->handleEvent( EventType::Loading, IEvent::loadingStateChanged,
                               completion->getArguments(), replacement, replacement, nullptr );
        require( component->updates == updates,
                 "retired listener must not refresh an unloaded component" );
        renderer->setGraphicsObject( nullptr );
        renderer->setLoadingState( LoadingState::Unloaded );
        replacement->unload( nullptr );
        material->unload( nullptr );
    }
}  // namespace

int main()
{
    try
    {
        Fixture fixture;
        run( fixture );
        std::cout << "PASS: scene material restoration, delayed completion, replacement, load order and "
                     "retirement\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
