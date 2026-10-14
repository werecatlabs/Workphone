#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPVehicleGeometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <utility>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            constexpr float kPi = 3.14159265358979323846f;
            constexpr float kTwoPi = 2.0f * kPi;
            constexpr float kEpsilon = 1.0e-7f;

            float clampF( float value, float lo, float hi )
            {
                return std::max( lo, std::min( value, hi ) );
            }

            Vector3F normalised( const Vector3F &v, const Vector3F &fallback = Vector3F( 0, 1, 0 ) )
            {
                const float length = v.length();
                return length > kEpsilon ? v / length : fallback;
            }

            struct Builder
            {
                VehicleMeshSection mesh;

                Builder( const char *name, VehicleMaterial material )
                {
                    mesh.name = name;
                    mesh.material = material;
                }

                std::uint32_t vertex( const Vector3F &p, const Vector2F &uv,
                                      const Vector3F &mask = Vector3F() )
                {
                    VehicleVertex v;
                    v.position = p;
                    v.uv = uv;
                    v.mask = mask;
                    mesh.vertices.push_back( v );
                    return static_cast<std::uint32_t>( mesh.vertices.size() - 1u );
                }

                void triangle( std::uint32_t a, std::uint32_t b, std::uint32_t c )
                {
                    mesh.indices.push_back( a );
                    mesh.indices.push_back( b );
                    mesh.indices.push_back( c );
                }

                void quad( const Vector3F &a, const Vector3F &b, const Vector3F &c, const Vector3F &d,
                           const Vector3F &mask = Vector3F() )
                {
                    const std::uint32_t base = static_cast<std::uint32_t>( mesh.vertices.size() );
                    vertex( a, Vector2F( 0, 0 ), mask );
                    vertex( b, Vector2F( 1, 0 ), mask );
                    vertex( c, Vector2F( 1, 1 ), mask );
                    vertex( d, Vector2F( 0, 1 ), mask );
                    triangle( base, base + 1, base + 2 );
                    triangle( base, base + 2, base + 3 );
                }

                void box( const Vector3F &centre, const Vector3F &size,
                          const Vector3F &mask = Vector3F() )
                {
                    const Vector3F h = size * 0.5f;
                    const std::array<Vector3F, 8> p = {
                        centre + Vector3F( -h.x, -h.y, -h.z ), centre + Vector3F( h.x, -h.y, -h.z ),
                        centre + Vector3F( h.x, h.y, -h.z ),   centre + Vector3F( -h.x, h.y, -h.z ),
                        centre + Vector3F( -h.x, -h.y, h.z ),  centre + Vector3F( h.x, -h.y, h.z ),
                        centre + Vector3F( h.x, h.y, h.z ),    centre + Vector3F( -h.x, h.y, h.z )
                    };
                    quad( p[0], p[3], p[2], p[1], mask );
                    quad( p[4], p[5], p[6], p[7], mask );
                    quad( p[0], p[4], p[7], p[3], mask );
                    quad( p[1], p[2], p[6], p[5], mask );
                    quad( p[0], p[1], p[5], p[4], mask );
                    quad( p[3], p[7], p[6], p[2], mask );
                }

                void finalise()
                {
                    for( auto &v : mesh.vertices )
                    {
                        v.normal = Vector3F();
                        v.tangent = Vector3F();
                        v.tangentSign = 1.0f;
                    }

                    Array<Vector3F> bitangents( mesh.vertices.size(), Vector3F() );
                    for( std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3 )
                    {
                        const std::uint32_t ia = mesh.indices[i];
                        const std::uint32_t ib = mesh.indices[i + 1];
                        const std::uint32_t ic = mesh.indices[i + 2];
                        if( ia >= mesh.vertices.size() || ib >= mesh.vertices.size() ||
                            ic >= mesh.vertices.size() )
                            continue;

                        VehicleVertex &a = mesh.vertices[ia];
                        VehicleVertex &b = mesh.vertices[ib];
                        VehicleVertex &c = mesh.vertices[ic];
                        const Vector3F e1 = b.position - a.position;
                        const Vector3F e2 = c.position - a.position;
                        const Vector3F face = e1.crossProduct( e2 );
                        a.normal += face;
                        b.normal += face;
                        c.normal += face;

                        const float du1 = b.uv.x - a.uv.x;
                        const float dv1 = b.uv.y - a.uv.y;
                        const float du2 = c.uv.x - a.uv.x;
                        const float dv2 = c.uv.y - a.uv.y;
                        const float determinant = du1 * dv2 - du2 * dv1;
                        Vector3F tangent;
                        Vector3F bitangent;
                        if( std::abs( determinant ) > kEpsilon )
                        {
                            const float r = 1.0f / determinant;
                            tangent = ( e1 * dv2 - e2 * dv1 ) * r;
                            bitangent = ( e2 * du1 - e1 * du2 ) * r;
                        }
                        else
                        {
                            tangent = normalised( e1, Vector3F( 1, 0, 0 ) );
                            bitangent = normalised( face.crossProduct( tangent ), Vector3F( 0, 0, 1 ) );
                        }
                        a.tangent += tangent;
                        b.tangent += tangent;
                        c.tangent += tangent;
                        bitangents[ia] += bitangent;
                        bitangents[ib] += bitangent;
                        bitangents[ic] += bitangent;
                    }

                    for( std::size_t i = 0; i < mesh.vertices.size(); ++i )
                    {
                        VehicleVertex &v = mesh.vertices[i];
                        v.normal = normalised( v.normal );
                        v.tangent = normalised( v.tangent - v.normal * v.normal.dotProduct( v.tangent ),
                                                normalised( Vector3F( 0, 1, 0 ).crossProduct( v.normal ),
                                                            Vector3F( 1, 0, 0 ) ) );
                        v.tangentSign =
                            v.normal.crossProduct( v.tangent ).dotProduct( bitangents[i] ) < 0.0f ? -1.0f
                                                                                                  : 1.0f;
                    }
                }
            };

            struct BodyStation
            {
                float z;
                float halfWidth;
                float bottom;
                float top;
                float shoulder;
            };

            struct PodStation
            {
                float z;
                float centreX;
                float halfWidth;
                float bottom;
                float top;
            };

            template <class Station>
            Array<Station> refineStations( const Array<Station> &stations,
                                                 std::uint32_t steps,
                                                 std::initializer_list<float Station::*> fields )
            {
                if( steps <= 1 || stations.size() < 2 ) return stations;
                const auto slope = [&]( std::size_t i, float Station::*field ) {
                    const auto delta = [&]( std::size_t j ) {
                        return ( stations[j + 1].*field - stations[j].*field ) /
                               ( stations[j + 1].z - stations[j].z );
                    };
                    if( i == 0 ) return delta( 0 );
                    if( i + 1 == stations.size() ) return delta( i - 1 );
                    const float before = delta( i - 1 ), after = delta( i );
                    if( before * after <= 0 ) return 0.f;
                    const float h0 = stations[i].z - stations[i - 1].z;
                    const float h1 = stations[i + 1].z - stations[i].z;
                    const float w0 = 2 * h1 + h0, w1 = h1 + 2 * h0;
                    return ( w0 + w1 ) / ( w0 / before + w1 / after );
                };
                Array<Station> refined;
                refined.reserve( ( stations.size() - 1 ) * steps + 1 );
                for( std::size_t i = 0; i + 1 < stations.size(); ++i )
                {
                    const float distance = stations[i + 1].z - stations[i].z;
                    for( std::uint32_t step = 0; step < steps; ++step )
                    {
                        const float t = float( step ) / steps, t2 = t * t, t3 = t2 * t;
                        auto station = stations[i];
                        station.z += distance * t;
                        for( auto field : fields )
                        {
                            const float a = stations[i].*field, b = stations[i + 1].*field;
                            // Shape-preserving Hermite slopes keep widths/height extrema
                            // bounded while removing long planar reflection patches.
                            station.*field = clampF( ( 2 * t3 - 3 * t2 + 1 ) * a +
                                ( t3 - 2 * t2 + t ) * distance * slope( i, field ) +
                                ( -2 * t3 + 3 * t2 ) * b +
                                ( t3 - t2 ) * distance * slope( i + 1, field ),
                                std::min( a, b ), std::max( a, b ) );
                        }
                        refined.push_back( station );
                    }
                }
                refined.push_back( stations.back() );
                return refined;
            }

            void loftBody( Builder &b, const Array<BodyStation> &stations, std::uint32_t radial )
            {
                radial = std::max<std::uint32_t>( 8u, radial );
                const std::uint32_t cols = radial + 1u;
                for( std::size_t row = 0; row < stations.size(); ++row )
                {
                    const BodyStation &s = stations[row];
                    const float cy = 0.5f * ( s.bottom + s.top );
                    const float hh = 0.5f * ( s.top - s.bottom );
                    for( std::uint32_t col = 0; col <= radial; ++col )
                    {
                        const float u = static_cast<float>( col ) / radial;
                        const float a = u * kTwoPi;
                        const float ca = std::cos( a );
                        const float sa = std::sin( a );
                        // A superellipse creates a broad upper deck, while shoulder roll-off
                        // keeps reflections flowing into the undercut rather than pinching.
                        float x = std::copysign( std::pow( std::abs( ca ), 0.58f ), ca ) * s.halfWidth;
                        float y = cy + std::copysign( std::pow( std::abs( sa ), 0.72f ), sa ) * hh;
                        if( sa > 0.0f )
                            y -= s.shoulder * std::sin( kPi * sa ) *
                                 std::pow( std::abs( x ) / std::max( s.halfWidth, 0.01f ), 2.2f );
                        if( sa < 0.0f )
                            x *= 0.72f + 0.28f * ( 1.0f + sa );  // genuine lower-body undercut
                        const float grime = clampF( ( 0.34f - y ) * 1.8f, 0.0f, 1.0f );
                        b.vertex( Vector3F( x, y, s.z ),
                                  Vector2F( u, static_cast<float>( row ) / ( stations.size() - 1u ) ),
                                  Vector3F( 0.0f, grime, 0.15f * grime ) );
                    }
                }
                for( std::uint32_t row = 0; row + 1u < stations.size(); ++row )
                {
                    for( std::uint32_t col = 0; col < radial; ++col )
                    {
                        const std::uint32_t a = row * cols + col;
                        const std::uint32_t d = a + cols;
                        const float middleAngle = ( static_cast<float>( col ) + 0.5f ) / radial * kTwoPi;
                        if( stations[row].z >= -0.39f && stations[row + 1].z <= 0.77f &&
                            std::sin( middleAngle ) > 0.73f )
                            continue; // Open cockpit, never a painted roof over the driver.
                        // ring tangent x longitudinal row points out of the shell
                        b.triangle( a, a + 1u, d );
                        b.triangle( a + 1u, d + 1u, d );
                    }
                }
                // Correctly outward-facing caps, independent of ring winding.
                const std::uint32_t frontCentre =
                    b.vertex( Vector3F( 0, 0.5f * ( stations.front().bottom + stations.front().top ),
                                        stations.front().z ),
                              Vector2F( 0.5f, 0.5f ) );
                const std::uint32_t rearCentre =
                    b.vertex( Vector3F( 0, 0.5f * ( stations.back().bottom + stations.back().top ),
                                        stations.back().z ),
                              Vector2F( 0.5f, 0.5f ) );
                const std::uint32_t rearBase = static_cast<std::uint32_t>( stations.size() - 1u ) * cols;
                for( std::uint32_t col = 0; col < radial; ++col )
                {
                    b.triangle( frontCentre, col + 1u, col );
                    b.triangle( rearCentre, rearBase + col, rearBase + col + 1u );
                }
            }

            Vector3F podPoint( const PodStation &s, float u )
            {
                const float sa = std::sin( u * kTwoPi ), ca = std::cos( u * kTwoPi );
                float x = s.centreX + std::copysign( std::pow( std::abs( ca ), .62f ), ca ) * s.halfWidth;
                float y = .5f * ( s.bottom + s.top ) +
                    std::copysign( std::pow( std::abs( sa ), .72f ), sa ) * .5f * ( s.top - s.bottom );
                if( sa > 0 ) y -= std::min( .045f, ( s.top - s.bottom ) * .16f ) * std::sin( kPi * sa );
                if( sa < 0 ) x = s.centreX + ( x - s.centreX ) * ( .74f + .26f * ( 1 + sa ) );
                return { x, y, s.z };
            }

            void inletLip( Builder &paint, Builder &cavity, const PodStation &station,
                           std::uint32_t radial, float depth )
            {
                const auto innerPoint = [&]( std::uint32_t col, float z ) {
                    auto p = podPoint( station, float( col ) / radial );
                    p.x = station.centreX + ( p.x - station.centreX ) * .83f;
                    const float cy = .5f * ( station.bottom + station.top );
                    p.y = cy + ( p.y - cy ) * .72f;
                    p.z = z;
                    return p;
                };
                const float backZ = station.z + depth;
                const auto centre = cavity.vertex( { station.centreX,
                    .5f * ( station.bottom + station.top ), backZ }, { .5f, .5f } );
                for( std::uint32_t col = 0; col < radial; ++col )
                {
                    const auto a = podPoint( station, float( col ) / radial );
                    const auto b = podPoint( station, float( col + 1 ) / radial );
                    const auto ia = innerPoint( col, station.z ), ib = innerPoint( col + 1, station.z );
                    const auto ba = innerPoint( col, backZ ), bb = innerPoint( col + 1, backZ );
                    paint.quad( a, ia, ib, b );
                    cavity.quad( ia, ba, bb, ib );
                    const auto va = cavity.vertex( ba, { float( col ) / radial, 0 } );
                    const auto vb = cavity.vertex( bb, { float( col + 1 ) / radial, 1 } );
                    cavity.triangle( centre, vb, va );
                }
            }

            void loftPod( Builder &b, const Array<PodStation> &stations, std::uint32_t radial,
                          bool closeFront = true )
            {
                const auto base = static_cast<std::uint32_t>( b.mesh.vertices.size() );
                const std::uint32_t cols = radial + 1u;
                for( std::size_t row = 0; row < stations.size(); ++row )
                {
                    const PodStation &s = stations[row];
                    for( std::uint32_t col = 0; col <= radial; ++col )
                    {
                        const float u = static_cast<float>( col ) / radial;
                        const auto p = podPoint( s, u );
                        b.vertex( p,
                                  Vector2F( u, static_cast<float>( row ) / ( stations.size() - 1u ) ),
                                  Vector3F( 0, clampF( ( 0.30f - p.y ) * 2.0f, 0.0f, 1.0f ), 0.22f ) );
                    }
                }
                for( std::uint32_t row = 0; row + 1u < stations.size(); ++row )
                    for( std::uint32_t col = 0; col < radial; ++col )
                    {
                        const std::uint32_t a = base + row * cols + col;
                        const std::uint32_t d = a + cols;
                        b.triangle( a, a + 1u, d );
                        b.triangle( a + 1u, d + 1u, d );
                    }
                const auto front = closeFront ? b.vertex( Vector3F( stations.front().centreX,
                    ( stations.front().bottom + stations.front().top ) * 0.5f, stations.front().z ), Vector2F( 0.5f, 0.5f ) ) : 0u;
                const auto rear = b.vertex( Vector3F( stations.back().centreX,
                    ( stations.back().bottom + stations.back().top ) * 0.5f, stations.back().z ), Vector2F( 0.5f, 0.5f ) );
                const auto rearBase = base + static_cast<std::uint32_t>( stations.size() - 1 ) * cols;
                for( std::uint32_t col = 0; col < radial; ++col )
                {
                    if( closeFront ) b.triangle( front, base + col + 1, base + col );
                    b.triangle( rear, rearBase + col, rearBase + col + 1 );
                }
            }

            void addTyre( Builder &b, const Vector3F &centre, float width, float radius,
                          std::uint32_t around, std::uint32_t profile, bool rightSide, float wearScale )
            {
                const auto base = static_cast<std::uint32_t>( b.mesh.vertices.size() );
                const float rim = radius * 0.635f;
                const float halfWidth = width * 0.5f;
                const std::uint32_t cols = profile + 1u;
                for( std::uint32_t ring = 0; ring <= around; ++ring )
                {
                    const float u = static_cast<float>( ring ) / around;
                    const float a = u * kTwoPi;
                    for( std::uint32_t p = 0; p <= profile; ++p )
                    {
                        const float v = static_cast<float>( p ) / profile;
                        const float q = v * kPi;
                        // Broad slick tread with rounded shoulders, rather than a thin torus crown.
                        const float radial = rim + ( radius - rim ) *
                                                       std::pow( std::max( 0.0f, std::sin( q ) ), 0.32f );
                        const float sideBulge = std::sin( q );
                        const float axial =
                            ( v * 2.0f - 1.0f ) * halfWidth * ( 0.91f + 0.09f * sideBulge );
                        const float x = centre.x + axial;
                        const float y = centre.y + std::cos( a ) * radial;
                        const float z = centre.z + std::sin( a ) * radial;
                        const float treadWear = std::pow( std::sin( q ), 8.0f ) * 0.25f * wearScale;
                        b.vertex( Vector3F( x, y, z ),
                                  Vector2F( rightSide ? u : 1.0f - u, rightSide ? v : 1.0f - v ),
                                  Vector3F( treadWear, 0.15f, 0.25f ) );
                    }
                }
                for( std::uint32_t ring = 0; ring < around; ++ring )
                    for( std::uint32_t p = 0; p < profile; ++p )
                    {
                        const std::uint32_t a = base + ring * cols + p;
                        const std::uint32_t d = a + cols;
                        // Circumferential tangent x axial profile points outward.
                        b.triangle( a, d, a + 1u );
                        b.triangle( a + 1u, d, d + 1u );
                    }
            }

            void addDisc( Builder &b, const Vector3F &centre, float x, float radius,
                          std::uint32_t segments, bool reverse )
            {
                const std::uint32_t c =
                    b.vertex( Vector3F( x, centre.y, centre.z ), Vector2F( 0.5f, 0.5f ) );
                const std::uint32_t first = static_cast<std::uint32_t>( b.mesh.vertices.size() );
                for( std::uint32_t i = 0; i <= segments; ++i )
                {
                    const float a = static_cast<float>( i ) / segments * kTwoPi;
                    b.vertex( Vector3F( x, centre.y + std::cos( a ) * radius,
                                        centre.z + std::sin( a ) * radius ),
                              Vector2F( 0.5f + std::cos( a ) * 0.5f, 0.5f + std::sin( a ) * 0.5f ),
                              Vector3F( 0, 0.05f, 0.65f ) );
                }
                for( std::uint32_t i = 0; i < segments; ++i )
                {
                    if( reverse )
                        b.triangle( c, first + i + 1u, first + i );
                    else
                        b.triangle( c, first + i, first + i + 1u );
                }
            }

            void addRod( Builder &b, const Vector3F &a, const Vector3F &c, float radius,
                         std::uint32_t sides )
            {
                const Vector3F axis = normalised( c - a, Vector3F( 0, 0, 1 ) );
                const Vector3F seed =
                    std::abs( axis.y ) < 0.9f ? Vector3F( 0, 1, 0 ) : Vector3F( 1, 0, 0 );
                const Vector3F side = normalised( axis.crossProduct( seed ), Vector3F( 1, 0, 0 ) );
                const Vector3F up = normalised( side.crossProduct( axis ), Vector3F( 0, 1, 0 ) );
                const std::uint32_t base = static_cast<std::uint32_t>( b.mesh.vertices.size() );
                for( std::uint32_t end = 0; end < 2; ++end )
                    for( std::uint32_t i = 0; i <= sides; ++i )
                    {
                        const float u = static_cast<float>( i ) / sides;
                        const float angle = u * kTwoPi;
                        b.vertex( ( end ? c : a ) + side * ( std::cos( angle ) * radius ) +
                                      up * ( std::sin( angle ) * radius ),
                                  Vector2F( u, static_cast<float>( end ) ),
                                  Vector3F( 0, 0.08f, 0.35f ) );
                    }
                const std::uint32_t cols = sides + 1u;
                for( std::uint32_t i = 0; i < sides; ++i )
                {
                    b.triangle( base + i, base + cols + i, base + i + 1u );
                    b.triangle( base + i + 1u, base + cols + i, base + cols + i + 1u );
                }
            }

            void addRim( Builder &b, const Vector3F &centre, float width, float radius,
                         std::uint32_t segments, bool right, std::uint32_t level )
            {
                const float sign = right ? 1.0f : -1.0f;
                const float faceX = centre.x + sign * width * 0.47f;
                // A closed, recessed barrel with a raised outer lip. The open centre
                // exposes the spokes and brake instead of covering them with a disc.
                const std::array<Vector2F, 6> profile = {
                    Vector2F( -0.075f, 0.62f ), Vector2F( -0.012f, 0.64f ),
                    Vector2F( 0.006f, 0.625f ), Vector2F( 0.006f, 0.55f ),
                    Vector2F( -0.075f, 0.54f ), Vector2F( -0.08f, 0.60f )
                };
                auto point = [&]( std::size_t p, float angle ) {
                    return Vector3F( faceX + sign * profile[p].x,
                                     centre.y + radius * profile[p].y * std::cos( angle ),
                                     centre.z + radius * profile[p].y * std::sin( angle ) );
                };
                for( std::size_t p = 0; p < profile.size(); ++p )
                    for( std::uint32_t i = 0; i < segments; ++i )
                    {
                        const auto next = ( p + 1 ) % profile.size();
                        const float a = float( i ) / segments * kTwoPi;
                        const float c = float( i + 1 ) / segments * kTwoPi;
                        if( right )
                            b.quad( point( p, a ), point( p, c ), point( next, c ), point( next, a ) );
                        else
                            b.quad( point( p, c ), point( p, a ), point( next, a ), point( next, c ) );
                    }
                const std::uint32_t spokes = level == 0 ? 10u : 8u;
                const auto sides = level == 0 ? 8u : 6u;
                for( std::uint32_t i = 0; i < spokes; ++i )
                {
                    const float angle = float( i ) / spokes * kTwoPi;
                    auto spokePoint = [&]( float r, float offset, float recess ) {
                        return Vector3F( faceX - sign * recess,
                                         centre.y + radius * r * std::cos( angle + offset ),
                                         centre.z + radius * r * std::sin( angle + offset ) );
                    };
                    const auto root = spokePoint( .14f, 0, .018f );
                    addRod( b, root, spokePoint( .565f, .065f, .035f ), radius * .043f, sides );
                    if( level == 0 )
                        addRod( b, spokePoint( .32f, .02f, .025f ),
                                spokePoint( .565f, -.065f, .035f ), radius * .026f, sides );
                }
                addRod( b, Vector3F( faceX - sign * .065f, centre.y, centre.z ),
                        Vector3F( faceX + sign * .016f, centre.y, centre.z ), radius * .15f,
                        std::max<std::uint32_t>( 12u, segments / 2u ) );
                addDisc( b, centre, faceX + sign * .016f, radius * .15f,
                         std::max<std::uint32_t>( 12u, segments / 2u ), !right );
            }

            void addAirfoil( Builder &builder, float span, float chord, float height,
                             float z, float rise, std::uint32_t subdivisions )
            {
                const std::uint32_t rows = std::max<std::uint32_t>( 2u, subdivisions );
                const std::uint32_t spans = rows >= 12u ? 10u : ( rows >= 6u ? 6u : 4u );
                const std::uint32_t columns = spans + 1u;
                const bool front = z < 0.0f;
                const auto profile = [&]( float u, float sign, float x ) {
                    const float edge = std::abs( x ) / ( span * 0.5f );
                    const float edgeSquared = edge * edge;
                    const float localChord = chord * ( 1.0f - ( front ? 0.13f : 0.06f ) * edgeSquared );
                    const float thickness = localChord *
                        ( 0.003f + 0.10f * std::sqrt( std::max( 0.0f, u ) ) * ( 1.0f - u ) );
                    const float centreBlend = std::max( 0.0f, 1.0f - edgeSquared * 4.0f );
                    const float gull = ( front ? 0.04f : 0.012f ) * centreBlend * centreBlend;
                    const float twist = ( front ? 0.012f : 0.005f ) * edgeSquared * u;
                    const float camber = rise * u + localChord * 0.09f * std::sin( kPi * u );
                    const float sweep = 0.055f * edgeSquared;
                    return Vector3F( x, height + gull + twist + camber + sign * thickness,
                                     z + localChord * ( u - 0.5f ) + sweep );
                };
                // Share vertices within each smooth skin. The separate closures preserve
                // crisp perimeter normals without a hard shading seam at every chord strip.
                for( int skin = 0; skin < 2; ++skin )
                {
                    const float sign = skin == 0 ? 1.0f : -1.0f;
                    const auto base = static_cast<std::uint32_t>( builder.mesh.vertices.size() );
                    for( std::uint32_t row = 0; row <= rows; ++row )
                        for( std::uint32_t col = 0; col <= spans; ++col )
                        {
                            const float u = static_cast<float>( row ) / rows;
                            const float v = static_cast<float>( col ) / spans;
                            builder.vertex( profile( u, sign, span * ( v - 0.5f ) ), Vector2F( v, u ) );
                        }
                    for( std::uint32_t row = 0; row < rows; ++row )
                        for( std::uint32_t col = 0; col < spans; ++col )
                        {
                            const auto a = base + row * columns + col;
                            const auto d = a + columns;
                            if( skin == 0 )
                            {
                                builder.triangle( a, d, d + 1u );
                                builder.triangle( a, d + 1u, a + 1u );
                            }
                            else
                            {
                                builder.triangle( a, d + 1u, d );
                                builder.triangle( a, a + 1u, d + 1u );
                            }
                        }
                }
                const float left = -span * 0.5f, right = span * 0.5f;
                for( std::uint32_t row = 0; row < rows; ++row )
                {
                    const float u0 = static_cast<float>( row ) / rows;
                    const float u1 = static_cast<float>( row + 1u ) / rows;
                    builder.quad( profile( u0, -1, left ), profile( u1, -1, left ),
                                  profile( u1, 1, left ), profile( u0, 1, left ) );
                    builder.quad( profile( u0, 1, right ), profile( u1, 1, right ),
                                  profile( u1, -1, right ), profile( u0, -1, right ) );
                }
                for( std::uint32_t col = 0; col < spans; ++col )
                {
                    const float x0 = span * ( static_cast<float>( col ) / spans - 0.5f );
                    const float x1 = span * ( static_cast<float>( col + 1u ) / spans - 0.5f );
                    builder.quad( profile( 0, -1, x0 ), profile( 0, 1, x0 ),
                                  profile( 0, 1, x1 ), profile( 0, -1, x1 ) );
                    builder.quad( profile( 1, -1, x1 ), profile( 1, 1, x1 ),
                                  profile( 1, 1, x0 ), profile( 1, -1, x0 ) );
                }
            }

            VehicleLOD buildLOD( const VehicleGeometryConfig &cfg, std::uint32_t level )
            {
                const std::uint32_t bodyRadial = level == 0 ? 32u : ( level == 1 ? 18u : 10u );
                const std::uint32_t tyreAround = level == 0 ? 48u : ( level == 1 ? 28u : 16u );
                const std::uint32_t tyreProfile = level == 0 ? 20u : ( level == 1 ? 12u : 8u );
                const float frontZ = -cfg.wheelbase * 0.45f;
                const float rearZ = frontZ + cfg.wheelbase;

                VehicleLOD lod;
                lod.level = level;
                lod.suggestedScreenCoverage = level == 0 ? 0.32f : ( level == 1 ? 0.085f : 0.0f );

                Builder paint( "body_paint", VehicleMaterial::Paint );
                Builder carbonMatte( "aero_carbon_matte", VehicleMaterial::CarbonMatte );
                const auto loftSteps = level == 0 ? 6u : ( level == 1 ? 3u : 1u );
                const Array<BodyStation> stations = {
                    { -cfg.bodyLength * 0.54f, 0.075f, 0.21f, 0.30f, 0.00f },
                    { -cfg.bodyLength * 0.48f, 0.15f, 0.18f, 0.37f, 0.01f },
                    { frontZ - 0.28f, 0.22f, 0.15f, 0.46f, 0.025f },
                    { frontZ + 0.34f, 0.31f, 0.14f, 0.58f, 0.04f },
                    { -0.38f, 0.36f, 0.13f, 0.80f, 0.035f },
                    { 0.18f, 0.38f, 0.12f, 0.86f, 0.04f },
                    { 0.76f, 0.36f, 0.11f, 0.88f, 0.04f },
                    { rearZ - 0.30f, 0.32f, 0.12f, 0.71f, 0.04f },
                    { rearZ + 0.33f, 0.32f, 0.16f, 0.58f, 0.05f },
                    { std::max( cfg.bodyLength * 0.47f, rearZ + .55f ), 0.09f, 0.22f, 0.37f, 0.01f }
                };
                loftBody( paint, refineStations( stations, loftSteps,
                    { &BodyStation::halfWidth, &BodyStation::bottom, &BodyStation::top, &BodyStation::shoulder } ), bodyRadial );
                // Independent continuously lofted sidepods preserve the inlet lip,
                // downwash shoulder and rear coke-bottle taper in every LOD.
                for( int side : { -1, 1 } )
                {
                    const float sign = static_cast<float>( side );
                    const Array<PodStation> pods = { { 0.03f, sign * 0.59f, 0.205f, 0.51f, 0.65f },
                                                           { 0.26f, sign * 0.60f, 0.27f, 0.25f, 0.70f },
                                                           { 0.62f, sign * 0.55f, 0.30f, 0.13f, 0.66f },
                                                           { 1.03f, sign * 0.50f, 0.27f, 0.13f, 0.59f },
                                                           { 1.44f, sign * 0.39f, 0.19f, 0.14f,
                                                             0.52f } };
                    const auto podRadial = std::max<std::uint32_t>( 12u, bodyRadial * 3u / 4u );
                    loftPod( paint, refineStations( pods, loftSteps,
                        { &PodStation::centreX, &PodStation::halfWidth, &PodStation::bottom, &PodStation::top } ),
                        podRadial, false );
                    inletLip( paint, carbonMatte, pods.front(), podRadial, .16f );
                }
                const Array<PodStation> airbox = {
                    { 0.39f, 0.0f, 0.11f, 0.79f, 1.16f },
                    { 0.54f, 0.0f, 0.14f, 0.80f, 1.22f },
                    { 0.83f, 0.0f, 0.13f, 0.79f, 1.16f },
                    { .83f + ( rearZ - .17f - .83f ) * .65f, 0.0f, 0.07f, 0.65f, 0.99f },
                    { rearZ - 0.17f, 0.0f, 0.025f, 0.57f, 0.74f }
                };
                loftPod( paint, refineStations( airbox, loftSteps,
                    { &PodStation::centreX, &PodStation::halfWidth, &PodStation::bottom, &PodStation::top } ),
                    std::max<std::uint32_t>( 8u, bodyRadial / 2u ), false );
                inletLip( paint, carbonMatte, airbox.front(),
                    std::max<std::uint32_t>( 8u, bodyRadial / 2u ), .14f );
                paint.finalise();
                lod.sections.push_back( std::move( paint.mesh ) );

                carbonMatte.box( Vector3F( 0, 0.105f, 0.35f ), Vector3F( 1.86f, 0.075f, 3.34f ),
                                 Vector3F( 0.08f, 0.32f, 0.42f ) );
                const auto wingSteps = level == 0 ? 14u : ( level == 1 ? 8u : 4u );
                addAirfoil( carbonMatte, 1.82f, 0.40f, 0.17f, frontZ - 1.14f, 0.035f, wingSteps );
                addAirfoil( carbonMatte, 1.68f, 0.27f, 0.25f, frontZ - 0.98f, 0.065f, wingSteps );
                addAirfoil( carbonMatte, 1.07f, 0.36f, 0.78f, rearZ + 0.30f, 0.07f, wingSteps );
                addAirfoil( carbonMatte, 1.04f, 0.25f, 0.94f, rearZ + 0.42f, 0.07f, wingSteps );
                // Wing pylons and front endplates remain in all silhouette LODs.
                for( int side : { -1, 1 } )
                {
                    const float sign = static_cast<float>( side );
                    carbonMatte.box( Vector3F( sign * 0.90f, 0.24f, frontZ - 1.10f ), Vector3F( 0.027f, 0.24f, 0.48f ) );
                    addRod( carbonMatte, Vector3F( sign * 0.13f, 0.25f, frontZ - 0.83f ), Vector3F( sign * 0.16f, 0.20f, frontZ - 1.15f ), 0.024f, 6 );
                    addRod( carbonMatte, Vector3F( sign * 0.17f, 0.39f, rearZ + 0.08f ), Vector3F( sign * 0.22f, 0.83f, rearZ + 0.31f ), 0.025f, 6 );
                }
                Builder carbonGloss( "aero_carbon_gloss", VehicleMaterial::CarbonGloss );
                // Edge-on endplates retain the deep clearcoat response; broad sky-facing
                // planes stay matte so they do not collapse into uniform mirrored slabs.
                carbonGloss.box( Vector3F( -0.56f, 0.61f, rearZ + 0.35f ),
                                 Vector3F( 0.035f, 0.74f, 0.48f ) );
                carbonGloss.box( Vector3F( 0.56f, 0.61f, rearZ + 0.35f ),
                                 Vector3F( 0.035f, 0.74f, 0.48f ) );
                // The halo is primary silhouette geometry in every LOD.
                {
                    const std::uint32_t haloSides = level == 0 ? 10u : ( level == 1 ? 6u : 4u );
                    const std::uint32_t haloSteps = level == 0 ? 20u : ( level == 1 ? 12u : 8u );
                    addRod( carbonMatte, Vector3F( 0, 0.60f, -0.33f ), Vector3F( 0, 0.98f, -0.27f ),
                            0.026f, haloSides );
                    const auto base = static_cast<std::uint32_t>( carbonMatte.mesh.vertices.size() );
                    const auto columns = haloSides + 1;
                    for( std::uint32_t row = 0; row <= haloSteps; ++row )
                    {
                        const float t = float( row ) / haloSteps, angle = t * kPi;
                        const Vector3F centre( -.34f * std::cos( angle ), .82f + .16f * std::sin( angle ),
                                                .43f - .70f * std::sin( angle ) );
                        const auto axis = normalised( Vector3F( .34f * std::sin( angle ),
                            .16f * std::cos( angle ), -.70f * std::cos( angle ) ) );
                        const auto side = normalised( axis.crossProduct( Vector3F( 0, 1, 0 ) ) );
                        const auto up = side.crossProduct( axis );
                        for( std::uint32_t col = 0; col <= haloSides; ++col )
                        {
                            const float u = float( col ) / haloSides;
                            carbonMatte.vertex( centre + side * ( .028f * std::cos( u * kTwoPi ) ) +
                                up * ( .028f * std::sin( u * kTwoPi ) ), { u, t } );
                        }
                    }
                    for( std::uint32_t row = 0; row < haloSteps; ++row )
                        for( std::uint32_t col = 0; col < haloSides; ++col )
                        {
                            const auto a = base + row * columns + col, d = a + columns;
                            carbonMatte.triangle( a, d, a + 1 );
                            carbonMatte.triangle( a + 1, d, d + 1 );
                        }
                }
                carbonMatte.box( Vector3F( 0, 0.52f, 0.10f ), Vector3F( 0.45f, 0.035f, 0.85f ) );
                for( int side : { -1, 1 } )
                    carbonMatte.box( { side * .215f, .635f, .10f }, { .025f, .23f, .85f } );
                carbonMatte.box( Vector3F( 0, 0.80f, 0.43f ), Vector3F( 0.33f, 0.25f, 0.12f ) );
                for( int side : { -1, 1 } )
                    addRod( carbonMatte, Vector3F( side * 0.27f, 0.79f, -0.34f ),
                            Vector3F( side * 0.29f, 0.84f, 0.48f ), 0.035f, level == 0 ? 10u : 6u );
                carbonMatte.quad( Vector3F( -0.085f, 0.90f, 0.375f ), Vector3F( 0.085f, 0.90f, 0.375f ),
                                  Vector3F( 0.075f, 1.10f, 0.375f ), Vector3F( -0.075f, 1.10f, 0.375f ) );
                if( cfg.includeFineDetails && level < 2 )
                {
                    for( int side : { -1, 1 } )
                    {
                        addRod( carbonMatte, Vector3F( side * 0.30f, 0.73f, -0.28f ), Vector3F( side * 0.54f, 0.78f, -0.14f ), 0.014f, 6 );
                        carbonMatte.box( Vector3F( side * 0.55f, 0.78f, -0.12f ), Vector3F( 0.16f, 0.055f, 0.08f ) );
                    }
                    for( int fin = -2; fin <= 2; ++fin )
                        carbonMatte.box( Vector3F( fin * 0.25f, 0.16f, rearZ + 0.10f ), Vector3F( 0.018f, 0.19f, 0.76f ) );
                }
                // Diffuser kick and strakes make the rear silhouette credible at grazing angles.
                carbonMatte.quad(
                    Vector3F( -0.76f, 0.08f, rearZ - 0.30f ), Vector3F( 0.76f, 0.08f, rearZ - 0.30f ),
                    Vector3F( 0.66f, 0.31f, rearZ + 0.62f ), Vector3F( -0.66f, 0.31f, rearZ + 0.62f ),
                    Vector3F( 0.05f, 0.5f, 0.55f ) );
                carbonMatte.finalise();
                carbonGloss.finalise();
                lod.sections.push_back( std::move( carbonMatte.mesh ) );
                lod.sections.push_back( std::move( carbonGloss.mesh ) );

                Builder glass( "cockpit_glass", VehicleMaterial::Glass );
                glass.quad( Vector3F( -0.24f, 0.76f, -0.49f ), Vector3F( 0.24f, 0.76f, -0.49f ),
                            Vector3F( 0.22f, 0.87f, -0.30f ), Vector3F( -0.22f, 0.87f, -0.30f ),
                            Vector3F( 0, 0.03f, 0.38f ) );
                glass.finalise();
                lod.sections.push_back( std::move( glass.mesh ) );

                Builder rainLight( "rain_light", VehicleMaterial::RainLight );
                rainLight.box( Vector3F( 0, 0.43f, rearZ + 0.63f ), Vector3F( 0.13f, 0.055f, 0.025f ) );
                rainLight.finalise();
                lod.sections.push_back( std::move( rainLight.mesh ) );
                if( level < 2 && cfg.includeFineDetails )
                {
                    Builder headlights( "front_marker_lights", VehicleMaterial::Headlight );
                    headlights.box( Vector3F( -0.69f, 0.18f, frontZ - 1.25f ),
                                    Vector3F( 0.17f, 0.025f, 0.035f ) );
                    headlights.box( Vector3F( 0.69f, 0.18f, frontZ - 1.25f ),
                                    Vector3F( 0.17f, 0.025f, 0.035f ) );
                    headlights.finalise();
                    lod.sections.push_back( std::move( headlights.mesh ) );
                }

                Builder tyres( "tyres", VehicleMaterial::Tyre );
                Builder rims( "rims", VehicleMaterial::Rim );
                Builder brakes( "brakes", VehicleMaterial::Brake );
                Builder suspension( "suspension", VehicleMaterial::Suspension );
                const std::array<Vector3F, 4> centres = {
                    Vector3F( -cfg.frontTrack * 0.5f, cfg.tyreRadius, frontZ ),
                    Vector3F( cfg.frontTrack * 0.5f, cfg.tyreRadius, frontZ ),
                    Vector3F( -cfg.rearTrack * 0.5f, cfg.tyreRadius, rearZ ),
                    Vector3F( cfg.rearTrack * 0.5f, cfg.tyreRadius, rearZ )
                };
                for( std::size_t wheel = 0; wheel < centres.size(); ++wheel )
                {
                    const bool front = wheel < 2;
                    const bool right = ( wheel & 1u ) != 0u;
                    const float width = front ? cfg.frontTyreWidth : cfg.rearTyreWidth;
                    std::uint32_t wheelHash =
                        cfg.seed ^ ( 0x9E3779B9u * static_cast<std::uint32_t>( wheel + 1u ) );
                    wheelHash ^= wheelHash >> 16u;
                    wheelHash *= 0x7FEB352Du;
                    wheelHash ^= wheelHash >> 15u;
                    const float wearScale =
                        0.86f + static_cast<float>( wheelHash & 1023u ) / 1023.0f * 0.24f;
                    addTyre( tyres, centres[wheel], width, cfg.tyreRadius, tyreAround, tyreProfile,
                             right, wearScale );
                    addRim( rims, centres[wheel], width, cfg.tyreRadius, tyreAround, right, level );
                    if( level < 2 && cfg.includeFineDetails )
                        addDisc( brakes, centres[wheel], centres[wheel].x, cfg.tyreRadius * 0.48f,
                                 std::max<std::uint32_t>( 12u, tyreAround / 2u ), !right );

                    if( level < 2 && cfg.includeFineDetails )
                    {
                        const float sx = right ? 0.34f : -0.34f;
                        addRod( suspension, Vector3F( sx, 0.32f, centres[wheel].z - 0.24f ),
                                centres[wheel] +
                                    Vector3F( right ? -width * 0.45f : width * 0.45f, -0.12f, -0.16f ),
                                level == 0 ? 0.018f : 0.024f, level == 0 ? 8u : 6u );
                        addRod( suspension, Vector3F( sx, 0.48f, centres[wheel].z + 0.20f ),
                                centres[wheel] +
                                    Vector3F( right ? -width * 0.45f : width * 0.45f, 0.14f, 0.14f ),
                                level == 0 ? 0.016f : 0.022f, level == 0 ? 8u : 6u );
                    }
                }
                tyres.finalise();
                rims.finalise();
                if( brakes.mesh.hasGeometry() )
                    brakes.finalise();
                if( suspension.mesh.hasGeometry() )
                    suspension.finalise();
                lod.sections.push_back( std::move( tyres.mesh ) );
                lod.sections.push_back( std::move( rims.mesh ) );
                if( brakes.mesh.hasGeometry() )
                    lod.sections.push_back( std::move( brakes.mesh ) );
                if( suspension.mesh.hasGeometry() )
                    lod.sections.push_back( std::move( suspension.mesh ) );
                return lod;
            }
            VehicleLOD buildCoupeLOD( const VehicleGeometryConfig &cfg, std::uint32_t level )
            {
                // Retain the established tyre, rim and brake meshes and their LOD policy.
                auto lod = buildLOD( cfg, level );
                lod.sections.erase( std::remove_if( lod.sections.begin(), lod.sections.end(),
                    []( const VehicleMeshSection &section ) {
                        return section.material != VehicleMaterial::Tyre &&
                               section.material != VehicleMaterial::Rim &&
                               section.material != VehicleMaterial::Brake;
                    } ), lod.sections.end() );
                const float front = -cfg.wheelbase * .45f;
                const float rear = front + cfg.wheelbase;
                const float nose = front - ( cfg.bodyLength - cfg.wheelbase ) * .52f;
                const float tail = nose + cfg.bodyLength;
                const float half = cfg.bodyWidth * .5f;
                const float shoulder = std::max( .84f, cfg.tyreRadius * 2 + .12f );
                const bool sport = cfg.bodyStyle == VehicleBodyStyle::RoadSport;
                Builder paint( sport ? "sports_body_paint" : "gt_body_paint", VehicleMaterial::Paint );
                Builder glass( sport ? "sports_cabin_glass" : "gt_cabin_glass", VehicleMaterial::Glass );
                Builder aero( sport ? "sports_trim" : "gt_aero", VehicleMaterial::CarbonMatte );
                Builder headlights( sport ? "sports_headlights" : "gt_headlights", VehicleMaterial::Headlight );
                Builder taillights( sport ? "sports_tail_lights" : "gt_tail_lights", VehicleMaterial::TailLight );
                // Side panels follow the wheel openings rather than covering the tyres.
                const unsigned steps = level == 0 ? 96u : ( level == 1 ? 64u : 40u );
                auto lowerEdge = [&]( float z ) {
                    float y = .18f;
                    for( float axle : { front, rear } )
                    {
                        const float dz = z - axle;
                        const float radius = cfg.tyreRadius + .065f;
                        if( std::abs( dz ) < radius )
                            y = std::max( y, cfg.tyreRadius + std::sqrt( radius * radius - dz * dz ) );
                    }
                    return y;
                };
                for( unsigned i = 0; i < steps; ++i )
                {
                    const float z0 = nose + cfg.bodyLength * i / steps;
                    const float z1 = nose + cfg.bodyLength * ( i + 1 ) / steps;
                    for( int side : { -1, 1 } )
                    {
                        const float x = side * half;
                        const Vector3F a( x, lowerEdge( z0 ), z0 ), b( x, lowerEdge( z1 ), z1 );
                        const Vector3F c( x, shoulder, z1 ), d( x, shoulder, z0 );
                        if( side > 0 ) paint.quad( a, d, c, b );
                        else paint.quad( a, b, c, d );
                    }
                }
                // Hood and rear deck surround a separate glazed coupe cabin.
                paint.quad( { -half, shoulder, nose }, { -half, shoulder, -.65f },
                            { half, shoulder, -.65f }, { half, shoulder, nose } );
                paint.quad( { -half, shoulder, .95f }, { -half, shoulder, tail },
                            { half, shoulder, tail }, { half, shoulder, .95f } );
                paint.box( { 0, .44f, nose }, { cfg.bodyWidth, .52f, .10f } );
                paint.box( { 0, .44f, tail }, { cfg.bodyWidth, .52f, .10f } );
                const float roof = shoulder + ( sport ? .38f : .48f );
                const float cabin = half * .72f;
                paint.box( { 0, roof, .17f }, { cabin * 2, .055f, .94f } );
                glass.quad( { -cabin, shoulder, -.65f }, { -cabin, roof, -.30f },
                            { cabin, roof, -.30f }, { cabin, shoulder, -.65f } );
                glass.quad( { -cabin, roof, .64f }, { -cabin, shoulder, .95f },
                            { cabin, shoulder, .95f }, { cabin, roof, .64f } );
                for( int side : { -1, 1 } )
                {
                    const float x = side * cabin;
                    const Vector3F a( x, shoulder, -.65f ), b( x, shoulder, .95f );
                    const Vector3F c( x, roof, .64f ), d( x, roof, -.30f );
                    if( side > 0 ) glass.quad( a, d, c, b );
                    else glass.quad( a, b, c, d );
                    // Painted pillars and roof rails frame the glazing at every LOD.
                    const unsigned pillarSides = level == 0 ? 8u : 4u;
                    addRod( paint, a, d, .035f, pillarSides );
                    addRod( paint, b, c, .035f, pillarSides );
                    addRod( paint, c, d, .025f, pillarSides );
                    addRod( paint, { x, shoulder, .22f }, { x, roof, .22f }, .025f, pillarSides );
                    const float outer = side * half;
                    if( side > 0 ) paint.quad( { x, shoulder, -.65f }, { x, shoulder, .95f },
                        { outer, shoulder, .95f }, { outer, shoulder, -.65f } );
                    else paint.quad( { x, shoulder, -.65f }, { outer, shoulder, -.65f },
                        { outer, shoulder, .95f }, { x, shoulder, .95f } );
                    if( !sport )
                        aero.box( { side * .48f, shoulder + .20f, tail - .24f }, { .045f, .40f, .12f } );
                    headlights.box( { side * half * .70f, .64f, nose - .06f }, { sport ? .42f : .38f, sport ? .055f : .10f, .035f } );
                    taillights.box( { side * half * .70f, .64f, tail + .06f }, { .40f, .075f, .035f } );
                    if( cfg.includeFineDetails && level < 2 )
                        aero.box( { side * ( cabin + .14f ), shoulder + .25f, -.40f }, { .20f, .09f, .17f } );
                }
                aero.box( { 0, .38f, nose - .06f }, { cfg.bodyWidth * .58f, .22f, .035f } );
                for( int side : { -1, 1 } )
                    aero.box( { side * half, .18f, ( front + rear ) * .5f },
                              { .07f, .10f, cfg.wheelbase - 2 * cfg.tyreRadius - .14f } );
                if( cfg.includeFineDetails && level < 2 )
                    for( int fin = -2; fin <= 2; ++fin )
                        aero.box( { fin * .24f, .22f, tail - .20f }, { .018f, .16f, .52f } );
                aero.box( { 0, .14f, nose + .08f }, { cfg.bodyWidth + .08f, .045f, .44f } );
                aero.box( { 0, .16f, tail - .20f }, { cfg.bodyWidth * .85f, .06f, .52f } );
                if( sport )
                {
                    // A subtle painted lip replaces the GT racing wing and pylons.
                    paint.box( { 0, shoulder + .025f, tail - .06f },
                               { cfg.bodyWidth * .82f, .045f, .12f } );
                    if( cfg.includeFineDetails && level < 2 )
                        for( int side : { -1, 1 } )
                            aero.box( { side * .58f, .26f, tail + .07f }, { .14f, .09f, .16f } );
                }
                else
                    addAirfoil( aero, cfg.bodyWidth * .94f, .34f, shoulder + .43f,
                                tail - .24f, .03f, level == 0 ? 14u : 6u );
                for( Builder *builder : { &paint, &glass, &aero, &headlights, &taillights } )
                {
                    builder->finalise();
                    lod.sections.push_back( std::move( builder->mesh ) );
                }
                return lod;
            }
            VehicleLOD buildHelicopterLOD( const VehicleGeometryConfig &cfg, std::uint32_t level )
            {
                VehicleLOD lod;
                lod.level = level;
                lod.suggestedScreenCoverage = level == 0 ? .32f : ( level == 1 ? .085f : 0.0f );
                const float scale = cfg.bodyLength / 7.0f;
                const float width = cfg.bodyWidth / 1.5f;
                const unsigned around = level == 0 ? 48u : ( level == 1 ? 28u : 16u );
                const unsigned rows = around / 2;
                const unsigned rodSides = level == 0 ? 12u : ( level == 1 ? 8u : 6u );
                Builder hull( "helicopter_fuselage", VehicleMaterial::Paint );
                Builder glazing( "helicopter_bubble_cockpit", VehicleMaterial::Glass );
                Builder frame( "helicopter_cockpit_frame", VehicleMaterial::CarbonMatte );
                Builder gear( "helicopter_skids", VehicleMaterial::Suspension );
                Builder mainRotor( "helicopter_main_rotor", VehicleMaterial::CarbonMatte );
                Builder tailRotor( "helicopter_tail_rotor", VehicleMaterial::CarbonMatte );
                Builder lights( "helicopter_navigation_lights", VehicleMaterial::TailLight );
                // Partition one ellipsoid into painted shell and glazing, avoiding
                // overlapping glass/paint surfaces and zero-area polar quads.
                auto bubble = [&]( unsigned row, unsigned col ) {
                    const float latitude = -kPi * .5f + kPi * row / rows;
                    const float longitude = kTwoPi * col / around;
                    const float ring = std::cos( latitude );
                    return Vector3F( .75f * width * ring * std::cos( longitude ),
                                     ( 1.42f + .86f * std::sin( latitude ) ) * scale,
                                     ( -.90f + 1.65f * ring * std::sin( longitude ) ) * scale );
                };
                for( unsigned row = 0; row < rows; ++row )
                    for( unsigned col = 0; col < around; ++col )
                    {
                        const auto a = bubble( row, col ), b = bubble( row + 1, col );
                        const auto c = bubble( row + 1, col + 1 ), d = bubble( row, col + 1 );
                        const auto centre = ( a + b + c + d ) * .25f;
                        auto &builder = centre.z < -.48f * scale && centre.y > 1.05f * scale
                                            ? glazing : hull;
                        if( row == 0 || row + 1 == rows )
                        {
                            const auto base = static_cast<std::uint32_t>( builder.mesh.vertices.size() );
                            if( row == 0 )
                            {
                                builder.vertex( a, { .5f, 0 } );
                                builder.vertex( b, { 0, 1 } );
                                builder.vertex( c, { 1, 1 } );
                            }
                            else
                            {
                                builder.vertex( a, { 0, 0 } );
                                builder.vertex( b, { .5f, 1 } );
                                builder.vertex( d, { 1, 0 } );
                            }
                            builder.triangle( base, base + 1, base + 2 );
                        }
                        else builder.quad( a, b, c, d );
                    }
                // Curved centre windshield bar follows the same bubble surface.
                for( unsigned row = rows / 3; row + 1 < rows; ++row )
                    addRod( frame, bubble( row, around * 3 / 4 ),
                            bubble( row + 1, around * 3 / 4 ), .018f * scale, rodSides );
                addRod( hull, { 0, 1.45f * scale, .35f * scale },
                              { 0, 1.75f * scale, 2.8f * scale }, .14f * width, rodSides );
                addRod( hull, { 0, 1.75f * scale, 2.8f * scale },
                              { 0, 1.85f * scale, 4.3f * scale }, .085f * width, rodSides );
                // T tail, vertical fin and conventional exposed anti-torque rotor.
                hull.box( { 0, 2.18f * scale, 4.10f * scale },
                          { .07f * width, .85f * scale, .44f * scale } );
                hull.box( { 0, 2.52f * scale, 4.10f * scale },
                          { 1.35f * width, .055f * scale, .38f * scale } );
                for( int side : { -1, 1 } )
                {
                    const float x = side * .9f * width;
                    addRod( gear, { x, .12f * scale, -1.9f * scale },
                                  { x, .12f * scale, 1.15f * scale }, .045f * scale, rodSides );
                    addRod( gear, { x, .12f * scale, -1.9f * scale },
                                  { x, .30f * scale, -2.15f * scale }, .045f * scale, rodSides );
                    for( float z : { -1.25f, .05f } )
                        addRod( gear, { side * .42f * width, 1.05f * scale, z * scale },
                                      { x, .12f * scale, z * scale }, .038f * scale, rodSides );
                }
                const Vector3F hub( 0, 2.73f * scale, -.55f * scale );
                addRod( frame, { 0, 2.05f * scale, -.55f * scale }, hub, .065f * scale, rodSides );
                frame.box( hub, { .36f * scale, .10f * scale, .36f * scale } );
                for( unsigned blade = 0; blade < cfg.mainRotorBlades; ++blade )
                {
                    const float angle = kTwoPi * blade / cfg.mainRotorBlades + .18f;
                    const auto begin = mainRotor.mesh.vertices.size();
                    const float root = .22f;
                    mainRotor.box( hub + Vector3F( ( root + cfg.mainRotorRadius ) * .5f, 0, 0 ),
                                   { cfg.mainRotorRadius - root, .035f * scale, .20f * scale } );
                    for( auto i = begin; i < mainRotor.mesh.vertices.size(); ++i )
                    {
                        auto &position = mainRotor.mesh.vertices[i].position;
                        const auto local = position - hub;
                        position = hub + Vector3F( local.x * std::cos( angle ) - local.z * std::sin( angle ),
                                                   local.y,
                                                   local.x * std::sin( angle ) + local.z * std::cos( angle ) );
                    }
                }
                const Vector3F tailHub( -.29f * width, 1.85f * scale, 4.3f * scale );
                addRod( frame, { 0, tailHub.y, tailHub.z }, tailHub, .05f * scale, rodSides );
                for( unsigned blade = 0; blade < cfg.tailRotorBlades; ++blade )
                {
                    const float angle = kTwoPi * blade / cfg.tailRotorBlades + .35f;
                    const auto begin = tailRotor.mesh.vertices.size();
                    const float root = .08f;
                    tailRotor.box( tailHub + Vector3F( 0, ( root + cfg.tailRotorRadius ) * .5f, 0 ),
                                   { .025f * scale, cfg.tailRotorRadius - root, .10f * scale } );
                    for( auto i = begin; i < tailRotor.mesh.vertices.size(); ++i )
                    {
                        auto &position = tailRotor.mesh.vertices[i].position;
                        const auto local = position - tailHub;
                        position = tailHub + Vector3F( local.x,
                            local.y * std::cos( angle ) - local.z * std::sin( angle ),
                            local.y * std::sin( angle ) + local.z * std::cos( angle ) );
                    }
                }
                lights.box( { 0, 2.62f * scale, 4.1f * scale }, { .07f, .07f, .07f } );
                if( cfg.includeFineDetails && level < 2 )
                {
                    // Exhaust, cabin seats and aerial remain optional at distance.
                    addRod( frame, { .35f * width, 1.85f * scale, .30f * scale },
                                  { .50f * width, 1.90f * scale, .65f * scale }, .11f * scale, rodSides );
                    addRod( frame, { 0, .60f * scale, -.1f * scale },
                                  { 0, .28f * scale, -.1f * scale }, .012f * scale, 6 );
                    for( int side : { -1, 1 } )
                        frame.box( { side * .32f * width, 1.1f * scale, -.65f * scale },
                                   { .42f * width, .40f * scale, .38f * scale } );
                }
                for( Builder *builder : { &hull, &glazing, &frame, &gear, &mainRotor, &tailRotor, &lights } )
                {
                    builder->finalise();
                    lod.sections.push_back( std::move( builder->mesh ) );
                }
                return lod;
            }
            VehicleLOD buildWarbirdLOD( const VehicleGeometryConfig &cfg, std::uint32_t level )
            {
                VehicleLOD lod;
                lod.level = level;
                lod.suggestedScreenCoverage = level == 0 ? .32f : ( level == 1 ? .085f : 0.0f );
                const float scale = cfg.bodyLength / 9.8f;
                const float width = cfg.bodyWidth / 1.2f;
                const unsigned radial = level == 0 ? 40u : ( level == 1 ? 24u : 12u );
                Builder hull( "warbird_fuselage", VehicleMaterial::Paint );
                Builder wings( "warbird_wings", VehicleMaterial::Paint );
                Builder tail( "warbird_tail", VehicleMaterial::Paint );
                Builder canopy( "warbird_canopy", VehicleMaterial::Glass );
                Builder trim( "warbird_details", VehicleMaterial::CarbonMatte );
                Builder propeller( "warbird_propeller", VehicleMaterial::CarbonMatte );
                Builder gear( "warbird_landing_gear", VehicleMaterial::Suspension );
                Builder tyres( "warbird_tyres", VehicleMaterial::Tyre );
                struct Station { float z, halfWidth, y, height; };
                auto loft = [&]( Builder &builder, const Array<Station> &stations ) {
                    const auto base = static_cast<std::uint32_t>( builder.mesh.vertices.size() );
                    for( const auto &station : stations )
                        for( unsigned col = 0; col <= radial; ++col )
                        {
                            const float angle = kTwoPi * col / radial;
                            builder.vertex( { station.halfWidth * width * std::cos( angle ),
                                ( station.y + station.height * std::sin( angle ) ) * scale,
                                station.z * scale }, { float( col ) / radial,
                                ( station.z + 4.8f ) / 9.5f } );
                        }
                    const unsigned columns = radial + 1;
                    for( unsigned row = 0; row + 1 < stations.size(); ++row )
                        for( unsigned col = 0; col < radial; ++col )
                        {
                            const auto a = base + row * columns + col, d = a + columns;
                            builder.triangle( a, a + 1, d );
                            builder.triangle( a + 1, d + 1, d );
                        }
                    for( bool front : { true, false } )
                    {
                        const auto &station = front ? stations.front() : stations.back();
                        const auto centre = builder.vertex( { 0, station.y * scale, station.z * scale }, { .5f, .5f } );
                        const auto ring = front ? base : base + ( stations.size() - 1 ) * columns;
                        for( unsigned col = 0; col < radial; ++col )
                            if( front ) builder.triangle( centre, ring + col + 1, ring + col );
                            else builder.triangle( centre, ring + col, ring + col + 1 );
                    }
                };
                loft( hull, { { -4.0f, .40f, 1.90f, .43f }, { -3.5f, .52f, 1.9f, .55f },
                    { -2.1f, .60f, 1.9f, .64f }, { -.8f, .58f, 1.85f, .65f },
                    { .8f, .49f, 1.8f, .55f }, { 2.6f, .26f, 1.85f, .34f },
                    { 3.8f, .12f, 1.9f, .19f }, { 4.7f, .035f, 1.9f, .05f } } );
                loft( hull, { { -4.8f, .025f, 1.9f, .025f }, { -4.55f, .23f, 1.9f, .23f },
                             { -4.02f, .40f, 1.9f, .40f } } );
                loft( canopy, { { -.95f, .03f, 2.32f, .03f }, { -.60f, .38f, 2.48f, .38f },
                               { .15f, .43f, 2.49f, .47f }, { .85f, .32f, 2.36f, .33f },
                               { 1.35f, .03f, 2.23f, .03f } } );
                // Closed elliptical planform surfaces with a thin airfoil section.
                auto face = []( Builder &b, Vector3F a, Vector3F c, Vector3F d, Vector3F e, Vector3F outward ) {
                    if( ( c - a ).crossProduct( d - a ).dotProduct( outward ) < 0 )
                        b.quad( a, e, d, c );
                    else b.quad( a, c, d, e );
                };
                auto wing = [&]( Builder &builder, float span, float chord, float z, float y, float rise ) {
                    const unsigned spans = level == 0 ? 24u : ( level == 1 ? 14u : 8u );
                    const unsigned chords = level == 0 ? 12u : ( level == 1 ? 8u : 4u );
                    for( int side : { -1, 1 } )
                    {
                        auto point = [&]( unsigned row, unsigned col, int upper ) {
                            const float t = float( row ) / spans, q = float( col ) / chords;
                            const float localChord = chord * std::sqrt( std::max( .015f, 1 - t * t ) );
                            const float thickness = .005f + chord * .045f * std::sin( q * kPi ) * ( 1 - .65f * t );
                            return Vector3F( side * ( .20f * width + ( span * .5f - .20f * width ) * t ),
                                ( y + rise * t + upper * thickness ) * scale,
                                ( z + .32f * t + ( q - .45f ) * localChord ) * scale );
                        };
                        for( unsigned row = 0; row < spans; ++row )
                        {
                            for( unsigned col = 0; col < chords; ++col )
                                for( int upper : { -1, 1 } )
                                    face( builder, point( row, col, upper ), point( row + 1, col, upper ),
                                        point( row + 1, col + 1, upper ), point( row, col + 1, upper ), { 0, float( upper ), 0 } );
                            for( unsigned col : { 0u, chords } )
                                face( builder, point( row, col, -1 ), point( row + 1, col, -1 ),
                                    point( row + 1, col, 1 ), point( row, col, 1 ), { 0, 0, col == 0 ? -1.0f : 1.0f } );
                        }
                        for( unsigned row : { 0u, spans } )
                            for( unsigned col = 0; col < chords; ++col )
                                face( builder, point( row, col, -1 ), point( row, col + 1, -1 ),
                                    point( row, col + 1, 1 ), point( row, col, 1 ),
                                    { float( side ) * ( row == 0 ? -1.0f : 1.0f ), 0, 0 } );
                    }
                };
                wing( wings, cfg.wingSpan, 2.65f, -.55f, 1.37f, .36f );
                wing( tail, cfg.wingSpan * .32f, 1.25f, 3.65f, 1.92f, .06f );
                // Rounded vertical tail, using the same loft in a rotated local frame.
                Builder fin( "fin", VehicleMaterial::Paint );
                loft( fin, { { 0, .025f, 0, .025f }, { .35f, .07f, .04f, .62f },
                             { 1.05f, .065f, .16f, .54f }, { 1.55f, .025f, .30f, .10f } } );
                for( auto &vertex : fin.mesh.vertices )
                {
                    const auto p = vertex.position;
                    vertex.position = { p.x, 1.9f * scale + p.z, 3.75f * scale + p.y };
                }
                // Swapping the local vertical/longitudinal axes reverses handedness.
                for( std::size_t i = 0; i < fin.mesh.indices.size(); i += 3 )
                    std::swap( fin.mesh.indices[i + 1], fin.mesh.indices[i + 2] );
                const auto tailBase = static_cast<std::uint32_t>( tail.mesh.vertices.size() );
                tail.mesh.vertices.insert( tail.mesh.vertices.end(), fin.mesh.vertices.begin(), fin.mesh.vertices.end() );
                for( auto index : fin.mesh.indices ) tail.mesh.indices.push_back( tailBase + index );
                const Vector3F hub( 0, 1.9f * scale, -4.04f * scale );
                for( unsigned blade = 0; blade < cfg.propellerBlades; ++blade )
                {
                    const float angle = kTwoPi * blade / cfg.propellerBlades + .25f;
                    const auto begin = propeller.mesh.vertices.size();
                    propeller.box( hub + Vector3F( 0, ( .28f + cfg.propellerRadius ) * .5f, 0 ),
                                   { .20f * scale, cfg.propellerRadius - .28f, .045f * scale } );
                    for( auto i = begin; i < propeller.mesh.vertices.size(); ++i )
                    {
                        const auto local = propeller.mesh.vertices[i].position - hub;
                        propeller.mesh.vertices[i].position = hub + Vector3F(
                            local.x * std::cos( angle ) - local.y * std::sin( angle ),
                            local.x * std::sin( angle ) + local.y * std::cos( angle ), local.z );
                    }
                }
                if( cfg.landingGearDown )
                {
                    for( int side : { -1, 1 } )
                    {
                        const Vector3F centre( side * 1.35f * width, .32f * scale, -.70f * scale );
                        addRod( gear, { side * 1.6f * width, 1.4f * scale, -.55f * scale }, centre, .045f * scale, radial / 2 );
                        addTyre( tyres, centre, .20f * scale, .32f * scale, radial, radial / 2, side > 0, 1.0f );
                    }
                    const Vector3F centre( 0, 1.10f * scale, 3.95f * scale );
                    addRod( gear, { 0, 1.75f * scale, 3.80f * scale }, centre, .035f * scale, radial / 2 );
                    addTyre( tyres, centre, .12f * scale, .18f * scale, radial, radial / 2, false, 1.0f );
                }
                for( float z : { -.55f, .85f } )
                    for( unsigned i = 0; i < radial / 2; ++i )
                    {
                        const float a = kPi * i / ( radial / 2 ), b = kPi * ( i + 1 ) / ( radial / 2 );
                        const float radius = z < 0 ? .38f : .32f;
                        const float y = z < 0 ? 2.48f : 2.36f;
                        addRod( trim, { radius * width * std::cos( a ), ( y + radius * std::sin( a ) ) * scale, z * scale },
                                      { radius * width * std::cos( b ), ( y + radius * std::sin( b ) ) * scale, z * scale },
                                      .018f * scale, 6 );
                    }
                if( cfg.includeFineDetails && level < 2 )
                {
                    trim.box( { 0, 1.05f * scale, .45f * scale }, { .48f * width, .35f * scale, .85f * scale } );
                    for( int side : { -1, 1 } )
                        for( unsigned pipe = 0; pipe < 6; ++pipe )
                            addRod( trim, { side * .52f * width, 2.05f * scale, ( -3.25f + pipe * .18f ) * scale },
                                          { side * .68f * width, 2.0f * scale, ( -3.12f + pipe * .18f ) * scale }, .045f * scale, 6 );
                }
                for( Builder *builder : { &hull, &wings, &tail, &canopy, &trim, &propeller, &gear, &tyres } )
                    if( builder->mesh.hasGeometry() )
                    {
                        builder->finalise();
                        lod.sections.push_back( std::move( builder->mesh ) );
                    }
                return lod;
            }
        }  // namespace





        VehicleGeometry WPVehicleGeometry::generate( const VehicleGeometryConfig &input )
        {
            VehicleGeometryConfig cfg = input;
            cfg.wheelbase = clampF( cfg.wheelbase, 2.0f, 5.0f );
            cfg.frontTrack = clampF( cfg.frontTrack, 1.0f, 2.4f );
            cfg.rearTrack = clampF( cfg.rearTrack, 1.0f, 2.4f );
            cfg.tyreRadius = clampF( cfg.tyreRadius, 0.22f, 0.65f );
            cfg.frontTyreWidth = clampF( cfg.frontTyreWidth, 0.14f, 0.55f );
            cfg.rearTyreWidth = clampF( cfg.rearTyreWidth, 0.14f, 0.65f );
            const bool helicopter = cfg.bodyStyle == VehicleBodyStyle::HelicopterLittleBird;
            const bool plane = cfg.bodyStyle == VehicleBodyStyle::Warbird;
            const bool aircraft = helicopter || plane;
            cfg.bodyLength = aircraft ? clampF( cfg.bodyLength, 5.0f, 12.0f )
                                        : clampF( cfg.bodyLength, cfg.wheelbase + .7f, 7.0f );
            cfg.wingSpan = clampF( cfg.wingSpan, 6.0f, 18.0f );
            cfg.propellerRadius = clampF( cfg.propellerRadius, .6f, 2.0f );
            cfg.propellerBlades = std::clamp<std::uint32_t>( cfg.propellerBlades, 2, 5 );
            cfg.mainRotorRadius = clampF( cfg.mainRotorRadius, 1.5f, 8.0f );
            cfg.tailRotorRadius = clampF( cfg.tailRotorRadius, .25f, 1.5f );
            cfg.mainRotorBlades = std::clamp<std::uint32_t>( cfg.mainRotorBlades, 2, 8 );
            cfg.tailRotorBlades = std::clamp<std::uint32_t>( cfg.tailRotorBlades, 2, 6 );
            cfg.bodyWidth = clampF( cfg.bodyWidth, 1.2f, 2.4f );

            VehicleGeometry result;
            const std::uint32_t count = cfg.generateThreeLODs ? 3u : 1u;
            result.lods.reserve( count );
            for( std::uint32_t level = 0; level < count; ++level )
                result.lods.push_back( plane ? buildWarbirdLOD( cfg, level ) : helicopter ? buildHelicopterLOD( cfg, level )
                                       : cfg.bodyStyle != VehicleBodyStyle::GrandPrix
                                           ? buildCoupeLOD( cfg, level ) : buildLOD( cfg, level ) );

            // Bounds are authoritative mesh data, not nominal design dimensions.
            // Aero elements, tyre shoulders and optional details can extend beyond
            // bodyLength/bodyWidth, so reduce over every final LOD vertex only after
            // construction. A millimetre of padding absorbs float conversion and
            // downstream transform noise without concealing real containment errors.
            const float largest = std::numeric_limits<float>::max();
            Vector3F meshMin( largest, largest, largest );
            Vector3F meshMax( -largest, -largest, -largest );
            bool foundVertex = false;
            for( const auto &lod : result.lods )
                for( const auto &section : lod.sections )
                    for( const auto &vertex : section.vertices )
                    {
                        foundVertex = true;
                        meshMin.x = std::min( meshMin.x, vertex.position.x );
                        meshMin.y = std::min( meshMin.y, vertex.position.y );
                        meshMin.z = std::min( meshMin.z, vertex.position.z );
                        meshMax.x = std::max( meshMax.x, vertex.position.x );
                        meshMax.y = std::max( meshMax.y, vertex.position.y );
                        meshMax.z = std::max( meshMax.z, vertex.position.z );
                    }
            constexpr float boundsPadding = 0.001f;
            result.boundsMin = foundVertex
                                   ? meshMin - Vector3F( boundsPadding, boundsPadding, boundsPadding )
                                   : Vector3F();
            result.boundsMax = foundVertex
                                   ? meshMax + Vector3F( boundsPadding, boundsPadding, boundsPadding )
                                   : Vector3F();

            const float frontZ = -cfg.wheelbase * 0.45f;
            const float rearZ = frontZ + cfg.wheelbase;
            result.centreOfMass = Vector3F( 0, 0.31f, 0.18f );
            result.frontAxle = aircraft ? Vector3F() : Vector3F( 0, cfg.tyreRadius, frontZ );
            result.rearAxle = aircraft ? Vector3F() : Vector3F( 0, cfg.tyreRadius, rearZ );
            result.cockpitEye = Vector3F( 0, cfg.bodyStyle == VehicleBodyStyle::GrandPrix ? .84f :
                                                 ( cfg.bodyStyle == VehicleBodyStyle::GT ? 1.12f : 1.04f ), .10f );
            if( helicopter )
                result.cockpitEye = Vector3F( 0, 1.65f * cfg.bodyLength / 7.0f, -1.25f * cfg.bodyLength / 7.0f );
            if( plane )
                result.cockpitEye = Vector3F( 0, 2.65f * cfg.bodyLength / 9.8f, .15f * cfg.bodyLength / 9.8f );
            return result;
        }
    }  // namespace procedural
}  // namespace workphone
