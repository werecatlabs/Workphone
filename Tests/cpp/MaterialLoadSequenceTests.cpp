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
#include <Workphone/State/States/MaterialStateData.hpp>
#include <Workphone/Graphics/MaterialShader.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <Workphone/Graphics/GraphicsScene.hpp>
#include <Workphone/Graphics/GraphicsSystem.hpp>
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
#include <optional>
#include <functional>
#include <chrono>
#include <unordered_set>
#include <array>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <cstring>
#include <Workphone/System/FactoryTemplate.hpp>
#include <Workphone/System/StateContext.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/System/StateQueue.hpp>
#include "MaterialHeapProfile.hpp"
#include "workphone_graphics_material.h"
#include "workphone_graphics_cull_job.h"
#include "workphone_graphics_camera.h"
#include "workphone_graphics_scenenode.h"
#include "MaterialRenderCacheBenchmark.hpp"
#include "MaterialPassStorageReference.hpp"
#include "MaterialTextureColdStorageContracts.hpp"
#include "ClawMaterialResidencyContracts.hpp"
#include "ClawMaterialMetadataBudgetContracts.hpp"
#include "GenericEngineCatalogBudgetContracts.hpp"

namespace
{
    using namespace workphone;
    namespace fs = std::filesystem;

    void require( bool value, const char *message )
    {
        if( !value )
            throw std::runtime_error( message );
    }

    u32 readUIntSetting( const MaterialPassStateData &state, MaterialUIntSetting setting )
    {
        u32 value = 0;
        require( state.getUIntSetting( setting, value ), "typed pass setting is available" );
        return value;
    }

    void runDrawSorting()
    {
        using namespace render;
        using Key = ClawDrawSortKey;
        auto key = []( u32 index, u32 material, double depth, bool transparent = false ) {
            Key value; value.index = index; value.material = material; value.depth = depth;
            value.transparent = transparent; value.barrier = false; return value;
        };
        const std::vector<Key> original{ key( 0, 1, 25 ), key( 1, 0, 8 ), key( 2, 1, 3 ),
            key( 3, 0, 24 ), key( 4, 0, 7, true ), key( 5, 1, 30, true ), key( 6, 0, 7, true ) };
        const u32 expected[4][7] = {
            { 0, 1, 2, 3, 4, 5, 6 }, { 2, 1, 3, 0, 5, 4, 6 },
            { 1, 3, 2, 0, 5, 4, 6 }, { 1, 2, 3, 0, 5, 4, 6 } };
        for( u32 mode = 0; mode < 4; ++mode )
        {
            auto keys = original;
            sortDrawKeys( keys.data(), keys.size(), static_cast<ClawDrawSortStrategy>( mode ) );
            for( u32 i = 0; i < keys.size(); ++i )
                require( keys[i].index == expected[mode][i], "strategy-specific opaque order and stable back-to-front alpha" );
        }
        for( auto boundary : { 0, 1, 2, 3 } )
        {
            std::vector<Key> keys{ key( 0, 1, 10 ), key( 1, 1, 20 ), key( 2, 0, 1 ), key( 3, 0, 2 ) };
            if( boundary == 0 ) keys[1].barrier = true;
            if( boundary == 1 ) { keys[2].queue = 1; keys[3].queue = 1; }
            if( boundary == 2 ) { keys[2].zOrder = 1; keys[3].zOrder = 1; }
            if( boundary == 3 ) keys[1].depth = std::numeric_limits<double>::quiet_NaN();
            sortDrawKeys( keys.data(), keys.size(), ClawDrawSortStrategy::MaterialFirst );
            for( u32 i = 0; i < keys.size(); ++i ) require( keys[i].index == i, "ordering boundaries retain positions" );
        }
        require( !validDrawSort( static_cast<ClawDrawSortStrategy>( 99 ), 10 ) &&
                 !validDrawSort( ClawDrawSortStrategy::DepthBuckets, 0 ) &&
                 !validDrawSort( ClawDrawSortStrategy::DepthBuckets, std::numeric_limits<double>::infinity() ),
                 "invalid sorting policies reject" );
        std::vector<Key> large;
        for( u32 i = 0; i < 4096; ++i ) large.push_back( key( i, i % 17, ( i * 31 ) % 101 ) );
        auto expectedKeys = large;
        std::stable_sort( expectedKeys.begin(), expectedKeys.end(), []( const auto &a, const auto &b ) {
            return a.material != b.material ? a.material < b.material : a.depth < b.depth;
        } );
        sortDrawKeys( large.data(), large.size(), ClawDrawSortStrategy::MaterialFirst );
        for( size_t i = 0; i < large.size(); ++i )
            require( large[i].index == expectedKeys[i].index, "numeric sorting matches stable reference with repeated keys" );
    }

    void benchmarkDrawSorting()
    {
        using namespace render;
        std::vector<ClawDrawSortKey> input( 65536 ), work( input.size() );
        u32 random = 1337;
        for( u32 i = 0; i < input.size(); ++i )
        {
            random ^= random << 13; random ^= random >> 17; random ^= random << 5;
            auto &key = input[i]; key.index = i; key.material = random % 128;
            key.depth = double( ( random >> 7 ) % 100000 ) / 100.0;
            key.transparent = i % 8 == 0; key.barrier = false;
        }
        std::vector<double> samples[4];
        u64 transitions[4]{}, inversions[4]{}, checksums[4]{};
        for( int sample = 0; sample < 7; ++sample ) for( int order = 0; order < 4; ++order )
        {
            const auto mode = ( sample + order ) % 4;
            const auto begin = std::chrono::steady_clock::now();
            work = input;
            sortDrawKeys( work.data(), work.size(), static_cast<ClawDrawSortStrategy>( mode ), 10.0 );
            const auto end = std::chrono::steady_clock::now();
            samples[mode].push_back( std::chrono::duration<double, std::milli>( end - begin ).count() );
            u64 sum = 0, changes = 0, reversals = 0;
            const ClawDrawSortKey *previous = nullptr;
            for( const auto &key : work )
            {
                sum += key.index;
                if( !key.transparent )
                {
                    if( previous ) { changes += key.material != previous->material; reversals += key.depth < previous->depth; }
                    previous = &key;
                }
            }
            require( sum == u64( input.size() ) * ( input.size() - 1 ) / 2, "sorting benchmark retains all indices" );
            transitions[mode] = changes; inversions[mode] = reversals; checksums[mode] = sum;
        }
        const char *names[] = { "queue_order", "front_to_back", "material_first", "depth_buckets" };
        std::cout << "CPU key copy/sort only: 65536 objects, 128 materials, 1/8 alpha, 7 rotating samples; bucket=10 units\n";
        for( int mode = 0; mode < 4; ++mode )
        {
            std::sort( samples[mode].begin(), samples[mode].end() );
            std::cout << names[mode] << " median_ms=" << samples[mode][3]
                << " opaque_material_changes=" << transitions[mode] << " opaque_depth_reversals=" << inversions[mode]
                << " checksum=" << checksums[mode] << '\n';
        }
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

    template<class T> struct SnapshotAllocationTracker
    {
        using value_type = T;
        u32 *live;
        explicit SnapshotAllocationTracker(u32 &count) : live(&count) {}
        template<class U> SnapshotAllocationTracker(const SnapshotAllocationTracker<U> &other) : live(other.live) {}
        T *allocate(size_t count) { auto result = std::allocator<T>{}.allocate(count); ++*live; return result; }
        void deallocate(T *pointer, size_t count) { --*live; std::allocator<T>{}.deallocate(pointer, count); }
        template<class U> bool operator==(const SnapshotAllocationTracker<U> &other) const { return live == other.live; }
        template<class U> bool operator!=(const SnapshotAllocationTracker<U> &other) const { return !(*this == other); }
    };

    void runIdleRecordMaintenance()
    {
        using namespace render;
        u32 liveAllocations = 0;
        ClawMaterialRenderCache cache;
        std::vector<MaterialRenderState::Ptr> states;
        for(u32 i = 0; i < 257; ++i)
        {
            auto state = std::allocate_shared<MaterialRenderState>(SnapshotAllocationTracker<MaterialRenderState>(liveAllocations));
            cache.resolve(state); states.push_back(std::move(state));
        }
        auto pinned = states.back();
        const auto handle = cache.resolve(pinned);
        const auto prepared = cache.lookup(handle);
        auto lease = cache.retain();
        states.clear();
        const auto resident = cache.getResidentCount();
        cache.maintain(0);
        require(cache.getResidentCount() == resident && liveAllocations == resident,
            "zero maintenance budget leaves expired weak metadata untouched");
        constexpr u32 budget = 8;
        const auto sweeps = (cache.getSlotCount() + budget - 1) / budget +
                            (ClawMaterialRenderCache::Capacity + budget - 1) / budget;
        const auto preparations = cache.getPreparationCount(), allocations = cache.getStorageAllocationCount();
        for(u32 frame = 0; frame < sweeps; ++frame)
        {
            const auto before = cache.getResidentCount(); cache.maintain(budget);
            require(before - cache.getResidentCount() <= budget && lease.get(handle) == prepared,
                "budgeted idle collection bounds slots inspected and preserves pinned versions");
        }
        require(cache.getResidentCount() == 1 && liveAllocations == 1 &&
                cache.getPreparationCount() == preparations && cache.getStorageAllocationCount() == allocations,
            "idle sweeps release table and front-cache weak control blocks without preparation or allocation");
        pinned.reset();
        for(u32 frame = 0; frame < sweeps; ++frame) cache.maintain(budget);
        require(liveAllocations == 0 && !lease.get(handle),
            "idle collection releases the last coallocated snapshot while table leases remain alive");
        auto final = std::allocate_shared<MaterialRenderState>(SnapshotAllocationTracker<MaterialRenderState>(liveAllocations));
        cache.resolve(final); final.reset(); cache.collectExpired();
        require(liveAllocations == 0, "explicit collection also clears expired front-cache weak owners");
    }

    void runPersistentRecordTable()
    {
        using namespace render;
        ClawMaterialRenderCache cache, foreign;
        std::vector<MaterialRenderState::Ptr> states;
        std::vector<ClawMaterialRenderCache::Handle> handles;
        std::vector<const ClawPreparedMaterialDX11 *> addresses;
        for(u32 i = 0; i < 1024; ++i)
        {
            auto state = std::make_shared<MaterialRenderState>(); state->roughness = f32(i) / 1024;
            states.push_back(state); handles.push_back(cache.resolve(state));
            addresses.push_back(cache.lookup(handles.back()));
        }
        const auto allocations = cache.getStorageAllocationCount();
        const auto bytes = cache.getStorageBytes();
        const auto owners = states.front().use_count();
        for(u32 frame = 0; frame < 8; ++frame)
            for(u32 view = 0; view < 3; ++view)
                for(u32 i = 0; i < states.size(); ++i)
                {
                    const auto handle = cache.resolve(states[i]);
                    require(handle.table == handles[i].table && handle.slot == handles[i].slot &&
                            handle.generation == handles[i].generation && cache.lookup(handle) == addresses[i] &&
                            cache.lookup(handle)->constants.surface.y == f32(i) / 1024,
                            "persistent identities and stable records survive frames/views beyond front-cache capacity");
                }
        require(cache.getPreparationCount() == states.size() && cache.getStorageAllocationCount() == allocations &&
                cache.getStorageBytes() == bytes && states.front().use_count() == owners,
                "unchanged large working set performs no preparation, allocation or strong-owner acquisition");
        require(!foreign.lookup(handles.front()), "another table rejects a material handle");
        auto invalid = handles.front(); ++invalid.generation;
        require(!cache.lookup(invalid), "wrong material slot generation rejects");
        invalid = handles.front(); invalid.slot = ~u32(0);
        require(!cache.lookup(invalid), "out-of-range material slot rejects");
        for(u32 edit = 0; edit < 5000; ++edit)
        {
            auto transient = std::make_shared<MaterialRenderState>(); transient->metalness = f32(edit);
            const auto handle = cache.resolve(transient);
            require(cache.lookup(handle)->constants.surface.x == f32(edit), "recycled slots contain their current version");
        }
        require(cache.getSlotCount() < states.size() + 128, "expired generations reuse slots during sustained churn");
        const auto retired = handles.back(); states.back().reset(); cache.collectExpired();
        require(!cache.lookup(retired), "retired snapshot rejects its old handle");
        auto replacement = std::make_shared<MaterialRenderState>();
        const auto fresh = cache.resolve(replacement);
        require(fresh.table == retired.table && (fresh.slot != retired.slot || fresh.generation != retired.generation) &&
                !cache.lookup(retired), "slot reuse cannot revive a retired generation");
        auto lease = cache.retain();
        cache.clear();
        require(!cache.lookup(handles.front()) && lease.get(handles.front()) == addresses.front(),
                "clear retires the current epoch while in-flight publications retain their CPU records");
        const auto newEpoch = cache.resolve(states.front());
        require(newEpoch.table != handles.front().table && !cache.lookup(handles.front()), "new epoch never aliases old handles");
        std::weak_ptr<const MaterialRenderState> weak = states.front();
        states.clear(); cache.collectExpired();
        require(weak.expired() && !lease.get(handles.front()), "table leases do not retain material snapshots or textures");
        std::cout << "persistent_records=1024 frames=8 views=3 preparations=1024 warm_allocations=0"
                  << " accounted_table_bytes=" << bytes << " slot_bytes_per_version="
                  << double(bytes - sizeof(cache)) / 1024 << '\n';
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
            generic_engine_catalog_budget_contracts::initializeTypes(types);
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
            WorldStreamingScheduler::shutdownShared();
            types.unload();
            if( !root.empty() && root.parent_path() == fs::temp_directory_path() &&
                root.filename().string().find( "workphone_material_sequence_" ) == 0 )
                fs::remove_all( root );
        }
    };

#include "MaterialSurfaceHandoffContracts.hpp"
#include "MaterialFullMemoryProfile.hpp"

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

