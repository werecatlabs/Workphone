#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CityRoadJunctions.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        constexpr double pi = 3.14159265358979323846;
        constexpr double contactTolerance = .002;
        constexpr u64 maximumVisits = 2000000;
        using Sample = procedural::CityRoadSplineSample;
        struct P { double x = 0, z = 0; };
        P point( Vector3F p ) { return { p.x, p.z }; }
        P operator+( P a, P b ) { return { a.x + b.x, a.z + b.z }; }
        P operator-( P a, P b ) { return { a.x - b.x, a.z - b.z }; }
        P operator*( P a, double b ) { return { a.x * b, a.z * b }; }
        double dot( P a, P b ) { return a.x * b.x + a.z * b.z; }
        double cross( P a, P b ) { return a.x * b.z - a.z * b.x; }
        double length( P a ) { return std::hypot( a.x, a.z ); }
        P unit( P a ) { const auto n = length( a ); return n > 1e-9 ? a * ( 1 / n ) : P{}; }
        P left( P a ) { return { -a.z, a.x }; }
        void require( bool value, const char *message ) { if( !value ) throw std::runtime_error( message ); }
        struct Bounds
        {
            double x0, x1, z0, z1;
            bool overlaps( const Bounds &b ) const
            { return x1 + contactTolerance >= b.x0 && b.x1 + contactTolerance >= x0 &&
                     z1 + contactTolerance >= b.z0 && b.z1 + contactTolerance >= z0; }
        };
        Bounds bounds( P a, P b )
        { return { std::min( a.x, b.x ), std::max( a.x, b.x ), std::min( a.z, b.z ), std::max( a.z, b.z ) }; }
        struct Hit { u32 road = 0; double station = 0; };
        struct Contact { P center; SourceBudgetVector<Hit> hits; };
        struct Arm
        {
            u32 road = 0;
            double station = 0, angle = 0, trim = 0, available = 0;
            bool forward = true;
            P direction;
        };
        struct Plan { Contact contact; SourceBudgetVector<Arm> arms; };
        struct Cut { u32 road = 0; double first = 0, last = 0; };

        Sample at( const SourceBudgetVector<Sample> &samples, double station )
        {
            require( samples.size() >= 2 && std::isfinite( station ), "Junction station is invalid." );
            if( station <= samples.front().distance ) return samples.front();
            if( station >= samples.back().distance ) return samples.back();
            const auto after = std::upper_bound( samples.begin(), samples.end(), station,
                []( double distance, const Sample &sample ) { return distance < sample.distance; } );
            require( after != samples.begin() && after != samples.end(), "Junction station could not be located." );
            const auto &a = *( after - 1 ), &b = *after;
            const float t = float( ( station - a.distance ) / ( b.distance - a.distance ) );
            Sample result;
            result.position = a.position + ( b.position - a.position ) * t;
            result.left = a.left + ( b.left - a.left ) * t;
            result.right = a.right + ( b.right - a.right ) * t;
            result.tangent = a.tangent + ( b.tangent - a.tangent ) * t;
            if( std::hypot( result.tangent.x, result.tangent.z ) < 1e-6f )
                result.tangent = b.position - a.position;
            result.distance = float( station );
            return result;
        }

        P directionAt( const SourceBudgetVector<Sample> &samples, double station )
        {
            auto sample = at( samples, station );
            auto direction = unit( point( sample.tangent ) );
            if( length( direction ) < .5 )
                direction = unit( point( at( samples, std::min( double( samples.back().distance ), station + .1 ) ).position ) -
                                  point( at( samples, std::max( 0., station - .1 ) ).position ) );
            require( length( direction ) > .5, "Junction approach has no horizontal direction." );
            return direction;
        }

        Vector3F grounded( P p, float offset, const procedural::CityRoadHeightSampler &terrain )
        {
            Vector3F result{ float( p.x ), 0, float( p.z ) };
            float y = 0;
            require( terrain && terrain( result, y ) && std::isfinite( y ),
                     "Junction boundary has no terrain support." );
            result.y = y + offset;
            return result;
        }

        double angleGap( double first, double last )
        { auto gap = last - first; while( gap <= 0 ) gap += 2 * pi; return gap; }

        void addHit( Contact &contact, u32 road, double station )
        {
            for( const auto &hit : contact.hits )
                if( hit.road == road )
                {
                    require( std::abs( hit.station - station ) <= .01,
                             "One road revisits the same junction footprint." );
                    return;
                }
            contact.hits.push_back( { road, station } );
        }

        SourceBudgetVector<Contact> contacts( const procedural::CityRoadSplineSource &source,
                                              const procedural::CityRoadSplineBake &bake )
        {
            SourceBudgetVector<Bounds> roadBounds;
            SourceBudgetVector<SourceBudgetVector<Bounds>> segmentBounds;
            size_t sampleCount = 0;
            for( size_t r = 0; r < bake.roads.size(); ++r )
            {
                const auto &samples = bake.roads[r].points;
                require( bake.roads[r].roadId == source.roads[r].id && samples.size() >= 2,
                         "Junction bake has missing or mismatched road samples." );
                sampleCount += samples.size();
                require( sampleCount <= 65536, "Junction input exceeds its sample budget." );
                Bounds bound = bounds( point( samples.front().position ), point( samples.front().position ) );
                SourceBudgetVector<Bounds> segments;
                segments.reserve( samples.size() - 1 );
                for( size_t i = 0; i < samples.size(); ++i )
                {
                    const auto &p = samples[i];
                    const auto finite = []( Vector3F value ) {
                        return std::isfinite( value.x ) && std::isfinite( value.y ) && std::isfinite( value.z );
                    };
                    require( finite( p.position ) && finite( p.left ) && finite( p.right ) && finite( p.tangent ) &&
                             std::isfinite( p.distance ) && p.distance >= 0 &&
                             ( !i || p.distance > samples[i - 1].distance ),
                             "Junction samples must be finite with increasing arc distance." );
                    require( !i || length( point( p.position ) - point( samples[i - 1].position ) ) > 1e-6,
                             "Junction input contains a zero horizontal span." );
                    bound.x0 = std::min( bound.x0, double( p.position.x ) );
                    bound.x1 = std::max( bound.x1, double( p.position.x ) );
                    bound.z0 = std::min( bound.z0, double( p.position.z ) );
                    bound.z1 = std::max( bound.z1, double( p.position.z ) );
                    if( i ) segments.push_back( bounds( point( samples[i - 1].position ), point( p.position ) ) );
                }
                roadBounds.push_back( bound ); segmentBounds.push_back( std::move( segments ) );
            }
            SourceBudgetVector<Contact> result;
            u64 visits = 0;
            for( u32 a = 0; a < bake.roads.size(); ++a ) for( u32 b = a + 1; b < bake.roads.size(); ++b )
            {
                if( !roadBounds[a].overlaps( roadBounds[b] ) ) continue;
                const auto &first = bake.roads[a].points, &second = bake.roads[b].points;
                for( size_t i = 1; i < first.size(); ++i )
                {
                    if( !segmentBounds[a][i - 1].overlaps( roadBounds[b] ) ) continue;
                    for( size_t j = 1; j < second.size(); ++j )
                    {
                        require( ++visits <= maximumVisits, "Junction contact search exceeded its work budget." );
                        if( !segmentBounds[a][i - 1].overlaps( segmentBounds[b][j - 1] ) ) continue;
                        const auto p = point( first[i - 1].position ), q = point( second[j - 1].position );
                        const auto da = point( first[i].position ) - p, db = point( second[j].position ) - q;
                        const auto delta = q - p;
                        const double denominator = cross( da, db );
                        double t = 0, u = 0;
                        if( std::abs( denominator ) > 1e-10 * length( da ) * length( db ) )
                        {
                            t = cross( delta, db ) / denominator; u = cross( delta, da ) / denominator;
                            if( t < -1e-6 || t > 1.000001 || u < -1e-6 || u > 1.000001 ) continue;
                        }
                        else
                        {
                            if( std::abs( cross( delta, da ) ) > contactTolerance * length( da ) ) continue;
                            const double d2 = dot( da, da );
                            require( d2 > 1e-10, "Junction input contains a zero-span segment." );
                            const double from = dot( delta, da ) / d2, to = dot( delta + db, da ) / d2;
                            const double lo = std::max( 0., std::min( from, to ) ), hi = std::min( 1., std::max( from, to ) );
                            if( hi < lo - 1e-6 ) continue;
                            require( ( hi - lo ) * std::sqrt( d2 ) <= contactTolerance,
                                     "Coincident road runs cannot form an unambiguous junction." );
                            t = std::clamp( ( lo + hi ) * .5, 0., 1. );
                            u = dot( p + da * t - q, db ) / dot( db, db );
                        }
                        t = std::clamp( t, 0., 1. ); u = std::clamp( u, 0., 1. );
                        const auto location = p + da * t;
                        const double sa = first[i - 1].distance + ( first[i].distance - first[i - 1].distance ) * t;
                        const double sb = second[j - 1].distance + ( second[j].distance - second[j - 1].distance ) * u;
                        auto found = std::find_if( result.begin(), result.end(), [&]( const Contact &contact ) {
                            return length( contact.center - location ) <= contactTolerance;
                        } );
                        if( found == result.end() )
                        {
                            require( result.size() < 256, "Junction count exceeds its bounded capacity." );
                            result.push_back( { location, {} } ); found = result.end() - 1;
                        }
                        addHit( *found, a, sa ); addHit( *found, b, sb );
                    }
                }
            }
            return result;
        }

        void appendDistinct( SourceBudgetVector<Vector3F> &points, Vector3F position )
        { if( points.empty() || length( point( points.back() ) - point( position ) ) > .00001 ) points.push_back( position ); }

        void fitCurvedCaps( Plan &plan, const procedural::CityRoadSplineBake &bake,
                            const CityRoadJunctionOptions &options )
        {
            for( unsigned iteration = 0; iteration < 16; ++iteration )
            {
                std::array<P, 4> directions{}, edgeLeft{}, edgeRight{};
                std::array<double, 4> growth{};
                for( size_t i = 0; i < plan.arms.size(); ++i )
                {
                    const auto &arm = plan.arms[i];
                    const float station = float( arm.station + ( arm.forward ? arm.trim : -arm.trim ) );
                    const auto sample = at( bake.roads[arm.road].points, station );
                    directions[i] = directionAt( bake.roads[arm.road].points, station ) * ( arm.forward ? 1. : -1. );
                    edgeLeft[i] = point( arm.forward ? sample.right : sample.left );
                    edgeRight[i] = point( arm.forward ? sample.left : sample.right );
                }
                for( size_t i = 0; i < plan.arms.size(); ++i )
                {
                    const size_t next = ( i + 1 ) % plan.arms.size();
                    const auto da = directions[i], db = directions[next];
                    const double gap = angleGap( std::atan2( da.z, da.x ), std::atan2( db.z, db.x ) );
                    if( gap >= pi - .01 ) continue;
                    require( gap >= pi / 6, "Curved junction approaches become too acute at their caps." );
                    const double denominator = cross( da, db );
                    const auto delta = edgeRight[next] - edgeLeft[i];
                    const double tangent = options.cornerRadius / std::tan( gap * .5 );
                    const double a = cross( delta, db ) / denominator + tangent + .25;
                    const double b = cross( delta, da ) / denominator + tangent + .25;
                    if( a > 0 ) growth[i] = std::max( growth[i], a + .25 );
                    if( b > 0 ) growth[next] = std::max( growth[next], b + .25 );
                }
                bool changed = false;
                for( size_t i = 0; i < plan.arms.size(); ++i ) if( growth[i] > 0 )
                {
                    auto &arm = plan.arms[i];
                    require( arm.trim + growth[i] <= arm.available,
                             "Curved junction radius cannot fit within the available approach run." );
                    arm.trim += growth[i]; changed = true;
                }
                if( !changed ) return;
            }
            throw std::runtime_error( "Curved junction cap fitting exceeded its bounded iterations." );
        }

        using Triangle = std::array<P, 3>;
        Bounds bounds( const Triangle &triangle )
        {
            auto result = bounds( triangle[0], triangle[1] );
            result.x0 = std::min( result.x0, triangle[2].x ); result.x1 = std::max( result.x1, triangle[2].x );
            result.z0 = std::min( result.z0, triangle[2].z ); result.z1 = std::max( result.z1, triangle[2].z );
            return result;
        }

        bool overlapsInterior( const Triangle &a, const Triangle &b )
        {
            if( std::abs( cross( a[1] - a[0], a[2] - a[0] ) ) < 1e-10 ||
                std::abs( cross( b[1] - b[0], b[2] - b[0] ) ) < 1e-10 ) return false;
            for( const auto *triangle : { &a, &b } ) for( size_t edge = 0; edge < 3; ++edge )
            {
                const auto axis = left( unit( ( *triangle )[( edge + 1 ) % 3] - ( *triangle )[edge] ) );
                if( length( axis ) < .5 ) continue;
                double a0 = dot( a[0], axis ), a1 = a0, b0 = dot( b[0], axis ), b1 = b0;
                for( size_t i = 1; i < 3; ++i )
                {
                    const double pa = dot( a[i], axis ), pb = dot( b[i], axis );
                    a0 = std::min( a0, pa ); a1 = std::max( a1, pa );
                    b0 = std::min( b0, pb ); b1 = std::max( b1, pb );
                }
                // Shared port edges are intentional. Only positive-area overlap
                // beyond 10 micrometres is an unsolved carriageway conflict.
                if( std::min( a1, b1 ) - std::max( a0, b0 ) <= .00001 ) return false;
            }
            return true;
        }

        bool overlapsDisc( const Triangle &triangle, P center, double radius )
        {
            const double sign = cross( triangle[1] - triangle[0], triangle[2] - triangle[0] );
            if( std::abs( sign ) < 1e-10 ) return false;
            bool inside = true;
            double nearest = std::numeric_limits<double>::max();
            for( size_t i = 0; i < 3; ++i )
            {
                const auto a = triangle[i], b = triangle[( i + 1 ) % 3], direction = b - a;
                inside = inside && cross( direction, center - a ) * sign >= 0;
                const double span = dot( direction, direction );
                const double t = span > 1e-12 ? std::clamp( dot( center - a, direction ) / span, 0., 1. ) : 0;
                nearest = std::min( nearest, length( a + direction * t - center ) );
            }
            return inside || nearest < radius - .00001;
        }

        void validateFootprints( const CityRoadJunctionBake &bake,
                                 const procedural::CityRoadSplineSource &source )
        {
            struct Patch { Bounds bound; SourceBudgetVector<Triangle> triangles; SourceBudgetVector<Bounds> bounds; };
            SourceBudgetVector<Patch> patches;
            for( const auto &junction : bake.junctions )
            {
                Patch patch; patch.bound = bounds( point( junction.center ), point( junction.center ) );
                const auto vertex = [&]( u32 index ) { return point( index ? junction.boundary[index - 1] : junction.center ); };
                for( size_t i = 0; i < junction.indices.size(); i += 3 )
                {
                    const Triangle triangle{ vertex( junction.indices[i] ), vertex( junction.indices[i + 1] ), vertex( junction.indices[i + 2] ) };
                    const auto bound = bounds( triangle );
                    patch.bound.x0 = std::min( patch.bound.x0, bound.x0 ); patch.bound.x1 = std::max( patch.bound.x1, bound.x1 );
                    patch.bound.z0 = std::min( patch.bound.z0, bound.z0 ); patch.bound.z1 = std::max( patch.bound.z1, bound.z1 );
                    patch.triangles.push_back( triangle ); patch.bounds.push_back( bound );
                }
                patches.push_back( std::move( patch ) );
            }
            SourceBudgetVector<Bounds> runBounds;
            for( const auto &run : bake.roadRuns )
            {
                auto bound = bounds( point( run.points.front().left ), point( run.points.front().right ) );
                for( const auto &sample : run.points ) for( const auto p : { sample.left, sample.right } )
                {
                    bound.x0 = std::min( bound.x0, double( p.x ) ); bound.x1 = std::max( bound.x1, double( p.x ) );
                    bound.z0 = std::min( bound.z0, double( p.z ) ); bound.z1 = std::max( bound.z1, double( p.z ) );
                }
                runBounds.push_back( bound );
            }
            u64 visits = 0;
            for( size_t i = 0; i < patches.size(); ++i )
            {
                const auto &patch = patches[i];
                for( size_t r = 0; r < bake.roadRuns.size(); ++r )
                {
                    if( !patch.bound.overlaps( runBounds[r] ) ) continue;
                    const auto &points = bake.roadRuns[r].points;
                    for( size_t s = 1; s < points.size(); ++s )
                    {
                        require( ++visits <= maximumVisits, "Junction overlap validation exceeded its work budget." );
                        const auto &a = points[s - 1], &b = points[s];
                        for( const auto triangle : { Triangle{ point( a.left ), point( a.right ), point( b.left ) },
                                                    Triangle{ point( a.right ), point( b.right ), point( b.left ) } } )
                        {
                            const auto bound = bounds( triangle );
                            if( !patch.bound.overlaps( bound ) ) continue;
                            for( size_t t = 0; t < patch.triangles.size(); ++t )
                            {
                                if( !patch.bounds[t].overlaps( bound ) ) continue;
                                require( ++visits <= maximumVisits, "Junction overlap validation exceeded its work budget." );
                                require( !overlapsInterior( patch.triangles[t], triangle ),
                                         "Rounded junction overlaps a retained road ribbon." );
                            }
                        }
                    }
                }
                for( size_t j = i + 1; j < patches.size(); ++j )
                {
                    const auto &other = patches[j];
                    if( !patch.bound.overlaps( other.bound ) ) continue;
                    for( size_t a = 0; a < patch.triangles.size(); ++a ) for( size_t b = 0; b < other.triangles.size(); ++b )
                    {
                        require( ++visits <= maximumVisits, "Junction overlap validation exceeded its work budget." );
                        if( patch.bounds[a].overlaps( other.bounds[b] ) )
                            require( !overlapsInterior( patch.triangles[a], other.triangles[b] ),
                                     "Rounded junction footprints overlap one another." );
                    }
                }
                for( const auto &roundabout : source.roundabouts )
                {
                    const auto center = point( roundabout.center );
                    const double radius = roundabout.outerRadius;
                    const Bounds discBounds{ center.x - radius, center.x + radius,
                                             center.z - radius, center.z + radius };
                    if( patch.bound.overlaps( discBounds ) ) for( size_t t = 0; t < patch.triangles.size(); ++t )
                    {
                        if( !patch.bounds[t].overlaps( discBounds ) ) continue;
                        require( ++visits <= maximumVisits, "Junction overlap validation exceeded its work budget." );
                        require( !overlapsDisc( patch.triangles[t], center, radius ),
                                 "Rounded junction overlaps an existing roundabout disc." );
                    }
                    for( const auto &port : roundabout.ports )
                    {
                        const auto direction = unit( point( port.outward ) ), normal = left( direction );
                        const double halfWidth = port.width * .5;
                        const auto start = center + direction * std::sqrt( std::max( 0., radius * radius - halfWidth * halfWidth ) );
                        const auto end = center + direction * ( radius + port.length );
                        const auto a = start + normal * halfWidth, b = start - normal * halfWidth,
                                   c = end + normal * halfWidth, d = end - normal * halfWidth;
                        for( const auto triangle : { Triangle{ a, b, c }, Triangle{ b, d, c } } )
                        {
                            const auto bound = bounds( triangle );
                            if( !patch.bound.overlaps( bound ) ) continue;
                            for( size_t t = 0; t < patch.triangles.size(); ++t )
                            {
                                if( !patch.bounds[t].overlaps( bound ) ) continue;
                                require( ++visits <= maximumVisits, "Junction overlap validation exceeded its work budget." );
                                require( !overlapsInterior( patch.triangles[t], triangle ),
                                         "Rounded junction overlaps an existing roundabout approach." );
                            }
                        }
                    }
                }
            }
        }

        CityRoadJunction buildPatch( Plan &plan, const procedural::CityRoadSplineSource &source,
                                    const procedural::CityRoadSplineBake &bake,
                                    const procedural::CityRoadHeightSampler &terrain,
                                    const CityRoadJunctionOptions &options )
        {
            CityRoadJunction patch;
            patch.heightOffset = source.roads[plan.arms.front().road].heightOffset;
            patch.maxGrade = source.roads[plan.arms.front().road].maxGrade;
            patch.center = grounded( plan.contact.center, patch.heightOffset, terrain );
            SourceBudgetVector<P> directions, edgeLeft, edgeRight;
            for( const auto &arm : plan.arms )
            {
                const auto &road = source.roads[arm.road];
                require( road.heightMode == procedural::CityRoadHeightMode::Drape && !road.closed,
                         "Ordinary junctions require open draped roads." );
                require( std::abs( road.heightOffset - patch.heightOffset ) <= .00001f,
                         "Connected roads have incompatible surface height offsets." );
                patch.maxGrade = std::min( patch.maxGrade, road.maxGrade );
                CityRoadJunctionPort port;
                port.roadIndex = arm.road; port.forward = arm.forward;
                port.station = float( arm.station + ( arm.forward ? arm.trim : -arm.trim ) );
                port.boundary = at( bake.roads[arm.road].points, port.station );
                const auto direction = directionAt( bake.roads[arm.road].points, port.station ) * ( arm.forward ? 1. : -1. );
                directions.push_back( direction );
                edgeLeft.push_back( point( arm.forward ? port.boundary.right : port.boundary.left ) );
                edgeRight.push_back( point( arm.forward ? port.boundary.left : port.boundary.right ) );
                patch.ports.push_back( port );
            }
            for( size_t i = 0; i < plan.arms.size(); ++i )
            {
                const size_t next = ( i + 1 ) % plan.arms.size();
                const auto a = edgeLeft[i], b = edgeRight[next], da = directions[i], db = directions[next];
                const double gap = angleGap( std::atan2( da.z, da.x ), std::atan2( db.z, db.x ) );
                CityRoadJunctionCorner corner;
                const auto append = [&]( P point ) { appendDistinct( corner.points, grounded( point, patch.heightOffset, terrain ) ); };
                append( a );
                if( gap < pi - .01 )
                {
                    require( gap >= pi / 6, "Junction approaches are too acute for a bounded connector." );
                    const double denominator = cross( da, db );
                    const double alongA = cross( b - a, db ) / denominator;
                    const double alongB = cross( b - a, da ) / denominator;
                    const auto intersection = a + da * alongA;
                    const double tangent = options.cornerRadius / std::tan( gap * .5 );
                    require( alongA + tangent <= -.01 && alongB + tangent <= -.01,
                             "Junction radius does not fit between its approach caps." );
                    const auto first = intersection + da * tangent, last = intersection + db * tangent;
                    const auto center = intersection + unit( da + db ) * ( options.cornerRadius / std::sin( gap * .5 ) );
                    append( first );
                    const double startAngle = std::atan2( first.z - center.z, first.x - center.x );
                    for( u32 part = 1; part < options.cornerSegments; ++part )
                    {
                        const double angle = startAngle + ( gap - pi ) * part / options.cornerSegments;
                        append( center + P{ std::cos( angle ), std::sin( angle ) } * options.cornerRadius );
                    }
                    append( last );
                }
                else if( gap > pi + .01 )
                {
                    // A missing fourth arm leaves a convex back edge. A hub arc
                    // supports skewed T/Y contacts without crossing the centre.
                    const double radiusA = source.roads[plan.arms[i].road].width * .5;
                    const double radiusB = source.roads[plan.arms[next].road].width * .5;
                    const double startAngle = std::atan2( da.z, da.x ) + pi * .5;
                    for( u32 part = 0; part <= options.cornerSegments; ++part )
                    {
                        const double t = double( part ) / options.cornerSegments, blend = t * t * ( 3 - 2 * t );
                        const double angle = startAngle + ( gap - pi ) * t;
                        append( plan.contact.center + P{ std::cos( angle ), std::sin( angle ) } *
                            ( radiusA + ( radiusB - radiusA ) * blend ) );
                    }
                }
                append( b );
                require( corner.points.size() >= 2, "Junction corner has no boundary span." );
                corner.outward.reserve( corner.points.size() );
                for( size_t p = 0; p < corner.points.size(); ++p )
                {
                    P offset;
                    if( p == 0 )
                    {
                        const auto &port = patch.ports[i];
                        offset = unit( point( port.boundary.right ) - point( port.boundary.left ) ) * ( port.forward ? 1. : -1. );
                    }
                    else if( p + 1 == corner.points.size() )
                    {
                        const auto &port = patch.ports[next];
                        offset = unit( point( port.boundary.right ) - point( port.boundary.left ) ) * ( port.forward ? -1. : 1. );
                    }
                    else
                    {
                        const auto first = unit( point( corner.points[p] ) - point( corner.points[p - 1] ) );
                        const auto last = unit( point( corner.points[p + 1] ) - point( corner.points[p] ) );
                        const P n0{ first.z, -first.x }, n1{ last.z, -last.x };
                        const auto sum = unit( n0 + n1 );
                        const double denominator = dot( sum, n0 );
                        require( denominator >= .25, "Junction footway boundary has an excessive miter." );
                        offset = sum * ( 1 / denominator );
                    }
                    corner.outward.push_back( { float( offset.x ), 0, float( offset.z ) } );
                }
                appendDistinct( patch.boundary, grounded( edgeRight[i], patch.heightOffset, terrain ) );
                for( size_t p = 0; p + 1 < corner.points.size(); ++p ) appendDistinct( patch.boundary, corner.points[p] );
                patch.corners.push_back( std::move( corner ) );
            }
            double winding = 0;
            for( size_t i = 0; i < patch.boundary.size(); ++i )
            {
                const auto a = point( patch.boundary[i] ) - plan.contact.center;
                const auto b = point( patch.boundary[( i + 1 ) % patch.boundary.size()] ) - plan.contact.center;
                const double area = cross( a, b );
                require( area >= -1e-5, "Junction footprint folds across its centre." );
                winding += std::atan2( area, dot( a, b ) );
                if( area > 1e-7 )
                {
                    patch.indices.push_back( 0 ); patch.indices.push_back( u32( i + 1 ) );
                    patch.indices.push_back( u32( ( i + 1 ) % patch.boundary.size() + 1 ) );
                }
            }
            require( std::abs( winding - 2 * pi ) < .001 && !patch.indices.empty(),
                     "Junction boundary is not a simple connected footprint." );
            return patch;
        }
    }

    void CityRoadJunctionBake::swap( CityRoadJunctionBake &other ) noexcept
    { std::swap( sourceRevision, other.sourceRevision ); roadRuns.swap( other.roadRuns ); junctions.swap( other.junctions ); }

    bool buildCityRoadJunctions( const procedural::CityRoadSplineSource &source,
                                const procedural::CityRoadSplineBake &bake,
                                const procedural::CityRoadHeightSampler &terrain,
                                const CityRoadJunctionOptions &options,
                                CityRoadJunctionBake &output, String &error )
    {
        try
        {
            if( !procedural::validateCityRoadSplineSource( source, error ) ) throw std::runtime_error( error.c_str() );
            require( bake.sourceRevision == source.revision && bake.roads.size() == source.roads.size(),
                     "Junction bake is stale or does not match its source." );
            require( std::isfinite( options.cornerRadius ) && options.cornerRadius >= .25f && options.cornerRadius <= 16 &&
                     options.cornerSegments >= 2 && options.cornerSegments <= 24 && std::isfinite( options.maximumTrim ) &&
                     options.maximumTrim >= 2 && options.maximumTrim <= 64,
                     "Invalid bounded junction geometry options." );
            CityRoadJunctionBake candidate; candidate.sourceRevision = source.revision;
            SourceBudgetVector<Plan> plans;
            for( auto &contact : contacts( source, bake ) )
            {
                Plan plan; plan.contact = std::move( contact );
                for( const auto &hit : plan.contact.hits )
                {
                    const auto &samples = bake.roads[hit.road].points;
                    const auto direction = directionAt( samples, hit.station );
                    for( bool forward : { false, true } )
                    {
                        const double available = forward ? samples.back().distance - hit.station : hit.station;
                        if( available < .001 ) continue;
                        const auto outward = direction * ( forward ? 1. : -1. );
                        plan.arms.push_back( { hit.road, hit.station, std::atan2( outward.z, outward.x ),
                                               0, available - .02, forward, outward } );
                    }
                }
                if( plan.arms.size() == 2 )
                {
                    const auto &a = plan.arms[0], &b = plan.arms[1];
                    if( source.roads[a.road].width == source.roads[b.road].width ) continue;
                }
                if( plan.arms.size() < 2 ) continue;
                require( plan.arms.size() <= 4, "Ordinary junctions support at most four distinct approaches." );
                std::sort( plan.arms.begin(), plan.arms.end(), []( const Arm &a, const Arm &b ) { return a.angle < b.angle; } );
                plans.push_back( std::move( plan ) );
            }
            SourceBudgetVector<Cut> cuts;
            for( auto &plan : plans )
            {
                for( auto &arm : plan.arms )
                {
                    arm.available = std::min( arm.available, double( options.maximumTrim ) );
                    for( const auto &other : plans ) if( &other != &plan ) for( const auto &hit : other.contact.hits )
                    {
                        if( hit.road != arm.road ) continue;
                        const double delta = ( hit.station - arm.station ) * ( arm.forward ? 1 : -1 );
                        if( delta > .001 ) arm.available = std::min( arm.available, delta * .49 );
                    }
                    arm.trim = std::max( 2., double( options.cornerRadius ) );
                }
                for( size_t i = 0; i < plan.arms.size(); ++i )
                {
                    auto &a = plan.arms[i], &b = plan.arms[( i + 1 ) % plan.arms.size()];
                    const double gap = angleGap( a.angle, b.angle );
                    if( gap >= pi - .01 ) continue;
                    require( gap >= pi / 6, "Junction approaches are too acute for a bounded connector." );
                    const auto edgeA = left( a.direction ) * ( source.roads[a.road].width * .5 );
                    const auto edgeB = left( b.direction ) * ( -source.roads[b.road].width * .5 );
                    const double denominator = cross( a.direction, b.direction );
                    const double alongA = cross( edgeB - edgeA, b.direction ) / denominator;
                    const double alongB = cross( edgeB - edgeA, a.direction ) / denominator;
                    const double tangent = options.cornerRadius / std::tan( gap * .5 );
                    a.trim = std::max( a.trim, alongA + tangent + .5 );
                    b.trim = std::max( b.trim, alongB + tangent + .5 );
                }
                for( const auto &arm : plan.arms ) require( arm.trim <= arm.available,
                    "Consecutive junctions leave insufficient room for the configured corner radius." );
                fitCurvedCaps( plan, bake, options );
                auto patch = buildPatch( plan, source, bake, terrain, options );
                for( const auto &hit : plan.contact.hits )
                {
                    Cut cut{ hit.road, hit.station, hit.station };
                    // Use the very same rounded float station as the published
                    // cap, so ribbon and connector share bit-identical edges.
                    for( const auto &port : patch.ports ) if( port.roadIndex == hit.road )
                    { if( port.forward ) cut.last = port.station; else cut.first = port.station; }
                    cuts.push_back( cut );
                }
                candidate.junctions.push_back( std::move( patch ) );
            }
            std::sort( cuts.begin(), cuts.end(), []( const Cut &a, const Cut &b ) {
                return a.road != b.road ? a.road < b.road : a.first < b.first;
            } );
            for( u32 r = 0; r < source.roads.size(); ++r )
            {
                const auto &samples = bake.roads[r].points;
                const auto appendRun = [&]( double first, double last ) {
                    if( last - first < .001 ) return;
                    CityRoadJunctionRun run; run.roadIndex = r;
                    run.points.push_back( at( samples, first ) );
                    for( const auto &sample : samples ) if( sample.distance > first + .00001 && sample.distance < last - .00001 )
                        run.points.push_back( sample );
                    run.points.push_back( at( samples, last ) );
                    candidate.roadRuns.push_back( std::move( run ) );
                };
                double station = samples.front().distance;
                for( const auto &cut : cuts ) if( cut.road == r )
                {
                    require( cut.first >= station - .001, "Neighbouring junction footprints overlap along a road." );
                    appendRun( station, cut.first ); station = cut.last;
                }
                appendRun( station, samples.back().distance );
            }
            validateFootprints( candidate, source );
            output.swap( candidate ); error.clear(); return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }
}  // namespace workphone::scene
