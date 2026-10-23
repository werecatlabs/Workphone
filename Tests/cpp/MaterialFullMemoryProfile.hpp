#ifndef WP_TEST_MATERIAL_FULL_MEMORY_PROFILE_HPP
#define WP_TEST_MATERIAL_FULL_MEMORY_PROFILE_HPP

// Include inside MaterialLoadSequenceTests' anonymous namespace after Fixture.
// MaterialHeapProfile.hpp and the standard/engine headers must be included at
// file scope. The measurement path keeps fixed sample storage and defers output
// until all measured owners have retired.
struct FullMaterialMemoryProfileMetadata
{
    size_t liveMaterials = 0;
    size_t textureFacades = 0;
    size_t valuePayloads = 0;
    size_t currentSnapshots = 0;
    size_t pendingSurfaceBytes = 0;
    size_t retiredSnapshots = 0;
    size_t viewCount = 0;
    size_t recordResidents = 0;
    size_t recordTableBytes = 0;
    size_t graphicsQueueCount = 0;
    size_t graphicsQueueCapacity = 0;
    size_t poolBlocks = 0;
    size_t nominalPoolCapacity = 0;
    double checksum = 0;
};

struct FullMaterialMemoryProfileResult
{
    struct Sample
    {
        const char *phase = nullptr;
        material_heap_profile::Snapshot heap;
        FullMaterialMemoryProfileMetadata owners;
    };
    std::array<Sample, 24> samples{};
    size_t sampleCount = 0;
    size_t materialCount = 0;
    u32 parameterCount = 0;
    std::int64_t fixtureInputHeapBytes = 0;
    std::int64_t fixtureMeshHeapBytes = 0;
    material_heap_profile::Snapshot baseline;
    material_heap_profile::Peaks peaks;

    void sample( const char *phase, const FullMaterialMemoryProfileMetadata &owners = {} )
    {
        require( sampleCount < samples.size(), "full-memory fixed sample buffer has capacity" );
        auto &entry = samples[sampleCount++];
        entry.phase = phase;
        entry.owners = owners;
        entry.heap = material_heap_profile::capture();
        peaks.observe( entry.heap );
    }

    void sampleExternal( const char *phase, const material_heap_profile::Snapshot &heap )
    {
        require( sampleCount < samples.size(), "full-memory external sample buffer has capacity" );
        auto &entry = samples[sampleCount++];
        entry.phase = phase;
        entry.heap = heap;
        peaks.observe( heap );
    }

