#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPVehicleGeometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
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

                    std::vector<Vector3F> bitangents( mesh.vertices.size(), Vector3F() );
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

            void loftBody( Builder &b, const std::vector<BodyStation> &stations, std::uint32_t radial )
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
                            y -= s.shoulder *
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

            void loftPod( Builder &b, const std::vector<PodStation> &stations, std::uint32_t radial )
            {
                const std::uint32_t cols = radial + 1u;
                for( std::size_t row = 0; row < stations.size(); ++row )
                {
                    const PodStation &s = stations[row];
                    const float cy = 0.5f * ( s.bottom + s.top );
                    const float hh = 0.5f * ( s.top - s.bottom );
                    for( std::uint32_t col = 0; col <= radial; ++col )
                    {
                        const float u = static_cast<float>( col ) / radial;
                        const float a = u * kTwoPi;
                        const float sa = std::sin( a );
                        const float ca = std::cos( a );
                        float x = s.centreX + ca * s.halfWidth;
                        float y = cy + std::copysign( std::pow( std::abs( sa ), 0.72f ), sa ) * hh;
                        // Downwash shoulder and coke-bottle floor undercut.
                        if( sa > 0.0f )
                            y -= 0.07f * std::pow( std::abs( ca ), 2.0f );
                        if( sa < 0.0f )
                            x = s.centreX + ( x - s.centreX ) * ( 0.74f + 0.26f * ( 1.0f + sa ) );
                        b.vertex( Vector3F( x, y, s.z ),
                                  Vector2F( u, static_cast<float>( row ) / ( stations.size() - 1u ) ),
                                  Vector3F( 0, clampF( ( 0.30f - y ) * 2.0f, 0.0f, 1.0f ), 0.22f ) );
                    }
                }
                for( std::uint32_t row = 0; row + 1u < stations.size(); ++row )
                    for( std::uint32_t col = 0; col < radial; ++col )
                    {
                        const std::uint32_t a = row * cols + col;
                        const std::uint32_t d = a + cols;
                        b.triangle( a, a + 1u, d );
                        b.triangle( a + 1u, d + 1u, d );
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
                const std::vector<BodyStation> stations = {
                    { -cfg.bodyLength * 0.54f, 0.025f, 0.18f, 0.27f, 0.00f },
                    { -cfg.bodyLength * 0.48f, 0.13f, 0.16f, 0.36f, 0.01f },
                    { frontZ - 0.28f, 0.22f, 0.15f, 0.46f, 0.025f },
                    { frontZ + 0.34f, 0.31f, 0.14f, 0.58f, 0.04f },
                    { -0.38f, 0.49f, 0.13f, 0.80f, 0.09f },
                    { 0.18f, 0.57f, 0.12f, 0.91f, 0.13f },
                    { 0.76f, 0.69f, 0.11f, 0.82f, 0.15f },
                    { rearZ - 0.30f, 0.56f, 0.12f, 0.71f, 0.12f },
                    { rearZ + 0.33f, 0.32f, 0.16f, 0.58f, 0.05f },
                    { cfg.bodyLength * 0.47f, 0.09f, 0.22f, 0.37f, 0.01f }
                };
                loftBody( paint, stations, bodyRadial );
                // Independent continuously lofted sidepods preserve the inlet lip,
                // downwash shoulder and rear coke-bottle taper in every LOD.
                for( int side : { -1, 1 } )
                {
                    const float sign = static_cast<float>( side );
                    const std::vector<PodStation> pods = { { 0.06f, sign * 0.43f, 0.12f, 0.22f, 0.63f },
                                                           { 0.26f, sign * 0.52f, 0.26f, 0.15f, 0.70f },
                                                           { 0.62f, sign * 0.55f, 0.30f, 0.13f, 0.66f },
                                                           { 1.03f, sign * 0.50f, 0.27f, 0.13f, 0.59f },
                                                           { 1.44f, sign * 0.39f, 0.19f, 0.14f,
                                                             0.52f } };
                    loftPod( paint, pods, std::max<std::uint32_t>( 8u, bodyRadial / 2u ) );
                }
                paint.finalise();
                lod.sections.push_back( std::move( paint.mesh ) );

                Builder carbonMatte( "aero_carbon_matte", VehicleMaterial::CarbonMatte );
                for( int side : { -1, 1 } )
                {
                    const float cx = static_cast<float>( side ) * 0.43f;
                    carbonMatte.quad(
                        Vector3F( cx - 0.105f, 0.25f, 0.055f ), Vector3F( cx - 0.105f, 0.59f, 0.055f ),
                        Vector3F( cx + 0.105f, 0.59f, 0.055f ), Vector3F( cx + 0.105f, 0.25f, 0.055f ),
                        Vector3F( 0, 0.18f, 0.70f ) );
                }
                carbonMatte.box( Vector3F( 0, 0.105f, 0.35f ), Vector3F( 1.86f, 0.075f, 3.34f ),
                                 Vector3F( 0.08f, 0.32f, 0.42f ) );
                carbonMatte.box( Vector3F( 0, 0.18f, frontZ - 1.16f ),
                                 Vector3F( 1.82f, 0.055f, 0.38f ) );
                carbonMatte.box( Vector3F( 0, 0.28f, frontZ - 1.04f ),
                                 Vector3F( 1.66f, 0.045f, 0.31f ) );
                carbonMatte.box( Vector3F( 0, 0.79f, rearZ + 0.31f ), Vector3F( 1.05f, 0.07f, 0.34f ) );
                carbonMatte.box( Vector3F( 0, 0.94f, rearZ + 0.38f ), Vector3F( 1.03f, 0.055f, 0.27f ) );
                Builder carbonGloss( "aero_carbon_gloss", VehicleMaterial::CarbonGloss );
                // Edge-on endplates retain the deep clearcoat response; broad sky-facing
                // planes stay matte so they do not collapse into uniform mirrored slabs.
                carbonGloss.box( Vector3F( -0.56f, 0.61f, rearZ + 0.35f ),
                                 Vector3F( 0.035f, 0.74f, 0.48f ) );
                carbonGloss.box( Vector3F( 0.56f, 0.61f, rearZ + 0.35f ),
                                 Vector3F( 0.035f, 0.74f, 0.48f ) );
                if( level < 2 && cfg.includeFineDetails )
                {
                    const std::uint32_t haloSides = level == 0 ? 10u : 6u;
                    addRod( carbonMatte, Vector3F( 0, 0.60f, -0.33f ), Vector3F( 0, 0.98f, -0.22f ),
                            0.026f, haloSides );
                    addRod( carbonMatte, Vector3F( 0, 0.98f, -0.22f ), Vector3F( -0.34f, 0.82f, 0.43f ),
                            0.028f, haloSides );
                    addRod( carbonMatte, Vector3F( 0, 0.98f, -0.22f ), Vector3F( 0.34f, 0.82f, 0.43f ),
                            0.028f, haloSides );
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
                glass.quad( Vector3F( -0.285f, 0.79f, -0.50f ), Vector3F( 0.285f, 0.79f, -0.50f ),
                            Vector3F( 0.245f, 0.92f, 0.16f ), Vector3F( -0.245f, 0.92f, 0.16f ),
                            Vector3F( 0, 0.03f, 0.38f ) );
                glass.quad( Vector3F( -0.245f, 0.92f, 0.16f ), Vector3F( 0.245f, 0.92f, 0.16f ),
                            Vector3F( 0.16f, 0.82f, 0.52f ), Vector3F( -0.16f, 0.82f, 0.52f ),
                            Vector3F( 0, 0.04f, 0.45f ) );
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
            cfg.bodyLength = clampF( cfg.bodyLength, cfg.wheelbase + 0.7f, 7.0f );
            cfg.bodyWidth = clampF( cfg.bodyWidth, 1.2f, 2.4f );

            VehicleGeometry result;
            const std::uint32_t count = cfg.generateThreeLODs ? 3u : 1u;
            result.lods.reserve( count );
            for( std::uint32_t level = 0; level < count; ++level )
                result.lods.push_back( buildLOD( cfg, level ) );

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
            result.frontAxle = Vector3F( 0, cfg.tyreRadius, frontZ );
            result.rearAxle = Vector3F( 0, cfg.tyreRadius, rearZ );
            result.cockpitEye = Vector3F( 0, 0.84f, 0.10f );
            return result;
        }
    }  // namespace procedural
}  // namespace workphone