    void runCapturedDrawSorting( Fixture &fixture )
    {
        using namespace render;
        auto scene = wp_graphics_scene_create();
        auto camera = wp_camera_create();
        wp_camera_set_position( camera, { 0, 0, 50 } );
        std::vector<SmartPtr<ClawMaterial>> materials;
        for( int i = 0; i < 3; ++i )
        {
            auto material = make_ptr<ClawMaterial>(); material->load( nullptr );
            if( i == 2 ) material->setOpacity( 0.5f );
            materials.push_back( material );
        }
        std::vector<std::unique_ptr<CapturedMaterialMesh>> meshes;
        std::vector<wp_graphics_object *> objects;
        const float depths[] = { 25, 8, 3, 24, 7, 30, 7 };
        ClawMaterialDrawRecords records;
        for( int i = 0; i < 7; ++i )
        {
            auto mesh = std::make_unique<CapturedMaterialMesh>();
            for( int section = 0; section < 2; ++section ) mesh->setMaterial( materials[i < 4 ? i % 2 : 2], section );
            auto object = wp_graphics_scene_create_object( scene );
            wp_graphics_object_set_render_func( object, []( wp_graphics_object *, wp_renderer * ) {} );
            wp_graphics_object_set_submit_data( object, mesh.get() );
            auto node = wp_graphics_scene_create_node( scene );
            wp_scenenode_set_position( node, { 0, 0, 50 - depths[i] } );
            wp_scenenode_attach_object( node, object );
            objects.push_back( object ); records.capture( mesh.get() ); meshes.push_back( std::move( mesh ) );
        }
        records.seal();
        const u32 expected[4][7] = {
            { 0, 1, 2, 3, 4, 5, 6 }, { 2, 1, 3, 0, 5, 4, 6 },
            { 2, 0, 1, 3, 5, 4, 6 }, { 2, 1, 0, 3, 5, 4, 6 } };
        auto source = wp_graphics_scene_prepare_cull( scene, camera, WP_GRAPHICS_CULL_PASS_MAIN );
        require( source && wp_graphics_cull_result_get_visible_count( source ) == 7, "sort fixture captures all objects" );
        const int identity[] = { 0, 1, 2, 3, 4, 5, 6 };
        // Live edits must not affect the captured classification or query materials.
        materials[0]->setOpacity( 0.25f );
        for( u32 mode = 0; mode < 4; ++mode )
        {
            auto view = wp_graphics_cull_result_create_ordered( source, identity, 7 );
            require( records.sortView( view, static_cast<ClawDrawSortStrategy>( mode ) ), "captured draw strategy succeeds" );
            for( int i = 0; i < 7; ++i )
                require( wp_graphics_cull_result_get_object( view, i ) == objects[expected[mode][i]],
                         "captured strategy orders native objects using frozen materials and translated camera depth" );
            require( wp_graphics_cull_result_validate( view, scene, wp_graphics_cull_result_get_scene_generation( view ) ) == WP_GRAPHICS_CULL_OK,
                     "reordered view retains source validation and transforms" );
            wp_graphics_cull_result_destroy( view );
        }
        for( auto &mesh : meshes ) require( mesh->materialQueries == 2, "sorting never reacquires materials" );
        const auto warmAllocations = records.getStorageAllocationCount();
        for( int repeat = 0; repeat < 16; ++repeat )
        {
            auto view = wp_graphics_cull_result_create_ordered( source, identity, 7 );
            require( records.sortView( view, ClawDrawSortStrategy::DepthBuckets ) &&
                     records.getStorageAllocationCount() == warmAllocations,
                     "warmed numeric key/order buffers are reused across views" );
            wp_graphics_cull_result_destroy( view );
        }
        wp_graphics_cull_result_destroy( source );
        // Mixed section materials prevent an object-level reorder through it.
        meshes[1]->setMaterial( materials[0], 1 );
        ClawMaterialDrawRecords mixed;
        for( auto &mesh : meshes ) mixed.capture( mesh.get() );
        mixed.seal();
        source = wp_graphics_scene_prepare_cull( scene, camera, WP_GRAPHICS_CULL_PASS_MAIN );
        require( mixed.sortView( source, ClawDrawSortStrategy::MaterialFirst ) &&
                 wp_graphics_cull_result_get_object( source, 0 ) == objects[0] &&
                 wp_graphics_cull_result_get_object( source, 1 ) == objects[1],
                 "mixed-material mesh is a barrier to object-level sorting" );
        wp_graphics_cull_result_destroy( source ); mixed.clear();
        source = wp_graphics_scene_prepare_cull( scene, nullptr, WP_GRAPHICS_CULL_PASS_MAIN );
        require( records.sortView( source, ClawDrawSortStrategy::MaterialFirst ), "missing camera retains queue order" );
        for( int i = 0; i < 7; ++i ) require( wp_graphics_cull_result_get_object( source, i ) == objects[i], "no-camera order preserved" );
        wp_graphics_cull_result_destroy( source );
        records.clear();
        wp_graphics_scene_destroy( scene ); wp_camera_destroy( camera );
        meshes.clear();
        for( auto material : materials ) material->unload( nullptr );
        fixture.graphics->pending.clear();
    }

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
            require( record && record->state == snapshot.get() && record->prepared->constants.base_color.x == 1,
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
        // More distinct materials than the front cache can retain. Persistent
        // records must survive all passes, frames and independent views.
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
                require( record && record->prepared->constants.surface.y == f32( index ) / 384.0f &&
                         manyMeshes[index]->materialQueries == 2,
                         "multiple passes consume every retained dense record without live material resolution" );
            }
        require( cache.getPreparationCount() == 384, "record consumption never re-enters the bounded cache" );
        const auto stable = *captured.get(*captured.find(manyMeshes.front().get()), 0);
        ClawMaterialDrawRecords anotherView;
        for(auto &mesh : manyMeshes) anotherView.capture(mesh.get());
        anotherView.seal(&cache);
        const auto shared = anotherView.get(*anotherView.find(manyMeshes.front().get()), 0);
        require(shared->prepared == stable.prepared && shared->handle.table == stable.handle.table &&
                shared->handle.slot == stable.handle.slot && shared->handle.generation == stable.handle.generation &&
                anotherView.getPreparationCount() == 0,
                "independent views share a persistent prepared record instead of copying its constants");
        anotherView.clear();
        // Warm/reuse storage without retaining the material owners. Exercise
        // more entries than the preparation cache and changing mesh identities.
        for( int frame = 0; frame < 3; ++frame )
        {
            captured.clear();
            for( auto &mesh : manyMeshes ) captured.capture( mesh.get() );
            captured.seal( &cache );
            require(captured.getPreparationCount() == 0 && cache.getPreparationCount() == 384,
                    "new frame publications reuse every persistent record without front-cache thrashing");
        }
        const auto warmAllocations = captured.getStorageAllocationCount();
        const auto retainedBytes = captured.getRetainedStorageBytes();
        auto retiredBinding = *captured.find( manyMeshes.front().get() );
        for( int frame = 0; frame < 32; ++frame )
        {
            captured.clear();
            std::vector<std::unique_ptr<CapturedMaterialMesh>> retiredMeshes;
            if( frame % 4 == 0 )
            {
                for( auto &value : manyMaterials )
                {
                    auto mesh = std::make_unique<CapturedMaterialMesh>();
                    mesh->setMaterial( value, 0 ); mesh->setMaterial( value, 1 );
                    retiredMeshes.push_back( std::move( mesh ) );
                }
                manyMeshes.swap( retiredMeshes ); // Keep old addresses alive during the check.
            }
            require( !captured.get( retiredBinding, 0 ) && captured.getRecordCount() == 0,
                     "storage reuse never revives retired bindings or records" );
            for( auto &mesh : manyMeshes ) captured.capture( mesh.get() );
            captured.seal( &cache );
            require( retiredMeshes.empty() || !captured.find( retiredMeshes.front().get() ),
                     "storage reuse removes old mesh identity mappings" );
            require(captured.getPreparationCount() == 0 && cache.getPreparationCount() == 384,
                    "mesh identity changes across frames do not reprepare unchanged materials");
            if( captured.getStorageAllocationCount() != warmAllocations || captured.getRetainedStorageBytes() != retainedBytes )
                std::cerr << "storage_reuse frame=" << frame << " allocations=" << captured.getStorageAllocationCount()
                          << " expected=" << warmAllocations << " bytes=" << captured.getRetainedStorageBytes()
                          << " expected_bytes=" << retainedBytes << '\n';
            require( captured.getStorageAllocationCount() == warmAllocations &&
                     captured.getRetainedStorageBytes() == retainedBytes,
                     "warm publication storage makes no upstream allocations and has bounded retained bytes" );
            retiredBinding = *captured.find( manyMeshes.front().get() );
        }
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
            struct Measurement { double ms; u64 checksum, preparations, denseBytes, allocations, retainedBytes, persistentBytes; };
            const auto measure = [&]( int mode ) {
                const bool captured = mode != 0;
                ClawMaterialRenderCache cache;
                for( auto &material : materials ) cache.get( material->getRenderState() );
                ClawMaterialDrawRecords workspace;
                if( mode == 2 ) for( int warm = 0; warm < 3; ++warm )
                {
                    for( int pass = 0; pass < 2; ++pass )
                        for( auto &mesh : meshes ) workspace.capture( mesh.get() );
                    workspace.seal( &cache ); workspace.clear();
                }
                const auto storageBefore = workspace.getStorageAllocationCount();
                const auto preparedBefore = cache.getPreparationCount();
                u64 checksum = 0, denseBytes = 0, allocations = 0, retainedBytes = 0;
                const auto start = std::chrono::steady_clock::now();
                for( int frame = 0; frame < frames; ++frame )
                {
                    std::optional<ClawMaterialDrawRecords> cold;
                    auto records = mode == 2 ? &workspace : nullptr;
                    if( mode == 1 ) { cold.emplace(); records = &*cold; }
                    if( captured )
                    {
                        for( int pass = 0; pass < 2; ++pass )
                            for( auto &mesh : meshes ) records->capture( mesh.get() );
                        records->seal( &cache );
                        denseBytes = records->getDenseCapacityBytes();
                        retainedBytes = std::max( retainedBytes, records->getRetainedStorageBytes() );
                        if( mode == 1 ) allocations += records->getStorageAllocationCount();
                    }
                    for( int pass = 0; pass < 2; ++pass )
                        for( auto &mesh : meshes )
                        {
                            const auto binding = captured ? records->find( mesh.get() ) : nullptr;
                            for( u32 section = 0; section < 2; ++section )
                            {
                                SmartPtr<IMaterial> material;
                                MaterialRenderState::Ptr state;
                                const ClawPreparedMaterialDX11 *prepared;
                                if( captured ) prepared = records->get( *binding, section )->prepared;
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
                    if( mode == 2 ) workspace.clear(); // Include owner retirement in the timer.
                }
                if( mode == 2 ) allocations = workspace.getStorageAllocationCount() - storageBefore;
                return Measurement{ std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - start ).count(), checksum,
                    cache.getPreparationCount() - preparedBefore, denseBytes, allocations, retainedBytes,
                    cache.getStorageBytes() };
            };
            std::array<double, samples> oldTimes{}, newTimes{}, reusedTimes{};
            std::array<Measurement, 3> result{};
            for( int sample = 0; sample < samples; ++sample )
            {
                for( int order = 0; order < 3; ++order )
                { const auto mode = ( sample + order ) % 3; result[mode] = measure( mode ); }
                require( result[0].checksum == result[1].checksum && result[0].checksum == result[2].checksum &&
                         result[2].allocations == 0, "scene records output parity and zero warm storage allocation" );
                oldTimes[sample] = result[0].ms; newTimes[sample] = result[1].ms; reusedTimes[sample] = result[2].ms;
            }
            std::sort( oldTimes.begin(), oldTimes.end() ); std::sort( newTimes.begin(), newTimes.end() );
            std::sort( reusedTimes.begin(), reusedTimes.end() );
            std::cout << "workload=scene_records materials=" << materialCount << " meshes=" << meshCount
                << " frames=" << frames << " sections=2 passes=2 samples=" << samples
                << " direct_median_ms=" << oldTimes[samples / 2] << " captured_median_ms=" << newTimes[samples / 2]
                << " reused_median_ms=" << reusedTimes[samples / 2]
                << " direct_preparations=" << result[0].preparations << " captured_preparations=" << result[2].preparations
                << " cold_storage_allocations=" << result[1].allocations << " reused_storage_allocations=" << result[2].allocations
                << " retained_storage_bytes=" << result[2].retainedBytes << " captured_dense_capacity_bytes=" << result[2].denseBytes
                << " shared_persistent_accounted_bytes=" << result[2].persistentBytes
                << " checksum=" << result[2].checksum << '\n';
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

    void benchmarkSchemaRebind()
    {
        using namespace render;
        constexpr size_t count = 1024, parameterCount = 128, sampleCount = 7;
        MaterialShader schema( "SchemaRebindBenchmark" );
        for( size_t i = 0; i < parameterCount; ++i )
            schema.addParameter( MaterialShaderParameterType::Scalar, "Value" + String( std::to_string( i ) ) );
        MaterialShaderRegistry::instance().registerShader( schema );
        auto source = make_ptr<ClawMaterial>();
        source->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
        for( u32 i = 0; i < parameterCount; ++i )
            source->getShaderParameters().SetScalar( source->getShaderParameters().GetParameter( i ), f32( i + 1 ) * 0.125f );
        std::vector<SmartPtr<ClawMaterial>> population;
        for( size_t i = 0; i < count; ++i )
        {
            auto material = make_ptr<ClawMaterial>();
            material->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
            material->getShaderParameters() = source->getShaderParameters();
            population.push_back( material );
        }
        const auto charge = [&] {
            std::unordered_set<const void *> owners;
            size_t bytes = 0;
            const auto add = [&]( const MaterialShaderParametersInstance &parameters ) {
                if( auto identity = parameters.GetValueStorageIdentity() )
                    if( owners.insert( identity ).second ) bytes += parameters.GetValueStorageBytes();
            };
            add( source->getShaderParameters() );
            for( const auto &material : population ) add( material->getShaderParameters() );
            return std::make_pair( owners.size(), bytes );
        };
        const auto before = charge();
        u64 storageChanges = 0, revisionChanges = 0;
        std::vector<double> milliseconds;
        for( size_t sample = 0; sample < sampleCount; ++sample )
        {
            const auto start = std::chrono::steady_clock::now();
            for( auto &material : population )
            {
                const auto &parameters = material->getShaderParameters();
                const auto identity = parameters.GetValueStorageIdentity();
                const auto revision = parameters.GetRevision();
                material->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
                storageChanges += parameters.GetValueStorageIdentity() != identity;
                revisionChanges += parameters.GetRevision() != revision;
            }
            milliseconds.push_back( std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - start ).count() );
        }
        const auto after = charge();
        std::sort( milliseconds.begin(), milliseconds.end() );
        require( population.back()->getShaderParameters().GetScalar(
                     population.back()->getShaderParameters().GetParameter( parameterCount - 1 ) ) == 16.0f,
                 "schema rebind benchmark preserves authored values" );
        std::cout << "schema_rebind materials=" << count << " parameters=" << parameterCount
                  << " samples=" << sampleCount << " cpu_median_ms=" << milliseconds[sampleCount / 2]
                  << " payloads_before=" << before.first << " payloads_after=" << after.first
                  << " payload_bytes_before=" << before.second << " payload_bytes_after=" << after.second
                  << " storage_changes=" << storageChanges << " revision_changes=" << revisionChanges
                  << " source_payload_included=1 control_blocks_layouts_native_and_graph_storage_excluded=1\n";
        for( auto &material : population ) material->unload( nullptr );
        source->unload( nullptr );
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
        std::array<double, samples> oldTimes{}, newTimes{}, batchTimes{};
        u64 oldPublications = 0, newPublications = 0, batchPublications = 0;
        auto legacyWrite = [&]( Material &m, f32 value ) {
            const auto color = ColourF( value, 1.0f - value, value * 0.5f, 1 );
            const auto emission = ColourF( value * 0.25f, 0, 0, 1 );
            m.setDiffuse( color ); m.setMetalness( value );
            m.setRoughness( 1.0f - value ); m.setEmissive( emission );
            m.setOpacity( value ); m.setAlphaClip( 1.0f - value );
        };
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
        const auto measureBatch = [&] {
            legacy->batch( [&]( Material &m ) { legacyWrite( m, 0.75f ); } );
            const auto before = legacy->getRenderState()->revision;
            const auto start = std::chrono::steady_clock::now();
            for( int update = 0; update < updates; ++update )
                legacy->batch( [&]( Material &m ) { legacyWrite( m, update % 2 ? 0.75f : 0.25f ); } );
            const auto elapsed = std::chrono::steady_clock::now() - start;
            batchPublications = legacy->getRenderState()->revision - before;
            return std::chrono::duration<double, std::milli>( elapsed ).count();
        };
        for( int sample = 0; sample < samples; ++sample )
        {
            const auto order = sample % 3;
            if( order == 0 ) { newTimes[sample] = measure( true ); oldTimes[sample] = measure( false ); batchTimes[sample] = measureBatch(); }
            else if( order == 1 ) { oldTimes[sample] = measure( false ); batchTimes[sample] = measureBatch(); newTimes[sample] = measure( true ); }
            else { batchTimes[sample] = measureBatch(); newTimes[sample] = measure( true ); oldTimes[sample] = measure( false ); }
            const auto before = legacy->getRenderState(), after = batched->getRenderState();
            require( before->diffuseColour == after->diffuseColour && before->emissiveColour == after->emissiveColour &&
                     before->metalness == after->metalness && before->roughness == after->roughness &&
                     before->opacity == after->opacity && before->alphaClip == after->alphaClip &&
                     before->flags == after->flags && before->blendMode == after->blendMode &&
                     newPublications == updates && oldPublications == updates * 6 && batchPublications == updates &&
                     parameters.GetNameLookupCount() == names, "publication benchmark output/count/name parity" );
        }
        std::sort( oldTimes.begin(), oldTimes.end() ); std::sort( newTimes.begin(), newTimes.end() );
        std::sort( batchTimes.begin(), batchTimes.end() );
        std::cout << "updates=" << updates << " samples=" << samples
                  << " legacy_median_ms=" << oldTimes[samples / 2] << " batched_median_ms=" << newTimes[samples / 2]
                  << " legacy_batch_median_ms=" << batchTimes[samples / 2]
                  << " legacy_publications=" << oldPublications << " batched_publications=" << newPublications
                  << " legacy_batch_publications=" << batchPublications
                  << " name_lookups_during_updates=" << parameters.GetNameLookupCount() - names << '\n';
        legacy->unload( nullptr ); batched->unload( nullptr );
    }

    class TextureBindingProbe : public render::Material
    {
    public:
        using Material::setTexture;
        std::vector<std::pair<u32,String>> calls;
        bool record = true;
        u64 checksum = 0;
        void setTexture(const String &path, u32 layer) override
        {
            checksum += path.size() + layer;
            if(record) calls.emplace_back(layer, path);
        }
        u64 bindingLayout() const { return m_shaderAuthoring ? m_shaderAuthoring->bindingLayoutId : 0; }
        size_t bindingBytes() const { return m_shaderAuthoring ? m_shaderAuthoring->textureBindings.capacity() * sizeof(ShaderTextureBinding) : 0; }
        size_t bindingControlBytes() const { return sizeof(m_shaderAuthoring) + (m_shaderAuthoring ? sizeof(ShaderAuthoringData) : 0); }
        const void *bindingStorage() const { return m_shaderAuthoring ? m_shaderAuthoring->textureBindings.data() : nullptr; }
        bool hasAuthoring() const { return bool(m_shaderAuthoring); }
        size_t authoringBytes() const { return sizeof(ShaderAuthoringData); }
        void legacyApply()
        {
            u32 layer = 0;
            for(const auto &parameter : getShaderParameters().GetParameterInfo()) {
                if(parameter.m_type != render::MaterialShaderParameterType::Texture) continue;
                auto path = getShaderTexture(parameter.m_name);
                if(!path.empty()) setTexture(path, layer);
                ++layer;
            }
        }
    };

    void runTextureBindings()
    {
        using namespace render;
        auto material = make_ptr<TextureBindingProbe>();
        require(!material->hasAuthoring(), "legacy materials have no shader authoring allocation");
        require(material->getShaderID().empty() && !material->isDataDriven() &&
            material->getShaderTexture("Absent").empty(), "empty authoring reads retain legacy defaults");
        material->setShaderID(""); material->applyDataDrivenTextures();
        require(!material->hasAuthoring(), "empty shader writes and texture applies allocate no authoring storage");
        Array<MaterialShaderParameterInfo> scalarSchema = {{MaterialShaderParameterType::Scalar,"Value"}};
        material->setShaderParameters(scalarSchema);
        require(!material->hasAuthoring(), "parameter-only layouts need no named authoring allocation");
        material->setShaderTexture("Retained", "retained.png");
        require(material->hasAuthoring(), "texture authoring allocates on demand without a shader ID");
        material->setShaderID("Temporary"); material->setShaderID("");
        require(!material->isDataDriven() && material->getShaderTexture("Retained") == "retained.png",
            "clearing a shader retains unresolved authoring references");
        std::cout << "shader_authoring pointer_bytes=" << sizeof(void*)
            << " optional_object_bytes=" << material->authoringBytes()
            << " legacy_population=10000 avoided_embedded_bytes="
            << 10000 * (material->authoringBytes() - sizeof(void*)) << '\n';
        Array<MaterialShaderParameterInfo> schema = {
            {MaterialShaderParameterType::Texture,"Missing"}, {MaterialShaderParameterType::Scalar,"Scalar"},
            {MaterialShaderParameterType::Texture,"Second"}, {MaterialShaderParameterType::Buffer,"Buffer"},
            {MaterialShaderParameterType::Texture,"Empty"}, {MaterialShaderParameterType::Texture,"Last"}};
        material->setShaderParameters(schema, "TextureBindingContract");
        // Reverse authoring order, unknown names, wrong types and empty paths must
        // preserve the old adapter's schema order and gaps between texture slots.
        material->setShaderTexture("Last", "last.png");
        material->setShaderTexture("Future", "future.png");
        material->setShaderTexture("Empty", "");
        material->setShaderTexture("Scalar", "ignored.png");
        material->setShaderTexture("Second", "second.png");
        const auto compare = [&] {
            material->calls.clear(); material->legacyApply();
            const auto expected = material->calls;
            material->calls.clear(); material->applyDataDrivenTextures();
            require(material->calls == expected, "cached texture bindings preserve named adapter calls and schema order");
        };
        compare();
        require(material->calls == std::vector<std::pair<u32,String>>{{1,"second.png"},{3,"last.png"}},
            "nontextures, missing references and empty paths preserve ordinal gaps");
        const auto identity = material->bindingLayout(), bytes = material->bindingBytes();
        for(int repeat = 0; repeat < 32; ++repeat) compare();
        require(material->bindingLayout() == identity && material->bindingBytes() == bytes,
            "unchanged texture applies reuse compact binding storage");
        const auto storage = material->bindingStorage();
        material->setShaderTexture("Second", "replacement.png"); compare();
        require(material->bindingStorage() == storage, "path-only edits reuse bindings and read the current authored path");
        material->setShaderTexture("Empty", "new.png"); compare();
        material->setShaderTexture("Last", ""); compare();
        material->setShaderTexture("Missing", "first.png"); compare();
        require(material->bindingLayout() == identity && material->getShaderTexture("Future") == "future.png",
            "new references rebuild bindings without dropping unknown authoring data");
        schema = {{MaterialShaderParameterType::Texture,"Future"}, {MaterialShaderParameterType::Scalar,"Second"},
            {MaterialShaderParameterType::Texture,"Missing"}};
        material->setShaderParameters(schema, "ReorderedTextureBindingContract"); compare();
        require(material->calls == std::vector<std::pair<u32,String>>{{0,"future.png"},{1,"first.png"}} &&
            material->bindingLayout() != identity, "schema reorder, type changes and removal replace ordinal bindings");
        const auto replacement = material->bindingLayout();
        auto invalid = schema; invalid.push_back(invalid.front());
        material->setShaderParameters(invalid, "RejectedBindingLayout"); compare();
        require(material->bindingLayout() == replacement, "invalid schemas retain valid binding maps");
        schema = {{MaterialShaderParameterType::Scalar,"Other"}, {MaterialShaderParameterType::Texture,"Second"}};
        material->getShaderParameters().Reset(schema); compare();
        require(material->calls == std::vector<std::pair<u32,String>>{{0,"replacement.png"}},
            "retained mutable parameter resets invalidate bindings through layout identity");
        schema = {{MaterialShaderParameterType::Scalar,"NoTextures"}};
        material->getShaderParameters().Reset(schema); compare();
        require(material->bindingBytes() == 0, "texture-free layouts release obsolete binding storage");
        material->getShaderParameters().Reset(MaterialShaderParameterLayout::Ptr{});
        material->calls.clear(); material->applyDataDrivenTextures();
        require(material->calls.empty(), "invalid parameter instances do not use stale bindings");
        std::cout << "Texture binding contracts: schema-order slots, warm reuse, reference edits, unknown names and layout replacement passed.\n";
    }

