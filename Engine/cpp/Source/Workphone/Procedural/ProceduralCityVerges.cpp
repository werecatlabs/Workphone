#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::race
{
    using namespace sceneBuild;

    namespace
    {
        #include "Workphone/Scene/Components/CityVergeTransitions.hpp"
    }

    namespace sceneBuild
    {
        void buildTerrainCityVerges( SceneAssets &assets, const CityRoadTerrainMesh &walks, u32 seed,
                                     VehicleAppearanceQuality quality )
        {
            const auto ground = assets.roadAuthoring->getTerrainSnapshot();
            if( !ground )
            {
                return;
            }
            const auto &placement = assets.roadAuthoring->getTerrainPlacement();
            const auto &source = assets.roadAuthoring->getSource();
            const auto &roads = assets.roadAuthoring->getBake().roads;
            if( source.roads.size() != roads.size() )
            {
                return;
            }
            const auto scalar = []( u64 key ) {
                return float( render::foliageStableHash( key ) >> 40 ) / 16777216.f;
            };
            const auto locationKey = [seed]( Vector3F point ) {
                const auto x = static_cast<u32>( static_cast<s32>( std::floor( point.x * 2 ) ) );
                const auto z = static_cast<u32>( static_cast<s32>( std::floor( point.z * 2 ) ) );
                return render::foliageStableHash( ( u64( x ) << 32 ) | z ) ^ seed;
            };
            const auto distanceSquared = []( Vector3F p, Vector3F a, Vector3F b ) {
                const float x = b.x - a.x, z = b.z - a.z;
                const float d = x * x + z * z;
                const float t =
                    d > 0 ? std::clamp( ( ( p.x - a.x ) * x + ( p.z - a.z ) * z ) / d, 0.f, 1.f ) : 0;
                const float rx = p.x - ( a.x + t * x ), rz = p.z - ( a.z + t * z );
                return rx * rx + rz * rz;
            };
            enum class VergePlant
            {
                Meadow,
                Fescue,
                GroundCover,
                Bramble,
                Heather
            };
            const auto woody = []( VergePlant plant ) {
                return plant == VergePlant::Bramble || plant == VergePlant::Heather;
            };

            struct Site
            {
                Vector3F position;
                u64 key;
                float radius;
                VergePlant plant;
                bool blocked = false;
            };

            const bool preview = quality == VehicleAppearanceQuality::Preview;
            const size_t limit = preview ? 580 : 1580;
            SourceBudgetVector<Site> sites;
            sites.reserve( limit );
            const auto plant = [&]( Vector3F centre, Vector3F along, float roadWidth, u64 key,
                                    bool outer, bool terminal ) {
                const Vector3F side( -along.z, 0, along.x );
                for( const float sign : { -1.f, 1.f } )
                {
                    const auto sideKey = key ^ ( sign > 0 ? 0xdfe493b9u : 0xbc6b2fbdu );
                    const float density = scalar( sideKey );
                    if( !terminal && density < ( outer ? .12f : .44f ) )
                    {
                        continue;
                    }
                    const unsigned members =
                        ( terminal ? 8u : 4u ) + unsigned( density * ( outer ? 7.f : 4.f ) );
                    const float drift = ( scalar( sideKey + 11 ) - .5f ) * ( outer ? 4.2f : 2.4f );
                    const float patchDepth = scalar( sideKey + 17 ) * ( outer ? 3.3f : 1.3f );
                    const float stretch = terminal ? 8.8f : 3.8f + 6.2f * density;
                    const float habitat = scalar( sideKey + 23 );
                    for( unsigned i = 0; i < members && sites.size() < limit; ++i )
                    {
                        const auto variation = render::foliageStableHash( sideKey + i );
                        const float mix = scalar( variation );
                        VergePlant species = habitat < .34f ? VergePlant::Fescue : VergePlant::Meadow;
                        if( mix < .26f )
                        {
                            species = VergePlant::GroundCover;
                        }
                        if( outer && i >= 3 && mix > .75f )
                        {
                            species = habitat > .43f ? VergePlant::Bramble : VergePlant::Heather;
                        }
                        const auto size = scalar( variation + 3 );
                        const float radius = woody( species ) ? .58f + .85f * size : .22f + .45f * size;
                        // The first members form a low kerb fringe. The rest
                        // drift outward in an irregular patch, with scrub set back.
                        const float patch = i < 3 ? .48f * patchDepth : patchDepth;
                        const float swale = outer && density > .62f && i >= 3
                                                ? 1.2f + 2.3f * scalar( sideKey + 28 )
                                                : 0.f;
                        const float across = roadWidth * .5f + 2.63f + radius + patch +
                                             ( woody( species ) ? 1.6f : 0.f ) + swale +
                                             scalar( variation + 1 ) * ( i < 3 ? 1.7f : 3.2f );
                        const float offset = drift + ( scalar( variation + 2 ) - .5f ) * stretch;
                        auto point = centre + side * ( sign * across ) + along * offset;
                        if( !render::sampleTerrainHeight( *ground, placement, point, point.y ) )
                        {
                            continue;
                        }
                        bool supported = true;
                        for( const auto direction : { Vector3F( 1, 0, 0 ), Vector3F( -1, 0, 0 ),
                                                      Vector3F( 0, 0, 1 ), Vector3F( 0, 0, -1 ) } )
                        {
                            float height = 0;
                            supported = supported &&
                                        render::sampleTerrainHeight(
                                            *ground, placement, point + direction * radius, height ) &&
                                        std::abs( height - point.y ) < .42f * radius;
                        }
                        if( supported )
                        {
                            sites.push_back( { point, variation, radius, species } );
                        }
                    }
                }
            };
            const float outerRadius = assets.cityLayout.extent * .76f;
            // A roundabout owns its approach pieces, so they never receive the
            // ordinary spline station pass below. Fill their supported outer verges
            // with irregular drifts as well as the occasional formal planting bed.
            const auto streetDetail = std::clamp( assets.cityStreetDetail, 0, 100 );
            const size_t approachLimit = std::min( limit / 5, size_t( 3 * streetDetail ) );
            for( const auto &junction : source.roundabouts )
            {
                for( const auto &port : junction.ports )
                {
                    if( sites.size() >= approachLimit )
                    {
                        break;
                    }
                    if( port.length < 10.f )
                    {
                        continue;
                    }
                    const auto direction = Vector3F( port.outward.x, 0, port.outward.z ).normaliseCopy();
                    const auto point =
                        junction.center + direction * ( junction.outerRadius + port.length * .63f );
                    plant( point, direction, port.width, locationKey( point ) ^ 0x70a7bd31u, false,
                           true );
                }
                if( sites.size() >= approachLimit )
                {
                    break;
                }
            }
            // Give the remaining budget to the outskirts first, including the last few
            // metres of a free approach viewed from outside the city.
            for( bool endsOnly : { true, false } )
            {
                for( size_t r = 0; r < roads.size() && sites.size() < limit; ++r )
                {
                    const auto &points = roads[r].points;
                    if( points.size() < 2 )
                    {
                        continue;
                    }
                    if( endsOnly )
                    {
                        if( source.roads[r].closed )
                        {
                            continue;
                        }
                        for( unsigned end = 0; end < 2; ++end )
                        {
                            const auto &sample = end ? points.back() : points.front();
                            const auto &p = sample.position;
                            if( std::max( std::abs( p.x ), std::abs( p.z ) ) < outerRadius )
                            {
                                continue;
                            }
                            Vector3F inward( sample.tangent.x, 0, sample.tangent.z );
                            inward.normalise();
                            inward *= end ? -1.f : 1.f;
                            // Keep the endpoint's forward vehicle approach clear.
                            for( unsigned group = 0; group < 3; ++group )
                            {
                                plant( p + inward * ( 1.6f + group * 7.5f ), inward,
                                       source.roads[r].width,
                                       locationKey( p ) ^ ( u64( group + 1 ) << 19 ), true, true );
                            }
                        }
                    }
                    else
                    {
                        float station = 9.f + 8.f * scalar( locationKey( points.front().position ) );
                        for( size_t j = 1; j < points.size() && sites.size() < limit; ++j )
                        {
                            const auto &a = points[j - 1], &b = points[j];
                            const float length = b.distance - a.distance;
                            if( length <= 0 )
                            {
                                continue;
                            }
                            Vector3F direction( b.position.x - a.position.x, 0,
                                                b.position.z - a.position.z );
                            if( direction.lengthSquared() < 1e-8f )
                            {
                                continue;
                            }
                            direction.normalise();
                            station = std::max( station, a.distance );
                            while( station <= b.distance && sites.size() < limit )
                            {
                                const float t = ( station - a.distance ) / length;
                                const auto p = a.position + ( b.position - a.position ) * t;
                                const bool outer =
                                    std::max( std::abs( p.x ), std::abs( p.z ) ) >= outerRadius;
                                const auto key = locationKey( p ) ^ 0x93c9e1f0u;
                                plant( p, direction, source.roads[r].width, key, outer, false );
                                station +=
                                    ( outer ? 8.f : 21.f ) + ( outer ? 16.f : 13.f ) * scalar( key );
                            }
                        }
                    }
                }
            }

            constexpr float bucketSize = 12.f;
            constexpr float maxRadius = 1.65f;  // Largest plant plus separation from paving.
            using Bucket = std::pair<int, int>;
            SourceBudgetMap<Bucket, SourceBudgetVector<size_t>> buckets;
            const auto cell = [bucketSize]( float value ) {
                return int( std::floor( value / bucketSize ) );
            };
            for( size_t i = 0; i < sites.size(); ++i )
            {
                buckets[{ cell( sites[i].position.x ), cell( sites[i].position.z ) }].push_back( i );
            }
            const auto visit = [&]( float minX, float minZ, float maxX, float maxZ, const auto &test ) {
                // Probe only populated rows. Long source segments cannot turn a
                // sparse planting pass into a scan over empty world cells.
                const int x0 = cell( minX ), x1 = cell( maxX ), z0 = cell( minZ ), z1 = cell( maxZ );
                for( auto entry = buckets.lower_bound( { x0, std::numeric_limits<int>::min() } );
                     entry != buckets.end() && entry->first.first <= x1; ++entry )
                {
                    if( entry->first.second >= z0 && entry->first.second <= z1 )
                    {
                        for( const auto index : entry->second )
                        {
                            if( !sites[index].blocked )
                            {
                                sites[index].blocked = test( sites[index] );
                            }
                        }
                    }
                }
            };
            for( size_t r = 0; r < roads.size(); ++r )
            {
                const auto &points = roads[r].points;
                const float reserve = source.roads[r].width * .5f + 2.45f;
                const float extent = reserve + maxRadius;
                for( size_t i = 1; i < points.size(); ++i )
                {
                    const auto a = points[i - 1].position, b = points[i].position;
                    visit( std::min( a.x, b.x ) - extent, std::min( a.z, b.z ) - extent,
                           std::max( a.x, b.x ) + extent, std::max( a.z, b.z ) + extent,
                           [&]( const Site &site ) {
                               const float clearance = reserve + site.radius;
                               return distanceSquared( site.position, a, b ) < clearance * clearance;
                           } );
                }
            }
            for( const auto &junction : source.roundabouts )
            {
                const auto centre = junction.center;
                const float reserve = junction.outerRadius + 2.45f;
                const float extent = reserve + maxRadius;
                visit( centre.x - extent, centre.z - extent, centre.x + extent, centre.z + extent,
                       [&]( const Site &site ) {
                           const float clearance = reserve + site.radius;
                           return distanceSquared( site.position, centre, centre ) <
                                  clearance * clearance;
                       } );
            }
            const auto cross = []( Vector3F a, Vector3F b, Vector3F p ) {
                return ( b.x - a.x ) * ( p.z - a.z ) - ( b.z - a.z ) * ( p.x - a.x );
            };
            for( size_t i = 0; i < walks.indices.size(); i += 3 )
            {
                const auto a = walks.vertices[walks.indices[i]].position;
                const auto b = walks.vertices[walks.indices[i + 1]].position;
                const auto c = walks.vertices[walks.indices[i + 2]].position;
                if( std::abs( cross( a, b, c ) ) < 1e-6f )
                {
                    continue;
                }
                visit( std::min( { a.x, b.x, c.x } ) - maxRadius,
                       std::min( { a.z, b.z, c.z } ) - maxRadius,
                       std::max( { a.x, b.x, c.x } ) + maxRadius,
                       std::max( { a.z, b.z, c.z } ) + maxRadius, [&]( const Site &site ) {
                           const auto p = site.position;
                           const auto u = cross( a, b, p ), v = cross( b, c, p ), w = cross( c, a, p );
                           const float radius = site.radius + .2f;
                           return ( u <= 0 && v <= 0 && w <= 0 ) || ( u >= 0 && v >= 0 && w >= 0 ) ||
                                  std::min( { distanceSquared( p, a, b ), distanceSquared( p, b, c ),
                                              distanceSquared( p, c, a ) } ) < radius * radius;
                       } );
            }

            Geometry grasses[2], seeds, leaves, blooms[2], stems;
            std::array<Geometry, 3> canopies;
            const auto triangle = []( Geometry &shape, Vector3F a, Vector3F b, Vector3F c,
                                      Vector3F normal ) {
                const auto tangent = ( b - a ).normaliseCopy();
                const auto offset = static_cast<u32>( shape.positions.size() );
                for( const auto point : { a, b, c } )
                {
                    shape.positions.push_back( point );
                    shape.normals.push_back( normal );
                    shape.tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
                }
                shape.uv.push_back( { 0, 0, 0 } );
                shape.uv.push_back( { 1, 0, 0 } );
                shape.uv.push_back( { .5f, 1, 0 } );
                const auto forward = ( b - a ).crossProduct( c - a ).dotProduct( normal ) >= 0;
                shape.indices.push_back( offset );
                shape.indices.push_back( offset + ( forward ? 1 : 2 ) );
                shape.indices.push_back( offset + ( forward ? 2 : 1 ) );
            };
            const auto fanLeaf = [&]( Vector3F base, Vector3F tip, Vector3F across, float rib ) {
                const auto middle = base + ( tip - base ) * .52f;
                const auto ridge = middle + Vector3F( 0, rib, 0 );
                const auto left = middle - across, right = middle + across;
                const auto forward = ( tip - base ).normaliseCopy();
                const auto normal = ( Vector3F( 0, 1, 0 ) - forward * forward.y ).normaliseCopy();
                const auto tilt = across.normaliseCopy() * .38f;
                triangle( leaves, base, left, ridge, ( normal - tilt ).normaliseCopy() );
                triangle( leaves, ridge, left, tip, ( normal - tilt ).normaliseCopy() );
                triangle( leaves, base, ridge, right, ( normal + tilt ).normaliseCopy() );
                triangle( leaves, ridge, tip, right, ( normal + tilt ).normaliseCopy() );
            };
            const auto canopyLobe = [&]( Geometry &shape, Vector3F centre, float radius, float height,
                                         float angle, u64 key ) {
                constexpr u32 sides = 8, stride = sides + 1;
                const auto first = static_cast<u32>( shape.positions.size() );
                const auto vertex = [&]( Vector3F point, Vector3F normal, Vector3F tangent, float u,
                                         float v ) {
                    shape.positions.push_back( point );
                    shape.normals.push_back( normal );
                    shape.tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
                    shape.uv.push_back( { u, v, 0 } );
                };
                for( u32 ring = 0; ring < 3; ++ring )
                {
                    const float latitude = ( float( ring ) - 1.f ) * ( pi * .25f );
                    const float slope = std::sin( latitude ), span = std::cos( latitude );
                    for( u32 side = 0; side <= sides; ++side )
                    {
                        const auto sector = side % sides;
                        const float azimuth = angle + sector * ( 2.f * pi / sides );
                        const Vector3F direction( std::cos( azimuth ), 0, std::sin( azimuth ) );
                        const float ripple = .88f + .12f * scalar( key + sector + ring * sides );
                        const auto point =
                            centre + direction * ( radius * span * ripple ) +
                            Vector3F( 0, height * ( slope + .09f * ( ripple - .94f ) ), 0 );
                        const auto normal =
                            ( direction * ( span * height ) + Vector3F( 0, radius * slope, 0 ) )
                                .normaliseCopy();
                        vertex( point, normal, { -direction.z, 0, direction.x }, float( side ) / sides,
                                .25f * ( ring + 1 ) );
                    }
                }
                for( u32 side = 0; side < sides; ++side )
                {
                    const auto pole = static_cast<u32>( shape.positions.size() );
                    const float u = ( side + .5f ) / sides;
                    const float azimuth = angle + 2.f * pi * u;
                    const Vector3F tangent( -std::sin( azimuth ), 0, std::cos( azimuth ) );
                    vertex( centre - Vector3F( 0, height, 0 ), { 0, -1, 0 }, tangent, u, 0 );
                    vertex( centre + Vector3F( 0, height, 0 ), { 0, 1, 0 }, tangent, u, 1 );
                    const u32 a = first + side, b = a + stride, c = b + stride;
                    for( const auto index : { pole, a, a + 1, a, b, a + 1, a + 1, b, b + 1, b, c, b + 1,
                                              b + 1, c, c + 1, c, pole + 1, c + 1 } )
                    {
                        shape.indices.push_back( index );
                    }
                }
            };
            const auto flower = [&]( Vector3F head, float spread, float angle, unsigned tint ) {
                for( unsigned petal = 0; petal < 5; ++petal )
                {
                    const float azimuth = angle + petal * ( 2.f * pi / 5.f );
                    const Vector3F direction( std::cos( azimuth ), 0, std::sin( azimuth ) );
                    const Vector3F side( -direction.z, 0, direction.x );
                    const auto centre = head + direction * ( spread * .63f );
                    const auto lip = centre + Vector3F( 0, spread * .2f, 0 );
                    blooms[tint].quad( head, centre + side * ( spread * .25f ),
                                       lip + direction * ( spread * .37f ),
                                       centre - side * ( spread * .25f ), { 0, 1, 0 } );
                }
            };

            struct PlantClearance
            {
                Vector3F position;
                float radius;
            };

            SourceBudgetVector<PlantClearance> planted;
            planted.reserve( sites.size() );
            size_t remainingTriangles = preview ? 45000 : 156000;
            for( const auto &site : sites )
            {
                const auto p = site.position;
                if( site.blocked || std::any_of( assets.cityLayout.lots.begin(),
                                                 assets.cityLayout.lots.end(), [&]( const auto &lot ) {
                                                     return std::abs( p.x - lot.parcelX ) <
                                                                lot.parcelWidth * .5f + site.radius &&
                                                            std::abs( p.z - lot.parcelZ ) <
                                                                lot.parcelDepth * .5f + site.radius;
                                                 } ) )
                {
                    continue;
                }
                const bool shrub = woody( site.plant );
                const float separation = site.radius * ( shrub ? .73f : .37f );
                if( std::any_of( planted.begin(), planted.end(), [&]( const PlantClearance &other ) {
                        const float clearance = other.radius + separation;
                        return distanceSquared( p, other.position, other.position ) <
                               clearance * clearance;
                    } ) )
                {
                    continue;
                }
                // Upper bounds include stems, seed clusters and petal geometry.
                const size_t triangleBudget = shrub                                   ? 248
                                              : site.plant == VergePlant::GroundCover ? 76
                                                                                      : 132;
                if( triangleBudget > remainingTriangles )
                {
                    continue;
                }
                remainingTriangles -= triangleBudget;
                planted.push_back( { p, separation } );
                const float rotation = scalar( site.key ) * 2.f * pi;
                if( shrub )
                {
                    const bool bramble = site.plant == VergePlant::Bramble;
                    const unsigned clusters = bramble ? 4 : 3;
                    const float height = site.radius * ( bramble ? .71f : .40f );
                    for( unsigned cluster = 0; cluster < clusters; ++cluster )
                    {
                        const auto key = render::foliageStableHash( site.key + 53 + cluster );
                        const float angle = rotation + cluster * 2.39996323f;
                        const Vector3F direction( std::cos( angle ), 0, std::sin( angle ) );
                        const float offset = cluster == 0 ? .05f : .27f + .15f * scalar( key );
                        const float width = site.radius * ( .32f + .12f * scalar( key + 1 ) );
                        auto crown = p + direction * ( site.radius * offset );
                        if( !render::sampleTerrainHeight( *ground, placement, crown, crown.y ) )
                        {
                            continue;
                        }
                        crown.y += height * ( cluster == 0 ? .68f : .29f + .19f * scalar( key + 2 ) );
                        const unsigned foliage =
                            bramble ? unsigned( cluster == 0 || scalar( key + 3 ) > .53f ) : 2;
                        const float lobeHeight = height * ( .33f + .13f * scalar( key + 4 ) );
                        canopyLobe( canopies[foliage], crown, width, lobeHeight, angle, key );
                        if( cluster == 0 || ( bramble && cluster == 2 ) )
                        {
                            stems.beam( p, crown, .013f );
                        }
                        for( unsigned leaf = 0; leaf < ( bramble ? 4u : 6u ); ++leaf )
                        {
                            const auto leafKey = render::foliageStableHash( key + 18 + leaf );
                            const float turn = angle + ( leaf - ( bramble ? 1.5f : 2.5f ) ) * .64f +
                                               .29f * scalar( leafKey );
                            const Vector3F outward( std::cos( turn ), 0, std::sin( turn ) );
                            const auto base =
                                crown + outward * ( width * .62f ) +
                                Vector3F( 0, lobeHeight * ( .07f + .44f * scalar( leafKey + 1 ) ), 0 );
                            const auto tip =
                                base + outward * ( width * ( .40f + .24f * scalar( leafKey + 2 ) ) ) +
                                Vector3F( 0, lobeHeight * ( .76f * scalar( leafKey + 3 ) - .26f ), 0 );
                            const auto side =
                                Vector3F( -outward.z, .28f - .56f * scalar( leafKey + 5 ), outward.x ) *
                                ( width * ( bramble ? .12f : .082f ) );
                            const auto middle = ( base + tip ) * .5f;
                            const auto normal = side.crossProduct( tip - base ).normaliseCopy();
                            leaves.quad( base, middle - side, tip, middle + side, normal );
                        }
                        if( !bramble && cluster < 2 && scalar( site.key + 61 ) > .29f )
                        {
                            flower(
                                crown + direction * ( width * .44f ) + Vector3F( 0, height * .22f, 0 ),
                                .04f + .035f * scalar( key + 7 ), angle, 1 );
                        }
                    }
                    continue;
                }
                if( site.plant == VergePlant::GroundCover )
                {
                    for( unsigned leaf = 0; leaf < 8; ++leaf )
                    {
                        const float angle = rotation + leaf * 2.39996323f;
                        const Vector3F direction( std::cos( angle ), 0, std::sin( angle ) );
                        const float fraction = .35f + .46f * scalar( site.key + 71 + leaf );
                        auto tip = p + direction * ( site.radius * fraction );
                        if( !render::sampleTerrainHeight( *ground, placement, tip, tip.y ) )
                        {
                            continue;
                        }
                        tip.y += .035f + site.radius * .13f * scalar( site.key + 83 + leaf );
                        fanLeaf(
                            p + Vector3F( 0, .045f, 0 ), tip,
                            Vector3F( -direction.z, 0, direction.x ) * ( site.radius * fraction * .27f ),
                            .019f );
                    }
                    if( scalar( site.key + 97 ) > .43f )
                    {
                        for( unsigned stem = 0; stem < 2; ++stem )
                        {
                            const float angle = rotation + stem * 2.39996323f;
                            const Vector3F direction( std::cos( angle ), 0, std::sin( angle ) );
                            const auto head =
                                p + direction * ( site.radius * .23f ) +
                                Vector3F( 0, .12f + .33f * site.radius * scalar( site.key + 101 + stem ),
                                          0 );
                            stems.beam( p, head, .004f );
                            flower( head, .037f + .043f * scalar( site.key + 108 + stem ), angle, 0 );
                        }
                    }
                    continue;
                }
                const bool fescue = site.plant == VergePlant::Fescue;
                const unsigned blades = fescue ? 8 : 7;
                for( unsigned blade = 0; blade < blades; ++blade )
                {
                    const auto variation = render::foliageStableHash( site.key + blade + 37 );
                    const float angle = rotation + blade * 2.39996323f + .65f * scalar( variation + 11 );
                    const Vector3F direction( std::cos( angle ), 0, std::sin( angle ) );
                    const Vector3F across( -direction.z, 0, direction.x );
                    auto root = p + direction * ( site.radius * ( .08f + .34f * scalar( variation ) ) );
                    if( !render::sampleTerrainHeight( *ground, placement, root, root.y ) )
                    {
                        continue;
                    }
                    root.y -= .015f;
                    const bool shortBlade = blade >= ( blades + 1 ) / 2;
                    const float tier = shortBlade ? .51f + .26f * scalar( variation + 16 )
                                                  : .87f + .13f * scalar( variation + 16 );
                    const float height =
                        tier *
                        ( ( fescue ? .068f : .095f ) +
                          site.radius * ( .14f + ( fescue ? .24f : .35f ) * scalar( variation + 1 ) ) );
                    const float bend = site.radius * ( .24f + .29f * scalar( variation + 2 ) ) *
                                       ( shortBlade ? .77f : 1.f );
                    const auto lean = across * ( bend * .44f * ( scalar( variation + 8 ) - .5f ) );
                    auto lower = root + direction * ( bend * .22f ) + lean * .12f +
                                 Vector3F( 0, height * .46f, 0 );
                    auto upper =
                        root + direction * ( bend * .65f ) + lean * .57f + Vector3F( 0, height, 0 );
                    auto tip = root + direction * bend + lean +
                               Vector3F( 0, height * ( .59f + .43f * scalar( variation + 4 ) ), 0 );
                    const auto followSurface = [&]( Vector3F &point ) {
                        float groundHeight = 0;
                        if( !render::sampleTerrainHeight( *ground, placement, point, groundHeight ) )
                        {
                            return false;
                        }
                        point.y = std::max( point.y, groundHeight + .025f );
                        return true;
                    };
                    if( !followSurface( lower ) || !followSurface( upper ) || !followSurface( tip ) )
                    {
                        continue;
                    }
                    const float width = fescue ? .012f + .007f * scalar( variation + 3 )
                                               : .014f + .009f * scalar( variation + 3 );
                    auto &grass = grasses[fescue == ( scalar( variation + 12 ) > .18f ) ? 1 : 0];
                    cityVerge::foldedGrassBlade( grass, std::array{ root, lower, upper, tip },
                                                 { width * .71f, width, width * .61f }, across,
                                                 [&]( Vector3F point ) {
                                                     float groundHeight = 0;
                                                     return render::sampleTerrainHeight(
                                                         *ground, placement, point, groundHeight );
                                                 } );
                    if( !fescue && blade == 0 && scalar( variation + 5 ) > .36f )
                    {
                        const auto head =
                            root + direction * ( bend * .66f ) + Vector3F( 0, height + .085f, 0 );
                        seeds.beam( root, head, .005f );
                        canopyLobe( seeds, head, .015f + .004f * scalar( variation + 7 ), .034f, angle,
                                    variation );
                    }
                }
            }
            SmartPtr<render::ITexture> canopyTexture;
            if( std::any_of( canopies.begin(), canopies.end(),
                             []( const Geometry &shape ) { return !shape.indices.empty(); } ) )
            {
                constexpr int resolution = 128;
                TextureBuffer leafMap( resolution, resolution );
                const auto paint = [&]( int x, int y, u8 light ) {
                    const auto index = ( size_t( ( y + resolution ) % resolution ) * resolution +
                                         ( x + resolution ) % resolution ) *
                                       4;
                    leafMap.pixels[index] = light;
                    leafMap.pixels[index + 1] = light;
                    leafMap.pixels[index + 2] = light;
                    leafMap.pixels[index + 3] = 255;
                };
                for( int y = 0; y < resolution; ++y )
                {
                    for( int x = 0; x < resolution; ++x )
                    {
                        paint( x, y, u8( 165 + 25 * scalar( size_t( y ) * resolution + x ) ) );
                    }
                }
                for( u64 leaf = 0; leaf < 360; ++leaf )
                {
                    const auto key = render::foliageStableHash( seed ^ ( 0xc2a9au + leaf ) );
                    const int cx = int( resolution * scalar( key ) ),
                              cy = int( resolution * scalar( key + 1 ) );
                    const float angle = 2.f * pi * scalar( key + 2 );
                    const float ax = std::cos( angle ), ay = std::sin( angle );
                    const float radius = 3.5f + 4.0f * scalar( key + 3 );
                    const float bladeWidth = 1.8f + 1.8f * scalar( key + 4 );
                    const float brightness = 182.f + 56.f * scalar( key + 5 );
                    const int reach = int( std::ceil( radius ) );
                    for( int y = -reach; y <= reach; ++y )
                    {
                        for( int x = -reach; x <= reach; ++x )
                        {
                            const float along = ( ax * x + ay * y ) / radius;
                            const float across = ( ax * y - ay * x ) / bladeWidth;
                            const float edge =
                                along * along + across * across * ( 1 + .7f * std::abs( along ) );
                            if( edge < 1.f )
                            {
                                const float vein = std::abs( across ) < .15f ? 13.f : 0.f;
                                paint( cx + x, cy + y, u8( brightness - 23.f * edge + vein ) );
                            }
                        }
                    }
                }
                canopyTexture = upload( assets, leafMap );
            }
            const auto publish = [&]( const char *name, const Geometry &geometry, ColourF tint,
                                      bool twoSided, bool textured = false, bool castShadows = true ) {
                if( geometry.indices.empty() )
                {
                    return;
                }
                auto surface = material( assets, tint, .94f );
                if( !twoSided )
                {
                    surface->setCullMode( 2 );
                }
                if( textured )
                {
                    surface->setTexture( canopyTexture, 0 );
                }
                if( auto actor =
                        mesh( assets, String( "Terrain city verge " ) + name, geometry, surface );
                    actor && !castShadows )
                {
                    actor->getComponent<scene::MeshRenderer>()->setCastShadows(
                        scene::Renderer::CastShadows::Off );
                }
            };
            publish( "meadow grass", grasses[0], { .334f, .402f, .177f, 1 }, true, false, false );
            publish( "dry fescue", grasses[1], { .398f, .399f, .216f, 1 }, true, false, false );
            publish( "seed heads", seeds, { .447f, .388f, .238f, 1 }, false, false, false );
            publish( "shaded shrub foliage", canopies[0], { .29f, .39f, .158f, 1 }, false, true );
            publish( "shrub growth tips", canopies[1], { .355f, .442f, .204f, 1 }, false, true );
            publish( "heather foliage", canopies[2], { .36f, .41f, .245f, 1 }, false, true );
            publish( "broad leaves", leaves, { .303f, .389f, .176f, 1 }, true );
            publish( "meadow flowers", blooms[0], { .70f, .63f, .375f, 1 }, true );
            publish( "heather flowers", blooms[1], { .48f, .277f, .335f, 1 }, true );
            publish( "plant stems", stems, { .285f, .229f, .123f, 1 }, false );
        }
    }  // namespace sceneBuild
}  // namespace workphone::scene::race
