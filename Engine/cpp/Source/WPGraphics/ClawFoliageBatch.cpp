#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawFoliageBatch.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include "ClawMeshMaterial.hpp"
#include <Workphone/Scene/TerrainIntegration.hpp>
#include <WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h>
#include <WorkphoneGraphics/workphone_graphics_mesh.h>
#include <cstring>
#include <stdexcept>

namespace workphone::render
{
    struct ClawFoliageBatch::Impl
    {
        SmartPtr<ClawMesh> mesh;
        Array<wp_instance_pntc_dx11> instances;
        u32 mask = ~u32( 0 );
        bool shadows = true;
    };

    ClawFoliageBatch::ClawFoliageBatch() : m_impl( std::make_unique<Impl>() ) {}
    ClawFoliageBatch::~ClawFoliageBatch() = default;

    std::shared_ptr<const ClawFoliageBatch> ClawFoliageBatch::create(
        SmartPtr<ClawMesh> mesh, const Array<ClawFoliageInstance> &instances, String &error,
        u32 mask, bool shadows )
    {
        return createPrepared( mesh, instances, error, mask, shadows, false );
    }

    std::shared_ptr<const ClawFoliageBatch> ClawFoliageBatch::createTerrainGrounded(
        SmartPtr<ClawMesh> mesh, const scene::TerrainGroundPopulation &population, String &error,
        const ColourF &tint, u32 mask, bool shadows )
    {
        if( !population.getTerrainSnapshot() )
        { error = "Foliage ground population has not been sampled."; return nullptr; }
        Array<ClawFoliageInstance> instances;
        for( const auto &placement : population.getPlacements() )
        {
            if( !placement.supported ) continue;
            const auto &pose = placement.worldTransform;
            const auto &p = pose.getPosition(); const auto &s = pose.getScale(); const auto &q = pose.getOrientation();
            ClawFoliageInstance instance;
            instance.transform.makeTransform( { f32(p.x), f32(p.y), f32(p.z) },
                { f32(s.x), f32(s.y), f32(s.z) }, { f32(q.w), f32(q.x), f32(q.y), f32(q.z) } );
            instance.tint = tint;
            instances.push_back( instance );
        }
        return createPrepared( mesh, instances, error, mask, shadows, true );
    }

    std::shared_ptr<const ClawFoliageBatch> ClawFoliageBatch::createPrepared(
        SmartPtr<ClawMesh> mesh, const Array<ClawFoliageInstance> &instances, String &error,
        u32 mask, bool shadows, bool allowEmpty )
    {
        error.clear();
        try
        {
            auto native = mesh ? mesh->getNativeMesh() : nullptr;
            if( !native || ( instances.empty() && !allowEmpty ) || instances.size() > maximumInstances ||
                wp_graphics_mesh_get_primitive_type( native ) != WORKPHONE_PRIMITIVE_TRIANGLE_LIST ||
                !wp_graphics_mesh_get_vertex_count( native ) ||
                !wp_graphics_mesh_get_index_count( native ) )
            {
                error = "Foliage requires an indexed triangle mesh and 1..65536 instances.";
                return nullptr;
            }
            const auto vertexCount = wp_graphics_mesh_get_vertex_count( native );
            const auto indexCount = wp_graphics_mesh_get_index_count( native );
            const auto indices = wp_graphics_mesh_get_indices( native );
            if( !indices || indexCount % 3 )
            {
                error = "Foliage requires complete indexed triangles.";
                return nullptr;
            }
            for( u32 i = 0; i < indexCount; ++i )
            {
                const auto index = wp_graphics_mesh_get_index_format( native ) == WORKPHONE_INDEX_FORMAT_UINT16
                    ? static_cast<const uint16_t *>( indices )[i] : static_cast<const wp_u32 *>( indices )[i];
                if( index >= vertexCount )
                {
                    error = "Foliage mesh index is outside its vertex buffer.";
                    return nullptr;
                }
            }
            for( s32 i = 0; i < wp_graphics_mesh_get_submesh_count( native ); ++i )
            {
                const auto section = wp_graphics_mesh_get_submesh( native, i );
                if( !section || !section->index_count || section->index_count % 3 ||
                    section->index_start > indexCount || section->index_count > indexCount - section->index_start )
                {
                    error = "Foliage material section is outside its index buffer or has incomplete triangles.";
                    return nullptr;
                }
            }
            const auto sections = std::max<s32>( 1, wp_graphics_mesh_get_submesh_count( native ) );
            for( s32 i = 0; i < sections; ++i )
            {
                if( !isSupportedFoliageMaterial( resolveMeshMaterial( mesh.get(), i ) ) )
                {
                    error = "Foliage material sections must be opaque or alpha-tested; blended transparency is unsupported.";
                    return nullptr;
                }
            }
            auto candidate = std::shared_ptr<ClawFoliageBatch>( new ClawFoliageBatch );
            candidate->m_impl->instances.resize( instances.size() );
            for( size_t i = 0; i < instances.size(); ++i )
            {
                auto &output = candidate->m_impl->instances[i];
                std::memcpy( output.transform.m, instances[i].transform.ptr(), sizeof( output.transform.m ) );
                const auto &tint = instances[i].tint;
                output.tint = { tint.r, tint.g, tint.b, tint.a };
            }
            if( !instances.empty() && !wp_renderer_dx11_validate_instances_pntc( candidate->m_impl->instances.data(),
                                                          static_cast<wp_s32>( instances.size() ) ) )
            {
                error = "Foliage transforms must be finite, affine and nonsingular with positive determinant; tint must be finite.";
                return nullptr;
            }
            candidate->m_impl->mesh = mesh;
            candidate->m_impl->mask = mask;
            candidate->m_impl->shadows = shadows;
            return candidate;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return nullptr;
        }
    }

    u32 ClawFoliageBatch::instanceCount() const { return static_cast<u32>( m_impl->instances.size() ); }
    u32 ClawFoliageBatch::visibilityMask() const { return m_impl->mask; }
    bool ClawFoliageBatch::castsShadows() const { return m_impl->shadows; }
    u64 ClawFoliageBatch::render( ClawRendererDX11 &renderer ) const
    {
        return renderer.renderMeshInstances( m_impl->mesh.get(), m_impl->instances.data(), instanceCount() );
    }
}