    void benchmarkTextureBindings()
    {
        using namespace render;
        auto material = make_ptr<TextureBindingProbe>(); material->record = false;
        Array<MaterialShaderParameterInfo> schema;
        for(int i = 0; i < 64; ++i)
            schema.emplace_back(i % 2 ? MaterialShaderParameterType::Texture : MaterialShaderParameterType::Scalar,
                "BindingParameter" + std::to_string(i));
        material->setShaderParameters(schema, "TextureBindingBenchmark");
        for(int i = 63; i >= 1; i -= 2)
            material->setShaderTexture("BindingParameter" + std::to_string(i), String(160, 'x') + std::to_string(i));
        material->applyDataDrivenTextures();
        constexpr int samples = 7, applies = 10000;
        std::array<double,samples> timings[2]; u64 sums[2]{};
        for(int sample = 0; sample < samples; ++sample) for(int order = 0; order < 2; ++order) {
            const auto mode = (sample + order) % 2;
            material->checksum = 0;
            const auto start = std::chrono::steady_clock::now();
            for(int i = 0; i < applies; ++i) {
                if(mode) material->applyDataDrivenTextures(); else material->legacyApply();
            }
            timings[mode][sample] = std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now() - start).count();
            sums[mode] = material->checksum;
        }
        require(sums[0] == sums[1], "texture binding benchmark preserves submitted slot/path checksums");
        for(auto &times : timings) std::sort(times.begin(), times.end());
        std::cout << "Texture bindings applies=" << applies << " parameters=64 textures=32 samples=" << samples
            << " named_median_ms=" << timings[0][samples/2] << " cached_median_ms=" << timings[1][samples/2]
            << " binding_control_bytes=" << material->bindingControlBytes()
            << " binding_capacity_bytes=" << material->bindingBytes() << " checksum=" << sums[1] << '\n';
    }

    class NamedSettingProbeMaterial : public render::ClawMaterial
    {
    public:
        mutable u64 namedCalls = 0;
        void setEditorFloat( const String &name, f32 value ) override
        { ++namedCalls; Material::setEditorFloat( name, value ); }
        f32 getEditorFloat( const String &name, f32 fallback ) const override
        { ++namedCalls; return Material::getEditorFloat( name, fallback ); }
        void setEditorUInt( const String &name, u32 value ) override
        { ++namedCalls; Material::setEditorUInt( name, value ); }
        u32 getEditorUInt( const String &name, u32 fallback ) const override
        { ++namedCalls; return Material::getEditorUInt( name, fallback ); }
        void setEditorBool( const String &name, bool value ) override
        { ++namedCalls; Material::setEditorBool( name, value ); }
        bool getEditorBool( const String &name, bool fallback ) const override
        { ++namedCalls; return Material::getEditorBool( name, fallback ); }
    };

    void benchmarkNumericControls( Fixture &fixture )
    {
        using namespace render;
        auto named = make_ptr<ClawMaterial>(), numeric = make_ptr<ClawMaterial>();
        named->load( nullptr ); numeric->load( nullptr );
        constexpr int samples = 7, updates = 1000;
        std::array<double, samples> times[2];
        double sums[2]{}; u64 publications[2]{};
        const auto measure = [&]( int mode ) {
            auto material = mode ? numeric : named;
            const auto write = [&]( f32 value ) {
                material->batch( [&]( Material &m ) {
                    if( mode )
                    {
                        m.setNormalStrength( value ); m.setDetailNormalStrength( value );
                        m.setAlphaClip( value ); m.setUVRotation( value ); m.setTriplanarScale( value + 1 );
                    }
                    else
                    {
                        m.setEditorFloat( Material::normalStrengthStr, value );
                        m.setEditorFloat( Material::detailNormalStrengthStr, value );
                        m.setEditorFloat( Material::alphaClipStr, value );
                        m.setEditorFloat( Material::uvRotationStr, value );
                        m.setEditorFloat( Material::uvTriplanarScaleStr, value + 1 );
                    }
                } );
            };
            write( 0.9f );
            const auto revision = material->getRenderState()->revision;
            double sum = 0;
            const auto start = std::chrono::steady_clock::now();
            for( int update = 0; update < updates; ++update )
            {
                write( update % 2 ? 0.6f : 0.2f );
                if( mode ) sum += material->getNormalStrength() + material->getDetailNormalStrength() +
                    material->getAlphaClip() + material->getUVRotation() + material->getTriplanarScale();
                else sum += material->getEditorFloat( Material::normalStrengthStr, 0 ) +
                    material->getEditorFloat( Material::detailNormalStrengthStr, 0 ) +
                    material->getEditorFloat( Material::alphaClipStr, 0 ) +
                    material->getEditorFloat( Material::uvRotationStr, 0 ) +
                    material->getEditorFloat( Material::uvTriplanarScaleStr, 0 );
            }
            const auto elapsed = std::chrono::steady_clock::now() - start;
            sums[mode] = sum; publications[mode] = material->getRenderState()->revision - revision;
            return std::chrono::duration<double, std::milli>( elapsed ).count();
        };
        for( int sample = 0; sample < samples; ++sample )
        {
            const auto first = sample % 2;
            times[first][sample] = measure( first ); times[1-first][sample] = measure( 1-first );
            const auto a = named->getRenderState(), b = numeric->getRenderState();
            require( sums[0] == sums[1] && publications[0] == updates && publications[1] == updates &&
                     a->normalStrength == b->normalStrength && named->getDetailNormalStrength() == numeric->getDetailNormalStrength() &&
                     a->alphaClip == b->alphaClip && a->uvRotation == b->uvRotation && a->uvTriplanarScale == b->uvTriplanarScale,
                     "named/numeric control benchmark has equal values, checksum and publication work" );
        }
        for( auto &values : times ) std::sort( values.begin(), values.end() );
        std::cout << "numeric_controls updates=" << updates << " samples=" << samples
                  << " named_median_ms=" << times[0][samples/2] << " numeric_median_ms=" << times[1][samples/2]
                  << " publications_per_path=" << publications[0] << " checksum=" << sums[0]
                  << " named_setting_resolutions=" << updates * 10 << " builtin_setting_resolutions=0\n";
        named->unload( nullptr ); numeric->unload( nullptr ); fixture.graphics->pending.clear();
    }

    void runNumericControls( Fixture &fixture )
    {
        using namespace render;
        auto material = make_ptr<NamedSettingProbeMaterial>();
        require( material->getNormalStrength() == 1 && material->getWorkflow() == 0 && material->getDepthWrite() &&
                 material->namedCalls == 0,
                 "unloaded numeric controls preserve defaults without a selected technique" );
        material->load( nullptr );
        auto technique = material->getTechnique( 0 );
        for( int pass = 1; pass < 3; ++pass ) technique->createPass()->load( nullptr );
        material->setTechnique( nullptr );
        material->namedCalls = 0;
        material->batch( []( Material &m ) {
            m.setRenderMode( 3 ); m.setWorkflow( 2 ); m.setSpecularAmount( 0.32f );
            m.setNormalStrength( 0.7f ); m.setDetailNormalStrength( 0.6f ); m.setAlphaClip( 0.4f );
            m.setOpacity( 0.8f ); m.setBlendMode( 1 ); m.setDepthWrite( false ); m.setDepthTest( 1 ); m.setCullMode( 1 );
            m.setUVTiling( Vector2F( 2, 3 ) ); m.setUVOffset( Vector2F( 0.2f, 0.3f ) );
            m.setUVRotation( 0.5f ); m.setUVProjection( 4 ); m.setUVSet( 2 ); m.setTriplanarScale( 2.5f );
        } );
        require( material->getRenderMode() == 3 && material->getWorkflow() == 2 &&
                 material->getSpecularAmount() == 0.32f && material->getNormalStrength() == 0.7f &&
                 material->getDetailNormalStrength() == 0.6f && material->getAlphaClip() == 0.4f &&
                 material->getOpacity() == 0.8f && material->getBlendMode() == 1 && !material->getDepthWrite() &&
                 material->getDepthTest() == 1 && material->getCullMode() == 1 &&
                 material->getUVTiling() == Vector2F( 2, 3 ) && material->getUVOffset() == Vector2F( 0.2f, 0.3f ) &&
                 material->getUVRotation() == 0.5f && material->getUVProjection() == 4 && material->getUVSet() == 2 &&
                 material->getTriplanarScale() == 2.5f && material->namedCalls == 0,
                 "all built-in scalar controls read/write numeric IDs without calling named editor adapters" );
        for( auto pass : technique->getPasses() )
        {
            auto state = pass->getStateContext()->getStateDataById<MaterialPassStateData>( pass->getId() );
            auto snapshot = state->getRenderState();
            require( snapshot->normalStrength == 0.7f && snapshot->alphaClip == 0.4f && snapshot->opacity == 0.8f &&
                     snapshot->uvTilingY == 3 && snapshot->uvOffsetX == 0.2f && snapshot->uvProjection == 4 &&
                     !pass->isDepthWriteEnabled() && pass->isDepthCheckEnabled() && pass->getCullingMode() == 1 &&
                     snapshot->blendMode == 1, "numeric control edits retain every pass and backend side effect" );
        }
        material->setWorkflow( 99 ); material->setUVProjection( 99 ); material->setUVSet( 99 );
        material->setTriplanarScale( std::numeric_limits<f32>::quiet_NaN() );
        material->setOpacity( -1 ); material->setRenderMode( 99 );
        require( material->getWorkflow() == 0 && material->getUVProjection() == 0 && material->getUVSet() == 0 &&
                 material->getTriplanarScale() == 1 && material->getOpacity() == 0 && material->getRenderMode() == 0 &&
                 material->namedCalls == 0, "numeric controls preserve normalization without named routing" );
        material->setEditorFloat( "normalStrength", 0.9f );
        material->setEditorUInt( "workflow", 1 );
        material->setEditorBool( "doubleSided", true );
        require( material->getEditorFloat( "normalStrength", 0 ) == 0.9f &&
                 material->getEditorUInt( "workflow", 0 ) == 1 && material->getEditorBool( "doubleSided", false ) &&
                 material->namedCalls == 6 && material->getCullMode() == 1,
                 "named authoring API remains virtual and preserves double-sided culling behavior" );
        material->setEditorBool( "doubleSided", false );
        require( !material->getEditorBool( "doubleSided", true ), "double-sided authoring toggle remains available" );
        material->unload( nullptr ); fixture.graphics->pending.clear();
    }

    void runLegacyBatchScopes( Fixture &fixture )
    {
        using namespace render;
        auto material = make_ptr<ClawMaterial>(); material->load( nullptr );
        auto technique = material->getTechnique( 0 );
        for( int i = 1; i < 6; ++i ) technique->createPass()->load( nullptr );
        std::vector<SmartPtr<IState>> states;
        std::vector<SmartPtr<MaterialPassStateData>> data;
        std::vector<MaterialRenderState::Ptr> before;
        for( auto pass : technique->getPasses() )
        {
            auto state = pass->getStateContext()->getStateById( pass->getId() );
            states.push_back( state );
            data.push_back( static_pointer_cast<MaterialPassStateData>( state->getData() ) );
            before.push_back( data.back()->getRenderState() );
        }
        material->batch( [&]( Material &m ) {
            m.setMetalness( 0.25f ); m.setRoughness( 0.75f );
            m.batch( [&]( Material &nested ) { nested.setUVOffset( Vector2F( 0.2f, 0.3f ) ); } );
            for( size_t i = 0; i < data.size(); ++i )
                require( data[i]->getRenderState() == before[i], "all six pass versions remain frozen inside nested batch" );
        } );
        for( size_t i = 0; i < data.size(); ++i )
        {
            const auto snapshot = data[i]->getRenderState();
            require( snapshot->revision == before[i]->revision + 1 && snapshot->metalness == 0.25f &&
                     snapshot->roughness == 0.75f && snapshot->uvOffsetY == 0.3f,
                     "inline and overflow pass scopes each publish one complete version" );
            require( data[i]->getPendingSurfaceStorageBytes() == 0 && &data[i]->getSurfaceState() == snapshot.get(),
                     "multi-pass material adapters retire every staged surface at commit" );
            before[i] = snapshot;
            states[i]->setDirty( false );
        }
        states[0]->setDirty( true );
        material->batch( []( Material & ) {} );
        material->batch( {} );
        for( size_t i = 0; i < data.size(); ++i )
            require( data[i]->getRenderState() == before[i] && states[i]->isDirty() == ( i == 0 ),
                     "empty batches preserve clean and preexisting dirty states without publication" );
        material->batch( []( Material &m ) {
            m.setMetalness( 0.5f ); m.setMetalness( 0.25f );
        } );
        for( size_t i = 0; i < data.size(); ++i )
            require( data[i]->getRenderState() == before[i] && data[i]->getPendingSurfaceStorageBytes() == 0,
                     "net-zero batches retain each existing snapshot and retire staging" );
        std::atomic<bool> finished{ false }, coherent{ true };
        std::thread reader( [&] {
            while( !finished.load() )
                for( const auto &value : data )
                {
                    auto snapshot = value->getRenderState();
                    if( std::abs( snapshot->metalness + snapshot->roughness - 1 ) > 0.00001f ) coherent.store( false );
                }
        } );
        for( int i = 0; i < 128; ++i )
            material->batch( [&]( Material &m ) {
                const auto value = i % 2 ? 0.25f : 0.75f;
                m.setMetalness( value ); m.setRoughness( 1 - value );
            } );
        finished.store( true ); reader.join();
        require( coherent.load(), "render readers never observe partial legacy batches on any authored pass" );
        material->unload( nullptr );
        material->batch( []( Material &m ) { m.setMetalness( 0.62f ); m.setOpacity( 0.37f ); } );
        require( !material->getRenderState(), "unloaded batch does not resurrect retired pass graph" );
        material->load( nullptr );
        require( material->getRenderState()->metalness == 0.62f && material->getRenderState()->opacity == 0.37f,
                 "pending legacy batch values survive graph reload" );
        material->unload( nullptr ); fixture.graphics->pending.clear();
    }

    class UpdatePublicationScene : public render::GraphicsScene
    {
    public:
        std::function<void()> edits;
        void update() override { if(edits) edits(); }
    };

    class UpdatePublicationGraphics : public render::GraphicsSystem
    {
    public:
        void add(SmartPtr<render::IGraphicsScene> scene) { m_scenes.push_back(scene); }
        void clear() { m_scenes.clear(); }
    };

    void runUpdatePublicationScopes( Fixture &fixture )
    {
        using namespace render;
        auto material = make_ptr<ClawMaterial>(); material->load(nullptr);
        material->setRenderMode(3u);
        auto technique = material->getTechnique(0);
        technique->createPass()->load(nullptr);
        auto schema = observedSchema();
        material->setShaderParameters(schema.getParameterInfo(), schema.getShaderID());
        auto &parameters = material->getShaderParameters();
        const auto opacity = parameters.Resolve<f32>("Opacity");
        const auto metal = parameters.Resolve<f32>("Metalness");
        const auto names = parameters.GetNameLookupCount();
        std::vector<SmartPtr<MaterialPassStateData>> data;
        for(auto pass : technique->getPasses())
            data.push_back(static_pointer_cast<MaterialPassStateData>(
                pass->getStateContext()->getStateById(pass->getId())->getData()));
        std::vector<MaterialRenderState::Ptr> before;
        for(auto &state : data) before.push_back(state->getRenderState());
        auto graphics = make_ptr<UpdatePublicationGraphics>();
        auto scene = make_ptr<UpdatePublicationScene>(); graphics->add(scene);
        scene->edits = [&] {
            require(MaterialPublicationScope::isActive(), "graphics update owns a publication boundary");
            material->setDiffuse(ColourF(0.2f, 0.4f, 0.6f, 1));
            material->setMetalness(0.3f);
            material->setRoughness(0.7f);
            material->setEmissive(ColourF(0.1f, 0, 0, 1));
            material->syncPBRToShaderParameters();
            f32 importedMetal = 0;
            require(parameters.Get(metal, importedMetal) && importedMetal == 0.3f,
                    "explicit PBR import reads live values inside update");
            parameters.Set(opacity, 0.45f);
            {
                MaterialPublicationScope nested;
                parameters.Set(metal, 0.6f);
                material->batch([](Material &m) { m.setRoughness(0.4f); m.setAlphaClip(0.15f); });
            }
            require(material->getMetalness() == 0.6f && material->getOpacity() == 0.45f,
                    "owning-thread getters see live edits inside update");
            for(size_t i = 0; i < data.size(); ++i)
                require(data[i]->getRenderState() == before[i], "readers retain old versions until update closes");
        };
        graphics->update();
        for(size_t i = 0; i < data.size(); ++i)
        {
            auto after = data[i]->getRenderState();
            require(after->revision == before[i]->revision + 1 && after->metalness == 0.6f &&
                    after->roughness == 0.4f && after->opacity == 0.45f && after->alphaClip == 0.15f &&
                    after->diffuseColour.r == 0.2f && after->diffuseColour.b == 0.6f,
                    "independent legacy/typed edits and nested batch publish once on every authored pass");
        }
        require(parameters.GetNameLookupCount() == names, "update edits use retained numeric handles");
        auto snapshot = material->getRenderState();
        scene->edits = [&] {
            material->setMetalness(0.2f); material->setMetalness(0.6f);
            material->setRoughness(0.4f);
        };
        graphics->update();
        require(material->getRenderState() == snapshot, "net-zero update preserves the existing version");
        scene->edits = [&] { material->setMetalness(0.8f); throw std::runtime_error("update failure"); };
        bool caught = false;
        try { graphics->update(); } catch(const std::runtime_error &) { caught = true; }
        require(caught && material->getRenderState()->metalness == 0.8f &&
                !MaterialPublicationScope::isActive() && !MaterialPublicationScope::pendingEntryCount(),
                "exception unwind publishes completed writes and releases the update queue");
        scene->edits = {};
        graphics->clear();

        // An update nested inside an existing explicit batch must not expose it early.
        snapshot = material->getRenderState();
        material->batch([&](Material &m) {
            { MaterialPublicationScope inner; m.setMetalness(0.25f); }
            require(material->getRenderState() == snapshot, "enclosing write batch owns the final boundary");
            m.setRoughness(0.75f);
        });
        require(material->getRenderState()->revision == snapshot->revision + 1 &&
                material->getRenderState()->roughness == 0.75f, "enclosing batch publishes complete final values");
        snapshot = material->getRenderState();
        {
            MaterialPublicationScope update;
            material->setMetalness(0.55f); // Already queued before the explicit batch takes its locks.
            material->batch([&](Material &m) {
                update.commit();
                require(material->getRenderState() == snapshot,
                        "committing a queued pass inside a write batch does not publish partial values");
                m.setRoughness(0.45f);
            });
        }
        require(material->getRenderState()->revision == snapshot->revision + 1 &&
                material->getRenderState()->metalness == 0.55f && material->getRenderState()->roughness == 0.45f,
                "a queued pass publishes once when its remaining write batch closes");

        std::atomic<bool> finished{false}, coherent{true};
        std::thread reader([&] {
            while(!finished.load())
            {
                auto version = material->getRenderState();
                if(std::abs(version->metalness + version->roughness - 1) > 0.00001f) coherent.store(false);
            }
        });
        for(int i = 0; i < 128; ++i)
        {
            MaterialPublicationScope update;
            const auto value = i % 2 ? 0.25f : 0.75f;
            material->setMetalness(value); material->setRoughness(1 - value);
        }
        finished.store(true); reader.join();
        require(coherent.load(), "concurrent readers observe coherent old or new update versions");
        snapshot = material->getRenderState();
        auto firstTexture = make_ptr<ClawTexture>(), finalTexture = make_ptr<ClawTexture>();
        {
            MaterialPublicationScope update;
            material->setTexture(firstTexture, 22);
            material->setTexture(finalTexture, 22);
            require(material->getRenderState() == snapshot, "texture edits retain the captured resource version");
        }
        require(material->getRenderState()->revision == snapshot->revision + 1 &&
                material->getRenderState()->textures[22] == finalTexture && !snapshot->textures[22],
                "texture replacement coalesces and preserves the retained old resource version");
        {
            MaterialPublicationScope update;
            auto independent = make_ptr<MaterialPassStateData>();
            require(bool(independent->getRenderState()), "construction inside update publishes an initial version");
            std::thread writer([&] { independent->setFloatSetting(MaterialFloatSetting::roughness, 0.23f); });
            writer.join();
            require(independent->getRenderState()->roughness == 0.23f,
                    "another thread without a scope retains immediate publication");
            {
                MaterialPassStateData stack;
                const auto initial = stack.getRenderState();
                require(bool(initial), "stack construction inside update publishes an initial version");
                stack.setFloatSetting(MaterialFloatSetting::roughness, 0.34f);
                require(stack.getRenderState() == initial, "stack edits share the update boundary while alive");
            }
            require(!MaterialPublicationScope::pendingEntryCount(), "stack destruction removes its pending entries");
        }
        {
            MaterialPublicationScope update;
            material->setRoughness(0.37f);
            material->unload(nullptr);
            update.commit(); update.commit();
            require(!material->getRenderState(), "commit never resurrects an unloaded pass graph");
        }
        {
            MaterialPublicationScope update;
            material->load(nullptr);
            material->setRoughness(0.61f);
        }
        require(material->getRenderState()->roughness == 0.61f,
                "a replacement pass graph publishes its own pending edits");
        material->unload(nullptr);
        fixture.graphics->pending.clear();

        // Colliding queue slots and unusually large updates must remain correct
        // without permanently retaining the largest update's allocation.
        std::vector<SmartPtr<MaterialPassStateData>> burst;
        std::vector<MaterialRenderState::Ptr> versions;
        for(int i = 0; i < 5000; ++i)
        {
            burst.push_back(make_ptr<MaterialPassStateData>());
            versions.push_back(burst.back()->getRenderState());
        }
        {
            MaterialPublicationScope update;
            for(auto &state : burst) state->setFloatSetting(MaterialFloatSetting::metalness, 0.3f);
            for(auto &state : burst) state->setFloatSetting(MaterialFloatSetting::roughness, 0.7f);
        }
        for(size_t i = 0; i < burst.size(); ++i)
            require(burst[i]->getRenderState()->revision == versions[i]->revision + 1 &&
                    burst[i]->getRenderState()->metalness == 0.3f && burst[i]->getRenderState()->roughness == 0.7f,
                    "collisions coalesce repeated writes and retain every pass in a large update");
        require(!MaterialPublicationScope::pendingEntryCount() &&
                MaterialPublicationScope::retainedStorageBytes() < 4096 * sizeof(void *),
                "large update releases burst storage instead of retaining peak capacity");
        auto retired = make_ptr<MaterialPassStateData>();
        WeakPtr<MaterialPassStateData> weak(retired);
        {
            MaterialPublicationScope update;
            retired->setFloatSetting(MaterialFloatSetting::roughness, 0.29f);
            retired = nullptr;
            require(!weak.expired(), "update retains pending state until commit");
        }
        require(weak.expired(), "commit releases temporary state owners");
        std::cout << "PASS: update publication boundaries, nested batches, concurrency, retirement and bounded burst storage\n";
    }

    void benchmarkUpdatePublication()
    {
        using namespace render;
        constexpr int samples = 7;
        for(int materialCount : {1, 64})
        {
            const int frames = materialCount == 1 ? 1000 : 16;
            const int updates = materialCount * frames;
            std::vector<SmartPtr<ClawMaterial>> materials;
            for(int i = 0; i < materialCount; ++i)
            {
                auto material = make_ptr<ClawMaterial>(); material->load(nullptr);
                material->setRenderMode(3u); materials.push_back(material);
            }
            for(int edits : {1, 6})
            {
                std::array<std::array<double, samples>, 3> timings{};
                std::array<u64, 3> publications{};
                std::array<MaterialRenderState::Ptr, 3> results;
                auto write = [&](Material &m, f32 value) {
                    m.setMetalness(value);
                    if(edits == 1) return;
                    m.setDiffuse(ColourF(value, 1 - value, value * 0.5f, 1));
                    m.setRoughness(1 - value); m.setEmissive(ColourF(value * 0.25f, 0, 0, 1));
                    m.setOpacity(value); m.setAlphaClip(1 - value);
                };
                for(int sample = 0; sample < samples; ++sample)
                    for(int offset = 0; offset < 3; ++offset)
                    {
                        const auto mode = (sample + offset) % 3;
                        u64 before = 0;
                        for(auto &material : materials)
                        {
                            write(*material, 0.75f);
                            before += material->getRenderState()->revision;
                        }
                        const auto start = std::chrono::steady_clock::now();
                        for(int frame = 0; frame < frames; ++frame)
                        {
                            const auto value = frame % 2 ? 0.75f : 0.25f;
                            auto update = [&] {
                                for(auto &material : materials)
                                    if(mode == 2) material->batch([&](Material &m) { write(m, value); });
                                    else write(*material, value);
                            };
                            if(mode == 1) { MaterialPublicationScope scope; update(); }
                            else update();
                        }
                        timings[mode][sample] = std::chrono::duration<double, std::milli>(
                            std::chrono::steady_clock::now() - start).count();
                        u64 after = 0;
                        for(auto &material : materials)
                        {
                            const auto result = material->getRenderState();
                            after += result->revision;
                            require(result->metalness == 0.75f && (edits == 1 ||
                                    (result->diffuseColour.r == 0.75f && result->roughness == 0.25f &&
                                     result->emissiveColour.r == 0.1875f && result->opacity == 0.75f && result->alphaClip == 0.25f)),
                                    "every benchmark material has the expected output");
                        }
                        publications[mode] = after - before;
                        results[mode] = materials.front()->getRenderState();
                        require(publications[mode] == u64(updates * (mode == 0 ? edits : 1)),
                                "update benchmark publication count");
                    }
                for(int mode = 1; mode < 3; ++mode)
                    require(results[mode]->metalness == results[0]->metalness &&
                            results[mode]->roughness == results[0]->roughness &&
                            results[mode]->diffuseColour == results[0]->diffuseColour &&
                            results[mode]->emissiveColour == results[0]->emissiveColour &&
                            results[mode]->opacity == results[0]->opacity && results[mode]->alphaClip == results[0]->alphaClip,
                            "update benchmark preserves material output");
                for(auto &times : timings) std::sort(times.begin(), times.end());
                std::cout << "update_publication materials=" << materialCount << " frames=" << frames
                          << " edits=" << edits << " material_updates=" << updates << " samples=" << samples
                          << " immediate_median_ms=" << timings[0][samples / 2] << " update_median_ms=" << timings[1][samples / 2]
                          << " explicit_batch_median_ms=" << timings[2][samples / 2]
                          << " immediate_publications=" << publications[0] << " update_publications=" << publications[1]
                          << " explicit_batch_publications=" << publications[2]
                          << " retained_thread_bytes=" << MaterialPublicationScope::retainedStorageBytes() << '\n';
            }
            for(auto &material : materials) material->unload(nullptr);
        }
    }

    void runCubicTexturePublications()
    {
        using namespace render;
        auto material = make_ptr<ClawMaterial>();
        require(!material->getCubicTextureSnapshot() && material->getCubicTextures().empty(),
            "empty materials have no cube-face publication");
        material->setCubicTextures({});
        require(!material->getCubicTextureSnapshot(), "empty cube assignments allocate no publication");
        auto first = make_ptr<ClawTexture>(), second = make_ptr<ClawTexture>();
        Array<SmartPtr<ITexture>> faces(6, first);
        material->setCubicTextures(faces);
        auto retained = material->getCubicTextureSnapshot();
        require(retained && retained->size() == 6 && (*retained)[0] == first,
            "cube publication preserves ordered face resources");
        faces[0] = second;
        require((*retained)[0] == first, "publication owns its values independently of the input array");
        auto legacyCopy = material->getCubicTextures(); legacyCopy[1] = second;
        for(int i = 0; i < 1000; ++i)
        {
            require(material->getCubicTextureSnapshot() == retained,
                "warm cube reads reuse the same immutable publication");
            material->setCubicTextures(*retained);
        }
        require(material->getCubicTextureSnapshot() == retained && (*retained)[1] == first,
            "legacy copies and no-op setters do not alter or replace the publication");
        material->setCubicTextures(faces);
        require(material->getCubicTextureSnapshot() != retained &&
            (*material->getCubicTextureSnapshot())[0] == second && (*retained)[0] == first,
            "replacement retains complete previous face lists for queued readers");
        material->setCubeTexture(second);
        material->setCubicTextures({});
        require(!material->getCubicTextureSnapshot() && material->getCubeTexture() == second &&
            retained->size() == 6, "clearing faces preserves the independent cube resource and retained readers");
        material->setCubeTexture(nullptr);
        std::atomic<bool> failed{false};
        std::thread writer([&] {
            for(int i = 0; i < 2000; ++i)
                material->setCubicTextures(Array<SmartPtr<ITexture>>(i % 2 ? 3 : 6, i % 2 ? second : first));
        });
        std::thread reader([&] {
            for(int i = 0; i < 4000; ++i)
            {
                const auto snapshot = material->getCubicTextureSnapshot();
                if(!snapshot) continue;
                const auto expected = snapshot->size() == 3 ? second : first;
                if(snapshot->size() != 3 && snapshot->size() != 6) failed = true;
                for(const auto &face : *snapshot) if(face != expected) failed = true;
            }
        });
        writer.join(); reader.join();
        require(!failed, "concurrent cube publication readers observe complete versions");
        material->setCubicTextures({}); material = nullptr; faces.clear(); legacyCopy.clear();
        WeakPtr<ITexture> weakFirst = first; first = nullptr;
        require(weakFirst.lock(), "retained publication pins old resources after replacement and clear");
        retained.reset();
        require(!weakFirst.lock(), "last publication release retires old cube resources");
        std::cout << "cube_face_container_reference_bytes=" << sizeof(ConcurrentArray<SmartPtr<ITexture>>)
            << " publication_pointer_bytes=" << sizeof(Material::CubicTextureSnapshot)
            << " base_texture_unit_bytes=" << sizeof(MaterialTexture) << '\n';
    }

    void benchmarkCubicTexturePublications()
    {
        using namespace render;
        auto material = make_ptr<ClawMaterial>();
        auto texture = make_ptr<ClawTexture>();
        material->setCubicTextures(Array<SmartPtr<ITexture>>(6, texture));
        constexpr int samples = 7, reads = 100000;
        std::array<double, samples> times[2];
        u64 checksums[2]{};
        for(int sample = 0; sample < samples; ++sample) for(int order = 0; order < 2; ++order)
        {
            const auto mode = (sample + order) % 2;
            u64 sum = 0;
            const auto start = std::chrono::steady_clock::now();
            for(int i = 0; i < reads; ++i)
            {
                if(mode)
                {
                    const auto view = material->getCubicTextureSnapshot();
                    sum += view->size() + ((*view)[0] == texture);
                }
                else
                {
                    const auto copy = material->getCubicTextures();
                    sum += copy.size() + (copy[0] == texture);
                }
            }
            times[mode][sample] = std::chrono::duration<double,std::milli>(
                std::chrono::steady_clock::now() - start).count();
            checksums[mode] = sum;
        }
        require(checksums[0] == checksums[1], "cube read benchmark preserves resource/face checksums");
        for(auto &values : times) std::sort(values.begin(), values.end());
        std::cout << "cube_reads=" << reads << " faces=6 samples=" << samples
            << " copied_getter_median_ms=" << times[0][samples/2]
            << " shared_view_median_ms=" << times[1][samples/2]
            << " checksum=" << checksums[1] << '\n';
        material->setCubicTextures({});
    }

    void runTextureNameStorage( Fixture &fixture )
    {
        using namespace render;
        class UnitProbe : public ClawMaterialTexture
        {
        public:
            const SourceBudgetString *pendingName() const { return m_unresolvedTextureName.get(); }
        };
        class TextureProbe : public ClawTexture
        {
        public:
            unsigned loads = 0;
            void load( SmartPtr<ISharedObject> ) override
            {
                ++loads;
                setLoadingState( LoadingState::Loaded );
            }
        };
        class DatabaseProbe : public ResourceDatabase
        {
        public:
            SmartPtr<ITexture> resolved;
            unsigned lookups = 0;
            SmartPtr<IResource> loadResource( const String & ) override
            {
                ++lookups;
                return resolved;
            }
            SmartPtr<IResource> loadResourceById( const workphone::UUID & ) override { return resolved; }
        };
        struct RestoreServices
        {
            Fixture &fixture;
            ~RestoreServices()
            {
                fixture.app->setResourceDatabase( fixture.resources );
                fixture.app->setGraphicsSystem( fixture.graphics );
            }
        } restore{ fixture };
        auto database = make_ptr<DatabaseProbe>();
        fixture.app->setResourceDatabase( database );
        auto unit = make_ptr<UnitProbe>();
        require( sizeof( ClawMaterialTexture ) == sizeof( MaterialTexture ) &&
                 unit->getTextureName().empty() && unit->getUnresolvedTextureNameObjectBytes() == 0,
                 "empty texture units have no derived name copy or cold name allocation" );
        for( const auto &name : { String( "missing.png" ), String( 512, 'x' ),
                                  String( u8"Textures/石/é.png" ), String( "a\0b", 3 ) } )
        {
            unit->setTextureName( name );
            auto pending = unit->pendingName();
            const auto lookups = database->lookups;
            unit->setTextureName( name );
            require( unit->getTextureName() == name && !unit->getTexture() &&
                     unit->getUnresolvedTextureNameObjectBytes() == sizeof( SourceBudgetString ) &&
                     unit->pendingName() == pending && database->lookups == lookups + 1,
                     "unresolved names retain exact bytes and retry lookup without replacing cold storage" );
            auto properties = unit->getProperties();
            require( properties->getProperty( IMaterialTexture::texturePathStr ) == name,
                     "Claw properties preserve unresolved texture names" );
            auto cloned = make_ptr<ClawMaterialTexture>();
            cloned->fromData( unit->toData() );
            require( cloned->getTextureName() == name && !cloned->getTexture(),
                     "Claw texture descriptors round-trip unresolved names" );
            auto base = make_ptr<MaterialTexture>();
            base->fromData( unit->MaterialTexture::toData() );
            require( base->getTextureName() == name && !base->getTexture(),
                     "base descriptors preserve unresolved texture path fallbacks" );
            unit->load( nullptr ); unit->unload( nullptr );
            require( unit->getTextureName() == name, "unresolved authoring survives load/unload" );
        }
        auto texture = make_ptr<TextureProbe>();
        texture->setFilePath( "Textures/Resolved.png" );
        texture->setName( "ResolvedTexture" );
        database->resolved = texture;
        unit->setTextureName( unit->getTextureName() );
        require( unit->getTexture() == texture && texture->loads == 1 &&
                 !unit->pendingName() && unit->getTextureName() == texture->getFilePath(),
                 "later resource availability resolves pending names and retires cold storage" );
        auto pass = make_ptr<ClawMaterialPass>();
        pass->addTextureUnit( unit ); pass->load( nullptr );
        auto snapshot = [&] {
            return pass->getStateContext()->getStateDataById<MaterialPassStateData>( pass->getId() )->getRenderState();
        };
        const auto bound = snapshot();
        for( int i = 0; i < 32; ++i ) unit->setTextureName( "Textures/Resolved.png" );
        require( snapshot() == bound && texture->loads == 1 && !unit->pendingName(),
                 "repeated resolved names retain one binding without intermediate null publications" );
        texture->setFilePath( "Textures/Renamed.png" );
        require( unit->getTextureName() == "Textures/Renamed.png" &&
                 static_pointer_cast<Properties>( unit->MaterialTexture::toData() )->getProperty( "texturePath" ) == "Textures/Renamed.png" &&
                 unit->getProperties()->getProperty( MaterialTexture::textureStr ) == texture->getHandle()->getUUIDAsString() &&
                 snapshot() == bound, "bound names follow the resource without changing the render binding" );
        texture->setFilePath( "" );
        require( unit->getTextureName() == "ResolvedTexture", "bound names fall back to the resource name" );
        auto baseProperties = unit->MaterialTexture::getProperties();
        require( baseProperties->getProperty( IMaterialTexture::texturePathStr ) == texture->getHandle()->getUUIDAsString(),
                 "base editor resource-identifier property schema is preserved" );
        auto replacement = make_ptr<TextureProbe>();
        replacement->setFilePath( "Textures/Replacement.png" );
        database->resolved = replacement;
        unit->setTextureName( "Textures/Replacement.png" );
        const auto replaced = snapshot();
        require( replaced->revision == bound->revision + 1 && replaced->textures[0] == replacement &&
                 bound->textures[0] == texture && !unit->pendingName(),
                 "named replacement publishes once and preserves retained resource ownership" );
        database->resolved = nullptr;
        unit->setTextureName( "Textures/Unavailable.png" );
        const auto missing = snapshot();
        unit->setTextureName( "Textures/Unavailable.png" );
        require( missing->revision == replaced->revision + 1 && !missing->textures[0] &&
                 replaced->textures[0] == replacement && snapshot() == missing,
                 "unresolved replacement clears the binding once and keeps retained snapshots valid" );
        unit->MaterialTexture::setTexture( texture );
        require( !unit->pendingName() && unit->getTextureName() == "ResolvedTexture",
                 "base-qualified binding clears the same unresolved name owner" );
        unit->MaterialTexture::setTexture( nullptr );
        require( unit->getTextureName().empty() && !unit->pendingName(),
                 "base-qualified clear cannot resurrect an obsolete authored name" );
        unit->setTextureName( "Pending" ); unit->setTextureName( "" );
        require( !unit->pendingName() && unit->getTextureName().empty(), "empty name releases cold storage" );
        auto base = make_ptr<MaterialTexture>();
        database->resolved = texture; texture->setLoadingState( LoadingState::Unloaded );
        base->setTextureName( "ResolvedTexture" );
        require( base->getTexture() == texture && texture->loads == 1 &&
                 base->getUnresolvedTextureNameObjectBytes() == 0, "base name adapter preserves deferred loading" );
        auto descriptor = unit->MaterialTexture::textureToData( texture );
        base->setTexture( nullptr ); base->fromData( descriptor );
        require( base->getTexture() == texture && texture->loads == 2 &&
                 base->getUnresolvedTextureNameObjectBytes() == 0, "UUID restore retains resource ownership without a name copy" );
        auto unloaded = make_ptr<ClawMaterialTexture>();
        unloaded->setTexture( texture ); unloaded->setLoadingState( LoadingState::Loaded );
        unloaded->unload( nullptr );
        require( !unloaded->getTexture() && unloaded->getTextureName() == "ResolvedTexture" &&
                 unloaded->getUnresolvedTextureNameObjectBytes() == sizeof( SourceBudgetString ),
                 "released resolved resources hand their authoring name to cold storage during unload" );
        unloaded->setTexture( texture );
        require( unloaded->getUnresolvedTextureNameObjectBytes() == 0,
                 "rebinding after unload retires the retained authoring name" );
        unloaded->setTexture( nullptr );
        fixture.app->setResourceDatabase( nullptr ); fixture.app->setGraphicsSystem( nullptr );
        base->setTextureName( "Offline/Pending.png" );
        auto offline = make_ptr<MaterialTexture>();
        offline->fromData( base->toData() );
        require( offline->getTextureName() == "Offline/Pending.png" && !offline->getTexture(),
                 "detached descriptors preserve unresolved paths before graphics/database startup" );
        // Restore graphics before releasing the attached pass.
        fixture.app->setGraphicsSystem( fixture.graphics );
        pass->unload( nullptr );
        std::cout << "texture_name_storage base_unit_bytes=" << sizeof( MaterialTexture )
                  << " claw_unit_bytes=" << sizeof( ClawMaterialTexture )
                  << " empty_or_resolved_cold_object_bytes=0 unresolved_string_object_bytes=" << sizeof( String )
                  << " string_capacity_and_allocator_overhead_excluded=1\n";
    }

    void runLazyTextureUnits(Fixture &fixture)
    {
        using namespace render;
        auto material = make_ptr<ClawMaterial>(); material->load(nullptr);
        auto pass = dynamic_pointer_cast<MaterialPass>(material->getTechnique(0)->getPass(0));
        // The pass constructor loads before its owner is attached. Exercise the
        // attached-pass initialization used by authored material load/reload.
        pass->load(nullptr);
        const auto slots = pass->getNumTexturesNodes();
        require(slots == static_cast<size_t>(PbsTextureTypes::NUM_PBSM_TEXTURE_TYPES) &&
                pass->getMaterializedTextureUnitCount() == 0, "default material keeps logical slots without authoring objects");
        const auto initial = material->getRenderState();
        for(int i = 0; i < 100; ++i)
            require(!material->getTexture(0) && !material->getTexture(31) && material->getRenderState() == initial,
                    "warm texture/snapshot reads stay on dense state");
        auto serialized = static_pointer_cast<Properties>(pass->toData());
        require(serialized->getChildrenByName("textures").size() == slots &&
                pass->getMaterializedTextureUnitCount() == 0, "serialization preserves ordinal descriptors without materializing empty units");
        auto restored = make_ptr<ClawMaterial>(); restored->load(nullptr);
        auto restoredPass = dynamic_pointer_cast<MaterialPass>(restored->getTechnique(0)->getPass(0));
        restoredPass->fromData(serialized);
        require(restoredPass->getNumTexturesNodes() == slots && restoredPass->getMaterializedTextureUnitCount() == 0,
                "empty legacy descriptors reload lazily");
        pass->setTexture(SmartPtr<ITexture>(), 0);
        require(pass->getMaterializedTextureUnitCount() == 0, "clearing an implicit slot allocates no facade");
        auto first = make_ptr<ClawTexture>();
        auto second = make_ptr<ClawTexture>();
        pass->setTexture(first, 0);
        require(pass->getMaterializedTextureUnitCount() == 1 && material->getTexture(0) == first,
                "binding one texture creates one authoring unit");
        pass->setTexture(String(), 31);
        require(material->getTexture(0) == first && pass->getNumTexturesNodes() == 32 &&
                pass->getMaterializedTextureUnitCount() == 1, "string-based extension preserves earlier bindings and leaves gaps implicit");
        auto previous = material->getRenderState();
        auto unit = pass->getTextureUnit(31);
        require(unit && unit->getParent().get() == pass.get() && unit->getTextureType() == 31 &&
                pass->getMaterializedTextureUnitCount() == 2 && material->getRenderState() == previous,
                "single-slot authoring access preserves state identity and ordinal metadata");
        unit->setTexture(second);
        require(material->getTexture(31) == second && !previous->textures[31], "extension facade edits publish through canonical state");
        {
            auto state = pass->getStateContext()->invalidateStateDataById<MaterialPassStateData>(pass->getId());
            state->setTextureBinding( 25, second );
        }
        auto data = static_pointer_cast<Properties>(pass->toData());
        const auto descriptors = data->getChildrenByName("textures");
        require(descriptors.size() == 32 && pass->getMaterializedTextureUnitCount() == 2 &&
                descriptors[0]->getProperty("texture") == first->getHandle()->getUUIDAsString() &&
                descriptors[31]->getProperty("texture") == second->getHandle()->getUUIDAsString() &&
                descriptors[25]->getProperty("texture") == second->getHandle()->getUUIDAsString(),
                "implicit runtime binding serializes through the shared UUID/path descriptor writer");
        require(pass->getTextureUnit(25)->getTexture() == second, "authoring facade borrows an existing direct state binding");
        const auto all = pass->getTextureUnits();
        require(all.size() == 32 && pass->getMaterializedTextureUnitCount() == 32 && !pass->getTextureUnit(32),
                "legacy authoring enumeration returns all concrete units with bounds checking");
        for(u32 i = 0; i < all.size(); ++i)
            require(all[i] && all[i] == pass->getTextureUnit(i), "materialized facade identities remain stable");
        pass->removeTextureUnit(all[0]);
        require(pass->getNumTexturesNodes() == 31 && material->getTexture(24) == second && material->getTexture(30) == second,
                "authoring removal compacts binding ordinals consistently");
        material->unload(nullptr); restored->unload(nullptr);
        fixture.graphics->pending.clear();

        constexpr u32 count = 1000;
        std::vector<SmartPtr<ClawMaterial>> population;
        for(u32 i = 0; i < count; ++i)
        {
            auto value = make_ptr<ClawMaterial>(); value->load(nullptr);
            auto p = dynamic_pointer_cast<MaterialPass>(value->getTechnique(0)->getPass(0));
            p->load(nullptr);
            require(p->getNumTexturesNodes() == slots && p->getMaterializedTextureUnitCount() == 0,
                    "default material populations allocate zero texture authoring objects");
            population.push_back(value);
        }
        std::cout << "lazy_texture_units configured_materials=" << count << " slots_per_pass=" << slots
                  << " eager_unit_count=" << count * slots << " lazy_unit_count=0"
                  << " avoided_unit_object_bytes=" << count * slots * sizeof(MaterialTexture)
                  << " texture_unit_bytes=" << sizeof(MaterialTexture)
                  << " allocator_handles_and_other_graph_storage_excluded=1\n";
        for(auto &value : population) value->unload(nullptr);
        fixture.graphics->pending.clear();
    }

    void runRootMaterialStateStorage( Fixture &fixture )
    {
        using namespace render;
        require( sizeof( MaterialStateData ) == sizeof( StateData ),
                 "root material state contains no duplicate surface, texture or resource-name payload" );
        for( bool pbr : { false, true } )
        {
            auto material = make_ptr<ClawMaterial>(); material->setName( "RootStateStorage" );
            if( pbr )
            {
                auto schema = observedSchema();
                material->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
            }
            material->load( nullptr );
            auto context = material->getStateContext();
            require( context && context->getOwner().get() == material.get(), "root context retains its material owner" );
            auto states = context->getStates();
            require( states.size() == 1, "loaded material retains one root lifecycle state" );
            auto root = dynamic_pointer_cast<MaterialStateData>( states[0]->getData() );
            require( root && root->getOwner() == states[0], "factory-created root retains its material type and state owner" );
            const ColourF colour( 0.2f, 0.4f, 0.6f, 1 );
            material->setDiffuse( colour ); material->setMetalness( 0.3f ); material->setRoughness( 0.6f );
            auto properties = material->getProperties(); material->setProperties( properties ); material->load( nullptr );
            require( material->getStateContext() == context && states[0]->getData() == root &&
                     material->getDiffuse() == colour && material->getMetalness() == 0.3f &&
                     material->getRoughness() == 0.6f && material->getName() == "RootStateStorage",
                     "surface edits, property round-trip and repeated load keep the root marker and active values" );
            material->unload( nullptr );
            require( !material->getStateContext() && !context->getOwner() && context->getStates().empty() &&
                     context->getStateListeners().empty() && !states[0]->getData(),
                     "unload retires the compact root through the existing context lifecycle" );
            material->load( nullptr );
            auto replacement = material->getStateContext();
            require( replacement && replacement != context && replacement->getStates().size() == 1 &&
                     replacement->getStates()[0]->getData() != root && material->getMetalness() == 0.3f &&
                     material->getRoughness() == 0.6f, "reload creates a new root without losing pass/shader-owned surface values" );
            material = nullptr;
            require( !replacement->getOwner() && replacement->getStateListeners().empty() && replacement->getStates().empty(),
                     "destruction retires the replacement root even when its context is retained" );
        }
        fixture.graphics->pending.clear();
    }

    void runColorOnlySurfaceOwnership( Fixture &fixture )
    {
        using namespace render;
        MaterialShaderRegistry::instance().registerBuiltinShaders();
        const auto info = MaterialShaderRegistry::instance().find( Material::colorOnlyShaderIDStr )->getParameterInfo();
        const ColourF colour( 0.2f, 0.4f, 0.6f, 0.7f ), glow( 0.1f, 0.8f, 0.2f, 0.3f );
        const ColourF localDiffuse( 1, 0, 0, 0.9f ), localEmission( 0, 0, 1, 1 );
        const auto rendered = []( const ColourF &value, f32 alpha ) {
            return ColourF( value.r, value.g, value.b, alpha );
        };
        const auto stateFor = []( SmartPtr<IMaterialPass> pass ) {
            return static_pointer_cast<MaterialPassStateData>(
                pass->getStateContext()->getStateById( pass->getId() )->getData() );
        };
        auto material = make_ptr<ClawMaterial>(); material->setOpacity( 0.2f );
        material->setShaderParameters( info, Material::colorOnlyShaderIDStr );
        auto &parameters = material->getShaderParameters();
        const auto colorSlot = parameters.Resolve<ColourF>( "Color" );
        const auto opacity = parameters.Resolve<f32>( "Opacity" );
        const auto names = parameters.GetNameLookupCount();
        material->setDiffuse( colour ); material->load( nullptr );
        auto first = material->getTechnique( 0 )->getPass( 0 ); auto firstData = stateFor( first );
        require( parameters.GetColour( colorSlot.handle ) == colour && parameters.GetScalar( opacity.handle ) == 0.7f &&
                 first->getDiffuse() == colour && first->getEmissive() == colour && material->getLegacySurfaceStorageBytes() == 0,
                 "ColorOnly diffuse RGBA writes shared Color and Opacity before loading without a legacy mirror" );
        parameters.Set( colorSlot, glow );
        require( first->getDiffuse() == rendered( glow, 0.7f ) && first->getEmissive() == glow,
                 "typed Color keeps separate opacity while supplying both diffuse RGB and emission" );
        first->setDiffuse( localDiffuse ); first->setEmissive( localEmission );
        auto before = firstData->getRenderState();
        auto second = material->createTechnique()->createPass(); second->load( nullptr ); auto secondData = stateFor( second );
        require( firstData->getRenderState() == before && second->getDiffuse() == rendered( glow, 0.7f ) &&
                 second->getEmissive() == glow && secondData->getSurfaceState().opacity == 0.7f,
                 "ColorOnly graph additions seed both colours and opacity without replaying old copies over overrides" );
        material->setOpacity( 0.4f );
        require( first->getDiffuse() == rendered( localDiffuse, 0.4f ) && first->getEmissive() == localEmission,
                 "ColorOnly opacity-only edits preserve local RGB and emission overrides" );
        material->setEmissive( glow );
        require( first->getDiffuse() == rendered( glow, 0.4f ) && first->getEmissive() == glow &&
                 parameters.GetScalar( opacity.handle ) == 0.4f,
                 "same-authored emission alias restores the shared Color outputs without changing opacity" );
        before = firstData->getRenderState(); const auto secondBefore = secondData->getRenderState();
        auto retained = parameters;
        material->batch( [&]( Material &m ) {
            parameters.Set( colorSlot, localEmission ); m.setDiffuse( colour );
            parameters.Set( opacity, 0.2f ); m.setEditorFloat( "opacity", 0.6f ); m.setEmissive( glow );
            require( firstData->getRenderState() == before, "ColorOnly batches retain the previous render version" );
        } );
        require( firstData->getRenderState()->revision == before->revision + 1 &&
                 secondData->getRenderState()->revision == secondBefore->revision + 1 &&
                 first->getDiffuse() == rendered( glow, 0.6f ) && first->getEmissive() == glow &&
                 parameters.GetColour( colorSlot.handle ) == glow && parameters.GetScalar( opacity.handle ) == 0.6f &&
                 retained.GetScalar( opacity.handle ) == 0.4f,
                 "mixed ColorOnly writes obey call order, detach shared payloads and publish once per pass" );
        material->applyShaderParametersToRenderer();
        require( material->getLegacySurfaceStorageBytes() == 0, "ColorOnly full apply does not recreate the opacity mirror" );
        first->setDiffuse( localDiffuse ); first->setEmissive( localEmission );
        auto properties = material->getProperties();
        auto restored = make_ptr<ClawMaterial>(); restored->load( nullptr ); restored->setProperties( properties );
        auto &values = restored->getShaderParameters();
        require( restored->getDiffuse() == localDiffuse && restored->getEmissive() == localEmission &&
                 values.GetColour( values.GetBuiltin( MaterialBuiltinParameter::Color ) ) == glow &&
                 values.GetScalar( values.GetBuiltin( MaterialBuiltinParameter::Opacity ) ) == 0.6f,
                 "ColorOnly saved native aliases preserve independent overrides and canonical shader values" );
        const auto storage = values.GetValueStorageIdentity(); const auto revision = values.GetRevision();
        restored->setProperties( properties );
        require( values.GetValueStorageIdentity() == storage && values.GetRevision() == revision,
                 "no-op ColorOnly restore preserves payload sharing and revision" );
        restored->applyShaderParametersToRenderer();
        require( restored->getDiffuse() == rendered( glow, 0.6f ) && restored->getEmissive() == glow,
                 "full ColorOnly reapply restores both shared-colour outputs and authored opacity" );
        auto partial = make_ptr<Properties>(); partial->setProperty( "diffuseColour", colour );
        restored->setProperties( partial );
        require( values.GetColour( values.GetBuiltin( MaterialBuiltinParameter::Color ) ) == colour &&
                 values.GetScalar( values.GetBuiltin( MaterialBuiltinParameter::Opacity ) ) == 0.7f,
                 "partial legacy diffuse RGBA bags update both ColorOnly canonical slots" );
        restored->unload( nullptr );
        require( parameters.GetNameLookupCount() == names, "ColorOnly mutation uses cached semantic handles" );
        {
            auto edit = parameters.Edit(); material->setDiffuse( colour ); material->setOpacity( 0.25f );
            auto pending = material->createTechnique()->createPass(); pending->load( nullptr );
            require( pending->getDiffuse() == rendered( colour, 0.25f ) && pending->getEmissive() == colour,
                     "ColorOnly passes seed pending canonical edits before parameter publication closes" );
        }
        material->unload( nullptr ); material->load( nullptr );
        require( material->getDiffuse() == rendered( colour, 0.25f ) && material->getEmissive() == colour &&
                 material->getLegacySurfaceStorageBytes() == 0, "ColorOnly canonical values survive graph reload" );
        material->unload( nullptr );

        auto imported = make_ptr<ClawMaterial>(); imported->setOpacity( 0.35f );
        imported->setShaderParameters( info, Material::colorOnlyShaderIDStr ); imported->syncPBRToShaderParameters();
        imported->load( nullptr );
        require( imported->getOpacity() == 0.35f && imported->getLegacySurfaceStorageBytes() == 0,
                 "explicit ColorOnly import retires pre-load legacy opacity" );
        imported->setDiffuse( colour ); auto importedFirst = imported->getTechnique( 0 )->getPass( 0 );
        importedFirst->setEmissive( localEmission ); imported->syncPBRToShaderParameters();
        auto importedPass = imported->createTechnique()->createPass(); importedPass->load( nullptr );
        require( importedFirst->getEmissive() == localEmission && importedPass->getEmissive() == colour &&
                 importedPass->getDiffuse() == colour,
                 "explicit ColorOnly import chooses primary diffuse for shared Color without feeding back into overrides" );
        imported->unload( nullptr );

        auto cleared = make_ptr<ClawMaterial>(); cleared->setShaderParameters( info, Material::colorOnlyShaderIDStr );
        cleared->setOpacity( 0.42f ); cleared->setShaderID( String() ); cleared->load( nullptr );
        require( cleared->getOpacity() == 0.42f && cleared->getLegacySurfaceStorageBytes() > 0,
                 "clearing ColorOnly hands declared opacity back before first-load legacy seeding" );
        cleared->unload( nullptr );
        MaterialShader sparse( Material::colorOnlyShaderIDStr );
        sparse.addParameter( MaterialShaderParameterType::Colour, "Color" );
        auto removed = make_ptr<ClawMaterial>(); removed->setShaderParameters( info, Material::colorOnlyShaderIDStr );
        removed->setDiffuse( rendered( colour, 0.43f ) ); removed->setShaderParameters( sparse.getParameterInfo(), sparse.getShaderID() );
        removed->load( nullptr );
        require( removed->getOpacity() == 0.43f && removed->getLegacySurfaceStorageBytes() > 0,
                 "removing ColorOnly opacity preserves its last declared value for future passes" );
        removed->unload( nullptr );
        material->setShaderParameters( sparse.getParameterInfo(), sparse.getShaderID() ); material->load( nullptr );
        parameters.SetColour( parameters.GetBuiltin( MaterialBuiltinParameter::Color ), colour );
        const auto sparseRevision = parameters.GetRevision(); material->setDiffuse( localDiffuse ); material->setOpacity( 0.3f );
        require( parameters.GetRevision() == sparseRevision && material->getDiffuse() == rendered( localDiffuse, 0.3f ),
                 "ColorOnly without Opacity keeps diffuse RGBA and opacity edits on the legacy fallback" );
        auto fallback = material->createTechnique()->createPass(); fallback->load( nullptr );
        require( fallback->getDiffuse() == rendered( colour, 0.3f ) && fallback->getEmissive() == colour,
                 "partial ColorOnly seeds declared Color and retained legacy opacity independently" );
        parameters.SetColour( parameters.GetBuiltin( MaterialBuiltinParameter::Color ), glow );
        require( material->getOpacity() == 0.3f && fallback->getDiffuse() == rendered( glow, 0.3f ),
                 "partial ColorOnly colour edits preserve explicit legacy opacity over the implicit opaque default" );
        material->unload( nullptr );
        MaterialShader wrongType( Material::colorOnlyShaderIDStr );
        wrongType.addParameter( MaterialShaderParameterType::Int, "Color" )
                 .addParameter( MaterialShaderParameterType::Int, "Opacity" );
        material->setShaderParameters( wrongType.getParameterInfo(), wrongType.getShaderID() ); material->load( nullptr );
        const auto customColor = parameters.Resolve<s32>( "Color" ), customOpacity = parameters.Resolve<s32>( "Opacity" );
        parameters.Set( customColor, s32( 17 ) ); parameters.Set( customOpacity, s32( 23 ) );
        material->setDiffuse( localDiffuse ); material->setEmissive( localEmission ); material->setOpacity( 0.45f );
        require( parameters.GetInt( customColor.handle ) == 17 && parameters.GetInt( customOpacity.handle ) == 23 &&
                 material->getDiffuse() == rendered( localDiffuse, 0.45f ) && material->getEmissive() == localEmission,
                 "wrong-type ColorOnly slots remain independent custom values while legacy setters retain their contract" );
        material->unload( nullptr ); fixture.graphics->pending.clear();
    }

    void runUnlitSurfaceOwnership( Fixture &fixture )
    {
        using namespace render;
        MaterialShaderRegistry::instance().registerBuiltinShaders();
        const auto info = MaterialShaderRegistry::instance().find( Material::unlitShaderIDStr )->getParameterInfo();
        const ColourF glow( 0.2f, 0.4f, 0.6f, 1 ), localGlow( 0.9f, 0.1f, 0.3f, 1 );
        auto material = make_ptr<ClawMaterial>(); material->setOpacity( 0.25f );
        require( material->getLegacySurfaceStorageBytes() > 0, "pre-binding opacity has a legacy owner" );
        material->setShaderParameters( info, Material::unlitShaderIDStr );
        auto &parameters = material->getShaderParameters();
        const auto emission = parameters.Resolve<ColourF>( "Emissive" );
        const auto opacity = parameters.Resolve<f32>( "Opacity" );
        const auto names = parameters.GetNameLookupCount();
        material->setEmissive( glow ); material->setEditorFloat( "opacity", 0.6f );
        require( parameters.GetColour( emission.handle ) == glow && parameters.GetScalar( opacity.handle ) == 0.6f &&
                 material->getLegacySurfaceStorageBytes() == 0,
                 "Unlit material and named edits transfer pre-load values to the shader owner" );
        material->load( nullptr );
        const auto stateFor = []( SmartPtr<IMaterialPass> pass ) {
            return static_pointer_cast<MaterialPassStateData>(
                pass->getStateContext()->getStateById( pass->getId() )->getData() );
        };
        auto first = material->getTechnique( 0 )->getPass( 0 );
        auto firstData = stateFor( first );
        require( first->getEmissive() == glow && firstData->getSurfaceState().opacity == 0.6f &&
                 first->getDiffuse().a == 0.6f && firstData->getRenderState()->getFlag( transparentFlag ),
                 "initial Unlit passes seed canonical colour, opacity and transparent normalization" );
        first->setEmissive( localGlow ); first->setDiffuse( ColourF( 1, 0, 0, 0.2f ) );
        auto before = firstData->getRenderState();
        auto second = material->createTechnique()->createPass(); second->load( nullptr );
        auto secondData = stateFor( second );
        require( firstData->getRenderState() == before && second->getEmissive() == glow && secondData->getSurfaceState().opacity == 0.6f,
                 "new Unlit passes read the canonical owner without replaying values over local overrides" );
        material->setEmissive( glow );
        require( first->getEmissive() == glow && first->getDiffuse() == ColourF( 1, 0, 0, 0.2f ),
                 "same-authored Unlit emission restores only emission overrides" );
        material->setOpacity( 0.6f );
        require( first->getDiffuse() == ColourF( 1, 0, 0, 0.6f ) && secondData->getSurfaceState().opacity == 0.6f,
                 "same-authored Unlit opacity restores alpha while preserving local RGB" );
        before = firstData->getRenderState(); const auto secondBefore = secondData->getRenderState();
        auto retained = parameters;
        material->batch( [&]( Material &m ) {
            parameters.Set( emission, localGlow ); m.setEmissive( glow );
            parameters.Set( opacity, 0.2f ); m.setEditorFloat( "opacity", 0.4f );
            require( firstData->getRenderState() == before, "Unlit batches retain the previous immutable version" );
        } );
        require( firstData->getRenderState()->revision == before->revision + 1 &&
                 secondData->getRenderState()->revision == secondBefore->revision + 1 &&
                 first->getDiffuse() == ColourF( 1, 0, 0, 0.4f ) && first->getEmissive() == glow &&
                 retained.GetScalar( opacity.handle ) == 0.6f && parameters.GetScalar( opacity.handle ) == 0.4f,
                 "mixed Unlit edits obey call order, detach shared bytes and publish once per pass" );
        material->applyShaderParametersToRenderer();
        require( material->getLegacySurfaceStorageBytes() == 0,
                 "full Unlit apply does not allocate a legacy opacity copy" );
        first->setEmissive( localGlow );
        auto properties = material->getProperties();
        auto restored = make_ptr<ClawMaterial>(); restored->load( nullptr ); restored->setProperties( properties );
        auto &restoredValues = restored->getShaderParameters();
        require( restored->getEmissive() == localGlow &&
                 restoredValues.GetColour( restoredValues.GetBuiltin( MaterialBuiltinParameter::Emissive ) ) == glow,
                 "Unlit property aliases restore local emission without overwriting canonical shader colour" );
        const auto storage = restoredValues.GetValueStorageIdentity();
        const auto revision = restoredValues.GetRevision();
        restored->setProperties( properties );
        require( restoredValues.GetValueStorageIdentity() == storage && restoredValues.GetRevision() == revision,
                 "no-op Unlit property restore retains authored sharing and revision" );
        const auto restoredLegacyBytes = restored->getLegacySurfaceStorageBytes();
        restored->applyShaderParametersToRenderer();
        require( restored->getEmissive() == glow && restored->getOpacity() == 0.4f,
                 "full Unlit reapply restores authored emission and opacity" );
        require( restored->getLegacySurfaceStorageBytes() == restoredLegacyBytes,
                 "full Unlit reapply retains only the existing fallback block from native scalar aliases" );
        auto partial = make_ptr<Properties>(); partial->setProperty( "emissiveColour", localGlow );
        restored->setProperties( partial );
        require( restoredValues.GetColour( restoredValues.GetBuiltin( MaterialBuiltinParameter::Emissive ) ) == localGlow,
                 "legacy-only Unlit emission property edits update the canonical owner" );
        restored->unload( nullptr );
        require( parameters.GetNameLookupCount() == names, "Unlit mutation uses cached semantic handles" );
        {
            auto edit = parameters.Edit();
            material->setEmissive( localGlow ); material->setOpacity( 0.3f );
            auto pending = material->createTechnique()->createPass(); pending->load( nullptr );
            require( pending->getEmissive() == localGlow && stateFor( pending )->getSurfaceState().opacity == 0.3f,
                     "Unlit graph additions seed pending canonical edits before publication closes" );
        }
        material->unload( nullptr ); material->load( nullptr );
        require( material->getEmissive() == localGlow && material->getOpacity() == 0.3f &&
                 material->getLegacySurfaceStorageBytes() == 0, "Unlit canonical values survive graph reload" );
        material->unload( nullptr );

        auto imported = make_ptr<ClawMaterial>(); imported->setOpacity( 0.35f );
        imported->setShaderParameters( info, Material::unlitShaderIDStr ); imported->syncPBRToShaderParameters();
        imported->load( nullptr );
        require( imported->getOpacity() == 0.35f && imported->getLegacySurfaceStorageBytes() == 0,
                 "explicit Unlit import transfers pre-load opacity and retires its legacy owner" );
        imported->unload( nullptr );

        auto mixed = make_ptr<ClawMaterial>(); mixed->setMetalness( 0.2f ); mixed->setOpacity( 0.3f );
        mixed->setShaderParameters( info, Material::unlitShaderIDStr ); mixed->setOpacity( 0.7f );
        mixed->load( nullptr );
        auto mixedPass = mixed->createTechnique()->createPass(); mixedPass->load( nullptr );
        require( mixed->getLegacySurfaceStorageBytes() > 0 && mixedPass->getMetalness() == 0.2f &&
                 stateFor( mixedPass )->getSurfaceState().opacity == 0.7f,
                 "Unlit opacity transfer retains unrelated legacy fields in the optional block" );
        mixed->unload( nullptr );

        auto cleared = make_ptr<ClawMaterial>();
        cleared->setShaderParameters( info, Material::unlitShaderIDStr ); cleared->setOpacity( 0.42f );
        cleared->setShaderID( String() ); cleared->load( nullptr );
        require( cleared->getOpacity() == 0.42f && cleared->getLegacySurfaceStorageBytes() > 0,
                 "clearing Unlit transfers canonical opacity back to the legacy future-pass owner" );
        cleared->unload( nullptr );
        MaterialShader noOpacity( Material::unlitShaderIDStr );
        noOpacity.addParameter( MaterialShaderParameterType::Colour, "Emissive" );
        auto removed = make_ptr<ClawMaterial>();
        removed->setShaderParameters( info, Material::unlitShaderIDStr ); removed->setOpacity( 0.43f );
        removed->setShaderParameters( noOpacity.getParameterInfo(), noOpacity.getShaderID() );
        removed->load( nullptr );
        require( removed->getOpacity() == 0.43f && removed->getLegacySurfaceStorageBytes() > 0,
                 "removing Unlit opacity preserves its last canonical value through first graph creation" );
        removed->unload( nullptr );
        MaterialShader colorOnly( Material::colorOnlyShaderIDStr );
        colorOnly.addParameter( MaterialShaderParameterType::Colour, "Color" )
                 .addParameter( MaterialShaderParameterType::Scalar, "Opacity" );
        auto switched = make_ptr<ClawMaterial>();
        switched->setShaderParameters( info, Material::unlitShaderIDStr ); switched->setOpacity( 0.44f );
        switched->setShaderParameters( colorOnly.getParameterInfo(), colorOnly.getShaderID() );
        switched->load( nullptr );
        require( switched->getOpacity() == 0.44f,
                 "Unlit to ColorOnly schema migration retains canonical opacity" );
        switched->unload( nullptr );

        MaterialShader wrongType( Material::unlitShaderIDStr );
        wrongType.addParameter( MaterialShaderParameterType::Int, "Opacity" )
                 .addParameter( MaterialShaderParameterType::Int, "Emissive" );
        material->setShaderParameters( wrongType.getParameterInfo(), wrongType.getShaderID() ); material->load( nullptr );
        const auto customOpacity = parameters.Resolve<s32>( "Opacity" ), customEmission = parameters.Resolve<s32>( "Emissive" );
        parameters.Set( customOpacity, s32( 17 ) ); parameters.Set( customEmission, s32( 23 ) );
        material->setOpacity( 0.45f ); material->setEmissive( glow );
        require( material->getOpacity() == 0.45f && material->getEmissive() == glow &&
                 material->getLegacySurfaceStorageBytes() > 0 && parameters.GetInt( customOpacity.handle ) == 17 &&
                 parameters.GetInt( customEmission.handle ) == 23,
                 "wrong-type Unlit semantics retain the legacy contract and independent custom values" );
        material->unload( nullptr ); fixture.graphics->pending.clear();
    }

    void runLegacySurfaceStorage( Fixture &fixture )
    {
        using namespace render;
        auto material = make_ptr<ClawMaterial>();
        require( material->getLegacySurfaceStorageBytes() == 0,
                 "default material has no legacy future-pass allocation" );
        material->load( nullptr ); material->getProperties();
        require( material->getLegacySurfaceStorageBytes() == 0,
                 "loading and exporting an unchanged legacy material does not allocate pending values" );
        material->setMetalness( 0.25f ); material->setRoughness( 0.75f ); material->setOpacity( 0.6f );
        const auto legacyBytes = material->getLegacySurfaceStorageBytes();
        require( legacyBytes > 0, "legacy surface edits retain their future-pass values in cold storage" );
        material->unload( nullptr ); material->load( nullptr );
        require( material->getMetalness() == 0.25f && material->getRoughness() == 0.75f &&
                 material->getOpacity() == 0.6f && material->getLegacySurfaceStorageBytes() == legacyBytes,
                 "cold legacy surface values survive graph reload" );
        auto added = material->createTechnique()->createPass(); added->load( nullptr );
        auto addedData = static_pointer_cast<MaterialPassStateData>(
            added->getStateContext()->getStateById( added->getId() )->getData() );
        require( added->getMetalness() == 0.25f && added->getRoughness() == 0.75f &&
                 added->getDiffuse().a == 0.6f && addedData->getSurfaceState().opacity == 0.6f && readUIntSetting( *addedData, MaterialUIntSetting::renderMode ) == 3u &&
                 addedData->getRenderState()->getFlag( transparentFlag ),
                 "newly attached legacy passes inherit cached values and opacity normalization" );
        added->setMetalness( 0.9f ); added->setMaterial( material ); added->load( nullptr );
        require( added->getMetalness() == 0.9f, "repeated attachment/load does not replay legacy defaults over a local override" );

        auto schema = observedSchema();
        material->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
        auto &parameters = material->getShaderParameters();
        {
            auto edit = parameters.Edit();
            parameters.SetScalar( parameters.GetBuiltin( MaterialBuiltinParameter::Metalness ), 0.4f );
            parameters.SetScalar( parameters.GetBuiltin( MaterialBuiltinParameter::Roughness ), 0.65f );
            parameters.SetScalar( parameters.GetBuiltin( MaterialBuiltinParameter::Opacity ), 0.8f );
        }
        require( material->getLegacySurfaceStorageBytes() == 0 && material->getMetalness() == 0.4f &&
                 material->getRoughness() == 0.65f && material->getOpacity() == 0.8f,
                 "committing canonical PBR values releases the redundant legacy block" );
        material->setMetalness( 0.45f ); material->setDiffuse( ColourF( 0.2f, 0.4f, 0.6f, 0.7f ) );
        material->setEditorFloat( "roughness", 0.6f ); material->applyShaderParametersToRenderer();
        require( material->getLegacySurfaceStorageBytes() == 0,
                 "typed, material, named and full PBR applies do not recreate the legacy mirror" );
        auto first = material->getTechnique( 0 )->getPass( 0 );
        auto firstData = static_pointer_cast<MaterialPassStateData>(
            first->getStateContext()->getStateById( first->getId() )->getData() );
        first->setMetalness( 0.91f ); first->setRoughness( 0.12f );
        first->setDiffuse( ColourF( 1, 0, 0, 0.22f ) );
        const auto before = firstData->getRenderState();
        auto canonicalPass = material->createTechnique()->createPass(); canonicalPass->load( nullptr );
        require( firstData->getRenderState() == before && canonicalPass->getMetalness() == 0.45f &&
                 canonicalPass->getRoughness() == 0.6f && canonicalPass->getDiffuse() == ColourF( 0.2f, 0.4f, 0.6f, 0.7f ) &&
                 material->getLegacySurfaceStorageBytes() == 0,
                 "adding PBR passes seeds canonical values without replaying a mirror over existing local overrides" );
        material->unload( nullptr ); material->load( nullptr );
        require( material->getMetalness() == 0.45f && material->getRoughness() == 0.6f &&
                 material->getOpacity() == 0.7f && material->getLegacySurfaceStorageBytes() == 0,
                 "PBR graph reload reads the canonical owner with no legacy allocation" );
        material->unload( nullptr );

        auto imported = make_ptr<ClawMaterial>();
        imported->setMetalness( 0.2f ); imported->setRoughness( 0.3f ); imported->setOpacity( 0.4f );
        imported->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
        imported->syncPBRToShaderParameters();
        require( imported->getLegacySurfaceStorageBytes() == 0,
                 "explicit import transfers pre-load values and releases their legacy owner" );
        imported->load( nullptr );
        require( imported->getMetalness() == 0.2f && imported->getRoughness() == 0.3f && imported->getOpacity() == 0.4f,
                 "explicit pre-load import preserves pending values through graph creation" );
        imported->batch( [&]( Material &m ) {
            auto pass = imported->getTechnique( 0 )->getPass( 0 );
            pass->setMetalness( 0.9f ); pass->setRoughness( 0.1f );
            m.syncPBRToShaderParameters();
        } );
        auto importedPass = imported->createTechnique()->createPass(); importedPass->load( nullptr );
        require( importedPass->getMetalness() == 0.9f && importedPass->getRoughness() == 0.1f &&
                 importedPass->getDiffuse().a == 0.4f && imported->getLegacySurfaceStorageBytes() == 0,
                 "explicit import inside a batch reads live pass values for future passes" );
        imported->unload( nullptr );

        auto loadedImport = make_ptr<ClawMaterial>(); loadedImport->load( nullptr );
        loadedImport->setMetalness( 0.2f ); loadedImport->setRoughness( 0.3f ); loadedImport->setOpacity( 0.4f );
        auto local = loadedImport->getTechnique( 0 )->getPass( 0 );
        local->setMetalness( 0.85f ); local->setRoughness( 0.15f );
        loadedImport->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
        loadedImport->syncPBRToShaderParameters();
        auto loadedPass = loadedImport->createTechnique()->createPass(); loadedPass->load( nullptr );
        require( loadedImport->getLegacySurfaceStorageBytes() == 0 && local->getMetalness() == 0.85f &&
                 loadedPass->getMetalness() == 0.85f && loadedPass->getRoughness() == 0.15f &&
                 loadedPass->getDiffuse().a == 0.4f,
                 "explicit loaded import retires stale legacy values before future graph additions" );
        loadedImport->unload( nullptr );

        // A partial schema must release only the fields it actually owns.
        auto partial = make_ptr<ClawMaterial>();
        partial->setMetalness( 0.2f ); partial->setRoughness( 0.3f ); partial->setOpacity( 0.4f );
        MaterialShader sparse( "LegacySurfaceSparsePBR" );
        sparse.addParameter( MaterialShaderParameterType::Scalar, "Metalness" );
        partial->setShaderParameters( sparse.getParameterInfo(), sparse.getShaderID() );
        partial->getShaderParameters().SetScalar(
            partial->getShaderParameters().GetBuiltin( MaterialBuiltinParameter::Metalness ), 0.8f );
        require( partial->getLegacySurfaceStorageBytes() == legacyBytes,
                 "a partial canonical transfer retains unrelated legacy values" );
        partial->load( nullptr );
        require( partial->getMetalness() == 0.8f && partial->getRoughness() == 0.3f && partial->getOpacity() == 0.4f,
                 "graph creation does not replay a retired legacy field over a canonical value" );
        sparse.addParameter( MaterialShaderParameterType::Scalar, "Roughness" )
              .addParameter( MaterialShaderParameterType::Scalar, "Opacity" );
        partial->setShaderParameters( sparse.getParameterInfo(), sparse.getShaderID() );
        partial->setRoughness( 0.55f );
        require( partial->getLegacySurfaceStorageBytes() == legacyBytes,
                 "legacy storage remains until the last retained field transfers" );
        partial->setOpacity( 0.75f );
        require( partial->getLegacySurfaceStorageBytes() == 0,
                 "transferring the last field frees the optional legacy storage" );
        partial->unload( nullptr );

        auto alpha = make_ptr<ClawMaterial>(); alpha->setOpacity( 0.2f );
        MaterialShader albedoOnly( "LegacySurfaceAlbedoOnly" );
        albedoOnly.addParameter( MaterialShaderParameterType::Colour, "Albedo" );
        alpha->setShaderParameters( albedoOnly.getParameterInfo(), albedoOnly.getShaderID() );
        alpha->setOpacity( 0.35f );
        require( alpha->getLegacySurfaceStorageBytes() == 0,
                 "Albedo-owned alpha retires the legacy opacity field without a scalar Opacity slot" );
        alpha->load( nullptr );
        require( alpha->getDiffuse().a == 0.35f && alpha->getOpacity() == 0.35f,
                 "implicit canonical opacity survives the first graph load" );
        alpha->unload( nullptr );

        auto unlit = make_ptr<ClawMaterial>();
        MaterialShader unlitSchema( Material::unlitShaderIDStr );
        unlitSchema.addParameter( MaterialShaderParameterType::Scalar, "Opacity" );
        unlit->setShaderParameters( unlitSchema.getParameterInfo(), unlitSchema.getShaderID() );
        unlit->getShaderParameters().SetScalar(
            unlit->getShaderParameters().GetBuiltin( MaterialBuiltinParameter::Opacity ), 0.5f );
        unlit->load( nullptr );
        auto unlitPass = unlit->createTechnique()->createPass(); unlitPass->load( nullptr );
        require( unlit->getLegacySurfaceStorageBytes() == 0 && unlitPass->getDiffuse().a == 0.5f,
                 "Unlit opacity seeds future passes without a legacy scalar copy" );
        unlit->unload( nullptr );
        std::cout << "legacy_surface_storage material_core_bytes=" << sizeof( ClawMaterial )
                  << " optional_block_bytes=" << legacyBytes << " canonical_pbr_block_bytes=0\n";
        fixture.graphics->pending.clear();
    }

    size_t inventoryStringBytes( const String &text )
    {
#if WP_USE_WP_STRING && WP_USE_STANDARD_STRING_ALLOCATOR
        if( text.using_inline_storage() ) return 0;
#endif
        return text.capacity() + 1;
    }

    class StorageInventoryMaterial : public render::ClawMaterial
    {
    public:
        size_t optionalOwnerBytes() const
        {
            size_t bytes = m_techniques.capacity() * sizeof( SmartPtr<render::IMaterialTechnique> );
            if( m_legacySurface ) bytes += sizeof( LegacySurfaceData );
            if( m_primaryPassSelection ) bytes += sizeof( PrimaryPassSelection );
            if( m_cubicTextures ) bytes += sizeof( Array<SmartPtr<render::ITexture>> ) +
                m_cubicTextures->capacity() * sizeof( SmartPtr<render::ITexture> );
            if( m_shaderAuthoring )
            {
                bytes += sizeof( ShaderAuthoringData ) + inventoryStringBytes( m_shaderAuthoring->shaderID ) +
                    m_shaderAuthoring->textureRefs.capacity() * sizeof( ShaderTextureRef ) +
                    m_shaderAuthoring->textureBindings.capacity() * sizeof( ShaderTextureBinding );
                for( const auto &reference : m_shaderAuthoring->textureRefs )
                    bytes += inventoryStringBytes( reference.paramName ) + inventoryStringBytes( reference.texturePath );
            }
            return bytes;
        }
    };

    // A scoped owned-payload inventory of real loaded graphs. It deliberately
    // reports its exclusions instead of treating object arithmetic as process
    // allocation profiling. The frozen comparison changes only the old inline
    // pass representation; every other measured owner has the same charge.
    void runOwnedStorageInventory( Fixture &fixture, size_t count )
    {
        using namespace render;
        MaterialShader schema( "OwnedStorageInventory" );
        for( u32 i = 0; i < 128; ++i )
            schema.addParameter( i % 4 == 0 ? MaterialShaderParameterType::Scalar :
                                 i % 4 == 1 ? MaterialShaderParameterType::Colour :
                                 i % 4 == 2 ? MaterialShaderParameterType::Matrix :
                                              MaterialShaderParameterType::Texture,
                                 "Value" + String( std::to_string( i ) ) );
        auto &registry = MaterialShaderRegistry::instance();
        registry.registerBuiltinShaders();
        registry.registerShader( schema );
        const auto layout = registry.find( schema.getShaderID() )->getLayout();
        MaterialShaderParametersInstance prototype;
        prototype.Reset( layout );
        const auto scalar = prototype.Resolve<f32>( "Value0" );
        prototype.Set( scalar, 0.75f );
        std::vector<SmartPtr<StorageInventoryMaterial>> materials;
        std::vector<SmartPtr<MaterialPassStateData>> states;
        std::vector<std::unique_ptr<CapturedMaterialMesh>> meshes;
        materials.reserve( count ); states.reserve( count ); meshes.reserve( count );
        for( size_t i = 0; i < count; ++i )
        {
            auto material = make_ptr<StorageInventoryMaterial>();
            material->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
            material->getShaderParameters() = prototype;
            material->load( nullptr );
            auto technique = material->getTechnique( 0 );
            require( material->getTechniques().size() == 1 && technique->getPasses().size() == 1,
                     "storage fixture uses the same one-technique one-pass graph for every instance" );
            auto pass = technique->getPass( 0 );
            require( static_pointer_cast<ClawMaterialPass>( pass )->getMaterializedTextureUnitCount() == 0,
                     "storage inventory does not materialize empty authoring facades" );
            auto state = pass->getStateContext()->getStateDataById<MaterialPassStateData>( pass->getId() );
            states.emplace_back( state.get() );
            auto mesh = std::make_unique<CapturedMaterialMesh>();
            mesh->setMaterial( material, 0 ); mesh->setMaterial( material, 1 );
            meshes.push_back( std::move( mesh ) ); materials.push_back( material );
        }
        ClawMaterialRenderCache cache;
        ClawMaterialDrawRecords oldView, currentView;
        std::unique_ptr<ClawMaterialDrawRecords> updatedView;
        const auto capture = [&]( ClawMaterialDrawRecords &view ) {
            view.clear();
            for( const auto &mesh : meshes ) view.capture( mesh.get() );
            view.seal( &cache );
        };
        capture( oldView ); capture( currentView );
        require( oldView.getPersistentTableIdentity() == currentView.getPersistentTableIdentity(),
                 "independent storage-fixture views share one persistent record table" );
        size_t peakSubtotal = 0;
        const auto inventory = [&]( const char *phase ) {
            size_t inlineBytes = count * ( sizeof( StorageInventoryMaterial ) + sizeof( ClawMaterialTechnique ) +
                sizeof( ClawMaterialPass ) + sizeof( MaterialStateData ) + sizeof( MaterialPassStateData ) );
            size_t optionalBytes = 0, nativeBytes = 0, pendingBytes = 0, valuesBytes = prototype.GetValueStorageBytes();
            std::unordered_set<const void *> values{prototype.GetValueStorageIdentity()};
            std::unordered_set<const MaterialRenderState *> current, retired;
            double checksum = 0;
            for( size_t i = 0; i < count; ++i )
            {
                auto &material = materials[i]; auto &state = states[i];
                optionalBytes += material->optionalOwnerBytes() + state->getSurfaceAuthoringStorageBytes() +
                    state->getNumericAuthoringStorageBytes() + state->getStringAuthoringStorageBytes();
                nativeBytes += wp_graphics_material_get_storage_size( material->getNativeMaterial() );
                pendingBytes += state->getPendingSurfaceStorageBytes();
                const auto &parameters = material->getShaderParameters();
                require( parameters.GetLayout() == layout, "owned inventory charges a truly shared shader layout once" );
                if( auto identity = parameters.GetValueStorageIdentity(); identity && values.insert( identity ).second )
                    valuesBytes += parameters.GetValueStorageBytes();
                const auto snapshot = state->getRenderState(); current.insert( snapshot.get() );
                checksum += snapshot->roughness + parameters.GetScalar( scalar.handle );
                if( !state->getPendingSurfaceStorageBytes() ) require( &state->getSurfaceState() == snapshot.get(),
                    "resting graph has no second writable surface owner" );
            }
            size_t views = 0;
            for( const auto *view : {&oldView, &currentView, updatedView.get()} )
                if( view )
                {
                    if( view->getRecordCount() ) ++views;
                    for( const auto &mesh : meshes )
                        if( const auto binding = view->find( mesh.get() ) )
                            if( const auto record = view->get( *binding, 0 ); record && !current.count( record->state ) )
                                retired.insert( record->state );
                }
            const auto *publishedSchema = registry.find( schema.getShaderID() );
            size_t descriptorBytes = sizeof( MaterialShader ) +
                publishedSchema->getParameterInfo().capacity() * sizeof( MaterialShaderParameterInfo ) +
                inventoryStringBytes( publishedSchema->getShaderID() );
            for( const auto &parameter : publishedSchema->getParameterInfo() ) descriptorBytes += inventoryStringBytes( parameter.m_name );
            const auto sharedBytes = layout->GetStorageBytes() + descriptorBytes + valuesBytes;
            const auto currentBytes = current.size() * sizeof( MaterialRenderState );
            const auto retiredBytes = retired.size() * sizeof( MaterialRenderState );
            const auto tableBytes = cache.getStorageBytes(); // Includes table/front object; charge once across all views.
            const auto viewBytes = sizeof( oldView ) + sizeof( currentView ) +
                oldView.getRetainedStorageBytes() + currentView.getRetainedStorageBytes() +
                ( updatedView ? sizeof( *updatedView ) + updatedView->getRetainedStorageBytes() : 0 );
            const auto subtotal = inlineBytes + optionalBytes + nativeBytes + sharedBytes + currentBytes +
                retiredBytes + pendingBytes + tableBytes + viewBytes;
            peakSubtotal = std::max( peakSubtotal, subtotal );
            // The preceding representation already had writable inline values;
            // it did not allocate a separate pending surface during these edits.
            const auto beforeReference = subtotal + count * ( sizeof( InlinePassStateStorageBeforeSurfaceMigration ) -
                sizeof( MaterialPassStateData ) ) - pendingBytes;
            std::cout << "owned_storage phase=" << phase << " materials=" << count << " parameters=128 views=" << views
                << " view_workspaces=" << ( updatedView ? 3 : 2 )
                << " inline_graph_object_bytes=" << inlineBytes << " optional_graph_payload_bytes=" << optionalBytes
                << " native_material_bytes=" << nativeBytes << " shared_schema_and_value_bytes=" << sharedBytes
                << " distinct_value_payloads=" << values.size() << " current_snapshot_bytes=" << currentBytes
                << " retired_snapshot_bytes=" << retiredBytes << " pending_surface_bytes=" << pendingBytes
                << " persistent_table_bytes=" << tableBytes << " view_storage_bytes=" << viewBytes
                << " measured_subtotal_bytes=" << subtotal << " sampled_peak_subtotal_bytes=" << peakSubtotal
                << " frozen_inline_pass_reference_subtotal_bytes=" << beforeReference
                << " source_payload_included=1"
                << " checksum=" << checksum
                << " context_listener_resource_handle_graph_child_capacity_control_blocks_allocator_gpu_tls_publication_queue_registry_spare_capacity_and_fixture_inputs_excluded=1\n";
            if( !pendingBytes ) require( subtotal < beforeReference,
                "resting loaded graphs reduce the scoped owned-storage subtotal after inline surface retirement" );
            return std::make_pair( pendingBytes, retiredBytes );
        };
        require( inventory( "shared_authored_resting" ).first == 0,
                 "shared authored population has no resting pending surfaces" );
        const auto sparseCount = std::max<size_t>( 1, count / 100 );
        for( size_t i = 0; i < sparseCount; ++i )
        {
            states[i]->lock(); states[i]->setSurfaceValue( &MaterialRenderState::roughness, 0.25f );
            materials[i]->getShaderParameters().Set( scalar, 0.25f );
        }
        require( inventory( "one_percent_pending" ).first == sparseCount * sizeof( MaterialRenderState ),
                 "sparse writes allocate one pending surface only for each changed pass" );
        for( size_t i = 0; i < sparseCount; ++i ) states[i]->unlock();
        capture( currentView );
        require( inventory( "one_percent_published_old_view_retained" ).second == sparseCount * sizeof( MaterialRenderState ),
                 "old views retain only the replaced immutable surface versions" );
        for( size_t i = 0; i < count; ++i )
        {
            states[i]->lock(); states[i]->setSurfaceValue( &MaterialRenderState::roughness, 0.75f );
            materials[i]->getShaderParameters().Set( scalar, 0.5f );
        }
        require( inventory( "all_pending_two_old_views" ).first == count * sizeof( MaterialRenderState ),
                 "all-edited peak reports pending copies separately from current and retired publications" );
        for( auto state : states ) state->unlock();
        require( inventory( "all_published_two_old_views" ).second ==
                     ( count + sparseCount ) * sizeof( MaterialRenderState ),
                 "independent old views account for both distinct generations without double counting" );
        updatedView = std::make_unique<ClawMaterialDrawRecords>();
        const auto beforeUpdatedPreparations = cache.getPreparationCount();
        capture( *updatedView );
        require( updatedView->getPersistentTableIdentity() == oldView.getPersistentTableIdentity() &&
                     updatedView->getPersistentTableIdentity() == currentView.getPersistentTableIdentity() &&
                     cache.getPreparationCount() == beforeUpdatedPreparations + count &&
                     cache.getResidentCount() == count * 2 + sparseCount,
                 "fully updated view prepares every latest version while both previous generations remain resident" );
        for( size_t i = 0; i < count; ++i )
        {
            const auto binding = updatedView->find( meshes[i].get() );
            const auto record = binding ? updatedView->get( *binding, 0 ) : nullptr;
            require( record && record->state == states[i]->getRenderState().get() &&
                         record->prepared->constants.surface.y == 0.75f,
                     "third view consumes the latest current surface and its freshly prepared constants" );
        }
        require( inventory( "all_updated_view_prepared_two_old_views_retained" ).second ==
                     ( count + sparseCount ) * sizeof( MaterialRenderState ),
                 "third-view peak charges all workspaces while deduplicating its current snapshots" );
        const auto currentHandle = cache.resolve( states.front()->getRenderState() );
        std::weak_ptr<const MaterialRenderState> currentLifetime = states.front()->getRenderState();
        oldView.clear(); cache.collectExpired();
        require( inventory( "oldest_view_released" ).second == count * sizeof( MaterialRenderState ) &&
                     cache.getResidentCount() == count * 2,
                 "oldest-view retirement releases only versions no longer retained by the other two views" );
        currentView.clear(); cache.collectExpired();
        require( inventory( "intermediate_view_released" ).second == 0 && cache.getResidentCount() == count,
                 "retiring the second old view releases the final obsolete versions and preserves current records" );
        updatedView->clear(); cache.collectExpired();
        require( inventory( "all_views_released_current_graphs_alive" ).second == 0 &&
                     cache.getResidentCount() == count && !currentLifetime.expired() && cache.lookup( currentHandle ),
                 "releasing the final captured view preserves versions still owned by current material graphs" );
        updatedView.reset();
        inventory( "updated_workspace_destroyed" );
        for( const auto &state : states ) require( state->getPendingSurfaceStorageBytes() == 0 &&
            state->getSurfaceState().roughness == 0.75f, "published graph retains its final surface without staging" );
        meshes.clear(); states.clear();
        for( auto &material : materials ) material->unload( nullptr );
        materials.clear(); fixture.graphics->pending.clear();
        cache.collectExpired();
        require( currentLifetime.expired() && cache.getResidentCount() == 0 && !cache.lookup( currentHandle ),
                 "current versions and their prepared handles retire after the final graph owners unload" );
        std::cout << "owned_storage_retirement materials=" << count
                  << " current_graph_versions_alive_after_view_release=1 resident_records_after_graph_unload=0"
                  << " persistent_table_retained_bytes=" << cache.getStorageBytes() << '\n';
    }

    void runSchemaRebindContracts( Fixture &fixture )
    {
        using namespace render;
        MaterialShader schema( "SchemaRebindContract" );
        schema.addParameter( MaterialShaderParameterType::Scalar, "Value" )
              .addParameter( MaterialShaderParameterType::Int, "Signed" )
              .addParameter( MaterialShaderParameterType::Colour, "Tint" )
              .addParameter( MaterialShaderParameterType::Texture, "Map" );
        MaterialShaderRegistry::instance().registerShader( schema );
        auto material = make_ptr<ClawMaterial>(); material->load( nullptr );
        material->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
        auto &parameters = material->getShaderParameters();
        const auto value = parameters.Resolve<f32>( "Value" );
        parameters.Set( value, 0.375f );
        parameters.SetScalar( parameters.GetParameter( 1 ), s32( -17 ) );
        parameters.SetColour( parameters.GetParameter( 2 ), ColourF( 0.1f, 0.2f, 0.3f, 0.4f ) );
        parameters.SetTexture( parameters.GetParameter( 3 ), MaterialResourceHandle( 37 ) );
        auto shared = parameters;
        auto layout = parameters.GetLayout();
        auto storage = parameters.GetValueStorageIdentity();
        auto revision = parameters.GetRevision();
        const auto *borrowed = &parameters.GetParameterMemory();
        auto before = material->getRenderState();
        material->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
        material->setShaderParameters( schema.getParameterInfo() );
        require( parameters.GetLayout() == layout && parameters.GetRevision() == revision &&
                 parameters.GetValueStorageIdentity() == storage && shared.GetValueStorageIdentity() == storage &&
                 &parameters.GetParameterMemory() == borrowed && material->getRenderState() == before,
                 "unchanged registered schema rebind preserves shared payload, borrowed bytes, revision and snapshot" );
        require( parameters.Set( value, 0.75f ) && shared.GetScalar( value.handle ) == 0.375f &&
                 parameters.GetValueStorageIdentity() != shared.GetValueStorageIdentity(),
                 "retained handles and physical observer remain usable after schema reuse" );

        // A new registry identity must migrate even if every field is identical.
        MaterialShader replacement( schema.getShaderID() );
        for( const auto &info : schema.getParameterInfo() ) replacement.addParameter( info.m_type, info.m_name );
        MaterialShaderRegistry::instance().registerShader( replacement );
        material->setShaderParameters( replacement.getParameterInfo(), replacement.getShaderID() );
        require( parameters.GetLayout() != layout && parameters.GetLayout() == replacement.getLayout() &&
                 !parameters.Set( value, 1.0f ) && parameters.GetScalar( parameters.GetParameter( 0 ) ) == 0.75f &&
                 parameters.GetInt( parameters.GetParameter( 1 ) ) == -17 &&
                 parameters.GetColour( parameters.GetParameter( 2 ) ) == ColourF( 0.1f, 0.2f, 0.3f, 0.4f ) &&
                 parameters.GetResource( parameters.GetParameter( 3 ) ) == 37,
                 "identical-field registry generation replaces layout, invalidates handles and migrates all value types" );

        Array<MaterialShaderParameterInfo> custom;
        custom.emplace_back( MaterialShaderParameterType::Scalar, "Local" );
        material->setShaderParameters( custom, "UnregisteredSchemaRebind" );
        parameters.SetScalar( parameters.GetParameter( 0 ), 0.65f );
        layout = parameters.GetLayout(); storage = parameters.GetValueStorageIdentity(); revision = parameters.GetRevision();
        material->setShaderParameters( custom );
        require( parameters.GetLayout() == layout && parameters.GetValueStorageIdentity() == storage &&
                 parameters.GetRevision() == revision, "identical unregistered schema reuses its finalized layout and values" );
        auto invalid = custom; invalid.push_back( custom[0] );
        material->setShaderParameters( invalid, "RejectedRebind" );
        require( parameters.GetLayout() == layout && parameters.GetValueStorageIdentity() == storage &&
                 parameters.GetRevision() == revision && material->getShaderID() == "UnregisteredSchemaRebind",
                 "invalid rebind leaves the prior shader, payload and revision intact" );
        material->setShaderParameters( Array<MaterialShaderParameterInfo>{} );
        layout = parameters.GetLayout(); revision = parameters.GetRevision();
        material->setShaderParameters( Array<MaterialShaderParameterInfo>{} );
        require( !parameters.IsValid() && parameters.GetLayout() == layout && parameters.GetRevision() == revision &&
                 !parameters.GetValueStorageIdentity(), "empty custom schemas can be rebound without creating a new identity" );
        material->unload( nullptr );

        // Full property bags must not defeat copy-on-write sharing on a no-op.
        auto pbr = observedSchema();
        material->setShaderParameters( pbr.getParameterInfo(), pbr.getShaderID() ); material->load( nullptr );
        material->setDiffuse( ColourF( 0.3f, 0.4f, 0.5f, 0.8f ) ); material->setOpacity( 0.6f );
        material->setMetalness( 0.2f ); material->setRoughness( 0.7f );
        auto native = material->getNativeMaterial();
        require( wp_graphics_material_has_flag( native, WORKPHONE_MATERIAL_FLAG_TRANSPARENT ) &&
                 !wp_graphics_material_has_flag( native, WORKPHONE_MATERIAL_FLAG_CUTOUT ),
                 "native publication reads pass transparency rather than inherited legacy flag bits" );
        material->setRenderMode( 1u ); native = material->getNativeMaterial();
        require( !wp_graphics_material_has_flag( native, WORKPHONE_MATERIAL_FLAG_TRANSPARENT ) &&
                 wp_graphics_material_has_flag( native, WORKPHONE_MATERIAL_FLAG_CUTOUT ),
                 "native publication maps cutout independently from transparency" );
        material->setRenderMode( 3u );
        auto properties = material->getProperties();
        shared = parameters; storage = parameters.GetValueStorageIdentity(); revision = parameters.GetRevision();
        before = material->getRenderState();
        material->setProperties( properties );
        require( parameters.GetValueStorageIdentity() == storage && shared.GetValueStorageIdentity() == storage &&
                 parameters.GetRevision() == revision && material->getRenderState() == before,
                 "unchanged full property restore retains payload sharing and render version" );
        material->unload( nullptr ); fixture.graphics->pending.clear();
    }

    void runNamedPbrOwnership( Fixture &fixture )
    {
        using namespace render;
        MaterialShaderRegistry::instance().registerBuiltinShaders();
        auto material = make_ptr<ClawMaterial>(); material->load( nullptr );
        const ColourF colour( 0.2f, 0.4f, 0.6f, 1 ), overrideColour( 0.9f, 0.1f, 0.2f, 0.7f );
        material->setDiffuse( colour ); material->setMetalness( 0.2f ); material->setRoughness( 0.6f );
        auto properties = make_ptr<Properties>();
        properties->setProperty( Material::shaderIDStr, Material::defaultPBRShaderIDStr );
        properties->setProperty( "ShaderParam.Roughness", 0.7f );
        material->setProperties( properties );
        auto &parameters = material->getShaderParameters();
        const auto metal = parameters.Resolve<f32>( "Metalness" ), rough = parameters.Resolve<f32>( "Roughness" );
        const auto opacity = parameters.Resolve<f32>( "Opacity" ), clip = parameters.Resolve<f32>( "AlphaClip" );
        const auto albedo = parameters.Resolve<ColourF>( "Albedo" );
        require( parameters.GetScalar( metal.handle ) == 0.2f && parameters.GetScalar( rough.handle ) == 0.7f &&
                 parameters.GetColour( albedo.handle ) == colour,
                 "first DefaultPBR property binding still initializes unspecified values from legacy surface" );
        auto pass = material->getTechnique( 0 )->getPass( 0 );
        pass->setMetalness( 0.9f ); pass->setDiffuse( overrideColour );
        auto oldValues = parameters;
        properties->setProperty( "ShaderParam.Roughness", 0.5f );
        material->setProperties( properties );
        require( parameters.GetScalar( metal.handle ) == 0.2f && parameters.GetColour( albedo.handle ) == colour &&
                 parameters.GetScalar( rough.handle ) == 0.5f && oldValues.GetScalar( rough.handle ) == 0.7f &&
                 material->getMetalness() == 0.9f && material->getDiffuse() == overrideColour,
                 "partial DefaultPBR restore changes only supplied authored values and preserves unrelated pass overrides" );
        oldValues = parameters;
        const auto storage = parameters.GetValueStorageIdentity();
        const auto revision = parameters.GetRevision();
        auto before = material->getRenderState();
        material->setProperties( properties );
        require( parameters.GetValueStorageIdentity() == storage && oldValues.GetValueStorageIdentity() == storage &&
                 parameters.GetRevision() == revision && material->getRenderState() == before,
                 "no-op DefaultPBR restore does not re-import overrides or detach authored values" );
        const auto names = parameters.GetNameLookupCount();
        material->batch( [&]( Material &m ) {
            m.setEditorFloat( "metalness", 0.4f ); m.setEditorFloat( "roughness", 0.6f );
            m.setEditorFloat( "alphaClip", 0.2f ); m.setEditorFloat( "opacity", 0.8f );
        } );
        require( parameters.GetScalar( metal.handle ) == 0.4f && parameters.GetScalar( rough.handle ) == 0.6f &&
                 parameters.GetScalar( clip.handle ) == 0.2f && parameters.GetScalar( opacity.handle ) == 0.8f &&
                 material->getRenderState()->revision == before->revision + 1 &&
                 material->getDiffuse() == ColourF( overrideColour.r, overrideColour.g, overrideColour.b, 0.8f ) &&
                 parameters.GetNameLookupCount() == names, "named PBR scalar adapters share canonical values and publish one batch" );
        material->setEditorFloat( "opacity", -1.0f );
        require( parameters.GetScalar( opacity.handle ) == 0.0f && material->getDiffuse().a == 0.0f,
                 "named canonical opacity uses typed setter normalization" );
        material->setShaderID( String() ); material->setEditorFloat( "opacity", -0.25f );
        require( material->getEditorFloat( "opacity", 1 ) == -0.25f && parameters.GetScalar( opacity.handle ) == 0.0f,
                 "legacy named settings retain their existing raw-value contract" );
        MaterialShader wrongType( "NamedPbrWrongType" );
        wrongType.addParameter( MaterialShaderParameterType::Int, "Metalness" );
        material->setShaderParameters( wrongType.getParameterInfo(), wrongType.getShaderID() );
        const auto custom = parameters.Resolve<s32>( "Metalness" ); parameters.Set( custom, s32( 23 ) );
        material->setEditorFloat( "metalness", 0.75f );
        require( parameters.GetInt( custom.handle ) == 23 && material->getMetalness() == 0.75f,
                 "wrong-type named semantics keep pass fallback without corrupting custom shader values" );
        wrongType.addParameter( MaterialShaderParameterType::Colour, "Albedo" )
                 .addParameter( MaterialShaderParameterType::Int, "Opacity" );
        material->setShaderParameters( wrongType.getParameterInfo(), wrongType.getShaderID() );
        const auto customOpacity = parameters.GetBuiltin( MaterialBuiltinParameter::Opacity );
        parameters.SetScalar( customOpacity, s32( 17 ) );
        material->setDiffuse( colour ); pass->setDiffuse( overrideColour );
        material->setEditorFloat( "opacity", 0.3f );
        require( parameters.GetInt( customOpacity ) == 17 &&
                 parameters.GetColour( parameters.GetBuiltin( MaterialBuiltinParameter::Albedo ) ).a == 0.3f &&
                 material->getDiffuse() == ColourF( overrideColour.r, overrideColour.g, overrideColour.b, 0.3f ),
                 "named Albedo-owned opacity preserves pass RGB and wrong-type custom opacity" );
        material->unload( nullptr ); fixture.graphics->pending.clear();
    }

    void runCanonicalSurfaceColours( Fixture &fixture )
    {
        using namespace render;
        auto schema = observedSchema();
        auto material = make_ptr<ClawMaterial>();
        material->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
        auto &parameters = material->getShaderParameters();
        const auto albedo = parameters.Resolve<ColourF>( "Albedo" );
        const auto emission = parameters.Resolve<ColourF>( "Emissive" );
        const auto opacity = parameters.Resolve<f32>( "Opacity" );
        const auto names = parameters.GetNameLookupCount();
        const ColourF initial( 0.2f, 0.4f, 0.6f, 0.6f ), glow( 0.1f, 0.3f, 0.5f, 1 );
        material->setDiffuse( initial ); material->setEmissive( glow ); material->setOpacity( 0.35f );
        require( parameters.GetColour( albedo.handle ) == initial &&
                 parameters.GetColour( emission.handle ) == glow && parameters.GetScalar( opacity.handle ) == 0.35f,
                 "pre-load colour and opacity setters edit the canonical PBR values" );
        material->load( nullptr );
        auto technique = material->getTechnique( 0 );
        auto first = technique->getPass( 0 );
        auto second = technique->createPass(); second->load( nullptr );
        const auto stateFor = []( SmartPtr<IMaterialPass> pass ) {
            return static_pointer_cast<MaterialPassStateData>(
                pass->getStateContext()->getStateById( pass->getId() )->getData() );
        };
        auto firstData = stateFor( first ), secondData = stateFor( second );
        const ColourF initialRendered( initial.r, initial.g, initial.b, 0.35f );
        require( material->getDiffuse() == initialRendered && second->getDiffuse() == initialRendered &&
                 second->getEmissive() == glow && secondData->getSurfaceState().opacity == 0.35f && readUIntSetting( *secondData, MaterialUIntSetting::renderMode ) == 3u &&
                 secondData->getRenderState()->getFlag( transparentFlag ),
                 "new passes seed canonical colour, opacity and transparent render state" );

        const ColourF red( 1, 0, 0, 0.7f ), blue( 0, 0, 1, 0.8f );
        first->setDiffuse( red ); second->setDiffuse( blue );
        first->setEmissive( red ); second->setEmissive( blue );
        {
            auto edit = second->getStateContext()->invalidateStateDataById<MaterialPassStateData>( second->getId() );
            edit->setUIntSetting( MaterialUIntSetting::renderMode, 1u );
            edit->setUIntSetting( MaterialUIntSetting::blendMode, 0u );
            edit->setBoolSetting( MaterialBoolSetting::cutout, true );
        }
        auto before = firstData->getRenderState(), secondBefore = secondData->getRenderState();
        material->setOpacity( 0.45f );
        require( first->getDiffuse() == ColourF( 1, 0, 0, 0.45f ) &&
                 second->getDiffuse() == ColourF( 0, 0, 1, 0.45f ) &&
                 first->getEmissive() == red && second->getEmissive() == blue &&
                 firstData->getRenderState()->revision == before->revision + 1 &&
                 secondData->getRenderState()->revision == secondBefore->revision + 1 &&
                 readUIntSetting( *secondData, MaterialUIntSetting::renderMode ) == 3u && secondData->getSurfaceState().blendMode == 1u && !second->isCutout(),
                 "opacity-only edits preserve each pass RGB/emission and normalize each pass render mode" );
        before = firstData->getRenderState();
        auto shared = parameters;
        auto storage = parameters.GetValueStorageIdentity();
        const auto revision = parameters.GetRevision();
        material->setOpacity( 0.45f );
        require( firstData->getRenderState() == before && parameters.GetRevision() == revision &&
                 parameters.GetValueStorageIdentity() == storage && shared.GetValueStorageIdentity() == storage,
                 "opacity no-op retains shared payload, revision and render snapshot" );
        secondBefore = secondData->getRenderState();
        material->setDiffuse( initial );
        require( material->getDiffuse() == initial && second->getDiffuse() == initial &&
                 material->getOpacity() == initial.a && parameters.GetScalar( opacity.handle ) == initial.a &&
                 firstData->getRenderState()->revision == before->revision + 1 &&
                 secondData->getRenderState()->revision == secondBefore->revision + 1 && second->getEmissive() == blue,
                 "RGBA material assignment restores colour overrides and publishes colour/opacity together" );
        material->setEmissive( glow );
        require( first->getEmissive() == glow && second->getEmissive() == glow && first->getDiffuse() == initial,
                 "same-authored-value emission assignment restores only emission overrides" );

        before = firstData->getRenderState(); secondBefore = secondData->getRenderState();
        const ColourF finalColour( 0.7f, 0.3f, 0.2f, 0.7f ), finalGlow( 0.9f, 0.4f, 0.1f, 1 );
        material->batch( [&]( Material &m ) {
            parameters.Set( albedo, red ); m.setDiffuse( finalColour );
            parameters.Set( opacity, 0.2f ); m.setOpacity( 0.4f );
            parameters.Set( emission, blue ); m.setEmissive( finalGlow );
            require( m.getRenderState() == before, "mixed colour edits retain the prior publication until close" );
        } );
        const ColourF finalRendered( finalColour.r, finalColour.g, finalColour.b, 0.4f );
        require( material->getDiffuse() == finalRendered && second->getDiffuse() == finalRendered &&
                 material->getEmissive() == finalGlow && parameters.GetColour( albedo.handle ) == finalColour &&
                 parameters.GetScalar( opacity.handle ) == 0.4f &&
                 firstData->getRenderState()->revision == before->revision + 1 &&
                 secondData->getRenderState()->revision == secondBefore->revision + 1,
                 "mixed colour/opacity writes follow call order and publish once per pass" );
        material->applyShaderParametersToRenderer();
        require( material->getDiffuse() == finalRendered && material->getEmissive() == finalGlow,
                 "full reapply retains canonical material colour and opacity edits" );
        auto properties = material->getProperties();
        auto restored = make_ptr<ClawMaterial>(); restored->load( nullptr ); restored->setProperties( properties );
        require( restored->getDiffuse() == finalRendered && restored->getEmissive() == finalGlow &&
                 restored->getOpacity() == 0.4f && restored->getShaderParameters().GetColour(
                     restored->getShaderParameters().GetBuiltin( MaterialBuiltinParameter::Albedo ) ) == finalColour,
                 "shader property round-trip preserves authored RGBA as well as normalized render values" );
        first->setDiffuse( blue ); first->setEmissive( red );
        first->setMetalness( 0.9f ); first->setRoughness( 0.1f );
        auto overrides = material->getProperties();
        restored->setProperties( overrides );
        const auto &restoredParameters = restored->getShaderParameters();
        require( restored->getDiffuse() == blue && restored->getEmissive() == red &&
                 restored->getMetalness() == 0.9f && restored->getRoughness() == 0.1f &&
                 restoredParameters.GetColour( restoredParameters.GetBuiltin( MaterialBuiltinParameter::Albedo ) ) == finalColour &&
                 restoredParameters.GetColour( restoredParameters.GetBuiltin( MaterialBuiltinParameter::Emissive ) ) == finalGlow &&
                 restoredParameters.GetScalar( restoredParameters.GetBuiltin( MaterialBuiltinParameter::Metalness ) ) == 0.0f &&
                 restoredParameters.GetScalar( restoredParameters.GetBuiltin( MaterialBuiltinParameter::Roughness ) ) == 0.0f,
                 "saved legacy aliases restore pass overrides without overwriting canonical shader values" );
        restored->applyShaderParametersToRenderer();
        require( restored->getDiffuse() == finalRendered && restored->getEmissive() == finalGlow &&
                 restored->getMetalness() == 0.0f && restored->getRoughness() == 0.0f,
                 "full reapply after import restores the original authored values" );
        auto partial = make_ptr<Properties>();
        partial->setProperty( "diffuseColour", initial );
        partial->setProperty( "emissiveColour", glow );
        partial->setProperty( "metalness", 0.6f );
        partial->setProperty( "roughness", 0.4f );
        restored->setProperties( partial ); restored->applyShaderParametersToRenderer();
        require( restored->getDiffuse() == initial && restored->getEmissive() == glow &&
                 restored->getMetalness() == 0.6f && restored->getRoughness() == 0.4f &&
                 restoredParameters.GetColour( restoredParameters.GetBuiltin( MaterialBuiltinParameter::Albedo ) ) == initial,
                 "legacy-only partial property bags remain canonical material edits" );
        restored->unload( nullptr );
        require( parameters.GetNameLookupCount() == names, "canonical colour/opacity edits use cached numeric semantics" );
        {
            auto edit = parameters.Edit();
            material->setDiffuse( initial ); material->setOpacity( 0.25f ); material->setEmissive( glow );
            auto added = technique->createPass(); added->load( nullptr );
            require( added->getDiffuse() == ColourF( initial.r, initial.g, initial.b, 0.25f ) &&
                     added->getEmissive() == glow && stateFor( added )->getSurfaceState().opacity == 0.25f,
                     "new passes read pending canonical colours while parameter publication is deferred" );
        }
        material->unload( nullptr ); material->load( nullptr );
        require( material->getDiffuse() == ColourF( initial.r, initial.g, initial.b, 0.25f ) &&
                 material->getEmissive() == glow, "canonical colour and opacity survive graph reload" );
        material->unload( nullptr );

        MaterialShader sparse( "SparseColourPBR" );
        sparse.addParameter( MaterialShaderParameterType::Colour, "Albedo" )
              .addParameter( MaterialShaderParameterType::Int, "Opacity" )
              .addParameter( MaterialShaderParameterType::Int, "Emissive" );
        material->setShaderParameters( sparse.getParameterInfo(), sparse.getShaderID() );
        const auto sparseAlbedo = parameters.Resolve<ColourF>( "Albedo" );
        const auto customOpacity = parameters.Resolve<s32>( "Opacity" );
        const auto customEmission = parameters.Resolve<s32>( "Emissive" );
        parameters.Set( customOpacity, s32( 17 ) ); parameters.Set( customEmission, s32( 23 ) );
        material->setDiffuse( initial ); material->load( nullptr );
        first = material->getTechnique( 0 )->getPass( 0 ); first->setDiffuse( blue );
        material->setOpacity( 0.3f ); material->setEmissive( glow );
        require( first->getDiffuse() == ColourF( 0, 0, 1, 0.3f ) && material->getOpacity() == 0.3f &&
                 parameters.GetColour( sparseAlbedo.handle ) == ColourF( initial.r, initial.g, initial.b, 0.3f ) &&
                 parameters.GetInt( customOpacity.handle ) == 17 && parameters.GetInt( customEmission.handle ) == 23 &&
                 first->getEmissive() == glow, "Albedo-only alpha edits preserve RGB overrides and wrong-type custom slots" );
        material->batch( [&]( Material &m ) {
            m.setDiffuse( initial ); m.setOpacity( 0.2f );
        } );
        require( material->getDiffuse() == ColourF( initial.r, initial.g, initial.b, 0.2f ),
                 "Albedo-only opacity edits preserve pending authored RGB in mixed batches" );
        material->setOpacity( 4.0f );
        require( material->getOpacity() == 1.0f && material->getDiffuse().a == 1.0f,
                 "Albedo-only opacity setters retain clamping" );
        material->unload( nullptr );
        material->setShaderParameters( schema.getParameterInfo(), Material::unlitShaderIDStr );
        material->load( nullptr );
        const auto unlitRevision = parameters.GetRevision();
        material->setDiffuse( initial ); material->setEmissive( glow ); material->setOpacity( 0.5f );
        require( parameters.GetRevision() > unlitRevision && material->getDiffuse().r == initial.r &&
                 material->getDiffuse().a == 0.5f && material->getEmissive() == glow &&
                 parameters.GetColour( parameters.GetBuiltin( MaterialBuiltinParameter::Emissive ) ) == glow &&
                 parameters.GetScalar( parameters.GetBuiltin( MaterialBuiltinParameter::Opacity ) ) == 0.5f,
                 "Unlit emission and opacity share shader ownership while diffuse RGB remains independent" );
        material->unload( nullptr ); fixture.graphics->pending.clear();
    }

    void runCanonicalSurfaceScalars( Fixture &fixture )
    {
        using namespace render;
        auto schema = observedSchema();
        auto material = make_ptr<ClawMaterial>();
        material->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
        auto &parameters = material->getShaderParameters();
        const auto metal = parameters.Resolve<f32>( "Metalness" );
        const auto rough = parameters.Resolve<f32>( "Roughness" );
        const auto clip = parameters.Resolve<f32>( "AlphaClip" );
        const auto custom = parameters.Resolve<s32>( "Custom" );
        const auto names = parameters.GetNameLookupCount();
        material->setMetalness( 0.2f ); material->setRoughness( 0.8f ); material->setAlphaClip( 0.3f );
        require( parameters.GetScalar( metal.handle ) == 0.2f &&
                 parameters.GetScalar( rough.handle ) == 0.8f && parameters.GetScalar( clip.handle ) == 0.3f,
                 "pre-load material setters edit the canonical authored scalar values" );
        material->load( nullptr );
        auto technique = material->getTechnique( 0 );
        auto firstPass = technique->getPass( 0 );
        auto secondPass = technique->createPass(); secondPass->load( nullptr );
        auto secondData = static_pointer_cast<MaterialPassStateData>(
            secondPass->getStateContext()->getStateById( secondPass->getId() )->getData() );
        require( material->getRenderState()->metalness == 0.2f && secondData->getSurfaceState().roughness == 0.8f &&
                 secondData->getSurfaceState().alphaClip == 0.3f, "loaded and newly added passes seed canonical PBR scalars" );
        firstPass->setMetalness( 0.9f ); secondPass->setMetalness( 0.7f );
        secondPass->setMaterial( material ); secondPass->load( nullptr );
        material->setRoughness( 0.6f );
        require( material->getMetalness() == 0.9f && secondData->getSurfaceState().metalness == 0.7f &&
                 parameters.GetScalar( metal.handle ) == 0.2f,
                 "direct pass overrides remain local and unrelated canonical edits preserve them" );
        material->setMetalness( 0.2f );
        require( material->getMetalness() == 0.2f && secondData->getSurfaceState().metalness == 0.2f,
                 "same-authored-value material assignment restores all pass overrides" );
        auto before = material->getRenderState();
        auto secondBefore = secondData->getRenderState();
        const auto storage = parameters.GetValueStorageIdentity();
        const auto revision = parameters.GetRevision();
        material->setMetalness( 0.2f );
        require( material->getRenderState() == before && parameters.GetRevision() == revision &&
                 parameters.GetValueStorageIdentity() == storage, "canonical no-op reuses payload and render version" );
        material->batch( [&]( Material &m ) {
            parameters.Set( metal, 0.4f ); m.setMetalness( 0.5f );
            m.setRoughness( 0.7f ); parameters.Set( rough, 0.25f ); m.setAlphaClip( 0.1f );
            parameters.Set( custom, s32( 42 ) );
            require( m.getRenderState() == before, "mixed canonical edits retain the published version until close" );
        } );
        require( parameters.GetScalar( metal.handle ) == 0.5f && parameters.GetScalar( rough.handle ) == 0.25f &&
                 material->getMetalness() == 0.5f && material->getRoughness() == 0.25f &&
                 material->getAlphaClip() == 0.1f && parameters.GetInt( custom.handle ) == 42 &&
                 material->getRenderState()->revision == before->revision + 1 &&
                 secondData->getRenderState()->revision == secondBefore->revision + 1,
                 "material and parameter writes share last-write order and publish once per pass" );
        material->applyShaderParametersToRenderer();
        require( material->getMetalness() == 0.5f && material->getRoughness() == 0.25f &&
                 material->getAlphaClip() == 0.1f, "full reapply cannot restore stale material-level scalars" );
        auto properties = material->getProperties();
        f32 saved = 0;
        require( properties->getPropertyValue( "ShaderParam.Metalness", saved ) && saved == 0.5f,
                 "canonical material edits persist through the existing shader property schema" );
        require( parameters.GetNameLookupCount() == names, "canonical scalar setters use cached numeric semantics" );
        auto restored = make_ptr<ClawMaterial>(); restored->load( nullptr ); restored->setProperties( properties );
        require( restored->getMetalness() == 0.5f && restored->getRoughness() == 0.25f &&
                 restored->getAlphaClip() == 0.1f, "canonical scalar edits survive property round-trip" );
        restored->unload( nullptr );
        {
            auto edit = parameters.Edit();
            material->setAlphaClip( 0.45f ); material->setMetalness( 0.15f );
            auto added = technique->createPass(); added->load( nullptr );
            auto addedData = added->getStateContext()->getStateDataById<MaterialPassStateData>( added->getId() );
            require( addedData->getSurfaceState().alphaClip == 0.45f && addedData->getSurfaceState().metalness == 0.15f,
                     "new passes read canonical values while publication is deferred" );
        }
        material->unload( nullptr );

        // Matching names alone do not make a shader slot a compatible surface value.
        MaterialShader mismatched( "ScalarFallbackPBR" );
        mismatched.addParameter( MaterialShaderParameterType::Int, "Metalness" );
        material->setShaderParameters( mismatched.getParameterInfo(), mismatched.getShaderID() );
        material->load( nullptr ); material->setMetalness( 0.65f ); material->setRoughness( 0.35f );
        require( material->getMetalness() == 0.65f && material->getRoughness() == 0.35f &&
                 material->getShaderParameters().GetInt( material->getShaderParameters().GetBuiltin(
                     MaterialBuiltinParameter::Metalness ) ) == 0,
                 "wrong-type and absent shader slots keep legacy setters without corrupting custom values" );
        material->setShaderParameters( schema.getParameterInfo(), Material::unlitShaderIDStr );
        auto &unlit = material->getShaderParameters();
        const auto unlitMetal = unlit.GetBuiltin( MaterialBuiltinParameter::Metalness );
        const auto unlitValue = unlit.GetScalar( unlitMetal );
        material->setMetalness( 0.85f );
        require( material->getMetalness() == 0.85f && unlit.GetScalar( unlitMetal ) == unlitValue,
                 "inactive PBR semantics in unlit layouts keep their independent custom values" );
        material->unload( nullptr ); fixture.graphics->pending.clear();
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
        material->getTechnique( 0 )->getPass( 0 )->setMetalness( 0.9f );
        secondPass->setMetalness( 0.9f );
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
        require(detached.GetValueStorageIdentity() == parameters.GetValueStorageIdentity(),
                "copying observed shader parameters shares values without copying their observer");
        detached.Set( rough, 0.8f );
        require( material->getRenderState() == transparent, "copies never inherit a material observer" );
        parameters = detached;
        require(parameters.GetValueStorageIdentity() == detached.GetValueStorageIdentity(),
                "assignment shares the authored payload while retaining physical owner identity");
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

        // Legacy compound setters publish one snapshot per call batch: setOpacity
        // used to publish once per inner write (opacity float, diffuse alpha, plus
        // render-mode/blend adjustments at the transparency threshold).
        other->setRenderMode( 3u );
        other->setDiffuse( ColourF( 1, 1, 1, 0.4f ) );
        auto batchBefore = other->getRenderState();
        other->setOpacity( 0.45f );
        auto batchAfter = other->getRenderState();
        require( batchAfter->revision == batchBefore->revision + 1 &&
                 batchAfter->opacity == 0.45f && batchAfter->diffuseColour.a == 0.45f,
                 "compound legacy setter publishes one snapshot per pass" );
        other->batch( []( Material &m ) {
            m.setDiffuse( ColourF( 0.11f, 0.22f, 0.33f, 1 ) );
            m.setMetalness( 0.61f ); m.setRoughness( 0.39f );
        } );
        auto batchGrouped = other->getRenderState();
        require( batchGrouped->revision == batchAfter->revision + 1 &&
                 batchGrouped->metalness == 0.61f && batchGrouped->roughness == 0.39f &&
                 batchGrouped->diffuseColour == ColourF( 0.11f, 0.22f, 0.33f, 1 ) &&
                 batchAfter->diffuseColour != batchGrouped->diffuseColour,
                 "explicit batch coalesces independent legacy setters into one publication" );
        other->batch( []( Material &m ) {
            m.setMetalness( 0.61f ); m.setRoughness( 0.39f );
        } );
        require( other->getRenderState() == batchGrouped,
                 "a batch with no net change reuses the current snapshot" );

        const auto mixedOpacity = otherParameters.Resolve<f32>( "Opacity" );
        other->batch( [&]( Material &m ) {
            m.setDiffuse( ColourF( 0.7f, 0.3f, 0.2f, 1 ) );
            m.batch( [&]( Material &nested ) {
                nested.setUVOffset( Vector2F( 0.1f, 0.2f ) );
                otherParameters.Set( mixedOpacity, 0.6f );
            } );
            require( m.getRenderState() == batchGrouped, "nested mixed edits retain the published version until exit" );
        } );
        auto mixed = other->getRenderState();
        require( mixed->revision == batchGrouped->revision + 1 &&
                 mixed->diffuseColour == ColourF( 0.7f, 0.3f, 0.2f, 0.6f ) && mixed->opacity == 0.6f &&
                 mixed->uvOffsetX == 0.1f && mixed->uvOffsetY == 0.2f,
                 "typed opacity preserves a legacy colour changed inside the same batch" );

        bool threw = false;
        try
        {
            other->batch( [&]( Material &m ) {
                m.setMetalness( 0.23f );
                otherParameters.Set( nextRough, 0.77f );
                throw std::runtime_error( "expected batch callback failure" );
            } );
        }
        catch( const std::runtime_error & ) { threw = true; }
        require( threw && other->getRenderState()->revision == mixed->revision + 1 &&
                 other->getRenderState()->metalness == 0.23f && other->getRenderState()->roughness == 0.77f,
                 "exception unwinding publishes completed writes and propagates the callback failure" );
        auto afterFailure = other->getRenderState();
        other->batch( []( Material &m ) { m.setRoughness( 0.41f ); } );
        require( other->getRenderState()->revision == afterFailure->revision + 1 &&
                 other->getRenderState()->roughness == 0.41f, "batch scopes are usable after a callback throws" );

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
        auto stringsPass = make_ptr<render::ClawMaterialPass>();
        stringsPass->load(nullptr);
        stringsPass->setVertexShaderName("PackedVertex");
        stringsPass->setFragmentShaderName("PackedFragment");
        stringsPass->setGeometryShaderName("PackedGeometry");
        {
            auto data = stringsPass->getStateContext()->invalidateStateDataById<MaterialPassStateData>(stringsPass->getId());
            data->setEditorString("shaderPath", "Shaders/Packed");
            data->setEditorString("keywords", "FOG SHADOWS");
        }
        require(stringsPass->getVertexShaderName() == "PackedVertex", "native packed vertex adapter writes source state");
        auto stringProperties = stringsPass->getProperties();
        String exportedVertex;
        require(stringProperties->getPropertyValue(render::MaterialPass::vertexShaderStr, exportedVertex) &&
            exportedVertex == "PackedVertex", "native packed vertex property export");
        auto restoredStringsPass = make_ptr<render::ClawMaterialPass>();
        restoredStringsPass->load(nullptr);
        auto stringContext = restoredStringsPass->getStateContext();
        auto stringState = stringContext->getStateById(restoredStringsPass->getId());
        restoredStringsPass->setProperties(stringProperties);
        require(restoredStringsPass->getStateContext() == stringContext &&
            stringContext->getStateById(restoredStringsPass->getId()) == stringState,
            "property import retains the canonical pass context and record");
        {
            auto data = restoredStringsPass->getStateContext()->getStateDataById<MaterialPassStateData>(restoredStringsPass->getId());
            String text;
            require(restoredStringsPass->getVertexShaderName() == "PackedVertex", "native packed vertex name round trip");
            require(restoredStringsPass->getFragmentShaderName() == "PackedFragment", "native packed fragment name round trip");
            require(restoredStringsPass->getGeometryShaderName() == "PackedGeometry", "native packed geometry name round trip");
            require(data->getEditorString("shaderPath", text) && text == "Shaders/Packed", "native packed shader path round trip");
            require(data->getEditorString("keywords", text) && text == "FOG SHADOWS", "native packed keywords round trip");
        }
        restoredStringsPass->unload(nullptr);
        stringsPass->unload(nullptr);
        auto pass = material->getTechnique( 0 )->getPass( 0 );
        pass->setMetalness( 0.6f );
        require( material->getRenderState()->metalness == 0.6f, "direct pass edit is visible" );
        {
            auto data = pass->getStateContext()->invalidateStateDataById<MaterialPassStateData>( pass->getId() );
            require( bool( data ), "owned pass data" );
            data->setSurfaceValue( &MaterialRenderState::roughness, 0.8f );
            data->setSurfaceValue( &MaterialRenderState::uvOffsetX, 0.25f );
        }
        auto direct = material->getRenderState();
        require( direct->roughness == 0.8f && direct->uvOffsetX == 0.25f,
                 "existing SafePtr writes publish coherently" );
        {
            auto data = pass->getStateContext()->getStateDataById<MaterialPassStateData>( pass->getId() );
            require( &data->getSurfaceState() == direct.get() && data->getPendingSurfaceStorageBytes() == 0,
                     "direct, typed and native adapters leave one immutable surface owner at rest" );
        }
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
        {
            // Binding identity drives immutable publication; no parallel dirty flags are needed.
            auto bindingSnapshot = [&] {
                return detached->getStateContext()->getStateDataById<MaterialPassStateData>(detached->getId())->getRenderState();
            };
            auto bound = bindingSnapshot();
            detachedUnit->setTexture(texture);
            require(bindingSnapshot() == bound, "same texture binding reuses immutable record");
            auto replacementTexture = make_ptr<render::ClawTexture>();
            detachedUnit->setTexture(replacementTexture);
            auto replacedBinding = bindingSnapshot();
            require(replacedBinding != bound && replacedBinding->revision == bound->revision + 1 &&
                    replacedBinding->textures[0] == replacementTexture && bound->textures[0] == texture,
                    "base texture replacement publishes once and preserves retained binding");
            detachedUnit->setTexture(texture);
        }
        auto concreteUnit = detachedUnit;
        require(concreteUnit->MaterialTexture::getTexture() == texture,
                "native and legacy texture adapters share the canonical resource owner");
        concreteUnit->setScale(Vector3F(2, 3, 4));
        concreteUnit->setTint(ColourF::Red);
        concreteUnit->setTextureType(7);
        require(concreteUnit->MaterialTexture::getTint() == concreteUnit->getTint() &&
                concreteUnit->MaterialTexture::getTextureType() == concreteUnit->getTextureType(),
                "texture adapters share tint and type storage");
        concreteUnit->MaterialTexture::setTexture(nullptr);
        {
            auto data = detached->getStateContext()->getStateDataById<MaterialPassStateData>(detached->getId());
            require(!concreteUnit->getTexture() && !data->getRenderState()->textures[0],
                    "base-qualified texture edits invalidate the same published binding");
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
        if( argc == 2 && std::strcmp( argv[1], "--heap-profile-self-test" ) == 0 )
        {
            material_heap_profile::runSelfTest();
            std::cout << "PASS: CRT heap, aligned allocation and private commitment profiling\n";
            return 0;
        }
        if( argc == 4 && std::strcmp( argv[1], "--profile-full-memory" ) == 0 )
        {
            size_t parsed = 0;
            const auto count = std::stoull( argv[2], &parsed );
            require( parsed == std::strlen( argv[2] ) && count > 0 && count <= 100000,
                     "full profile requires 1..100000 materials" );
            const auto parameters = std::stoul( argv[3], &parsed );
            require( parsed == std::strlen( argv[3] ) &&
                     ( parameters == 8 || parameters == 32 || parameters == 128 ),
                     "full profile requires 8, 32 or 128 mixed parameters" );
            // Separate mode: no preceding contracts may pre-grow material pools.
            const auto bootstrap = material_heap_profile::capture();
            std::optional<FullMaterialMemoryProfileResult> result;
            {
                Fixture fixture;
                result.emplace( runFullMaterialMemoryProfile( fixture, static_cast<size_t>( count ),
                                                              static_cast<u32>( parameters ) ) );
                result->sampleExternal( "fixture_inputs_released", material_heap_profile::capture() );
            }
            const auto retired = material_heap_profile::capture();
            result->report( bootstrap, retired );
            return 0;
        }
        std::cout << "root_material_state_bytes=" << sizeof( MaterialStateData )
                  << " base_state_bytes=" << sizeof( StateData ) << '\n';
        if( argc == 2 && std::string( argv[1] ) == "--root-state-storage" ) return 0;
        runDrawSorting();
        if( argc == 2 && std::string( argv[1] ) == "--benchmark-sorting" )
        {
            benchmarkDrawSorting();
            return 0;
        }
        if( argc == 2 && std::string( argv[1] ) == "--benchmark-preparation" )
        {
            material_cache_benchmark::run();
            return 0;
        }
        material_cache_benchmark::contracts();
        runMaterialPreparation();
        runPersistentRecordTable();
        runIdleRecordMaintenance();
        std::cout << "authoring_pass_bytes=" << sizeof(render::ClawMaterialPass)
                  << " authoring_material_bytes=" << sizeof(render::ClawMaterial)
                  << " authoring_texture_unit_bytes=" << sizeof(render::ClawMaterialTexture)
                  << " draw_record_bytes=" << sizeof(render::ClawMaterialDrawRecords::Record) << '\n';
        generic_engine_catalog_budget_contracts::bootstrap();
        Fixture fixture;
        generic_engine_catalog_budget_contracts::run(fixture);
        material_texture_cold_contracts::run();
        claw_material_residency_contracts::run( fixture );
        claw_material_metadata_budget_contracts::run();
        if( argc == 2 && std::string( argv[1] ) == "--benchmark-owned-storage" )
        {
            runOwnedStorageInventory( fixture, 10000 );
            return 0;
        }
        if( argc == 2 && std::string( argv[1] ) == "--benchmark-schema-rebind" )
        {
            benchmarkSchemaRebind();
            return 0;
        }
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
        if( argc == 2 && std::string( argv[1] ) == "--benchmark-update-publication" )
        {
            benchmarkUpdatePublication();
            return 0;
        }
        if( argc == 2 && std::string( argv[1] ) == "--benchmark-controls" )
        {
            benchmarkNumericControls( fixture );
            return 0;
        }
        if( argc == 2 && std::string( argv[1] ) == "--benchmark-texture-bindings" )
        {
            benchmarkTextureBindings();
            return 0;
        }
        runCubicTexturePublications();
        if( argc == 2 && std::string( argv[1] ) == "--benchmark-cube-publication" )
        {
            benchmarkCubicTexturePublications();
            return 0;
        }
        runTextureNameStorage(fixture);
        runLazyTextureUnits(fixture);
        runCapturedMaterialRecords( fixture );
        runCapturedDrawSorting( fixture );
        runObservedParameters( fixture );
        runCanonicalSurfaceScalars( fixture );
        runCanonicalSurfaceColours( fixture );
        runSchemaRebindContracts( fixture );
        runNamedPbrOwnership( fixture );
        runLegacySurfaceStorage( fixture );
        runMaterialSurfaceHandoff( fixture );
        runUnlitSurfaceOwnership( fixture );
        runColorOnlySurfaceOwnership( fixture );
        runRootMaterialStateStorage( fixture );
        runOwnedStorageInventory( fixture, 64 );
        runNumericControls( fixture );
        runTextureBindings();
        runLegacyBatchScopes( fixture );
        runUpdatePublicationScopes( fixture );
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
