#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <algorithm>
#include <cJSON.h>
#include <cmath>
#include <limits>

namespace workphone::render
{
    namespace
    {
        bool fail( String &error, const char *message )
        {
            error = message;
            return false;
        }

        bool validShape( const TerrainData &data )
        {
            const auto width = data.dimensions.x;
            const auto depth = data.dimensions.y;
            if( width < 2 || depth < 2 || width > terrainMaximumDimension ||
                depth > terrainMaximumDimension )
                return false;
            const u64 count = u64( width ) * u64( depth );
            return count <= terrainMaximumSamples && count == data.heights.size() &&
                   std::isfinite( data.spacing.x ) && data.spacing.x > 0 &&
                   std::isfinite( data.spacing.y ) && data.spacing.y > 0 &&
                   std::isfinite( data.origin.x ) && std::isfinite( data.origin.y ) &&
                   std::isfinite( data.heightScale ) &&
                   std::isfinite( data.origin.x + ( width - 1 ) * data.spacing.x ) &&
                   std::isfinite( data.origin.y + ( depth - 1 ) * data.spacing.y );
        }

        using CalculationVector = Vector3<f64>;

        CalculationVector position( const TerrainData &data, s32 x, s32 z )
        {
            return { f64( data.origin.x ) + f64( x ) * data.spacing.x,
                     f64( data.heights[size_t( z ) * data.dimensions.x + x] ) * data.heightScale,
                     f64( data.origin.y ) + f64( z ) * data.spacing.y };
        }

        Quaternion<f64> rotation( const Transform3<real_Num> &transform )
        {
            const auto &q = transform.getOrientation();
            return { f64( q.w ), f64( q.x ), f64( q.y ), f64( q.z ) };
        }

        CalculationVector worldPoint( const Transform3<real_Num> &transform,
                                      const CalculationVector &local )
        {
            // Match Matrix4::makeTransform used by the renderer. Transform3's legacy
            // transformPoint applies scale after translation and is not suitable here.
            const auto &p = transform.getPosition();
            const auto &s = transform.getScale();
            return CalculationVector( p.x, p.y, p.z ) +
                   rotation( transform ) *
                       CalculationVector( local.x * s.x, local.y * s.y, local.z * s.z );
        }

        CalculationVector localPoint( const Transform3<real_Num> &transform,
                                      const CalculationVector &world )
        {
            const auto &p = transform.getPosition();
            const auto &s = transform.getScale();
            const auto rotated =
                rotation( transform ).inverse() * ( world - CalculationVector( p.x, p.y, p.z ) );
            return { rotated.x / s.x, rotated.y / s.y, rotated.z / s.z };
        }

        Vector2<f64> footprintTolerance( const TerrainData &data, const Transform3<real_Num> &transform,
                                         f64 worldX, f64 worldZ )
        {
            // World vertices are rendered in f32. Inverse rotation/scale can put an
            // exact rendered edge a few ulps outside the authored grid.
            const auto &p = transform.getPosition();
            const auto &s = transform.getScale();
            const auto magnitude = std::max( { std::abs( worldX ), std::abs( worldZ ),
                                               std::abs( f64( p.x ) ), std::abs( f64( p.z ) ) } );
            constexpr f64 roundoff = 8.0 * std::numeric_limits<f32>::epsilon();
            return { roundoff * std::max( { 1.0, f64( data.dimensions.x - 1 ),
                                            std::abs( f64( data.origin.x ) / data.spacing.x ),
                                            magnitude / ( f64( s.x ) * data.spacing.x ) } ),
                     roundoff * std::max( { 1.0, f64( data.dimensions.y - 1 ),
                                            std::abs( f64( data.origin.y ) / data.spacing.y ),
                                            magnitude / ( f64( s.z ) * data.spacing.y ) } ) };
        }

        bool rayTriangle( const CalculationVector &origin, const CalculationVector &direction,
                          const CalculationVector &a, const CalculationVector &b,
                          const CalculationVector &c, f64 &distance )
        {
            const auto e1 = b - a;
            const auto e2 = c - a;
            const auto p = direction.crossProduct( e2 );
            const auto determinant = e1.dotProduct( p );
            if( !std::isfinite( determinant ) || determinant == 0 )
                return false;
            const auto inv = 1.0 / determinant;
            const auto s = origin - a;
            const auto u = s.dotProduct( p ) * inv;
            constexpr f64 tolerance = 1.e-7;
            if( u < -tolerance || u > 1 + tolerance )
                return false;
            const auto q = s.crossProduct( e1 );
            const auto v = direction.dotProduct( q ) * inv;
            if( v < -tolerance || u + v > 1 + tolerance )
                return false;
            distance = e2.dotProduct( q ) * inv;
            return std::isfinite( distance ) && distance >= -tolerance;
        }
    }  // namespace

    Vector4F normalizeTerrainWeights( const Vector4F &weights )
    {
        Vector4F result;
        double sum = 0;
        for( size_t i = 0; i < 4; ++i )
        {
            result[i] = std::isfinite( weights[i] ) ? std::max( 0.0f, weights[i] ) : 0;
            sum += result[i];
        }
        if( sum <= 0 )
            return Vector4F( 1, 0, 0, 0 );
        for( size_t i = 0; i < 4; ++i )
            result[i] = f32( result[i] / sum );
        return result;
    }

    bool terrainCellIsHole( const TerrainData &data, s32 x, s32 z )
    {
        if( x < 0 || z < 0 || x >= data.dimensions.x - 1 || z >= data.dimensions.y - 1 )
            return true;
        const size_t index = size_t( z ) * ( data.dimensions.x - 1 ) + x;
        return !data.holes.empty() && ( index >= data.holes.size() || data.holes[index] != 0 );
    }

