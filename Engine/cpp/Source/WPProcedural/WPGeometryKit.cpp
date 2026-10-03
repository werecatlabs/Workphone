#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPGeometryKit.hpp"
#include "WPProcedural/WPNoise.hpp"
#include <cmath>
#include <algorithm>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            inline real_Num lerpR( real_Num a, real_Num b, real_Num t )
            {
                return a + t * ( b - a );
            }
        }  // namespace

        // ===================================================================
        // CHAMFERED BOX
        // ===================================================================
        ProceduralMesh WPGeometryKit::chamferedBox( real_Num w, real_Num h, real_Num d, real_Num bevel,
                                                    u32 /*subdivX*/, u32 /*subdivY*/, u32 /*subdivZ*/ )
        {
            ProceduralMesh mesh;
            real_Num hw = w * 0.5f, hh = h * 0.5f;

            auto face = [&]( Vector3<real_Num> normal, real_Num offset ) {
                Vector3<real_Num> up( 0, 1, 0 );
                Vector3<real_Num> right = normal.crossProduct( up );
                if( right.lengthSquared() < 0.001f )
                {
                    up = Vector3<real_Num>( 1, 0, 0 );
                    right = normal.crossProduct( up );
                }
                right.normalise();
                Vector3<real_Num> trueUp = right.crossProduct( normal );
                trueUp.normalise();

                Vector3<real_Num> origin = normal * offset;
                Vector3<real_Num> hwU = right * hw;
                Vector3<real_Num> hwV = trueUp * hh;

                ProceduralVertex v0;
                v0.position = origin + hwU + hwV;
                v0.normal = normal;
                ProceduralVertex v1;
                v1.position = origin - hwU + hwV;
                v1.normal = normal;
                ProceduralVertex v2;
                v1.position;  // dummy
                ProceduralVertex v2v;
                v2v.position = origin - hwU - hwV;
                v2v.normal = normal;
                ProceduralVertex v3;
                v3.position = origin + hwU - hwV;
                v3.normal = normal;

                v0.uv = Vector2<real_Num>( 1, 1 );
                v1.uv = Vector2<real_Num>( 0, 1 );
                v2v.uv = Vector2<real_Num>( 0, 0 );
                v3.uv = Vector2<real_Num>( 1, 0 );

                u32 i0 = mesh.addVertex( v0 );
                u32 i1 = mesh.addVertex( v1 );
                u32 i2 = mesh.addVertex( v2v );
                u32 i3 = mesh.addVertex( v3 );

                bool flip = ( normal.z < 0 ) || ( normal.x < 0 ) || ( normal.y < 0 && normal.z == 0 );
                if( flip )
                {
                    mesh.addTriangle( i0, i2, i1 );
                    mesh.addTriangle( i0, i3, i2 );
                }
                else
                {
                    mesh.addTriangle( i0, i1, i2 );
                    mesh.addTriangle( i0, i2, i3 );
                }
            };

            face( Vector3<real_Num>( 0, 0, 1 ), d * 0.5f );
            face( Vector3<real_Num>( 0, 0, -1 ), -d * 0.5f );
            face( Vector3<real_Num>( 1, 0, 0 ), w * 0.5f );
            face( Vector3<real_Num>( -1, 0, 0 ), -w * 0.5f );
            face( Vector3<real_Num>( 0, 1, 0 ), h * 0.5f );
            face( Vector3<real_Num>( 0, -1, 0 ), -h * 0.5f );

            // Apply a subtle warp to give the box a non-perfect surface
            if( bevel > 0.0f )
                warpMesh( mesh, bevel, 0.5f, 12345 );

            mesh.computeNormals();
            return mesh;
        }

        // ===================================================================
        // PLAIN BOX
        // ===================================================================
        ProceduralMesh WPGeometryKit::plainBox( real_Num w, real_Num h, real_Num d )
        {
            ProceduralMesh m;
            real_Num hw = w * 0.5f, hh = h * 0.5f, hd = d * 0.5f;
            Vector3<real_Num> p[8] = { { -hw, -hh, -hd }, { hw, -hh, -hd }, { hw, hh, -hd },
                                       { -hw, hh, -hd },  { -hw, -hh, hd }, { hw, -hh, hd },
                                       { hw, hh, hd },    { -hw, hh, hd } };
            auto quad = [&]( u32 a, u32 b, u32 c, u32 d, Vector3<real_Num> n ) {
                ProceduralVertex v0;
                v0.position = p[a];
                v0.normal = n;
                v0.uv = { 0, 0 };
                ProceduralVertex v1;
                v1.position = p[b];
                v1.normal = n;
                v1.uv = { 1, 0 };
                ProceduralVertex v2;
                v2.position = p[c];
                v2.normal = n;
                v2.uv = { 1, 1 };
                ProceduralVertex v3;
                v3.position = p[d];
                v3.normal = n;
                v3.uv = { 0, 1 };
                u32 i0 = m.addVertex( v0 );
                u32 i1 = m.addVertex( v1 );
                u32 i2 = m.addVertex( v2 );
                u32 i3 = m.addVertex( v3 );
                m.addTriangle( i0, i1, i2 );
                m.addTriangle( i0, i2, i3 );
            };
            quad( 4, 5, 6, 7, { 0, 0, 1 } );
            quad( 1, 0, 3, 2, { 0, 0, -1 } );
            quad( 5, 1, 2, 6, { 1, 0, 0 } );
            quad( 0, 4, 7, 3, { -1, 0, 0 } );
            quad( 7, 6, 2, 3, { 0, 1, 0 } );
            quad( 0, 1, 5, 4, { 0, -1, 0 } );
            return m;
        }

        // ===================================================================
        // QUAD
        // ===================================================================
        ProceduralMesh WPGeometryKit::quad( real_Num w, real_Num h )
        {
            ProceduralMesh m;
            real_Num hw = w * 0.5f, hh = h * 0.5f;
            ProceduralVertex v0;
            v0.position = { -hw, -hh, 0 };
            v0.normal = { 0, 0, 1 };
            v0.uv = { 0, 0 };
            ProceduralVertex v1;
            v1.position = { hw, -hh, 0 };
            v1.normal = { 0, 0, 1 };
            v1.uv = { 1, 0 };
            ProceduralVertex v2;
            v2.position = { hw, hh, 0 };
            v2.normal = { 0, 0, 1 };
            v2.uv = { 1, 1 };
            ProceduralVertex v3;
            v3.position = { -hw, hh, 0 };
            v3.normal = { 0, 0, 1 };
            v3.uv = { 0, 1 };
            u32 i0 = m.addVertex( v0 );
            u32 i1 = m.addVertex( v1 );
            u32 i2 = m.addVertex( v2 );
            u32 i3 = m.addVertex( v3 );
            m.addTriangle( i0, i1, i2 );
            m.addTriangle( i0, i2, i3 );
            return m;
        }

        // ===================================================================
        // SANDBAG - Lp-ball silhouette (NOT ellipsoid)
        // ===================================================================
        ProceduralMesh WPGeometryKit::sandbag( u32 seed, s32 variant, real_Num w, real_Num h, real_Num d,
                                               real_Num box, real_Num lump )
        {
            const u32 latSegments = 20;
            const u32 lonSegments = 24;
            ProceduralMesh mesh;
            WPNoise noise( seed );

            std::vector<Vector3<real_Num>> verts;
            verts.reserve( ( latSegments + 1 ) * ( lonSegments + 1 ) );
            for( u32 lat = 0; lat <= latSegments; ++lat )
            {
                real_Num theta = static_cast<real_Num>( lat ) * 3.14159265f / latSegments;
                real_Num sinT = std::sin( theta );
                real_Num cosT = std::cos( theta );
                for( u32 lon = 0; lon <= lonSegments; ++lon )
                {
                    real_Num phi = static_cast<real_Num>( lon ) * 2.0f * 3.14159265f / lonSegments;
                    real_Num sinP = std::sin( phi );
                    real_Num cosP = std::cos( phi );

                    real_Num ux = sinT * cosP;
                    real_Num uy = cosT;
                    real_Num uz = sinT * sinP;

                    real_Num sum = std::pow( std::abs( ux ), box ) + std::pow( std::abs( uy ), box ) +
                                   std::pow( std::abs( uz ), box );
                    real_Num r = sum > 1e-6f ? 1.0f / std::pow( sum, 1.0f / box ) : 1.0f;
                    ux *= r;
                    uy *= r;
                    uz *= r;

                    real_Num flat = uy > 0 ? 1.0f - uy * uy * 0.2f : 1.0f;

                    real_Num n =
                        noise.fbm3( ux * 3.4f + seed, uy * 3.4f + seed, uz * 3.4f + seed, 3 ) - 0.5f;
                    real_Num n2 =
                        noise.fbm3( ux * 9 + seed * 2, uy * 8 + seed, uz * 9 + seed * 3, 2 ) - 0.5f;

                    real_Num px = ux * w * 0.5f * ( 1.0f + n * 0.09f * lump );
                    real_Num pz = uz * d * 0.5f * flat * ( 1.0f + n * 0.26f * lump + n2 * 0.11f * lump );
                    real_Num py = uy * h * 0.5f * ( 1.0f + n * 0.24f * lump + n2 * 0.10f * lump );

                    real_Num t = px / ( w * 0.5f );
                    real_Num neck = std::max( 0.0f, std::abs( t ) - 0.7f ) / 0.3f;
                    pz *= 1.0f - neck * neck * 0.3f;
                    py *= 1.0f - neck * neck * 0.55f;
                    if( neck > 0.55f )
                        py += ( uy >= 0 ? 1.0f : -1.0f ) * h * 0.02f * ( neck - 0.55f ) * 2.0f;
                    if( uy > 0.15f )
                        py += h * 0.05f *
                              std::exp( -( pz / ( d * 0.42f ) ) * ( pz / ( d * 0.42f ) ) * 6.0f ) *
                              ( 1.0f - neck * 0.8f );

                    if( variant == 0 )
                    {
                        pz *= 1.0f + 0.06f * std::cos( t * 2.6f );
                    }
                    else if( variant == 1 )
                    {
                        px += w * 0.04f * t;
                        real_Num fatter = 1.0f + 0.13f * ( 0.5f - t );
                        pz *= fatter;
                        py *= fatter * ( 1.0f - 0.16f * std::exp( -( t / 0.32f ) * ( t / 0.32f ) ) );
                        if( uy > 0.3f )
                            py -= h * 0.05f * std::exp( -( t / 0.45f ) * ( t / 0.45f ) );
                    }
                    else
                    {
                        real_Num crease =
                            std::exp( -( ( t - 0.1f ) / 0.16f ) * ( ( t - 0.1f ) / 0.16f ) );
                        pz *= 1.0f - crease * 0.2f;
                        py *= 1.0f - crease * 0.26f;
                        if( t > 0.5f )
                        {
                            py *= 1.0f - ( t - 0.5f ) * 0.5f;
                            pz *= 1.0f + ( t - 0.5f ) * 0.22f;
                        }
                    }

                    verts.push_back( Vector3<real_Num>( px, py, pz ) );
                }
            }

            for( u32 lat = 0; lat < latSegments; ++lat )
            {
                for( u32 lon = 0; lon < lonSegments; ++lon )
                {
                    u32 a = lat * ( lonSegments + 1 ) + lon;
                    u32 b = a + 1;
                    u32 c = a + ( lonSegments + 1 );
                    u32 d = c + 1;
                    ProceduralVertex vA;
                    vA.position = verts[a];
                    ProceduralVertex vB;
                    vB.position = verts[b];
                    ProceduralVertex vC;
                    vC.position = verts[c];
                    ProceduralVertex vD;
                    vD.position = verts[d];
                    vA.uv =
                        Vector2<real_Num>( (real_Num)lon / lonSegments, (real_Num)lat / latSegments );
                    vB.uv = Vector2<real_Num>( (real_Num)( lon + 1 ) / lonSegments,
                                               (real_Num)lat / latSegments );
                    vC.uv = Vector2<real_Num>( (real_Num)lon / lonSegments,
                                               (real_Num)( lat + 1 ) / latSegments );
                    vD.uv = Vector2<real_Num>( (real_Num)( lon + 1 ) / lonSegments,
                                               (real_Num)( lat + 1 ) / latSegments );
                    u32 iA = mesh.addVertex( vA );
                    u32 iB = mesh.addVertex( vB );
                    u32 iC = mesh.addVertex( vC );
                    u32 iD = mesh.addVertex( vD );
                    mesh.addTriangle( iA, iB, iC );
                    mesh.addTriangle( iB, iD, iC );
                }
            }

            mesh.computeNormals();
            return mesh;
        }

        // ===================================================================
        // ROCK - noise-deformed faceted sphere
        // ===================================================================
        ProceduralMesh WPGeometryKit::rock( u32 seed, real_Num radius, u32 lod, real_Num lump )
        {
            const u32 segs = lod == 0 ? 12 : ( lod == 1 ? 8 : 6 );
            ProceduralMesh mesh;
            WPNoise noise( seed );

            std::vector<Vector3<real_Num>> verts;
            for( u32 lat = 0; lat <= segs; ++lat )
            {
                real_Num theta = static_cast<real_Num>( lat ) * 3.14159265f / segs;
                real_Num sinT = std::sin( theta );
                real_Num cosT = std::cos( theta );
                for( u32 lon = 0; lon <= segs; ++lon )
                {
                    real_Num phi = static_cast<real_Num>( lon ) * 2.0f * 3.14159265f / segs;
                    real_Num x = sinT * std::cos( phi );
                    real_Num y = cosT;
                    real_Num z = sinT * std::sin( phi );

                    real_Num n = noise.fbm3( x * 3 + seed, y * 3 + seed, z * 3 + seed, 3 ) - 0.5f;
                    real_Num r = radius * ( 1.0f + n * lump );

                    verts.push_back( Vector3<real_Num>( x * r, y * r, z * r ) );
                }
            }

            for( u32 lat = 0; lat < segs; ++lat )
            {
                for( u32 lon = 0; lon < segs; ++lon )
                {
                    u32 a = lat * ( segs + 1 ) + lon;
                    u32 b = a + 1;
                    u32 c = a + ( segs + 1 );
                    u32 d = c + 1;
                    ProceduralVertex vA;
                    vA.position = verts[a];
                    ProceduralVertex vB;
                    vB.position = verts[b];
                    ProceduralVertex vC;
                    vC.position = verts[c];
                    ProceduralVertex vD;
                    vD.position = verts[d];
                    u32 iA = mesh.addVertex( vA );
                    u32 iB = mesh.addVertex( vB );
                    u32 iC = mesh.addVertex( vC );
                    u32 iD = mesh.addVertex( vD );
                    mesh.addTriangle( iA, iB, iC );
                    mesh.addTriangle( iB, iD, iC );
                }
            }

            mesh.computeNormals();
            return mesh;
        }

        // ===================================================================
        // POLYPRISM
        // ===================================================================
        ProceduralMesh WPGeometryKit::polyPrism( const std::vector<Vector2<real_Num>> &polygon,
                                                 real_Num depth )
        {
            ProceduralMesh mesh;
            if( polygon.size() < 3 )
                return mesh;

            for( size_t i = 0; i < polygon.size(); ++i )
            {
                ProceduralVertex v;
                v.position = Vector3<real_Num>( polygon[i].x, polygon[i].y, depth );
                v.normal = Vector3<real_Num>( 0, 0, 1 );
                v.uv = Vector2<real_Num>( (real_Num)i / polygon.size(), 0 );
                (void)mesh.addVertex( v );
            }
            for( size_t i = 1; i + 1 < polygon.size(); ++i )
            {
                mesh.addTriangle( 0, static_cast<u32>( i ), static_cast<u32>( i + 1 ) );
            }

            u32 baseBottom = static_cast<u32>( mesh.vertices.size() );
            for( size_t i = 0; i < polygon.size(); ++i )
            {
                ProceduralVertex v;
                v.position = Vector3<real_Num>( polygon[i].x, polygon[i].y, 0 );
                v.normal = Vector3<real_Num>( 0, 0, -1 );
                v.uv = Vector2<real_Num>( (real_Num)i / polygon.size(), 0 );
                (void)mesh.addVertex( v );
            }
            for( size_t i = 1; i + 1 < polygon.size(); ++i )
            {
                mesh.addTriangle( baseBottom, baseBottom + static_cast<u32>( i + 1 ),
                                  baseBottom + static_cast<u32>( i ) );
            }

            for( size_t i = 0; i < polygon.size(); ++i )
            {
                size_t j = ( i + 1 ) % polygon.size();
                Vector2<real_Num> a = polygon[i];
                Vector2<real_Num> b = polygon[j];
                Vector2<real_Num> edge = ( b - a );
                Vector2<real_Num> n2( -edge.y, edge.x );
                real_Num l = std::sqrt( n2.x * n2.x + n2.y * n2.y );
                if( l > 1e-6f )
                    n2 = n2 / l;
                Vector3<real_Num> n3( n2.x, n2.y, 0 );

                ProceduralVertex v0;
                v0.position = Vector3<real_Num>( a.x, a.y, 0 );
                v0.normal = n3;
                ProceduralVertex v1;
                v1.position = Vector3<real_Num>( b.x, b.y, 0 );
                v1.normal = n3;
                ProceduralVertex v2;
                v2.position = Vector3<real_Num>( b.x, b.y, depth );
                v2.normal = n3;
                ProceduralVertex v3;
                v3.position = Vector3<real_Num>( a.x, a.y, depth );
                v3.normal = n3;

                u32 i0 = mesh.addVertex( v0 );
                u32 i1 = mesh.addVertex( v1 );
                u32 i2 = mesh.addVertex( v2 );
                u32 i3 = mesh.addVertex( v3 );
                mesh.addTriangle( i0, i1, i2 );
                mesh.addTriangle( i0, i2, i3 );
            }

            mesh.computeNormals();
            return mesh;
        }

        // ===================================================================
        // CYLINDER
        // ===================================================================
        ProceduralMesh WPGeometryKit::cylinder( real_Num radius, real_Num height, u32 radialSegments,
                                                u32 heightSegments, bool open, real_Num taper )
        {
            ProceduralMesh mesh;
            real_Num radiusTop = radius * taper;

            for( u32 y = 0; y <= heightSegments; ++y )
            {
                real_Num v = real_Num( y ) / heightSegments;
                real_Num r = lerpR( radius, radiusTop, v );
                real_Num py = v * height;
                for( u32 i = 0; i <= radialSegments; ++i )
                {
                    real_Num a = ( real_Num( i ) / radialSegments ) * 2.0f * 3.14159265f;
                    real_Num x = std::cos( a ) * r;
                    real_Num z = std::sin( a ) * r;
                    ProceduralVertex vx;
                    vx.position = Vector3<real_Num>( x, py, z );
                    vx.normal = Vector3<real_Num>( std::cos( a ), 0, std::sin( a ) );
                    vx.uv = Vector2<real_Num>( (real_Num)i / radialSegments, v );
                    (void)mesh.addVertex( vx );
                }
            }
            for( u32 y = 0; y < heightSegments; ++y )
            {
                for( u32 i = 0; i < radialSegments; ++i )
                {
                    u32 a = y * ( radialSegments + 1 ) + i;
                    u32 b = a + 1;
                    u32 c = a + ( radialSegments + 1 );
                    u32 d = c + 1;
                    mesh.addTriangle( a, b, c );
                    mesh.addTriangle( b, d, c );
                }
            }

            if( !open )
            {
                u32 centerBottom = mesh.addVertex(
                    ProceduralVertex{ { 0, 0, 0 }, { 0, -1, 0 }, { 0.5f, 0.5f }, { 0, 0, 0 } } );
                u32 centerTop = mesh.addVertex(
                    ProceduralVertex{ { 0, height, 0 }, { 0, 1, 0 }, { 0.5f, 0.5f }, { 0, 0, 0 } } );
                u32 baseBottom = static_cast<u32>( mesh.vertices.size() );
                u32 baseTop = baseBottom + radialSegments + 1;
                for( u32 i = 0; i <= radialSegments; ++i )
                {
                    ProceduralVertex v;
                    v.position = mesh.vertices[i].position;
                    v.position.y = 0;
                    v.normal = Vector3<real_Num>( 0, -1, 0 );
                    v.uv = Vector2<real_Num>( 0.5f, 0.5f );
                    (void)mesh.addVertex( v );
                }
                for( u32 i = 0; i <= radialSegments; ++i )
                {
                    ProceduralVertex v;
                    v.position = mesh.vertices[heightSegments * ( radialSegments + 1 ) + i].position;
                    v.position.y = height;
                    v.normal = Vector3<real_Num>( 0, 1, 0 );
                    v.uv = Vector2<real_Num>( 0.5f, 0.5f );
                    (void)mesh.addVertex( v );
                }

                for( u32 i = 0; i < radialSegments; ++i )
                {
                    mesh.addTriangle( centerBottom, baseBottom + i + 1, baseBottom + i );
                    mesh.addTriangle( centerTop, baseTop + i, baseTop + i + 1 );
                }
            }

            mesh.computeNormals();
            return mesh;
        }

        ProceduralMesh WPGeometryKit::tube( real_Num radius, real_Num height, u32 radial )
        {
            return cylinder( radius, height, radial, 1, true, 1.0f );
        }

        // ===================================================================
        // CLOTH
        // ===================================================================
        ProceduralMesh WPGeometryKit::cloth( real_Num w, real_Num h, const Vector3<real_Num> &from,
                                             const Vector3<real_Num> &to, real_Num sag, u32 segsX,
                                             u32 segsY, real_Num jitter, u32 seed )
        {
            ProceduralMesh mesh;
            WPNoise noise( seed );

            Vector3<real_Num> delta = to - from;
            real_Num dist = delta.length();
            Vector3<real_Num> dir = dist > 1e-6f ? delta / dist : Vector3<real_Num>( 1, 0, 0 );
            Vector3<real_Num> up( 0, 1, 0 );
            Vector3<real_Num> right = dir.crossProduct( up );
            if( right.lengthSquared() < 0.001f )
                right = Vector3<real_Num>( 1, 0, 0 );
            right.normalise();
            Vector3<real_Num> realUp = right.crossProduct( dir );
            realUp.normalise();

            for( u32 y = 0; y <= segsY; ++y )
            {
                real_Num fy = real_Num( y ) / segsY;
                for( u32 x = 0; x <= segsX; ++x )
                {
                    real_Num fx = real_Num( x ) / segsX;
                    Vector3<real_Num> base = from + delta * fx;
                    Vector3<real_Num> across = right * ( fx - 0.5f ) * w;
                    real_Num droop = sag * std::sin( fx * 3.14159265f );
                    Vector3<real_Num> pos = base + across + Vector3<real_Num>( 0, -droop - fy * h, 0 );

                    pos.x += jitter * ( noise.noise3( x * 0.5f, y * 0.5f, seed ) - 0.5f );
                    pos.z += jitter * ( noise.noise3( y * 0.5f, x * 0.5f, seed * 2 ) - 0.5f );

                    ProceduralVertex v;
                    v.position = pos;
                    v.normal = Vector3<real_Num>( 0, 1, 0 );
                    v.uv = Vector2<real_Num>( fx, fy );
                    (void)mesh.addVertex( v );
                }
            }

            for( u32 y = 0; y < segsY; ++y )
            {
                for( u32 x = 0; x < segsX; ++x )
                {
                    u32 a = y * ( segsX + 1 ) + x;
                    u32 b = a + 1;
                    u32 c = a + ( segsX + 1 );
                    u32 d = c + 1;
                    mesh.addTriangle( a, b, c );
                    mesh.addTriangle( b, d, c );
                }
            }

            mesh.computeNormals();
            return mesh;
        }

        // ===================================================================
        // STAIR RUN
        // ===================================================================
        ProceduralMesh WPGeometryKit::stairRun( real_Num width, s32 steps, real_Num rise, real_Num run,
                                                const std::string &sideRails )
        {
            ProceduralMesh mesh;
            if( steps < 1 )
                return mesh;
            real_Num stepRise = rise / steps;
            real_Num stepRun = run / steps;

            for( s32 i = 0; i < steps; ++i )
            {
                real_Num y = i * stepRise;
                real_Num z = i * stepRun;

                ProceduralMesh tread =
                    chamferedBox( width, stepRise * 0.5f, stepRun * 1.001f, 0.008f, 1, 1, 1 );
                for( auto &v : tread.vertices )
                    v.position += Vector3<real_Num>( 0, y + stepRise * 0.25f, z + stepRun * 0.5f );
                mesh.append( tread, Vector3<real_Num>( 0, 0, 0 ) );

                ProceduralMesh riser = chamferedBox( width, stepRise * 1.001f, 0.02f, 0.008f, 1, 1, 1 );
                for( auto &v : riser.vertices )
                    v.position += Vector3<real_Num>( 0, y + stepRise * 0.5f, z );
                mesh.append( riser, Vector3<real_Num>( 0, 0, 0 ) );

                if( sideRails == "right" || sideRails == "both" )
                {
                    ProceduralMesh rail = chamferedBox( 0.05f, rise + 1.0f, 0.05f, 0.005f, 1, 1, 1 );
                    for( auto &v : rail.vertices )
                        v.position += Vector3<real_Num>( width * 0.5f, rise * 0.5f, z + stepRun );
                    mesh.append( rail, Vector3<real_Num>( 0, 0, 0 ) );
                }
                if( sideRails == "left" || sideRails == "both" )
                {
                    ProceduralMesh rail = chamferedBox( 0.05f, rise + 1.0f, 0.05f, 0.005f, 1, 1, 1 );
                    for( auto &v : rail.vertices )
                        v.position += Vector3<real_Num>( -width * 0.5f, rise * 0.5f, z + stepRun );
                    mesh.append( rail, Vector3<real_Num>( 0, 0, 0 ) );
                }
            }

            return mesh;
        }

        // ===================================================================
        // PATCH
        // ===================================================================
        ProceduralMesh WPGeometryKit::patch( u32 seed, real_Num radius, u32 lobes, real_Num wobble )
        {
            std::vector<Vector2<real_Num>> polygon;
            WPNoise noise( seed );
            for( u32 i = 0; i < lobes; ++i )
            {
                real_Num a = ( real_Num( i ) / lobes ) * 2.0f * 3.14159265f;
                real_Num rr =
                    0.5f *
                    ( 1.0f +
                      wobble *
                          ( noise.noise3( std::cos( a ) * 4 + 9, std::sin( a ) * 4 + 3, 0 ) - 0.5f ) *
                          2.0f );
                polygon.push_back(
                    Vector2<real_Num>( std::cos( a ) * rr * radius, std::sin( a ) * rr * radius ) );
            }
            return polyPrism( polygon, 0.001f );
        }

        // ===================================================================
        // WARP MESH
        // ===================================================================
        void WPGeometryKit::warpMesh( ProceduralMesh &mesh, real_Num amp, real_Num freq, u32 seed )
        {
            WPNoise noise( seed );
            for( auto &v : mesh.vertices )
            {
                real_Num dx = ( noise.noise3( v.position.x * freq, v.position.y * freq,
                                              v.position.z * freq + seed ) -
                                0.5f ) *
                              2.0f;
                real_Num dy = ( noise.noise3( v.position.z * freq, v.position.x * freq,
                                              v.position.y * freq + seed ) -
                                0.5f ) *
                              2.0f;
                real_Num dz = ( noise.noise3( v.position.y * freq, v.position.z * freq,
                                              v.position.x * freq + seed ) -
                                0.5f ) *
                              2.0f;
                v.position = v.position + Vector3<real_Num>( dx, dy, dz ) * amp;
            }
            mesh.computeNormals();
        }
    }  // namespace procedural
}  // namespace workphone
