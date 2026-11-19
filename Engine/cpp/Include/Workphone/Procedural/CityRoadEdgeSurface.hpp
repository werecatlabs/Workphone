#pragma once

namespace cityRoadEdges
{
    float cross( Vector3F a, Vector3F b )
    {
        return a.x * b.z - a.z * b.x;
    }

    struct Bounds
    {
        float x0, z0, x1, z1;

        template <class Points>
        explicit Bounds( const Points &points ) :
            x0( points[0].x ),
            z0( points[0].z ),
            x1( x0 ),
            z1( z0 )
        {
            for( const auto &p : points )
            {
                x0 = std::min( x0, p.x );
                z0 = std::min( z0, p.z );
                x1 = std::max( x1, p.x );
                z1 = std::max( z1, p.z );
            }
        }

        bool intersects( const Bounds &b ) const
        {
            return x0 <= b.x1 && x1 >= b.x0 && z0 <= b.z1 && z1 >= b.z0;
        }

        template <class Visit>
        bool cells( const Visit &visit ) const
        {
            const auto cell = []( float v ) { return s32( std::floor( v / 4.f ) ); };
            for( auto x = cell( x0 ); x <= cell( x1 ); ++x )
            {
                for( auto z = cell( z0 ); z <= cell( z1 ); ++z )
                {
                    if( visit( std::pair{ x, z } ) )
                    {
                        return true;
                    }
                }
            }
            return false;
        }
    };

    struct Face
    {
        std::array<Vector3F, 3> points;
        Bounds bounds;

        Face( Vector3F a, Vector3F b, Vector3F c ) : points{ a, b, c }, bounds( points )
        {
        }

        float height( Vector3F p ) const
        {
            const auto a = points[1] - points[0], b = points[2] - points[0];
            const auto delta = p - points[0];
            return points[0].y + ( cross( delta, b ) * a.y + cross( a, delta ) * b.y ) / cross( a, b );
        }
    };

    struct Surface
    {
        Surface() = default;

        explicit Surface( SmartPtr<IMesh> road )
        {
            ready = cityCarriagewayUtilities::visitTriangles(
                road, [&]( Vector3F a, Vector3F b, Vector3F c ) { addTriangle( a, b, c ); } );
        }

        void addTriangle( Vector3F a, Vector3F b, Vector3F c )
        {
            const auto area = cross( b - a, c - a );
            if( std::abs( area ) <= 1e-7f )
            {
                return;
            }
            if( area < 0 )
            {
                std::swap( b, c );
            }
            const auto number = u32( faces.size() );
            faces.emplace_back( a, b, c );
            faces.back().bounds.cells( [&]( auto cell ) {
                index[cell].push_back( number );
                return false;
            } );
            ready = true;
        }

        bool contains( Vector3F point ) const
        {
            return visit( Bounds( std::array{ point } ), [&]( const Face &face ) {
                const auto &p = face.points;
                return cross( p[1] - p[0], point - p[0] ) >= -1e-6f &&
                       cross( p[2] - p[1], point - p[1] ) >= -1e-6f &&
                       cross( p[0] - p[2], point - p[2] ) >= -1e-6f;
            } );
        }

        bool overlaps( std::array<Vector3F, 3> triangle ) const
        {
            if( cross( triangle[1] - triangle[0], triangle[2] - triangle[0] ) < 0 )
            {
                std::swap( triangle[1], triangle[2] );
            }
            // XZ convex polygons separate along an edge of either face.
            const auto separates = []( const auto &edges, const auto &points ) {
                for( size_t i = 0; i < edges.size(); ++i )
                {
                    const auto a = edges[i], b = edges[( i + 1 ) % edges.size()];
                    if( std::all_of( points.begin(), points.end(),
                                     [&]( Vector3F p ) { return cross( b - a, p - a ) < -1e-6f; } ) )
                    {
                        return true;
                    }
                }
                return false;
            };
            return visit( Bounds( triangle ), [&]( const Face &face ) {
                return !separates( face.points, triangle ) && !separates( triangle, face.points );
            } );
        }