    bool validateTerrainData( const TerrainData &data, String &error )
    {
        error.clear();
        if( !validShape( data ) )
            return fail( error,
                         "Terrain requires bounded rectangular dimensions, matching samples, finite "
                         "origin/scale and positive spacing" );
        for( const auto value : data.heights )
            if( !std::isfinite( value ) || !std::isfinite( value * data.heightScale ) )
                return fail( error, "Terrain samples and scaled heights must be finite" );
        const auto inRange = []( f32 value, f32 low, f32 high ) {
            return std::isfinite( value ) && value >= low && value <= high;
        };
        const auto &appearance = data.appearance;
        if( u32( appearance.projection ) > 1 || !inRange( appearance.triplanarSharpness, 1, 16 ) ||
            !inRange( appearance.heightTransition, .001f, 1 ) ||
            !inRange( appearance.detailStrength, 0, 16 ) ||
            !std::isfinite( appearance.detailUvScale.x ) || appearance.detailUvScale.x <= 0 ||
            !std::isfinite( appearance.detailUvScale.y ) || appearance.detailUvScale.y <= 0 ||
            !std::isfinite( appearance.macroScale ) || appearance.macroScale <= 0 ||
            !inRange( appearance.macroStrength, 0, 1 ) || !inRange( appearance.wetness, 0, 1 ) ||
            !inRange( appearance.wetDarkening, 0, 1 ) || !inRange( appearance.wetRoughness, .045f, 1 ) ||
            !inRange( appearance.snow, 0, 1 ) || !inRange( appearance.snowRoughness, .045f, 1 ) )
            return fail( error, "Terrain appearance settings exceed their finite supported ranges" );
        if( appearance.detailNormal.size() > 4096 ||
            appearance.detailNormal.find( '\0' ) != String::npos )
            return fail( error, "Terrain detail normal identifier is invalid" );
        for( size_t channel = 0; channel < 4; ++channel )
            if( !inRange( appearance.macroTint[channel], 0, 1 ) ||
                !inRange( appearance.snowTint[channel], 0, 1 ) )
                return fail( error, "Terrain appearance tints must be in [0,1]" );
        if( !data.surfaceMasks.empty() && data.surfaceMasks.size() != data.heights.size() )
            return fail( error, "Terrain wetness/snow masks must match sample dimensions" );
        for( const auto &mask : data.surfaceMasks )
            if( !inRange( mask.x, 0, 1 ) || !inRange( mask.y, 0, 1 ) )
                return fail( error, "Terrain wetness/snow masks must be finite in [0,1]" );
        f64 maximumCoordinate = std::max(
            { std::abs( f64( data.origin.x ) ), std::abs( f64( data.origin.y ) ),
              std::abs( f64( data.origin.x ) + ( data.dimensions.x - 1 ) * f64( data.spacing.x ) ),
              std::abs( f64( data.origin.y ) + ( data.dimensions.y - 1 ) * f64( data.spacing.y ) ) } );
        if( appearance.macroStrength > 0 &&
            maximumCoordinate * appearance.macroScale > std::numeric_limits<f32>::max() )
            return fail( error, "Terrain macro coordinates exceed the shader range" );
        if( appearance.projection == TerrainProjection::Triplanar )
            for( const auto value : data.heights )
                maximumCoordinate =
                    std::max( maximumCoordinate, std::abs( f64( value ) * data.heightScale ) );
        if( !appearance.detailNormal.empty() &&
            maximumCoordinate * std::max( appearance.detailUvScale.x, appearance.detailUvScale.y ) >
                std::numeric_limits<f32>::max() )
            return fail( error, "Terrain detail coordinates exceed the shader range" );
        if( !data.layerWeights.empty() && data.layerWeights.size() != data.heights.size() )
            return fail( error, "Terrain control weights must match sample dimensions" );
        for( const auto &weights : data.layerWeights )
        {
            double sum = 0;
            for( size_t i = 0; i < 4; ++i )
            {
                if( !std::isfinite( weights[i] ) || weights[i] < 0 || weights[i] > 1 )
                    return fail( error, "Terrain weights must be finite and between zero and one" );
                sum += weights[i];
            }
            if( sum != 0 && std::abs( sum - 1 ) > 1.e-5 )
                return fail( error, "Terrain weights must sum to one (zero falls back to layer zero)" );
        }
        if( !data.holes.empty() &&
            data.holes.size() != size_t( data.dimensions.x - 1 ) * ( data.dimensions.y - 1 ) )
            return fail( error, "Terrain holes must match cell dimensions" );
        for( const auto hole : data.holes )
            if( hole > 1 )
                return fail( error, "Terrain hole values must be zero or one" );
        for( const auto &layer : data.layers )
        {
            for( const auto *resource : { &layer.albedo, &layer.normal, &layer.orm } )
                if( resource->size() > 4096 || resource->find( '\0' ) != String::npos )
                    return fail( error,
                                 "Terrain texture identifiers must be bounded and contain no NUL" );
            if( !std::isfinite( layer.uvScale.x ) || !std::isfinite( layer.uvScale.y ) ||
                layer.uvScale.x <= 0 || layer.uvScale.y <= 0 || !std::isfinite( layer.roughness ) ||
                layer.roughness < 0 || layer.roughness > 1 || !std::isfinite( layer.metallic ) ||
                layer.metallic < 0 || layer.metallic > 1 || !std::isfinite( layer.normalStrength ) ||
                layer.normalStrength < 0 || layer.normalStrength > 16 ||
                !inRange( layer.heightScale, 0, 4 ) || !inRange( layer.heightBias, -1, 1 ) )
                return fail( error,
                             "Terrain material scalars are outside their finite supported ranges" );
            for( size_t i = 0; i < 4; ++i )
                if( !std::isfinite( layer.tint[i] ) || layer.tint[i] < 0 || layer.tint[i] > 1 )
                    return fail( error, "Terrain tint must be finite and between zero and one" );
            if( appearance.projection == TerrainProjection::Triplanar &&
                maximumCoordinate * std::max( layer.uvScale.x, layer.uvScale.y ) >
                    std::numeric_limits<f32>::max() )
                return fail( error, "Terrain triplanar coordinates exceed the shader range" );
            // The material shader multiplies local-metre f32 positions by the
            // layer tiling. Finite inputs alone do not ensure finite UVs.
            for( const auto axis : { 0, 1 } )
            {
                const auto origin = axis ? data.origin.y : data.origin.x;
                const auto spacing = axis ? data.spacing.y : data.spacing.x;
                const auto size = axis ? data.dimensions.y : data.dimensions.x;
                const auto endpoint = f32( f64( origin ) + f64( size - 1 ) * spacing );
                const auto scale = axis ? layer.uvScale.y : layer.uvScale.x;
                const auto magnitude =
                    std::max( std::abs( f64( origin ) ), std::abs( f64( endpoint ) ) );
                if( magnitude * scale > std::numeric_limits<f32>::max() )
                    return fail( error, "Terrain layer tiling overflows renderer UV coordinates" );
            }
        }
        // Native vertices are f32 even in a double-precision engine build. Ensure
        // every adjacent coordinate remains distinct after its final conversion.
        for( const auto axis : { 0, 1 } )
        {
            const auto origin = axis ? data.origin.y : data.origin.x;
            const auto spacing = axis ? data.spacing.y : data.spacing.x;
            const auto size = axis ? data.dimensions.y : data.dimensions.x;
            auto previous = origin;
            for( s32 i = 1; i < size; ++i )
            {
                const auto coordinate = f32( f64( origin ) + f64( i ) * spacing );
                if( coordinate <= previous )
                    return fail( error, "Terrain spacing collapses adjacent renderer coordinates" );
                previous = coordinate;
            }
        }
        return true;
    }

