#ifndef WPTerrainData_h__
#define WPTerrainData_h__

#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector4.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Math/Ray3.hpp>
#include <memory>
#include <array>

namespace workphone::render
{
    inline constexpr u32 terrainDataFormatVersion = 3;
    inline constexpr u64 terrainMaximumSamples = 4ull * 1024ull * 1024ull;
    inline constexpr u32 terrainMaximumDimension = 8193;

    struct TerrainLayer
    {
        String albedo, normal, orm;
        Vector2F uvScale = Vector2F( 1, 1 );  // texture cycles per local metre
        Vector4F tint = Vector4F( 1, 1, 1, 1 );
        f32 roughness = 1, metallic = 0, normalStrength = 1;
        // Height blending reads the linear ORM alpha channel; missing ORM has height zero.
        f32 heightScale = 1, heightBias = 0;

        bool operator==( const TerrainLayer &other ) const
        {
            return albedo == other.albedo && normal == other.normal && orm == other.orm &&
                   uvScale == other.uvScale && tint == other.tint && roughness == other.roughness &&
                   metallic == other.metallic && normalStrength == other.normalStrength &&
                   heightScale == other.heightScale && heightBias == other.heightBias;
        }
    };

    enum class TerrainProjection : u32
    {
        Planar = 0,
        Triplanar = 1
    };

    struct TerrainAppearance
    {
        TerrainProjection projection = TerrainProjection::Planar;
        f32 triplanarSharpness = 4;
        bool heightBlend = false;
        f32 heightTransition = .2f;
        String detailNormal;
        Vector2F detailUvScale = Vector2F( 4, 4 );
        f32 detailStrength = 1;
        f32 macroScale = .01f, macroStrength = 0;
        u32 macroSeed = 0;
        Vector4F macroTint = Vector4F( .5f, .5f, .5f, 1 );
        f32 wetness = 0, wetDarkening = .35f, wetRoughness = .12f;
        f32 snow = 0, snowRoughness = .8f;
        Vector4F snowTint = Vector4F( .9f, .95f, 1, 1 );

        bool operator==( const TerrainAppearance &o ) const
        {
            return projection == o.projection && triplanarSharpness == o.triplanarSharpness &&
                   heightBlend == o.heightBlend && heightTransition == o.heightTransition &&
                   detailNormal == o.detailNormal && detailUvScale == o.detailUvScale &&
                   detailStrength == o.detailStrength && macroScale == o.macroScale &&
                   macroStrength == o.macroStrength && macroSeed == o.macroSeed &&
                   macroTint == o.macroTint && wetness == o.wetness && wetDarkening == o.wetDarkening &&
                   wetRoughness == o.wetRoughness && snow == o.snow &&
                   snowRoughness == o.snowRoughness && snowTint == o.snowTint;
        }
    };

    /** Version-three rectangular heightfield. Heights are unscaled row-major samples:
     * index=z*dimensions.x+x; local position=(origin.x+x*spacing.x,
     * heights[index]*heightScale, origin.y+z*spacing.y). New assets use origin
     * -(dimensions-1)*spacing/2. Explicit origin preserves older asymmetric grids.
     * Terrain assigns revision when publishing; persistence must not reuse it as a token.
     */
    struct TerrainData
    {
        Vector2I dimensions = Vector2I( 2, 2 );
        Vector2F spacing = Vector2F( 1, 1 );
        Vector2F origin = Vector2F( -0.5f, -0.5f );
        f32 heightScale = 1.0f;
        Array<f32> heights = Array<f32>( 4, 0.0f );
        std::array<TerrainLayer, 4> layers;
        // Empty weights mean (1,0,0,0) everywhere. Otherwise one normalized value per sample.
        Array<Vector4F> layerWeights;
        TerrainAppearance appearance;
        // Optional sample-grid wetness/snow masks in [0,1]. Empty means (1,1);
        // global appearance strengths default to zero, preserving earlier assets.
        Array<Vector2F> surfaceMasks;
        // Empty means solid. Otherwise one 0/1 per row-major CELL, stride=dimensions.x-1.
        // A hole removes both runtime triangles; there is no hidden collision or gameplay pick surface.
        Array<u8> holes;
        u64 revision = 0;
    };

    using TerrainSnapshot = std::shared_ptr<const TerrainData>;

    struct TerrainMeshData
    {
        Array<Vector3F> positions;
        Array<Vector3F> normals;
        Array<Vector2F> uvs;
        Array<u32> indices;
    };

    WPCore_API bool validateTerrainData( const TerrainData &data, String &error );
    WPCore_API Vector4F normalizeTerrainWeights( const Vector4F &weights );
    WPCore_API bool terrainCellIsHole( const TerrainData &data, s32 x, s32 z );
    WPCore_API String serializeTerrainData( const TerrainData &data );
    /** Transactional parse; v1/v2 migrate with their existing appearance unchanged. */
    WPCore_API bool deserializeTerrainData( const String &json, TerrainData &output, String &error );
    /** Finite translation, positive nonzero scale and a normalized yaw-only quaternion. */
    WPCore_API bool validateTerrainTransform( const Transform3<real_Num> &transform, String &error );
    WPCore_API bool validateTerrainPlacement( const TerrainData &data,
                                              const Transform3<real_Num> &transform, String &error );

    /** Query validated snapshots without copying or rescanning their samples. Outside
     * the finite terrain footprint returns false. Triangles share the top-right to
     * bottom-left diagonal used by the render and collision mesh.
     */
    WPCore_API bool sampleTerrainHeight( const TerrainData &data, const Transform3<real_Num> &transform,
                                         const Vector3<real_Num> &worldPosition, f32 &worldHeight );
    WPCore_API bool intersectTerrain( const TerrainData &data, const Transform3<real_Num> &transform,
                                      const Ray3F &ray, Vector3<real_Num> &worldHit );
    /** Editor repair tools may query the authored heightfield beneath hole cells.
     * This does not make those cells solid for rendering, collision or gameplay queries.
     */
    WPCore_API bool sampleTerrainHeightForAuthoring(
        const TerrainData &data, const Transform3<real_Num> &transform,
        const Vector3<real_Num> &worldPosition, f32 &worldHeight );
    WPCore_API bool intersectTerrainHeightfieldForAuthoring(
        const TerrainData &data, const Transform3<real_Num> &transform, const Ray3F &ray,
        Vector3<real_Num> &worldHit );
    WPCore_API bool buildTerrainMeshData( const TerrainData &data, TerrainMeshData &output,
                                          String &error );
}  // namespace workphone::render

#endif
