#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawMaterialDrawRecords.hpp>
#include <WPGraphics/ClawMaterial.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include "ClawMeshMaterial.hpp"
#include <atomic>
#include <limits>
#include <stdexcept>
#include <optional>
#include "workphone_graphics_cull_job.h"
#include "workphone_graphics_object.h"

namespace workphone::render
{
    namespace
    {
        std::atomic<u64> nextPublication{ 1 };

        u64 newPublication()
        {
            // Saturate instead of reusing an identity, including after overflow.
            auto id = nextPublication.load( std::memory_order_relaxed );
            do
            {
                if( id == std::numeric_limits<u64>::max() )
                {
                    throw std::overflow_error( "material publication identities exhausted" );
                }
            } while( !nextPublication.compare_exchange_weak( id, id + 1, std::memory_order_relaxed ) );
            return id;
        }
    }  // namespace

    ClawMaterialDrawRecords::ClawMaterialDrawRecords() : m_publication( newPublication() )
    {
    }

    ClawMaterialDrawRecords::~ClawMaterialDrawRecords() = default;

    size_t ClawMaterialDrawRecords::StorageResource::payloadOffset( size_t alignment )
    {
        return ( sizeof( Block ) + alignment - 1 ) & ~( alignment - 1 );
    }

    ClawMaterialDrawRecords::StorageResource::~StorageResource()
    {
        while( m_blocks )
        {
            const auto block = m_blocks;
            m_blocks = block->allNext;
            freeSourceBudgetStorage( block, block->allocationAlignment );
        }
    }

    void *ClawMaterialDrawRecords::StorageResource::do_allocate( size_t requested, size_t alignment )
    {
        // Keep returned lookup/bucket allocations until scene retirement so
        // later frames can reuse them. Small nodes are retained in lazy slabs;
        // this list holds upstream blocks/arrays, rather than individual nodes.
        for( auto link = &m_free; m_cache && *link; link = &( *link )->freeNext )
        {
            if( auto block = *link; block->requested == requested && block->alignment == alignment )
            {
                *link = block->freeNext;
                return reinterpret_cast<char *>( block ) + payloadOffset( alignment );
            }
        }
        const auto offset = payloadOffset( alignment );
        if( requested > std::numeric_limits<size_t>::max() - offset )
        {
            throw std::bad_alloc();
        }
        const auto total = offset + requested;
        const auto allocationAlignment = std::max( alignment, alignof( Block ) );
        auto pointer = allocateSourceBudgetStorage( total, allocationAlignment );
        auto block = ::new( pointer )
            Block{ m_blocks, nullptr, requested, alignment, total, allocationAlignment };
        m_blocks = block;
        ++m_counters.allocations;
        m_counters.bytes += total;
        m_counters.peak = std::max( m_counters.peak, m_counters.bytes );
        return static_cast<char *>( pointer ) + offset;
    }

    void ClawMaterialDrawRecords::StorageResource::do_deallocate( void *pointer, size_t requested,
                                                                  size_t alignment )
    {
        (void)requested;
        auto block =
            reinterpret_cast<Block *>( static_cast<char *>( pointer ) - payloadOffset( alignment ) );
        if( m_cache )
        {
            block->freeNext = m_free;
            m_free = block;
        }
        else
        {
            auto link = &m_blocks;
            while( *link != block )
            {
                link = &( *link )->allNext;
            }
            *link = block->allNext;
            const auto total = block->total, allocationAlignment = block->allocationAlignment;
            freeSourceBudgetStorage( block, allocationAlignment );
            m_counters.bytes -= total;
        }
    }