        bool gutter( Vector3F kerb, Vector3F inward, Vector3F &road ) const
        {
            constexpr float reach = .12f, overlap = .003f;
            float best = reach;
            const auto end = kerb + inward * reach;
            visit( Bounds( std::array{ kerb, end } ), [&]( const Face &face ) {
                // Clip the horizontal kerb normal to an actual road triangle.
                float enter = 0, leave = reach;
                for( size_t i = 0; i < 3; ++i )
                {
                    const auto a = face.points[i], edge = face.points[( i + 1 ) % 3] - a;
                    const auto d = cross( edge, kerb - a ), k = cross( edge, inward );
                    if( std::abs( k ) < 1e-7f )
                    {
                        if( d < 0 )
                        {
                            return false;
                        }
                        continue;
                    }
                    const auto t = -d / k;
                    if( k > 0 )
                    {
                        enter = std::max( enter, t );
                    }
                    else
                    {
                        leave = std::min( leave, t );
                    }
                }
                if( leave - enter < .0005f || enter >= best )
                {
                    return false;
                }
                auto hit = kerb + inward * std::min( enter + overlap, ( enter + leave ) * .5f );
                hit.y = face.height( hit );
                if( std::abs( hit.y - kerb.y ) > .09f )
                {
                    return false;
                }
                best = enter;
                road = hit;
                return best < .008f;
            } );
            return best >= .008f && best < reach;
        }

    private:
        template <class Visit>
        bool visit( const Bounds &bounds, const Visit &test ) const
        {
            return ready && bounds.cells( [&]( auto cell ) {
                const auto found = index.find( cell );
                if( found == index.end() )
                {
                    return false;
                }
                for( const auto i : found->second )
                {
                    if( faces[i].bounds.intersects( bounds ) && test( faces[i] ) )
                    {
                        return true;
                    }
                }
                return false;
            } );
        }

        SourceBudgetVector<Face> faces;
        SourceBudgetMap<std::pair<s32, s32>, SourceBudgetVector<u32>> index;
        bool ready = false;
    };

