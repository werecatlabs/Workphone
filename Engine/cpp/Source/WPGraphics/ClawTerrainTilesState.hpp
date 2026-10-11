#ifndef ClawTerrainTilesState_h__
#define ClawTerrainTilesState_h__
#include <WPGraphics/ClawTerrain.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <workphone_graphics_mesh.h>
#include <map>

namespace workphone::render
{
    struct ClawTerrainTileMesh
    {
        wp_graphics_mesh *mesh = nullptr;
        u64 bytes = 0;
        // One lease follows the actual buffer, including when active and staged
        // maps share it. Member destruction releases it after the GPU is retired.
        StreamBudget gpuBudget;
        ~ClawTerrainTileMesh()
        {
            if( mesh )
            {
                ClawRendererDX11::forgetMesh( mesh );
                wp_graphics_mesh_destroy( mesh );
            }
        }
    };
    struct ClawTerrainTileState
    {
        // Declare leases before snapshots/worker so they outlive retained data.
        StreamBudget requestedSourceBudget, activeSourceBudget;
        TerrainTileStream stream;
        TerrainSnapshot requested, active;
        TerrainSnapshot cookedSource;
        Array<TerrainTileInfo> cookedDescriptions;
        bool usedCookedTileManifest = false;
        Array<TerrainTileInfo> descriptions;
        Array<TerrainTileKey> demand;
        using Meshes = std::map<u64, std::shared_ptr<ClawTerrainTileMesh>>;
        Meshes resident, staged;
        u32 maximumTiles = 2048, uploadsPerUpdate = 4;
        u64 maximumBytes = 128ull * 1024 * 1024, uploadedBytes = 0;
        u64 uploadFrame = 0;
        u32 frameUploads = 0;
        void *device = nullptr;
        String lastError;
        String preparationError;
        ClawTerrainMaterial::TextureResolver textureResolver;
        bool refreshMaterial = false;
        std::shared_ptr<const ClawTerrainMaterial::Candidate> materialCandidate;
        static u64 key( TerrainTileKey k )
        {
            return ( u64( k.tile ) << 32 ) | k.lod;
        }
        static u64 bytes( const Meshes &m )
        {
            u64 result = 0;
            for( const auto &e : m )
                result += e.second->bytes;
            return result;
        }
    };
}  // namespace workphone::render
#endif
