#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CityRoadParcelSupport.hpp>
#include <Workphone/Scene/Components/CityRoadTerrainMesh.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
#include <string_view>

namespace workphone::scene
{
    namespace
    {
        void require( bool valid, const char *message ) { if( !valid ) throw std::runtime_error( message ); }
        bool finite( Vector3F p ) { return std::isfinite( p.x ) && std::isfinite( p.y ) && std::isfinite( p.z ); }
        // Match CityRoadFootways'0.05m carriageway gap +2.4m walk. Full parcel
        // foundations must reserve the pedestrian surface as well as the road.
        constexpr double pedestrianEnvelope = .05 + 2.4;
        struct Rectangle { double x, z, width, depth; };
        struct Segment { double ax, az, bx, bz, clearance; };
        double pointSegmentSquared( double x, double z, const Segment &segment )
        {
            const double dx = segment.bx - segment.ax, dz = segment.bz - segment.az;
            const double length = dx * dx + dz * dz;
            const double t = length > 0 ? std::clamp( ( ( x - segment.ax ) * dx + ( z - segment.az ) * dz ) / length, 0., 1. ) : 0;
            const double ox = x - segment.ax - dx * t, oz = z - segment.az - dz * t;
            return ox * ox + oz * oz;
        }
        double segmentRectangleSquared( const Segment &segment, const Rectangle &r )
        {
            const double x0 = r.x - r.width * .5, x1 = r.x + r.width * .5;
            const double z0 = r.z - r.depth * .5, z1 = r.z + r.depth * .5;
            double first = 0, last = 1;
            const auto clip = [&]( double p, double q ) {
                if( std::abs( p ) < 1e-12 ) return q >= 0;
                const double t = q / p;
                if( p < 0 ) first = std::max( first, t ); else last = std::min( last, t );
                return first <= last;
            };
            if( clip( segment.ax - segment.bx, segment.ax - x0 ) && clip( segment.bx - segment.ax, x1 - segment.ax ) &&
                clip( segment.az - segment.bz, segment.az - z0 ) && clip( segment.bz - segment.az, z1 - segment.az ) ) return 0;
            const auto pointBox = [&]( double x, double z ) {
                const double dx = std::max( { x0 - x, 0., x - x1 } ), dz = std::max( { z0 - z, 0., z - z1 } );
                return dx * dx + dz * dz;
            };
            return std::min( { pointBox( segment.ax, segment.az ), pointBox( segment.bx, segment.bz ),
                pointSegmentSquared( x0, z0, segment ), pointSegmentSquared( x1, z0, segment ),
                pointSegmentSquared( x0, z1, segment ), pointSegmentSquared( x1, z1, segment ) } );
        }
        bool junctionOverlap( const Rectangle &r, const procedural::CityRoadRoundaboutSpec &junction )
        {
            const double dx = std::max( std::abs( r.x - junction.center.x ) - r.width * .5, 0. );
            const double dz = std::max( std::abs( r.z - junction.center.z ) - r.depth * .5, 0. );
            const double radius = junction.outerRadius + pedestrianEnvelope;
            if( dx * dx + dz * dz <= radius * radius ) return true;
            for( const auto &port : junction.ports )
            {
                const double run = std::hypot( double( port.outward.x ), double( port.outward.z ) );
                require( std::isfinite( run ) && run > .001, "Parcel junction approach has an invalid horizontal direction." );
                const double inverse = 1. / run;
                const double ux = port.outward.x * inverse, uz = port.outward.z * inverse;
                const double along = port.length * .5 + pedestrianEnvelope, across = port.width * .5 + pedestrianEnvelope;
                const double cx = junction.center.x + ux * ( junction.outerRadius + port.length * .5 );
                const double cz = junction.center.z + uz * ( junction.outerRadius + port.length * .5 );
                const double x = r.x - cx, z = r.z - cz;
                // Four separating axes reserve the oriented approach and both
                // sidewalks; the circle also contains their skew miter contacts.
                if( std::abs( x ) > r.width * .5 + std::abs( ux ) * along + std::abs( uz ) * across ||
                    std::abs( z ) > r.depth * .5 + std::abs( uz ) * along + std::abs( ux ) * across ||
                    std::abs( x * ux + z * uz ) > along + std::abs( ux ) * r.width * .5 + std::abs( uz ) * r.depth * .5 ||
                    std::abs( -x * uz + z * ux ) > across + std::abs( uz ) * r.width * .5 + std::abs( ux ) * r.depth * .5 ) continue;
                return true;
            }
            return false;
        }
        bool unsupportedTerrain( const String &error, u64 denialBefore )
        {
            // Legacy bool adapters catch allocation/work errors. Never turn a
            // denied temporary mesh into a successful, silently empty city.
            if( sourceBudgetDenialCount() != denialBefore || error == std::bad_alloc().what() ) return false;
            const std::string_view text( error.data(), error.size() );
            return text == "Terrain sample is outside, a hole or unavailable." ||
                text == "Road profile exceeds grade or has zero horizontal span." ||
                text == "A road or junction footprint intersects a terrain hole." ||
                text == "Road or junction footprint extends outside its bound terrain." ||
                text == "Road footprint is not completely supported by terrain cells." ||
                text == "Covered terrain or junction carriageway exceeds the road grade limit.";
        }
    }