    void buildGutters( SceneAssets &assets, const CityRoadTerrainMesh &walks, const Surface &road,
                       const SmartPtr<render::IMaterial> &asphalt )
    {
        if( !asphalt )
        {
            return;
        }
        const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
        if( !terrain || terrain->revision != walks.terrainRevision )
        {
            return;
        }
        const auto &placement = assets.roadAuthoring->getTerrainPlacement();
        Geometry gutters, drainage;
        constexpr size_t maximumQuads = 16000;
        const auto runoffWidth = []( Vector3F point ) {
            // Keep the dirty band continuous across independently tessellated
            // kerb skirts, but let its width wander over several metres.
            // World coordinates also keep the same patch when a spline is edited.
            constexpr float scale = 1.f / 2.7f;
            const float x = point.x * scale, z = point.z * scale;
            const auto ix = s32( std::floor( x ) ), iz = s32( std::floor( z ) );
            const auto smooth = []( float t ) { return t * t * ( 3.f - 2.f * t ); };
            const auto value = []( s32 cx, s32 cz ) {
                const auto key = ( u64( u32( cx ) ) << 32 ) | u32( cz );
                return float( render::foliageStableHash( key ^ 0x4e47b8d6c29a5041ull ) >> 40 ) /
                       16777216.f;
            };
            const float u = smooth( x - std::floor( x ) ), v = smooth( z - std::floor( z ) );
            const float low = value( ix, iz ) * ( 1.f - u ) + value( ix + 1, iz ) * u;
            const float high = value( ix, iz + 1 ) * ( 1.f - u ) + value( ix + 1, iz + 1 ) * u;
            const float noise = low * ( 1.f - v ) + high * v;
            return .012f + .045f * noise;
        };
        for( size_t i = 0; i + 2 < walks.indices.size() && gutters.indices.size() / 6 < maximumQuads;
             i += 3 )
        {
            const auto &top = walks.vertices[walks.indices[i]];
            const auto a = walks.vertices[walks.indices[i + 1]].position;
            const auto b = walks.vertices[walks.indices[i + 2]].position;
            // Only the first triangle in each fitted kerb skirt has two bottom points.
            if( std::abs( top.normal.y ) > .01f || top.position.y - a.y < .010f ||
                std::abs( top.position.x - a.x ) > .001f || std::abs( top.position.z - a.z ) > .001f )
            {
                continue;
            }
            const auto delta = b - a;
            const auto length = std::hypot( delta.x, delta.z );
            if( length < .006f )
            {
                continue;
            }
            const auto count = std::max( 1, int( std::ceil( length / .95f ) ) );
            for( int step = 0; step < count && gutters.indices.size() / 6 < maximumQuads; ++step )
            {
                auto p = a + delta * ( float( step ) / count );
                auto q = a + delta * ( float( step + 1 ) / count );
                const auto mid = ( p + q ) * .5f;
                Vector3F r, s, centre;
                if( !road.gutter( p, top.normal, s ) || !road.gutter( q, top.normal, r ) ||
                    !road.gutter( mid, top.normal, centre ) )
                {
                    continue;
                }
                const auto middle = ( r + s ) * .5f;
                if( std::hypot( middle.x - centre.x, middle.z - centre.z ) > .018f ||
                    std::abs( middle.y - centre.y ) > .012f )
                {
                    continue;
                }

                // Keep the apron within the existing skirt and within the terrain-supported slot.
                // The few millimetres of asphalt overlap prevent pinholes from independent tessellation.
                const auto skirtRise = top.position.y - a.y;
                p.y = std::min( p.y + skirtRise - .002f, std::max( p.y + .010f, s.y - .005f ) );
                q.y = std::min( q.y + skirtRise - .002f, std::max( q.y + .010f, r.y - .005f ) );
                r.y += .003f;
                s.y += .003f;
                auto support = ( p + q + r + s ) * .25f;
                float ground;
                if( !render::sampleTerrainHeight( *terrain, placement, support, ground ) ||
                    support.y < ground + .004f || support.y > ground + .09f )
                {
                    continue;
                }
                auto normal = ( q - p ).crossProduct( s - p );
                normal.normalise();
                if( normal.y < 0 )
                {
                    normal = -normal;
                }
                if( normal.y >= .7f )
                {
                    gutters.quad( p, q, r, s, normal );
                    // A narrow, darker binder strip follows the actual fitted
                    // kerb-to-asphalt apron. Keep it below the kerb top and
                    // slightly above the apron so the two surfaces never fight.
                    const float pReach = std::hypot( s.x - p.x, s.z - p.z );
                    const float qReach = std::hypot( r.x - q.x, r.z - q.z );
                    if( pReach >= .025f && qReach >= .025f )
                    {
                        const auto innerP = p + ( s - p ) * std::min( .55f, runoffWidth( p ) / pReach );
                        const auto innerQ = q + ( r - q ) * std::min( .55f, runoffWidth( q ) / qReach );
                        const Vector3F lift( 0, .0008f, 0 );
                        drainage.quad( p + lift, q + lift, innerQ + lift, innerP + lift, normal );
                    }
                }
            }
        }
        if( auto actor = mesh( assets, "Terrain city fitted gutters", gutters, asphalt ) )
        {
            actor->getComponent<scene::MeshRenderer>()->setCastShadows(
                scene::Renderer::CastShadows::Off );
            WP_LOG( String( "Terrain city fitted gutter segments=" ) +
                    StringUtil::toString( gutters.indices.size() / 6 ) );
        }
        if( !drainage.indices.empty() )
        {
            auto colour = asphalt->getDiffuse() * .81f;
            colour.a = 1.f;
            auto weathered = material( assets, colour, .88f );
            // This material shares the scanned asphalt recipe, so keep
            // every native PBR map bound with its darkened colour tint.
            for( const u32 slot : { 0u, 1u, 22u, 3u, 2u } )
            {
                if( auto texture = asphalt->getTexture( slot ) )
                {
                    weathered->setTexture( texture, slot );
                }
            }
            weathered->setRoughness( 1.f );
            weathered->setMetalness( 1.f );
            weathered->setNormalStrength( .18f );
            weathered->setEditorUInt( "uvFilter", 3 );
            weathered->setEditorFloat( "uvAniso", 8.f );
            if( auto actor = mesh( assets, "Terrain city kerb drainage patina", drainage, weathered ) )
            {
                actor->getComponent<scene::MeshRenderer>()->setCastShadows(
                    scene::Renderer::CastShadows::Off );
                WP_LOG( String( "Terrain city kerb drainage segments=" ) +
                        StringUtil::toString( drainage.indices.size() / 6 ) );
            }
        }
    }
}  // namespace cityRoadEdges
