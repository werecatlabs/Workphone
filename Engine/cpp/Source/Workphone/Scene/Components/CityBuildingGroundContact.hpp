#pragma once

#include "CityFrontagePlanting.hpp"
#include "CityEntranceRoute.hpp"
#include "CityPedestrianSurfacing.hpp"
#include "CityEntranceHandrails.hpp"

        namespace cityGroundContact
        {
            struct Entry
            {
                const OpenCityLayout::Lot *lot;
                Vector3F door, outward, along;
                float span, doorOffset, halfWidth;
            };

            float cross( Vector3F a, Vector3F b ) { return a.x * b.z - a.z * b.x; }

            struct WalkEdges
            {
                struct Edge { Vector3F a, b, outside; };
                using Cell = std::pair<s32, s32>;
                SourceBudgetMap<Cell, SourceBudgetVector<Edge>> index;

                static Cell cell( Vector3F p )
                {
                    return { s32( std::floor( p.x / 12.f ) ), s32( std::floor( p.z / 12.f ) ) };
                }

                explicit WalkEdges( const CityRoadTerrainMesh &footways )
                {
                    for( size_t i = 0; i + 2 < footways.indices.size(); i += 3 )
                    {
                        const auto &top = footways.vertices[footways.indices[i]];
                        const auto &a = footways.vertices[footways.indices[i + 1]];
                        const auto &b = footways.vertices[footways.indices[i + 2]];
                        // Fitted footways emit the first skirt triangle as top A, bottom A,
                        // bottom B. Only the real, exposed boundary can join a frontage.
                        if( std::abs( top.normal.y ) > .01f || top.position.y - a.position.y < .10f ||
                            std::abs( top.position.x - a.position.x ) > .001f ||
                            std::abs( top.position.z - a.position.z ) > .001f ) continue;
                        auto farTop = b.position + Vector3F( 0, top.position.y - a.position.y, 0 );
                        if( i + 5 < footways.indices.size() && footways.indices[i + 3] == footways.indices[i] &&
                            footways.indices[i + 4] == footways.indices[i + 2] )
                        {
                            const auto candidate = footways.vertices[footways.indices[i + 5]].position;
                            if( std::hypot( candidate.x - b.position.x, candidate.z - b.position.z ) < .001f )
                                farTop = candidate;
                        }
                        const auto lo = cell( { std::min( a.position.x, b.position.x ), 0,
                                                std::min( a.position.z, b.position.z ) } );
                        const auto hi = cell( { std::max( a.position.x, b.position.x ), 0,
                                                std::max( a.position.z, b.position.z ) } );
                        for( auto x = lo.first; x <= hi.first; ++x )
                            for( auto z = lo.second; z <= hi.second; ++z )
                                index[{ x, z }].push_back( { top.position, farTop, top.normal } );
                    }
                }

                float reach( Vector3F origin, Vector3F outward, bool fromRoad = false ) const
                {
                    constexpr float limit = 16.f;
                    const auto end = origin + outward * limit;
                    const auto lo = cell( { std::min( origin.x, end.x ), 0, std::min( origin.z, end.z ) } );
                    const auto hi = cell( { std::max( origin.x, end.x ), 0, std::max( origin.z, end.z ) } );
                    float closest = limit;
                    for( auto x = lo.first; x <= hi.first; ++x )
                    {
                        for( auto z = lo.second; z <= hi.second; ++z )
                        {
                            const auto found = index.find( { x, z } );
                            if( found == index.end() ) continue;
                            for( const auto &edge : found->second )
                            {
                                const float facing = edge.outside.dotProduct( outward );
                                if( fromRoad ? facing < .35f : facing > -.35f ) continue;
                                const auto delta = edge.b - edge.a;
                                const auto determinant = cross( outward, delta );
                                if( std::abs( determinant ) < .00001f ) continue;
                                const auto from = edge.a - origin;
                                const auto distance = cross( from, delta ) / determinant;
                                const auto fraction = cross( from, outward ) / determinant;
                                if( distance >= 0 && distance < closest && fraction >= -.0001f && fraction <= 1.0001f )
                                    closest = distance;
                            }
                        }
                    }
                    return closest < limit ? closest : -1.f;
                }
            };

            float visibleFoundationTop( const OpenCityLayout::Lot &lot )
            {
                // Match the visual pad in the city builder. A facade that occupies
                // the full parcel already covers the extra collider overlap.
                const bool covered = !lot.park && std::abs( lot.x - lot.parcelX ) < .001f &&
                                     std::abs( lot.z - lot.parcelZ ) < .001f &&
                                     std::abs( lot.width - lot.parcelWidth ) < .001f &&
                                     std::abs( lot.depth - lot.parcelDepth ) < .001f;
                return lot.groundHeight + ( covered ? 0.f : .15f );
            }

            float stoneVariation( u64 key )
            {
                return float( render::foliageStableHash( key ) >> 40 ) / 16777216.f;
            }

            struct RaisedApproach
            {
                float end = 0;
                float height = 0;
                int risers = 0;
                bool handrails = false;
            };

            template <class Surface, class Clear>
            RaisedApproach buildRaisedApproach( const Entry &entry, float depth, float half,
                                                Vector3F floor, const Surface &surface,
                                                const Clear &clear, Geometry &stairs,
                                                cityEntranceHandrails::Batches &handrails )
            {
                constexpr float landing = .83f, tread = .34f, apronRun = .96f;
                constexpr int maxRisers = 9;
                const auto outward = entry.outward, along = entry.along;
                const float start = std::max( .025f, depth + .005f );
                const auto point = [&]( float s, float u ) { return floor + outward * s + along * u; };
                const auto supported = [&]( float s, std::array<Vector3F, 3> &row ) {
                    row = { point( s, -half ), point( s, 0 ), point( s, half ) };
                    return std::all_of( row.begin(), row.end(), [&]( auto &p ) { return surface( p ); } );
                };
                std::array<Vector3F, 3> upperGround{};
                if( !supported( start + .24f, upperGround ) || floor.y - upperGround[1].y < .055f ) return {};
                for( int count = 1; count <= maxRisers; ++count )
                {
                    const float end = start + landing + count * tread;
                    if( !clear( point( start, -half ), point( end + apronRun, half ) ) ) continue;
                    std::array<Vector3F, 3> lastTread{}, bottom{}, outer{};
                    if( !supported( end - tread, lastTread ) || !supported( end, bottom ) ||
                        !supported( end + apronRun, outer ) ) continue;
                    const float base = std::max( { lastTread[0].y, lastTread[1].y, lastTread[2].y,
                                                   bottom[0].y, bottom[1].y, bottom[2].y } );
                    const float riser = ( floor.y - base ) / count;
                    if( riser < ( count == 1 ? .060f : .10f ) || riser > .181f ||
                        base - std::min( bottom.front().y, bottom.back().y ) > .27f ||
                        std::abs( outer[1].y - base ) > .24f ||
                        std::abs( outer[0].y - outer[2].y ) > .25f ) continue;
                    auto midpoint = ( bottom.front() + outer.back() ) * .5f;
                    const float apronMiddle = ( base + outer.back().y ) * .5f;
                    if( !surface( midpoint ) || midpoint.y > apronMiddle + .028f ||
                        apronMiddle - midpoint.y > .12f ) continue;

                    std::array<std::array<Vector3F, 3>, maxRisers + 2> ground{};
                    bool fits = supported( start, ground[0] );
                    for( int row = 1; fits && row <= count + 1; ++row )
                    {
                        const float s = start + landing + ( row - 1 ) * tread;
                        fits = supported( s, ground[row] );
                        const float top = floor.y - std::min( row, count ) * riser;
                        for( const auto p : ground[row] ) fits = fits && p.y <= top + .025f && top - p.y <= 1.55f;
                    }
                    if( !fits ) continue;

                    const auto cheek = [&]( Vector3F lowerA, Vector3F lowerB, Vector3F topA,
                                            Vector3F topB, Vector3F normal ) {
                        lowerA.y = std::min( lowerA.y - .030f, topA.y );
                        lowerB.y = std::min( lowerB.y - .030f, topB.y );
                        if( std::max( topA.y - lowerA.y, topB.y - lowerB.y ) > .035f )
                            stairs.quad( lowerA, lowerB, topB, topA, normal );
                    };
                    for( int step = 0; step <= count; ++step )
                    {
                        const auto &a = ground[step], &b = ground[step + 1];
                        const float y = floor.y - step * riser;
                        auto ll = a.front(), lr = a.back(), farLeft = b.front(), farRight = b.back();
                        ll.y = lr.y = farLeft.y = farRight.y = y;
                        stairs.quad( ll, lr, farRight, farLeft, { 0, 1, 0 } );
                        cheek( a.front(), b.front(), ll, farLeft, -along );
                        cheek( a.back(), b.back(), lr, farRight, along );
                        if( step < count )
                        {
                            const auto drop = Vector3F( 0, riser, 0 );
                            stairs.quad( farLeft - drop, farRight - drop, farRight, farLeft, outward );
                        }
                    }
                    // Level treads can meet a bank with crossfall. Close both shoulders
                    // of the landing as it tapers down to the existing terrain.
                    auto apronLeft = bottom.front(), apronRight = bottom.back();
                    apronLeft.y = apronRight.y = base;
                    auto normal = ( apronRight - apronLeft ).crossProduct( outer.front() - apronLeft ).normaliseCopy();
                    if( normal.y < 0 ) normal = -normal;
                    stairs.quad( apronLeft, apronRight, outer.back(), outer.front(), normal );
                    cheek( bottom.front(), outer.front(), apronLeft, outer.front(), -along );
                    cheek( bottom.back(), outer.back(), apronRight, outer.back(), along );
                    const bool railed = cityEntranceHandrails::append(
                        handrails, entry.lot->id, { floor, along, outward, half, start, landing, tread, riser, count },
                        surface, clear );
                    return { end + apronRun, outer[1].y, count, railed };
                }
                return {};
            }

            void buildFoundationFacing( const OpenCityLayout &city, const render::TerrainData &terrain,
                                        const Transform3<real_Num> &placement, Geometry ( &courses )[2],
                                        Geometry &caps )
            {
                const size_t piecesPerLot = std::min<size_t>( 192, 18000 / std::max<size_t>( city.lots.size(), 1 ) );
                for( const auto &lot : city.lots )
                {
                    if( lot.park || piecesPerLot == 0 ) continue;
                    const float top = visibleFoundationTop( lot ), bottom = lot.groundMinimumHeight - .25f;
                    const Vector3F centre( lot.parcelX, top, lot.parcelZ );
                    const float height = .248f + .079f * stoneVariation( lot.id + 31 );
                    const float capHeight = .096f + .041f * stoneVariation( lot.id + 73 );
                    size_t pieces = 0;
                    for( unsigned row = 0; row < 14 && pieces < piecesPerLot; ++row )
                    {
                        const float high = row == 0 ? top : top - capHeight - ( row - 1 ) * height - .008f;
                        const float low = std::max( bottom, row == 0 ? top - capHeight : high - height + .018f );
                        if( high - low < .05f ) break;
                        for( const int axis : { 0, 1 } )
                            for( const int sign : { -1, 1 } )
                            {
                                const Vector3F normal( axis ? float( sign ) : 0, 0, axis ? 0 : float( sign ) );
                                const Vector3F along( axis ? 0 : 1, 0, axis ? 1 : 0 );
                                const float halfSpan = ( axis ? lot.parcelDepth : lot.parcelWidth ) * .5f;
                                const float face = ( axis ? lot.parcelWidth : lot.parcelDepth ) * .5f;
                                const float pitch = ( row == 0 ? 1.26f : .69f ) + .31f * stoneVariation( lot.id + 19 + axis );
                                const float phase = row % 2 ? .52f * pitch : 0.f;
                                for( unsigned piece = 0; pieces < piecesPerLot; ++piece )
                                {
                                    const float s0 = -halfSpan + pitch * piece - phase;
                                    if( s0 >= halfSpan ) break;
                                    const auto key = lot.id ^ ( ( u64( axis * 2 + ( sign > 0 ) ) << 29 ) |
                                                                ( u64( row ) << 20 ) | piece );
                                    const float relief = row == 0 ? .021f : .011f + .007f * stoneVariation( key + 1 );
                                    const float left = std::max( -halfSpan, s0 + .008f );
                                    const float right = std::min( halfSpan, s0 + pitch - .008f );
                                    if( right - left < .08f ) continue;
                                    const auto side = centre + normal * ( face + relief );
                                    const auto a = side + along * left, b = side + along * right;
                                    float h0 = 0, h1 = 0, mid = 0;
                                    if( !render::sampleTerrainHeight( terrain, placement, a, h0 ) ||
                                        !render::sampleTerrainHeight( terrain, placement, b, h1 ) ||
                                        !render::sampleTerrainHeight( terrain, placement, ( a + b ) * .5f, mid ) ||
                                        high - std::min( { h0, h1, mid } ) < .052f ) continue;
                                    const float u = .05f + .29f * stoneVariation( key + 2 );
                                    const float v = .13f + .57f * stoneVariation( key + 3 );
                                    const float frequency = .42f;
                                    auto &geometry = row == 0 ? caps : courses[stoneVariation( key + 4 ) < .30f ? 1 : 0];
                                    geometry.quad( { a.x, low, a.z }, { b.x, low, b.z }, { b.x, high, b.z },
                                                   { a.x, high, a.z }, normal, u, u + ( right - left ) * frequency,
                                                   v, v + ( high - low ) * frequency );
                                    ++pieces;
                                }
                            }
                    }
                }
            }

            void addWalkCollision( SmartPtr<scene::IGameActor> actor )
            {
                actor->setStatic( true );
                const auto resource = actor->getComponent<scene::Mesh>()->getMeshResource();
                auto collision = actor->addComponent<CollisionMesh>();
                if( !collision ) throw std::runtime_error( "Cannot create terrain-city pedestrian collision." );
                collision->setConvex( false );
                collision->setMeshResource( resource );
                if( auto shape = dynamic_pointer_cast<physics::IMeshShape>( collision->getShape() ) )
                    shape->setMeshResource( resource );
                if( !actor->addComponent<Rigidbody>() )
                    throw std::runtime_error( "Cannot create terrain-city pedestrian rigidbody." );
            }

            void build( SceneAssets &assets, const SourceBudgetVector<Entry> &entries,
                        CityRoadTerrainMesh &landscapeClearance, SmartPtr<render::IMaterial> paving,
                        SmartPtr<ITextureForge> forge, const cityRoadEdges::Surface *roadSurface,
                        SmartPtr<IMesh> sidewalks, u32 seed, SourceBudgetVector<cityRoadEdges::Bounds> &pedestrianCorridors )
            {
                if( entries.empty() ) return;
                const auto terrain = assets.roadAuthoring->getTerrainSnapshot();
                if( !terrain ) return;
                const auto &placement = assets.roadAuthoring->getTerrainPlacement();
                const WalkEdges edges( landscapeClearance );
                const cityRoadEdges::Surface sidewalkSurface( sidewalks );
                struct Bounds { float x0, x1, z0, z1; };
                SourceBudgetVector<Bounds> roadBounds;
                const auto addRoadBounds = [&]( Vector3F a, Vector3F b, float radius ) {
                    roadBounds.push_back( { std::min( a.x, b.x ) - radius, std::max( a.x, b.x ) + radius,
                                            std::min( a.z, b.z ) - radius, std::max( a.z, b.z ) + radius } );
                };
                const auto &source = assets.roadAuthoring->getSource();
                const auto &roads = assets.roadAuthoring->getBake().roads;
                if( source.roads.size() != roads.size() ) return;
                for( size_t i = 0; i < roads.size(); ++i )
                {
                    const auto &points = roads[i].points;
                    for( size_t j = 1; j < points.size(); ++j )
                        addRoadBounds( points[j - 1].position, points[j].position, source.roads[i].width * .5f + .12f );
                }
                for( const auto &junction : source.roundabouts )
                {
                    addRoadBounds( junction.center, junction.center, junction.outerRadius );
                    for( const auto &port : junction.ports )
                    {
                        const auto start = junction.center + port.outward * junction.outerRadius;
                        addRoadBounds( start, start + port.outward * port.length, port.width * .5f + .12f );
                    }
                }
                const auto parcelClear = [&]( const Bounds &bounds, const OpenCityLayout::Lot *exempt ) {
                    return std::none_of( assets.cityLayout.lots.begin(), assets.cityLayout.lots.end(),
                        [&]( const auto &other ) {
                            return ( !exempt || other.id != exempt->id ) &&
                                   bounds.x0 < other.parcelX + other.parcelWidth * .5f &&
                                   bounds.x1 > other.parcelX - other.parcelWidth * .5f &&
                                   bounds.z0 < other.parcelZ + other.parcelDepth * .5f &&
                                   bounds.z1 > other.parcelZ - other.parcelDepth * .5f;
                        } );
                };
                const auto footprintClear = [&]( Vector3F a, Vector3F b, const OpenCityLayout::Lot *exempt ) {
                    const auto x0 = std::min( a.x, b.x ), x1 = std::max( a.x, b.x );
                    const auto z0 = std::min( a.z, b.z ), z1 = std::max( a.z, b.z );
                    return std::none_of( roadBounds.begin(), roadBounds.end(), [&]( const Bounds &road ) {
                        return x0 < road.x1 && x1 > road.x0 && z0 < road.z1 && z1 > road.z0;
                    } ) && parcelClear( { x0, x1, z0, z1 }, exempt );
                };
                Geometry approach, gravel, stoneEdge, courses[2], caps, risers, stairs;
                cityPedestrianSurfacing::Batches routedPaving;
                cityEntranceHandrails::Batches handrails;
                buildFoundationFacing( assets.cityLayout, *terrain, placement, courses, caps );
                FrontagePlanting planting( assets.cityStreetDetail );
                const Vector3F up( 0, 1, 0 );
                const auto surface = [&]( Vector3F &p ) {
                    if( !render::sampleTerrainHeight( *terrain, placement, p, p.y ) ) return false;
                    p.y += .028f;
                    return true;
                };
                size_t contacts = 0, connections = 0, raised = 0, routed = 0, bends = 0;
                for( const auto &entry : entries )
                {
                    const auto &lot = *entry.lot;
                    const auto normal = entry.outward, along = entry.along;
                    const Vector3F centre( lot.parcelX, entry.door.y, lot.parcelZ );
                    const Vector3F extent( lot.parcelWidth * .5f, 0, lot.parcelDepth * .5f );
                    const auto relative = entry.door - centre;
                    const float depth = std::max( 0.f, std::abs( extent.dotProduct( normal ) ) - relative.dotProduct( normal ) );
                    const float crossHalf = extent.dotProduct( along ), across = relative.dotProduct( along );
                    const float leftRoom = crossHalf + across - .035f, rightRoom = crossHalf - across - .035f;
                    const float half = std::min( { std::clamp( entry.halfWidth + .24f, .64f, 1.3f ),
                                                  leftRoom, rightRoom } );
                    if( half < .52f ) continue;
                    const auto floor = entry.door + up * .167f;
                    const auto end = floor + normal * depth;
                    const auto groundClear = [&]( Vector3F a, Vector3F b ) {
                        return footprintClear( a, b, &lot );
                    };
                    const auto landing = lot.district == OpenCityLayout::District::Commercial
                        ? buildRaisedApproach( entry, depth, half, floor, surface, groundClear, stairs, handrails )
                        : RaisedApproach{};
                    if( landing.end > 0 )
                    {
                        ++raised;
                        const float reserved = half + ( landing.handrails ? cityEntranceHandrails::clearance : .12f );
                        planting.reserveAccess( entry.door - along * reserved,
                                                 entry.door + normal * landing.end + along * reserved );
                        WP_LOG( String( "Terrain city raised entrance lot=" ) + StringUtil::toString( lot.id ) +
                                ", risers=" + StringUtil::toString( landing.risers ) );
                    }
                    const auto strip = [&]( Geometry &geometry, float left, float right, float start,
                                            float finish, float lift ) {
                        if( right - left < .035f || finish - start < .035f ) return;
                        const auto p = floor + up * lift;
                        geometry.quad( p + along * left + normal * start, p + along * right + normal * start,
                                       p + along * right + normal * finish, p + along * left + normal * finish, up );
                    };

                    const auto groundStrip = [&]( Geometry &geometry, float left, float right,
                                                   float begin, float finish, bool threshold = false ) {
                        if( right - left < .035f || finish - begin < .055f ) return;
                        const auto front = entry.door + normal * begin;
                        if( !groundClear( front + along * left, entry.door + along * right + normal * finish ) ) return;
                        const int sections = std::max( 1, int( std::ceil( ( right - left ) / .88f ) ) );
                        for( int step = 0; step < sections; ++step )
                        {
                            const float s0 = left + ( right - left ) * ( float( step ) / sections );
                            const float s1 = left + ( right - left ) * ( float( step + 1 ) / sections );
                            auto a = front + along * s0, b = front + along * s1;
                            auto c = entry.door + along * s1 + normal * finish;
                            auto d = entry.door + along * s0 + normal * finish;
                            const auto walkGap = edges.reach( ( a + b ) * .5f, normal );
                            if( walkGap >= 0 && walkGap < finish - begin + .055f ) continue;
                            if( !surface( a ) || !surface( b ) || !surface( c ) || !surface( d ) ) continue;
                            auto diagonal = ( a + c ) * .5f;
                            const float interpolated = diagonal.y;
                            if( !surface( diagonal ) || std::abs( diagonal.y - interpolated ) >= .026f ) continue;
                            auto n = ( b - a ).crossProduct( c - a ).normaliseCopy();
                            if( n.y < 0 ) n = -n;
                            if( n.y <= .84f ) continue;
                            geometry.quad( a, b, c, d, n );
                            if( !threshold ) continue;
                            // Close the visual recess at the building side of an
                            // emitted apron, below the aperture's original sill.
                            auto lowA = a - normal * .004f;
                            auto lowB = b - normal * .004f;
                            const float padTop = visibleFoundationTop( lot );
                            lowA.y = std::max( padTop, a.y - .012f );
                            lowB.y = std::max( padTop, b.y - .012f );
                            const float high = floor.y - .012f;
                            if( high - std::max( lowA.y, lowB.y ) < .029f ||
                                high - std::min( lowA.y, lowB.y ) > .24f ) continue;
                            risers.quad( lowA, lowB, { lowB.x, high, lowB.z }, { lowA.x, high, lowA.z },
                                         normal, .24f, .24f + ( s1 - s0 ) * .42f, .31f, .43f );
                        }
                    };
                    // Setback foundations have a continuous flush landing. In zero-setback
                    // natural lots the drain and apron meet the foot of the retaining face.
                    const bool setback = depth >= .08f;
                    const float bedDepth = .66f + .17f * float( lot.variant % 4 );
                    if( setback )
                    {
                        strip( approach, -half, half, .009f, depth, 0 );
                        strip( stoneEdge, -half, half, .009f, std::min( .22f, depth ), .006f );
                    }
                    else if( !landing.end )
                    {
                        groundStrip( stoneEdge, -half, half, .020f, .24f, true );
                    }
                    const float faceLeft = std::max( -leftRoom, -entry.span * .5f - entry.doorOffset + .19f );
                    const float faceRight = std::min( rightRoom, entry.span * .5f - entry.doorOffset - .19f );
                    planting.offer( lot, entry.door, along, normal, half, faceLeft, faceRight, depth, bedDepth,
                                    [&]( Vector3F point ) { return edges.reach( point, normal ); }, groundClear );
                    for( const auto &range : { Vector2F( faceLeft, -half - .04f ),
                                               Vector2F( half + .04f, faceRight ) } )
                    {
                        if( setback )
                        {
                            const float length = std::min( depth - .04f, bedDepth );
                            strip( gravel, range.x, range.y, .022f, length, -.003f );
                            strip( stoneEdge, range.x, range.y, std::max( .024f, length - .072f ), length, .002f );
                        }
                        else
                        {
                            groundStrip( gravel, range.x, range.y, .032f, bedDepth );
                            groundStrip( stoneEdge, range.x, range.y, bedDepth, bedDepth + .075f );
                        }
                    }
                    ++contacts;

                    // Verify both margins and the centre reach the same outward face of the
                    // walk without crossing any other parcel between the door and the curb.
                    const auto connectStraight = [&]() -> bool {
                        const auto left = end - along * half, right = end + along * half;
                        const float l = edges.reach( left, normal ), r = edges.reach( right, normal );
                        const float m = edges.reach( end, normal );
                        if( std::min( { l, r, m } ) < .14f || std::abs( l - r ) > 2.8f ||
                            std::abs( ( l + r ) * .5f - m ) > .27f ) return false;
                        // A narrow reveal prevents coincident triangles against the fitted kerb, even
                        // where the outer edge curves between the three sampled rays.
                        const float span = std::max( l, r );
                        if( !groundClear( left, right + normal * span ) ) return false;

                        const int segments = std::max( 1, int( std::ceil( span / .85f ) ) );
                        std::array<std::pair<Vector3F, Vector3F>, 21> rows;
                        const float approachStart = landing.end > 0 ? landing.end - depth : setback ? .015f : .24f;
                        const float startHeight = landing.end > 0 ? landing.height : floor.y;
                        if( std::min( l, r ) - .10f <= approachStart ) return false;
                        bool grounded = true;
                        for( int row = 0; row <= segments; ++row )
                        {
                            const float t = float( row ) / segments;
                            auto a = left + normal * ( approachStart + ( l - .10f - approachStart ) * t );
                            auto b = right + normal * ( approachStart + ( r - .10f - approachStart ) * t );
                            auto centre = ( a + b ) * .5f;
                            grounded = surface( a ) && surface( b ) && surface( centre ) &&
                                       std::abs( ( a.y + b.y ) * .5f - centre.y ) < .025f;
                            if( row == 0 )
                                grounded = grounded && startHeight >= centre.y - .05f && startHeight - centre.y < .39f;
                            else
                            {
                                const auto &last = rows[size_t( row - 1 )];
                                auto midpoint = ( last.first + b ) * .5f;
                                const float interpolated = midpoint.y;
                                grounded = grounded && surface( midpoint ) &&
                                           std::abs( midpoint.y - interpolated ) < .025f &&
                                           std::abs( last.first.y - a.y ) < .28f && std::abs( last.second.y - b.y ) < .28f;
                            }
                            if( !grounded ) break;
                            rows[size_t( row )] = { a, b };
                        }
                        if( !grounded ) return false;
                        for( int row = 1; row <= segments; ++row )
                        {
                            const auto &[a, b] = rows[size_t( row - 1 )];
                            const auto &[c, d] = rows[size_t( row )];
                            auto n = ( b - a ).crossProduct( d - a );
                            n.normalise();
                            if( n.y < 0 ) n = -n;
                            approach.quad( a, b, d, c, n );
                        }
                        planting.reserveAccess( left - along * .16f, right + along * .16f + normal * span );
                        ++connections;
                        return true;
                    };
                    if( connectStraight() || !landing.end || !roadSurface || !sidewalks ) continue;

                    const auto toe = entry.door + normal * landing.end;
                    cityEntranceRoute::Row launch{ toe - along * half, toe, toe + along * half };
                    if( !surface( launch.left ) || !surface( launch.right ) ) continue;
                    launch.centre.y = ( launch.left.y + launch.right.y ) * .5f;
                    const auto overlaps = []( const cityRoadEdges::Surface &layer, const auto &quad ) {
                        return layer.overlaps( { quad[0], quad[1], quad[2] } ) ||
                               layer.overlaps( { quad[0], quad[2], quad[3] } );
                    };
                    const auto clear = [&]( std::array<Vector3F, 4> quad, bool terminal ) {
                        const cityRoadEdges::Bounds bounds( quad );
                        if( !parcelClear( { bounds.x0, bounds.x1, bounds.z0, bounds.z1 }, &lot ) ||
                            overlaps( *roadSurface, quad ) ||
                            std::any_of( pedestrianCorridors.begin(), pedestrianCorridors.end(),
                                         [&]( const auto &other ) { return bounds.intersects( other ); } ) ) return false;
                        // Permit the last row to share the kerb's upper rim. The clearance
                        // test ends 21 mm short of that seam, leaving the rendered row flush.
                        if( terminal )
                        {
                            quad[2] += cityEntranceRoute::horizontal( quad[1] - quad[2] ).normaliseCopy() * .021f;
                            quad[3] += cityEntranceRoute::horizontal( quad[0] - quad[3] ).normaliseCopy() * .021f;
                        }
                        return !overlaps( sidewalkSurface, quad );
                    };
                    cityEntranceRoute::Route route;
                    if( !cityEntranceRoute::plan( edges, launch, normal, std::min( .84f, half ), clear, surface, route ) ||
                        !cityPedestrianSurfacing::append( routedPaving, route, surface ) ) continue;

                    for( size_t row = 1; row < route.rowCount; ++row )
                    {
                        const auto &a = route.rows[row - 1], &b = route.rows[row];
                        const cityRoadEdges::Bounds stripBounds( std::array{ a.left, a.right, b.left, b.right } );
                        const Vector3F low( stripBounds.x0 - .12f, 0, stripBounds.z0 - .12f );
                        const Vector3F high( stripBounds.x1 + .12f, 0, stripBounds.z1 + .12f );
                        planting.reserveAccess( low, high );
                        pedestrianCorridors.emplace_back( std::array{ low, high } );
                    }
                    ++routed;
                    bends += route.bends;
                    const auto position = []( Vector3F p ) {
                        return StringUtil::toString( p.x ) + "/" + StringUtil::toString( p.y ) + "/" + StringUtil::toString( p.z );
                    };
                    WP_LOG( String( "Terrain city pedestrian connector lot=" ) + StringUtil::toString( lot.id ) +
                            ", rows=" + StringUtil::toString( route.rowCount ) + ", bends=" + StringUtil::toString( route.bends ) +
                            ", length=" + StringUtil::toString( route.length ) + ", start=" + position( launch.centre ) +
                            ", end=" + position( route.rows[route.rowCount - 1].centre ) );
                }

                // Approaches through occupied neighbourhoods can have an open verge
                // alongside a building whose door addresses a different street.
                // Put small courts there only where the real walk edge leaves room.
                size_t portNumber = 0;
                for( const auto &junction : source.roundabouts )
                    for( const auto &port : junction.ports )
                    {
                        ++portNumber;
                        if( port.length < 10.f ) continue;
                        const auto along = Vector3F( port.outward.x, 0, port.outward.z ).normaliseCopy();
                        const Vector3F side( -along.z, 0, along.x );
                        const auto point = junction.center + along * ( junction.outerRadius + port.length * .49f );
                        for( const int sign : { -1, 1 } )
                            planting.offerStreetCourt( point, along, side * float( sign ), port.width * .5f,
                                render::foliageStableHash( ( u64( portNumber ) << 2 ) | u64( sign > 0 ) ),
                                [&]( Vector3F p, Vector3F outward ) { return edges.reach( p, outward, true ); },
                                [&]( Vector3F a, Vector3F b ) {
                                    if( !footprintClear( a, b, nullptr ) ) return false;
                                    const auto centre = ( a + b ) * .5f;
                                    return std::any_of( entries.begin(), entries.end(), [&]( const auto &entry ) {
                                        const auto &lot = *entry.lot;
                                        const float dx = std::max( 0.f, std::abs( centre.x - lot.x ) - lot.width * .5f );
                                        const float dz = std::max( 0.f, std::abs( centre.z - lot.z ) - lot.depth * .5f );
                                        return dx * dx + dz * dz <= 16.f * 16.f;
                                    } );
                                } );
                    }

                const auto pavingVariant = [&]( ColourF tint, float scale ) {
                    auto surface = material( assets, tint, paving->getRoughness(), paving->getMetalness() );
                    for( const u32 slot : { 0u, 1u, 2u, 3u, 22u } )
                        if( auto texture = paving->getTexture( slot ) ) surface->setTexture( texture, slot );
                    surface->setUVProjection( 1 );
                    surface->setTriplanarScale( scale );
                    surface->setNormalStrength( .24f );
                    surface->setEditorFloat( "aoStrength", .42f );
                    surface->setEditorUInt( "uvFilter", 3 );
                    surface->setEditorFloat( "uvAniso", 8.f );
                    return surface;
                };
                const auto publish = [&]( const char *name, const Geometry &geometry, const auto &surface ) {
                    if( auto actor = mesh( assets, name, geometry, surface ) )
                        actor->getComponent<scene::MeshRenderer>()->setCastShadows( scene::Renderer::CastShadows::Off );
                };
                if( !approach.indices.empty() )
                    publish( "Terrain city building entrance paving", approach,
                             pavingVariant( { .82f, .84f, .80f, 1 }, .86f ) );
                if( !stairs.indices.empty() )
                {
                    auto actor = mesh( assets, "Terrain city raised entrance steps", stairs,
                                       pavingVariant( { .73f, .75f, .71f, 1 }, 1.0f ) );
                    if( !actor ) throw std::runtime_error( "Cannot publish terrain-city entrance steps." );
                    addWalkCollision( actor );
                }
                cityEntranceHandrails::publish( assets, handrails );
                for( auto actor : cityPedestrianSurfacing::publish( assets, routedPaving, seed ) )
                    if( actor ) addWalkCollision( actor );
                if( !stoneEdge.indices.empty() )
                    publish( "Terrain city building threshold joints", stoneEdge,
                             pavingVariant( { .56f, .55f, .50f, 1 }, 1.10f ) );
                SmartPtr<render::IMaterial> firstStone;
                const auto publishStone = [&]( const char *name, const Geometry &geometry, ColourF tint,
                                                float normalStrength, bool projected = false ) {
                    if( geometry.indices.empty() ) return;
                    auto surface = material( assets, tint, .94f );
                    if( !firstStone )
                    {
                        SurfaceBakeParams params;
                        params.size = 128;
                        const auto stone = forge->bakeSurface( SurfaceTag::Stone, params );
                        maps( assets, surface, stone.albedo, stone.normal, stone.orm );
                        firstStone = surface;
                    }
                    else
                    {
                        for( const u32 slot : { 0u, 1u, 2u, 3u, 22u } )
                            if( auto texture = firstStone->getTexture( slot ) ) surface->setTexture( texture, slot );
                        surface->setRoughness( firstStone->getRoughness() );
                        surface->setMetalness( firstStone->getMetalness() );
                    }
                    surface->setDiffuse( tint );
                    if( projected )
                    {
                        surface->setUVProjection( 1 );
                        surface->setTriplanarScale( 2.7f );
                    }
                    surface->setNormalStrength( normalStrength );
                    publish( name, geometry, surface );
                };
                publishStone( "Terrain city building gravel drainage beds", gravel, { .63f, .58f, .49f, 1 }, .22f, true );
                publishStone( "Terrain city building stone plinth courses", courses[0], { .81f, .76f, .67f, 1 }, .32f );
                publishStone( "Terrain city building stone bond variation", courses[1], { .69f, .66f, .59f, 1 }, .29f );
                publishStone( "Terrain city building stone cap courses", caps, { .88f, .84f, .76f, 1 }, .24f );
                publishStone( "Terrain city building threshold risers", risers, { .77f, .73f, .66f, 1 }, .18f );
                planting.publish( assets, landscapeClearance, *terrain, placement );

                // The fitted footway's render mesh and collider have already been published.
                // Feed the exact entrance and drainage triangles into later landscape passes.
                for( const auto *surface : { &approach, &stoneEdge, &gravel, &stairs, &routedPaving.paving,
                                            &routedPaving.edging, &routedPaving.drainage } )
                {
                    const auto base = u32( landscapeClearance.vertices.size() );
                    landscapeClearance.vertices.reserve( base + surface->positions.size() );
                    for( size_t i = 0; i < surface->positions.size(); ++i )
                        landscapeClearance.vertices.push_back( { surface->positions[i], surface->normals[i], {} } );
                    landscapeClearance.indices.reserve( landscapeClearance.indices.size() + surface->indices.size() );
                    for( const auto index : surface->indices )
                        landscapeClearance.indices.push_back( base + index );
                    landscapeClearance.triangleKinds.insert( landscapeClearance.triangleKinds.end(),
                        surface->indices.size() / 3, CityRoadTerrainTriangleKind::Kerb );
                }
                WP_LOG( String( "Terrain city pedestrian connectors=" ) + StringUtil::toString( routed ) +
                        ", bends=" + StringUtil::toString( bends ) );
                WP_LOG( String( "Terrain city building contacts=" ) + StringUtil::toString( contacts ) +
                        " fittedWalkConnections=" + StringUtil::toString( connections ) +
                        " raisedApproaches=" + StringUtil::toString( raised ) +
                        " stonePlinthPieces=" + StringUtil::toString( ( courses[0].indices.size() +
                            courses[1].indices.size() + caps.indices.size() ) / 6 ) +
                        " thresholdRisers=" + StringUtil::toString( risers.indices.size() / 6 ) );
            }
        }  // namespace cityGroundContact