    bool validateTerrainTransform( const Transform3<real_Num> &transform, String &error )
    {
        error.clear();
        if( !transform.isSane() )
            return fail( error, "Terrain transform must be finite and nondegenerate" );
        const auto &p = transform.getPosition();
        const auto &s = transform.getScale();
        const auto &q = transform.getOrientation();
        for( const auto value : { p.x, p.y, p.z, s.x, s.y, s.z, q.w, q.x, q.y, q.z } )
            if( !std::isfinite( value ) || !std::isfinite( f32( value ) ) )
                return fail( error, "Terrain transform values must be finite" );
        if( s.x <= 0 || s.y <= 0 || s.z <= 0 || f32( s.x ) <= 0 || f32( s.y ) <= 0 || f32( s.z ) <= 0 )
            return fail( error, "Terrain transform scale must be positive and nonzero" );
        const auto norm = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
        if( std::abs( norm - 1 ) > real_Num( 1.e-5 ) || std::abs( q.x ) > real_Num( 1.e-7 ) ||
            std::abs( q.z ) > real_Num( 1.e-7 ) )
            return fail( error,
                         "Terrain supports normalized yaw rotation; pitch and roll are unavailable" );
        return true;
    }

    bool validateTerrainPlacement( const TerrainData &data, const Transform3<real_Num> &transform,
                                   String &error )
    {
        if( !validateTerrainData( data, error ) || !validateTerrainTransform( transform, error ) )
            return false;
        const auto extrema = std::minmax_element( data.heights.begin(), data.heights.end() );
        for( const auto x : { s32( 0 ), data.dimensions.x - 1 } )
            for( const auto z : { s32( 0 ), data.dimensions.y - 1 } )
                for( const auto y : { *extrema.first, *extrema.second } )
                {
                    auto local = position( data, x, z );
                    local.y = f64( y ) * data.heightScale;
                    const auto world = worldPoint( transform, local );
                    if( !std::isfinite( f32( world.x ) ) || !std::isfinite( f32( world.y ) ) ||
                        !std::isfinite( f32( world.z ) ) )
                        return fail( error,
                                     "Terrain world coordinates exceed the renderer's finite range" );
                }
        return true;
    }

    bool sampleTerrainHeight( const TerrainData &data, const Transform3<real_Num> &transform,
                              const Vector3<real_Num> &worldPosition, f32 &worldHeight )
    {
        worldHeight = 0;
        String error;
        if( !validShape( data ) || !validateTerrainTransform( transform, error ) ||
            !std::isfinite( worldPosition.x ) || !std::isfinite( worldPosition.y ) ||
            !std::isfinite( worldPosition.z ) )
            return false;
        const auto local =
            localPoint( transform, { worldPosition.x, worldPosition.y, worldPosition.z } );
        auto x = ( local.x - data.origin.x ) / data.spacing.x;
        auto z = ( local.z - data.origin.y ) / data.spacing.y;
        const auto tolerance = footprintTolerance( data, transform, worldPosition.x, worldPosition.z );
        if( !std::isfinite( x ) || !std::isfinite( z ) || x < -tolerance.x || z < -tolerance.y ||
            x > data.dimensions.x - 1 + tolerance.x || z > data.dimensions.y - 1 + tolerance.y )
            return false;
        x = std::clamp( x, 0.0, f64( data.dimensions.x - 1 ) );
        z = std::clamp( z, 0.0, f64( data.dimensions.y - 1 ) );
        const auto ix = std::min( static_cast<s32>( x ), data.dimensions.x - 2 );
        const auto iz = std::min( static_cast<s32>( z ), data.dimensions.y - 2 );
        if( terrainCellIsHole( data, ix, iz ) )
            return false;
        const auto fx = x - ix;
        const auto fz = z - iz;
        const size_t i = size_t( iz ) * data.dimensions.x + ix;
        const auto a = f64( data.heights[i] );
        const auto b = f64( data.heights[i + 1] );
        const auto c = f64( data.heights[i + data.dimensions.x] );
        const auto d = f64( data.heights[i + data.dimensions.x + 1] );
        const auto sample = fx + fz <= 1 ? a + fx * ( b - a ) + fz * ( c - a )
                                         : d + ( 1 - fx ) * ( c - d ) + ( 1 - fz ) * ( b - d );
        const auto world = worldPoint( transform, { local.x, sample * data.heightScale, local.z } );
        worldHeight = static_cast<f32>( world.y );
        return std::isfinite( worldHeight );
    }