    // Call only after Fixture destruction. Registry descriptors intentionally
    // have process lifetime and remain visible in the final heap sample.
    void report( const material_heap_profile::Snapshot &bootstrap,
                 const material_heap_profile::Snapshot &afterFixture ) const
    {
#if defined( WP_MATERIAL_MEMORY_PROFILE_BASELINE )
        const char *implementation = "baseline957";
#else
        const char *implementation = "optimized";
#endif
        const auto emit = [&]( const char *phase, const material_heap_profile::Snapshot &heap,
                               const FullMaterialMemoryProfileMetadata &owners ) {
            std::cout << "full_material_memory implementation=" << implementation
                << " phase=" << phase << " materials=" << materialCount << " parameters=" << parameterCount
                << " schema=mixed_scalar_colour_matrix_texture names=ValueN source_prototype_included=1"
                << " fixture_arrays_preallocated=1 fixture_array_heap_busy_bytes=" << fixtureInputHeapBytes
                << " fixture_meshes_added_after_primary=1 fixture_mesh_heap_busy_bytes=" << fixtureMeshHeapBytes
                << " live_materials=" << owners.liveMaterials << " materialized_texture_facades=" << owners.textureFacades
                << " distinct_value_payloads_including_prototype=" << owners.valuePayloads
                << " current_snapshot_count=" << owners.currentSnapshots
#if defined( WP_MATERIAL_MEMORY_PROFILE_BASELINE )
                << " current_snapshot_bytes=0 retired_snapshot_bytes=0"
#else
                << " current_snapshot_bytes=" << owners.currentSnapshots * sizeof( MaterialRenderState )
                << " retired_snapshot_bytes=" << owners.retiredSnapshots * sizeof( MaterialRenderState )
#endif
                << " pending_surface_bytes=" << owners.pendingSurfaceBytes << " retained_view_count=" << owners.viewCount
                << " resident_prepared_records=" << owners.recordResidents << " record_table_payload_bytes=" << owners.recordTableBytes
                << " graphics_queue_count=" << owners.graphicsQueueCount
                << " graphics_queue_capacity=" << owners.graphicsQueueCapacity
                << " observed_factory_pool_blocks=" << owners.poolBlocks
                << " nominal_factory_pool_capacity_elements=" << owners.nominalPoolCapacity
                << " engine_memory_pool_enabled=" << WP_USE_MEMORY_POOL
                << " heap_busy_data_bytes=" << heap.heapBusyDataBytes
                << " heap_busy_overhead_bytes=" << heap.heapBusyOverheadBytes
                << " heap_busy_bytes=" << heap.heapBusyBytes()
                << " heap_busy_delta_from_before_schema_bytes=" << material_heap_profile::delta( heap.heapBusyBytes(), baseline.heapBusyBytes() )
                << " heap_free_data_bytes=" << heap.heapFreeDataBytes
                << " heap_free_overhead_bytes=" << heap.heapFreeOverheadBytes
                << " heap_busy_blocks=" << heap.heapBusyBlocks << " heap_free_blocks=" << heap.heapFreeBlocks
                << " heap_region_count=" << heap.heapRegionCount
                << " private_commit_bytes=" << heap.privateCommitBytes
                << " private_commit_delta_from_before_schema_bytes=" << material_heap_profile::delta( heap.privateCommitBytes, baseline.privateCommitBytes )
                << " vm_private_committed_bytes=" << heap.vmPrivateCommittedBytes
                << " vm_private_reserved_bytes=" << heap.vmPrivateReservedBytes
                << " working_set_bytes=" << heap.workingSetBytes
                << " process_lifetime_peak_private_commit_bytes=" << heap.processLifetimePeakPrivateCommitBytes
                << " process_lifetime_peak_working_set_bytes=" << heap.processLifetimePeakWorkingSetBytes
                << " sampled_heap_busy_peak_bytes=" << peaks.sampledHeapBusyBytes
                << " sampled_heap_busy_peak_delta_bytes=" << material_heap_profile::delta( peaks.sampledHeapBusyBytes, baseline.heapBusyBytes() )
                << " transient_heap_peak_measured=0 gpu_allocations_in_fixture=0 shared_registry_process_lifetime=1"
                << " checksum=" << owners.checksum << '\n';
        };
        emit( "process_bootstrap", bootstrap, {} );
        for( size_t i = 0; i < sampleCount; ++i ) emit( samples[i].phase, samples[i].heap, samples[i].owners );
        emit( "fixture_destroyed_registry_retained", afterFixture, {} );
    }
};

class FullMemoryProfileMesh : public render::ClawMesh
{
public:
    FullMemoryProfileMesh()
    {
        m_mesh = wp_graphics_mesh_create();
        require( m_mesh && wp_graphics_mesh_add_submesh( m_mesh, 0, 3, 0 ) == 0 &&
                 wp_graphics_mesh_add_submesh( m_mesh, 3, 3, 1 ) == 1, "full-memory mesh has two sections" );
        // Allocate the input mesh's two material slots before the gate baseline.
        setMaterial( nullptr, 0 );
        setMaterial( nullptr, 1 );
    }
};

