#pragma once

#include "CityRoadCrossings.hpp"
#include <stdexcept>

namespace workphone::scene::cityRoadCrossings
{
    namespace
    {
        using DPoint = Vector3<double>;
        constexpr double drop = .11, flank = .90, back = 1.84, shelf = .34, inner = -.38;
        constexpr size_t maximumRampVertices = 300000;

        struct Plane
        {
            double x, z, constant;
            double at( DPoint p ) const { return x * p.x + z * p.z + constant; }
            Plane operator-( const Plane &p ) const { return { x - p.x, z - p.z, constant - p.constant }; }
        };

        struct Bounds
        {
            double x0, z0, x1, z1;
            bool overlaps( const Bounds &b ) const
            {
                return x0 < b.x1 && x1 > b.x0 && z0 < b.z1 && z1 > b.z0;
            }
        };

        DPoint precise( Vector3F p ) { return { p.x, p.y, p.z }; }
        Vector3F renderPoint( DPoint p ) { return { float( p.x ), float( p.y ), float( p.z ) }; }
        Bounds boundsOf( DPoint a, DPoint b, DPoint c )
        {
            return { std::min( { a.x, b.x, c.x } ), std::min( { a.z, b.z, c.z } ),
                     std::max( { a.x, b.x, c.x } ), std::max( { a.z, b.z, c.z } ) };
        }

        double cross2( DPoint a, DPoint b ) { return a.x * b.z - a.z * b.x; }

        struct Ramp
        {
            std::array<Plane, 5> fall;
            Bounds bounds;

            double lowering( DPoint p ) const
            {
                double height = drop;
                for( const auto &surface : fall ) height = std::min( height, surface.at( p ) );
                return std::max( 0., height );
            }
        };

        Ramp makeRamp( const Crossing &crossing, float sign )
        {
            const auto centre = precise( crossing.centre ), along = precise( crossing.outward );
            const auto across = precise( crossing.across ) * double( sign );
            const double half = crossing.width * .5, length = crossing.depth * .5 + flank;
            const auto plane = [&]( DPoint axis, double offset, double gradient ) {
                return Plane{ axis.x * gradient, axis.z * gradient,
                              ( offset - axis.x * centre.x - axis.z * centre.z ) * gradient };
            };
            const auto left = centre + across * ( half + inner );
            const auto right = centre + across * ( half + back );
            auto bounds = boundsOf( left - along * length, left + along * length, right - along * length );
            const auto corner = right + along * length;
            bounds.x0 = std::min( bounds.x0, corner.x ); bounds.x1 = std::max( bounds.x1, corner.x );
            bounds.z0 = std::min( bounds.z0, corner.z ); bounds.z1 = std::max( bounds.z1, corner.z );
            return { { Plane{ 0, 0, drop }, plane( along, length, drop / flank ),
                       plane( -along, length, drop / flank ),
                       plane( across, -half - inner, drop / .23 ),
                       plane( -across, half + back, drop / ( back - shelf ) ) }, bounds };
        }

        using Cell = std::pair<s32, s32>;
        Cell cell( DPoint p ) { return { s32( std::floor( p.x / 8. ) ), s32( std::floor( p.z / 8. ) ) }; }

        template <class Visit>
        bool visitCells( const Bounds &bounds, const Visit &visit )
        {
            const auto a = cell( { bounds.x0, 0, bounds.z0 } ), b = cell( { bounds.x1, 0, bounds.z1 } );
            if( u64( s64( b.first ) - a.first + 1 ) * u64( s64( b.second ) - a.second + 1 ) > 512 ) return false;
            for( auto x = a.first; x <= b.first; ++x )
                for( auto z = a.second; z <= b.second; ++z ) visit( Cell( x, z ) );
            return true;
        }

        struct Skirt { std::array<u32, 4> vertices; };

        u32 append( CityRoadTerrainMesh &mesh, CityRoadTerrainVertex vertex )
        {
            if( mesh.vertices.size() >= maximumRampVertices ) throw std::runtime_error( "Dropped kerb vertex budget exceeded." );
            mesh.vertices.push_back( vertex );
            return u32( mesh.vertices.size() - 1 );
        }

        void triangle( CityRoadTerrainMesh &mesh, u32 a, u32 b, u32 c )
        {
            mesh.indices.insert( mesh.indices.end(), { a, b, c } );
            mesh.triangleKinds.push_back( CityRoadTerrainTriangleKind::Kerb );
        }

        struct Polygon { std::array<DPoint, 16> points; size_t count = 0; };

