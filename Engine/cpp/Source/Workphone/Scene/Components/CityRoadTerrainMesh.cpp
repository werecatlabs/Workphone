#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CityRoadTerrainMesh.hpp>
#include <Workphone/Interface/Procedural/CityRoadRoundabout.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace workphone::scene
{
    namespace
    {
        constexpr u64 maximumCellVisits = 2000000;
        constexpr size_t maximumVertices = 1000000;
        struct Point { double x = 0, z = 0, y = 0, u = 0, v = 0; };
        using Polygon = std::array<Point, 10>;
        struct Policy
        {
            bool drape = false, roundabout = false;
            float offset = 0, maxGrade = .08f, tolerance = .03f;
            Vector3F center{};
            float gradeX = 0, gradeZ = 0;
            CityRoadTerrainTriangleKind kind = CityRoadTerrainTriangleKind::Road;
        };
        void require( bool condition, const char *message )
        { if( !condition ) throw std::runtime_error( message ); }
        double area( Point a, Point b, Point c )
        { return ( b.x - a.x ) * ( c.z - a.z ) - ( b.z - a.z ) * ( c.x - a.x ); }
        Point lerp( Point a, Point b, double t )
        {
            return { a.x + ( b.x - a.x ) * t, a.z + ( b.z - a.z ) * t,
                     a.y + ( b.y - a.y ) * t, a.u + ( b.u - a.u ) * t,
                     a.v + ( b.v - a.v ) * t };
        }
        size_t clip( const Polygon &input, size_t count, Point a, Point b, Polygon &output )
        {
            size_t size = 0;
            if( !count ) return 0;
            auto previous = input[count - 1];
            double previousDistance = area( a, b, previous );
            for( size_t i = 0; i < count; ++i )
            {
                const auto current = input[i];
                const double distance = area( a, b, current );
                const bool inside = distance >= 0, previousInside = previousDistance >= 0;
                if( inside != previousInside )
                {
                    require( size < output.size(), "Road clipping polygon capacity exceeded." );
                    output[size++] = lerp( previous, current,
                        std::clamp( previousDistance / ( previousDistance - distance ), 0.0, 1.0 ) );
                }
                if( inside )
                {
                    require( size < output.size(), "Road clipping polygon capacity exceeded." );
                    output[size++] = current;
                }
                previous = current; previousDistance = distance;
            }
            return size;
        }
        double polygonArea( const Polygon &polygon, size_t size )
        {
            double sum = 0;
            for( size_t i = 1; i + 1 < size; ++i ) sum += area( polygon[0], polygon[i], polygon[i + 1] );
            return std::abs( sum ) * .5;
        }

        void validatePort( const procedural::CityRoadRoundaboutSpec &junction,
                           const procedural::CityRoadRoundaboutPort &port,
                           const procedural::CityRoadSplineSource &source,
                           const procedural::CityRoadSplineBake &bake,
                           const render::TerrainData &terrain, const Transform3<real_Num> &placement )
        {
            size_t index = source.roads.size();
            for( size_t i = 0; i < source.roads.size(); ++i )
                if( source.roads[i].id == port.approachId ) { index = i; break; }
            require( index < source.roads.size(), "Roundabout approach references a missing road." );
            const auto &road = source.roads[index];
            const auto &samples = bake.roads[index].points;
            require( !road.closed && std::abs( road.width - port.width ) <= .001f,
                     "Roundabout approach requires an open road with matching width within 0.001 m." );
            const float length = std::hypot( port.outward.x, port.outward.z );
            require( length > 1e-5f, "Roundabout approach direction is invalid." );
            const Vector3F direction{ port.outward.x / length, 0, port.outward.z / length };
            const auto plane = [&]( Vector3F point ) {
                point.y = junction.center.y + junction.gradeX * ( point.x - junction.center.x ) +
                    junction.gradeZ * ( point.z - junction.center.z ); return point;
            };
            const auto center = plane( junction.center + direction * ( junction.outerRadius + port.length ) );
            const auto fitted = [&]( Vector3F point ) {
                if( road.heightMode == procedural::CityRoadHeightMode::Drape )
                {
                    float height = 0;
                    require( render::sampleTerrainHeight( terrain, placement, point, height ),
                             "Roundabout approach endpoint has no terrain support." );
                    point.y = height + road.heightOffset;
                }
                return point;
            };
            const bool atStart = ( fitted( samples.front().position ) - center ).length() <= .001f;
            const bool atEnd = ( fitted( samples.back().position ) - center ).length() <= .001f;
            require( atStart != atEnd,
                     "Roundabout approach must meet exactly one road endpoint within 0.001 m." );
            const auto &endpoint = atStart ? samples.front() : samples.back();
            const float tangentLength = std::hypot( endpoint.tangent.x, endpoint.tangent.z );
            require( std::isfinite( tangentLength ) && tangentLength > 1e-5f,
                     "Roundabout approach tangent is invalid." );
            const float sign = atStart ? 1.f : -1.f;
            const float tx = sign * endpoint.tangent.x / tangentLength;
            const float tz = sign * endpoint.tangent.z / tangentLength;
            require( tx * direction.x + tz * direction.z >= .999999f &&
                     std::abs( tx * direction.z - tz * direction.x ) <= .0001f,
                     "Roundabout approach road tangent does not align with its port." );
            const Vector3F side{ -direction.z, 0, direction.x };
            const auto a = plane( center + side * ( port.width * .5f ) );
            const auto b = plane( center - side * ( port.width * .5f ) );
            const auto left = fitted( endpoint.left ), right = fitted( endpoint.right );
            require( ( ( left - a ).length() <= .001f && ( right - b ).length() <= .001f ) ||
                     ( ( left - b ).length() <= .001f && ( right - a ).length() <= .001f ),
                     "Roundabout approach edges do not meet the port grade plane within 0.001 m." );
        }

        class Builder
        {
        public:
            Builder( const render::TerrainData &terrain, const Transform3<real_Num> &placement,
                     CityRoadTerrainMesh &mesh ) : terrain( terrain ), placement( placement ), mesh( mesh )
            {
                const auto &q = placement.getOrientation();
                rotation = Quaternion<f64>( q.w, q.x, q.y, q.z ); inverseRotation = rotation.inverse();
                const auto &p = placement.getPosition(), &s = placement.getScale();
                origin = { p.x, p.y, p.z }; scale = { s.x, s.y, s.z };
            }

            Point local( Vector3F p, Vector2F uv = {} ) const
            {
                require( std::isfinite( p.x ) && std::isfinite( p.y ) && std::isfinite( p.z ),
                         "Road mesh contains a nonfinite source vertex." );
                require( std::isfinite( uv.x ) && std::isfinite( uv.y ),
                         "Road mesh contains nonfinite texture coordinates." );
                const auto q = inverseRotation * ( Vector3<f64>( p.x, p.y, p.z ) - origin ) / scale;
                return { q.x, q.z, p.y, uv.x, uv.y };
            }
            Point terrainPoint( int x, int z ) const
            {
                return { double( terrain.origin.x ) + double( x ) * terrain.spacing.x,
                         double( terrain.origin.y ) + double( z ) * terrain.spacing.y,
                         double( terrain.heights[size_t( z ) * terrain.dimensions.x + x] ) * terrain.heightScale };
            }
            Vector3F world( Point point ) const
            {
                // Match TerrainData and Matrix4::makeTransform. Transform3's
                // legacy transformPoint scales translation and cannot be used.
                const auto pointWorld = origin + rotation *
                    ( Vector3<f64>( point.x, point.y, point.z ) * scale );
                return { float( pointWorld.x ), float( pointWorld.y ), float( pointWorld.z ) };
            }

            void emit( std::array<CityRoadTerrainVertex, 3> triangle, CityRoadTerrainTriangleKind kind,
                       bool up, Vector3F reference = {} )
            {
                auto normal = ( triangle[1].position - triangle[0].position ).crossProduct(
                    triangle[2].position - triangle[0].position );
                const float length = normal.length();
                if( length <= 1e-8f ) return;
                require( std::isfinite( length ), "Road triangle normal is nonfinite." );
                if( ( up && normal.y < 0 ) || ( !up && normal.dotProduct( reference ) < 0 ) )
                { std::swap( triangle[1], triangle[2] ); normal = -normal; }
                normal /= length;
                require( mesh.vertices.size() + 3 <= maximumVertices, "Road terrain mesh vertex budget exceeded." );
                const u32 base = u32( mesh.vertices.size() );
                for( auto &vertex : triangle ) { vertex.normal = normal; mesh.vertices.push_back( vertex ); }
                mesh.indices.push_back( base ); mesh.indices.push_back( base + 1 ); mesh.indices.push_back( base + 2 );
                mesh.triangleKinds.push_back( kind );
            }

            void triangle( const std::array<Vector3F, 3> &positions,
                           const std::array<Vector2F, 3> &uvs, const Policy &policy,
                           Vector3F reference = {} )
            {
                Polygon original{}, scratch{}, clipped{};
                for( size_t i = 0; i < 3; ++i ) original[i] = local( positions[i], uvs[i] );
                const double originalArea = std::abs( area( original[0], original[1], original[2] ) ) * .5;
                if( originalArea <= 1e-10 )
                {
                    require( policy.roundabout && policy.kind != CityRoadTerrainTriangleKind::Road,
                             "Road carriageway has a degenerate or reversed footprint." );
                    validateVertical( original, policy );
                    std::array<CityRoadTerrainVertex, 3> output{};
                    for( size_t i = 0; i < 3; ++i ) { output[i].position = positions[i]; output[i].uv = uvs[i]; }
                    emit( output, policy.kind, false, reference ); return;
                }
                if( area( original[0], original[1], original[2] ) < 0 ) std::swap( original[1], original[2] );
                int x0, z0, x1, z1;
                bounds( original, 3, x0, z0, x1, z1 );
                double coveredArea = 0;
                for( int z = z0; z <= z1; ++z ) for( int x = x0; x <= x1; ++x )
                {
                    visit();
                    const auto a = terrainPoint( x, z ), b = terrainPoint( x + 1, z );
                    const auto c = terrainPoint( x, z + 1 ), d = terrainPoint( x + 1, z + 1 );
                    for( const auto cell : { std::array<Point, 3>{ a, b, c }, std::array<Point, 3>{ b, d, c } } )
                    {
                        size_t count = clip( original, 3, cell[0], cell[1], scratch );
                        count = clip( scratch, count, cell[1], cell[2], clipped );
                        count = clip( clipped, count, cell[2], cell[0], scratch );
                        const double covered = polygonArea( scratch, count );
                        if( covered <= 1e-12 ) continue;
                        require( !render::terrainCellIsHole( terrain, x, z ),
                                 "A road or junction footprint intersects a terrain hole." );
                        coveredArea += covered;
                        std::array<CityRoadTerrainVertex, 10> fitted{};
                        for( size_t i = 0; i < count; ++i )
                        {
                            const auto &point = scratch[i];
                            const double denominator = area( cell[0], cell[1], cell[2] );
                            const double w0 = area( cell[1], cell[2], point ) / denominator;
                            const double w1 = area( cell[2], cell[0], point ) / denominator;
                            const double height = w0 * cell[0].y + w1 * cell[1].y + ( 1 - w0 - w1 ) * cell[2].y;
                            auto position = world( { point.x, point.z, height } );
                            supported( point, position, policy );
                            position.y = policy.drape ? position.y + policy.offset : float( point.y );
                            fitted[i].position = position; fitted[i].uv = { float( point.u ), float( point.v ) };
                        }
                        if( policy.kind == CityRoadTerrainTriangleKind::Road )
                        {
                            const auto wa = world( cell[0] ), wb = world( cell[1] ), wc = world( cell[2] );
                            const auto normal = policy.drape ? ( wb - wa ).crossProduct( wc - wa ) :
                                ( positions[1] - positions[0] ).crossProduct( positions[2] - positions[0] );
                            const double horizontal = std::hypot( double( normal.x ), double( normal.z ) );
                            require( std::abs( normal.y ) > 1e-10 && horizontal <=
                                         std::abs( double( normal.y ) ) * ( policy.maxGrade + 1e-5 ),
                                     "Covered terrain or junction carriageway exceeds the road grade limit." );
                        }
                        for( size_t i = 1; i + 1 < count; ++i )
                            emit( { fitted[0], fitted[i], fitted[i + 1] }, policy.kind, true );
                    }
                }
                require( std::abs( coveredArea - originalArea ) <= std::max( 1e-5, originalArea * 2e-5 ),
                         "Road footprint is not completely supported by terrain cells." );
            }

        private:
            void visit()
            { require( ++visits <= maximumCellVisits, "Road terrain clipping work budget exceeded." ); }
            void bounds( const Polygon &points, size_t size, int &x0, int &z0, int &x1, int &z1 ) const
            {
                double minX = points[0].x, maxX = minX, minZ = points[0].z, maxZ = minZ;
                const double right = terrain.origin.x + double( terrain.dimensions.x - 1 ) * terrain.spacing.x;
                const double farBound = terrain.origin.y + double( terrain.dimensions.y - 1 ) * terrain.spacing.y;
                for( size_t i = 0; i < size; ++i )
                {
                    minX = std::min( minX, points[i].x ); maxX = std::max( maxX, points[i].x );
                    minZ = std::min( minZ, points[i].z ); maxZ = std::max( maxZ, points[i].z );
                }
                require( minX >= terrain.origin.x - 1e-4 && maxX <= right + 1e-4 &&
                         minZ >= terrain.origin.y - 1e-4 && maxZ <= farBound + 1e-4,
                         "Road or junction footprint extends outside its bound terrain." );
                x0 = std::clamp( int( std::floor( ( minX - terrain.origin.x ) / terrain.spacing.x ) ), 0, terrain.dimensions.x - 2 );
                x1 = std::clamp( int( std::floor( ( maxX - terrain.origin.x ) / terrain.spacing.x ) ), 0, terrain.dimensions.x - 2 );
                z0 = std::clamp( int( std::floor( ( minZ - terrain.origin.y ) / terrain.spacing.y ) ), 0, terrain.dimensions.y - 2 );
                z1 = std::clamp( int( std::floor( ( maxZ - terrain.origin.y ) / terrain.spacing.y ) ), 0, terrain.dimensions.y - 2 );
                require( u64( x1 - x0 + 1 ) * u64( z1 - z0 + 1 ) <= maximumCellVisits - visits,
                         "Road terrain clipping work budget exceeded." );
            }
            void supported( Point point, Vector3F ground, const Policy &policy ) const
            {
                if( policy.drape ) return;
                const double base = policy.roundabout ? policy.center.y +
                    policy.gradeX * ( ground.x - policy.center.x ) +
                    policy.gradeZ * ( ground.z - policy.center.z ) : point.y;
                require( std::abs( base - ground.y ) <= policy.tolerance + 1e-5,
                         "Fixed road or junction has an unsupported floating/cut terrain profile." );
            }
            void validateVertical( const Polygon &triangle, const Policy &policy )
            {
                int x0, z0, x1, z1;
                bounds( triangle, 3, x0, z0, x1, z1 );
                Point a = triangle[0], b = triangle[1];
                double longest = -1;
                for( size_t i = 0; i < 3; ++i ) for( size_t j = i + 1; j < 3; ++j )
                {
                    const double length = std::hypot( triangle[i].x - triangle[j].x, triangle[i].z - triangle[j].z );
                    if( length > longest ) { longest = length; a = triangle[i]; b = triangle[j]; }
                }
                require( longest > 1e-7, "Junction wall has a degenerate footprint." );
                for( int z = z0; z <= z1; ++z ) for( int x = x0; x <= x1; ++x )
                {
                    visit();
                    const auto origin = terrainPoint( x, z );
                    double first = 0, last = 1;
                    const auto slab = [&]( double start, double delta, double lo, double hi ) {
                        if( std::abs( delta ) < 1e-12 ) return start >= lo && start <= hi;
                        double enter = ( lo - start ) / delta, exit = ( hi - start ) / delta;
                        if( enter > exit ) std::swap( enter, exit );
                        first = std::max( first, enter ); last = std::min( last, exit ); return first <= last;
                    };
                    if( !slab( a.x, b.x - a.x, origin.x, origin.x + terrain.spacing.x ) ||
                        !slab( a.z, b.z - a.z, origin.z, origin.z + terrain.spacing.y ) || last - first < 1e-9 ) continue;
                    require( !render::terrainCellIsHole( terrain, x, z ), "A junction kerb crosses a terrain hole." );
                    // The line crosses at most one diagonal inside this cell. Check
                    // that intersection as well as both clipped interval endpoints.
                    std::array<double, 3> stations{ first, last, first };
                    const double denominator = ( b.x - a.x ) / terrain.spacing.x + ( b.z - a.z ) / terrain.spacing.y;
                    if( std::abs( denominator ) > 1e-12 )
                        stations[2] = std::clamp( ( 1 - ( a.x - origin.x ) / terrain.spacing.x -
                            ( a.z - origin.z ) / terrain.spacing.y ) / denominator, first, last );
                    for( double t : stations )
                    {
                        const auto point = lerp( a, b, t );
                        const auto p = world( { point.x, point.z, 0 } );
                        float height = 0;
                        require( render::sampleTerrainHeight( terrain, placement, p, height ),
                                 "A junction wall has no supported terrain." );
                        supported( point, { p.x, height, p.z }, policy );
                    }
                }
            }
            const render::TerrainData &terrain;
            const Transform3<real_Num> &placement;
            CityRoadTerrainMesh &mesh;
            Quaternion<f64> rotation, inverseRotation;
            Vector3<f64> origin, scale;
            u64 visits = 0;
        };
    }

    void CityRoadTerrainMesh::swap( CityRoadTerrainMesh &other ) noexcept
    {
        vertices.swap( other.vertices ); indices.swap( other.indices ); triangleKinds.swap( other.triangleKinds );
        std::swap( sourceRevision, other.sourceRevision ); std::swap( terrainRevision, other.terrainRevision );
    }

    bool buildCityRoadTerrainMesh( const procedural::CityRoadSplineSource &source,
        const procedural::CityRoadSplineBake &bake, render::TerrainSnapshot terrain,
        const Transform3<real_Num> &placement, CityRoadTerrainMesh &output, String &error )
    {
        try
        {
            if( !procedural::validateCityRoadSplineSource( source, error ) )
                throw std::runtime_error( error.c_str() );
            require( bake.sourceRevision == source.revision && bake.roads.size() == source.roads.size(),
                     "Road bake is stale or does not match its source." );
            CityRoadTerrainMesh candidate;
            candidate.sourceRevision = source.revision;
            candidate.terrainRevision = terrain ? terrain->revision : 0;
            if( source.roads.empty() && source.roundabouts.empty() )
            { output.swap( candidate ); error.clear(); return true; }
            require( bool( terrain ), "Authored roads require a retained terrain snapshot." );
            if( !render::validateTerrainPlacement( *terrain, placement, error ) )
                throw std::runtime_error( error.c_str() );
            Builder builder( *terrain, placement, candidate );
            for( size_t roadIndex = 0; roadIndex < source.roads.size(); ++roadIndex )
            {
                const auto &road = source.roads[roadIndex];
                const auto &samples = bake.roads[roadIndex];
                require( samples.roadId == road.id && samples.points.size() >= 2,
                         "Road bake has missing/mismatched samples." );
                for( size_t i = 0; i < samples.points.size(); ++i )
                    require( std::isfinite( samples.points[i].distance ) && samples.points[i].distance >= 0 &&
                             ( !i || samples.points[i].distance > samples.points[i - 1].distance ),
                             "Road bake arc distances must be finite and strictly increasing." );
                Policy policy;
                policy.drape = road.heightMode == procedural::CityRoadHeightMode::Drape;
                require( !policy.drape || std::abs( road.heightOffset ) <= .030001f,
                         "Draped road offset exceeds the supported 0.03 m surface bias; bridges are unavailable." );
                policy.offset = road.heightOffset; policy.maxGrade = road.maxGrade;
                for( size_t i = 1; i < samples.points.size(); ++i )
                {
                    const auto &a = samples.points[i - 1], &b = samples.points[i];
                    const float av = a.distance * .125f, bv = b.distance * .125f;
                    builder.triangle( { a.left, a.right, b.left }, { Vector2F( 0, av ), Vector2F( 1, av ), Vector2F( 0, bv ) }, policy );
                    builder.triangle( { a.right, b.right, b.left }, { Vector2F( 1, av ), Vector2F( 1, bv ), Vector2F( 0, bv ) }, policy );
                }
            }
            const auto sampler = [terrain, placement]( const Vector3F &point, float &height ) {
                return render::sampleTerrainHeight( *terrain, placement, point, height );
            };
            for( const auto &junction : source.roundabouts )
            {
                procedural::CityRoadRoundaboutBake junctionBake;
                if( !procedural::bakeCityRoadRoundabout( junction, sampler, junctionBake, error ) )
                    throw std::runtime_error( error.c_str() );
                for( const auto &port : junction.ports ) validatePort( junction, port, source, bake, *terrain, placement );
                require( junctionBake.indices.size() % 3 == 0 &&
                         junctionBake.triangleKinds.size() == junctionBake.indices.size() / 3,
                         "Roundabout bake has inconsistent surface kinds." );
                Policy policy;
                policy.roundabout = true; policy.maxGrade = junction.maxGrade;
                policy.tolerance = std::min( junction.maxTerrainDeviation, .08f );
                policy.center = junction.center; policy.gradeX = junction.gradeX; policy.gradeZ = junction.gradeZ;
                for( size_t i = 0; i < junctionBake.indices.size(); i += 3 )
                {
                    const auto kind = junctionBake.triangleKinds[i / 3];
                    policy.kind = kind == procedural::CityRoadRoundaboutSurface::Island ? CityRoadTerrainTriangleKind::Island :
                        kind == procedural::CityRoadRoundaboutSurface::Kerb ? CityRoadTerrainTriangleKind::Kerb : CityRoadTerrainTriangleKind::Road;
                    std::array<Vector3F, 3> positions;
                    std::array<Vector2F, 3> uvs;
                    Vector3F normal;
                    for( size_t v = 0; v < 3; ++v )
                    {
                        const u32 index = junctionBake.indices[i + v];
                        require( index < junctionBake.vertices.size(), "Roundabout triangle has an invalid index." );
                        const auto &vertex = junctionBake.vertices[index];
                        positions[v] = vertex.position; uvs[v] = { vertex.u, vertex.v }; normal = vertex.normal;
                    }
                    builder.triangle( positions, uvs, policy, normal );
                }
            }
            output.swap( candidate ); error.clear(); return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }
}
