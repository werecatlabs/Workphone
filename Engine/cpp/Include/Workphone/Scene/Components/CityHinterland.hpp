#pragma once

namespace cityHinterland
{
    struct Relief
    {
        const render::TerrainData &terrain;
        float stepX, stepZ, verticalScale, midpointX, midpointZ, halfWidth, halfDepth;
        float baseline;
        u32 seed;

        float sample( s32 x, s32 z ) const
        {
            return float( double( terrain.heights[size_t( z ) * terrain.dimensions.x + x] ) *
                          terrain.heightScale ) *
                   verticalScale;
        }

        float alongEdge( float x, float z ) const
        {
            x = std::clamp( x, 0.f, float( terrain.dimensions.x - 1 ) );
            z = std::clamp( z, 0.f, float( terrain.dimensions.y - 1 ) );
            const auto ix = s32( x ), iz = s32( z );
            const auto ex = std::min( ix + 1, terrain.dimensions.x - 1 );
            const auto ez = std::min( iz + 1, terrain.dimensions.y - 1 );
            const auto blend = []( float a, float b, float t ) { return a + ( b - a ) * t; };
            return blend( blend( sample( ix, iz ), sample( ex, iz ), x - ix ),
                          blend( sample( ix, ez ), sample( ex, ez ), x - ix ), z - iz );
        }

        Vector2F gradient( float x, float z ) const
        {
            const float loX = std::max( 0.f, x - 1 ),
                        hiX = std::min( float( terrain.dimensions.x - 1 ), x + 1 );
            const float loZ = std::max( 0.f, z - 1 ),
                        hiZ = std::min( float( terrain.dimensions.y - 1 ), z + 1 );
            return { ( alongEdge( hiX, z ) - alongEdge( loX, z ) ) / ( ( hiX - loX ) * stepX ),
                     ( alongEdge( x, hiZ ) - alongEdge( x, loZ ) ) / ( ( hiZ - loZ ) * stepZ ) };
        }

        float height( float x, float z ) const
        {
            const float px = x - midpointX, pz = z - midpointZ;
            const float distance =
                std::max( { 0.f, std::abs( px ) - halfWidth, std::abs( pz ) - halfDepth } );
            const float nx = px / ( halfWidth + distance ), nz = pz / ( halfDepth + distance );
            const float sx = std::clamp( nx * .5f + .5f, 0.f, 1.f ) * ( terrain.dimensions.x - 1 );
            const float sz = std::clamp( nz * .5f + .5f, 0.f, 1.f ) * ( terrain.dimensions.y - 1 );
            const float edge = alongEdge( sx, sz );
            const auto grade = gradient( sx, sz );
            const float slope = std::clamp( nx * grade.x + nz * grade.y, -.24f, .24f );
            const float continued = edge + slope * distance / ( 1.f + distance / 56.f );
            const float hills =
                baseline + 12.f + 60.f * ( cityValueNoise( seed + 307, px / 650.f, pz / 775.f ) - .5f ) +
                24.f * ( cityValueNoise( seed + 401, px / 340.f + 1.8f, pz / 475.f ) - .5f ) +
                8.f * ( cityValueNoise( seed + 503, px / 225.f, pz / 260.f - 2.8f ) - .5f );
            const float t = std::clamp( distance / 240.f, 0.f, 1.f );
            return continued + ( hills - continued ) * t * t * ( 3.f - 2.f * t );
        }
    };

