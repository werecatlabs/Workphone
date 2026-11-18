#pragma once

        // Render-only lipping on individual pavers. The fitted footway remains
        // the exact terrain-following surface used for collision.
        namespace cityPavementRelief
        {
            constexpr size_t maximumPatches = 1024;
            constexpr size_t maximumTriangles = 26000;
            constexpr size_t maximumPatchTriangles = 128;

            struct Patch
            {
                float x0 = 0, x1 = 0, z0 = 0, z1 = 0;
                float edgeLift = 0, crown = 0, tilt = 0;

                std::array<Vector3F, 4> footprint() const
                {
                    return { Vector3F( x0, 0, z0 ), Vector3F( x1, 0, z0 ),
                             Vector3F( x1, 0, z1 ), Vector3F( x0, 0, z1 ) };
                }

                float lift( Vector3F point ) const
                {
                    const float edge = std::max( 0.f, std::min( { point.x - x0, x1 - point.x,
                                                                  point.z - z0, z1 - point.z } ) );
                    const float t = std::clamp( edge / .036f, 0.f, 1.f );
                    const float bevel = t * t * ( 3.f - 2.f * t );
                    const float lean = ( point.x - ( x0 + x1 ) * .5f ) / ( x1 - x0 );
                    // 4-7 mm of lipping at the arris and a shallow crown;
                    // even the highest point remains below 12 mm.
                    return edgeLift + crown * bevel + tilt * lean;
                }
            };

            float unit( u64 key )
            {
                return float( render::foliageStableHash( key ) >> 40 ) / 16777216.f;
            }

            // Reconstruct precisely the row stagger and long/short cut used by
            // CityFootwayAppearance's baked colour and height maps. The small
            // inset keeps this overlay within the existing mortar joint.
            Patch slabAt( Vector3F point, u32 seed, u64 variation )
            {
                constexpr float course = cityFootwayAppearance::pavingCourseMetres;
                constexpr float pairMetres = cityFootwayAppearance::pavingPairMetres;
                const auto globalRow = s32( std::floor( point.z / course ) );
                const auto row = u32( ( globalRow % s32( cityFootwayAppearance::pavingCourses ) +
                                        s32( cityFootwayAppearance::pavingCourses ) ) %
                                       s32( cityFootwayAppearance::pavingCourses ) );
                const float stagger = ( row % 2 ? .40f : .13f ) + .09f *
                    ( unit( ( u64( seed + 829u ) << 32 ) | row ) - .5f );
                const float tile = cityFootwayAppearance::pavingTileU;
                const float localX = point.x - std::floor( point.x / tile ) * tile;
                const float along = localX + stagger;
                const auto pair = u32( std::floor( along / pairMetres ) );
                const auto wrappedPair = pair % cityFootwayAppearance::pavingPairs;
                const float within = along - pair * pairMetres;
                const float longSlab = .52f + .05f *
                    ( unit( ( u64( seed + 839u ) << 32 ) |
                            ( row * cityFootwayAppearance::pavingPairs + wrappedPair ) ) - .5f );
                const bool shortSlab = within > longSlab;
                const float start = shortSlab ? longSlab : 0.f;
                const float end = shortSlab ? pairMetres : longSlab;
                constexpr float jointInset = .006f;
                return { point.x + start - within + jointInset, point.x + end - within - jointInset,
                         globalRow * course + jointInset, ( globalRow + 1 ) * course - jointInset,
                         .0045f + .0025f * unit( variation + 1 ),
                         .0020f + .0020f * unit( variation + 2 ),
                         .0005f * ( unit( variation + 3 ) - .5f ) };
            }

            bool nearTactileLanding( const Patch &patch, const CityRoadTerrainMesh &walks )
            {
                const auto corners = patch.footprint();
                for( const auto &crossing : walks.crossings )
                {
                    float s0 = std::numeric_limits<float>::max(), l0 = s0;
                    float s1 = -s0, l1 = -s0;
                    for( const auto &point : corners )
                    {
                        const auto delta = point - crossing.centre;
                        const float station = delta.dotProduct( crossing.outward );
                        const float lateral = delta.dotProduct( crossing.across );
                        s0 = std::min( s0, station ); s1 = std::max( s1, station );
                        l0 = std::min( l0, lateral ); l1 = std::max( l1, lateral );
                    }
                    const float stationLimit = crossing.depth * .5f + .72f;
                    const float lateralLimit = crossing.width * .5f + 1.08f;
                    if( s0 <= stationLimit && s1 >= -stationLimit &&
                        l0 <= lateralLimit && l1 >= -lateralLimit ) return true;
                }
                return false;
            }

            bool triangle( Geometry &geometry, Vector3F a, Vector3F b, Vector3F c )
            {
                auto normal = ( b - a ).crossProduct( c - a );
                if( normal.lengthSquared() < 1e-12f ) return false;
                if( normal.y < 0 ) std::swap( b, c );
                normal = ( b - a ).crossProduct( c - a ).normaliseCopy();
                if( normal.y < .70f ) return false;
                auto tangent = Vector3F( 1, 0, 0 ) - normal * normal.x;
                tangent.normalise();
                for( const auto point : { a, b, c } )
                {
                    geometry.indices.push_back( u32( geometry.positions.size() ) );
                    geometry.positions.push_back( point );
                    geometry.normals.push_back( normal );
                    geometry.uv.push_back( { point.x / cityFootwayAppearance::pavingTileU,
                                             point.z / cityFootwayAppearance::pavingTileV, 0 } );
                    geometry.tangents.push_back( { tangent.x, tangent.y, tangent.z, -1.f } );
                }
                return true;
            }

            void wallTriangle( Geometry &geometry, Vector3F a, Vector3F b, Vector3F c,
                               Vector3F outward )
            {
                if( ( b - a ).crossProduct( c - a ).dotProduct( outward ) < 0 ) std::swap( b, c );
                const auto bevelNormal = ( outward + Vector3F( 0, .55f, 0 ) ).normaliseCopy();
                const auto tangent = std::abs( outward.x ) > .5f ?
                    Vector3F( 0, 0, 1 ) : Vector3F( 1, 0, 0 );
                for( const auto point : { a, b, c } )
                {
                    geometry.indices.push_back( u32( geometry.positions.size() ) );
                    geometry.positions.push_back( point );
                    geometry.normals.push_back( bevelNormal );
                    geometry.uv.push_back( { point.x / cityFootwayAppearance::pavingTileU,
                                             point.z / cityFootwayAppearance::pavingTileV, 0 } );
                    geometry.tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
                }
            }

            void edgeWall( Geometry &geometry, Vector3F a, Vector3F b, const Patch &patch )
            {
                constexpr float tolerance = .0004f;
                Vector3F outward{};
                if( std::abs( a.x - patch.x0 ) < tolerance && std::abs( b.x - patch.x0 ) < tolerance )
                    outward = { -1, 0, 0 };
                else if( std::abs( a.x - patch.x1 ) < tolerance && std::abs( b.x - patch.x1 ) < tolerance )
                    outward = { 1, 0, 0 };
                else if( std::abs( a.z - patch.z0 ) < tolerance && std::abs( b.z - patch.z0 ) < tolerance )
                    outward = { 0, 0, -1 };
                else if( std::abs( a.z - patch.z1 ) < tolerance && std::abs( b.z - patch.z1 ) < tolerance )
                    outward = { 0, 0, 1 };
                else return;
                if( ( b - a ).lengthSquared() < 1e-8f ) return;
                auto upperA = a, upperB = b, lowerA = a, lowerB = b;
                upperA.y += patch.lift( a ); upperB.y += patch.lift( b );
                lowerA.y += .00025f; lowerB.y += .00025f;
                wallTriangle( geometry, upperA, lowerA, upperB, outward );
                wallTriangle( geometry, upperB, lowerA, lowerB, outward );
            }

            void build( SceneAssets &assets, const CityRoadTerrainMesh &walks,
                        const SmartPtr<render::IMaterial> &paving, u32 seed )
            {
                if( !paving || walks.crossings.empty() || walks.indices.empty() ) return;
                const cityKerbCoping::TopSurface top( walks );
                Geometry relief, joints;
                SourceBudgetVector<u32> candidates;
                SourceBudgetMap<std::array<s32, 3>, u8> committed;
                size_t installed = 0;
                for( size_t crossingIndex = 0; crossingIndex < walks.crossings.size() &&
                     installed < maximumPatches; ++crossingIndex )
                {
                    const auto &crossing = walks.crossings[crossingIndex];
                    auto along = crossing.outward; along.y = 0;
                    auto across = crossing.across; across.y = 0;
                    if( along.lengthSquared() < .9f || across.lengthSquared() < .9f ) continue;
                    along.normalise(); across.normalise();
                    for( const float side : { -1.f, 1.f } )
                    for( const float approach : { -1.f, 1.f } )
                    for( u32 step = 0; step < 7; ++step )
                    for( u32 band = 0; band < 2; ++band )
                    {
                        if( installed >= maximumPatches ||
                            ( relief.indices.size() + joints.indices.size() ) / 3 >= maximumTriangles ) break;
                        const u64 key = render::foliageStableHash( ( u64( seed ) << 32 ) ^
                            ( u64( crossingIndex ) << 8 ) ^ ( side > 0 ? 1u : 2u ) ^
                            ( approach > 0 ? 4u : 8u ) ^ ( u64( step ) << 4 ) ^
                            ( u64( band ) << 7 ) ^
                            0x7061766572656c69ull );
                        if( unit( key ) < .60f ) continue;
                        const float station = approach * ( crossing.depth * .5f + 1.40f +
                            step * 1.25f + .36f * unit( key + 1 ) );
                        const float lateral = side * ( crossing.width * .5f +
                            ( band ? 1.78f : .80f ) + .22f * unit( key + 2 ) );
                        const Patch patch = slabAt( crossing.centre + along * station + across * lateral,
                                                    seed, key );
                        if( nearTactileLanding( patch, walks ) ) continue;
                        const std::array<s32, 3> slabKey{
                            s32( std::lround( patch.x0 * 1000.f ) ),
                            s32( std::lround( patch.z0 * 1000.f ) ),
                            s32( std::lround( patch.x1 * 1000.f ) ) };
                        if( committed.find( slabKey ) != committed.end() ) continue;
                        const auto footprint = patch.footprint();
                        const cityRoadEdges::Bounds bounds( footprint );
                        candidates.clear();
                        bounds.cells( [&]( auto cell ) {
                            const auto found = top.cells.find( cell );
                            if( found != top.cells.end() )
                                candidates.insert( candidates.end(), found->second.begin(), found->second.end() );
                            return false;
                        } );
                        std::sort( candidates.begin(), candidates.end() );
                        candidates.erase( std::unique( candidates.begin(), candidates.end() ), candidates.end() );
                        Geometry piece, seam;
                        double covered = 0;
                        bool complete = true;
                        for( const auto id : candidates )
                        {
                            const auto &face = top.faces[id];
                            if( !face.bounds.intersects( bounds ) ) continue;
                            std::array<Vector3F, 8> polygon;
                            const auto count = cityKerbCoping::clip( polygon, face, footprint );
                            if( count < 3 ) continue;
                            Vector3F centroid{};
                            for( size_t i = 0; i < count; ++i ) centroid += polygon[i];
                            centroid /= float( count );
                            for( size_t i = 0; i < count; ++i )
                            {
                                const auto &a = polygon[i], &b = polygon[( i + 1 ) % count];
                                const double area = .5 * std::abs( double( cityRoadEdges::cross( a - centroid,
                                                                                                 b - centroid ) ) );
                                if( area < 1e-9 ) continue;
                                if( ( piece.indices.size() + seam.indices.size() ) / 3 + 3 >
                                    maximumPatchTriangles ) { complete = false; break; }
                                auto p = a, q = b, r = centroid;
                                p.y += patch.lift( p ); q.y += patch.lift( q ); r.y += patch.lift( r );
                                if( !triangle( piece, p, q, r ) ) { complete = false; break; }
                                edgeWall( seam, a, b, patch );
                                covered += area;
                            }
                            if( !complete ) break;
                        }
                        const double expected = double( patch.x1 - patch.x0 ) * ( patch.z1 - patch.z0 );
                        if( !complete || piece.indices.empty() || std::abs( covered - expected ) > expected * .012 ||
                            ( piece.indices.size() + seam.indices.size() ) / 3 >
                            maximumTriangles - ( relief.indices.size() + joints.indices.size() ) / 3 ) continue;
                        cityKerbCoping::append( relief, piece );
                        cityKerbCoping::append( joints, seam );
                        committed.emplace( slabKey, 1u );
                        if( ++installed == maximumPatches ) break;
                    }
                }
                if( relief.indices.empty() ) return;
                if( auto actor = mesh( assets, "Terrain city settled pavement slabs", relief, paving ) )
                {
                    actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
                    WP_LOG( String( "Terrain city settled pavement slabs=" ) + StringUtil::toString( installed ) +
                        " topTriangles=" + StringUtil::toString( relief.indices.size() / 3 ) +
                        " jointTriangles=" + StringUtil::toString( joints.indices.size() / 3 ) );
                }
                if( !joints.indices.empty() )
                {
                    // The beveled concrete edge shares the same world-aligned
                    // paving material, so its colour meets the baked mortar.
                    if( auto actor = mesh( assets, "Terrain city pavement slab joints", joints, paving ) )
                        actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
                }
            }
        }  // namespace cityPavementRelief
