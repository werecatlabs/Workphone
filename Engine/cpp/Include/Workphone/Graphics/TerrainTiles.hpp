#ifndef WPTerrainTiles_h__
#define WPTerrainTiles_h__

#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <array>
#include <functional>
#include <memory>

namespace workphone::render
{
    inline constexpr u32 terrainTileCells = 64;
    inline constexpr u32 terrainTileLods = 4;

    struct TerrainTileInfo
    {
        u32 id = 0;
        Vector2I first, last;  // Inclusive sample coordinates; adjacent tiles share the border.
        Vector3F minimum, maximum;
        std::array<f32, terrainTileLods> errors{};  // Conservative local vertical error, metres.
        bool hasHoles = false;
    };

    struct TerrainTileKey
    {
        u32 tile = 0, lod = 0;
        bool operator==( const TerrainTileKey &other ) const
        {
            return tile == other.tile && lod == other.lod;
        }
    };

    struct TerrainTileView
    {
        Matrix4F localToClip;
        Vector3F localCamera;
        f32 pixelsPerMetreAtUnitDistance = 512;
        f32 verticalScale = 1;
        f32 minimumWorldScale = 1;
        f32 maximumPixelError = 2;
        f32 prefetchMargin = 0;  // Extra fraction of the view width/height, clamped to 0..0.5.
        bool cull = true;
    };

    WPCore_API bool describeTerrainTiles( const TerrainData &, Array<TerrainTileInfo> &, String & );
    /** Coarse interiors retain EVERY source sample on tile boundaries. This fixed
     * border policy is watertight for any neighboring LOD and uses source-wide
     * normals/UVs. Tiles containing holes use exact LOD0 geometry. */
    WPCore_API bool buildTerrainTile( const TerrainData &, const TerrainTileInfo &, u32 lod,
                                      TerrainMeshData &, String &,
                                      const std::function<bool()> &cancelled = {} );
    WPCore_API Array<TerrainTileKey> selectTerrainTiles( const Array<TerrainTileInfo> &,
                                                         const Array<TerrainTileView> &,
                                                         const Array<TerrainTileKey> &previous,
                                                         u32 maximumTiles );
    WPCore_API u64 terrainMeshBytes( const TerrainMeshData & );
    WPCore_API u64 terrainSourceBytes( const TerrainData & );

    struct TerrainTileResult
    {
        TerrainSnapshot source;
        TerrainTileKey key;
        std::shared_ptr<const TerrainMeshData> mesh;
        String error;
    };
    struct TerrainStreamStats
    {
        u64 generation = 0, completed = 0, cancelled = 0;
        u64 readyBytes = 0;  // Conservative queued result reservations, not measured allocator usage.
        u32 pending = 0, active = 0, ready = 0;
    };

    /** Geometry producer on the shared bounded world streaming scheduler.
     * Replace demands on the owner thread. Source/demand changes cancel stale
     * work; destruction drains this producer. At most 256 requests, eight completed
     * tiles and 8 MiB of completed geometry can wait for render-owner upload. */
    class WPCore_API TerrainTileStream final
    {
    public:
        TerrainTileStream();
        ~TerrainTileStream();
        TerrainTileStream( const TerrainTileStream & ) = delete;
        TerrainTileStream &operator=( const TerrainTileStream & ) = delete;
        bool request( TerrainSnapshot, const Array<TerrainTileInfo> &, const Array<TerrainTileKey> &,
                      String & );
        bool take( TerrainTileResult & );
        void cancel();
        void drain();
        TerrainStreamStats stats() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}  // namespace workphone::render
#endif
