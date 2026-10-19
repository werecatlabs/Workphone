#include <Workphone/Workphone.hpp>
#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawMaterialRenderCache.hpp>
#include <WPGraphics/ClawMaterialDrawRecords.hpp>
#include <WPGraphics/ClawMaterialManager.hpp>
#include <WPGraphics/ClawMaterialTechnique.hpp>
#include <WPGraphics/ClawMaterialPass.hpp>
#include <WPGraphics/ClawMaterialTexture.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <Workphone/State/States/MaterialPassStateData.hpp>
#include <Workphone/Graphics/MaterialShader.hpp>
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
#include <limits>
#include <vector>
#include <thread>
#include <atomic>
#include "workphone_graphics_material.h"
#include "MaterialRenderCacheBenchmark.hpp"

namespace
{
    using namespace workphone;
    namespace fs = std::filesystem;

    void require( bool value, const char *message )
    {
        if( !value )
            throw std::runtime_error( message );
    }

    void runMaterialPreparation()
    {
        using namespace render;
        ClawMaterialRenderCache cache;
        auto authored = std::make_shared<MaterialRenderState>();
        authored->diffuseColour = ColourF( 0.2f, 0.3f, 0.4f, 0.6f );
        authored->specularColour = ColourF( 0.04f, 0.05f, 0.06f, 1.0f );
        authored->emissiveColour = ColourF( 0.5f, 0.25f, 0.125f, 0.75f );
        authored->flags = emissionEnabledFlag | cutoutFlag;
        authored->emissionIntensity = 4.0f;
        authored->alphaClip = 0.3f;
        authored->metalness = 0.8f;
        authored->roughness = 0.2f;
        authored->uvTilingX = 2.0f;
        authored->uvTilingY = 3.0f;
        authored->uvOffsetX = 0.1f;
        authored->uvOffsetY = 0.2f;
        authored->uvRotation = 45.0f;
        authored->uvProjection = 4u;
        authored->uvTriplanarScale = 2.5f;
        authored->normalStrength = 0.7f;
        authored->aoStrength = 0.6f;
        authored->blendMode = 2u;
        authored->metallicSource = authored->roughnessSource = 3u;
        authored->aoSource = 1u;
        authored->opacitySource = 2u;
        authored->uvWrapU = 3u;
        authored->uvWrapV = 4u;
        authored->uvFilter = 3u;
        authored->uvAniso = 12.5f;
        const MaterialRenderState::Ptr state = authored;
        const auto owners = state.use_count();
        const auto first = cache.get( state );
        const auto &c = first.constants;
        require( c.base_color.x == 0.2f && c.base_color.w == 0.6f &&
                 c.specular_color.y == 0.05f && c.emissive_color.x == 2.0f &&
                 c.emissive_color.y == 1.0f && c.emissive_color.w == 0.75f,
                 "prepared colours preserve opacity, F0 and emission intensity" );
        require( c.surface.x == 0.8f && c.surface.y == 0.2f && c.surface.z == 2.0f &&
                 c.surface.w == 3.0f && c.uv_transform.x == 0.1f && c.uv_transform.y == 0.2f &&
                 c.uv_transform.z == 1.0f && c.uv_transform.w == 45.0f &&
                 c.controls.x == 0.7f && c.controls.y == 0.6f && c.controls.z == 0.3f &&
                 c.controls.w == 1.0f && c.projection.x == 4.0f && c.projection.y == 2.5f,
                 "prepared surface, UV, projection and blend controls" );
        require( first.textureSlots == std::array<u32, 6>{ 1u, 25u, 25u, 13u, 25u, 25u } &&
                 c.texture_sources.x == 3.0f && c.texture_sources.w == 2.0f &&
                 first.wrapU == 3u && first.wrapV == 4u && first.filter == 3u && first.anisotropy == 12u,
                 "prepared packed-map bindings and sampler controls" );
        for( int draw = 0; draw < 20000; ++draw )
        {
            auto perView = cache.get( state ).compose();
            perView.light_color.x = static_cast<f32>( draw );
            perView.fog_params.x = 2.0f;
            perView.map_flags.x = 1.0f;
            require( perView.light_color.x == static_cast<f32>( draw ) && perView.fog_params.x == 2.0f,
                     "each view composes its own draw constants" );
        }
        require( cache.getPreparationCount() == 1 && state.use_count() == owners,
                 "repeated submeshes/views prepare once without taking resource owners" );
        require( cache.get( state ).compose().light_color.x == 0.0f &&
                 cache.get( state ).compose().fog_params.x == 0.0f &&
                 cache.get( state ).compose().map_flags.x == 0.0f,
                 "draw lighting, fog and texture readiness cannot pollute material preparation" );

        // Authoring modes: roughness R/A, metallic A, packed G, inverted
        // metallic smoothness A, and glTF roughness G respectively.
        constexpr u32 roughnessSlots[] = { 3u, 3u, 2u, 25u, 2u, 3u };
        ClawMaterialRenderCache routing;
        for( u32 mode = 0; mode < 6; ++mode )
        {
            auto selected = std::make_shared<MaterialRenderState>( *state );
            selected->roughnessSource = mode;
            require( routing.get( selected ).textureSlots[2] == roughnessSlots[mode],
                     "all authored roughness modes resolve their semantic texture owner" );
        }

        auto changed = std::make_shared<MaterialRenderState>( *state );
        changed->materialType = MaterialType::UI;
        changed->flags = 0;
        changed->roughness = 0.9f;
        changed->metallicSource = changed->roughnessSource = changed->aoSource = changed->opacitySource = 0u;
        changed->uvProjection = 999u;
        changed->uvTriplanarScale = std::numeric_limits<f32>::quiet_NaN();
        changed->uvAniso = std::numeric_limits<f32>::infinity();
        const auto updated = cache.get( changed );
        require( updated.constants.surface.y == 0.9f && updated.constants.uv_transform.z == 0.0f &&
                 updated.constants.emissive_color.x == 0.0f && updated.constants.controls.z == -1.0f &&
                 updated.constants.projection.x == 0.0f && updated.constants.projection.y == 1.0f &&
                 updated.anisotropy == 1u &&
                 updated.textureSlots == std::array<u32, 6>{ 1u, 2u, 3u, 13u, 22u, 24u },
                 "replacement snapshot updates UI, disabled features, map bindings and invalid numeric defaults" );
        require( cache.getPreparationCount() == 2 && cache.get( state ).constants.surface.y == 0.2f,
                 "snapshot ownership distinguishes identical local revisions and keeps old draws coherent" );
        std::weak_ptr<const MaterialRenderState> retired = changed;
        changed.reset();
        require( retired.expired(), "preparation cache does not keep retired snapshots or textures alive" );

        // Simulate allocator address reuse with distinct owners of one live object.
        MaterialRenderState sameAddress;
        MaterialRenderState::Ptr ownerA( &sameAddress, []( const auto * ) {} );
        MaterialRenderState::Ptr ownerB( &sameAddress, []( const auto * ) {} );
        cache.get( ownerA );
        const auto beforeReuse = cache.getPreparationCount();
        cache.get( ownerB );
        require( cache.getPreparationCount() == beforeReuse + 1,
                 "different owners at the same address cannot return a stale prepared record" );

        std::vector<MaterialRenderState::Ptr> stress;
        for( size_t i = 0; i < ClawMaterialRenderCache::Capacity * 3; ++i )
        {
            auto item = std::make_shared<MaterialRenderState>();
            item->roughness = static_cast<f32>( i );
            stress.push_back( item );
        }
        for( int pass = 0; pass < 3; ++pass )
            for( size_t i = 0; i < stress.size(); ++i )
                require( cache.get( stress[i] ).constants.surface.y == static_cast<f32>( i ),
                         "bounded cache collisions and eviction preserve material identity" );
        cache.clear();
        require( cache.getPreparationCount() == 0, "renderer teardown clears cached metadata" );
        cache.get( state );
        require( cache.getPreparationCount() == 1, "cleared cache rebuilds on first use" );
        std::cout << "material_preparation_cache_bytes=" << sizeof( cache )
                  << " capacity=" << ClawMaterialRenderCache::Capacity
                  << " repeated_draws=20000 preparations=1\n";
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

    void runSpecularRestoration( Fixture &fixture )
    {
        // Scene loads can contain a stale legacy colour alongside the saved slider.
        auto source = make_ptr<render::ClawMaterial>();
        source->load( nullptr );
        auto data = dynamic_pointer_cast<Properties>( source->toData() );
        auto schemes = data->getChildrenByName( "schemes" );
        require( !schemes.empty(), "specular source technique" );
        auto passes = schemes.front()->getChildrenByName( "passes" );
        require( !passes.empty(), "specular source pass" );
        passes.front()->setProperty( "specular", ColourF::White );
        passes.front()->getChild( "editorSettings" )->getChild( "floats" )
            ->setProperty( "specular", 0.17f );
        const auto path = fixture.root / "specular.mat";
        std::ofstream( path ) << DataUtil::toString( data.get() );
        auto restored = make_ptr<render::ClawMaterial>();
        restored->setFilePath( path.generic_string().c_str() );
        restored->load( nullptr );
        require( restored->isLoaded(), "specular material file load" );
        require( MathF::equals( restored->getSpecularAmount(), 0.17f ), "saved specular slider" );
        const auto before = wp_graphics_material_get_specular( restored->getNativeMaterial() );
        require( MathF::equals( before.r, 0.17f ) && MathF::equals( before.g, 0.17f ) &&
                     MathF::equals( before.b, 0.17f ), "first-load native specular strength" );
        restored->setSpecularAmount( 0.17f );
        const auto after = wp_graphics_material_get_specular( restored->getNativeMaterial() );
        require( MathF::equals( before.r, after.r ) && MathF::equals( before.g, after.g ) &&
                     MathF::equals( before.b, after.b ), "slider must not repair loaded specular" );

        auto rootEditor = make_ptr<Properties>();
        rootEditor->setName( "editorSettings" );
        auto rootFloats = make_ptr<Properties>();
        rootFloats->setName( "floats" );
        rootFloats->setProperty( "specular", 0.23f );
        rootEditor->addChild( rootFloats );
        data->addChild( rootEditor );
        restored->fromData( data );
        require( MathF::equals( restored->getSpecular().r, 0.23f ), "root saved specular strength" );

        data->removeChild( "editorSettings" );
        passes.front()->removeChild( "editorSettings" );
        const auto tint = ColourF( 0.2f, 0.4f, 0.6f, 1.0f );
        passes.front()->setProperty( "specular", tint );
        restored->fromData( data );
        require( restored->getSpecular() == tint, "legacy colour-only specular tint" );

        restored->unload( nullptr );
        source->unload( nullptr );
    }

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
        require( material->getObjectFlag( OBJECT_FLAG_LOADING_EVENTS ),
                 "materials must opt in to loading completion notifications" );
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

namespace
{
    class CapturedMaterialMesh : public render::ClawMesh
    {
    public:
        mutable u64 materialQueries = 0;
        CapturedMaterialMesh()
        {
            m_mesh = wp_graphics_mesh_create();
            require( m_mesh && wp_graphics_mesh_add_submesh( m_mesh, 0, 3, 0 ) == 0 &&
                     wp_graphics_mesh_add_submesh( m_mesh, 3, 3, 1 ) == 1, "capture mesh sections" );
        }
        SmartPtr<render::IMaterial> getMaterial( s32 index = -1 ) const override
        {
            ++materialQueries;
            return ClawMesh::getMaterial( index );
        }
    };

    void runCapturedMaterialRecords( Fixture &fixture )
    {
        using namespace render;
        auto material = make_ptr<ClawMaterial>();
        material->load( nullptr ); material->setDiffuse( ColourF::Red );
        CapturedMaterialMesh first, second;
        for( auto mesh : { &first, &second } )
        { mesh->setMaterial( material, 0 ); mesh->setMaterial( material, 1 ); }
        ClawMaterialRenderCache cache;
        ClawMaterialDrawRecords captured;
        captured.capture( &first ); captured.capture( &second ); captured.capture( &first );
        require( !captured.find( &first ), "unsealed records cannot be consumed" );
        captured.seal( &cache );
        const auto firstBinding = *captured.find( &first ), secondBinding = *captured.find( &second );
        require( captured.getRecordCount() == 1 && captured.getBindingCount() == 4 &&
                 captured.getMeshCount() == 2 && captured.getPreparationCount() == 1 &&
                 first.materialQueries == 2 && second.materialQueries == 2,
                 "shared materials capture once across repeated meshes and sections" );
        auto snapshot = material->getRenderState();
        const auto owners = snapshot.use_count();
        for( u32 repeat = 0; repeat < 20000; ++repeat )
        {
            const auto record = captured.get( repeat % 2 ? firstBinding : secondBinding, repeat % 2 );
            require( record && record->state == snapshot.get() && record->prepared.constants.base_color.x == 1,
                     "dense records borrow the frozen owner and prepared constants" );
        }
        require( snapshot.use_count() == owners && first.materialQueries == 2 && second.materialQueries == 2,
                 "warm borrowed records acquire no snapshot references or mesh materials" );
        require( !captured.get( firstBinding, 2 ) && !captured.valid( firstBinding, &second, 2 ) &&
                 !captured.valid( firstBinding, &first, 1 ), "wrong mesh/section ranges reject" );
        ClawMaterialDrawRecords next;
        next.capture( &first ); next.seal( &cache );
        require( next.getPreparationCount() == 0 && !next.get( firstBinding, 0 ),
                 "unchanged publications reuse preparation and reject foreign bindings" );
        next.clear();
        material->setDiffuse( ColourF::Green );
        next.capture( &first ); next.seal( &cache );
        const auto nextBinding = *next.find( &first );
        require( next.getPreparationCount() == 1 &&
                 next.get( nextBinding, 0 )->state->diffuseColour == ColourF::Green &&
                 captured.get( firstBinding, 0 )->state->diffuseColour == ColourF::Red,
                 "replacement publication retains the prior in-flight values" );
        bool rejected = false;
        try { next.capture( &second ); } catch( const std::logic_error & ) { rejected = true; }
        require( rejected, "sealed publications reject mutation" );
        std::weak_ptr<const MaterialRenderState> retired = snapshot;
        snapshot.reset();
        material->unload( nullptr );
        require( !retired.expired() && captured.get( firstBinding, 0 )->state->diffuseColour == ColourF::Red,
                 "captured owners survive material graph unload" );
        captured.clear();
        require( retired.expired() && !captured.get( firstBinding, 0 ),
                 "publication retirement releases snapshots and rejects stale bindings" );
        next.clear();
        require( !next.get( nextBinding, 0 ), "slot reuse invalidates the previous publication" );
        // More distinct materials than the preparation cache can retain. Draws
        // from the sealed table must not thrash that cache between passes.
        std::vector<SmartPtr<ClawMaterial>> manyMaterials;
        std::vector<std::unique_ptr<CapturedMaterialMesh>> manyMeshes;
        cache.clear();
        for( u32 index = 0; index < 384; ++index )
        {
            auto value = make_ptr<ClawMaterial>(); value->load( nullptr );
            value->setRoughness( f32( index ) / 384.0f );
            auto mesh = std::make_unique<CapturedMaterialMesh>();
            mesh->setMaterial( value, 0 ); mesh->setMaterial( value, 1 );
            captured.capture( mesh.get() );
            manyMaterials.push_back( value ); manyMeshes.push_back( std::move( mesh ) );
        }
        captured.seal( &cache );
        require( captured.getRecordCount() == 384 && captured.getBindingCount() == 768 &&
                 captured.getPreparationCount() == 384, "dense records exceed cache capacity without eviction during draws" );
        for( u32 pass = 0; pass < 3; ++pass )
            for( u32 index = 0; index < manyMeshes.size(); ++index )
            {
                const auto binding = captured.find( manyMeshes[index].get() );
                const auto record = captured.get( *binding, pass % 2 );
                require( record && record->prepared.constants.surface.y == f32( index ) / 384.0f &&
                         manyMeshes[index]->materialQueries == 2,
                         "multiple passes consume every retained dense record without live material resolution" );
            }
        require( cache.getPreparationCount() == 384, "record consumption never re-enters the bounded cache" );
        captured.clear(); manyMeshes.clear();
        for( auto &value : manyMaterials ) value->unload( nullptr );
        first.setMaterial( nullptr ); second.setMaterial( nullptr );
        fixture.graphics->pending.clear();
    }

    void benchmarkCapturedMaterialRecords( Fixture &fixture )
    {
        using namespace render;
        constexpr int meshCount = 1000, frames = 30, samples = 7;
        for( int materialCount : { 1, 100, 1000 } )
        {
            std::vector<SmartPtr<ClawMaterial>> materials;
            std::vector<std::unique_ptr<CapturedMaterialMesh>> meshes;
            for( int i = 0; i < materialCount; ++i )
            {
                auto material = make_ptr<ClawMaterial>(); material->load( nullptr );
                material->setDiffuse( ColourF( f32( i % 97 ) / 97.0f, 0.3f, 0.7f, 1 ) );
                materials.push_back( material );
            }
            for( int i = 0; i < meshCount; ++i )
            {
                auto mesh = std::make_unique<CapturedMaterialMesh>();
                mesh->setMaterial( materials[i % materialCount], 0 );
                mesh->setMaterial( materials[i % materialCount], 1 );
                meshes.push_back( std::move( mesh ) );
            }
            struct Measurement { double ms; u64 checksum, preparations, denseBytes; };
            const auto measure = [&]( bool captured ) {
                ClawMaterialRenderCache cache;
                for( auto &material : materials ) cache.get( material->getRenderState() );
                const auto preparedBefore = cache.getPreparationCount();
                u64 checksum = 0, denseBytes = 0;
                const auto start = std::chrono::steady_clock::now();
                for( int frame = 0; frame < frames; ++frame )
                {
                    ClawMaterialDrawRecords records;
                    if( captured )
                    {
                        for( int pass = 0; pass < 2; ++pass )
                            for( auto &mesh : meshes ) records.capture( mesh.get() );
                        records.seal( &cache );
                        denseBytes = records.getDenseCapacityBytes();
                    }
                    for( int pass = 0; pass < 2; ++pass )
                        for( auto &mesh : meshes )
                        {
                            const auto binding = captured ? records.find( mesh.get() ) : nullptr;
                            for( u32 section = 0; section < 2; ++section )
                            {
                                SmartPtr<IMaterial> material;
                                MaterialRenderState::Ptr state;
                                const ClawPreparedMaterialDX11 *prepared;
                                if( captured ) prepared = &records.get( *binding, section )->prepared;
                                else
                                {
                                    material = mesh->getMaterial( section );
                                    state = static_cast<Material *>( material.get() )->getRenderState();
                                    prepared = &cache.get( state );
                                }
                                auto draw = prepared->compose();
                                draw.camera_position = { f32( pass ), 0, 0, 0 };
                                checksum += material_cache_benchmark::consume( draw, prepared->textureSlots, prepared->filter );
                            }
                        }
                }
                return Measurement{ std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - start ).count(), checksum,
                    cache.getPreparationCount() - preparedBefore, denseBytes };
            };
            std::array<double, samples> oldTimes{}, newTimes{};
            Measurement before{}, after{};
            for( int sample = 0; sample < samples; ++sample )
            {
                if( sample % 2 ) { after = measure( true ); before = measure( false ); }
                else { before = measure( false ); after = measure( true ); }
                require( before.checksum == after.checksum, "scene records benchmark output parity" );
                oldTimes[sample] = before.ms; newTimes[sample] = after.ms;
            }
            std::sort( oldTimes.begin(), oldTimes.end() ); std::sort( newTimes.begin(), newTimes.end() );
            std::cout << "workload=scene_records materials=" << materialCount << " meshes=" << meshCount
                << " frames=" << frames << " sections=2 passes=2 samples=" << samples
                << " direct_median_ms=" << oldTimes[samples / 2] << " captured_median_ms=" << newTimes[samples / 2]
                << " direct_preparations=" << before.preparations << " captured_preparations=" << after.preparations
                << " captured_dense_capacity_bytes=" << after.denseBytes
                << " checksum=" << after.checksum << '\n';
            meshes.clear();
            for( auto &material : materials ) material->unload( nullptr );
            fixture.graphics->pending.clear();
        }
    }

    render::MaterialShader observedSchema()
    {
        render::MaterialShader schema( "ObservedPBR" );
        schema.addParameter( render::MaterialShaderParameterType::Colour, "Albedo" )
              .addParameter( render::MaterialShaderParameterType::Scalar, "Metalness" )
              .addParameter( render::MaterialShaderParameterType::Scalar, "Roughness" )
              .addParameter( render::MaterialShaderParameterType::Colour, "Emissive" )
              .addParameter( render::MaterialShaderParameterType::Scalar, "Opacity" )
              .addParameter( render::MaterialShaderParameterType::Scalar, "AlphaClip" )
              .addParameter( render::MaterialShaderParameterType::Int, "Custom" );
        render::MaterialShaderRegistry::instance().registerShader( schema );
        return schema;
    }

    void benchmarkObservedParameters()
    {
        using namespace render;
        auto schema = observedSchema();
        auto legacy = make_ptr<ClawMaterial>(), batched = make_ptr<ClawMaterial>();
        legacy->load( nullptr ); batched->load( nullptr );
        legacy->setRenderMode( 3u ); batched->setRenderMode( 3u );
        batched->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
        auto &parameters = batched->getShaderParameters();
        const auto albedo = parameters.Resolve<ColourF>( "Albedo" );
        const auto metal = parameters.Resolve<f32>( "Metalness" );
        const auto rough = parameters.Resolve<f32>( "Roughness" );
        const auto emissive = parameters.Resolve<ColourF>( "Emissive" );
        const auto opacity = parameters.Resolve<f32>( "Opacity" );
        const auto clip = parameters.Resolve<f32>( "AlphaClip" );
        const auto names = parameters.GetNameLookupCount();
        constexpr int updates = 1000, samples = 7;
        const auto write = [&]( bool useBatch, f32 value ) {
            const auto color = ColourF( value, 1.0f - value, value * 0.5f, 1 );
            const auto emission = ColourF( value * 0.25f, 0, 0, 1 );
            if( useBatch )
            {
                auto edit = parameters.Edit();
                parameters.Set( albedo, color ); parameters.Set( metal, value );
                parameters.Set( rough, 1.0f - value ); parameters.Set( emissive, emission );
                parameters.Set( opacity, value ); parameters.Set( clip, 1.0f - value );
            }
            else
            {
                legacy->setDiffuse( color ); legacy->setMetalness( value );
                legacy->setRoughness( 1.0f - value ); legacy->setEmissive( emission );
                legacy->setOpacity( value ); legacy->setAlphaClip( 1.0f - value );
            }
        };
        std::array<double, samples> oldTimes{}, newTimes{};
        u64 oldPublications = 0, newPublications = 0;
        const auto measure = [&]( bool useBatch ) {
            write( useBatch, 0.75f );
            auto material = useBatch ? batched : legacy;
            const auto before = material->getRenderState()->revision;
            const auto start = std::chrono::steady_clock::now();
            for( int update = 0; update < updates; ++update )
                write( useBatch, update % 2 ? 0.75f : 0.25f );
            const auto elapsed = std::chrono::steady_clock::now() - start;
            const auto publications = material->getRenderState()->revision - before;
            if( useBatch ) newPublications = publications; else oldPublications = publications;
            return std::chrono::duration<double, std::milli>( elapsed ).count();
        };
        for( int sample = 0; sample < samples; ++sample )
        {
            if( sample % 2 ) { newTimes[sample] = measure( true ); oldTimes[sample] = measure( false ); }
            else { oldTimes[sample] = measure( false ); newTimes[sample] = measure( true ); }
            const auto before = legacy->getRenderState(), after = batched->getRenderState();
            require( before->diffuseColour == after->diffuseColour && before->emissiveColour == after->emissiveColour &&
                     before->metalness == after->metalness && before->roughness == after->roughness &&
                     before->opacity == after->opacity && before->alphaClip == after->alphaClip &&
                     before->flags == after->flags && before->blendMode == after->blendMode &&
                     newPublications == updates && oldPublications == updates * 7 &&
                     parameters.GetNameLookupCount() == names, "publication benchmark output/count/name parity" );
        }
        std::sort( oldTimes.begin(), oldTimes.end() ); std::sort( newTimes.begin(), newTimes.end() );
        std::cout << "updates=" << updates << " samples=" << samples
                  << " legacy_median_ms=" << oldTimes[samples / 2] << " batched_median_ms=" << newTimes[samples / 2]
                  << " legacy_publications=" << oldPublications << " batched_publications=" << newPublications
                  << " name_lookups_during_updates=" << parameters.GetNameLookupCount() - names << '\n';
        legacy->unload( nullptr ); batched->unload( nullptr );
    }

    void runObservedParameters( Fixture &fixture )
    {
        using namespace render;
        auto material = make_ptr<ClawMaterial>();
        material->load( nullptr );
        auto secondPass = material->getTechnique( 0 )->createPass();
        secondPass->load( nullptr );
        auto schema = observedSchema();
        material->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
        auto &parameters = material->getShaderParameters();
        const auto albedo = parameters.Resolve<ColourF>( "Albedo" );
        const auto metal = parameters.Resolve<f32>( "Metalness" );
        const auto rough = parameters.Resolve<f32>( "Roughness" );
        const auto emissive = parameters.Resolve<ColourF>( "Emissive" );
        const auto opacity = parameters.Resolve<f32>( "Opacity" );
        const auto clip = parameters.Resolve<f32>( "AlphaClip" );
        const auto custom = parameters.Resolve<s32>( "Custom" );
        const auto names = parameters.GetNameLookupCount();
        auto before = material->getRenderState();
        auto secondBefore = secondPass->getStateContext()->getStateDataById<MaterialPassStateData>(
            secondPass->getId() )->getRenderState();
        {
            auto edit = parameters.Edit();
            parameters.Set( albedo, ColourF( 0.2f, 0.3f, 0.4f, 1 ) );
            {
                auto nested = parameters.Edit();
                parameters.Set( metal, 0.25f ); parameters.Set( rough, 0.75f );
                parameters.Set( emissive, ColourF::Black ); parameters.Set( opacity, 1.0f );
                parameters.Set( clip, 0.4f );
            }
            material->applyShaderParametersToRenderer();
            require( material->getRenderState() == before, "nested buffer edits retain the prior render version" );
        }
        auto committed = material->getRenderState();
        auto secondCommitted = secondPass->getStateContext()->getStateDataById<MaterialPassStateData>(
            secondPass->getId() )->getRenderState();
        require( committed->revision == before->revision + 1 &&
                 secondCommitted->revision == secondBefore->revision + 1 &&
                 committed->metalness == 0.25f && committed->roughness == 0.75f &&
                 committed->diffuseColour.r == 0.2f && secondCommitted->roughness == 0.75f,
                 "one coherent publication per pass for a complete parameter edit" );
        std::atomic<bool> finished{ false }, coherent{ true };
        std::thread reader( [&] {
            while( !finished.load() )
            {
                const auto snapshot = material->getRenderState();
                if( !snapshot || std::abs( snapshot->metalness + snapshot->roughness - 1.0f ) > 0.00001f )
                    coherent.store( false );
            }
        } );
        for( int update = 0; update < 256; ++update )
        {
            auto edit = parameters.Edit();
            const auto value = update % 2 ? 0.25f : 0.75f;
            parameters.Set( metal, value ); parameters.Set( rough, 1.0f - value );
        }
        finished.store( true ); reader.join();
        require( coherent.load(), "render readers see coherent versions during parameter edit batches" );
        parameters.Set( rough, 0.33f );
        require( material->getRenderState()->roughness == 0.33f && committed->roughness == 0.75f,
                 "retained mutable buffer references publish without a manual renderer apply" );
        before = material->getRenderState();
        parameters.Set( rough, 0.33f );
        require( !parameters.SetScalar( rough.handle, s32( 3 ) ) && material->getRenderState() == before,
                 "no-op and wrong-type writes do not publish" );
        {
            auto edit = parameters.Edit();
            parameters.Set( rough, 0.9f ); parameters.Set( rough, 0.33f );
        }
        require( material->getRenderState() == before, "a batch with no net surface change reuses its render version" );
        parameters.Set( custom, s32( 17 ) );
        require( material->getRenderState() == before && parameters.GetNameLookupCount() == names,
                 "custom CPU slots do not reapply unrelated surface values or resolve names" );
        material->setMetalness( 0.9f );
        parameters.Set( rough, 0.2f );
        require( material->getRenderState()->metalness == 0.9f,
                 "incremental parameter edits preserve unrelated legacy pass edits" );
        before = material->getRenderState();
        material->applyShaderParametersToRenderer();
        require( material->getRenderState()->metalness == 0.25f &&
                 material->getRenderState()->revision == before->revision + 1,
                 "explicit full reapply retains compatibility and coalesces publication" );
        parameters.Set( opacity, 0.4f );
        auto transparent = material->getRenderState();
        require( transparent->opacity == 0.4f && transparent->diffuseColour.a == 0.4f &&
                 transparent->getFlag( transparentFlag ) && !transparent->getFlag( cutoutFlag ) &&
                 transparent->blendMode == 1u && material->getRenderMode() == 3u,
                 "automatic opacity edits preserve alpha and render-mode normalization" );

        auto detached = parameters;
        detached.Set( rough, 0.8f );
        require( material->getRenderState() == transparent, "copies never inherit a material observer" );
        parameters = detached;
        require( material->getRenderState()->roughness == 0.8f, "assignment retains the destination observer" );
        detached.Set( rough, 0.6f );
        require( material->getRenderState()->roughness == 0.8f, "assigned sources remain independent" );
        before = material->getRenderState();
        const auto revision = parameters.GetRevision();
        parameters = parameters;
        require( parameters.GetRevision() == revision && material->getRenderState() == before,
                 "self assignment preserves versions" );
        auto other = make_ptr<ClawMaterial>();
        other->load( nullptr ); other->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
        auto &otherParameters = other->getShaderParameters();
        otherParameters = std::move( parameters );
        require( !parameters.IsValid() && other->getRenderState()->roughness == 0.8f &&
                 material->getRenderState() == before, "moves transfer values without rebinding either owner" );
        parameters.Reset( detached.GetLayout() );
        parameters.Set( rough, 0.12f ); otherParameters.Set( rough, 0.34f );
        require( material->getRenderState()->roughness == 0.12f && other->getRenderState()->roughness == 0.34f,
                 "moved-from and moved-to buffers continue notifying their own material" );

        before = other->getRenderState();
        auto bag = make_ptr<Properties>();
        bag->setProperty( Material::shaderIDStr, schema.getShaderID() );
        bag->setProperty( "ShaderParam.Roughness", 0.45f );
        bag->setProperty( "ShaderParam.Albedo", ColourF( 0.6f, 0.2f, 0.1f, 1 ) );
        other->setProperties( bag );
        require( other->getRenderState()->roughness == 0.45f && other->getRenderState()->diffuseColour.r == 0.6f &&
                 other->getRenderState()->revision == before->revision + 1,
                 "schema restore and property edits publish only their final state" );
        auto nextSchema = schema;
        nextSchema.addParameter( MaterialShaderParameterType::Scalar, "Extra" );
        MaterialShaderRegistry::instance().registerShader( nextSchema );
        before = other->getRenderState();
        other->setShaderParameters( nextSchema.getParameterInfo(), nextSchema.getShaderID() );
        require( other->getRenderState() == before && !otherParameters.Set( rough, 0.9f ),
                 "schema migration retains the render version and rejects old handles" );
        auto nextRough = otherParameters.Resolve<f32>( "Roughness" );
        otherParameters.Set( nextRough, 0.27f );
        require( other->getRenderState()->roughness == 0.27f,
                 "schema replacement retains observer binding on the physical instance" );

        material->unload( nullptr );
        {
            auto edit = parameters.Edit();
            parameters.Set( albedo, ColourF( 0.1f, 0.7f, 0.2f, 1 ) );
            parameters.Set( rough, 0.67f );
        }
        require( !material->getRenderState(), "edits after unload do not resurrect a retired graph" );
        material->load( nullptr );
        require( material->getRenderState()->roughness == 0.67f && material->getRenderState()->diffuseColour.g == 0.7f,
                 "pending parameter edits reach the new graph after reload" );
        material->unload( nullptr ); other->unload( nullptr );
        fixture.graphics->pending.clear();
    }

    void runRenderSnapshots( Fixture &fixture )
    {
        auto pending = make_ptr<render::ClawMaterial>();
        pending->setMetalness( 0.7f );
        pending->setRoughness( 0.25f );
        pending->load( nullptr );
        require( pending->getRenderState()->metalness == 0.7f &&
                 pending->getRenderState()->roughness == 0.25f,
                 "typed values assigned before load reach the first pass" );
        pending->unload( nullptr );
        auto material = make_ptr<render::ClawMaterial>();
        material->load( nullptr );
        auto first = material->getRenderState();
        require( first != nullptr, "material publishes primary-pass snapshot" );
        require( material->getRenderState() == first, "unchanged material reuses snapshot" );
        auto native = material->getNativeMaterial();
        const auto nativeRevision = wp_graphics_material_get_revision( native );
        require( material->getNativeMaterial() == native &&
                 wp_graphics_material_get_revision( native ) == nativeRevision,
                 "unchanged native access preserves revision" );
        material->setRoughness( 0.35f );
        auto edited = material->getRenderState();
        require( edited != first && edited->roughness == 0.35f && first->roughness != 0.35f,
                 "legacy setter publishes new values and retains old snapshot" );
        require( wp_graphics_material_get_roughness( material->getNativeMaterial() ) == 0.35f,
                 "native sync consumes latest snapshot" );
        material->setRoughness( 0.35f );
        require( material->getRenderState() == edited, "no-op material edit reuses snapshot" );
        auto pass = material->getTechnique( 0 )->getPass( 0 );
        pass->setMetalness( 0.6f );
        require( material->getRenderState()->metalness == 0.6f, "direct pass edit is visible" );
        {
            auto data = pass->getStateContext()->invalidateStateDataById<MaterialPassStateData>( pass->getId() );
            require( bool( data ), "owned pass data" );
            data->roughness = 0.8f;
            data->uvOffsetX = 0.25f;
        }
        auto direct = material->getRenderState();
        require( direct->roughness == 0.8f && direct->uvOffsetX == 0.25f,
                 "existing SafePtr writes publish coherently" );
        auto texture = make_ptr<render::ClawTexture>();
        auto detached = make_ptr<render::ClawMaterialPass>();
        auto detachedUnit = make_ptr<render::ClawMaterialTexture>();
        detachedUnit->setTexture( texture );
        detached->addTextureUnit( detachedUnit );
        detached->load( nullptr );
        {
            auto data = detached->getStateContext()->getStateDataById<MaterialPassStateData>( detached->getId() );
            require( data && data->getRenderState()->textures[0] == texture,
                     "texture units authored before state creation reach runtime snapshot" );
        }
        detached->unload( nullptr );
        material->setTexture( texture, 22 );
        auto textured = material->getRenderState();
        require( textured->textures[22] == texture, "extension texture slot in snapshot" );
        auto unit = pass->getTextureUnits()[22];
        unit->setTexture( nullptr );
        require( !material->getRenderState()->textures[22] && textured->textures[22] == texture,
                 "direct texture-unit removal updates new record and retains prior resource" );
        auto originalTechnique = material->getTechnique( 0 );
        auto secondTechnique = material->createTechnique();
        auto secondPass = secondTechnique->createPass();
        secondPass->load( nullptr );
        secondPass->setRoughness( 0.9f );
        material->removeTechnique( originalTechnique );
        require( material->getRenderState()->roughness == 0.9f,
                 "removing primary technique changes cached selection" );
        secondTechnique->removePass( secondPass );
        require( !material->getRenderState(), "removing last pass invalidates selection" );
        secondTechnique->addPass( secondPass );
        require( material->getRenderState()->roughness == 0.9f, "adding pass restores selection" );
        render::MaterialShader schema( "SnapshotSchema" );
        schema.addParameter( render::MaterialShaderParameterType::Scalar, "CustomValue" )
              .addParameter( render::MaterialShaderParameterType::Int, "SignedValue" );
        render::MaterialShaderRegistry::instance().registerShader( schema );
        material->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
        auto oldHandle = material->getShaderParameters().FindParameter( "CustomValue" );
        material->getShaderParameters().SetScalar( oldHandle, 0.42f );
        material->getShaderParameters().SetScalar(
            material->getShaderParameters().FindParameter( "SignedValue" ), s32( -17 ) );
        schema.addParameter( render::MaterialShaderParameterType::Bool, "Extra" );
        render::MaterialShaderRegistry::instance().registerShader( schema );
        material->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
        auto &parameters = material->getShaderParameters();
        require( !parameters.SetScalar( oldHandle, 1.0f ) &&
                 parameters.GetScalar( parameters.FindParameter( "CustomValue" ) ) == 0.42f &&
                 parameters.GetInt( parameters.FindParameter( "SignedValue" ) ) == -17 &&
                 !parameters.GetBool( parameters.FindParameter( "Extra" ) ),
                 "schema replacement migrates compatible values and defaults new slots" );
        const auto validLayout = parameters.GetLayout();
        auto invalidSchema = schema.getParameterInfo();
        invalidSchema.push_back( invalidSchema[0] );
        material->setShaderParameters( invalidSchema, "RejectedSchema" );
        require( parameters.GetLayout() == validLayout && material->getShaderID() == "SnapshotSchema",
                 "invalid candidate preserves prior shader/layout" );
        auto serialized = material->getProperties();
        auto restored = make_ptr<render::ClawMaterial>();
        restored->load( nullptr );
        restored->setProperties( serialized );
        auto &restoredParameters = restored->getShaderParameters();
        require( restoredParameters.GetInt( restoredParameters.FindParameter( "SignedValue" ) ) == -17 &&
                 restoredParameters.GetScalar( restoredParameters.FindParameter( "CustomValue" ) ) == 0.42f,
                 "typed values preserve existing property serialization" );
        restored->unload( nullptr );
        material->unload( nullptr );
        require( !material->getRenderState(), "unload releases selected graph" );
        fixture.graphics->pending.clear();
    }
}

int main( int argc, char **argv )
{
    try
    {
        if( argc == 2 && std::string( argv[1] ) == "--benchmark-preparation" )
        {
            material_cache_benchmark::run();
            return 0;
        }
        material_cache_benchmark::contracts();
        runMaterialPreparation();
        Fixture fixture;
        if( argc == 2 && std::string( argv[1] ) == "--benchmark-records" )
        {
            benchmarkCapturedMaterialRecords( fixture );
            return 0;
        }
        if( argc == 2 && std::string( argv[1] ) == "--benchmark-publication" )
        {
            benchmarkObservedParameters();
            return 0;
        }
        runCapturedMaterialRecords( fixture );
        runObservedParameters( fixture );
        runRenderSnapshots( fixture );
        runSpecularRestoration( fixture );
        run( fixture );
        std::cout << "PASS: first-load specular, scene material restoration, delayed completion, replacement, load order and "
                     "retirement\n";
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
