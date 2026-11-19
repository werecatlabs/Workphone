#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Procedural/CityRoadTerrainFit.hpp>
#include <Workphone/Procedural/CityRoadJunctions.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace workphone::scene
{
    struct CityRoadTerrainGradeData
    {
        struct Sample
        {
            u32 index;
            f32 original, applied;

            bool operator==( const Sample &other ) const
            {
                return index == other.index && original == other.original && applied == other.applied;
            }
        };

        SourceBudgetText terrainUuid;
        Vector2I dimensions;
        Vector2F origin, spacing;
        f32 heightScale = 1;
        std::array<double, 10> placement{};
        SourceBudgetVector<Sample> samples;
    };

    namespace
    {
        using namespace procedural;

        void require( bool value, const char *message )
        {
            if( !value )
            {
                throw std::runtime_error( message );
            }
        }

        void checked( bool value, const String &error )
        {
            if( !value )
            {
                throw std::runtime_error( error.c_str() );
            }
        }

        float distance( Vector3F a, Vector3F b )
        {
            return std::hypot( a.x - b.x, a.z - b.z );
        }

        float feather( float distance, float core, float shoulder )
        {
            const float t = std::clamp( ( distance - core ) / shoulder, 0.f, 1.f );
            return 1.f - t * t * ( 3.f - 2.f * t );
        }

        struct Grid
        {
            const render::TerrainData &data;
            const Transform3<real_Num> &placement;
            int width, depth;
            float dx, dz, apron;

            Grid( const render::TerrainData &d, const Transform3<real_Num> &p ) :
                data( d ),
                placement( p ),
                width( d.dimensions.x ),
                depth( d.dimensions.y ),
                dx( d.spacing.x * p.getScale().x ),
                dz( d.spacing.y * p.getScale().z ),
                apron( std::hypot( dx, dz ) + 3.f )
            {
            }

            Vector3F world( int x, int z ) const
            {
                const Vector3<real_Num> local( data.origin.x + x * data.spacing.x, 0,
                                               data.origin.y + z * data.spacing.y );
                const auto result = placement.getPosition() +
                                    placement.getOrientation() * ( local * placement.getScale() );
                return { float( result.x ), float( result.y ), float( result.z ) };
            }

            Vector2F coordinate( Vector3F point ) const
            {
                const auto local =
                    ( placement.getOrientation().inverse() *
                      ( Vector3<real_Num>( point.x, point.y, point.z ) - placement.getPosition() ) ) /
                    placement.getScale();
                return { float( ( local.x - data.origin.x ) / data.spacing.x ),
                         float( ( local.z - data.origin.y ) / data.spacing.y ) };
            }

            float original( size_t i ) const
            {
                return float( placement.getPosition().y +
                              data.heights[i] * data.heightScale * placement.getScale().y );
            }

            float sample( const SourceBudgetVector<float> &heights, Vector3F point ) const
            {
                auto c = coordinate( point );
                require( c.x >= -.001f && c.y >= -.001f && c.x <= width - 1 + .001f &&
                             c.y <= depth - 1 + .001f,
                         "Road fit control/profile lies outside the retained terrain." );
                c.x = std::clamp( c.x, 0.f, float( width - 1 ) );
                c.y = std::clamp( c.y, 0.f, float( depth - 1 ) );
                const int x = std::min( int( c.x ), width - 2 ), z = std::min( int( c.y ), depth - 2 );
                const float tx = c.x - x, tz = c.y - z;
                const auto i = size_t( z ) * width + x;
                return ( 1 - tz ) * ( heights[i] * ( 1 - tx ) + heights[i + 1] * tx ) +
                       tz * ( heights[i + width] * ( 1 - tx ) + heights[i + width + 1] * tx );
            }

            template <class Fn>
            void visit( Vector3F a, Vector3F b, float radius, Fn fn ) const
            {
                const auto ca = coordinate( a ), cb = coordinate( b );
                const double lowX =
                                 std::floor( double( std::min( ca.x, cb.x ) ) - double( radius ) / dx ),
                             lowZ =
                                 std::floor( double( std::min( ca.y, cb.y ) ) - double( radius ) / dz ),
                             highX =
                                 std::ceil( double( std::max( ca.x, cb.x ) ) + double( radius ) / dx ),
                             highZ =
                                 std::ceil( double( std::max( ca.y, cb.y ) ) + double( radius ) / dz );
                if( lowX > width - 1 || lowZ > depth - 1 || highX < 0 || highZ < 0 )
                {
                    return;
                }
                const int x0 = int( std::max( 0., lowX ) ), z0 = int( std::max( 0., lowZ ) ),
                          x1 = int( std::min( double( width - 1 ), highX ) ),
                          z1 = int( std::min( double( depth - 1 ), highZ ) );
                for( int z = z0; z <= z1; ++z )
                {
                    for( int x = x0; x <= x1; ++x )
                    {
                        fn( size_t( z ) * width + x, world( x, z ) );
                    }
                }
            }
        };

        // Sliding windows keep smoothing bounded independently of the selected radius.
        void smooth( SourceBudgetVector<float> &values, int width, int depth, int rx, int rz )
        {
            SourceBudgetVector<float> buffer( values.size() );
            for( int pass = 0; pass < 2; ++pass )
            {
                for( int z = 0; z < depth; ++z )
                {
                    double sum = 0;
                    int lo = 0, hi = -1;
                    for( int x = 0; x < width; ++x )
                    {
                        const int nextLo = std::max( 0, x - rx ), nextHi = std::min( width - 1, x + rx );
                        while( hi < nextHi )
                        {
                            sum += values[size_t( z ) * width + ++hi];
                        }
                        while( lo < nextLo )
                        {
                            sum -= values[size_t( z ) * width + lo++];
                        }
                        buffer[size_t( z ) * width + x] = float( sum / ( hi - lo + 1 ) );
                    }
                }
                for( int x = 0; x < width; ++x )
                {
                    double sum = 0;
                    int lo = 0, hi = -1;
                    for( int z = 0; z < depth; ++z )
                    {
                        const int nextLo = std::max( 0, z - rz ), nextHi = std::min( depth - 1, z + rz );
                        while( hi < nextHi )
                        {
                            sum += buffer[size_t( ++hi ) * width + x];
                        }
                        while( lo < nextLo )
                        {
                            sum -= buffer[size_t( lo++ ) * width + x];
                        }
                        values[size_t( z ) * width + x] = float( sum / ( hi - lo + 1 ) );
                    }
                }
            }
        }

        std::shared_ptr<render::TerrainData> copyTerrain( const render::TerrainData &from )
        {
            auto to = std::allocate_shared<render::TerrainData>(
                SourceBudgetAllocator<render::TerrainData>{} );
            to->dimensions = from.dimensions;
            to->spacing = from.spacing;
            to->origin = from.origin;
            to->heightScale = from.heightScale;
            to->revision = from.revision;
            to->layers = from.layers;
            to->appearance = from.appearance;
            to->autoMaterial = from.autoMaterial;
            to->heights = Array<float>( from.heights.begin(), from.heights.end(),
                                        Allocator<float>( sourceBudget ) );
            to->layerWeights = Array<Vector4F>( from.layerWeights.begin(), from.layerWeights.end(),
                                                Allocator<Vector4F>( sourceBudget ) );
            to->surfaceMasks = Array<Vector2F>( from.surfaceMasks.begin(), from.surfaceMasks.end(),
                                                Allocator<Vector2F>( sourceBudget ) );
            to->holes = Array<u8>( from.holes.begin(), from.holes.end(), Allocator<u8>( sourceBudget ) );
            return to;
        }

        constexpr size_t maximumGradingSamples = 262144;
        constexpr std::string_view gradingPrefix = "wp-road-grade-v1:";
        constexpr size_t gradingHeaderBytes = gradingPrefix.size() + 36 + 7 * 8 + 10 * 16;
        constexpr size_t gradingRecordBytes = 3 * 8;

        bool gradingUuid( std::string_view uuid )
        {
            if( uuid.size() != 36 )
            {
                return false;
            }
            for( size_t i = 0; i < uuid.size(); ++i )
            {
                const auto c = uuid[i];
                if( i == 8 || i == 13 || i == 18 || i == 23 )
                {
                    if( c != '-' )
                    {
                        return false;
                    }
                }
                else if( !( ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'f' ) ) )
                {
                    return false;
                }
            }
            return true;
        }

        std::array<double, 10> placementKey( const Transform3<real_Num> &placement )
        {
            const auto &p = placement.getPosition(), &s = placement.getScale();
            const auto &q = placement.getOrientation();
            return { p.x, p.y, p.z, s.x, s.y, s.z, q.W(), q.X(), q.Y(), q.Z() };
        }

        template <class A, class B>
        bool sameGradingGrid( const A &a, const B &b )
        {
            return a.dimensions.x == b.dimensions.x && a.dimensions.y == b.dimensions.y &&
                   a.spacing.x == b.spacing.x && a.spacing.y == b.spacing.y &&
                   a.origin.x == b.origin.x && a.origin.y == b.origin.y &&
                   a.heightScale == b.heightScale;
        }

        bool gradingMatches( const CityRoadTerrainGradeData &grading, std::string_view uuid,
                             const render::TerrainData &data, const Transform3<real_Num> &placement )
        {
            return std::string_view( grading.terrainUuid.data(), grading.terrainUuid.size() ) == uuid &&
                   sameGradingGrid( grading, data ) && grading.placement == placementKey( placement );
        }

        template <class T, class U>
        T gradeBits( U value )
        {
            static_assert( sizeof( T ) == sizeof( U ) );
            T result;
            std::memcpy( &result, &value, sizeof( result ) );
            return result;
        }

        template <class T>
        void writeGradeHex( SourceBudgetText &out, T value )
        {
            constexpr char digits[] = "0123456789abcdef";
            for( size_t i = sizeof( T ) * 2; i != 0; --i )
            {
                out.push_back( digits[( value >> ( ( i - 1 ) * 4 ) ) & 0xf] );
            }
        }

        template <class T>
        T readGradeHex( std::string_view &text )
        {
            constexpr size_t count = sizeof( T ) * 2;
            T value = 0;
            for( size_t i = 0; i < count; ++i )
            {
                const auto c = text[i];
                const auto nibble = c >= '0' && c <= '9'   ? c - '0'
                                    : c >= 'a' && c <= 'f' ? c - 'a' + 10
                                                           : -1;
                require( nibble >= 0, "Invalid terrain grading data digit." );
                value = ( value << 4 ) | T( nibble );
            }
            text.remove_prefix( count );
            return value;
        }

        bool samePoint( Vector3F a, Vector3F b, bool horizontal = false )
        {
            return a.x == b.x && a.z == b.z && ( horizontal || a.y == b.y );
        }

        bool splitPreservesRoadCurve( const CityRoadSpline &old, const CityRoadSpline &road )
        {
            if( old.closed || road.closed || road.knots.size() != old.knots.size() + 1 )
            {
                return false;
            }
            size_t at = 0;
            while( at < old.knots.size() && old.knots[at].id == road.knots[at].id )
            {
                ++at;
            }
            if( at == 0 || at == old.knots.size() )
            {
                return false;
            }
            for( size_t i = at; i < old.knots.size(); ++i )
            {
                if( old.knots[i].id != road.knots[i + 1].id )
                {
                    return false;
                }
            }
            const bool drape = road.heightMode == CityRoadHeightMode::Drape;
            for( size_t i = 0; i < old.knots.size(); ++i )
            {
                const auto &a = old.knots[i], &b = road.knots[i + ( i >= at )];
                if( !samePoint( a.position, b.position, drape ) ||
                    ( i != at && !samePoint( a.inHandle, b.inHandle, drape ) ) ||
                    ( i + 1 != at && !samePoint( a.outHandle, b.outHandle, drape ) ) )
                {
                    return false;
                }
            }
            const auto &first = old.knots[at - 1], &last = old.knots[at];
            const auto &splitFirst = road.knots[at - 1], &middle = road.knots[at],
                       &splitLast = road.knots[at + 1];
            const auto p0 = first.position, p1 = p0 + first.outHandle;
            const auto p3 = last.position, p2 = p3 + last.inHandle;
            const auto dot = [&]( Vector3F a, Vector3F b ) {
                return a.x * b.x + a.z * b.z + ( drape ? 0.f : a.y * b.y );
            };
            const auto lerp = []( Vector3F a, Vector3F b, float t ) { return a + ( b - a ) * t; };
            const auto closePosition = [&]( Vector3F a, Vector3F b ) {
                return std::abs( a.x - b.x ) <= .005f && std::abs( a.z - b.z ) <= .005f &&
                       ( drape || std::abs( a.y - b.y ) <= .005f );
            };
            float t = 0;
            if( dot( first.outHandle, first.outHandle ) > 1e-8f )
            {
                t = dot( splitFirst.outHandle, first.outHandle ) /
                    dot( first.outHandle, first.outHandle );
            }
            else if( dot( last.inHandle, last.inHandle ) > 1e-8f )
            {
                t = 1.f - dot( splitLast.inHandle, last.inHandle ) / dot( last.inHandle, last.inHandle );
            }
            else
            {
                const auto chord = p3 - p0;
                if( dot( chord, chord ) < 1e-8f )
                {
                    return false;
                }
                const float fraction = dot( middle.position - p0, chord ) / dot( chord, chord );
                float low = 0.f, high = 1.f;
                for( int i = 0; i < 24; ++i )
                {
                    const float mid = ( low + high ) * .5f;
                    if( mid * mid * ( 3.f - 2.f * mid ) < fraction )
                    {
                        low = mid;
                    }
                    else
                    {
                        high = mid;
                    }
                }
                t = ( low + high ) * .5f;
            }
            if( !std::isfinite( t ) || t <= 0.f || t >= 1.f )
            {
                return false;
            }
            const auto a = lerp( p0, p1, t ), b = lerp( p1, p2, t ), c = lerp( p2, p3, t );
            const auto d = lerp( a, b, t ), e = lerp( b, c, t ), f = lerp( d, e, t );
            return closePosition( splitFirst.position + splitFirst.outHandle, a ) &&
                   closePosition( middle.position + middle.inHandle, d ) &&
                   closePosition( middle.position, f ) &&
                   closePosition( middle.position + middle.outHandle, e ) &&
                   closePosition( splitLast.position + splitLast.inHandle, c );
        }

        bool changedRoadFootprint( const CityRoadSplineSource &before,
                                   const CityRoadSplineSource &draft )
        {
            for( const auto &road : draft.roads )
            {
                const auto old =
                    std::find_if( before.roads.begin(), before.roads.end(),
                                  [&]( const auto &entry ) { return entry.id == road.id; } );
                if( old == before.roads.end() )
                {
                    return true;
                }
                if( road.width != old->width || road.heightMode != old->heightMode ||
                    ( road.heightMode == CityRoadHeightMode::Fixed &&
                      road.heightOffset != old->heightOffset ) ||
                    road.closed != old->closed )
                {
                    return true;
                }
                const bool drape = road.heightMode == CityRoadHeightMode::Drape;
                if( road.knots.size() != old->knots.size() )
                {
                    // Only an exact Bezier subdivision retains the prior crown.
                    if( !splitPreservesRoadCurve( *old, road ) )
                    {
                        return true;
                    }
                    continue;
                }
                for( size_t i = 0; i < road.knots.size(); ++i )
                {
                    const auto &a = road.knots[i], &b = old->knots[i];
                    if( !samePoint( a.position, b.position, drape ) ||
                        !samePoint( a.inHandle, b.inHandle, drape ) ||
                        !samePoint( a.outHandle, b.outHandle, drape ) )
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        struct Bounds
        {
            float minX, maxX, minZ, maxZ;

            bool meets( Vector3F a, Vector3F b, float margin ) const
            {
                return std::max( a.x, b.x ) + margin >= minX && std::min( a.x, b.x ) - margin <= maxX &&
                       std::max( a.z, b.z ) + margin >= minZ && std::min( a.z, b.z ) - margin <= maxZ;
            }
        };

        struct EditedRoad
        {
            bool all = false;
            SourceBudgetVector<Bounds> spans;

            bool meets( Vector3F a, Vector3F b, float margin ) const
            {
                return all || std::any_of( spans.begin(), spans.end(), [&]( const auto &span ) {
                           return span.meets( a, b, margin );
                       } );
            }
        };

        SourceBudgetVector<EditedRoad> editedRoads( const CityRoadSplineSource &before,
                                                    const CityRoadSplineSource &draft )
        {
            SourceBudgetVector<EditedRoad> edits( draft.roads.size() );
            for( size_t r = 0; r < draft.roads.size(); ++r )
            {
                const auto &road = draft.roads[r];
                auto &edit = edits[r];
                const auto old =
                    std::find_if( before.roads.begin(), before.roads.end(),
                                  [&]( const auto &other ) { return other.id == road.id; } );
                if( old == before.roads.end() || road.width != old->width ||
                    road.maxGrade != old->maxGrade || road.closed != old->closed ||
                    road.heightOffset != old->heightOffset || road.heightMode != old->heightMode )
                {
                    edit.all = true;
                    continue;
                }

                const bool drape = road.heightMode == CityRoadHeightMode::Drape;
                SourceBudgetMap<SourceBudgetText, size_t> oldIndices;
                for( size_t i = 0; i < old->knots.size(); ++i )
                {
                    oldIndices.emplace( old->knots[i].id, i );
                }
                for( size_t i = 0; i < road.knots.size() - ( road.closed ? 0 : 1 ); ++i )
                {
                    const auto &a = road.knots[i], &b = road.knots[( i + 1 ) % road.knots.size()];
                    const auto found = oldIndices.find( a.id );
                    if( found != oldIndices.end() )
                    {
                        const auto &oldA = old->knots[found->second];
                        const auto index = found->second + 1;
                        if( index < old->knots.size() || old->closed )
                        {
                            const auto &oldB = old->knots[index % old->knots.size()];
                            if( b.id == oldB.id && samePoint( a.position, oldA.position, drape ) &&
                                samePoint( a.outHandle, oldA.outHandle, drape ) &&
                                samePoint( b.inHandle, oldB.inHandle, drape ) &&
                                samePoint( b.position, oldB.position, drape ) )
                            {
                                continue;
                            }
                        }
                    }
                    // The whole cubic lies inside its control hull. Limit grading
                    // to sampled pieces crossing an edited span's footprint.
                    const auto p = a.position + a.outHandle, q = b.position + b.inHandle;
                    edit.spans.push_back( { std::min( { a.position.x, p.x, q.x, b.position.x } ),
                                            std::max( { a.position.x, p.x, q.x, b.position.x } ),
                                            std::min( { a.position.z, p.z, q.z, b.position.z } ),
                                            std::max( { a.position.z, p.z, q.z, b.position.z } ) } );
                }
            }
            return edits;
        }

        bool editedRoundabout( const CityRoadSplineSource &before, const CityRoadRoundaboutSpec &draft )
        {
            const auto old = std::find_if(
                before.roundabouts.begin(), before.roundabouts.end(),
                [&]( const auto &other ) { return other.junctionId == draft.junctionId; } );
            if( old == before.roundabouts.end() || !samePoint( old->center, draft.center ) ||
                old->outerRadius != draft.outerRadius || old->islandRadius != draft.islandRadius ||
                old->gradeX != draft.gradeX || old->gradeZ != draft.gradeZ ||
                old->maxGrade != draft.maxGrade || old->ports.size() != draft.ports.size() )
            {
                return true;
            }
            for( const auto &port : draft.ports )
            {
                const auto other = std::find_if(
                    old->ports.begin(), old->ports.end(),
                    [&]( const auto &entry ) { return entry.approachId == port.approachId; } );
                if( other == old->ports.end() || !samePoint( other->outward, port.outward ) ||
                    other->width != port.width || other->length != port.length )
                {
                    return true;
                }
            }
            return false;
        }

        struct Pad
        {
            Vector3F center{};
            float radius = 0, height = 0, gx = 0, gz = 0, maxGrade = .08f;
            int roundabout = -1;
            float offset = 0;
            size_t group = 0;
            bool active = true;

            float at( Vector3F point ) const
            {
                return height + gx * ( point.x - center.x ) + gz * ( point.z - center.z );
            }
        };

        SourceBudgetVector<Pad> preparePads( const CityRoadSplineSource &source,
                                             const CityRoadSplineSource &flatSource,
                                             const CityRoadSplineBake &flatBake, const Grid &grid,
                                             bool connectRoads, const CityRoadSplineSource *before,
                                             const SourceBudgetVector<EditedRoad> &edits )
        {
            SourceBudgetVector<Pad> pads;
            String error;
            if( connectRoads )
            {
                CityRoadJunctionBake junctions;
                checked( buildCityRoadJunctions(
                             flatSource, flatBake,
                             []( const Vector3F &, float &y ) {
                                 y = 0;
                                 return true;
                             },
                             {}, junctions, error ),
                         error );
                for( const auto &junction : junctions.junctions )
                {
                    bool fixed = false;
                    for( const auto &port : junction.ports )
                    {
                        fixed = fixed ||
                                source.roads[port.roadIndex].heightMode == CityRoadHeightMode::Fixed;
                    }
                    if( fixed )
                    {
                        continue;
                    }
                    Pad pad;
                    pad.center = junction.center;
                    pad.maxGrade = junction.maxGrade;
                    for( auto point : junction.boundary )
                    {
                        pad.radius = std::max( pad.radius, distance( pad.center, point ) );
                    }
                    if( before )
                    {
                        pad.active = std::any_of( junction.ports.begin(), junction.ports.end(),
                                                  [&]( const auto &port ) {
                                                      return edits[port.roadIndex].meets(
                                                          pad.center, pad.center, pad.radius + .1f );
                                                  } );
                    }
                    pad.radius += grid.apron;
                    pads.push_back( pad );
                }
            }
            for( size_t j = 0; j < source.roundabouts.size(); ++j )
            {
                const auto &junction = source.roundabouts[j];
                Pad pad;
                pad.center = junction.center;
                pad.roundabout = int( j );
                pad.maxGrade = junction.maxGrade;
                pad.active = !before || editedRoundabout( *before, junction );
                pad.radius = junction.outerRadius;
                bool first = true;
                for( const auto &port : junction.ports )
                {
                    pad.radius = std::max(
                        pad.radius, std::hypot( junction.outerRadius + port.length, port.width * .5f ) );
                    const auto road =
                        std::find_if( source.roads.begin(), source.roads.end(),
                                      [&]( const auto &r ) { return r.id == port.approachId; } );
                    require( road != source.roads.end() && road->heightMode == CityRoadHeightMode::Drape,
                             "Terrain fitting currently requires draped roundabout approaches." );
                    pad.maxGrade = std::min( pad.maxGrade, road->maxGrade );
                    if( first )
                    {
                        pad.offset = road->heightOffset;
                        first = false;
                    }
                    require( std::abs( road->heightOffset - pad.offset ) < .00001f,
                             "Roundabout terrain fitting requires matching approach surface offsets." );
                }
                pad.radius += grid.apron;
                pads.push_back( pad );
            }
            require( pads.size() <= 1024, "Road terrain fit junction pad budget exceeded." );
            for( size_t i = 0; i < pads.size(); ++i )
            {
                pads[i].group = i;
            }
            // Connected pad footprints share one plane. Their union stays local: no
            // enclosing district-sized disc is introduced between separate junctions.
            for( size_t i = 0; i < pads.size(); ++i )
            {
                for( size_t j = 0; j < i; ++j )
                {
                    if( !pads[i].active || !pads[j].active )
                    {
                        continue;
                    }
                    // Different pinned planes may not occupy vertices of the same
                    // terrain triangle, even when their continuous discs miss.
                    if( distance( pads[i].center, pads[j].center ) >
                        pads[i].radius + pads[j].radius + std::hypot( grid.dx, grid.dz ) )
                    {
                        continue;
                    }
                    const auto a = pads[i].group, b = pads[j].group;
                    if( a == b )
                    {
                        continue;
                    }
                    for( auto &pad : pads )
                    {
                        if( pad.group == a )
                        {
                            pad.group = b;
                        }
                    }
                }
            }
            return pads;
        }

        CityRoadSplineSource flattenRoadSource( const CityRoadSplineSource &source )
        {
            auto flat = source;
            for( auto &road : flat.roads )
            {
                require( road.heightMode != CityRoadHeightMode::Bridge,
                         "Bridge terrain fitting is unsupported." );
                road.heightMode = CityRoadHeightMode::Drape;
                road.heightOffset = 0;
                for( auto &knot : road.knots )
                {
                    knot.position.y = 0;
                    knot.inHandle.y = 0;
                    knot.outHandle.y = 0;
                }
            }
            for( auto &junction : flat.roundabouts )
            {
                junction.center.y = 0;
                junction.gradeX = 0;
                junction.gradeZ = 0;
            }
            return flat;
        }

        SourceBudgetVector<u8> terrainGradingCoverage( const CityRoadSplineSource &source,
                                                       const Grid &grid, String &error )
        {
            SourceBudgetVector<u8> covered( grid.data.heights.size(), 0 );
            if( source.roads.empty() && source.roundabouts.empty() )
            {
                return covered;
            }

            const auto flat = flattenRoadSource( source );
            const CityRoadTerrainFitOptions options;
            CityRoadSplineBake bake;
            const auto zero = []( const Vector3F &, float &height ) {
                height = 0;
                return true;
            };
            checked( sampleCityRoadSplines( flat, options.sampling, zero, bake, error ), error );
            u64 visits = 0;
            for( size_t i = 0; i < bake.roads.size(); ++i )
            {
                const auto &points = bake.roads[i].points;
                const float radius = source.roads[i].width * .5f + grid.apron + options.shoulderWidth;
                for( size_t j = 1; j < points.size(); ++j )
                {
                    const auto a = points[j - 1].position, b = points[j].position;
                    const float dx = b.x - a.x, dz = b.z - a.z, length2 = dx * dx + dz * dz;
                    if( length2 < 1e-10f )
                    {
                        continue;
                    }
                    grid.visit( a, b, radius, [&]( size_t index, Vector3F point ) {
                        require( ++visits <= 64000000,
                                 "Road terrain restoration work budget exceeded." );
                        const float t = std::clamp(
                            ( ( point.x - a.x ) * dx + ( point.z - a.z ) * dz ) / length2, 0.f, 1.f );
                        if( std::hypot( point.x - a.x - t * dx, point.z - a.z - t * dz ) < radius )
                        {
                            covered[index] = 1;
                        }
                    } );
                }
            }
            // The complete influence envelopes also keep shared transition
            // shoulders smooth until the last road in that area moves away.
            const auto pads = preparePads( source, flat, bake, grid, options.connectRoads, nullptr, {} );
            for( const auto &pad : pads )
            {
                const float radius = pad.radius + options.shoulderWidth;
                grid.visit( pad.center, pad.center, radius, [&]( size_t index, Vector3F point ) {
                    require( ++visits <= 64000000, "Road terrain restoration work budget exceeded." );
                    if( distance( point, pad.center ) < radius )
                    {
                        covered[index] = 1;
                    }
                } );
            }
            return covered;
        }

        void setPadPlanes( SourceBudgetVector<Pad> &pads, const Grid &grid,
                           const SourceBudgetVector<float> &field, u64 &visited )
        {
            for( size_t i = 0; i < pads.size(); ++i )
            {
                if( pads[i].active && pads[i].group == i )
                {
                    // Least-squares plane from every supported grid vertex of this local
                    // connected group. Double precision keeps kilometre placements stable.
                    double n = 0, sx = 0, sz = 0, sy = 0, sxx = 0, szz = 0, sxz = 0, sxy = 0, szy = 0;
                    float allowedGrade = .35f;
                    const auto origin = pads[i].center;
                    for( const auto &pad : pads )
                    {
                        if( pad.group == i )
                        {
                            allowedGrade = std::min( allowedGrade, pad.maxGrade );
                            grid.visit( pad.center, pad.center, pad.radius,
                                        [&]( size_t index, Vector3F point ) {
                                            require( ++visited <= 64000000,
                                                     "Road terrain fit plane work budget exceeded." );
                                            if( distance( point, pad.center ) > pad.radius )
                                            {
                                                return;
                                            }
                                            const double x = point.x - origin.x, z = point.z - origin.z,
                                                         y = field[index];
                                            n += 1;
                                            sx += x;
                                            sz += z;
                                            sy += y;
                                            sxx += x * x;
                                            szz += z * z;
                                            sxz += x * z;
                                            sxy += x * y;
                                            szy += z * y;
                                        } );
                        }
                    }
                    require( n >= 3, "Road junction terrain fit has insufficient terrain support." );
                    const double xx = sxx - sx * sx / n, zz = szz - sz * sz / n, xz = sxz - sx * sz / n,
                                 xy = sxy - sx * sy / n, zy = szy - sz * sy / n,
                                 determinant = xx * zz - xz * xz;
                    double gx = determinant > 1e-8 ? ( xy * zz - zy * xz ) / determinant : 0,
                           gz = determinant > 1e-8 ? ( zy * xx - xy * xz ) / determinant : 0;
                    const double grade = std::hypot( gx, gz ), limit = allowedGrade * .985;
                    if( grade > limit )
                    {
                        gx *= limit / grade;
                        gz *= limit / grade;
                    }
                    const double height = ( sy - gx * sx - gz * sz ) / n;
                    for( auto &pad : pads )
                    {
                        if( pad.group == i )
                        {
                            pad.gx = float( gx );
                            pad.gz = float( gz );
                            pad.height = float( height + gx * ( pad.center.x - origin.x ) +
                                                gz * ( pad.center.z - origin.z ) );
                        }
                    }
                }
            }
        }

        struct Accumulator
        {
            double target = 0, weight = 0;
            float influence = 0;

            void add( float value, float amount, float confidence = 1 )
            {
                target += double( value ) * amount * confidence;
                weight += double( amount ) * confidence;
                influence = std::max( influence, amount );
            }
        };

        // Project local terrain triangles onto the allowed grade cone. Junction
        // planes and authored Fixed road profiles are hard constraints. This
        // removes short approach/shoulder spikes without flattening distant hills.
        struct GradeProjection
        {
            float maximum = 0;
            Vector3F position{};
            unsigned pins = 0, sweeps = 0;
        };

        GradeProjection relaxRoadGrades( SourceBudgetVector<float> &heights, const Grid &grid,
                                         const SourceBudgetVector<u8> &core,
                                         const SourceBudgetVector<u8> &pinned, float maximumGrade,
                                         u64 &visited )
        {
            const double limit = maximumGrade * .985, limit2 = limit * limit;
            const double ix = 1. / grid.dx, iz = 1. / grid.dz;

            struct Cell
            {
                u32 first;
                u8 triangles;
            };

            SourceBudgetVector<Cell> cells;
            auto values = heights.data();
            const auto active = core.data(), pinsAt = pinned.data();
            for( int z = 0; z < grid.depth - 1; ++z )
            {
                for( int x = 0; x < grid.width - 1; ++x )
                {
                    require( ++visited <= 64000000,
                             "Road terrain fit grade mask work budget exceeded." );
                    const u32 a = u32( z * grid.width + x ), b = a + 1, c = a + grid.width, d = c + 1;
                    const u8 triangles = ( active[a] && active[b] && active[c] ? 1 : 0 ) |
                                         ( active[d] && active[c] && active[b] ? 2 : 0 );
                    if( triangles )
                    {
                        cells.push_back( { a, triangles } );
                    }
                }
            }
            GradeProjection result;
            const auto measure = [&] {
                result.maximum = 0;
                result.pins = 0;
                const auto triangle = [&]( size_t a, size_t b, size_t c ) {
                    const float grade = float( std::hypot( ( double( values[b] ) - values[a] ) * ix,
                                                           ( double( values[c] ) - values[a] ) * iz ) );
                    if( grade > result.maximum )
                    {
                        result.maximum = grade;
                        result.position = grid.world( int( a % grid.width ), int( a / grid.width ) );
                        result.pins =
                            unsigned( pinsAt[a] ) + unsigned( pinsAt[b] ) + unsigned( pinsAt[c] );
                    }
                };
                for( const auto &cell : cells )
                {
                    require( ++visited <= 64000000,
                             "Road terrain fit grade verification work budget exceeded." );
                    const size_t a = cell.first, b = a + 1, c = a + grid.width, d = c + 1;
                    if( cell.triangles & 1 )
                    {
                        triangle( a, b, c );
                    }
                    if( cell.triangles & 2 )
                    {
                        triangle( d, c, b );
                    }
                }
            };
            for( unsigned pass = 0; pass < 1024; ++pass )
            {
                result.sweeps = pass + 1;
                bool changed = false;
                const auto triangle = [&]( size_t a, size_t b, size_t c ) {
                    const size_t ids[] = { a, b, c };
                    double h[] = { values[a], values[b], values[c] };
                    const double gx = ( h[1] - h[0] ) * ix, gz = ( h[2] - h[0] ) * iz;
                    const double grade2 = gx * gx + gz * gz;
                    if( grade2 <= limit2 * ( 1 + 1e-6 ) )
                    {
                        return;
                    }
                    unsigned pins = 0, free = 0;
                    for( unsigned j = 0; j < 3; ++j )
                    {
                        if( pinsAt[ids[j]] )
                        {
                            ++pins;
                        }
                        else
                        {
                            free = j;
                        }
                    }
                    if( pins == 3 )
                    {
                        return;
                    }
                    if( pins < 2 )
                    {
                        // Orthogonal projection onto the supporting halfspace
                        // of the grade cone. Radially scaling heights is not an
                        // orthogonal projection for right terrain triangles and
                        // can cycle as adjacent cells project one another.
                        const double grade = std::sqrt( grade2 ), nx = gx / grade, nz = gz / grade;
                        const double v[] = { -nx * ix - nz * iz, nx * ix, nz * iz };
                        double norm2 = 0;
                        for( unsigned j = 0; j < 3; ++j )
                        {
                            if( !pinsAt[ids[j]] )
                            {
                                norm2 += v[j] * v[j];
                            }
                        }
                        if( norm2 <= 1e-30 )
                        {
                            return;
                        }
                        const double step = ( grade - limit ) / norm2;
                        for( unsigned j = 0; j < 3; ++j )
                        {
                            if( !pinsAt[ids[j]] )
                            {
                                h[j] -= step * v[j];
                            }
                        }
                    }
                    else
                    {
                        // Solve for a height delta, avoiding cancellation between
                        // large absolute world-height terms in the quadratic.
                        const double ax = free == 0   ? -ix
                                          : free == 1 ? ix
                                                      : 0,
                                     az = free == 0   ? -iz
                                          : free == 2 ? iz
                                                      : 0,
                                     qa = ax * ax + az * az, dot = ax * gx + az * gz,
                                     minimumGrade2 = std::max( 0., grade2 - dot * dot / qa );
                        if( minimumGrade2 > double( maximumGrade ) * maximumGrade * ( 1 + 1e-6 ) )
                        {
                            return;
                        }
                        const double center = -dot / qa,
                                     radius = std::sqrt( std::max( 0., limit2 - minimumGrade2 ) / qa );
                        h[free] += std::clamp( 0., center - radius, center + radius );
                    }
                    for( unsigned j = 0; j < 3; ++j )
                    {
                        if( !pinsAt[ids[j]] )
                        {
                            const float value = float( h[j] );
                            changed = changed || value != values[ids[j]];
                            values[ids[j]] = value;
                        }
                    }
                };
                // Alternate traversal removes a directional bias from successive
                // projections while keeping the solver deterministic.
                for( size_t entry = 0; entry < cells.size(); ++entry )
                {
                    require( ++visited <= 64000000,
                             "Road terrain fit grade projection work budget exceeded." );
                    const auto cell = cells[pass & 1 ? cells.size() - 1 - entry : entry];
                    const size_t a = cell.first, b = a + 1, c = a + grid.width, d = c + 1;
                    if( cell.triangles & 1 )
                    {
                        triangle( a, b, c );
                    }
                    if( cell.triangles & 2 )
                    {
                        triangle( d, c, b );
                    }
                }
                if( !changed || pass % 8 == 7 )
                {
                    measure();
                    if( !changed || result.maximum <= maximumGrade * .999f )
                    {
                        return result;
                    }
                }
            }
            measure();
            return result;
        }

        float maximumGrade( const CityRoadSplineBake &bake )
        {
            float result = 0;
            const auto visit = [&]( Vector3F a, Vector3F b ) {
                const auto run = distance( a, b );
                if( run > .00001f )
                {
                    result = std::max( result, std::abs( a.y - b.y ) / run );
                }
            };
            for( const auto &road : bake.roads )
            {
                for( size_t i = 0; i < road.points.size(); ++i )
                {
                    const auto &p = road.points[i];
                    visit( p.left, p.position );
                    visit( p.position, p.right );
                    if( i )
                    {
                        const auto &a = road.points[i - 1];
                        visit( a.position, p.position );
                        visit( a.left, p.left );
                        visit( a.right, p.right );
                    }
                }
            }
            return result;
        }

        void setHeightSpan( CityRoadTerrainFitResult &candidate )
        {
            float minimum = std::numeric_limits<float>::max(), maximum = -minimum;
            for( const auto &road : candidate.bake.roads )
            {
                for( const auto &point : road.points )
                {
                    minimum = std::min( minimum, point.position.y );
                    maximum = std::max( maximum, point.position.y );
                }
            }
            candidate.report.roadHeightSpan = maximum >= minimum ? maximum - minimum : 0;
        }
    }  // namespace

    void CityRoadTerrainFitResult::swap( CityRoadTerrainFitResult &other ) noexcept
    {
        source.swap( other.source );
        terrain.swap( other.terrain );
        bake.roads.swap( other.bake.roads );
        using std::swap;
        swap( bake.sourceRevision, other.bake.sourceRevision );
        swap( report, other.report );
    }

    static bool prepareFit( const procedural::CityRoadSplineSource &source,
                            render::TerrainSnapshot terrain, const Transform3<real_Num> &placement,
                            const CityRoadTerrainFitOptions &options, const CityRoadSplineSource *before,
                            CityRoadTerrainFitResult &output, String &error )
    {
        try
        {
            require( bool( terrain ), "Road terrain fitting requires a retained terrain snapshot." );
            checked( render::validateTerrainPlacement( *terrain, placement, error ), error );
            const real_Num combinedVerticalScale = placement.getScale().y * terrain->heightScale;
            require( std::isfinite( combinedVerticalScale ) && combinedVerticalScale > 0,
                     "Road terrain fitting requires a finite positive combined vertical scale." );
            checked( procedural::validateCityRoadSplineSource( source, error ), error );
            String originalError;
            if( before )
            {
                checked( validateCityRoadSplineSource( *before, error ), error );
                require(
                    before->id == source.id && before->terrainResourceUuid == source.terrainResourceUuid,
                    "Road terrain fitting requires the same source and terrain binding." );
                const auto sampler = [&]( const Vector3F &point, float &height ) {
                    return render::sampleTerrainHeight(
                        *terrain, placement,
                        { real_Num( point.x ), real_Num( point.y ), real_Num( point.z ) }, height );
                };
                CityRoadTerrainFitResult candidate;
                CityRoadTerrainMesh mesh;
                const auto denials = sourceBudgetDenialCount();
                if( !changedRoadFootprint( *before, source ) &&
                    sampleCityRoadSplines( source, options.sampling, sampler, candidate.bake,
                                           originalError ) &&
                    buildCityRoadTerrainMesh( source, candidate.bake, terrain, placement, mesh,
                                              originalError, options.connectRoads ) )
                {
                    // Retain the exact snapshot for supported drawings, deletions
                    // and curve-preserving knot insertion.
                    candidate.source = source;
                    candidate.terrain = terrain;
                    candidate.report.maxRoadGrade = maximumGrade( candidate.bake );
                    setHeightSpan( candidate );
                    output.swap( candidate );
                    error.clear();
                    return true;
                }
                if( denials != sourceBudgetDenialCount() )
                {
                    throw std::bad_alloc();
                }
            }
            require( std::isfinite( options.shoulderWidth ) && options.shoulderWidth >= 1 &&
                         options.shoulderWidth <= 128 && std::isfinite( options.smoothingRadius ) &&
                         options.smoothingRadius >= 1 && options.smoothingRadius <= 256 &&
                         std::isfinite( options.maximumEarthwork ) && options.maximumEarthwork > 0 &&
                         options.maximumEarthwork <= 100 && options.maximumTerrainSamples >= 4 &&
                         options.maximumTerrainSamples <= render::terrainMaximumSamples,
                     "Invalid road terrain fit shoulder, smoothing, earthwork or sample budget." );
            require( terrain->heights.size() <= options.maximumTerrainSamples,
                     "Road terrain fitting exceeds the selected terrain sample budget." );
            Grid grid( *terrain, placement );
            const auto flatSource = flattenRoadSource( source );
            const auto zero = []( const Vector3F &, float &y ) {
                y = 0;
                return true;
            };
            CityRoadSplineBake flatBake;
            checked( sampleCityRoadSplines( flatSource, options.sampling, zero, flatBake, error ),
                     error );
            CityRoadSplineBake authoredBake;
            // Validates authored Fixed grades independently of terrain. Drape control
            // elevations are unused and are never rewritten by this helper.
            checked( sampleCityRoadSplines( source, options.sampling, zero, authoredBake, error ),
                     error );
            const auto edits =
                before ? editedRoads( *before, source ) : SourceBudgetVector<EditedRoad>{};
            auto pads =
                preparePads( source, flatSource, flatBake, grid, options.connectRoads, before, edits );
            require( !before ||
                         std::any_of(
                             edits.begin(), edits.end(),
                             []( const auto &entry ) { return entry.all || !entry.spans.empty(); } ) ||
                         std::any_of( pads.begin(), pads.end(),
                                      []( const auto &pad ) { return pad.active; } ),
                     originalError.c_str() );
            SourceBudgetVector<SourceBudgetVector<u8>> activeSegments;
            SourceBudgetVector<u8> protectedRoads, protectedPads;
            u64 protectionVisits = 0;
            if( before )
            {
                activeSegments.resize( source.roads.size() );
                protectedRoads.resize( terrain->heights.size(), 0 );
                protectedPads.resize( terrain->heights.size(), 0 );
                for( const auto &pad : pads )
                {
                    if( !pad.active )
                    {
                        grid.visit( pad.center, pad.center, pad.radius, [&]( size_t i, Vector3F point ) {
                            require( ++protectionVisits <= 64000000,
                                     "Road terrain fit protection work budget exceeded." );
                            if( distance( point, pad.center ) <= pad.radius )
                            {
                                protectedPads[i] = 1;
                            }
                        } );
                    }
                }
                for( size_t r = 0; r < source.roads.size(); ++r )
                {
                    const size_t oldRoad = [&] {
                        const auto old = std::find_if(
                            before->roads.begin(), before->roads.end(),
                            [&]( const auto &entry ) { return entry.id == source.roads[r].id; } );
                        return size_t( old - before->roads.begin() );
                    }();
                    const bool fixed = source.roads[r].heightMode == CityRoadHeightMode::Fixed;
                    const bool retainedRoad = oldRoad < before->roads.size();
                    const auto &points = fixed ? authoredBake.roads[r].points : flatBake.roads[r].points;
                    const float width = source.roads[r].width * .5f;
                    activeSegments[r].resize( points.size() );
                    for( size_t j = 1; j < points.size(); ++j )
                    {
                        const auto a = points[j - 1].position, b = points[j].position;
                        if( edits[r].meets( a, b, options.shoulderWidth + grid.apron ) )
                        {
                            activeSegments[r][j] = 1;
                            continue;
                        }
                        if( !retainedRoad )
                        {
                            continue;
                        }
                        const float radius = width + grid.apron;
                        const float vx = b.x - a.x, vz = b.z - a.z, length2 = vx * vx + vz * vz;
                        if( length2 < 1e-10f )
                        {
                            continue;
                        }
                        grid.visit( a, b, radius, [&]( size_t i, Vector3F point ) {
                            require( ++protectionVisits <= 64000000,
                                     "Road terrain fit protection work budget exceeded." );
                            const float t = std::clamp(
                                ( ( point.x - a.x ) * vx + ( point.z - a.z ) * vz ) / length2, 0.f,
                                1.f );
                            if( std::hypot( point.x - a.x - t * vx, point.z - a.z - t * vz ) <= radius )
                            {
                                protectedRoads[i] = fixed ? 2 : std::max( protectedRoads[i], u8( 1 ) );
                            }
                        } );
                    }
                }
            }
            SourceBudgetVector<float> smoothed( terrain->heights.size() );
            for( size_t i = 0; i < smoothed.size(); ++i )
            {
                smoothed[i] = grid.original( i );
            }
            smooth( smoothed, grid.width, grid.depth,
                    int( std::clamp( std::ceil( double( options.smoothingRadius ) / ( 2 * grid.dx ) ),
                                     1., double( grid.width - 1 ) ) ),
                    int( std::clamp( std::ceil( double( options.smoothingRadius ) / ( 2 * grid.dz ) ),
                                     1., double( grid.depth - 1 ) ) ) );
            double totalHeight = 0, totalPoints = 0;
            for( size_t r = 0; r < flatBake.roads.size(); ++r )
            {
                const auto &points = flatBake.roads[r].points;
                for( size_t j = 0; j < points.size(); ++j )
                {
                    if( before && !activeSegments[r][std::max( size_t( 1 ), j )] )
                    {
                        continue;
                    }
                    totalHeight += grid.sample( smoothed, points[j].position );
                    totalPoints += 1;
                }
            }
            for( const auto &pad : pads )
            {
                if( pad.active )
                {
                    totalHeight += grid.sample( smoothed, pad.center );
                    totalPoints += 1;
                }
            }
            const float anchor = totalPoints ? float( totalHeight / totalPoints ) : smoothed.front();
            // Start with all of the broad hill profile. Only failed continuous
            // support/grade checks can request a bounded gentler candidate.
            constexpr float gains[] = { 1.f, .8f, .6f, .4f, .25f };
            String lastError;
            SourceBudgetText attempts;
            for( u32 attempt = 0; attempt < sizeof( gains ) / sizeof( gains[0] ); ++attempt )
            {
                CityRoadTerrainFitResult candidate;
                candidate.source = source;
                candidate.report.attempts = attempt + 1;
                GradeProjection projection;
                const auto rejected = [&]( const char *stage ) {
                    char message[1024];
                    std::snprintf(
                        message, sizeof( message ),
                        "\nAttempt %u (hill gain %.2f, cut %.3f m, fill %.3f m, sampled grade %.3f%%, "
                        "core grade %.3f%% at %.2f,%.2f [%u pins/%u sweeps]), %s: %.650s",
                        unsigned( attempt + 1 ), double( gains[attempt] ),
                        double( candidate.report.maxCut ), double( candidate.report.maxFill ),
                        double( candidate.report.maxRoadGrade * 100 ),
                        double( projection.maximum * 100 ), double( projection.position.x ),
                        double( projection.position.z ), projection.pins, projection.sweeps, stage,
                        lastError.c_str() );
                    attempts.append( message );
                };
                auto fitted = copyTerrain( *terrain );
                SourceBudgetVector<float> field( smoothed.size() );
                for( size_t i = 0; i < field.size(); ++i )
                {
                    field[i] = anchor + ( smoothed[i] - anchor ) * gains[attempt];
                }
                u64 visited = protectionVisits;
                setPadPlanes( pads, grid, field, visited );
                SourceBudgetVector<Accumulator> roads( field.size() );

                struct FixedTarget
                {
                    float distance2 = std::numeric_limits<float>::max(), height = 0, influence = 0;
                };

                SourceBudgetVector<FixedTarget> fixedTargets;
                if( std::any_of( source.roads.begin(), source.roads.end(), []( const auto &r ) {
                        return r.heightMode == CityRoadHeightMode::Fixed;
                    } ) )
                {
                    fixedTargets.resize( field.size() );
                }
                for( size_t r = 0; r < source.roads.size(); ++r )
                {
                    const auto &road = source.roads[r];
                    const bool fixed = road.heightMode == CityRoadHeightMode::Fixed;
                    const auto &points = fixed ? authoredBake.roads[r].points : flatBake.roads[r].points;
                    const float core = road.width * .5f + grid.apron;
                    // The emitted carriageway follows the fitted heightfield exactly.
                    // A small crowned terrain profile gives both its render and collision
                    // mesh real crossfall without separating the road from its support.
                    const float crownHeight = fixed ? 0.f : std::min( .05f, road.width * .006f );
                    const float endTaper = std::max( 6.f, road.width * .75f );
                    const float roadLength = points.empty() ? 0.f : points.back().distance;
                    for( size_t j = 1; j < points.size(); ++j )
                    {
                        if( before && !activeSegments[r][j] )
                        {
                            continue;
                        }
                        const auto a = points[j - 1].position, b = points[j].position;
                        const float vx = b.x - a.x, vz = b.z - a.z, length2 = vx * vx + vz * vz;
                        if( length2 < 1e-10f )
                        {
                            continue;
                        }
                        const float ay = fixed ? a.y - road.heightOffset : grid.sample( field, a ),
                                    by = fixed ? b.y - road.heightOffset : grid.sample( field, b );
                        grid.visit(
                            a, b, core + options.shoulderWidth, [&]( size_t index, Vector3F point ) {
                                require( ++visited <= 64000000,
                                         "Road terrain fit raster work budget exceeded." );
                                const float t = std::clamp(
                                    ( ( point.x - a.x ) * vx + ( point.z - a.z ) * vz ) / length2, 0.f,
                                    1.f );
                                const float d =
                                    std::hypot( point.x - a.x - t * vx, point.z - a.z - t * vz );
                                const float influence = feather( d, core, options.shoulderWidth );
                                if( influence <= 0 )
                                {
                                    return;
                                }
                                if( fixed )
                                {
                                    auto &target = fixedTargets[index];
                                    if( d * d < target.distance2 )
                                    {
                                        target = { d * d, ay + ( by - ay ) * t, influence };
                                    }
                                    return;
                                }
                                // Short capped segments otherwise over-weight dense knot
                                // regions. Distance weighting preserves a continuous blend.
                                const float confidence = std::sqrt( length2 ) / ( 1.f + d * d );
                                float crown = 0.f;
                                if( d < road.width * .5f )
                                {
                                    float capTaper = 1.f;
                                    if( !road.closed )
                                    {
                                        const float station =
                                            points[j - 1].distance +
                                            ( points[j].distance - points[j - 1].distance ) * t;
                                        float start = station, end = roadLength - station;
                                        const auto &first = points.front().position,
                                                   &last = points.back().position;
                                        // A neighbouring segment can rasterize the same cap
                                        // vertex with a nonzero station; radial distance makes
                                        // the taper exact at the authored endpoint.
                                        if( std::abs( point.x - first.x ) < endTaper &&
                                            std::abs( point.z - first.z ) < endTaper )
                                        {
                                            start = std::min( start, distance( point, first ) );
                                        }
                                        if( std::abs( point.x - last.x ) < endTaper &&
                                            std::abs( point.z - last.z ) < endTaper )
                                        {
                                            end = std::min( end, distance( point, last ) );
                                        }
                                        capTaper = ( 1.f - feather( start, 0.f, endTaper ) ) *
                                                   ( 1.f - feather( end, 0.f, endTaper ) );
                                    }
                                    crown = crownHeight * feather( d, 0.f, road.width * .5f ) * capTaper;
                                }
                                // Junction and roundabout pad weights below blend this
                                // crown out through their shoulder into an exact plane.
                                roads[index].add( ay + ( by - ay ) * t + crown, influence, confidence );
                            } );
                    }
                }
                SourceBudgetVector<Accumulator> padValues( field.size() );
                SourceBudgetVector<u8> padCore( field.size(), 0 );
                for( const auto &pad : pads )
                {
                    if( !pad.active )
                    {
                        continue;
                    }
                    grid.visit( pad.center, pad.center, pad.radius + options.shoulderWidth,
                                [&]( size_t index, Vector3F point ) {
                                    require( ++visited <= 64000000,
                                             "Road terrain fit pad work budget exceeded." );
                                    const float d = distance( point, pad.center );
                                    const float influence =
                                        feather( d, pad.radius, options.shoulderWidth );
                                    if( influence <= 0 )
                                    {
                                        return;
                                    }
                                    const bool core = d <= pad.radius;
                                    if( padCore[index] && !core )
                                    {
                                        return;
                                    }
                                    if( core && !padCore[index] )
                                    {
                                        padValues[index] = {};
                                        padCore[index] = 1;
                                    }
                                    // Smooth partition of unity: a pad approaching its
                                    // hard core dominates every unrelated shoulder, so
                                    // the exact core plane has no discontinuous boundary.
                                    const float remaining = std::max( .00001f, 1.f - influence );
                                    padValues[index].add( pad.at( point ), influence,
                                                          1.f / ( remaining * remaining ) );
                                } );
                    if( pad.roundabout >= 0 )
                    {
                        auto &junction = candidate.source.roundabouts[size_t( pad.roundabout )];
                        junction.center.y = pad.height + pad.offset;
                        junction.gradeX = pad.gx;
                        junction.gradeZ = pad.gz;
                    }
                }
                SourceBudgetVector<u8> roadCore( field.size(), 0 ), pinned( field.size(), 0 );
                for( size_t i = 0; i < fitted->heights.size(); ++i )
                {
                    const auto original = grid.original( i );
                    float height = original;
                    // An edited ordinary junction owns its new intersection
                    // footprint, but remote supports and locked existing
                    // roundabout planes remain fixed during grade projection.
                    const bool protect = before && ( protectedPads[i] || protectedRoads[i] == 2 ||
                                                     ( protectedRoads[i] && !padCore[i] ) );
                    if( protect )
                    {
                        roadCore[i] = roads[i].influence >= 1.f || padCore[i];
                        pinned[i] = 1;
                        field[i] = original;
                        continue;
                    }
                    if( roads[i].weight > 0 )
                    {
                        height +=
                            ( float( roads[i].target / roads[i].weight ) - height ) * roads[i].influence;
                    }
                    if( !fixedTargets.empty() && fixedTargets[i].influence > 0 )
                    {
                        height += ( fixedTargets[i].height - height ) * fixedTargets[i].influence;
                    }
                    if( padValues[i].weight > 0 )
                    {
                        height += ( float( padValues[i].target / padValues[i].weight ) - height ) *
                                  padValues[i].influence;
                    }
                    const bool fixedCore = !fixedTargets.empty() && fixedTargets[i].influence >= 1.f;
                    roadCore[i] = roads[i].influence >= 1.f || fixedCore || padCore[i];
                    pinned[i] = fixedCore || padCore[i];
                    field[i] = height;
                }
                float gradeLimit = .35f;
                for( size_t r = 0; r < source.roads.size(); ++r )
                {
                    if( !before || edits[r].all || !edits[r].spans.empty() )
                    {
                        gradeLimit = std::min( gradeLimit, source.roads[r].maxGrade );
                    }
                }
                if( before )
                {
                    for( const auto &pad : pads )
                    {
                        if( pad.active )
                        {
                            gradeLimit = std::min( gradeLimit, pad.maxGrade );
                        }
                    }
                }
                else
                {
                    for( const auto &junction : source.roundabouts )
                    {
                        gradeLimit = std::min( gradeLimit, junction.maxGrade );
                    }
                }
                projection = relaxRoadGrades( field, grid, roadCore, pinned, gradeLimit, visited );
                bool earthworkExceeded = false;
                for( size_t i = 0; i < fitted->heights.size(); ++i )
                {
                    if( before && ( protectedPads[i] || protectedRoads[i] == 2 ||
                                    ( protectedRoads[i] && !padCore[i] ) ) )
                    {
                        continue;
                    }
                    if( roads[i].influence == 0 && padValues[i].influence == 0 &&
                        ( fixedTargets.empty() || fixedTargets[i].influence == 0 ) )
                    {
                        continue;
                    }
                    const auto original = grid.original( i ), height = field[i];
                    const float delta = height - original;
                    candidate.report.maxCut = std::max( candidate.report.maxCut, -delta );
                    candidate.report.maxFill = std::max( candidate.report.maxFill, delta );
                    earthworkExceeded =
                        earthworkExceeded || std::abs( delta ) > options.maximumEarthwork + .00001f;
                    fitted->heights[i] = float( ( height - placement.getPosition().y ) /
                                                ( placement.getScale().y * terrain->heightScale ) );
                    if( fitted->heights[i] != terrain->heights[i] )
                    {
                        ++candidate.report.changedSamples;
                    }
                }
                if( earthworkExceeded )
                {
                    lastError =
                        "Road terrain fit exceeds the selected maximum cut/fill; raise the limit or "
                        "edit the route.";
                    rejected( "earthwork" );
                    continue;
                }
                candidate.terrain = before && !candidate.report.changedSamples ? terrain : fitted;
                const auto sampler = [fitted, placement]( const Vector3F &point, float &height ) {
                    return render::sampleTerrainHeight(
                        *fitted, placement,
                        { real_Num( point.x ), real_Num( point.y ), real_Num( point.z ) }, height );
                };
                if( !sampleCityRoadSplines( candidate.source, options.sampling, sampler, candidate.bake,
                                            lastError ) )
                {
                    rejected( "road sampling" );
                    continue;
                }
                candidate.report.maxRoadGrade = maximumGrade( candidate.bake );
                CityRoadTerrainMesh mesh;
                if( !buildCityRoadTerrainMesh( candidate.source, candidate.bake, fitted, placement, mesh,
                                               lastError, options.connectRoads ) )
                {
                    rejected( "continuous terrain mesh" );
                    continue;
                }
                setHeightSpan( candidate );
                candidate.report.rejectedAttempts.swap( attempts );
                output.swap( candidate );
                error.clear();
                return true;
            }
            error =
                String( "Road terrain fit could not produce a supported profile:" ) + attempts.c_str();
            return false;
        }
        catch( const std::bad_alloc & )
        {
            error = "Road terrain fit exceeded the shared SourceCpu budget.";
            return false;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            return false;
        }
    }

    bool prepareCityRoadTerrainRestoration( const procedural::CityRoadSplineSource &draft,
                                            render::TerrainSnapshot active,
                                            const Transform3<real_Num> &placement,
                                            const CityRoadTerrainGrading &grading,
                                            render::TerrainSnapshot &output, String &error )
    {
        try
        {
            require( bool( active ), "Road terrain restoration requires an active heightfield." );
            const std::string_view uuid( draft.terrainResourceUuid.data(),
                                         draft.terrainResourceUuid.size() );
            if( !grading || !gradingMatches( *grading, uuid, *active, placement ) )
            {
                output = std::move( active );
                error.clear();
                return true;
            }
            checked( render::validateTerrainPlacement( *active, placement, error ), error );
            checked( procedural::validateCityRoadSplineSource( draft, error ), error );
            const auto covered = terrainGradingCoverage( draft, Grid( *active, placement ), error );
            std::shared_ptr<render::TerrainData> restored;
            for( const auto &sample : grading->samples )
            {
                if( covered[sample.index] || active->heights[sample.index] != sample.applied )
                {
                    continue;
                }
                if( !restored )
                {
                    restored = copyTerrain( *active );
                }
                restored->heights[sample.index] = sample.original;
            }
            output = restored ? render::TerrainSnapshot( std::move( restored ) ) : std::move( active );
            error.clear();
            return true;
        }
        catch( const std::bad_alloc & )
        {
            error = "Road terrain restoration exceeded the shared SourceCpu budget.";
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
        }
        return false;
    }

    bool retainCityRoadTerrainGrading( std::string_view terrainUuid, render::TerrainSnapshot active,
                                       render::TerrainSnapshot prepared,
                                       const Transform3<real_Num> &placement,
                                       const CityRoadTerrainGrading &previous,
                                       CityRoadTerrainGrading &output, String &error )
    {
        try
        {
            require( gradingUuid( terrainUuid ) && active && prepared &&
                         sameGradingGrid( *active, *prepared ) &&
                         active->heights.size() == prepared->heights.size(),
                     "Terrain grading records require matching terrain identity and grids." );
            const auto matching =
                previous && gradingMatches( *previous, terrainUuid, *active, placement );
            if( active == prepared && !matching )
            {
                // A changed placement does not invalidate the old samples if
                // the terrain later returns to that placement with its data.
                output = previous && std::string_view( previous->terrainUuid.data(),
                                                       previous->terrainUuid.size() ) == terrainUuid
                             ? previous
                             : nullptr;
                error.clear();
                return true;
            }
            require( active->heights.size() <= maximumGradingSamples && active->dimensions.x >= 2 &&
                         active->dimensions.y >= 2 &&
                         u64( active->dimensions.x ) * active->dimensions.y == active->heights.size(),
                     "Terrain grading exceeds the original sample record budget." );
            checked( render::validateTerrainPlacement( *active, placement, error ), error );
            auto next = std::allocate_shared<CityRoadTerrainGradeData>(
                SourceBudgetAllocator<CityRoadTerrainGradeData>{} );
            next->terrainUuid.assign( terrainUuid.data(), terrainUuid.size() );
            next->dimensions = active->dimensions;
            next->origin = active->origin;
            next->spacing = active->spacing;
            next->heightScale = active->heightScale;
            next->placement = placementKey( placement );
            size_t old = 0;
            for( size_t i = 0; i < active->heights.size(); ++i )
            {
                const auto current = active->heights[i];
                auto original = current;
                if( matching && old < previous->samples.size() && previous->samples[old].index == i )
                {
                    const auto &entry = previous->samples[old++];
                    // A brush edit starts a new baseline if this transaction
                    // grades that sample again. It is never put back silently.
                    if( current == entry.applied )
                    {
                        original = entry.original;
                    }
                }
                const auto applied = prepared->heights[i];
                require( std::isfinite( applied ), "Prepared road terrain has an invalid height." );
                if( applied != original )
                {
                    next->samples.push_back( { u32( i ), original, applied } );
                }
            }
            if( next->samples.empty() )
            {
                output.reset();
            }
            else if( matching && next->samples == previous->samples )
            {
                output = previous;
            }
            else
            {
                output = std::move( next );
            }
            error.clear();
            return true;
        }
        catch( const std::bad_alloc & )
        {
            error = "Road terrain records exceeded the shared SourceCpu budget.";
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
        }
        return false;
    }

    bool serializeCityRoadTerrainGrading( const CityRoadTerrainGrading &grading,
                                          SourceBudgetText &output, String &error )
    {
        try
        {
            SourceBudgetText encoded;
            if( grading )
            {
                // Fixed-width hexadecimal records round-trip exact heights
                // without building a second JSON tree for large terrain grids.
                const auto &data = *grading;
                encoded.reserve( gradingHeaderBytes + gradingRecordBytes * data.samples.size() );
                encoded.append( gradingPrefix.data(), gradingPrefix.size() );
                encoded.append( data.terrainUuid.data(), data.terrainUuid.size() );
                writeGradeHex( encoded, u32( data.dimensions.x ) );
                writeGradeHex( encoded, u32( data.dimensions.y ) );
                for( auto value :
                     { data.origin.x, data.origin.y, data.spacing.x, data.spacing.y, data.heightScale } )
                {
                    writeGradeHex( encoded, gradeBits<u32>( value ) );
                }
                for( auto value : data.placement )
                {
                    writeGradeHex( encoded, gradeBits<u64>( value ) );
                }
                for( const auto &sample : data.samples )
                {
                    writeGradeHex( encoded, sample.index );
                    writeGradeHex( encoded, gradeBits<u32>( sample.original ) );
                    writeGradeHex( encoded, gradeBits<u32>( sample.applied ) );
                }
            }
            output.swap( encoded );
            error.clear();
            return true;
        }
        catch( const std::bad_alloc & )
        {
            error = "Road terrain serialization exceeded the shared SourceCpu budget.";
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
        }
        return false;
    }

    bool deserializeCityRoadTerrainGrading( std::string_view value, std::string_view terrainUuid,
                                            CityRoadTerrainGrading &output, String &error )
    {
        try
        {
            if( value.empty() )
            {
                output.reset();
                error.clear();
                return true;
            }
            require(
                gradingUuid( terrainUuid ) && value.size() >= gradingHeaderBytes + gradingRecordBytes &&
                    value.size() <= gradingHeaderBytes + gradingRecordBytes * maximumGradingSamples &&
                    ( value.size() - gradingHeaderBytes ) % gradingRecordBytes == 0 &&
                    value.substr( 0, gradingPrefix.size() ) == gradingPrefix,
                "Invalid terrain grading data version or size." );
            value.remove_prefix( gradingPrefix.size() );
            require( value.substr( 0, 36 ) == terrainUuid,
                     "Terrain grading data belongs to a different terrain." );
            value.remove_prefix( 36 );
            auto decoded = std::allocate_shared<CityRoadTerrainGradeData>(
                SourceBudgetAllocator<CityRoadTerrainGradeData>{} );
            decoded->terrainUuid.assign( terrainUuid.data(), terrainUuid.size() );
            const auto width = readGradeHex<u32>( value ), depth = readGradeHex<u32>( value );
            const auto gridSize = u64( width ) * depth;
            require( width >= 2 && depth >= 2 && width <= render::terrainMaximumDimension &&
                         depth <= render::terrainMaximumDimension && gridSize <= maximumGradingSamples,
                     "Terrain grading source grid exceeds its dimensions budget." );
            decoded->dimensions = { s32( width ), s32( depth ) };
            const auto height = [&] { return gradeBits<f32>( readGradeHex<u32>( value ) ); };
            decoded->origin = { height(), height() };
            decoded->spacing = { height(), height() };
            decoded->heightScale = height();
            require( std::isfinite( decoded->origin.x ) && std::isfinite( decoded->origin.y ) &&
                         std::isfinite( decoded->spacing.x ) && std::isfinite( decoded->spacing.y ) &&
                         decoded->spacing.x > 0 && decoded->spacing.y > 0 &&
                         std::isfinite( decoded->heightScale ) && decoded->heightScale > 0,
                     "Terrain grading data has invalid grid metadata." );
            auto &pose = decoded->placement;
            for( auto &component : pose )
            {
                component = gradeBits<double>( readGradeHex<u64>( value ) );
            }
            const auto p = [&]( size_t i ) { return real_Num( pose[i] ); };
            const Transform3<real_Num> placement( { p( 0 ), p( 1 ), p( 2 ) },
                                                  { p( 6 ), p( 7 ), p( 8 ), p( 9 ) },
                                                  { p( 3 ), p( 4 ), p( 5 ) } );
            checked( render::validateTerrainTransform( placement, error ), error );
            require( value.size() / gradingRecordBytes <= gridSize,
                     "Terrain grading record count exceeds its grid." );
            decoded->samples.reserve( value.size() / gradingRecordBytes );
            while( !value.empty() )
            {
                const auto index = readGradeHex<u32>( value );
                const auto original = height(), applied = height();
                require( index < gridSize &&
                             ( decoded->samples.empty() || decoded->samples.back().index < index ) &&
                             std::isfinite( original ) && std::isfinite( applied ) &&
                             original != applied,
                         "Invalid or duplicate terrain grading sample record." );
                decoded->samples.push_back( { index, original, applied } );
            }
            output = std::move( decoded );
            error.clear();
            return true;
        }
        catch( const std::bad_alloc & )
        {
            error = "Road terrain deserialization exceeded the shared SourceCpu budget.";
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
        }
        return false;
    }

    bool prepareCityRoadTerrainFit( const CityRoadSplineSource &source, render::TerrainSnapshot terrain,
                                    const Transform3<real_Num> &placement,
                                    const CityRoadTerrainFitOptions &options,
                                    CityRoadTerrainFitResult &output, String &error )
    {
        return prepareFit( source, std::move( terrain ), placement, options, nullptr, output, error );
    }

    bool prepareCityRoadTerrainEdit( const CityRoadSplineSource &before,
                                     const CityRoadSplineSource &draft, render::TerrainSnapshot terrain,
                                     const Transform3<real_Num> &placement,
                                     const CityRoadTerrainFitOptions &options,
                                     CityRoadTerrainFitResult &output, String &error )
    {
        return prepareFit( draft, std::move( terrain ), placement, options, &before, output, error );
    }
}  // namespace workphone::scene
