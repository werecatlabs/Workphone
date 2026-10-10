#ifndef WPTerrainData_h__
#define WPTerrainData_h__

#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Math/Ray3.hpp>
#include <memory>

namespace workphone::render
{
    inline constexpr u32 terrainDataFormatVersion = 1;
    inline constexpr u64 terrainMaximumSamples = 4ull * 1024ull * 1024ull;
    inline constexpr u32 terrainMaximumDimension = 8193;

    /** Version-one rectangular heightfield. Heights are unscaled row-major samples:
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
    WPCore_API bool buildTerrainMeshData( const TerrainData &data, TerrainMeshData &output,
                                          String &error );
}  // namespace workphone::render

#endif
