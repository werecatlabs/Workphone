#pragma once

        namespace cityEntranceRoute
        {
            struct Row
            {
                Vector3F left, centre, right;
            };

            struct Route
            {
                std::array<Row, 96> rows{};
                size_t rowCount = 0;
                float length = 0;
                u32 bends = 0;
            };

            struct Target { Vector3F point, outward; float score; };

            float cross( Vector3F a, Vector3F b ) { return a.x * b.z - a.z * b.x; }

            Vector3F horizontal( Vector3F p ) { p.y = 0; return p; }

            template <class Edges, class Visit>
            void visitEdges( const Edges &edges, Vector3F low, Vector3F high, const Visit &visit )
            {
                const auto first = Edges::cell( low ), last = Edges::cell( high );
                for( s32 x = first.first; x <= last.first; ++x )
                    for( s32 z = first.second; z <= last.second; ++z )
                    {
                        const auto where = edges.index.find( { x, z } );
                        if( where == edges.index.end() ) continue;
                        for( const auto &edge : where->second ) visit( edge );
                    }
            }

            template <class Edges>
            bool rimPoint( const Edges &edges, Vector3F p, Vector3F heading, Vector3F outward, Vector3F &hit )
            {
                constexpr float reach = 1.55f;
                const auto a = p - heading * reach, b = p + heading * reach;
                float earliest = 2.f * reach;
                visitEdges( edges, { std::min( a.x, b.x ), 0, std::min( a.z, b.z ) },
                                   { std::max( a.x, b.x ), 0, std::max( a.z, b.z ) }, [&]( const auto &edge ) {
                    if( edge.outside.dotProduct( outward ) < .76f || edge.outside.dotProduct( heading ) > -.43f ) return;
                    const auto d = edge.b - edge.a;
                    const auto det = cross( heading, d );
                    if( std::abs( det ) < 1e-6f ) return;
                    const auto delta = edge.a - a;
                    const auto t = cross( delta, d ) / det, along = cross( delta, heading ) / det;
                    if( t < 0 || t >= earliest || along < -.0001f || along > 1.0001f ) return;
                    earliest = t;
                    hit = edge.a + d * std::clamp( along, 0.f, 1.f );
                } );
                return earliest < 2.f * reach && std::abs( earliest - reach ) < 1.12f;
            }

            struct Stations
            {
                std::array<Vector3F, 96> points{};
                size_t count = 0;

                bool lineTo( Vector3F end )
                {
                    const auto from = points[count - 1];
                    const float distance = horizontal( end - from ).length();
                    if( distance <= .06f ) return true;
                    const auto sections = std::max( 1, int( std::ceil( distance / .66f ) ) );
                    if( count + size_t( sections ) > points.size() ) return false;
                    for( int i = 1; i <= sections; ++i ) points[count++] = from + ( end - from ) * ( float( i ) / sections );
                    return true;
                }

                bool set( const std::array<Vector3F, 4> &knots, size_t size )
                {
                    points[0] = knots[0]; count = 1;
                    for( size_t i = 1; i + 1 < size; ++i )
                    {
                        const auto centre = knots[i];
                        const auto previous = horizontal( knots[i - 1] - centre );
                        const auto next = horizontal( knots[i + 1] - centre );
                        const float a = previous.length(), b = next.length();
                        if( a < .7f || b < .7f || previous.dotProduct( next ) > -.24f * a * b ) return false;
                        const float trim = std::min( { 1.12f, .42f * a, .42f * b } );
                        const auto approach = centre + previous * ( trim / a ), leave = centre + next * ( trim / b );
                        if( !lineTo( approach ) || count + 4 >= points.size() ) return false;
                        for( int section = 1; section <= 4; ++section )
                        {
                            const float t = float( section ) / 4, inv = 1 - t;
                            points[count++] = approach * ( inv * inv ) + centre * ( 2 * inv * t ) + leave * ( t * t );
                        }
                    }
                    return lineTo( knots[size - 1] );
                }
            };

            template <class Surface>
            bool terrainFace( Vector3F a, Vector3F b, Vector3F c, const Surface &surface )
            {
                if( cross( b - a, c - a ) <= .00002f ) return false;
                for( auto point : { ( a + b + c ) / 3.f, ( a + c ) * .5f, ( a + b ) * .5f, ( b + c ) * .5f } )
                {
                    const float expected = point.y;
                    if( !surface( point ) || expected < point.y - .034f || expected > point.y + .168f ) return false;
                }
                return true;
            }

            template <class Edges, class Clear, class Surface>
            bool fit( const Edges &edges, const Row &start, Vector3F departure, float width,
                      const Target &target, const std::array<Vector3F, 4> &knots, size_t count,
                      const Clear &clear, const Surface &surface, Route &route )
            {
                Stations centre;
                if( !centre.set( knots, count ) || centre.count < 3 ) return false;
                const size_t last = centre.count - 1;
                const auto &points = centre.points;
                const auto intoWalk = horizontal( points[last] - points[last - 1] ).normaliseCopy();
                if( target.outward.dotProduct( intoWalk ) > -.53f ||
                    departure.dotProduct( horizontal( points[1] - points[0] ).normaliseCopy() ) < .45f ) return false;
                const Vector3F rimAcross( intoWalk.z, 0, -intoWalk.x );
                Row rim;
                if( !rimPoint( edges, target.point - rimAcross * width, intoWalk, target.outward, rim.left ) ||
                    !rimPoint( edges, target.point, intoWalk, target.outward, rim.centre ) ||
                    !rimPoint( edges, target.point + rimAcross * width, intoWalk, target.outward, rim.right ) ||
                    horizontal( ( rim.left + rim.right ) * .5f - rim.centre ).length() > .24f ) return false;
                auto ground = rim;
                if( !surface( ground.left ) || !surface( ground.centre ) || !surface( ground.right ) ) return false;
                const Vector3F rise( rim.left.y - ground.left.y, rim.centre.y - ground.centre.y, rim.right.y - ground.right.y );
                if( std::min( { rise.x, rise.y, rise.z } ) < -.025f || std::max( { rise.x, rise.y, rise.z } ) > .162f ) return false;

                std::array<float, 96> station{};
                for( size_t i = 1; i <= last; ++i )
                    station[i] = station[i - 1] + horizontal( points[i] - points[i - 1] ).length();
                if( station[last] > 28.f ) return false;
                Route candidate;
                candidate.rows[0] = start;
                const float startWidth = .5f * horizontal( start.right - start.left ).length();
                for( size_t i = 1; i <= last; ++i )
                {
                    auto &row = candidate.rows[i];
                    if( i == last ) row = rim;
                    else
                    {
                        const auto axis = horizontal( points[i + 1] - points[i - 1] ).normaliseCopy();
                        const Vector3F across( axis.z, 0, -axis.x );
                        const float t = std::clamp( station[i] / 2.f, 0.f, 1.f );
                        const float half = startWidth + ( width - startWidth ) * t;
                        row = { points[i] - across * half, points[i], points[i] + across * half };
                        if( !surface( row.left ) || !surface( row.centre ) || !surface( row.right ) ) return false;
                        const float ramp = std::clamp( 1.f - ( station[last] - station[i] ) / 1.62f, 0.f, 1.f );
                        row.left.y += rise.x * ramp;
                        row.centre.y += rise.y * ramp;
                        row.right.y += rise.z * ramp;
                    }
                    const auto &prior = candidate.rows[i - 1];
                    const auto slope = [&]( Vector3F a, Vector3F b ) {
                        return std::abs( a.y - b.y ) <= .224f * horizontal( a - b ).length() + .012f;
                    };
                    if( !slope( prior.left, row.left ) || !slope( prior.centre, row.centre ) || !slope( prior.right, row.right ) ||
                        std::abs( row.left.y - row.right.y ) > .125f * horizontal( row.left - row.right ).length() + .019f ||
                        !clear( std::array{ prior.left, prior.right, row.right, row.left }, i == last ) ||
                        !terrainFace( prior.left, prior.centre, row.centre, surface ) ||
                        !terrainFace( prior.left, row.centre, row.left, surface ) ||
                        !terrainFace( prior.centre, prior.right, row.right, surface ) ||
                        !terrainFace( prior.centre, row.right, row.centre, surface ) ) return false;
                    candidate.length += horizontal( row.centre - prior.centre ).length();
                }
                candidate.rowCount = centre.count;
                candidate.bends = u32( count - 2 );
                route = candidate;
                return true;
            }

            template <class Edges, class Clear, class Surface>
            bool plan( const Edges &edges, const Row &start, Vector3F departure, float halfWidth,
                       const Clear &clear, const Surface &surface, Route &route )
            {
                constexpr float search = 25.f;
                const Vector3F extent( search, 0, search );
                std::array<Target, 28> nearest;
                size_t targets = 0;
                departure = horizontal( departure ).normaliseCopy();
                auto launch = start;
                const Vector3F right( departure.z, 0, -departure.x );
                if( ( launch.right - launch.left ).dotProduct( right ) < 0 ) std::swap( launch.right, launch.left );
                visitEdges( edges, start.centre - extent, start.centre + extent, [&]( const auto &edge ) {
                    const auto v = horizontal( edge.b - edge.a );
                    if( v.lengthSquared() < .0025f ) return;
                    const float t = std::clamp( horizontal( start.centre - edge.a ).dotProduct( v ) / v.lengthSquared(), 0.f, 1.f );
                    const auto p = edge.a + ( edge.b - edge.a ) * t;
                    const auto shift = horizontal( p - start.centre );
                    const float length = shift.length();
                    if( length < .9f || length > search || shift.dotProduct( edge.outside ) > -.35f * length ||
                        shift.dotProduct( departure ) < -.12f * length ) return;
                    const float score = length + 2.f * ( 1.f - shift.dotProduct( departure ) / length );
                    if( targets == nearest.size() && score >= nearest[targets - 1].score ) return;
                    for( size_t i = 0; i < targets; ++i )
                    {
                        if( horizontal( nearest[i].point - p ).lengthSquared() >= .72f * .72f ||
                            nearest[i].outward.dotProduct( edge.outside ) < .93f ) continue;
                        if( nearest[i].score <= score ) return;
                        nearest[i] = nearest[--targets];
                        std::sort( nearest.begin(), nearest.begin() + targets, []( const auto &a, const auto &b ) { return a.score < b.score; } );
                        break;
                    }
                    if( targets < nearest.size() ) ++targets;
                    size_t slot = targets - 1;
                    while( slot > 0 && nearest[slot - 1].score > score )
                    {
                        nearest[slot] = nearest[slot - 1];
                        --slot;
                    }
                    nearest[slot] = { p, edge.outside, score };
                } );

                for( size_t i = 0; i < targets; ++i )
                {
                    const auto &target = nearest[i];
                    const auto origin = start.centre;
                    const auto toe = origin + departure * 1.42f;
                    const auto mouth = target.point + target.outward * 1.67f;
                    const auto attempt = [&]( std::array<Vector3F, 4> corners, size_t count ) {
                        return fit( edges, launch, departure, halfWidth, target, corners, count, clear, surface, route );
                    };
                    if( attempt( { origin, target.point }, 2 ) || attempt( { origin, mouth, target.point }, 3 ) ||
                        attempt( { origin, toe, target.point }, 3 ) || attempt( { origin, toe, mouth, target.point }, 4 ) ) return true;
                }
                return false;
            }
        }
