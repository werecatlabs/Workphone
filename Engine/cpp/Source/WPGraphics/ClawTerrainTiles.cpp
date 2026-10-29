#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawTerrain.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawTerrainMaterial.hpp>
#include <WPGraphics/Resources/TerrainResource.hpp>
#include "ClawTerrainTilesState.hpp"
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>
#include <algorithm>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone::render
{
    namespace
    {
        std::shared_ptr<ClawTerrainTileMesh> uploadTile( ClawRendererDX11 &renderer,
                                                         const TerrainMeshData &data, String &error )
        {
            auto result = std::make_shared<ClawTerrainTileMesh>();
            if( data.indices.empty() )
            {
                return result;  // A completely excavated tile is valid.
            }
            result->bytes = data.positions.size() * sizeof( wp_graphics_mesh_vertex_pntc ) +
                            data.indices.size() * sizeof( u32 );
            result->gpuBudget = WorldStreamingScheduler::shared()->reserve( StreamMemoryClass::Gpu,
                                                                            result->bytes, error );
            if( !result->gpuBudget )
            {
                return {};
            }
            Array<wp_graphics_mesh_vertex_pntc> vertices( data.positions.size() );
            for( size_t i = 0; i < vertices.size(); ++i )
            {
                const auto &p = data.positions[i];
                const auto &n = data.normals[i];
                const auto &uv = data.uvs[i];
                vertices[i] = { { p.x, p.y, p.z }, { n.x, n.y, n.z }, { uv.x, uv.y }, 0xffffffffu };
            }
            result->mesh = wp_graphics_mesh_create();
            if( !result->mesh ||
                !wp_graphics_mesh_set_vertices( result->mesh, WORKPHONE_VERTEX_FORMAT_PNTC,
                                                vertices.data(), u32( vertices.size() ) ) ||
                !wp_graphics_mesh_set_indices_u32( result->mesh, data.indices.data(),
                                                   u32( data.indices.size() ) ) ||
                wp_graphics_mesh_add_submesh( result->mesh, 0, u32( data.indices.size() ), 0 ) < 0 ||
                !renderer.prepareTerrainGeometry( result->mesh ) )
            {
                error = "Terrain tile CPU/GPU allocation failed; previous terrain retained";
                return {};
            }
            return result;
        }
    }  // namespace

    bool ClawTerrain::setTerrainResidencyLimits( u32 tiles, u64 bytes, u32 uploads, String &error )
    {
        if( !tiles || tiles > 2048 || bytes < 512 * 1024 || bytes > 512ull * 1024 * 1024 || !uploads ||
            uploads > 16 )
        {
            error =
                "Terrain residency requires 1..2048 tiles, 512 KiB..512 MiB and 1..16 uploads/update";
            return false;
        }
        if( !m_tiles->resident.empty() || !m_tiles->staged.empty() )
        {
            error = "Unload terrain before changing residency limits";
            return false;
        }
        m_tiles->maximumTiles = tiles;
        m_tiles->maximumBytes = bytes;
        m_tiles->uploadsPerUpdate = uploads;
        error.clear();
        return true;
    }

    bool ClawTerrain::prepareRenderTiles( ClawRendererDX11 &renderer,
                                          const Array<TerrainTileView> &views, String &error )
    {
        auto &p = *m_tiles;
        error.clear();

        struct RecordError
        {
            String &destination;
            const String &source;

            ~RecordError() noexcept
            {
                try
                {
                    destination = source;
                }
                catch( ... )
                {
                }
            }
        } recordError{ p.preparationError, error };

        auto snapshot = getTerrainSnapshot();
        auto *native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        if( !snapshot || !native || views.empty() || views.size() > 16 )
        {
            error = "Terrain residency needs a source, renderer and one to sixteen views";
            return false;
        }
        auto *device = wp_renderer_dx11_get_device( native );
        try
        {
            if( snapshot != p.requested )
            {
                // Superseded staging can never publish. Release its reservations
                // before admitting the new request, or a full source budget can
                // leave every retry blocked by an obsolete pending generation.
                // Active data and geometry remain intact throughout this path.
                p.stream.cancel();
                p.materialCandidate.reset();
                p.staged.clear();
                p.descriptions.clear();
                p.demand.clear();
                p.lastError.clear();
                p.requested.reset();
                p.requestedSourceBudget.reset();
                if( snapshot != p.cookedSource )
                {
                    p.cookedSource.reset();
                    p.cookedDescriptions.clear();
                    p.usedCookedTileManifest = false;
                }
                auto sourceBudget = snapshot == p.active ? p.activeSourceBudget
                                                         : WorldStreamingScheduler::shared()->reserve(
                                                               StreamMemoryClass::SourceCpu,
                                                               terrainSourceBytes( *snapshot ), error );
                if( !sourceBudget )
                {
                    return false;
                }
                Array<TerrainTileInfo> descriptions;
                if( snapshot == p.cookedSource )
                {
                    descriptions = p.cookedDescriptions;
                    p.usedCookedTileManifest = true;
                }
                else
                {
                    if( !describeTerrainTiles( *snapshot, descriptions, error ) )
                    {
                        return false;
                    }
                }
                p.descriptions = std::move( descriptions );
                p.requested = snapshot;
                p.requestedSourceBudget = std::move( sourceBudget );
            }
            auto qualityViews = views;
            if( auto app = core::IApplicationManager::instancePtr() )
            {
                if( auto settings = app->getPlayerSettings() )
                {
                    s32 quality = 2;
                    settings->getPropertyValue( "quality.terrain", quality );
                    constexpr f32 tolerances[] = { 4.0f, 2.0f, 1.0f, 0.5f };
                    for( auto &view : qualityViews )
                    {
                        view.maximumPixelError *= tolerances[std::clamp( quality, 0, 3 )];
                    }
                }
            }
            auto demand = selectTerrainTiles( p.descriptions, qualityViews, p.demand, p.maximumTiles );
            // Distance order controls worker priority, never publication identity.
            // A camera crossing a tile centre must not restart the same demand.
            auto canonicalDemand = demand;
            std::sort( canonicalDemand.begin(), canonicalDemand.end(),
                       []( TerrainTileKey a, TerrainTileKey b ) {
                           return ClawTerrainTileState::key( a ) < ClawTerrainTileState::key( b );
                       } );
            const bool deviceChanged = p.device != device;
            const auto frame = renderer.terrainFrameToken();
            if( !frame || p.uploadFrame != frame || deviceChanged )
            {
                p.uploadFrame = frame;
                p.frameUploads = 0;
            }
            p.refreshMaterial = p.refreshMaterial || deviceChanged;
            const bool residentMatches =
                p.resident.size() == demand.size() &&
                std::all_of( demand.begin(), demand.end(), [&]( TerrainTileKey key ) {
                    return p.resident.count( p.key( key ) ) != 0;
                } );
            if( !deviceChanged && canonicalDemand == p.demand && p.active == snapshot &&
                residentMatches )
            {
                if( p.refreshMaterial )
                {
                    auto candidate = m_terrainMaterial->stage( renderer, snapshot, error,
                                                               p.textureResolver, getTexture( 0 ) );
                    if( !candidate ||
                        !m_terrainMaterial->publish( renderer, std::move( candidate ), error ) )
                    {
                        return false;
                    }
                    p.refreshMaterial = false;
                }
                if( hasPendingTerrainPublication() )
                {
                    return publishPendingTerrain( snapshot, {}, error );
                }
                return true;
            }
            if( deviceChanged || canonicalDemand != p.demand )
            {
                p.stream.cancel();
                p.lastError.clear();
                ClawTerrainTileState::Meshes previousStaged;
                previousStaged.swap( p.staged );
                if( deviceChanged )
                {
                    p.materialCandidate.reset();
                }
                Array<TerrainTileKey> missing;
                for( auto key : demand )
                {
                    const auto staged = previousStaged.find( p.key( key ) );
                    const auto existing = p.resident.find( p.key( key ) );
                    if( !deviceChanged && staged != previousStaged.end() )
                    {
                        p.staged.emplace( staged->first, staged->second );
                    }
                    else if( !deviceChanged && p.active == snapshot && existing != p.resident.end() )
                    {
                        p.staged.emplace( existing->first, existing->second );
                    }
                    else if( missing.size() < 256 )
                    {
                        missing.push_back( key );
                    }
                }
                p.demand = std::move( canonicalDemand );
                p.device = device;
                if( !p.stream.request( snapshot, p.descriptions, missing, error ) )
                {
                    return false;
                }
            }
            // Small assets remain immediately usable by tools and existing render
            // callers. Large assets are generated only by the bounded worker.
            const bool immediate = p.descriptions.size() == 1;
            const auto uploadBudget =
                p.uploadsPerUpdate - std::min( p.frameUploads, p.uploadsPerUpdate );
            u32 uploads = 0;
            while( uploads < uploadBudget )
            {
                StreamBudget immediateWork, immediateResult;
                TerrainTileResult result;
                if( immediate && !p.demand.empty() &&
                    p.staged.find( p.key( p.demand.front() ) ) == p.staged.end() )
                {
                    p.stream.cancel();
                    auto scheduler = WorldStreamingScheduler::shared();
                    immediateWork =
                        scheduler->reserve( StreamMemoryClass::WorkingCpu, 1ull << 20, error );
                    if( immediateWork )
                    {
                        immediateResult =
                            scheduler->reserve( StreamMemoryClass::ResultCpu, 1ull << 20, error );
                    }
                    if( !immediateWork || !immediateResult )
                    {
                        p.demand.clear();
                        return false;
                    }
                    result.source = snapshot;
                    result.key = p.demand.front();
                    auto mesh = std::make_shared<TerrainMeshData>();
                    if( buildTerrainTile( *snapshot, p.descriptions[result.key.tile], result.key.lod,
                                          *mesh, result.error ) )
                    {
                        result.mesh = std::move( mesh );
                    }
                }
                else if( !p.stream.take( result ) )
                {
                    break;
                }
                if( result.source != snapshot ||
                    std::find( p.demand.begin(), p.demand.end(), result.key ) == p.demand.end() )
                {
                    continue;
                }
                if( p.staged.count( p.key( result.key ) ) )
                {
                    continue;
                }
                if( !result.mesh )
                {
                    p.lastError = result.error;
                    error = p.lastError;
                    // Retry transient worker failures with the same view/source,
                    // preserving useful staged tiles and the active generation.
                    p.demand.clear();
                    return false;
                }
                const auto bytes =
                    result.mesh->indices.empty()
                        ? 0
                        : result.mesh->positions.size() * sizeof( wp_graphics_mesh_vertex_pntc ) +
                              result.mesh->indices.size() * sizeof( u32 );
                if( bytes > p.maximumBytes - p.bytes( p.staged ) )
                {
                    p.lastError =
                        "Terrain residency budget exhausted; reduce view demand or increase the "
                        "configured budget";
                    error = p.lastError;
                    return false;
                }
                auto uploaded = uploadTile( renderer, *result.mesh, error );
                if( !uploaded )
                {
                    p.lastError = error;
                    p.demand.clear();
                    return false;
                }
                p.uploadedBytes += uploaded->bytes;
                p.staged.emplace( p.key( result.key ), std::move( uploaded ) );
                ++uploads;
                ++p.frameUploads;
            }
            if( !p.lastError.empty() )
            {
                error = p.lastError;
                return false;
            }
            const auto worker = p.stream.stats();
            if( p.staged.size() < p.demand.size() && !worker.pending && !worker.active && !worker.ready )
            {
                Array<TerrainTileKey> missing;
                for( auto key : demand )
                {
                    if( !p.staged.count( p.key( key ) ) && missing.size() < 256 )
                    {
                        missing.push_back( key );
                    }
                }
                // Finish one bounded request window before queuing the next.
                p.stream.cancel();
                if( !p.stream.request( snapshot, p.descriptions, missing, error ) )
                {
                    return false;
                }
            }
            if( p.staged.size() == p.demand.size() && getTerrainSnapshot() == snapshot )
            {
                // Material preparation is transactional too: its published bundle
                // must use the same source as the complete tile generation.
                if( p.refreshMaterial || m_terrainMaterial->snapshot() != snapshot )
                {
                    if( !p.materialCandidate )
                    {
                        p.materialCandidate = m_terrainMaterial->stage(
                            renderer, snapshot, error, p.textureResolver, getTexture( 0 ) );
                    }
                    if( !p.materialCandidate ||
                        !m_terrainMaterial->validateCandidate( renderer, p.materialCandidate, error ) )
                    {
                        return false;
                    }
                }
                const auto retiredSourceBudget = p.activeSourceBudget;
                const auto retiredSource = p.active;
                if( !publishPendingTerrain(
                        snapshot,
                        [&] {
                            if( p.materialCandidate )
                            {
                                m_terrainMaterial->swapPrevalidated( p.materialCandidate );
                            }
                            p.resident.swap( p.staged );
                            p.active = snapshot;
                            p.activeSourceBudget = p.requestedSourceBudget;
                            p.refreshMaterial = false;
                        },
                        error ) )
                {
                    return false;
                }
                // Retire buffers after leaving source/physics publication locks.
                if( retiredSource != snapshot )
                {
                    releaseRenderMesh();
                }
                p.staged.clear();
                p.materialCandidate.reset();
            }
            return p.active == snapshot && p.resident.size() == p.demand.size() &&
                   std::all_of( p.demand.begin(), p.demand.end(), [&]( TerrainTileKey key ) {
                       return p.resident.count( p.key( key ) ) != 0;
                   } );
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    Array<wp_graphics_mesh *> ClawTerrain::getNativeRenderTiles() const
    {
        Array<wp_graphics_mesh *> result;
        for( const auto &entry : m_tiles->resident )
        {
            if( entry.second->mesh )
            {
                result.push_back( entry.second->mesh );
            }
        }
        return result;
    }

    TerrainSnapshot ClawTerrain::getRenderedTerrainSnapshot() const
    {
        return m_tiles->active;
    }

    String ClawTerrain::getTerrainResidencyError() const
    {
        return m_tiles->preparationError;
    }

    ClawTerrainMaterial &ClawTerrain::terrainMaterial()
    {
        return *m_terrainMaterial;
    }

    bool ClawTerrain::applyCookedTerrain( const TerrainData &data,
                                          ClawTerrainMaterial::TextureResolver resolver, String &error )
    {
        if( !resolver )
        {
            error = "Cooked terrain requires a retained texture dependency resolver";
            return false;
        }
        if( !applyTerrainData( data, error ) )
        {
            return false;
        }
        m_tiles->textureResolver = std::move( resolver );
        m_tiles->cookedSource.reset();
        m_tiles->cookedDescriptions.clear();
        m_tiles->usedCookedTileManifest = false;
        // A new dependency generation can leave height/weight bytes unchanged.
        // Force preparation rather than retaining that generation's old SRVs.
        m_tiles->requested.reset();
        m_tiles->refreshMaterial = true;
        return true;
    }

    bool ClawTerrain::applyCookedTerrain( std::shared_ptr<const CookedTerrainCandidate> candidate,
                                          String &error )
    {
        if( !candidate )
        {
            error = "Cooked terrain GPU candidate is missing";
            return false;
        }
        try
        {
            // Copy only small verified tile metadata. The retained texture resolver
            // owns its cooked closure; CPU decode scratch is released with candidate.
            auto descriptions = candidate->tiles;
            if( !applyCookedTerrain( candidate->terrain, candidate->resolver, error ) )
            {
                return false;
            }
            m_tiles->cookedSource = getTerrainSnapshot();
            m_tiles->cookedDescriptions = std::move( descriptions );
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    ClawTerrainResidency ClawTerrain::getTerrainResidency() const
    {
        const auto &p = *m_tiles;
        ClawTerrainResidency s;
        s.residentTiles = u32( p.resident.size() );
        s.stagedTiles = u32( p.staged.size() );
        s.requestedTiles = u32( p.demand.size() );
        s.residentBytes = p.bytes( p.resident );
        s.stagedBytes = p.bytes( p.staged );
        s.uploadedBytes = p.uploadedBytes;
        s.activeRevision = p.active ? p.active->revision : 0;
        s.requestedRevision = p.requested ? p.requested->revision : 0;
        s.usedCookedTileManifest = p.usedCookedTileManifest;
        s.worker = p.stream.stats();
        return s;
    }
}  // namespace workphone::render
