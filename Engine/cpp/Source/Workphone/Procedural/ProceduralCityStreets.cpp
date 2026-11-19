#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::race
{
    namespace scene_build
    {
        void buildCityStreetDressing( SceneAssets &assets, const OpenCityLayout &city,
                                      VehicleAppearanceQuality quality )
        {
            Geometry stone, metal, wood, dark, planting, joints;
            const bool preview = quality == VehicleAppearanceQuality::Preview;
            const auto horizontal = []( Geometry &g, Vector3F p, float width, float depth ) {
                const auto h = Vector3F( width * .5f, 0, depth * .5f );
                g.quad( p + Vector3F( -h.x, 0, -h.z ), p + Vector3F( h.x, 0, -h.z ),
                        p + Vector3F( h.x, 0, h.z ), p + Vector3F( -h.x, 0, h.z ), { 0, 1, 0 } );
            };
            const auto axisBox = []( Geometry &g, Vector3F p, float width, float height, float depth,
                                     bool xFace ) {
                g.box( p, xFace ? Vector3F( depth, height, width ) : Vector3F( width, height, depth ) );
            };
            const auto normalFor = []( OpenCityLayout::Frontage frontage ) {
                switch( frontage )
                {
                case OpenCityLayout::Frontage::West:
                    return Vector3F( -1, 0, 0 );
                case OpenCityLayout::Frontage::East:
                    return Vector3F( 1, 0, 0 );
                case OpenCityLayout::Frontage::South:
                    return Vector3F( 0, 0, 1 );
                default:
                    return Vector3F( 0, 0, -1 );
                }
            };
            SourceBudgetVector<const OpenCityLayout::Lot *> parkLots, frontages;
            parkLots.reserve( city.lots.size() );
            frontages.reserve( city.lots.size() );
            for( const auto &lot : city.lots )
            {
                if( !lot.park )
                {
                    frontages.push_back( &lot );
                    continue;
                }
                if( assets.cityTerrainRoads )
                {
                    continue;
                }
                parkLots.push_back( &lot );
                const float hx = lot.parcelWidth * .5f, hz = lot.parcelDepth * .5f;
                const Vector3F centre( lot.parcelX, lot.groundHeight + .166f, lot.parcelZ );
                // The perimeter loop stays inside the two-metre tree exclusion margin.
                // Corners belong to the transverse strips only, avoiding coplanar overlaps.
                for( int side : { -1, 1 } )
                {
                    horizontal( stone, centre + Vector3F( 0, 0, side * ( hz - 1.4f ) ),
                                lot.parcelWidth - 1.6f, 1.2f );
                    horizontal( stone, centre + Vector3F( side * ( hx - 1.4f ), 0, 0 ), 1.2f,
                                lot.parcelDepth - 4.f );
                    horizontal( stone, centre + Vector3F( 0, 0, side * ( hz - .56f ) ),
                                lot.parcelWidth - 1.f, .12f );
                    horizontal( stone, centre + Vector3F( side * ( hx - .56f ), 0, 0 ), .12f,
                                lot.parcelDepth - 1.24f );
                    horizontal( dark, centre + Vector3F( 0, .001f, side * ( hz - 2.f ) ),
                                lot.parcelWidth - 4.f, .025f );
                    horizontal( dark, centre + Vector3F( side * ( hx - 2.f ), .001f, 0 ), .025f,
                                lot.parcelDepth - 4.f );
                }
                if( !city.naturalRoads && !assets.cityTerrainRoads )
                {
                    const auto normal = normalFor( lot.frontage );
                    const auto tangent = Vector3F( normal.z, 0, -normal.x );
                    const bool xFace = std::abs( normal.x ) > .5f;
                    const auto front = centre + normal * ( xFace ? hx : hz );
                    auto inner = front - normal * .8f, outer = front + normal * 2.5f;
                    inner.y = .168f;
                    outer.y = .156f;
                    stone.quad( inner - tangent * .6f, outer - tangent * .6f, outer + tangent * .6f,
                                inner + tangent * .6f, { 0, 1, 0 } );
                }
            }
            const auto nearest = []( const auto *a, const auto *b ) {
                const float da = a->x * a->x + a->z * a->z, db = b->x * b->x + b->z * b->z;
                return da == db ? a->id < b->id : da < db;
            };
            std::sort( parkLots.begin(), parkLots.end(), nearest );
            std::sort( frontages.begin(), frontages.end(), nearest );
            const auto propLimit = [&]( size_t base ) {
                return ( base * size_t( std::clamp( assets.cityStreetPropDensity, 0, 200 ) ) + 99 ) /
                       100;
            };
            const size_t benchCount = std::min( parkLots.size(), propLimit( preview ? 8u : 16u ) );
            for( size_t i = 0; i < benchCount; ++i )
            {
                const auto &lot = *parkLots[i];
                const auto normal = normalFor( lot.frontage );
                const auto tangent = Vector3F( normal.z, 0, -normal.x );
                const bool xFace = std::abs( normal.x ) > .5f;
                const float half = ( xFace ? lot.parcelWidth : lot.parcelDepth ) * .5f;
                const float span = xFace ? lot.parcelDepth : lot.parcelWidth;
                const auto p =
                    Vector3F( lot.parcelX, lot.groundHeight + .15f, lot.parcelZ ) +
                    normal * ( half + ( city.naturalRoads || assets.cityTerrainRoads ? -.8f : 1.1f ) ) -
                    tangent * std::min( 6.f, span * .25f );
                for( int plank = -2; plank <= 2; ++plank )
                {
                    axisBox( wood, p + Vector3F( 0, .46f, 0 ) + normal * ( plank * .085f ), 1.8f, .055f,
                             .065f, xFace );
                }
                for( int row = 0; row < 3; ++row )
                {
                    axisBox( wood, p + Vector3F( 0, .70f + row * .13f, 0 ) - normal * .21f, 1.8f, .08f,
                             .045f, xFace );
                }
                for( int side : { -1, 1 } )
                {
                    axisBox( metal, p + tangent * ( side * .64f ) + Vector3F( 0, .215f, 0 ), .075f, .43f,
                             .43f, xFace );
                    metal.beam( p + tangent * ( side * .64f ) - normal * .22f + Vector3F( 0, .22f, 0 ),
                                p + tangent * ( side * .64f ) - normal * .22f + Vector3F( 0, .98f, 0 ),
                                .035f );
                }
                staticBox( assets, "City bench", p + Vector3F( 0, .515f, 0 ),
                           xFace ? Vector3F( .55f, 1.03f, 1.84f ) : Vector3F( 1.84f, 1.03f, .55f ) );
            }
            const size_t planterCount =
                city.naturalRoads || assets.cityTerrainRoads
                    ? 0
                    : std::min( frontages.size(), propLimit( preview ? 8u : 16u ) );
            for( size_t i = 0; i < planterCount; ++i )
            {
                const auto &lot = *frontages[i];
                const auto normal = normalFor( lot.frontage );
                const auto tangent = Vector3F( normal.z, 0, -normal.x );
                const bool xFace = std::abs( normal.x ) > .5f;
                const float half = ( xFace ? lot.width : lot.depth ) * .5f;
                const float span = xFace ? lot.depth : lot.width;
                const auto p = Vector3F( lot.x, .15f, lot.z ) + normal * ( half + 1.15f ) +
                               tangent * ( span * .35f );
                for( int side : { -1, 1 } )
                {
                    axisBox( stone, p + normal * ( side * .31f ) + Vector3F( 0, .275f, 0 ), 1.6f, .55f,
                             .08f, xFace );
                    axisBox( stone, p + tangent * ( side * .76f ) + Vector3F( 0, .275f, 0 ), .08f, .55f,
                             .54f, xFace );
                }
                const auto soil = p + Vector3F( 0, .49f, 0 );
                horizontal( dark, soil, xFace ? .54f : 1.44f, xFace ? 1.44f : .54f );
                for( int blade = 0; blade < ( preview ? 10 : 18 ); ++blade )
                {
                    const float angle = blade * 2.39996323f + float( lot.variant % 127 ) * .07f;
                    const Vector3F direction( std::cos( angle ), 0, std::sin( angle ) );
                    const auto across = Vector3F( direction.z, 0, -direction.x );
                    const auto base = soil + direction * .045f;
                    const float height = .4f + .25f * float( ( blade * 19 + lot.variant ) % 31 ) / 30;
                    for( int segment = 0; segment < 3; ++segment )
                    {
                        const float a = segment / 3.f, b = ( segment + 1 ) / 3.f;
                        const auto lower =
                            base + direction * ( .13f * a * a ) + Vector3F( 0, height * a, 0 );
                        const auto upper =
                            base + direction * ( .13f * b * b ) + Vector3F( 0, height * b, 0 );
                        const auto aWidth = across * ( .023f * ( 1 - .9f * a ) );
                        const auto bWidth = across * ( .023f * ( 1 - .9f * b ) );
                        auto faceNormal = ( lower + aWidth - ( lower - aWidth ) )
                                              .crossProduct( upper + bWidth - ( lower - aWidth ) );
                        faceNormal.normalise();
                        planting.quad( lower - aWidth, lower + aWidth, upper + bWidth, upper - bWidth,
                                       faceNormal );
                    }
                }
                staticBox( assets, "City planter", p + Vector3F( 0, .275f, 0 ),
                           xFace ? Vector3F( .7f, .55f, 1.6f ) : Vector3F( 1.6f, .55f, .7f ) );
            }
            const size_t binCount = city.naturalRoads || assets.cityTerrainRoads
                                        ? 0
                                        : std::min( frontages.size(), propLimit( preview ? 4u : 8u ) );
            for( size_t i = 0; i < binCount; ++i )
            {
                const auto &lot = *frontages[( i * 3 + 1 ) % frontages.size()];
                const auto normal = normalFor( lot.frontage );
                const auto tangent = Vector3F( normal.z, 0, -normal.x );
                const bool xFace = std::abs( normal.x ) > .5f;
                const float half = ( xFace ? lot.width : lot.depth ) * .5f;
                const float span = xFace ? lot.depth : lot.width;
                const auto p = Vector3F( lot.x, .15f, lot.z ) + normal * ( half + 1.1f ) -
                               tangent * ( span * .35f );
                metal.box( p + Vector3F( 0, .45f, 0 ), { .42f, .9f, .42f } );
                metal.box( p + Vector3F( 0, .93f, 0 ), { .47f, .06f, .47f } );
                axisBox( dark, p + Vector3F( 0, .75f, 0 ) + normal * .214f, .26f, .10f, .005f, xFace );
                staticBox( assets, "City waste bin", p + Vector3F( 0, .48f, 0 ), { .48f, .96f, .48f } );
            }
            const auto street = OpenCityStreetDetails::generate( city, 0, false );
            SourceBudgetVector<const OpenCityStreetDetails::Corner *> walks;
            walks.reserve( street.sidewalks.size() );
            for( const auto &walk : street.sidewalks )
            {
                walks.push_back( &walk );
            }
            std::sort( walks.begin(), walks.end(), []( const auto *a, const auto *b ) {
                return a->x * a->x + a->z * a->z < b->x * b->x + b->z * b->z;
            } );
            const size_t detailedWalks =
                assets.cityTerrainRoads ? 0 : std::min<size_t>( walks.size(), preview ? 32 : 64 );
            for( size_t i = 0; i < detailedWalks; ++i )
            {
                const auto &walk = *walks[i];
                const bool horizontalWalk = walk.width > walk.depth;
                const float length = horizontalWalk ? walk.width : walk.depth;
                const float step = preview ? 4.8f : 2.4f;
                for( float t = -length * .5f + step; t < length * .5f - .2f; t += step )
                {
                    horizontal( joints,
                                { walk.x + ( horizontalWalk ? t : 0 ), .156f,
                                  walk.z + ( horizontalWalk ? 0 : t ) },
                                horizontalWalk ? .025f : walk.width,
                                horizontalWalk ? walk.depth : .025f );
                }
                horizontal( joints, { walk.x, .156f, walk.z }, horizontalWalk ? length : .025f,
                            horizontalWalk ? .025f : length );
                if( i % 3 != 0 )
                {
                    continue;
                }
                const Vector3F centre( walk.x, .158f, walk.z );
                const auto vertex = [&]( Vector3F p, Vector3F uv ) {
                    const auto index = u32( metal.positions.size() );
                    metal.positions.push_back( p );
                    metal.normals.push_back( { 0, 1, 0 } );
                    metal.tangents.push_back( { 1, 0, 0, 1 } );
                    metal.uv.push_back( uv );
                    return index;
                };
                const auto c = vertex( centre, { .5f, .5f, 0 } );
                const auto first = u32( metal.positions.size() );
                constexpr int sides = 20;
                for( int segment = 0; segment <= sides; ++segment )
                {
                    const float angle = segment * 2 * pi / sides;
                    vertex( centre + Vector3F( std::cos( angle ) * .31f, 0, std::sin( angle ) * .31f ),
                            { .5f + .5f * std::cos( angle ), .5f + .5f * std::sin( angle ), 0 } );
                }
                for( int segment = 0; segment < sides; ++segment )
                {
                    metal.indices.push_back( c );
                    metal.indices.push_back( first + segment + 1 );
                    metal.indices.push_back( first + segment );
                }
                for( int groove = -2; groove <= 2; ++groove )
                {
                    const float z = groove * .09f;
                    horizontal( dark, centre + Vector3F( 0, .001f, z ),
                                2 * std::sqrt( .31f * .31f - z * z ) - .04f, .012f );
                }
            }
            mesh( assets, "City park paving and planter rims", stone,
                  material( assets, { .49f, .48f, .42f, 1 }, .9f ) );
            mesh( assets, "City bench frames bins and utility covers", metal,
                  material( assets, { .12f, .15f, .16f, 1 }, .62f, .55f ) );
            mesh( assets, "City bench timber", wood, material( assets, { .36f, .21f, .10f, 1 }, .8f ) );
            mesh( assets, "City paving joints and planter soil", dark,
                  material( assets, { .12f, .14f, .11f, 1 }, 1 ) );
            mesh( assets, "City concrete paving joints", joints,
                  material( assets, { .31f, .32f, .30f, 1 }, 1 ) );
            mesh( assets, "City ornamental planter grasses", planting,
                  material( assets, { .24f, .36f, .13f, 1 }, .92f ) );
        }

        void measureStreetFixtureSupport( CityStreetFixture &site, const Vector3F &a, const Vector3F &b,
                                          const Vector3F &c )
        {
            // Clip the actual fitted triangle in the fixture's horizontal frame.
            // Y stays affine through clipping, so support is tested over the whole base.
            std::array<Vector3F, 8> polygon{}, clipped{};
            size_t count = 0;
            for( const auto &p : { a, b, c } )
            {
                const auto offset = p - site.position;
                polygon[count++] = { offset.dotProduct( site.along ), p.y,
                                     offset.dotProduct( site.outward ) };
            }
            for( int axis = 0; axis < 2 && count >= 3; ++axis )
            {
                for( const float side : { -1.f, 1.f } )
                {
                    const float extent = axis == 0 ? site.halfLength : site.halfDepth;
                    const auto distance = [=]( const Vector3F &v ) {
                        return extent - side * ( axis == 0 ? v.x : v.z );
                    };
                    size_t next = 0;
                    auto previous = polygon[count - 1];
                    float from = distance( previous );
                    for( size_t i = 0; i < count; ++i )
                    {
                        const auto current = polygon[i];
                        const float to = distance( current );
                        if( ( from < 0 && to > 0 ) || ( from > 0 && to < 0 ) )
                        {
                            clipped[next++] =
                                previous + ( current - previous ) * ( from / ( from - to ) );
                        }
                        if( to >= 0 )
                        {
                            clipped[next++] = current;
                        }
                        previous = current;
                        from = to;
                    }
                    polygon.swap( clipped );
                    count = next;
                    if( count < 3 )
                    {
                        return;
                    }
                }
            }
            double area = 0;
            for( size_t i = 1; i + 1 < count; ++i )
            {
                const auto u = polygon[i] - polygon[0], v = polygon[i + 1] - polygon[0];
                area += double( u.x ) * v.z - double( u.z ) * v.x;
            }
            if( std::abs( area ) <= 1e-10 )
            {
                return;
            }
            site.area += std::abs( area ) * .5;
            for( size_t i = 0; i < count; ++i )
            {
                site.lowest = std::min( site.lowest, polygon[i].y );
                site.highest = std::max( site.highest, polygon[i].y );
            }
        }

        void buildTerrainCityStreetDressing( SceneAssets &assets, const CityRoadTerrainMesh &walks,
                                             const SourceBudgetVector<Vector3F> &existingLamps,
                                             VehicleAppearanceQuality quality,
                                             SourceBudgetVector<CityStreetFixture> &installed )
        {
            assets.cityStreetDetailCount = 0;
            const int detail = std::clamp( assets.cityStreetDetail, 0, 100 );
            const int density = std::clamp( assets.cityStreetPropDensity, 0, 200 );
            if( detail == 0 || density == 0 || walks.indices.empty() )
            {
                return;
            }
            constexpr size_t maxCandidates = 4096;
            constexpr float halfLengths[] = { 1.05f, .89f, .28f, .25f, .88f, .43f };
            constexpr float halfDepths[] = { .30f, .30f, .28f, .25f, .31f, .29f };
            constexpr float maxRadius = 1.2f;
            const auto &bake = assets.roadAuthoring->getBake();
            const auto &source = assets.roadAuthoring->getSource();
            const float extent = assets.cityLayout.extent + 12;
            SourceBudgetVector<CityStreetFixture> candidates;
            const auto addCandidate = [&]( const Vector3F &edge, const Vector3F &along,
                                           const Vector3F &outward, CityStreetFixture::Kind kind ) {
                // The deepest base uses only the outer 0.77 m of the 2.4 m walk.
                const auto position = edge + outward * 1.98f;
                if( candidates.size() >= maxCandidates || std::abs( position.x ) > extent ||
                    std::abs( position.z ) > extent )
                {
                    return;
                }
                if( std::any_of(
                        walks.crossings.begin(), walks.crossings.end(), [&]( const auto &crossing ) {
                            const auto offset = position - crossing.centre;
                            const auto extentAlong =
                                halfLengths[kind] * std::abs( crossing.outward.dotProduct( along ) ) +
                                halfDepths[kind] * std::abs( crossing.outward.dotProduct( outward ) );
                            const auto extentAcross =
                                halfLengths[kind] * std::abs( crossing.across.dotProduct( along ) ) +
                                halfDepths[kind] * std::abs( crossing.across.dotProduct( outward ) );
                            return std::abs( offset.dotProduct( crossing.outward ) ) <
                                       crossing.depth * .5f + extentAlong + .95f &&
                                   std::abs( offset.dotProduct( crossing.across ) ) <
                                       crossing.width * .5f + extentAcross + 2.5f;
                        } ) )
                {
                    return;
                }
                const float lampClearance = kind == CityStreetFixture::Lamp ? 9.f : 1.65f;
                if( std::any_of( existingLamps.begin(), existingLamps.end(), [&]( const auto &lamp ) {
                        const float dx = lamp.x - position.x, dz = lamp.z - position.z;
                        return dx * dx + dz * dz < lampClearance * lampClearance;
                    } ) )
                {
                    return;
                }
                candidates.push_back(
                    { position, along, outward, kind, halfLengths[kind], halfDepths[kind] } );
            };
            for( const auto &junction : source.roundabouts )
            {
                for( size_t i = 0; i < junction.ports.size(); ++i )
                {
                    const auto &port = junction.ports[i];
                    Vector3F along( port.outward.x, 0, port.outward.z );
                    along.normalise();
                    const auto centre =
                        junction.center + along * ( junction.outerRadius + port.length * .55f );
                    for( const int side : { -1, 1 } )
                    {
                        const Vector3F outward( side * along.z, 0, -side * along.x );
                        const auto kind = ( i % 2 == 0 ) == ( side > 0 ) ? CityStreetFixture::Planter
                                                                         : CityStreetFixture::Bench;
                        addCandidate( centre + outward * ( port.width * .5f ), along, outward, kind );
                    }
                }
            }
            for( size_t road = 0; road < bake.roads.size() && candidates.size() < maxCandidates; ++road )
            {
                const auto &points = bake.roads[road].points;
                float next = 11.f + float( road % 4 ) * 2.f;
                size_t station = 0;
                for( size_t i = 1; i < points.size() && candidates.size() < maxCandidates; ++i )
                {
                    const auto &a = points[i - 1], &b = points[i];
                    while( next <= b.distance && next + 4 < points.back().distance &&
                           candidates.size() + 2 <= maxCandidates )
                    {
                        const float t =
                            std::clamp( ( next - a.distance ) / ( b.distance - a.distance ), 0.f, 1.f );
                        next += 2500.f / float( density );
                        ++station;
                        const auto centre = a.position + ( b.position - a.position ) * t;
                        for( const bool left : { true, false } )
                        {
                            const auto edgeA = left ? a.left : a.right, edgeB = left ? b.left : b.right;
                            auto along = edgeB - edgeA;
                            along.y = 0;
                            if( along.lengthSquared() < 1e-8f )
                            {
                                continue;
                            }
                            along.normalise();
                            const auto edge = edgeA + ( edgeB - edgeA ) * t;
                            auto outward = Vector3F( along.z, 0, -along.x );
                            if( ( edge - centre ).dotProduct( outward ) < 0 )
                            {
                                outward = -outward;
                            }
                            const auto kind = static_cast<CityStreetFixture::Kind>(
                                ( road + station * 3 + ( left ? 0 : 2 ) ) % 6 );
                            addCandidate( edge, along, outward, kind );
                        }
                    }
                }
            }
            if( candidates.empty() )
            {
                return;
            }

            // Index the comparatively few fixture centres and stream the already-bounded
            // footway mesh. Skirt faces have horizontal normals and supply no support.
            const auto cell = []( float coordinate ) { return int( std::floor( coordinate / 16 ) ); };
            const int firstCell = cell( -extent ), lastCell = cell( extent );
            SourceBudgetMap<std::pair<int, int>, SourceBudgetVector<size_t>> buckets;
            for( size_t i = 0; i < candidates.size(); ++i )
            {
                const auto &p = candidates[i].position;
                buckets[{ cell( p.x ), cell( p.z ) }].push_back( i );
            }
            for( size_t i = 0; i < walks.indices.size(); i += 3 )
            {
                const auto &a = walks.vertices[walks.indices[i]];
                const auto &b = walks.vertices[walks.indices[i + 1]];
                const auto &c = walks.vertices[walks.indices[i + 2]];
                if( a.normal.y < .9f || b.normal.y < .9f || c.normal.y < .9f )
                {
                    continue;
                }
                const float x0 = std::min( { a.position.x, b.position.x, c.position.x } ) - maxRadius;
                const float x1 = std::max( { a.position.x, b.position.x, c.position.x } ) + maxRadius;
                const float z0 = std::min( { a.position.z, b.position.z, c.position.z } ) - maxRadius;
                const float z1 = std::max( { a.position.z, b.position.z, c.position.z } ) + maxRadius;
                const int endX = std::min( cell( x1 ), lastCell ),
                          endZ = std::min( cell( z1 ), lastCell );
                for( int x = std::max( cell( x0 ), firstCell ); x <= endX; ++x )
                {
                    for( int z = std::max( cell( z0 ), firstCell ); z <= endZ; ++z )
                    {
                        const auto found = buckets.find( { x, z } );
                        if( found == buckets.end() )
                        {
                            continue;
                        }
                        for( const auto index : found->second )
                        {
                            auto &site = candidates[index];
                            if( site.position.x >= x0 && site.position.x <= x1 &&
                                site.position.z >= z0 && site.position.z <= z1 )
                            {
                                measureStreetFixtureSupport( site, a.position, b.position, c.position );
                            }
                        }
                    }
                }
            }
            std::sort( candidates.begin(), candidates.end(), []( const auto &a, const auto &b ) {
                const auto &p = a.position, &q = b.position;
                const auto aDistance = p.x * p.x + p.z * p.z, bDistance = q.x * q.x + q.z * q.z;
                return std::tie( aDistance, p.x, p.z, a.kind, a.along.x, a.along.z ) <
                       std::tie( bDistance, q.x, q.z, b.kind, b.along.x, b.along.z );
            } );

            const size_t cap = quality == VehicleAppearanceQuality::Preview ? 40 : 88;
            const size_t limit = ( cap * size_t( detail ) * size_t( density ) + 9999 ) / 10000;
            SourceBudgetVector<CityStreetFixture> admitted;
            admitted.reserve( limit );
            for( auto &site : candidates )
            {
                const double baseArea = 4.0 * site.halfLength * site.halfDepth;
                if( std::abs( site.area - baseArea ) > .00005 || site.highest - site.lowest > .08f ||
                    std::any_of( admitted.begin(), admitted.end(), [&]( const auto &placed ) {
                        const float x = site.position.x - placed.position.x;
                        const float z = site.position.z - placed.position.z;
                        return x * x + z * z < 6.f * 6.f;
                    } ) )
                {
                    continue;
                }
                site.position.y = site.lowest;
                admitted.push_back( site );
                if( admitted.size() == limit )
                {
                    break;
                }
            }

            Geometry metal, timber, stone, planting, lenses, enamel;
            for( const auto &site : admitted )
            {
                const auto point = [&]( float u, float y, float v ) {
                    return site.position + site.along * u + Vector3F( 0, y, 0 ) + site.outward * v;
                };
                const auto box = [&]( Geometry &g, float u, float y, float v, float length, float height,
                                      float depth ) {
                    const float x0 = u - length * .5f, x1 = u + length * .5f;
                    const float y0 = y - height * .5f, y1 = y + height * .5f;
                    const float z0 = v - depth * .5f, z1 = v + depth * .5f;
                    const auto a = point( x0, y0, z0 ), b = point( x1, y0, z0 );
                    const auto c = point( x1, y0, z1 ), d = point( x0, y0, z1 );
                    const auto up = Vector3F( 0, y1 - y0, 0 );
                    g.quad( a + up, b + up, c + up, d + up, { 0, 1, 0 } );
                    g.quad( a, b, c, d, { 0, -1, 0 } );
                    g.quad( a, b, b + up, a + up, -site.outward );
                    g.quad( c, d, d + up, c + up, site.outward );
                    g.quad( d, a, a + up, d + up, -site.along );
                    g.quad( b, c, c + up, b + up, site.along );
                };
                switch( site.kind )
                {
                case CityStreetFixture::Bench:
                    for( int slat = -2; slat <= 2; ++slat )
                    {
                        box( timber, 0, .44f, slat * .075f, 1.94f, .045f, .064f );
                    }
                    for( int row = 0; row < 3; ++row )
                    {
                        box( timber, 0, .65f + row * .095f, .19f + row * .018f, 1.94f, .076f, .04f );
                    }
                    for( float side : { -.73f, .73f } )
                    {
                        box( metal, side, .18f, 0, .065f, .42f, .46f );
                        metal.beam( point( side, .33f, .16f ), point( side, .87f, .236f ), .035f );
                        metal.beam( point( side, .62f, .20f ), point( side, .62f, -.18f ), .023f );
                    }
                    break;
                case CityStreetFixture::Planter:
                    box( stone, 0, .19f, 0, 1.64f, .44f, .52f );
                    for( float side : { -1.f, 1.f } )
                    {
                        box( stone, 0, .42f, side * .24f, 1.72f, .12f, .10f );
                        box( stone, side * .81f, .42f, 0, .10f, .12f, .38f );
                    }
                    box( metal, 0, .404f, 0, 1.52f, .018f, .37f );
                    for( int tuft = -1; tuft <= 1; ++tuft )
                    {
                        for( int blade = 0; blade < 13; ++blade )
                        {
                            const float angle = blade * 2.39996323f + tuft;
                            const auto bend =
                                site.along * std::cos( angle ) + site.outward * std::sin( angle );
                            const auto across = Vector3F( bend.z, 0, -bend.x ) * .024f;
                            const auto base = point( tuft * .47f, .41f, 0 ) + bend * .06f;
                            const float height = .23f + .022f * float( ( blade * 7 ) % 13 );
                            const auto middle = base + bend * .055f + Vector3F( 0, height * .62f, 0 );
                            const auto tip = base + bend * .16f + Vector3F( 0, height, 0 );
                            auto normal = across.crossProduct( middle - base );
                            normal.normalise();
                            const auto top = across * .55f;
                            planting.quad( base - across, base + across, middle + top, middle - top,
                                           normal );
                            planting.quad( middle - top, middle + top, tip + top * .13f,
                                           tip - top * .13f, normal );
                        }
                    }
                    break;
                case CityStreetFixture::Bin:
                    box( metal, 0, .33f, 0, .37f, .72f, .37f );
                    box( metal, 0, .79f, 0, .51f, .09f, .51f );
                    box( metal, 0, .71f, .13f, .41f, .18f, .22f );
                    for( int slat = -2; slat <= 2; ++slat )
                    {
                        for( float side : { -.20f, .20f } )
                        {
                            box( timber, slat * .077f, .33f, side, .056f, .61f, .036f );
                            box( timber, side, .33f, slat * .077f, .036f, .61f, .056f );
                        }
                    }
                    break;
                case CityStreetFixture::Lamp:
                    box( stone, 0, .035f, 0, .44f, .13f, .44f );
                    metal.beam( point( 0, .07f, 0 ), point( 0, .54f, 0 ), .082f );
                    metal.beam( point( 0, .53f, 0 ), point( 0, 3.43f, 0 ), .044f );
                    metal.beam( point( 0, 3.43f, 0 ), point( 0, 3.66f, -.52f ), .036f );
                    box( metal, 0, 3.67f, -.55f, .28f, .10f, .50f );
                    box( lenses, 0, 3.61f, -.55f, .24f, .035f, .42f );
                    break;
                case CityStreetFixture::CycleRack:
                    for( float u : { -.58f, 0.f, .58f } )
                    {
                        metal.beam( point( u, .02f, -.22f ), point( u, .73f, -.22f ), .025f );
                        metal.beam( point( u, .73f, -.22f ), point( u, .73f, .22f ), .025f );
                        metal.beam( point( u, .73f, .22f ), point( u, .02f, .22f ), .025f );
                        box( stone, u, .025f, -.22f, .16f, .05f, .14f );
                        box( stone, u, .025f, .22f, .16f, .05f, .14f );
                    }
                    break;
                case CityStreetFixture::UtilityCabinet:
                    box( stone, 0, .035f, 0, .74f, .07f, .55f );
                    box( enamel, 0, .60f, 0, .66f, 1.10f, .43f );
                    box( metal, 0, 1.17f, 0, .70f, .06f, .47f );
                    for( int vent = 0; vent < 5; ++vent )
                    {
                        box( metal, -.10f + vent * .05f, .88f, -.224f, .022f, .15f, .012f );
                    }
                    box( metal, .235f, .57f, -.225f, .025f, .10f, .018f );
                    break;
                }
            }

            const auto publish = [&]( const String &label, const Geometry &geometry, ColourF colour,
                                      float roughness, float metalness = 0 ) {
                if( !geometry.indices.empty() )
                {
                    mesh( assets, "Terrain city street detail " + label, geometry,
                          material( assets, colour, roughness, metalness ) );
                }
            };
            publish( "metal frames", metal, { .13f, .17f, .16f, 1 }, .66f, .5f );
            publish( "timber slats", timber, { .48f, .28f, .13f, 1 }, .85f );
            publish( "stone planters and plinths", stone, { .67f, .63f, .53f, 1 }, .88f );
            publish( "ornamental grasses", planting, { .31f, .43f, .16f, 1 }, .93f );
            publish( "pedestrian lamp lenses", lenses, { .86f, .83f, .72f, 1 }, .38f );
            publish( "utility enamel", enamel, { .27f, .37f, .34f, 1 }, .62f, .35f );
            assets.cityStreetDetailCount = static_cast<u32>( admitted.size() );
            installed.swap( admitted );
        }

        void buildNaturalCityStreets( SceneAssets &assets, const OpenCityLayout &city, Geometry &asphalt,
                                      Geometry &pavement, Geometry &markings, Geometry &streetMetal,
                                      Geometry &lampGlass, VehicleAppearanceQuality quality )
        {
            const auto &network = city.roads;
            if( network.nodes.empty() || network.edges.empty() || network.nodes.size() > 160 ||
                network.edges.size() > 240 )
            {
                throw std::runtime_error( "Natural city requires a bounded connected road graph." );
            }
            const auto position = [&]( u32 node ) {
                if( node >= network.nodes.size() )
                {
                    throw std::runtime_error( "Natural road endpoint is out of range." );
                }
                const auto p = network.nodes[node];
                if( !std::isfinite( p.x ) || !std::isfinite( p.z ) )
                {
                    throw std::runtime_error( "Natural road endpoint is not finite." );
                }
                return Vector3F( p.x, 0, p.z );
            };
            SourceBudgetVector<u32> degree( network.nodes.size(), 0 );
            SourceBudgetVector<float> junctionHalfWidth( network.nodes.size(), 0.f );
            for( const auto &edge : network.edges )
            {
                if( edge.a == edge.b ||
                    ( position( edge.b ) - position( edge.a ) ).lengthSquared() < .01f )
                {
                    throw std::runtime_error( "Natural road has an invalid segment." );
                }
                ++degree[edge.a];
                ++degree[edge.b];
                const float halfWidth =
                    procedural::OpenCityRoadNetwork::roadWidth( edge.roadClass ) * .5f;
                junctionHalfWidth[edge.a] = std::max( junctionHalfWidth[edge.a], halfWidth );
                junctionHalfWidth[edge.b] = std::max( junctionHalfWidth[edge.b], halfWidth );
            }
            const auto stripe = []( Geometry &g, Vector3F a, Vector3F b, Vector3F across, float width,
                                    float y ) {
                a.y = b.y = y;
                const auto half = across * ( width * .5f );
                g.quad( a - half, b - half, b + half, a + half, { 0, 1, 0 } );
            };
            const auto trim = [&]( u32 node, u32 other, Vector3F direction, int side, bool paint,
                                   float ownHalfWidth ) {
                float distance = paint ? junctionHalfWidth[node] + 2.5f : 0;
                for( const auto &edge : network.edges )
                {
                    const u32 neighbour = edge.a == node ? edge.b : edge.b == node ? edge.a : node;
                    if( neighbour == node || neighbour == other )
                    {
                        continue;
                    }
                    auto q = position( neighbour ) - position( node );
                    q.normalise();
                    const float cosine = direction.dotProduct( q );
                    const float sine = direction.x * q.z - direction.z * q.x;
                    const float otherHalfWidth =
                        procedural::OpenCityRoadNetwork::roadWidth( edge.roadClass ) * .5f;
                    if( paint )
                    {
                        if( cosine > 0 && std::abs( sine ) > .001f )
                        {
                            const float outerPaintOffset = ownHalfWidth - .3f + .065f;
                            distance =
                                std::max( distance, ( otherHalfWidth + outerPaintOffset * cosine ) /
                                                            std::abs( sine ) +
                                                        .1f );
                        }
                    }
                    else if( sine * side > .001f )
                    {
                        // A branch on this side can cross the walk. Behind-facing
                        // continuations use the inner offset, avoiding enormous trim
                        // distances on gentle bends close to a straight continuation.
                        const float offset = cosine >= 0 ? ownHalfWidth + 2.5f : ownHalfWidth;
                        distance = std::max(
                            distance, ( otherHalfWidth + offset * cosine ) / std::abs( sine ) + .05f );
                    }
                }
                return distance;
            };
            const auto ribbonClear = [&]( Vector3F a, Vector3F b, Vector3F across ) {
                const auto centre = ( a + b ) * .5f;
                auto along = b - a;
                const float halfLength = along.length() * .5f;
                along.normalise();
                constexpr float halfWidth = 1.25f;
                const auto point = [&]( Vector3F p ) {
                    p -= centre;
                    return Vector2F( p.dotProduct( along ), p.dotProduct( across ) );
                };
                const auto distanceToBox = [&]( Vector2F p ) {
                    const float x = std::max( 0.f, std::abs( p.x ) - halfLength );
                    const float y = std::max( 0.f, std::abs( p.y ) - halfWidth );
                    return x * x + y * y;
                };
                for( const auto &edge : network.edges )
                {
                    const auto p = point( position( edge.a ) ), q = point( position( edge.b ) );
                    const auto delta = q - p;
                    float low = 0, high = 1;
                    bool intersects = true;
                    for( int axis = 0; axis < 2; ++axis )
                    {
                        const float start = axis ? p.y : p.x, direction = axis ? delta.y : delta.x;
                        const float extent = axis ? halfWidth : halfLength;
                        if( std::abs( direction ) < 1e-6f )
                        {
                            if( std::abs( start ) > extent )
                            {
                                intersects = false;
                                break;
                            }
                        }
                        else
                        {
                            float t0 = ( -extent - start ) / direction,
                                  t1 = ( extent - start ) / direction;
                            if( t0 > t1 )
                            {
                                std::swap( t0, t1 );
                            }
                            low = std::max( low, t0 );
                            high = std::min( high, t1 );
                            if( low > high )
                            {
                                intersects = false;
                                break;
                            }
                        }
                    }
                    if( intersects )
                    {
                        return false;
                    }
                    float distanceSquared = std::min( distanceToBox( p ), distanceToBox( q ) );
                    const float lengthSquared = delta.x * delta.x + delta.y * delta.y;
                    for( int sx : { -1, 1 } )
                    {
                        for( int sy : { -1, 1 } )
                        {
                            const Vector2F corner( sx * halfLength, sy * halfWidth );
                            const float t = std::clamp(
                                ( ( corner.x - p.x ) * delta.x + ( corner.y - p.y ) * delta.y ) /
                                    std::max( lengthSquared, 1e-8f ),
                                0.f, 1.f );
                            const auto offset = corner - ( p + delta * t );
                            distanceSquared =
                                std::min( distanceSquared, offset.x * offset.x + offset.y * offset.y );
                        }
                    }
                    const float roadHalf =
                        procedural::OpenCityRoadNetwork::roadWidth( edge.roadClass ) * .5f - .002f;
                    if( distanceSquared < roadHalf * roadHalf )
                    {
                        return false;
                    }
                }
                return true;
            };
            u32 lampCount = 0;
            for( const auto &edge : network.edges )
            {
                const auto a = position( edge.a ), b = position( edge.b );
                auto direction = b - a;
                const float length = direction.length();
                direction /= length;
                const Vector3F across( -direction.z, 0, direction.x );
                const float width = procedural::OpenCityRoadNetwork::roadWidth( edge.roadClass );
                const float halfWidth = width * .5f;
                // Full segment rectangles and round node fans form the road's
                // centreline-buffer union, including split crossings and dead ends.
                stripe( asphalt, a, b, across, width, .012f );
                const float paintStart = trim( edge.a, edge.b, direction, 0, true, halfWidth );
                const float paintEnd = trim( edge.b, edge.a, -direction, 0, true, halfWidth );
                if( length > paintStart + paintEnd + 2 )
                {
                    const auto start = a + direction * paintStart, end = b - direction * paintEnd;
                    for( int side : { -1, 1 } )
                    {
                        stripe( markings, start + across * ( side * ( halfWidth - .3f ) ),
                                end + across * ( side * ( halfWidth - .3f ) ), across, .13f, .018f );
                    }
                    for( float t = paintStart; t + 2 < length - paintEnd; t += 5 )
                    {
                        stripe( markings, a + direction * t, a + direction * ( t + 2 ), across, .15f,
                                .018f );
                    }
                }
                for( int side : { -1, 1 } )
                {
                    const float start = trim( edge.a, edge.b, direction, side, false, halfWidth );
                    const float end = trim( edge.b, edge.a, -direction, -side, false, halfWidth );
                    if( length <= start + end + 1 )
                    {
                        continue;
                    }
                    auto p = a + direction * start + across * ( side * ( halfWidth + 1.25f ) );
                    auto q = b - direction * end + across * ( side * ( halfWidth + 1.25f ) );
                    if( !ribbonClear( p, q, across ) )
                    {
                        continue;
                    }
                    const auto half = across * 1.25f;
                    const Vector3F up( 0, .15f, 0 );
                    pavement.quad( p - half + up, q - half + up, q + half + up, p + half + up,
                                   { 0, 1, 0 } );
                    pavement.quad( p - half, p + half, q + half, q - half, { 0, -1, 0 } );
                    pavement.quad( p + half, p + half + up, q + half + up, q + half, across );
                    pavement.quad( p - half, q - half, q - half + up, p - half + up, -across );
                    pavement.quad( p - half, p - half + up, p + half + up, p + half, -direction );
                    pavement.quad( q - half, q + half, q + half + up, q - half + up, direction );
                    const auto centre = ( p + q ) * .5f;
                    staticBox( assets, "City sidewalk", centre + Vector3F( 0, .075f, 0 ),
                               { 2.5f, .15f, ( q - p ).length() },
                               QuaternionF::eulerDegrees(
                                   0, std::atan2( direction.x, direction.z ) * 180 / pi, 0 ) );
                    if( lampCount >= ( quality == VehicleAppearanceQuality::Preview ? 24u : 64u ) ||
                        ( q - p ).lengthSquared() < 400 )
                    {
                        continue;
                    }
                    const auto base = centre + Vector3F( 0, .15f, 0 ),
                               top = base + Vector3F( 0, 6.2f, 0 );
                    const auto arm = across * ( -side * 1.1f ) + Vector3F( 0, -.15f, 0 );
                    streetMetal.beam( base, top, .055f );
                    streetMetal.beam( top, top + arm, .045f );
                    lampGlass.box( top + arm, { .5f, .1f, .5f } );
                    staticBox( assets, "City lamp post", base + Vector3F( 0, 3.1f, 0 ),
                               { .12f, 6.2f, .12f } );
                    ++lampCount;
                }
            }
            for( u32 node = 0; node < network.nodes.size(); ++node )
            {
                if( !degree[node] )
                {
                    continue;
                }
                const auto p = position( node ) + Vector3F( 0, .012f, 0 );
                const auto base = u32( asphalt.positions.size() );
                const auto vertex = [&]( Vector3F v ) {
                    asphalt.positions.push_back( v );
                    asphalt.normals.push_back( { 0, 1, 0 } );
                    asphalt.tangents.push_back( { 1, 0, 0, 1 } );
                    asphalt.uv.push_back( { v.x, v.z, 0 } );
                };
                vertex( p );
                constexpr u32 slices = 24;
                for( u32 i = 0; i <= slices; ++i )
                {
                    vertex( p + Vector3F( std::cos( i * 2 * pi / slices ) * junctionHalfWidth[node], 0,
                                          std::sin( i * 2 * pi / slices ) * junctionHalfWidth[node] ) );
                }
                for( u32 i = 0; i < slices; ++i )
                {
                    asphalt.indices.push_back( base );
                    asphalt.indices.push_back( base + i + 2 );
                    asphalt.indices.push_back( base + i + 1 );
                }
            }
        }
    }  // namespace sceneBuild
}  // namespace workphone::scene::race