    bool prepareCityRoadParcels( const procedural::OpenCityLayout &input,
        const procedural::CityRoadSplineSource &source, const procedural::CityRoadSplineBake &bake,
        render::TerrainSnapshot terrain, const Transform3<real_Num> &placement,
        procedural::OpenCityLayout &output, u32 &culled, u32 &supportChecks, String &error )
    {
        try
        {
            require( terrain && input.lots.size() <= 400, "Parcel support requires retained terrain and at most400 parcels." );
            if( !render::validateTerrainPlacement( *terrain, placement, error ) ||
                !procedural::validateCityRoadSplineSource( source, error ) ) throw std::runtime_error( error.c_str() );
            require( bake.sourceRevision == source.revision && bake.roads.size() == source.roads.size(),
                "Parcel support received a stale or mismatched canonical road bake." );
            for( const auto &junction : source.roundabouts ) for( const auto &port : junction.ports )
                require( finite( port.outward ) && std::hypot( double( port.outward.x ), double( port.outward.z ) ) > .001,
                    "Parcel junction approach has an invalid horizontal direction." );
            SourceBudgetVector<Segment> segments;
            size_t segmentCount = 0;
            for( size_t road = 0; road < source.roads.size(); ++road )
            {
                const auto &samples = bake.roads[road];
                require( samples.roadId == source.roads[road].id && samples.points.size() >= 2 &&
                    samples.points.size() <= 65536, "Parcel support received missing or excessive canonical road samples." );
                segmentCount += samples.points.size() - 1;
                require( segmentCount <= 65536, "Parcel road-clearance sample budget exceeded." );
            }
            segments.reserve( segmentCount );
            for( size_t road = 0; road < source.roads.size(); ++road )
            {
                const auto &points = bake.roads[road].points;
                for( size_t i = 0; i < points.size(); ++i )
                {
                    require( finite( points[i].position ) && std::isfinite( points[i].distance ) && points[i].distance >= 0 &&
                        ( !i || points[i].distance > points[i - 1].distance ), "Parcel road bake contains invalid samples." );
                    if( !i ) continue;
                    const auto a = points[i - 1].position, b = points[i].position;
                    require( std::hypot( b.x - a.x, b.z - a.z ) > 1e-5f, "Parcel road bake has a zero horizontal segment." );
                    segments.push_back( { a.x, a.z, b.x, b.z, source.roads[road].width * .5 + pedestrianEnvelope } );
                }
            }
            SourceBudgetSet<u64> ids;
            for( const auto &lot : input.lots )
                require( lot.id && ids.insert( lot.id ).second && std::isfinite( lot.parcelX ) && std::isfinite( lot.parcelZ ) &&
                    std::isfinite( lot.parcelWidth ) && std::isfinite( lot.parcelDepth ) &&
                    lot.parcelWidth >= .5f && lot.parcelDepth >= .5f && std::min( lot.parcelWidth, lot.parcelDepth ) <= 60 &&
                    std::abs( lot.parcelX ) + lot.parcelWidth * .5f <= 1000000 &&
                    std::abs( lot.parcelZ ) + lot.parcelDepth * .5f <= 1000000 &&
                    std::isfinite( lot.x ) && std::isfinite( lot.z ) && std::isfinite( lot.width ) && std::isfinite( lot.depth ) &&
                    std::isfinite( lot.height ) && lot.width > 0 && lot.depth > 0 && lot.height > 0 &&
                    std::abs( lot.x - lot.parcelX ) + lot.width * .5f <= lot.parcelWidth * .5f + .001f &&
                    std::abs( lot.z - lot.parcelZ ) + lot.depth * .5f <= lot.parcelDepth * .5f + .001f,
                    "Parcel support requires unique identities and finite bounded whole envelopes containing their building footprints." );

            auto candidate = input;
            candidate.lots.clear();
            u32 nextCulled = 0, nextChecks = 0;
            u64 clearanceVisits = 0, fittedVertices = 0;
            procedural::CityRoadSplineSource rectangle;
            rectangle.version = 2; rectangle.id = "a0000000-0000-4000-8000-000000000001";
            rectangle.terrainResourceUuid = "a0000000-0000-4000-8000-000000000002";
            procedural::CityRoadSpline road;
            road.id = "a0000000-0000-4000-8000-000000000003"; road.maxGrade = .35f;
            road.heightOffset = 0; road.heightMode = procedural::CityRoadHeightMode::Drape;
            road.knots = { { "a0000000-0000-4000-8000-000000000004", {}, {}, {} },
                { "a0000000-0000-4000-8000-000000000005", {}, {}, {} } };
            rectangle.roads.push_back( std::move( road ) );
            const procedural::CityRoadHeightSampler sampler = [&]( const Vector3F &point, f32 &height ) {
                return render::sampleTerrainHeight( *terrain, placement, point, height );
            };
            for( auto lot : input.lots )
            {
                const Rectangle footprint{ lot.parcelX, lot.parcelZ, lot.parcelWidth, lot.parcelDepth };
                bool excluded = false;
                for( const auto &segment : segments )
                {
                    require( ++clearanceVisits <= 32000000, "Parcel road-clearance work budget exceeded." );
                    if( segmentRectangleSquared( segment, footprint ) <= segment.clearance * segment.clearance ) { excluded = true; break; }
                }
                if( !excluded ) for( const auto &junction : source.roundabouts ) if( junctionOverlap( footprint, junction ) ) { excluded = true; break; }
                if( excluded ) { ++nextCulled; continue; }
                ++nextChecks;
                auto &sweep = rectangle.roads[0];
                const bool alongX = lot.parcelWidth > lot.parcelDepth;
                sweep.width = std::min( lot.parcelWidth, lot.parcelDepth );
                const Vector3F span = alongX ? Vector3F( lot.parcelWidth, 0, 0 ) : Vector3F( 0, 0, lot.parcelDepth );
                const Vector3F center{ lot.parcelX, 0, lot.parcelZ };
                sweep.knots[0].position = center - span * .5f; sweep.knots[1].position = center + span * .5f;
                sweep.knots[0].outHandle = span / 3.f; sweep.knots[1].inHandle = -span / 3.f;
                const auto denialBefore = sourceBudgetDenialCount();
                procedural::CityRoadSplineBake rectangleBake;
                CityRoadTerrainMesh surface;
                if( !procedural::sampleCityRoadSplines( rectangle, {}, sampler, rectangleBake, error ) ||
                    !buildCityRoadTerrainMesh( rectangle, rectangleBake, terrain, placement, surface, error ) )
                {
                    if( !unsupportedTerrain( error, denialBefore ) ) throw std::runtime_error( error.c_str() );
                    ++nextCulled; continue;
                }
                fittedVertices += surface.vertices.size();
                require( fittedVertices <= 8000000, "Parcel cumulative terrain-fit work budget exceeded." );
                require( !surface.vertices.empty(), "Parcel full-footprint support unexpectedly produced no surface." );
                float minimum = std::numeric_limits<float>::max(), maximum = -minimum;
                for( const auto &vertex : surface.vertices )
                { minimum = std::min( minimum, vertex.position.y ); maximum = std::max( maximum, vertex.position.y ); }
                if( maximum - minimum > 1.25f + 1e-5f ) { ++nextCulled; continue; }
                lot.groundHeight = maximum; lot.groundMinimumHeight = minimum;
                candidate.lots.push_back( lot );
            }
            output.swap( candidate ); culled = nextCulled; supportChecks = nextChecks;
            error.clear(); return true;
        }
        catch( const std::exception &exception ) { error = exception.what(); return false; }
    }
}