template <class T>
void addFullMemoryProfilePool( const SmartPtr<IFactoryManager> &manager,
                              FullMaterialMemoryProfileMetadata &owners )
{
    if( !manager ) return;
    const auto factory = dynamic_pointer_cast<FactoryTemplate<T>>( manager->getFactoryById( T::typeInfo() ) );
    if( !factory ) return;
    const auto pool = factory->getPool();
    if( !pool ) return;
    const auto blocks = static_cast<size_t>( pool->getSize() );
    owners.poolBlocks += blocks;
    owners.nominalPoolCapacity += blocks * static_cast<size_t>( pool->getNextSize() );
}

FullMaterialMemoryProfileResult runFullMaterialMemoryProfile( Fixture &fixture, size_t count, u32 parameterCount )
{
    using namespace render;
    require( count && ( parameterCount == 8 || parameterCount == 32 || parameterCount == 128 ),
             "full-memory profile uses a nonempty population and 8/32/128 slots" );
    FullMaterialMemoryProfileResult result;
    result.materialCount = count;
    result.parameterCount = parameterCount;
    fixture.graphics->pending.clear();
    result.sample( "fixture_ready_before_inputs" );
    {
        // These pointer/handle arrays are workload inputs. Their empty storage
        // does not construct shared engine objects or warm their object pools.
        std::vector<SmartPtr<ClawMaterial>> materials( count );
        std::vector<SmartPtr<MaterialPassStateData>> states( count );
        std::vector<SmartPtr<IMaterialPass>> passes( count );
        std::vector<MaterialShaderParameterHandle> scalarHandles( count );
        std::vector<std::unique_ptr<FullMemoryProfileMesh>> meshes( count );
        result.sample( "before_schema" );
        result.baseline = result.samples[result.sampleCount - 1].heap;
        result.fixtureInputHeapBytes = material_heap_profile::delta( result.baseline.heapBusyBytes(),
            result.samples.front().heap.heapBusyBytes() );

        MaterialShader schema( "FullMaterialMemoryProfile" );
        for( u32 i = 0; i < parameterCount; ++i )
            schema.addParameter( i % 4 == 0 ? MaterialShaderParameterType::Scalar :
                                 i % 4 == 1 ? MaterialShaderParameterType::Colour :
                                 i % 4 == 2 ? MaterialShaderParameterType::Matrix : MaterialShaderParameterType::Texture,
                                 "Value" + String( std::to_string( i ).c_str() ) );
        auto &registry = MaterialShaderRegistry::instance();
        registry.registerBuiltinShaders();
        registry.registerShader( schema );
        MaterialShaderParametersInstance prototype;
        const auto publishedSchema = registry.find( schema.getShaderID() );
        require( publishedSchema, "full-memory registered schema is available" );
        publishedSchema->buildInstance( prototype );
        const auto sourceHandle = prototype.FindParameter( "Value0" );
        require( sourceHandle.IsValid(), "full-memory source scalar exists" );
        prototype.SetScalar( sourceHandle, 0.75f );
        require( prototype.GetScalar( sourceHandle ) == 0.75f && prototype.GetParameterInfo().size() == parameterCount,
                 "full-memory source descriptor and authored value are valid" );

        size_t baselineMaterializedFacades = 0;
        const auto owners = [&]( size_t live, size_t edited, size_t retired, size_t views ) {
            FullMaterialMemoryProfileMetadata value;
            value.liveMaterials = live;
            value.graphicsQueueCount = fixture.graphics->pending.size();
            value.graphicsQueueCapacity = fixture.graphics->pending.capacity();
            value.retiredSnapshots = retired;
            value.viewCount = views;
#if defined( WP_MATERIAL_MEMORY_PROFILE_BASELINE )
            value.valuePayloads = live + 1; // Old instances eagerly copy every prototype payload.
            value.textureFacades = live ? baselineMaterializedFacades : 0;
#else
            value.valuePayloads = edited + 1; // The source remains alive after every clone detaches.
#endif
            for( size_t i = 0; i < live; ++i )
            {
#if !defined( WP_MATERIAL_MEMORY_PROFILE_BASELINE )
                value.pendingSurfaceBytes += states[i]->getPendingSurfaceStorageBytes();
                value.textureFacades += static_pointer_cast<ClawMaterialPass>( passes[i] )->getMaterializedTextureUnitCount();
                if( states[i]->getRenderState() ) ++value.currentSnapshots;
#else
                value.checksum += materials[i]->getShaderParameters().GetScalar( scalarHandles[i] ) + states[i]->roughness;
#endif
#if !defined( WP_MATERIAL_MEMORY_PROFILE_BASELINE )
                value.checksum += materials[i]->getShaderParameters().GetScalar( scalarHandles[i] ) + states[i]->getSurfaceState().roughness;
#endif
            }
            const auto coreFactories = fixture.app->getFactoryManager();
            const auto graphicsFactories = fixture.graphics->getFactoryManager();
            addFullMemoryProfilePool<StateContext>( coreFactories, value );
            addFullMemoryProfilePool<State>( coreFactories, value );
            addFullMemoryProfilePool<StateQueue>( coreFactories, value );
            addFullMemoryProfilePool<MaterialStateData>( coreFactories, value );
            addFullMemoryProfilePool<MaterialPassStateData>( coreFactories, value );
            addFullMemoryProfilePool<MaterialTexture>( coreFactories, value );
            addFullMemoryProfilePool<ClawMaterialTechnique>( graphicsFactories, value );
            addFullMemoryProfilePool<ClawMaterialPass>( graphicsFactories, value );
            addFullMemoryProfilePool<ClawMaterialTexture>( graphicsFactories, value );
            return value;
        };
        result.sample( "shared_schema_and_authored_source", owners( 0, 0, 0, 0 ) );
        for( size_t i = 0; i < count; ++i )
        {
            auto material = make_ptr<ClawMaterial>();
            material->setShaderParameters( schema.getParameterInfo(), schema.getShaderID() );
            material->getShaderParameters() = prototype;
            material->load( nullptr );
            require( material->getNumTechniques() == 1, "full-memory material has one technique" );
            const auto technique = material->getTechnique( 0 );
            require( technique && technique->getNumPasses() == 1, "full-memory technique has one pass" );
            passes[i] = technique->getPass( 0 );
            // Both revisions must measure fully attached/configured passes.
            // A backend constructor can initialize a pass before its material
            // or technique exists; that early load has no standard slots yet.
            passes[i]->setMaterial( material );
            passes[i]->setParent( technique );
            passes[i]->load( nullptr );
            require( passes[i]->getNumTexturesNodes() == 15,
                     "full-memory configured standard pass exposes fifteen logical texture slots" );
            auto state = passes[i]->getStateContext()->getStateDataById<MaterialPassStateData>( passes[i]->getId() );
            require( state && material->getShaderParameters().GetParameterInfo().size() == parameterCount,
                     "full-memory loaded graph owns the requested state and schema" );
            states[i] = state.get();
#if defined( WP_MATERIAL_MEMORY_PROFILE_BASELINE )
            for( const auto &unit : passes[i]->getTextureUnits() ) if( unit ) ++baselineMaterializedFacades;
#else
            require( static_pointer_cast<ClawMaterialPass>( passes[i] )->getMaterializedTextureUnitCount() == 0,
                     "full-memory loaded current graphs keep empty authoring slots lazy" );
#endif
            scalarHandles[i] = material->getShaderParameters().FindParameter( "Value0" );
            require( scalarHandles[i].IsValid() && material->getShaderParameters().GetScalar( scalarHandles[i] ) == 0.75f,
                     "full-memory prototype clone retains its authored scalar" );
#if !defined( WP_MATERIAL_MEMORY_PROFILE_BASELINE )
            require( material->getShaderParameters().GetLayout() == prototype.GetLayout() &&
                     material->getShaderParameters().GetValueStorageIdentity() == prototype.GetValueStorageIdentity(),
                     "full-memory optimized population shares the actual schema and authored payload" );
#endif
            materials[i] = material;
        }
        // This is the primary whole-material comparison: same loaded graphs,
        // no captured views, and all shared schema/source allocations included.
        result.sample( "loaded_materials_no_views", owners( count, 0, 0, 0 ) );
        fixture.graphics->pending.clear();
        result.sample( "loaded_materials_queue_drained", owners( count, 0, 0, 0 ) );

        // Meshes are added only after the primary material-only sample. Creating
        // them before that sample could pre-fill shared object pools and hide a
        // material population's first allocation cost.
        const auto beforeMeshes = result.samples[result.sampleCount - 1].heap;
        for( size_t i = 0; i < count; ++i )
        {
            meshes[i] = std::make_unique<FullMemoryProfileMesh>();
            meshes[i]->setMaterial( materials[i], 0 ); meshes[i]->setMaterial( materials[i], 1 );
        }
        result.sample( "fixture_mesh_inputs_added", owners( count, 0, 0, 0 ) );
        result.fixtureMeshHeapBytes = material_heap_profile::delta(
            result.samples[result.sampleCount - 1].heap.heapBusyBytes(), beforeMeshes.heapBusyBytes() );

        const auto sparseCount = std::max<size_t>( 1, count / 100 );
#if !defined( WP_MATERIAL_MEMORY_PROFILE_BASELINE )
        {
            ClawMaterialRenderCache cache;
            ClawMaterialDrawRecords oldest, intermediate, latest;
            const auto capture = [&]( ClawMaterialDrawRecords &view ) {
                view.clear();
                for( const auto &mesh : meshes ) view.capture( mesh.get() );
                view.seal( &cache );
                require( view.getRecordCount() == count && view.getBindingCount() == count * 2,
                         "full-memory view deduplicates two sections to one record per material" );
            };
            const auto record = [&]( const char *phase, size_t edited, size_t retired, size_t viewCount ) {
                auto value = owners( count, edited, retired, viewCount );
                value.recordResidents = cache.getResidentCount();
                value.recordTableBytes = cache.getStorageBytes();
                result.sample( phase, value );
            };
            capture( oldest ); capture( intermediate );
            require( cache.getResidentCount() == count && oldest.getPersistentTableIdentity() == intermediate.getPersistentTableIdentity(),
                     "full-memory initial views share a single prepared table" );
            record( "zero_percent_two_views", 0, 0, 2 );
            for( size_t i = 0; i < sparseCount; ++i )
            {
                states[i]->lock(); passes[i]->setRoughness( 0.25f );
                materials[i]->getShaderParameters().SetScalar( scalarHandles[i], 0.25f );
            }
            require( owners( count, sparseCount, 0, 2 ).pendingSurfaceBytes == sparseCount * sizeof( MaterialRenderState ),
                     "full-memory sparse stage owns exactly one candidate per edited pass" );
            record( "one_percent_pending_two_views", sparseCount, 0, 2 );
            for( size_t i = 0; i < sparseCount; ++i ) states[i]->unlock();
            for( size_t i = 0; i < sparseCount; ++i ) materials[i]->getNativeMaterial();
            capture( intermediate );
            require( cache.getResidentCount() == count + sparseCount, "full-memory sparse publication retains old versions" );
            record( "one_percent_published_old_view_retained", sparseCount, sparseCount, 2 );
            for( size_t i = 0; i < count; ++i )
            {
                states[i]->lock(); passes[i]->setRoughness( 0.75f );
                materials[i]->getShaderParameters().SetScalar( scalarHandles[i], 0.5f );
            }
            require( owners( count, count, sparseCount, 2 ).pendingSurfaceBytes == count * sizeof( MaterialRenderState ),
                     "full-memory all-edited stage accounts for every candidate" );
            record( "all_pending_two_old_views", count, sparseCount, 2 );
            for( auto &state : states ) state->unlock();
            for( const auto &material : materials ) material->getNativeMaterial();
            record( "all_published_two_old_views", count, count + sparseCount, 2 );
            capture( latest );
            require( cache.getResidentCount() == count * 2 + sparseCount,
                     "full-memory three views retain all distinct publication generations" );
            record( "all_updated_three_views", count, count + sparseCount, 3 );
            oldest.clear(); cache.collectExpired();
            require( cache.getResidentCount() == count * 2, "full-memory oldest-view retirement preserves other generations" );
            record( "oldest_view_released", count, count, 2 );
            intermediate.clear(); cache.collectExpired();
            require( cache.getResidentCount() == count, "full-memory intermediate retirement releases obsolete snapshots" );
            record( "intermediate_view_released", count, 0, 1 );
            latest.clear(); cache.collectExpired();
            require( cache.getResidentCount() == count, "full-memory current graphs retain prepared records without views" );
            record( "all_views_cleared_graphs_alive", count, 0, 0 );
            for( size_t i = 0; i < count; ++i )
            {
                require( materials[i]->getShaderParameters().GetScalar( scalarHandles[i] ) == 0.5f &&
                         passes[i]->getRoughness() == 0.75f && !states[i]->getPendingSurfaceStorageBytes(),
                         "full-memory final surface/value parity and no pending owners" );
                meshes[i]->setMaterial( nullptr, 0 ); meshes[i]->setMaterial( nullptr, 1 );
                passes[i] = nullptr; states[i] = nullptr;
                materials[i]->unload( nullptr ); materials[i] = nullptr;
            }
            fixture.graphics->pending.clear(); cache.collectExpired();
            require( cache.getResidentCount() == 0, "full-memory graph unload retires every prepared record" );
            auto unloaded = owners( 0, 0, 0, 0 );
            unloaded.recordTableBytes = cache.getStorageBytes();
            result.sample( "graphs_unloaded_cache_capacity_retained", unloaded );
        }
        result.sample( "views_and_record_cache_destroyed", owners( 0, 0, 0, 0 ) );
#else
        // The baseline has no immutable view records. Measure real old storage;
        // do not manufacture snapshots or persistent-table allocations for it.
        for( size_t i = 0; i < sparseCount; ++i )
        {
            passes[i]->setRoughness( 0.25f );
            materials[i]->getShaderParameters().SetScalar( scalarHandles[i], 0.25f );
            materials[i]->getNativeMaterial();
        }
        result.sample( "one_percent_edited_no_views", owners( count, sparseCount, 0, 0 ) );
        for( size_t i = 0; i < count; ++i )
        {
            passes[i]->setRoughness( 0.75f );
            materials[i]->getShaderParameters().SetScalar( scalarHandles[i], 0.5f );
            materials[i]->getNativeMaterial();
            require( materials[i]->getShaderParameters().GetScalar( scalarHandles[i] ) == 0.5f &&
                     passes[i]->getRoughness() == 0.75f, "full-memory baseline final surface/value parity" );
        }
        result.sample( "all_edited_no_views", owners( count, count, 0, 0 ) );
        for( size_t i = 0; i < count; ++i )
        {
            meshes[i]->setMaterial( nullptr, 0 ); meshes[i]->setMaterial( nullptr, 1 );
            passes[i] = nullptr; states[i] = nullptr;
            materials[i]->unload( nullptr ); materials[i] = nullptr;
        }
        fixture.graphics->pending.clear();
        result.sample( "graphs_unloaded", owners( 0, 0, 0, 0 ) );
#endif
        require( prototype.GetScalar( sourceHandle ) == 0.75f,
                 "full-memory edits never change the retained authored source" );
    }
    result.sample( "inputs_and_local_schema_destroyed_registry_retained" );
    return result;
}

#endif
