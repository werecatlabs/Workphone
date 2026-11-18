#pragma once

        namespace cityParkLandscape
        {
            struct TerrainProjection
            {
                const render::TerrainData &terrain;
                Vector3F origin, scale;
                QuaternionF rotation, inverse;

                TerrainProjection( const render::TerrainData &data, const Transform3<real_Num> &frame )
                    : terrain( data ),
                      origin( float( frame.getPosition().x ), float( frame.getPosition().y ), float( frame.getPosition().z ) ),
                      scale( float( frame.getScale().x ), float( frame.getScale().y ), float( frame.getScale().z ) ),
                      rotation( float( frame.getOrientation().w ), float( frame.getOrientation().x ),
                                float( frame.getOrientation().y ), float( frame.getOrientation().z ) ),
                      inverse( rotation.inverse() )
                {
                }

                Vector3F sample( s32 x, s32 z ) const
                {
                    return origin + rotation * Vector3F( ( terrain.origin.x + x * terrain.spacing.x ) * scale.x,
                        terrain.heights[size_t( z ) * terrain.dimensions.x + x] * terrain.heightScale * scale.y,
                        ( terrain.origin.y + z * terrain.spacing.y ) * scale.z );
                }

                template <size_t Count, class Height>
                bool emit( Geometry &target, const std::array<Vector3F, Count> &footprint, const Height &lift ) const
                {
                    float xMin = std::numeric_limits<float>::max(), zMin = xMin, xMax = -xMin, zMax = -zMin;
                    float area = 0;
                    for( size_t i = 0; i < Count; ++i )
                    {
                        const auto p = footprint[i];
                        const auto local = inverse * ( p - origin );
                        const float x = ( local.x / scale.x - terrain.origin.x ) / terrain.spacing.x;
                        const float z = ( local.z / scale.z - terrain.origin.y ) / terrain.spacing.y;
                        xMin = std::min( xMin, x ); xMax = std::max( xMax, x );
                        zMin = std::min( zMin, z ); zMax = std::max( zMax, z );
                        area += cityRoadEdges::cross( p - footprint[0], footprint[( i + 1 ) % Count] - footprint[0] );
                    }
                    const auto x0 = s32( std::floor( xMin ) ), z0 = s32( std::floor( zMin ) );
                    const auto x1 = s32( std::floor( xMax ) ), z1 = s32( std::floor( zMax ) );
                    if( x0 < 0 || z0 < 0 || x1 >= terrain.dimensions.x - 1 || z1 >= terrain.dimensions.y - 1 ||
                        s64( x1 - x0 + 1 ) * ( z1 - z0 + 1 ) > 4096 || std::abs( area ) < 1e-5f ) return false;
                    const auto side = area < 0 ? -1.f : 1.f;
                    float covered = 0;
                    for( auto z = z0; z <= z1; ++z )
                    {
                        for( auto x = x0; x <= x1; ++x )
                        {
                            if( render::terrainCellIsHole( terrain, x, z ) ) continue;
                            const auto a = sample( x, z ), b = sample( x + 1, z );
                            const auto c = sample( x, z + 1 ), d = sample( x + 1, z + 1 );
                            for( const auto &triangle : { std::array{ a, c, b }, std::array{ b, c, d } } )
                            {
                                std::array<Vector3F, Count + 4> polygon{}, buffer{};
                                std::copy( triangle.begin(), triangle.end(), polygon.begin() );
                                size_t count = 3;
                                for( size_t i = 0; i < Count && count >= 3; ++i )
                                {
                                    size_t output = 0;
                                    const auto start = footprint[i], edge = footprint[( i + 1 ) % Count] - start;
                                    for( size_t k = 0; k < count; ++k )
                                    {
                                        const auto p = polygon[k], q = polygon[( k + 1 ) % count];
                                        const auto dp = side * cityRoadEdges::cross( edge, p - start );
                                        const auto dq = side * cityRoadEdges::cross( edge, q - start );
                                        if( dp >= 0 ) buffer[output++] = p;
                                        if( ( dp > 0 && dq < 0 ) || ( dp < 0 && dq > 0 ) )
                                            buffer[output++] = p + ( q - p ) * ( dp / ( dp - dq ) );
                                    }
                                    count = output;
                                    polygon.swap( buffer );
                                }
                                if( count < 3 ) continue;
                                const auto base = u32( target.positions.size() );
                                for( size_t i = 0; i < count; ++i ) polygon[i].y += lift( polygon[i] );
                                const auto face = ( polygon[1] - polygon[0] ).crossProduct( polygon[2] - polygon[0] );
                                auto normal = face;
                                if( normal.lengthSquared() < 1e-10f )
                                    normal = ( triangle[1] - triangle[0] ).crossProduct( triangle[2] - triangle[0] );
                                normal.normalise();
                                for( size_t i = 0; i < count; ++i )
                                {
                                    const auto p = polygon[i];
                                    target.positions.push_back( p ); target.normals.push_back( normal );
                                    target.uv.push_back( { p.x * .48f, p.z * .48f, 0 } );
                                    target.tangents.push_back( { 1, 0, 0, 1 } );
                                }
                                for( size_t i = 1; i + 1 < count; ++i )
                                {
                                    const float twice = -cityRoadEdges::cross( polygon[i] - polygon[0], polygon[i + 1] - polygon[0] );
                                    if( twice <= 1e-8f ) continue;
                                    covered += twice;
                                    target.indices.push_back( base );
                                    target.indices.push_back( base + u32( i ) );
                                    target.indices.push_back( base + u32( i + 1 ) );
                                }
                            }
                        }
                    }
                    return std::abs( covered - std::abs( area ) ) < std::max( .001f, std::abs( area ) * .002f );
                }
            };

            void append( Geometry &scene, const Geometry &site )
            {
                const auto base = u32( scene.positions.size() );
                scene.positions.insert( scene.positions.end(), site.positions.begin(), site.positions.end() );
                scene.normals.insert( scene.normals.end(), site.normals.begin(), site.normals.end() );
                scene.uv.insert( scene.uv.end(), site.uv.begin(), site.uv.end() );
                scene.tangents.insert( scene.tangents.end(), site.tangents.begin(), site.tangents.end() );
                for( const auto i : site.indices ) scene.indices.push_back( base + i );
            }
        }  // namespace cityParkLandscape