    bool intersectTerrain( const TerrainData &data, const Transform3<real_Num> &transform,
                           const Ray3F &ray, Vector3<real_Num> &worldHit )
    {
        worldHit = Vector3<real_Num>::zero();
        String error;
        if( !validShape( data ) || !validateTerrainTransform( transform, error ) )
            return false;
        const auto ro = ray.getOrigin();
        const auto rd = ray.getDirection();
        for( const auto value : { ro.x, ro.y, ro.z, rd.x, rd.y, rd.z } )
            if( !std::isfinite( value ) )
                return false;
        auto origin = localPoint( transform, { ro.x, ro.y, ro.z } );
        const auto edgeTolerance = footprintTolerance( data, transform, ro.x, ro.z );
        const auto snapEdge = []( f64 value, f64 low, f64 high, f64 epsilon ) {
            if( std::abs( value - low ) <= epsilon )
                return low;
            if( std::abs( value - high ) <= epsilon )
                return high;
            return value;
        };
        origin.x = snapEdge( origin.x, data.origin.x,
                             f64( data.origin.x ) + ( data.dimensions.x - 1 ) * f64( data.spacing.x ),
                             edgeTolerance.x * data.spacing.x );
        origin.z = snapEdge( origin.z, data.origin.y,
                             f64( data.origin.y ) + ( data.dimensions.y - 1 ) * f64( data.spacing.y ),
                             edgeTolerance.y * data.spacing.y );
        const auto rotated = rotation( transform ).inverse() * CalculationVector( rd.x, rd.y, rd.z );
        const auto scale = transform.getScale();
        // Transform3::inverseTransformVector intentionally rotates only; a geometric
        // ray direction must also undo the nonuniform object scale.
        const CalculationVector direction( rotated.x / scale.x, rotated.y / scale.y,
                                           rotated.z / scale.z );
        const auto length = std::hypot( direction.x, direction.y, direction.z );
        if( !std::isfinite( length ) || length <= 0 )
            return false;
        f64 enter = 0;
        f64 leave = std::numeric_limits<f64>::infinity();
        const auto slab = [&]( f64 o, f64 d, f64 low, f64 high ) {
            if( d == 0 )
                return o >= low && o <= high;
            auto a = ( low - o ) / d;
            auto b = ( high - o ) / d;
            if( a > b )
                std::swap( a, b );
            enter = std::max( enter, a );
            leave = std::min( leave, b );
            return leave >= enter;
        };
        if( !slab( origin.x, direction.x, data.origin.x,
                   f64( data.origin.x ) + ( data.dimensions.x - 1 ) * f64( data.spacing.x ) ) ||
            !slab( origin.z, direction.z, data.origin.y,
                   f64( data.origin.y ) + ( data.dimensions.y - 1 ) * f64( data.spacing.y ) ) )
            return false;
        const auto entry = origin + direction * enter;
        if( !std::isfinite( enter ) || !std::isfinite( entry.x ) || !std::isfinite( entry.z ) )
            return false;
        s32 x = static_cast<s32>( std::clamp( std::floor( ( entry.x - data.origin.x ) / data.spacing.x ),
                                              0.0, f64( data.dimensions.x - 2 ) ) );
        s32 z = static_cast<s32>( std::clamp( std::floor( ( entry.z - data.origin.y ) / data.spacing.y ),
                                              0.0, f64( data.dimensions.y - 2 ) ) );
        const s32 stepX = direction.x > 0 ? 1 : direction.x < 0 ? -1 : 0;
        const s32 stepZ = direction.z > 0 ? 1 : direction.z < 0 ? -1 : 0;
        const auto infinity = std::numeric_limits<f64>::infinity();
        auto nextX = stepX ? ( f64( data.origin.x ) +
                               ( x + ( stepX > 0 ? 1 : 0 ) ) * f64( data.spacing.x ) - origin.x ) /
                                 direction.x
                           : infinity;
        auto nextZ = stepZ ? ( f64( data.origin.y ) +
                               ( z + ( stepZ > 0 ? 1 : 0 ) ) * f64( data.spacing.y ) - origin.z ) /
                                 direction.z
                           : infinity;
        const auto deltaX = stepX ? std::abs( f64( data.spacing.x ) / direction.x ) : infinity;
        const auto deltaZ = stepZ ? std::abs( f64( data.spacing.y ) / direction.z ) : infinity;
        // Two-dimensional DDA visits at most width+depth cells. Vertical rays test one.
        for( s32 count = 0; count <= data.dimensions.x + data.dimensions.y; ++count )
        {
            if( x < 0 || x >= data.dimensions.x - 1 || z < 0 || z >= data.dimensions.y - 1 )
                break;
            const auto a = position( data, x, z );
            const auto b = position( data, x + 1, z );
            const auto c = position( data, x, z + 1 );
            const auto d = position( data, x + 1, z + 1 );
            f64 first = 0, second = 0;
            const bool solid = !terrainCellIsHole( data, x, z );
            const bool hitFirst = solid && rayTriangle( origin, direction, a, c, b, first );
            const bool hitSecond = solid && rayTriangle( origin, direction, b, c, d, second );
            const auto distance = hitFirst && hitSecond ? std::min( first, second )
                                  : hitFirst            ? first
                                                        : second;
            const auto end = std::min( { nextX, nextZ, leave } );
            const auto tolerance = 1.e-6 * std::max( 1.0, std::abs( enter ) );
            if( ( hitFirst || hitSecond ) && distance >= enter - tolerance &&
                distance <= end + tolerance )
            {
                const auto hit = worldPoint( transform, origin + direction * std::max( distance, 0.0 ) );
                worldHit = { real_Num( hit.x ), real_Num( hit.y ), real_Num( hit.z ) };
                return std::isfinite( worldHit.x ) && std::isfinite( worldHit.y ) &&
                       std::isfinite( worldHit.z );
            }
            if( end >= leave || !std::isfinite( end ) )
                break;
            const bool advanceX = nextX <= nextZ;
            const bool advanceZ = nextZ <= nextX;
            if( advanceX )
            {
                x += stepX;
                nextX += deltaX;
            }
            if( advanceZ )
            {
                z += stepZ;
                nextZ += deltaZ;
            }
            enter = end;
        }
        return false;
    }