    u32 ClawMaterialDrawRecords::captureMaterial( SmartPtr<IMaterial> material )
    {
        if( auto found = m_materials.find( material.get() ); found != m_materials.end() )
        {
            return found->second;
        }
        if( m_records.size() >= std::numeric_limits<u32>::max() )
        {
            throw std::length_error( "too many captured materials" );
        }
        Owner owner;
        owner.material = std::move( material );
        Record record;
        if( auto claw = dynamic_cast<ClawMaterial *>( owner.material.get() ) )
        {
            owner.runtime = claw->getRuntimePublication();
            record.runtimeRequested = claw->hasRuntimeProgram();
            record.runtime = owner.runtime.get();
        }
        if( auto concrete = dynamic_cast<Material *>( owner.material.get() ) )
        {
            owner.state = concrete->getRenderState();
        }
        else if( owner.material )
        {
            for( auto technique : owner.material->getTechniques() )
            {
                if( technique )
                {
                    for( auto pass : technique->getPasses() )
                    {
                        if( auto context = pass ? pass->getStateContext() : nullptr )
                        {
                            if( auto data =
                                    context->getStateDataById<MaterialPassStateData>( pass->getId() ) )
                            {
                                owner.state = data->getRenderState();
                                break;
                            }
                        }
                    }
                }
                if( owner.state )
                {
                    break;
                }
            }
        }
        record.material = owner.material.get();
        record.state = owner.state.get();
        const auto index = static_cast<u32>( m_records.size() );
        m_owners.push_back( std::move( owner ) );
        try
        {
            m_records.push_back( record );
            try
            {
                m_materials.emplace( record.material, index );
            }
            catch( ... )
            {
                m_records.pop_back();
                throw;
            }
        }
        catch( ... )
        {
            m_owners.pop_back();
            throw;
        }
        return index;
    }

    void ClawMaterialDrawRecords::seal( ClawMaterialRenderCache *cache )
    {
        if( m_sealed )
        {
            return;
        }
        std::optional<ClawMaterialRenderCache> standalone;
        if( !cache )
        {
            standalone.emplace();
            cache = &*standalone;
        }
        const auto before = cache->getPreparationCount();
        for( size_t index = 0; index < m_records.size(); ++index )
        {
            if( auto &owner = m_owners[index]; owner.state )
            {
                m_records[index].handle = cache->resolve( owner.state );
                m_records[index].prepared = cache->lookup( m_records[index].handle );
            }
        }
        m_preparedRecords = cache->retain();
        m_preparations = cache->getPreparationCount() - before;
        // Release builder entries, retaining buckets and pooled nodes for the
        // next publication. No material/resource owner lives in this map.
        m_materials.clear();
        m_sealed = true;
    }

    void ClawMaterialDrawRecords::capture( ClawMesh *mesh )
    {
        if( m_sealed )
        {
            throw std::logic_error( "captured materials are immutable" );
        }
        if( !mesh || m_meshes.find( mesh ) != m_meshes.end() )
        {
            return;
        }
        const auto native = mesh->getNativeMesh();
        if( !native )
        {
            return;
        }
        const auto count =
            static_cast<u32>( std::max<s32>( 1, wp_graphics_mesh_get_submesh_count( native ) ) );
        if( m_bindings.size() > std::numeric_limits<u32>::max() - count )
        {
            throw std::length_error( "too many captured material bindings" );
        }
        Binding binding{ m_publication, mesh, static_cast<u32>( m_bindings.size() ), count };
        try
        {
            for( u32 section = 0; section < count; ++section )
            {
                m_bindings.push_back( captureMaterial( resolveMeshMaterial( mesh, section ) ) );
            }
            m_meshes.emplace( mesh, binding );
        }
        catch( ... )
        {
            m_bindings.resize( binding.first );
            throw;
        }
    }

    void ClawMaterialDrawRecords::clear()
    {
        const auto publication = newPublication();
        m_records.clear();
        m_owners.clear();
        m_preparedRecords = {};
        m_bindings.clear();
        m_materials.clear();
        m_meshes.clear();
        m_sortKeys.clear();
        m_sortOrder.clear();
        m_publication = publication;
        m_preparations = 0;
        m_sealed = false;
    }

    const ClawMaterialDrawRecords::Binding *ClawMaterialDrawRecords::find( const ClawMesh *mesh ) const
    {
        if( !m_sealed )
        {
            return nullptr;
        }
        const auto found = m_meshes.find( mesh );
        return found == m_meshes.end() ? nullptr : &found->second;
    }

    u64 ClawMaterialDrawRecords::getDenseCapacityBytes() const
    {
        return m_records.capacity() * sizeof( Record ) + m_owners.capacity() * sizeof( Owner ) +
               m_bindings.capacity() * sizeof( u32 ) +
               m_sortKeys.capacity() * sizeof( ClawDrawSortKey ) +
               m_sortOrder.capacity() * sizeof( int32_t );
    }