    void build( Geometry &geometry, const render::TerrainData &terrain,
                const Transform3<real_Num> &placement, float cityExtent, u32 seed,
                VehicleAppearanceQuality quality )
    {
        const auto width = terrain.dimensions.x, depth = terrain.dimensions.y;
        if( width < 2 || depth < 2 || terrain.heights.size() != size_t( width ) * depth )
        {
            return;
        }
        const auto &scale = placement.getScale();
        const float stepX = terrain.spacing.x * scale.x, stepZ = terrain.spacing.y * scale.z;
        if( stepX <= 0 || stepZ <= 0 )
        {
            return;
        }
        const float halfWidth = .5f * ( width - 1 ) * stepX;
        const float halfDepth = .5f * ( depth - 1 ) * stepZ;
        Relief relief{ terrain,
                       stepX,
                       stepZ,
                       float( scale.y ),
                       float( terrain.origin.x * scale.x ) + halfWidth,
                       float( terrain.origin.y * scale.z ) + halfDepth,
                       halfWidth,
                       halfDepth,
                       0.f,
                       seed };
        double boundarySum = 0;
        for( s32 x = 0; x < width; ++x )
        {
            boundarySum += relief.sample( x, 0 ) + relief.sample( x, depth - 1 );
        }
        for( s32 z = 1; z < depth - 1; ++z )
        {
            boundarySum += relief.sample( 0, z ) + relief.sample( width - 1, z );
        }
        relief.baseline = float( boundarySum / ( 2 * ( width + depth - 2 ) ) );

        const auto &position = placement.getPosition();
        const auto &rotation = placement.getOrientation();
        const bool preview = quality == VehicleAppearanceQuality::Preview;
        const int rows = preview ? 15 : 23;
        const int maxDivisions = preview ? 48 : 80;
        const float reach = std::max( 2350.f, cityExtent * 7.2f );
        const auto tangent = rotation * Vector3<real_Num>( 1, 0, 0 );
        const auto addVertex = [&]( float x, float y, float z, Vector2F grade ) {
            const auto p = position + rotation * Vector3<real_Num>( x, y, z );
            const auto n = rotation * Vector3<real_Num>( -grade.x, 1, -grade.y ).normaliseCopy();
            geometry.positions.push_back( { float( p.x ), float( p.y ), float( p.z ) } );
            geometry.normals.push_back( { float( n.x ), float( n.y ), float( n.z ) } );
            geometry.tangents.push_back(
                { float( tangent.x ), float( tangent.y ), float( tangent.z ), 1 } );
            geometry.uv.push_back( { float( p.x ), float( p.z ), 0 } );
        };
        const auto triangle = [&]( u32 a, u32 b, u32 c ) {
            const auto ab = geometry.positions[b] - geometry.positions[a];
            const auto ac = geometry.positions[c] - geometry.positions[a];
            if( ab.crossProduct( ac ).y < 0 )
            {
                std::swap( b, c );
            }
            for( const auto index : { a, b, c } )
            {
                geometry.indices.push_back( index );
            }
        };

        // Every terrain tile LOD preserves the original perimeter vertices. Stitch
        // the first row to those samples, then reduce detail as the hills recede.
        for( const bool alongX : { true, false } )
        {
            const int edgeSections = ( alongX ? width : depth ) - 1;
            const float extent = alongX ? halfWidth : halfDepth;
            for( const bool positive : { false, true } )
            {
                u32 previous = u32( geometry.positions.size() );
                int previousSections = edgeSections;
                for( int i = 0; i <= edgeSections; ++i )
                {
                    const int ix = alongX ? i : ( positive ? width - 1 : 0 );
                    const int iz = alongX ? ( positive ? depth - 1 : 0 ) : i;
                    const float x =
                        float( double( terrain.origin.x ) + double( ix ) * terrain.spacing.x ) * scale.x;
                    const float z =
                        float( double( terrain.origin.y ) + double( iz ) * terrain.spacing.y ) * scale.z;
                    addVertex( x, relief.sample( ix, iz ), z,
                               relief.gradient( float( ix ), float( iz ) ) );
                }
                for( int row = 1; row <= rows; ++row )
                {
                    const float t = float( row ) / rows;
                    const float distance = reach * t * t;
                    const float target = 16.f + .16f * distance;
                    const int sections =
                        int( std::clamp( std::ceil( 2.f * ( extent + distance ) / target ), 14.f,
                                         float( maxDivisions ) ) );
                    const auto current = u32( geometry.positions.size() );
                    for( int i = 0; i <= sections; ++i )
                    {
                        const float across = 2.f * i / sections - 1.f;
                        const float outward = positive ? 1.f : -1.f;
                        const float x =
                            relief.midpointX + ( halfWidth + distance ) * ( alongX ? across : outward );
                        const float z =
                            relief.midpointZ + ( halfDepth + distance ) * ( alongX ? outward : across );
                        const Vector2F grade(
                            ( relief.height( x + 1, z ) - relief.height( x - 1, z ) ) * .5f,
                            ( relief.height( x, z + 1 ) - relief.height( x, z - 1 ) ) * .5f );
                        addVertex( x, relief.height( x, z ), z, grade );
                    }
                    int innerIndex = 0, outerIndex = 0;
                    while( innerIndex < previousSections || outerIndex < sections )
                    {
                        if( outerIndex == sections ||
                            ( innerIndex < previousSections &&
                              ( innerIndex + 1 ) * sections <= ( outerIndex + 1 ) * previousSections ) )
                        {
                            triangle( previous + innerIndex, previous + innerIndex + 1,
                                      current + outerIndex );
                            ++innerIndex;
                        }
                        else
                        {
                            triangle( previous + innerIndex, current + outerIndex + 1,
                                      current + outerIndex );
                            ++outerIndex;
                        }
                    }
                    previous = current;
                    previousSections = sections;
                }
            }
        }
    }
}  // namespace cityHinterland