        Polygon clip( const Polygon &polygon, Plane plane, bool inside = true )
        {
            Polygon output;
            if( !polygon.count ) return output;
            auto previous = polygon.points[polygon.count - 1];
            double before = plane.at( previous ) * ( inside ? 1 : -1 );
            for( size_t i = 0; i < polygon.count; ++i )
            {
                const auto current = polygon.points[i];
                const double distance = plane.at( current ) * ( inside ? 1 : -1 );
                if( ( before < 0 && distance > 0 ) || ( before > 0 && distance < 0 ) )
                {
                    if( output.count == output.points.size() ) throw std::runtime_error( "Dropped kerb clip budget exceeded." );
                    output.points[output.count++] = previous + ( current - previous ) * ( before / ( before - distance ) );
                }
                if( distance >= 0 )
                {
                    if( output.count == output.points.size() ) throw std::runtime_error( "Dropped kerb clip budget exceeded." );
                    output.points[output.count++] = current;
                }
                previous = current;
                before = distance;
            }
            return output;
        }

        Polygon interior( Polygon polygon, const Ramp &ramp )
        {
            for( size_t i = 1; i < ramp.fall.size() && polygon.count >= 3; ++i )
                polygon = clip( polygon, ramp.fall[i] );
            return polygon;
        }

        void emit( CityRoadTerrainMesh &mesh, Polygon polygon, const Plane &dropPlane )
        {
            for( size_t i = 0; i < polygon.count; ++i )
                polygon.points[i].y -= std::clamp( dropPlane.at( polygon.points[i] ), 0., drop );
            for( size_t i = 1; i + 1 < polygon.count; ++i )
            {
                const auto a = renderPoint( polygon.points[0] ), b = renderPoint( polygon.points[i] ),
                           c = renderPoint( polygon.points[i + 1] );
                auto normal = ( b - a ).crossProduct( c - a );
                if( std::abs( normal.y ) < 1e-8f ) continue;
                const bool forward = normal.y > 0;
                normal = ( forward ? normal : -normal ).normaliseCopy();
                const auto vertex = [&]( Vector3F p ) {
                    return append( mesh, { p, normal, { p.x * .25f, p.z * .25f } } );
                };
                const auto ia = vertex( a ), ib = vertex( b ), ic = vertex( c );
                triangle( mesh, ia, forward ? ib : ic, forward ? ic : ib );
            }
        }
    }

