#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/FoliageData.hpp>
#include <Workphone/Procedural/CityRoadParcelSupport.hpp>
#include <Workphone/Procedural/CityRoadJunctions.hpp>
#include <Workphone/Procedural/CityRoadTerrainMesh.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace workphone::scene
{
    namespace
    {
        void require( bool valid, const char *message )
        {
            if( !valid )
            {
                throw std::runtime_error( message );
            }
        }

        bool finite( Vector3F p )
        {
            return std::isfinite( p.x ) && std::isfinite( p.y ) && std::isfinite( p.z );
        }

        // Match CityRoadFootways'0.05m carriageway gap +2.4m walk. Full parcel
        // foundations must reserve the pedestrian surface as well as the road.
        constexpr double pedestrianEnvelope = .05 + 2.4;

        constexpr size_t maxInputParcels = 400;
        constexpr size_t maxRoadSamples = 65536;
        constexpr size_t maxJunctionTriangles = 65536;
        constexpr u64 maxClearanceVisits = 32000000;
        constexpr u64 maxFittedVertices = 8000000;
        // Foundation relief limit shared with the building ground-contact rules.
        constexpr float maxFoundationRelief = 1.25f + 1e-5f;
        // World-aligned infill grid pitch; leaves pedestrian/service gaps between lots.
        constexpr float infillPitch = 18.f;

        struct Rectangle
        {
            double x, z, width, depth;
        };

        struct Segment
        {
            double ax, az, bx, bz, clearance;
        };

        struct Point
        {
            double x, z;
        };

        struct ReservedTriangle
        {
            std::array<Point, 3> points;
            double x0, x1, z0, z1;
        };

        struct Bounds
        {
            double x0, x1, z0, z1;
        };

        // Consecutive samples follow a spline or a junction corner. Bound small runs
        // once so each parcel need only inspect nearby stretches of the network.
        constexpr size_t reservationGroupSize = 16;

        template <class Items, class BoundsOf>
        SourceBudgetVector<Bounds> groupBounds( const Items &items, BoundsOf boundsOf )
        {
            SourceBudgetVector<Bounds> groups;
            for( size_t i = 0; i < items.size(); ++i )
            {
                const auto b = boundsOf( items[i] );
                if( i % reservationGroupSize == 0 )
                {
                    groups.push_back( b );
                }
                else
                {
                    auto &group = groups.back();
                    group.x0 = std::min( group.x0, b.x0 );
                    group.x1 = std::max( group.x1, b.x1 );
                    group.z0 = std::min( group.z0, b.z0 );
                    group.z1 = std::max( group.z1, b.z1 );
                }
            }
            return groups;
        }

        template <class Items, class Overlaps>
        bool overlapsReservation( const Items &items, const SourceBudgetVector<Bounds> &groups,
                                  const Rectangle &r, u64 &visits, Overlaps overlaps )
        {
            const Bounds parcel{ r.x - r.width * .5, r.x + r.width * .5, r.z - r.depth * .5,
                                 r.z + r.depth * .5 };
            const auto visit = [&visits] {
                require( ++visits <= maxClearanceVisits, "Parcel road-clearance work budget exceeded." );
            };
            for( size_t i = 0; i < groups.size(); ++i )
            {
                visit();
                const auto &b = groups[i];
                if( parcel.x1 < b.x0 || parcel.x0 > b.x1 || parcel.z1 < b.z0 || parcel.z0 > b.z1 )
                {
                    continue;
                }
                const size_t start = i * reservationGroupSize;
                const size_t end = std::min( items.size(), start + reservationGroupSize );
                for( size_t j = start; j < end; ++j )
                {
                    visit();
                    if( overlaps( items[j] ) )
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        bool triangleOverlap( const Rectangle &r, const ReservedTriangle &triangle )
        {
            const double halfX = r.width * .5, halfZ = r.depth * .5;
            if( r.x + halfX < triangle.x0 || r.x - halfX > triangle.x1 || r.z + halfZ < triangle.z0 ||
                r.z - halfZ > triangle.z1 )
            {
                return false;
            }
            // Rectangle axes were checked by the bounds. The three triangle
            // edge normals complete SAT, including boundary contact as reserved.
            for( size_t i = 0; i < triangle.points.size(); ++i )
            {
                const auto a = triangle.points[i], b = triangle.points[( i + 1 ) % 3];
                const double nx = a.z - b.z, nz = b.x - a.x;
                double minimum = std::numeric_limits<double>::max(), maximum = -minimum;
                for( const auto p : triangle.points )
                {
                    const double projected = ( p.x - r.x ) * nx + ( p.z - r.z ) * nz;
                    minimum = std::min( minimum, projected );
                    maximum = std::max( maximum, projected );
                }
                const double radius = std::abs( nx ) * halfX + std::abs( nz ) * halfZ;
                if( maximum < -radius || minimum > radius )
                {
                    return false;
                }
            }
            return true;
        }

        SourceBudgetVector<ReservedTriangle> junctionReservations(
            const CityRoadJunctionBake &connections )
        {
            SourceBudgetVector<ReservedTriangle> triangles;
            const auto point = []( Vector3F p ) { return Point{ p.x, p.z }; };
            const auto append = [&]( Point a, Point b, Point c ) {
                const double area = ( b.x - a.x ) * ( c.z - a.z ) - ( b.z - a.z ) * ( c.x - a.x );
                require( std::isfinite( area ), "Parcel junction reservation is not finite." );
                if( std::abs( area ) < 1e-10 )
                {
                    return;
                }
                require( triangles.size() < maxJunctionTriangles,
                         "Parcel junction reservation capacity exceeded." );
                triangles.push_back( { { a, b, c },
                                       std::min( { a.x, b.x, c.x } ),
                                       std::max( { a.x, b.x, c.x } ),
                                       std::min( { a.z, b.z, c.z } ),
                                       std::max( { a.z, b.z, c.z } ) } );
            };
            for( const auto &connection : connections.junctions )
            {
                const auto vertex = [&]( u32 index ) {
                    require( index <= connection.boundary.size(), "Parcel junction index is invalid." );
                    return point( index ? connection.boundary[index - 1] : connection.center );
                };
                require( connection.indices.size() % 3 == 0, "Parcel junction triangles are invalid." );
                for( size_t i = 0; i < connection.indices.size(); i += 3 )
                {
                    append( vertex( connection.indices[i] ), vertex( connection.indices[i + 1] ),
                            vertex( connection.indices[i + 2] ) );
                }
                for( const auto &corner : connection.corners )
                {
                    require( corner.points.size() == corner.outward.size(),
                             "Parcel junction pavement offsets are invalid." );
                    for( size_t i = 1; i < corner.points.size(); ++i )
                    {
                        const auto a = point( corner.points[i - 1] ), b = point( corner.points[i] ),
                                   na = point( corner.outward[i - 1] ), nb = point( corner.outward[i] );
                        const auto offset = []( Point p, Point n, double amount ) {
                            return Point{ p.x + n.x * amount, p.z + n.z * amount };
                        };
                        // Exactly CityRoadFootways' corner-strip endpoints. These
                        // rounded walks extend beyond the straight-road capsules;
                        // a disk around the junction would unnecessarily lose lots.
                        const auto ai = offset( a, na, .05 ), ao = offset( a, na, pedestrianEnvelope ),
                                   bi = offset( b, nb, .05 ), bo = offset( b, nb, pedestrianEnvelope );
                        append( ai, ao, bo );
                        append( ai, bo, bi );
                    }
                }
            }
            return triangles;
        }

        double pointSegmentSquared( double x, double z, const Segment &segment )
        {
            const double dx = segment.bx - segment.ax, dz = segment.bz - segment.az;
            const double length = dx * dx + dz * dz;
            const double t =
                length > 0 ? std::clamp( ( ( x - segment.ax ) * dx + ( z - segment.az ) * dz ) / length,
                                         0., 1. )
                           : 0;
            const double ox = x - segment.ax - dx * t, oz = z - segment.az - dz * t;
            return ox * ox + oz * oz;
        }

        double segmentRectangleSquared( const Segment &segment, const Rectangle &r )
        {
            const double x0 = r.x - r.width * .5, x1 = r.x + r.width * .5;
            const double z0 = r.z - r.depth * .5, z1 = r.z + r.depth * .5;
            double first = 0, last = 1;
            const auto clip = [&]( double p, double q ) {
                if( std::abs( p ) < 1e-12 )
                {
                    return q >= 0;
                }
                const double t = q / p;
                if( p < 0 )
                {
                    first = std::max( first, t );
                }
                else
                {
                    last = std::min( last, t );
                }
                return first <= last;
            };
            if( clip( segment.ax - segment.bx, segment.ax - x0 ) &&
                clip( segment.bx - segment.ax, x1 - segment.ax ) &&
                clip( segment.az - segment.bz, segment.az - z0 ) &&
                clip( segment.bz - segment.az, z1 - segment.az ) )
            {
                return 0;
            }
            const auto pointBox = [&]( double x, double z ) {
                const double dx = std::max( { x0 - x, 0., x - x1 } ),
                             dz = std::max( { z0 - z, 0., z - z1 } );
                return dx * dx + dz * dz;
            };
            return std::min(
                { pointBox( segment.ax, segment.az ), pointBox( segment.bx, segment.bz ),
                  pointSegmentSquared( x0, z0, segment ), pointSegmentSquared( x1, z0, segment ),
                  pointSegmentSquared( x0, z1, segment ), pointSegmentSquared( x1, z1, segment ) } );
        }

        bool junctionOverlap( const Rectangle &r, const procedural::CityRoadRoundaboutSpec &junction )
        {
            const double dx = std::max( std::abs( r.x - junction.center.x ) - r.width * .5, 0. );
            const double dz = std::max( std::abs( r.z - junction.center.z ) - r.depth * .5, 0. );
            const double radius = junction.outerRadius + pedestrianEnvelope;
            if( dx * dx + dz * dz <= radius * radius )
            {
                return true;
            }
            for( const auto &port : junction.ports )
            {
                const double run = std::hypot( double( port.outward.x ), double( port.outward.z ) );
                require( std::isfinite( run ) && run > .001,
                         "Parcel junction approach has an invalid horizontal direction." );
                const double inverse = 1. / run;
                const double ux = port.outward.x * inverse, uz = port.outward.z * inverse;
                const double along = port.length * .5 + pedestrianEnvelope,
                             across = port.width * .5 + pedestrianEnvelope;
                const double cx = junction.center.x + ux * ( junction.outerRadius + port.length * .5 );
                const double cz = junction.center.z + uz * ( junction.outerRadius + port.length * .5 );
                const double x = r.x - cx, z = r.z - cz;
                // Four separating axes reserve the oriented approach and both
                // sidewalks; the circle also contains their skew miter contacts.
                if( std::abs( x ) > r.width * .5 + std::abs( ux ) * along + std::abs( uz ) * across ||
                    std::abs( z ) > r.depth * .5 + std::abs( uz ) * along + std::abs( ux ) * across ||
                    std::abs( x * ux + z * uz ) >
                        along + std::abs( ux ) * r.width * .5 + std::abs( uz ) * r.depth * .5 ||
                    std::abs( -x * uz + z * ux ) >
                        across + std::abs( uz ) * r.width * .5 + std::abs( ux ) * r.depth * .5 )
                {
                    continue;
                }
                return true;
            }
            return false;
        }

        bool unsupportedTerrain( const String &error, u64 denialBefore )
        {
            // Legacy bool adapters catch allocation/work errors. Never turn a
            // denied temporary mesh into a successful, silently empty city.
            if( sourceBudgetDenialCount() != denialBefore || error == std::bad_alloc().what() )
            {
                return false;
            }
            const std::string_view text( error.data(), error.size() );
            return text == "Terrain sample is outside, a hole or unavailable." ||
                   text.find( "Road profile exceeds grade" ) == 0 ||
                   text == "Road profile has zero horizontal span." ||
                   text == "A road or junction footprint intersects a terrain hole." ||
                   text == "Road or junction footprint extends outside its bound terrain." ||
                   text == "Road footprint is not completely supported by terrain cells." ||
                   text == "Covered terrain or junction carriageway exceeds the road grade limit.";
        }

        using procedural::OpenCityLayout;

        struct BlockCandidate
        {
            OpenCityLayout::Lot lot;
            double streetDistanceSquared;
        };

        struct BlockCoverage
        {
            double minX, maxX, minZ, maxZ, area = 0, occupied = 0;
            SourceBudgetVector<BlockCandidate> sites;
            size_t next = 0;
        };

        // Test the real face, including concave corners. For nested road loops,
        // assign the site to the smallest enclosing boundary.
        size_t containingBlock( const CityRoadBlockFaces &faces,
                                const SourceBudgetVector<BlockCoverage> &blocks, float x, float z )
        {
            size_t owner = faces.size();
            for( size_t i = 0; i < faces.size(); ++i )
            {
                const auto &b = blocks[i];
                if( x <= b.minX || x >= b.maxX || z <= b.minZ || z >= b.maxZ ||
                    ( owner != faces.size() && b.area >= blocks[owner].area ) )
                {
                    continue;
                }
                bool inside = false;
                const auto &ring = faces[i];
                for( size_t j = 0, k = ring.size() - 1; j < ring.size(); k = j++ )
                {
                    const auto a = ring[j], c = ring[k];
                    if( ( a.z > z ) != ( c.z > z ) && x < a.x + ( double( z ) - a.z ) *
                                                                    ( double( c.x ) - a.x ) /
                                                                    ( double( c.z ) - a.z ) )
                    {
                        inside = !inside;
                    }
                }
                if( inside )
                {
                    owner = i;
                }
            }
            return owner;
        }

        using BlockRing = SourceBudgetVector<OpenCityLayout::Point>;

        void measureBlock( const BlockRing &ring, BlockCoverage &block )
        {
            require( ring.size() >= 3, "Invalid terrain city block boundary." );
            block.minX = block.maxX = ring.front().x;
            block.minZ = block.maxZ = ring.front().z;
            for( size_t j = 0, k = ring.size() - 1; j < ring.size(); k = j++ )
            {
                const auto a = ring[k], b = ring[j];
                block.area += double( a.x ) * b.z - double( b.x ) * a.z;
                block.minX = std::min<double>( block.minX, b.x );
                block.maxX = std::max<double>( block.maxX, b.x );
                block.minZ = std::min<double>( block.minZ, b.z );
                block.maxZ = std::max<double>( block.maxZ, b.z );
            }
            block.area = std::abs( block.area ) * .5;
            require( block.area > 0, "Terrain city block has no area." );
        }

        struct StreetEdge
        {
            double distanceSquared = std::numeric_limits<double>::max();
            OpenCityLayout::Frontage frontage = OpenCityLayout::Frontage::North;
        };

        // Closest block-boundary edge to the site, and the world side it lies on.
        StreetEdge nearestStreetEdge( const BlockRing &ring, float x, float z )
        {
            StreetEdge nearest;
            for( size_t j = 0, k = ring.size() - 1; j < ring.size(); k = j++ )
            {
                const auto a = ring[k], b = ring[j];
                const double dx = double( b.x ) - a.x, dz = double( b.z ) - a.z;
                const double length2 = dx * dx + dz * dz;
                if( length2 == 0 )
                {
                    continue;
                }
                const double t =
                    std::clamp( ( ( x - a.x ) * dx + ( z - a.z ) * dz ) / length2, 0., 1. );
                const double ox = a.x + t * dx - x, oz = a.z + t * dz - z;
                const double distance = ox * ox + oz * oz;
                if( distance < nearest.distanceSquared )
                {
                    nearest.distanceSquared = distance;
                    nearest.frontage = std::abs( ox ) > std::abs( oz )
                                           ? ( ox < 0 ? OpenCityLayout::Frontage::West
                                                      : OpenCityLayout::Frontage::East )
                                           : ( oz < 0 ? OpenCityLayout::Frontage::North
                                                      : OpenCityLayout::Frontage::South );
                }
            }
            return nearest;
        }

        // Stable identity per grid cell; the flag bit keeps infill clear of authored ids.
        u64 infillLotId( u32 seed, int ix, int iz )
        {
            return ( u64( seed ) << 32 ) | 0x80000000u | ( ( u32( ix ) & 0x7fffu ) << 15 ) |
                   ( u32( iz ) & 0x7fffu );
        }

        // Everything about an infill lot except its frontage; empty when its
        // envelope would cross the city's road setback.
        std::optional<OpenCityLayout::Lot> makeInfillLot( u64 id, float x, float z, float extent )
        {
            OpenCityLayout::Lot lot{};
            lot.id = id;
            const u64 variation = render::foliageStableHash( lot.id );
            lot.variant = u32( variation );
            lot.parcelX = lot.x = x;
            lot.parcelZ = lot.z = z;
            lot.parcelWidth = 12.f + float( variation & 3 );
            lot.parcelDepth = 12.f + float( ( variation >> 2 ) & 3 );
            const float limit = extent - OpenCityLayout::roadSetback;
            if( std::abs( x ) + lot.parcelWidth * .5f > limit ||
                std::abs( z ) + lot.parcelDepth * .5f > limit )
            {
                return std::nullopt;
            }
            lot.park = ( variation >> 32 ) % 16 == 0;
            lot.width = lot.parcelWidth - ( lot.park ? 0.f : 2.f );
            lot.depth = lot.parcelDepth - ( lot.park ? 0.f : 2.f );
            const bool central = std::hypot( x, z ) < extent * .40f;
            lot.district = central                        ? OpenCityLayout::District::Commercial
                           : ( variation >> 16 ) % 13 == 0 ? OpenCityLayout::District::Civic
                                                           : OpenCityLayout::District::Residential;
            lot.height = central ? 3.6f * ( 6 + ( variation >> 24 ) % 10 )
                                 : 3.2f * ( 2 + ( variation >> 24 ) % 5 );
            return lot;
        }

        template <class Lots>
        bool overlapsExistingLot( const Lots &lots, const OpenCityLayout::Lot &lot )
        {
            return std::any_of( lots.begin(), lots.end(), [&]( const auto &other ) {
                return OpenCityLayout::parcelsOverlap( lot, other );
            } );
        }

        using ShrinkScale = std::pair<float, float>;

        // Candidate (x, z) scale factors, largest area first; equal areas keep as
        // much street frontage as possible. Index 0 is always the unscaled lot.
        // Built once per frontage axis instead of sorting for every admitted lot.
        const std::array<ShrinkScale, 25> &shrinkScales( bool frontageAlongX )
        {
            static const std::array<std::array<ShrinkScale, 25>, 2> tables = [] {
                std::array<std::array<ShrinkScale, 25>, 2> result;
                for( size_t axis = 0; axis < result.size(); ++axis )
                {
                    auto &scales = result[axis];
                    size_t index = 0;
                    for( float x : { 1.f, .85f, .7f, .55f, .4f } )
                    {
                        for( float z : { 1.f, .85f, .7f, .55f, .4f } )
                        {
                            scales[index++] = { x, z };
                        }
                    }
                    std::sort( scales.begin(), scales.end(), [axis]( const auto &a, const auto &b ) {
                        const float areaA = a.first * a.second, areaB = b.first * b.second;
                        if( areaA != areaB )
                        {
                            return areaA > areaB;
                        }
                        return axis ? a.first > b.first : a.second > b.second;
                    } );
                }
                return result;
            }();
            return tables[frontageAlongX ? 1 : 0];
        }

        // Scale the envelope and building towards the road-facing edge so that edge,
        // the identity and the frontage are preserved.
        OpenCityLayout::Lot shrinkLot( const OpenCityLayout::Lot &original, ShrinkScale scale )
        {
            using Frontage = OpenCityLayout::Frontage;
            auto lot = original;
            const float anchorX = lot.frontage == Frontage::West ? -1.f
                                  : lot.frontage == Frontage::East ? 1.f
                                                                   : 0.f;
            const float anchorZ = lot.frontage == Frontage::North ? -1.f
                                  : lot.frontage == Frontage::South ? 1.f
                                                                    : 0.f;
            lot.parcelX += anchorX * original.parcelWidth * ( 1 - scale.first ) * .5f;
            lot.parcelZ += anchorZ * original.parcelDepth * ( 1 - scale.second ) * .5f;
            lot.parcelWidth *= scale.first;
            lot.parcelDepth *= scale.second;
            lot.x = lot.parcelX + ( original.x - original.parcelX ) * scale.first;
            lot.z = lot.parcelZ + ( original.z - original.parcelZ ) * scale.second;
            lot.width *= scale.first;
            lot.depth *= scale.second;
            return lot;
        }

        SourceBudgetVector<Segment> buildRoadSegments( const procedural::CityRoadSplineSource &source,
                                                       const procedural::CityRoadSplineBake &bake )
        {
            SourceBudgetVector<Segment> segments;
            size_t segmentCount = 0;
            for( size_t road = 0; road < source.roads.size(); ++road )
            {
                const auto &samples = bake.roads[road];
                require( samples.roadId == source.roads[road].id && samples.points.size() >= 2 &&
                             samples.points.size() <= maxRoadSamples,
                         "Parcel support received missing or excessive canonical road samples." );
                segmentCount += samples.points.size() - 1;
                require( segmentCount <= maxRoadSamples,
                         "Parcel road-clearance sample budget exceeded." );
            }
            segments.reserve( segmentCount );
            for( size_t road = 0; road < source.roads.size(); ++road )
            {
                const auto &points = bake.roads[road].points;
                for( size_t i = 0; i < points.size(); ++i )
                {
                    require( finite( points[i].position ) && std::isfinite( points[i].distance ) &&
                                 points[i].distance >= 0 &&
                                 ( !i || points[i].distance > points[i - 1].distance ),
                             "Parcel road bake contains invalid samples." );
                    if( !i )
                    {
                        continue;
                    }
                    const auto a = points[i - 1].position, b = points[i].position;
                    require( std::hypot( b.x - a.x, b.z - a.z ) > 1e-5f,
                             "Parcel road bake has a zero horizontal segment." );
                    segments.push_back(
                        { a.x, a.z, b.x, b.z, source.roads[road].width * .5 + pedestrianEnvelope } );
                }
            }
            return segments;
        }

        bool validParcelEnvelope( const OpenCityLayout::Lot &lot )
        {
            return std::isfinite( lot.parcelX ) && std::isfinite( lot.parcelZ ) &&
                   std::isfinite( lot.parcelWidth ) && std::isfinite( lot.parcelDepth ) &&
                   lot.parcelWidth >= .5f && lot.parcelDepth >= .5f &&
                   std::min( lot.parcelWidth, lot.parcelDepth ) <= 60 &&
                   std::abs( lot.parcelX ) + lot.parcelWidth * .5f <= 1000000 &&
                   std::abs( lot.parcelZ ) + lot.parcelDepth * .5f <= 1000000 &&
                   std::isfinite( lot.x ) && std::isfinite( lot.z ) && std::isfinite( lot.width ) &&
                   std::isfinite( lot.depth ) && std::isfinite( lot.height ) && lot.width > 0 &&
                   lot.depth > 0 && lot.height > 0 &&
                   std::abs( lot.x - lot.parcelX ) + lot.width * .5f <=
                       lot.parcelWidth * .5f + .001f &&
                   std::abs( lot.z - lot.parcelZ ) + lot.depth * .5f <=
                       lot.parcelDepth * .5f + .001f;
        }

        template <class Admit>
        void fillCityBlocks( OpenCityLayout &city, const CityRoadBlockFaces &faces, u32 seed,
                             SourceBudgetSet<u64> &ids, Admit admit )
        {
            require( city.blocks >= 1 && city.blocks <= 10 && std::isfinite( city.extent ) &&
                         city.extent > 0 && city.extent <= 1024,
                     "Invalid terrain city infill extent." );
            const auto limit = size_t( city.blocks * city.blocks * 4 );
            if( city.lots.size() >= limit || faces.empty() )
            {
                return;
            }
            SourceBudgetVector<BlockCoverage> blocks;
            for( const auto &ring : faces )
            {
                measureBlock( ring, blocks.emplace_back() );
            }
            for( const auto &lot : city.lots )
            {
                const size_t i = containingBlock( faces, blocks, lot.parcelX, lot.parcelZ );
                if( i < blocks.size() )
                {
                    blocks[i].occupied += lot.parcelWidth * lot.parcelDepth;
                }
            }

            // World-aligned sites retain their identity when an unrelated road is edited.
            const int half =
                int( std::floor( ( city.extent - OpenCityLayout::roadSetback ) / infillPitch ) );
            for( int ix = -half; ix <= half; ++ix )
            {
                for( int iz = -half; iz <= half; ++iz )
                {
                    const float x = ix * infillPitch, z = iz * infillPitch;
                    const auto owner = containingBlock( faces, blocks, x, z );
                    if( owner == faces.size() )
                    {
                        continue;
                    }
                    const u64 id = infillLotId( seed, ix, iz );
                    if( ids.count( id ) )
                    {
                        continue;
                    }
                    auto lot = makeInfillLot( id, x, z, city.extent );
                    if( !lot )
                    {
                        continue;
                    }
                    const auto edge = nearestStreetEdge( faces[owner], x, z );
                    lot->frontage = edge.frontage;
                    blocks[owner].sites.push_back( { *lot, edge.distanceSquared } );
                }
            }
            for( auto &block : blocks )
            {
                std::sort( block.sites.begin(), block.sites.end(), []( const auto &a, const auto &b ) {
                    if( a.streetDistanceSquared != b.streetDistanceSquared )
                    {
                        return a.streetDistanceSquared < b.streetDistanceSquared;
                    }
                    return a.lot.id < b.lot.id;
                } );
            }

            while( city.lots.size() < limit )
            {
                BlockCoverage *least = nullptr;
                for( auto &block : blocks )
                {
                    if( block.next < block.sites.size() &&
                        ( !least || block.occupied * least->area < least->occupied * block.area ) )
                    {
                        least = &block;
                    }
                }
                if( !least )
                {
                    break;
                }
                const auto &lot = least->sites[least->next++].lot;
                const bool occupied = overlapsExistingLot( city.lots, lot );
                if( !occupied && admit( lot ) )
                {
                    least->occupied += city.lots.back().parcelWidth * city.lots.back().parcelDepth;
                }
            }
        }
    }  // namespace

    bool prepareCityRoadParcels( const procedural::OpenCityLayout &input,
                                 const procedural::CityRoadSplineSource &source,
                                 const procedural::CityRoadSplineBake &bake,
                                 render::TerrainSnapshot terrain, const Transform3<real_Num> &placement,
                                 procedural::OpenCityLayout &output, u32 &culled, u32 &supportChecks,
                                 String &error )
    {
        return prepareCityRoadParcels( input, source, bake, std::move( terrain ), placement, output,
                                       culled, supportChecks, error, {} );
    }

    static bool prepareParcels( const OpenCityLayout &input,
                                const procedural::CityRoadSplineSource &source,
                                const procedural::CityRoadSplineBake &bake,
                                render::TerrainSnapshot terrain, const Transform3<real_Num> &placement,
                                OpenCityLayout &output, u32 &culled, u32 &supportChecks, String &error,
                                const CityRoadParcelSupportOptions &options,
                                std::optional<u32> infillSeed, CityRoadBlockFaces *outFaces )
    {
        try
        {
            require( terrain && input.lots.size() <= maxInputParcels,
                     "Parcel support requires retained terrain and at most400 parcels." );
            require( std::isfinite( options.minimumBuildingSpan ) && options.minimumBuildingSpan >= 2 &&
                         options.minimumBuildingSpan <= 30,
                     "Parcel adaptation requires a finite minimum building span from 2 to 30 metres." );
            if( !render::validateTerrainPlacement( *terrain, placement, error ) ||
                !procedural::validateCityRoadSplineSource( source, error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            require( bake.sourceRevision == source.revision && bake.roads.size() == source.roads.size(),
                     "Parcel support received a stale or mismatched canonical road bake." );
            for( const auto &junction : source.roundabouts )
            {
                for( const auto &port : junction.ports )
                {
                    require( finite( port.outward ) &&
                                 std::hypot( double( port.outward.x ), double( port.outward.z ) ) > .001,
                             "Parcel junction approach has an invalid horizontal direction." );
                }
            }
            const auto segments = buildRoadSegments( source, bake );
            SourceBudgetSet<u64> ids;
            for( const auto &lot : input.lots )
            {
                require( lot.id && ids.insert( lot.id ).second && validParcelEnvelope( lot ),
                         "Parcel support requires unique identities and finite bounded whole envelopes "
                         "containing their building footprints." );
            }

            auto candidate = input;
            candidate.lots.clear();
            u32 nextCulled = 0, nextChecks = 0;
            u64 clearanceVisits = 0, fittedVertices = 0;
            procedural::CityRoadSplineSource rectangle;
            rectangle.version = 2;
            rectangle.id = "a0000000-0000-4000-8000-000000000001";
            rectangle.terrainResourceUuid = "a0000000-0000-4000-8000-000000000002";
            procedural::CityRoadSpline road;
            road.id = "a0000000-0000-4000-8000-000000000003";
            road.maxGrade = .35f;
            road.heightOffset = 0;
            road.heightMode = procedural::CityRoadHeightMode::Drape;
            road.knots = { { "a0000000-0000-4000-8000-000000000004", {}, {}, {} },
                           { "a0000000-0000-4000-8000-000000000005", {}, {}, {} } };
            rectangle.roads.push_back( std::move( road ) );
            const procedural::CityRoadHeightSampler sampler = [&]( const Vector3F &point, f32 &height ) {
                return render::sampleTerrainHeight( *terrain, placement, point, height );
            };
            CityRoadJunctionBake connections;
            if( !buildCityRoadJunctions( source, bake, sampler, {}, connections, error ) )
            {
                throw std::runtime_error( error.c_str() );
            }
            const auto reservedJunctions = junctionReservations( connections );
            const auto roadBounds = groupBounds( segments, []( const Segment &s ) {
                return Bounds{ std::min( s.ax, s.bx ) - s.clearance,
                               std::max( s.ax, s.bx ) + s.clearance,
                               std::min( s.az, s.bz ) - s.clearance,
                               std::max( s.az, s.bz ) + s.clearance };
            } );
            const auto junctionBounds = groupBounds( reservedJunctions, []( const ReservedTriangle &t ) {
                return Bounds{ t.x0, t.x1, t.z0, t.z1 };
            } );
            const auto reserved = [&]( const Rectangle &footprint ) {
                if( overlapsReservation(
                        segments, roadBounds, footprint, clearanceVisits, [&]( const Segment &s ) {
                            return segmentRectangleSquared( s, footprint ) <= s.clearance * s.clearance;
                        } ) )
                {
                    return true;
                }
                for( const auto &junction : source.roundabouts )
                {
                    if( junctionOverlap( footprint, junction ) )
                    {
                        return true;
                    }
                }
                return overlapsReservation(
                    reservedJunctions, junctionBounds, footprint, clearanceVisits,
                    [&]( const ReservedTriangle &t ) { return triangleOverlap( footprint, t ); } );
            };
            enum class Foundation
            {
                Unsupported,
                TooSteep,
                Supported
            };
            // Fit the retained terrain to the whole parcel footprint. Missing, holed
            // or steep-celled support is a genuine exclusion; only excessive relief
            // on an otherwise safe surface may be resized by the caller.
            const auto fitFoundation = [&]( OpenCityLayout::Lot &lot ) {
                auto &sweep = rectangle.roads[0];
                const bool alongX = lot.parcelWidth > lot.parcelDepth;
                sweep.width = std::min( lot.parcelWidth, lot.parcelDepth );
                const Vector3F span =
                    alongX ? Vector3F( lot.parcelWidth, 0, 0 ) : Vector3F( 0, 0, lot.parcelDepth );
                const Vector3F center{ lot.parcelX, 0, lot.parcelZ };
                sweep.knots[0].position = center - span * .5f;
                sweep.knots[1].position = center + span * .5f;
                sweep.knots[0].outHandle = span / 3.f;
                sweep.knots[1].inHandle = -span / 3.f;
                const auto denialBefore = sourceBudgetDenialCount();
                procedural::CityRoadSplineBake rectangleBake;
                CityRoadTerrainMesh surface;
                if( !procedural::sampleCityRoadSplines( rectangle, {}, sampler, rectangleBake, error ) ||
                    !buildCityRoadTerrainMesh( rectangle, rectangleBake, terrain, placement, surface,
                                               error, false ) )
                {
                    if( !unsupportedTerrain( error, denialBefore ) )
                    {
                        throw std::runtime_error( error.c_str() );
                    }
                    return Foundation::Unsupported;
                }
                fittedVertices += surface.vertices.size();
                require( fittedVertices <= maxFittedVertices,
                         "Parcel cumulative terrain-fit work budget exceeded." );
                require( !surface.vertices.empty(),
                         "Parcel full-footprint support unexpectedly produced no surface." );
                float minimum = std::numeric_limits<float>::max(), maximum = -minimum;
                for( const auto &vertex : surface.vertices )
                {
                    minimum = std::min( minimum, vertex.position.y );
                    maximum = std::max( maximum, vertex.position.y );
                }
                if( maximum - minimum > maxFoundationRelief )
                {
                    return Foundation::TooSteep;
                }
                lot.groundHeight = maximum;
                lot.groundMinimumHeight = minimum;
                return Foundation::Supported;
            };
            const auto admit = [&]( OpenCityLayout::Lot lot ) {
                const Rectangle footprint{ lot.parcelX, lot.parcelZ, lot.parcelWidth, lot.parcelDepth };
                if( reserved( footprint ) )
                {
                    return false;
                }
                const auto original = lot;
                const bool frontageAlongX =
                    lot.frontage == procedural::OpenCityLayout::Frontage::North ||
                    lot.frontage == procedural::OpenCityLayout::Frontage::South;
                const auto &scales = shrinkScales( frontageAlongX );
                const size_t attempts = options.shrinkForRelief ? scales.size() : 1;
                for( size_t attempt = 0; attempt < attempts; ++attempt )
                {
                    lot = original;
                    if( attempt )
                    {
                        const auto scale = scales[attempt];
                        if( original.width * scale.first < options.minimumBuildingSpan ||
                            original.depth * scale.second < options.minimumBuildingSpan )
                        {
                            continue;
                        }
                        lot = shrinkLot( original, scale );
                        // Recheck even these mathematical subsets so float rounding
                        // at large world coordinates cannot erode a road reservation.
                        const Rectangle reduced{ lot.parcelX, lot.parcelZ, lot.parcelWidth,
                                                 lot.parcelDepth };
                        if( reserved( reduced ) )
                        {
                            continue;
                        }
                    }
                    ++nextChecks;
                    const auto fit = fitFoundation( lot );
                    if( fit == Foundation::Unsupported )
                    {
                        break;
                    }
                    if( fit == Foundation::Supported )
                    {
                        candidate.lots.push_back( lot );
                        return true;
                    }
                }
                return false;
            };
            for( const auto &lot : input.lots )
            {
                if( !admit( lot ) )
                {
                    ++nextCulled;
                }
            }
            CityRoadBlockFaces faces;
            if( infillSeed && input.naturalRoads )
            {
                faces = cityRoadBlockFaces( source, bake, connections );
                fillCityBlocks( candidate, faces, *infillSeed, ids, admit );
            }
            if( outFaces )
            {
                outFaces->swap( faces );
            }
            output.swap( candidate );
            culled = nextCulled;
            supportChecks = nextChecks;
            error.clear();
            return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
    }

    bool prepareCityRoadParcels( const OpenCityLayout &input,
                                 const procedural::CityRoadSplineSource &source,
                                 const procedural::CityRoadSplineBake &bake,
                                 render::TerrainSnapshot terrain, const Transform3<real_Num> &placement,
                                 OpenCityLayout &output, u32 &culled, u32 &supportChecks, String &error,
                                 const CityRoadParcelSupportOptions &options )
    {
        return prepareParcels( input, source, bake, std::move( terrain ), placement, output, culled,
                               supportChecks, error, options, std::nullopt, nullptr );
    }

    bool prepareCityRoadParcels( const OpenCityLayout &input,
                                 const procedural::CityRoadSplineSource &source,
                                 const procedural::CityRoadSplineBake &bake,
                                 render::TerrainSnapshot terrain, const Transform3<real_Num> &placement,
                                 OpenCityLayout &output, u32 &culled, u32 &supportChecks, String &error,
                                 const CityRoadParcelSupportOptions &options, u32 infillSeed,
                                 CityRoadBlockFaces *faces )
    {
        return prepareParcels( input, source, bake, std::move( terrain ), placement, output, culled,
                               supportChecks, error, options, infillSeed, faces );
    }
}  // namespace workphone::scene
