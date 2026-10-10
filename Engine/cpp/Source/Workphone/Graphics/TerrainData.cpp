#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <algorithm>
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
            const bool hitFirst = rayTriangle( origin, direction, a, c, b, first );
            const bool hitSecond = rayTriangle( origin, direction, b, c, d, second );
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
}  // namespace workphone::render