    bool buildTerrainMeshData( const TerrainData &data, TerrainMeshData &output, String &error )
    {
        output = {};
        if( !validateTerrainData( data, error ) )
            return false;
        try
        {
            TerrainMeshData result;
            const auto width = data.dimensions.x;
            const auto depth = data.dimensions.y;
            result.positions.reserve( data.heights.size() );
            result.normals.reserve( data.heights.size() );
            result.uvs.reserve( data.heights.size() );
            result.indices.reserve( size_t( width - 1 ) * ( depth - 1 ) * 6 );
            for( s32 z = 0; z < depth; ++z )
                for( s32 x = 0; x < width; ++x )
                {
                    const auto p = position( data, x, z );
                    const auto left = position( data, std::max( x - 1, 0 ), z );
                    const auto right = position( data, std::min( x + 1, width - 1 ), z );
                    const auto down = position( data, x, std::max( z - 1, 0 ) );
                    const auto up = position( data, x, std::min( z + 1, depth - 1 ) );
                    auto n = ( up - down ).crossProduct( right - left );
                    const auto length = std::hypot( n.x, n.y, n.z );
                    if( !std::isfinite( length ) || length <= 0 )
                        return fail( error, "Terrain normals exceed supported numeric range" );
                    n /= length;
                    result.positions.push_back( { f32( p.x ), f32( p.y ), f32( p.z ) } );
                    result.normals.push_back( { f32( n.x ), f32( n.y ), f32( n.z ) } );
                    result.uvs.push_back( { f32( x ) / ( width - 1 ), f32( z ) / ( depth - 1 ) } );
                }
            for( s32 z = 0; z < depth - 1; ++z )
                for( s32 x = 0; x < width - 1; ++x )
                {
                    if( terrainCellIsHole( data, x, z ) )
                        continue;
                    const auto a = u32( z * width + x );
                    const auto b = a + 1;
                    const auto c = a + u32( width );
                    const auto d = c + 1;
                    for( const auto index : { a, c, b, b, c, d } )
                        result.indices.push_back( index );
                }
            output = std::move( result );
            return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
    }
    String serializeTerrainData( const TerrainData &value )
    {
        String error;
        if( !validateTerrainData( value, error ) )
            return {};
        const auto data = &value;
        using Json = std::unique_ptr<cJSON, decltype( &cJSON_Delete )>;
        Json root( cJSON_CreateObject(), cJSON_Delete );
        const float spacing[] = { data->spacing.x, data->spacing.y };
        const float origin[] = { data->origin.x, data->origin.y };
        if( !root || !cJSON_AddStringToObject( root.get(), "format", "workphone.terrain" ) ||
            !cJSON_AddNumberToObject( root.get(), "version", render::terrainDataFormatVersion ) ||
            !cJSON_AddNumberToObject( root.get(), "width", data->dimensions.x ) ||
            !cJSON_AddNumberToObject( root.get(), "depth", data->dimensions.y ) ||
            !cJSON_AddNumberToObject( root.get(), "heightScale", data->heightScale ) )
            return {};
        const auto addArray = [&]( const char *name, const float *values, int count ) {
            Json array( count == 0 ? cJSON_CreateArray() : cJSON_CreateFloatArray( values, count ),
                        cJSON_Delete );
            if( !array || !cJSON_AddItemToObject( root.get(), name, array.get() ) )
                return false;
            array.release();
            return true;
        };
        if( !addArray( "spacing", spacing, 2 ) || !addArray( "origin", origin, 2 ) ||
            !addArray( "heights", data->heights.data(), static_cast<int>( data->heights.size() ) ) )
            return {};
        Array<f32> weights;
        weights.reserve( data->layerWeights.size() * 4 );
        for( const auto &weight : data->layerWeights )
            for( size_t channel = 0; channel < 4; ++channel )
                weights.push_back( weight[channel] );
        Array<f32> holes;
        holes.reserve( data->holes.size() );
        for( auto hole : data->holes )
            holes.push_back( hole );
        if( !addArray( "weights", weights.data(), static_cast<int>( weights.size() ) ) ||
            !addArray( "holes", holes.data(), static_cast<int>( holes.size() ) ) )
            return {};
        Array<f32> masks;
        masks.reserve( data->surfaceMasks.size() * 2 );
        for( const auto &mask : data->surfaceMasks )
        {
            masks.push_back( mask.x );
            masks.push_back( mask.y );
        }
        if( !addArray( "surfaceMasks", masks.data(), static_cast<int>( masks.size() ) ) )
            return {};
        auto appearance = cJSON_AddObjectToObject( root.get(), "appearance" );
        const auto &a = data->appearance;
        if( !appearance || !cJSON_AddNumberToObject( appearance, "projection", u32( a.projection ) ) ||
            !cJSON_AddBoolToObject( appearance, "heightBlend", a.heightBlend ) ||
            !cJSON_AddStringToObject( appearance, "detailNormal", a.detailNormal.c_str() ) ||
            !cJSON_AddNumberToObject( appearance, "macroSeed", a.macroSeed ) )
            return {};
        for( const auto &entry :
             { std::make_pair( "triplanarSharpness", a.triplanarSharpness ),
               std::make_pair( "heightTransition", a.heightTransition ),
               std::make_pair( "detailStrength", a.detailStrength ),
               std::make_pair( "macroScale", a.macroScale ),
               std::make_pair( "macroStrength", a.macroStrength ),
               std::make_pair( "wetness", a.wetness ), std::make_pair( "wetDarkening", a.wetDarkening ),
               std::make_pair( "wetRoughness", a.wetRoughness ), std::make_pair( "snow", a.snow ),
               std::make_pair( "snowRoughness", a.snowRoughness ) } )
            if( !cJSON_AddNumberToObject( appearance, entry.first, entry.second ) )
                return {};
        const auto addAppearanceArray = [&]( const char *name, const f32 *values, int count ) {
            Json array( cJSON_CreateFloatArray( values, count ), cJSON_Delete );
            if( !array || !cJSON_AddItemToObject( appearance, name, array.get() ) )
                return false;
            array.release();
            return true;
        };
        const f32 detailUv[] = { a.detailUvScale.x, a.detailUvScale.y };
        const f32 macroTint[] = { a.macroTint.x, a.macroTint.y, a.macroTint.z, a.macroTint.w };
        const f32 snowTint[] = { a.snowTint.x, a.snowTint.y, a.snowTint.z, a.snowTint.w };
        if( !addAppearanceArray( "detailUvScale", detailUv, 2 ) ||
            !addAppearanceArray( "macroTint", macroTint, 4 ) ||
            !addAppearanceArray( "snowTint", snowTint, 4 ) )
            return {};
        auto layers = cJSON_AddArrayToObject( root.get(), "layers" );
        if( !layers )
            return {};
        for( const auto &layer : data->layers )
        {
            Json item( cJSON_CreateObject(), cJSON_Delete );
            const float uv[] = { layer.uvScale.x, layer.uvScale.y };
            const float tint[] = { layer.tint.x, layer.tint.y, layer.tint.z, layer.tint.w };
            if( !item || !cJSON_AddStringToObject( item.get(), "albedo", layer.albedo.c_str() ) ||
                !cJSON_AddStringToObject( item.get(), "normal", layer.normal.c_str() ) ||
                !cJSON_AddStringToObject( item.get(), "orm", layer.orm.c_str() ) ||
                !cJSON_AddNumberToObject( item.get(), "roughness", layer.roughness ) ||
                !cJSON_AddNumberToObject( item.get(), "metallic", layer.metallic ) ||
                !cJSON_AddNumberToObject( item.get(), "normalStrength", layer.normalStrength ) ||
                !cJSON_AddNumberToObject( item.get(), "heightScale", layer.heightScale ) ||
                !cJSON_AddNumberToObject( item.get(), "heightBias", layer.heightBias ) )
                return {};
            Json uvArray( cJSON_CreateFloatArray( uv, 2 ), cJSON_Delete );
            Json tintArray( cJSON_CreateFloatArray( tint, 4 ), cJSON_Delete );
            if( !uvArray || !tintArray ||
                !cJSON_AddItemToObject( item.get(), "uvScale", uvArray.get() ) )
                return {};
            uvArray.release();
            if( !cJSON_AddItemToObject( item.get(), "tint", tintArray.get() ) )
                return {};
            tintArray.release();
            if( !cJSON_AddItemToArray( layers, item.get() ) )
                return {};
            item.release();
        }
        std::unique_ptr<char, decltype( &cJSON_free )> json( cJSON_PrintUnformatted( root.get() ),
                                                             cJSON_free );
        if( !json )
            return {};
        const auto length = std::char_traits<char>::length( json.get() );
        // Every emitted source asset must satisfy the bounded reader contract.
        // Large painted worlds are persisted as tiles rather than unloadable JSON.
        return length <= 128ull * 1024 * 1024 ? String( json.get(), length ) : String();
    }

    bool deserializeTerrainData( const String &json, TerrainData &output, String &error )
    {
        // Bound input before the parser allocates its object tree. No partial changes.
        if( json.empty() || json.size() > 128ull * 1024ull * 1024ull ||
            json.find( "\\u0000" ) != String::npos )
        {
            error = "Terrain JSON is empty or exceeds the 128 MiB limit.";
            return false;
        }
        try
        {
            using Json = std::unique_ptr<cJSON, decltype( &cJSON_Delete )>;
            const char *end = nullptr;
            Json root( cJSON_ParseWithLengthOpts( json.c_str(), json.size() + 1, &end, 1 ),
                       cJSON_Delete );
            if( !root || !cJSON_IsObject( root.get() ) || end != json.c_str() + json.size() )
            {
                error = "Invalid terrain JSON.";
                return false;
            }
            const auto field = [&]( const char *name ) {
                return cJSON_GetObjectItemCaseSensitive( root.get(), name );
            };
            const auto number = []( const cJSON *value ) {
                return cJSON_IsNumber( value ) && std::isfinite( value->valuedouble ) &&
                       std::abs( value->valuedouble ) <= std::numeric_limits<float>::max();
            };
            const auto format = field( "format" );
            const auto version = field( "version" );
            const auto width = field( "width" );
            const auto depth = field( "depth" );
            const auto scale = field( "heightScale" );
            const auto spacing = field( "spacing" );
            const auto origin = field( "origin" );
            const auto heights = field( "heights" );
            if( !cJSON_IsString( format ) || String( format->valuestring ) != "workphone.terrain" ||
                !number( version ) ||
                ( version->valuedouble != 1 && version->valuedouble != 2 &&
                  version->valuedouble != terrainDataFormatVersion ) ||
                !number( width ) || !number( depth ) || width->valuedouble < 2 ||
                depth->valuedouble < 2 || width->valuedouble > render::terrainMaximumDimension ||
                depth->valuedouble > render::terrainMaximumDimension ||
                std::floor( width->valuedouble ) != width->valuedouble ||
                std::floor( depth->valuedouble ) != depth->valuedouble ||
                width->valuedouble * depth->valuedouble > render::terrainMaximumSamples ||
                !number( scale ) || !cJSON_IsArray( spacing ) || cJSON_GetArraySize( spacing ) != 2 ||
                !cJSON_IsArray( origin ) || cJSON_GetArraySize( origin ) != 2 ||
                !cJSON_IsArray( heights ) )
            {
                error = "Unsupported terrain schema, dimensions or metadata.";
                return false;
            }
            render::TerrainData data;
            data.dimensions = Vector2I( static_cast<s32>( width->valuedouble ),
                                        static_cast<s32>( depth->valuedouble ) );
            const auto count = static_cast<size_t>( data.dimensions.x ) * data.dimensions.y;
            if( static_cast<size_t>( cJSON_GetArraySize( heights ) ) != count ||
                !number( cJSON_GetArrayItem( spacing, 0 ) ) ||
                !number( cJSON_GetArrayItem( spacing, 1 ) ) ||
                !number( cJSON_GetArrayItem( origin, 0 ) ) ||
                !number( cJSON_GetArrayItem( origin, 1 ) ) )
            {
                error = "Terrain arrays have invalid sizes or numbers.";
                return false;
            }
            data.spacing = Vector2F( static_cast<f32>( cJSON_GetArrayItem( spacing, 0 )->valuedouble ),
                                     static_cast<f32>( cJSON_GetArrayItem( spacing, 1 )->valuedouble ) );
            data.origin = Vector2F( static_cast<f32>( cJSON_GetArrayItem( origin, 0 )->valuedouble ),
                                    static_cast<f32>( cJSON_GetArrayItem( origin, 1 )->valuedouble ) );
            data.heightScale = static_cast<f32>( scale->valuedouble );
            data.heights.clear();
            data.heights.reserve( count );
            for( auto value = heights->child; value; value = value->next )
            {
                if( !number( value ) )
                {
                    error = "Terrain contains a nonfinite or invalid sample.";
                    return false;
                }
                data.heights.push_back( static_cast<f32>( value->valuedouble ) );
            }
            if( version->valuedouble >= 2 )
            {
                const auto weights = field( "weights" ), holes = field( "holes" ),
                           layers = field( "layers" );
                const auto holeCount = size_t( data.dimensions.x - 1 ) * ( data.dimensions.y - 1 );
                if( !cJSON_IsArray( weights ) || !cJSON_IsArray( holes ) || !cJSON_IsArray( layers ) ||
                    cJSON_GetArraySize( layers ) != 4 ||
                    ( cJSON_GetArraySize( weights ) != 0 &&
                      size_t( cJSON_GetArraySize( weights ) ) != count * 4 ) ||
                    ( cJSON_GetArraySize( holes ) != 0 &&
                      size_t( cJSON_GetArraySize( holes ) ) != holeCount ) )
                    return fail( error, "Terrain v2 requires four layers and matching weights/holes" );
                Vector4F weight;
                size_t channel = 0;
                for( auto item = weights->child; item; item = item->next )
                {
                    if( !number( item ) )
                        return fail( error, "Terrain contains invalid weights" );
                    weight[channel++] = f32( item->valuedouble );
                    if( channel == 4 )
                    {
                        data.layerWeights.push_back( weight );
                        channel = 0;
                    }
                }
                for( auto item = holes->child; item; item = item->next )
                {
                    if( !number( item ) || ( item->valuedouble != 0 && item->valuedouble != 1 ) )
                        return fail( error, "Terrain contains invalid hole values" );
                    data.holes.push_back( u8( item->valuedouble ) );
                }
                for( size_t i = 0; i < 4; ++i )
                {
                    const auto item = cJSON_GetArrayItem( layers, int( i ) );
                    const auto get = [&]( const char *name ) {
                        return cJSON_GetObjectItemCaseSensitive( item, name );
                    };
                    auto &layer = data.layers[i];
                    for( const auto &slot : { std::make_pair( "albedo", &layer.albedo ),
                                              std::make_pair( "normal", &layer.normal ),
                                              std::make_pair( "orm", &layer.orm ) } )
                    {
                        auto value = get( slot.first );
                        if( !cJSON_IsString( value ) )
                            return fail( error, "Terrain layer texture ID is missing" );
                        *slot.second = value->valuestring;
                    }
                    for( const auto &slot :
                         { std::make_pair( "roughness", &layer.roughness ),
                           std::make_pair( "metallic", &layer.metallic ),
                           std::make_pair( "normalStrength", &layer.normalStrength ) } )
                    {
                        auto value = get( slot.first );
                        if( !number( value ) )
                            return fail( error, "Terrain layer scalar is invalid" );
                        *slot.second = f32( value->valuedouble );
                    }
                    if( version->valuedouble >= 3 )
                        for( const auto &slot : { std::make_pair( "heightScale", &layer.heightScale ),
                                                  std::make_pair( "heightBias", &layer.heightBias ) } )
                        {
                            auto value = get( slot.first );
                            if( !number( value ) )
                                return fail( error,
                                             "Terrain layer height blend parameters are invalid" );
                            *slot.second = f32( value->valuedouble );
                        }
                    const auto uv = get( "uvScale" ), tint = get( "tint" );
                    if( !cJSON_IsArray( uv ) || cJSON_GetArraySize( uv ) != 2 ||
                        !cJSON_IsArray( tint ) || cJSON_GetArraySize( tint ) != 4 )
                        return fail( error, "Terrain layer vectors are invalid" );
                    for( int j = 0; j < 4; ++j )
                    {
                        auto value = cJSON_GetArrayItem( tint, j );
                        if( !number( value ) )
                            return fail( error, "Terrain layer tint is invalid" );
                        layer.tint[j] = f32( value->valuedouble );
                        if( j < 2 )
                        {
                            value = cJSON_GetArrayItem( uv, j );
                            if( !number( value ) )
                                return fail( error, "Terrain layer UV scale is invalid" );
                            layer.uvScale[j] = f32( value->valuedouble );
                        }
                    }
                }
            }
            if( version->valuedouble >= 3 )
            {
                const auto appearance = field( "appearance" ), masks = field( "surfaceMasks" );
                if( !cJSON_IsObject( appearance ) || !cJSON_IsArray( masks ) ||
                    ( cJSON_GetArraySize( masks ) != 0 &&
                      size_t( cJSON_GetArraySize( masks ) ) != count * 2 ) )
                    return fail( error, "Terrain v3 requires appearance and matching surface masks" );
                auto &a = data.appearance;
                const auto get = [&]( const char *name ) {
                    return cJSON_GetObjectItemCaseSensitive( appearance, name );
                };
                const auto projection = get( "projection" ), heightBlend = get( "heightBlend" ),
                           seed = get( "macroSeed" ), detail = get( "detailNormal" );
                if( !number( projection ) ||
                    ( projection->valuedouble != 0 && projection->valuedouble != 1 ) ||
                    !cJSON_IsBool( heightBlend ) || !number( seed ) || seed->valuedouble < 0 ||
                    seed->valuedouble > std::numeric_limits<u32>::max() ||
                    std::floor( seed->valuedouble ) != seed->valuedouble || !cJSON_IsString( detail ) )
                    return fail( error, "Terrain appearance mode, seed or detail texture is invalid" );
                a.projection = TerrainProjection( u32( projection->valuedouble ) );
                a.heightBlend = cJSON_IsTrue( heightBlend );
                a.macroSeed = u32( seed->valuedouble );
                a.detailNormal = detail->valuestring;
                for( const auto &entry : { std::make_pair( "triplanarSharpness", &a.triplanarSharpness ),
                                           std::make_pair( "heightTransition", &a.heightTransition ),
                                           std::make_pair( "detailStrength", &a.detailStrength ),
                                           std::make_pair( "macroScale", &a.macroScale ),
                                           std::make_pair( "macroStrength", &a.macroStrength ),
                                           std::make_pair( "wetness", &a.wetness ),
                                           std::make_pair( "wetDarkening", &a.wetDarkening ),
                                           std::make_pair( "wetRoughness", &a.wetRoughness ),
                                           std::make_pair( "snow", &a.snow ),
                                           std::make_pair( "snowRoughness", &a.snowRoughness ) } )
                {
                    const auto value = get( entry.first );
                    if( !number( value ) )
                        return fail( error, "Terrain appearance scalar is invalid" );
                    *entry.second = f32( value->valuedouble );
                }
                const auto readVector = [&]( const char *name, auto &vector, int size ) {
                    const auto array = get( name );
                    if( !cJSON_IsArray( array ) || cJSON_GetArraySize( array ) != size )
                        return false;
                    for( int i = 0; i < size; ++i )
                    {
                        const auto value = cJSON_GetArrayItem( array, i );
                        if( !number( value ) )
                            return false;
                        vector[i] = f32( value->valuedouble );
                    }
                    return true;
                };
                if( !readVector( "detailUvScale", a.detailUvScale, 2 ) ||
                    !readVector( "macroTint", a.macroTint, 4 ) ||
                    !readVector( "snowTint", a.snowTint, 4 ) )
                    return fail( error, "Terrain appearance vector is invalid" );
                Vector2F mask;
                size_t channel = 0;
                for( auto value = masks->child; value; value = value->next )
                {
                    if( !number( value ) )
                        return fail( error, "Terrain surface mask is invalid" );
                    mask[channel++] = f32( value->valuedouble );
                    if( channel == 2 )
                    {
                        data.surfaceMasks.push_back( mask );
                        channel = 0;
                    }
                }
            }
            if( !validateTerrainData( data, error ) )
                return false;
            output = std::move( data );
            return true;
        }
        catch( const std::exception &exception )
        {
            error = String( "Unable to read terrain: " ) + exception.what();
            return false;
        }
    }

}  // namespace workphone::render