    bool fitDroppedKerbs( CityRoadTerrainMesh &footways, const CityRoadTerrainMesh &roadMesh,
                          SourceBudgetVector<Crossing> &paintedCrossings, String &error )
    {
        error.clear();
        if( paintedCrossings.empty() || footways.indices.empty() ) return true;
        try
        {
            if( roadMesh.sourceRevision != footways.sourceRevision || roadMesh.terrainRevision != footways.terrainRevision )
                throw std::runtime_error( "Dropped kerbs require matching fitted road and sidewalk revisions." );
            SourceBudgetVector<Skirt> skirts;
            SourceBudgetMap<Cell, SourceBudgetVector<u32>> skirtCells, roadCells;
            size_t skirtStart = footways.indices.size(), topVertices = footways.vertices.size();
            for( size_t i = 0; i + 2 < footways.indices.size(); i += 3 )
            {
                const auto &a = footways.vertices[footways.indices[i]];
                const auto &b = footways.vertices[footways.indices[i + 1]];
                if( std::abs( a.normal.y ) < .01f && std::abs( a.position.x - b.position.x ) < .001f &&
                    std::abs( a.position.z - b.position.z ) < .001f && a.position.y - b.position.y > .10f )
                {
                    skirtStart = i;
                    break;
                }
            }
            if( skirtStart == footways.indices.size() ) return true;
            if( ( footways.indices.size() - skirtStart ) % 6 ) throw std::runtime_error( "Invalid fitted kerb topology." );
            for( size_t i = skirtStart; i < footways.indices.size(); i += 6 )
            {
                const auto *j = footways.indices.data() + i;
                if( j[0] != j[3] || j[2] != j[4] ) throw std::runtime_error( "Invalid fitted kerb faces." );
                topVertices = std::min( topVertices, size_t( std::min( { j[0], j[1], j[2], j[5] } ) ) );
                const auto a = precise( footways.vertices[j[0]].position ), b = precise( footways.vertices[j[5]].position );
                const auto id = u32( skirts.size() );
                skirts.push_back( { { j[0], j[1], j[2], j[5] } } );
                if( !visitCells( boundsOf( a, b, b ), [&]( Cell c ) { skirtCells[c].push_back( id ); } ) ) return true;
            }
            for( u32 i = 0; i < roadMesh.triangleKinds.size(); ++i )
            {
                if( roadMesh.triangleKinds[i] != CityRoadTerrainTriangleKind::Road ) continue;
                const auto a = precise( roadMesh.vertices[roadMesh.indices[3 * i]].position );
                const auto b = precise( roadMesh.vertices[roadMesh.indices[3 * i + 1]].position );
                const auto c = precise( roadMesh.vertices[roadMesh.indices[3 * i + 2]].position );
                if( !visitCells( boundsOf( a, b, c ), [&]( Cell c ) { roadCells[c].push_back( i ); } ) ) return true;
            }
            const auto roadHeight = [&]( DPoint p, double &height ) {
                const auto found = roadCells.find( cell( p ) );
                if( found == roadCells.end() ) return false;
                bool supported = false;
                for( const auto index : found->second )
                {
                    const auto a = precise( roadMesh.vertices[roadMesh.indices[3 * index]].position );
                    const auto b = precise( roadMesh.vertices[roadMesh.indices[3 * index + 1]].position );
                    const auto c = precise( roadMesh.vertices[roadMesh.indices[3 * index + 2]].position );
                    const double area = cross2( b - a, c - a );
                    if( std::abs( area ) < 1e-10 ) continue;
                    const double u = cross2( p - a, c - a ) / area, v = cross2( b - a, p - a ) / area;
                    if( std::min( { u, v, 1 - u - v } ) < -1e-6 ) continue;
                    const double y = a.y + u * ( b.y - a.y ) + v * ( c.y - a.y );
                    height = supported ? std::max( height, y ) : y;
                    supported = true;
                }
                return supported;
            };
            const auto supported = [&]( const Crossing &crossing, float sign, const Ramp &ramp ) {
                const auto axis = precise( crossing.across ) * double( sign );
                const double half = crossing.depth * .5 - .08, roadHalf = crossing.width * .5;
                for( const double offset : { -half, 0., half } )
                {
                    const auto origin = precise( crossing.centre ) + precise( crossing.outward ) * offset;
                    const auto p = origin + axis * ( roadHalf - .40 ), q = origin + axis * ( roadHalf + .42 );
                    double distance = std::numeric_limits<double>::max(), top = 0;
                    visitCells( boundsOf( p, q, q ), [&]( Cell key ) {
                        const auto found = skirtCells.find( key );
                        if( found == skirtCells.end() ) return;
                        for( const auto index : found->second )
                        {
                            const auto &vertices = skirts[index].vertices;
                            const auto &topA = footways.vertices[vertices[0]];
                            if( precise( topA.normal ).dotProduct( axis ) > -.46 ) continue;
                            const auto a = precise( topA.position ), b = precise( footways.vertices[vertices[3]].position );
                            const auto delta = b - a;
                            const double denominator = cross2( axis, delta );
                            if( std::abs( denominator ) < 1e-7 ) continue;
                            const double t = cross2( a - origin, delta ) / denominator;
                            const double u = cross2( a - origin, axis ) / denominator;
                            if( t >= roadHalf - .40 && t <= roadHalf + .42 && t < distance && u >= -1e-5 && u <= 1.00001 )
                            {
                                distance = t;
                                top = a.y + delta.y * u;
                            }
                        }
                    } );
                    double road = 0;
                    if( !std::isfinite( distance ) || distance > roadHalf + .42 ||
                        !roadHeight( origin + axis * ( distance - .18 ), road ) ) return false;
                    const double height = top - ramp.lowering( origin + axis * distance ) - road;
                    if( height < -.025 || height > .055 ) return false;
                }
                return true;
            };

            SourceBudgetVector<Ramp> ramps;
            SourceBudgetVector<size_t> accepted;
            for( size_t i = 0; i < paintedCrossings.size(); ++i )
            {
                const auto &crossing = paintedCrossings[i];
                const auto a = makeRamp( crossing, -1 ), b = makeRamp( crossing, 1 );
                if( std::any_of( ramps.begin(), ramps.end(), [&]( const Ramp &other ) {
                    return other.bounds.overlaps( a.bounds ) || other.bounds.overlaps( b.bounds );
                } ) || !supported( crossing, -1, a ) || !supported( crossing, 1, b ) ) continue;
                accepted.push_back( i );
                ramps.push_back( a ); ramps.push_back( b );
            }
            if( ramps.empty() ) return true;

            CityRoadTerrainMesh output;
            output.sourceRevision = footways.sourceRevision;
            output.terrainRevision = footways.terrainRevision;
            output.vertices.assign( footways.vertices.begin(), footways.vertices.begin() + topVertices );
            output.indices.reserve( footways.indices.size() );
            output.triangleKinds.reserve( footways.triangleKinds.size() );
            u64 comparisons = 0;
            for( size_t i = 0; i < skirtStart; i += 3 )
            {
                const auto a = footways.indices[i], b = footways.indices[i + 1], c = footways.indices[i + 2];
                if( std::max( { a, b, c } ) >= topVertices ) throw std::runtime_error( "Invalid fitted sidewalk faces." );
                Polygon original{ { precise( footways.vertices[a].position ), precise( footways.vertices[b].position ),
                                     precise( footways.vertices[c].position ) }, 3 };
                const auto bounds = boundsOf( original.points[0], original.points[1], original.points[2] );
                const Ramp *selected = nullptr;
                for( const auto &ramp : ramps )
                {
                    if( ++comparisons > 12000000 ) throw std::runtime_error( "Dropped kerb work budget exceeded." );
                    if( !ramp.bounds.overlaps( bounds ) || interior( original, ramp ).count < 3 ) continue;
                    if( selected ) throw std::runtime_error( "Two dropped kerbs share a fitted sidewalk face." );
                    selected = &ramp;
                }
                if( !selected ) { triangle( output, a, b, c ); continue; }
                auto remainder = original;
                for( size_t plane = 1; plane < selected->fall.size() && remainder.count >= 3; ++plane )
                {
                    emit( output, clip( remainder, selected->fall[plane], false ), { 0, 0, 0 } );
                    remainder = clip( remainder, selected->fall[plane] );
                }
                for( size_t region = 0; region < selected->fall.size(); ++region )
                {
                    auto polygon = remainder;
                    for( size_t other = 0; other < selected->fall.size() && polygon.count >= 3; ++other )
                        if( region != other ) polygon = clip( polygon, selected->fall[other] - selected->fall[region] );
                    emit( output, polygon, selected->fall[region] );
                }
            }

            SourceBudgetMap<u32, u32> unchangedSkirts;
            const auto remember = [&]( u32 index ) {
                const auto found = unchangedSkirts.find( index );
                if( found != unchangedSkirts.end() ) return found->second;
                const auto vertex = append( output, footways.vertices[index] );
                unchangedSkirts.emplace( index, vertex );
                return vertex;
            };
            const auto emitSkirt = [&]( u32 a, u32 b, u32 c, u32 d ) {
                triangle( output, a, b, c ); triangle( output, a, c, d );
            };
            for( const auto &skirt : skirts )
            {
                const auto &v = skirt.vertices;
                const auto a = precise( footways.vertices[v[0]].position ), b = precise( footways.vertices[v[3]].position );
                auto bounds = boundsOf( a, b, b );
                bounds.x0 -= .00001; bounds.x1 += .00001; bounds.z0 -= .00001; bounds.z1 += .00001;
                SourceBudgetVector<const Ramp *> local;
                SourceBudgetVector<double> cuts;
                for( const auto &ramp : ramps )
                {
                    if( !ramp.bounds.overlaps( bounds ) ) continue;
                    local.push_back( &ramp );
                    const auto crossing = [&]( Plane plane ) {
                        const double d0 = plane.at( a ), d1 = plane.at( b );
                        if( ( d0 < 0 && d1 > 0 ) || ( d0 > 0 && d1 < 0 ) ) cuts.push_back( d0 / ( d0 - d1 ) );
                    };
                    for( size_t i = 0; i < ramp.fall.size(); ++i )
                    {
                        crossing( ramp.fall[i] );
                        for( size_t j = i + 1; j < ramp.fall.size(); ++j ) crossing( ramp.fall[i] - ramp.fall[j] );
                    }
                }
                if( local.empty() ) { emitSkirt( remember( v[0] ), remember( v[1] ), remember( v[2] ), remember( v[3] ) ); continue; }
                cuts.push_back( 0 ); cuts.push_back( 1 );
                std::sort( cuts.begin(), cuts.end() );
                const auto bottomA = precise( footways.vertices[v[1]].position ), bottomB = precise( footways.vertices[v[2]].position );
                const auto normal = footways.vertices[v[0]].normal;
                const auto vertex = [&]( double t, bool top ) {
                    auto p = top ? a + ( b - a ) * t : bottomA + ( bottomB - bottomA ) * t;
                    double lowering = 0;
                    if( top ) for( const auto *ramp : local ) lowering = std::max( lowering, ramp->lowering( p ) );
                    p.y -= lowering;
                    return append( output, { renderPoint( p ), normal,
                                     { float( ( p.x * normal.z - p.z * normal.x ) * .25 ), float( p.y * .25 ) } } );
                };
                for( size_t i = 1; i < cuts.size(); ++i )
                {
                    const auto t0 = cuts[i - 1], t1 = cuts[i];
                    if( t1 - t0 < 1e-6 ) continue;
                    const auto p = vertex( t0, true ), q = vertex( t0, false );
                    const auto r = vertex( t1, false ), s = vertex( t1, true );
                    emitSkirt( p, q, r, s );
                }
            }
            footways.swap( output );
            for( const auto index : accepted ) paintedCrossings[index].droppedKerbs = true;
            return true;
        }
        catch( const std::exception &failure )
        {
            error = failure.what();
            return false;
        }
    }
}  // namespace workphone::scene::cityRoadCrossings