    bool ClawMaterialDrawRecords::sortView( wp_graphics_cull_result *&view,
                                            ClawDrawSortStrategy strategy, double bucketSize )
    {
        if( !m_sealed || !view || !validDrawSort( strategy, bucketSize ) )
        {
            return false;
        }
        if( strategy == ClawDrawSortStrategy::QueueOrder )
        {
            return true;
        }
        wp_mat4f camera, projection;
        if( !wp_graphics_cull_result_get_camera_matrices( view, &camera, &projection ) )
        {
            return true;
        }
        const auto count = wp_graphics_cull_result_get_visible_count( view );
        m_sortKeys.resize( count );
        m_sortOrder.resize( count );
        for( s32 index = 0; index < count; ++index )
        {
            auto &key = m_sortKeys[index];
            key = {};
            key.index = index;
            auto object = wp_graphics_cull_result_get_object( view, index );
            if( !object )
            {
                continue;
            }
            if( wp_graphics_object_get_flags( object ) &
                ( WORKPHONE_GRAPHICS_OBJECT_FLAG_OVERLAY | WORKPHONE_GRAPHICS_OBJECT_FLAG_UI ) )
            {
                continue;
            }
            key.queue = wp_graphics_object_get_render_queue_group( object );
            key.zOrder = wp_graphics_object_get_z_order( object );
            auto mesh = static_cast<const ClawMesh *>( wp_graphics_object_get_submit_data( object ) );
            auto binding = find( mesh );
            auto record = binding ? get( *binding, 0 ) : nullptr;
            if( !record || !record->state )
            {
                continue;
            }
            bool homogeneous = true;
            for( u32 section = 1; section < binding->count; ++section )
            {
                if( get( *binding, section ) != record )
                {
                    homogeneous = false;
                    break;
                }
            }
            const auto &state = *record->state;
            if( !homogeneous || ( state.depthTest != 1u && state.depthTest != 2u ) ||
                state.materialType == MaterialType::UI || state.getFlag( refractionEnabledFlag ) )
            {
                continue;
            }
            key.transparent = state.getFlag( transparentFlag ) || state.blendMode != 0;
            if( !key.transparent && !state.getFlag( depthWriteFlag ) )
            {
                continue;
            }
            wp_mat4f world;
            if( !wp_graphics_cull_result_get_world_matrix( view, index, &world ) )
            {
                continue;
            }
            // Native matrices use m[row][column]. Use signed view depth:
            // off-axis objects must not change transparency order by radius.
            key.depth =
                -( double( camera.m[2][0] ) * world.m[0][3] + double( camera.m[2][1] ) * world.m[1][3] +
                   double( camera.m[2][2] ) * world.m[2][3] + camera.m[2][3] );
            key.material = static_cast<u32>( record - m_records.data() );
            key.barrier = !std::isfinite( key.depth );
        }
        sortDrawKeys( m_sortKeys.data(), m_sortKeys.size(), strategy, bucketSize );
        bool changed = false;
        for( s32 index = 0; index < count; ++index )
        {
            m_sortOrder[index] = static_cast<s32>( m_sortKeys[index].index );
            changed = changed || m_sortOrder[index] != index;
        }
        if( !changed )
        {
            return true;
        }
        auto ordered = wp_graphics_cull_result_create_ordered( view, m_sortOrder.data(), count );
        if( !ordered )
        {
            return false;
        }
        wp_graphics_cull_result_destroy( view );
        view = ordered;
        return true;
    }

    bool ClawMaterialDrawRecords::valid( const Binding &binding, const ClawMesh *mesh,
                                         u32 sections ) const
    {
        const auto found = m_meshes.find( mesh );
        return m_sealed && found != m_meshes.end() && found->second.first == binding.first &&
               found->second.count == binding.count && binding.publication == m_publication &&
               binding.mesh == mesh && binding.count == sections && binding.first <= m_bindings.size() &&
               binding.count <= m_bindings.size() - binding.first;
    }

    const ClawMaterialDrawRecords::Record *ClawMaterialDrawRecords::get( const Binding &binding,
                                                                         u32 section ) const
    {
        // Binding/mesh correspondence is checked once by valid() at submission.
        // Per-section borrowing uses numeric bounds and identities, without a map lookup.
        if( !m_sealed || binding.publication != m_publication || section >= binding.count ||
            binding.first > m_bindings.size() || binding.count > m_bindings.size() - binding.first )
        {
            return nullptr;
        }
        const auto index = m_bindings[binding.first + section];
        if( index >= m_records.size() )
        {
            return nullptr;
        }
        const auto &record = m_records[index];
        if( record.state &&
            m_preparedRecords.getPinned( record.handle, record.state ) != record.prepared )
        {
            return nullptr;
        }
        return &record;
    }
}  // namespace workphone::render
