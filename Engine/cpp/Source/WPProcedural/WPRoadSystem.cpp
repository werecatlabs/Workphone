// ============================================================================
// WPRoadSystem.cpp - AAA Procedural Road System Implementation
// ============================================================================
// References the Claude-of-Duty ground.js for the AAA techniques:
//   - Cambered road surface (parabolic crown toward center)
//   - Wheel rut depressions where wheels have polished the surface
//   - Procedural asphalt variation (aggregate, wear patches, dust)
//   - Pavement slabs with gaps, broken corners, sand drifts
//   - Kerb stones sitting above the road, worn top edges
//   - Paving joints: darker grout lines every metre (reads as slabs)
//   - Seam scatter: pebbles and grit at material boundaries
//   - Gutter: wind-blown sand piled against kerbs
// ============================================================================
#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPRoadSystem.hpp"
#include "WPProcedural/WPNoise.hpp"
#include <cmath>
#include <algorithm>

namespace workphone
{
    namespace procedural
    {

        namespace
        {
            const real_Num PI = 3.14159265358979323846f;

            inline real_Num clamp01( real_Num v )
            {
                return v < 0.0f ? 0.0f : ( v > 1.0f ? 1.0f : v );
            }

            inline real_Num smoothstep( real_Num a, real_Num b, real_Num t )
            {
                t = clamp01( ( t - a ) / ( b - a ) );
                return t * t * ( 3.0f - 2.0f * t );
            }

            inline u8 toByte( real_Num v )
            {
                v = v < 0.0f ? 0.0f : ( v > 1.0f ? 1.0f : v );
                return static_cast<u8>( v * 255.0f + 0.5f );
            }
        }  // namespace

        // ===================================================================
        // WPRoadSystem - constructor / destructor
        // ===================================================================
        WPRoadSystem::WPRoadSystem( u32 seed ) : mSeed( seed )
        {
            mNoise = new WPNoise( seed );
        }

        WPRoadSystem::~WPRoadSystem()
        {
            delete mNoise;
        }

        void WPRoadSystem::setSeed( u32 seed )
        {
            mSeed = seed;
            delete mNoise;
            mNoise = new WPNoise( seed );
        }

        // ===================================================================
        // CLASS -> DIMENSION LOOKUPS
        // ===================================================================
        real_Num WPRoadSystem::getRoadWidth( RoadClass cls )
        {
            switch( cls )
            {
            case RoadClass::Highway:
                return 18.0f;
            case RoadClass::Arterial:
                return 12.0f;
            case RoadClass::Residential:
                return 8.0f;
            case RoadClass::Alley:
                return 5.0f;
            case RoadClass::Footway:
                return 3.0f;
            default:
                return 8.0f;
            }
        }

        real_Num WPRoadSystem::getSidewalkWidth( RoadClass cls )
        {
            switch( cls )
            {
            case RoadClass::Highway:
                return 0.0f;  // No sidewalks
            case RoadClass::Arterial:
                return 2.5f;
            case RoadClass::Residential:
                return 2.0f;
            case RoadClass::Alley:
                return 0.5f;
            case RoadClass::Footway:
                return 0.0f;
            default:
                return 2.0f;
            }
        }

        u32 WPRoadSystem::getLaneCount( RoadClass cls )
        {
            switch( cls )
            {
            case RoadClass::Highway:
                return 4;
            case RoadClass::Arterial:
                return 2;
            case RoadClass::Residential:
                return 2;
            case RoadClass::Alley:
                return 1;
            case RoadClass::Footway:
                return 0;
            default:
                return 2;
            }
        }

        RoadSurface WPRoadSystem::getDefaultSurface( RoadClass cls )
        {
            switch( cls )
            {
            case RoadClass::Highway:
                return RoadSurface::Asphalt;
            case RoadClass::Arterial:
                return RoadSurface::Asphalt;
            case RoadClass::Residential:
                return RoadSurface::Asphalt;
            case RoadClass::Alley:
                return RoadSurface::Dirt;
            case RoadClass::Footway:
                return RoadSurface::Concrete;
            default:
                return RoadSurface::Asphalt;
            }
        }

        // ===================================================================
        // GENERATE SEGMENT (top level)
        // ===================================================================
        RoadSegmentResult WPRoadSystem::generateSegment( const RoadSegmentSpec &spec )
        {
            RoadSegmentResult result;
            real_Num width = getRoadWidth( spec.roadClass );
            real_Num sidewalkW = getSidewalkWidth( spec.roadClass );
            Vector3<real_Num> delta = spec.end - spec.start;
            real_Num length = delta.length();
            if( length < 0.1f )
                return result;

            // Build the road surface
            result.roadSurface = buildRoadSurface( spec, width, length, 12, 32 );

            // Build sidewalks and kerbs
            if( spec.generateSidewalks && sidewalkW > 0.0f )
            {
                result.sidewalks[0] = buildSidewalk( spec, sidewalkW, length, true );
                result.sidewalks[1] = buildSidewalk( spec, sidewalkW, length, false );
            }
            if( spec.generateKerbs )
            {
                result.kerbs[0] = buildKerb( spec, 0.18f, length, true );
                result.kerbs[1] = buildKerb( spec, 0.18f, length, false );
            }

            // Build markings
            if( spec.generateMarkings )
                result.markings = buildMarkings( spec, width, length );

            // Build dressing
            if( spec.generateDressing )
                result.dressing = buildDressing( spec, width, length );

            // Build collision
            result.collision = buildCollision( spec, width, length );
            result.oldTarmac = buildOldTarmacPatches( spec, width, length );
            result.potholes = buildPotholes( spec, width, length );
            result.manholes = buildManholes( spec, width, length );
            result.gullyGrates = buildGullyGrates( spec, width, length );
            result.arrows = buildArrows( spec, width, length );
            result.pedXing = buildPedestrianCrossing( spec, width, length );

            // Orient everything along the road direction
            Vector3<real_Num> dir = delta / length;
            real_Num angle = std::atan2( dir.x, dir.z );  // Y rotation
            real_Num cs = std::cos( angle ), sn = std::sin( angle );

            auto orient = [cs, sn, &spec]( RoadMesh &m ) {
                for( auto &v : m.vertices )
                {
                    // Rotate from Z-aligned local to world direction
                    Vector3<real_Num> p( v.position.x * cs + v.position.z * sn, v.position.y,
                                         -v.position.x * sn + v.position.z * cs );
                    v.position = p + spec.start;
                }
            };

            orient( result.roadSurface );
            orient( result.sidewalks[0] );
            orient( result.sidewalks[1] );
            orient( result.kerbs[0] );
            orient( result.kerbs[1] );
            orient( result.markings );
            orient( result.dressing );
            orient( result.collision );

            // Recompute normals after transform
            result.roadSurface.computeNormals();
            result.sidewalks[0].computeNormals();
            result.sidewalks[1].computeNormals();
            result.kerbs[0].computeNormals();
            result.kerbs[1].computeNormals();
            result.markings.computeNormals();
            result.collision.computeNormals();

            return result;
        }

        // ===================================================================
        // BUILD ROAD SURFACE - the AAA core: camber + ruts + asphalt variation
        // ===================================================================
        // This is what separates a real road from a flat strip. The camber
        // (parabolic crown toward center) lets water drain to the edges. The
        // wheel ruts are where traffic has polished the surface. The asphalt
        // variation (aggregate bumps, wear patches, dust) is what makes it
        // look like tarmac instead of a shader.
        // ===================================================================
        RoadMesh WPRoadSystem::buildRoadSurface( const RoadSegmentSpec &spec, real_Num width,
                                                 real_Num length, u32 segments, u32 lengthSegs )
        {
            RoadMesh mesh;
            real_Num hw = width * 0.5f;

            // Per-vertex color variation for the asphalt PBR material
            for( u32 zi = 0; zi <= lengthSegs; ++zi )
            {
                real_Num v = real_Num( zi ) / lengthSegs;
                real_Num z = v * length;

                for( u32 xi = 0; xi <= segments; ++xi )
                {
                    real_Num u = real_Num( xi ) / segments;
                    real_Num x = ( u - 0.5f ) * width;

                    // --- Camber (parabolic crown toward center) ---
                    // Crown is highest at center, drops toward edges.
                    // Like real tarmac: (1 - (x/hw)^2) * crownHeight
                    real_Num normX = x / hw;
                    real_Num camber = ( 1.0f - normX * normX ) * 0.055f;

                    // --- Wheel ruts ---
                    // Two ruts where wheels have polished the surface.
                    // Real wheel tracks are ~1.6m from center on a residential road.
                    real_Num rutOffset = 1.6f;
                    real_Num rutL =
                        -std::exp( -( ( x + rutOffset ) * ( x + rutOffset ) ) / 0.5f ) * 0.022f;
                    real_Num rutR =
                        -std::exp( -( ( x - rutOffset ) * ( x - rutOffset ) ) / 0.5f ) * 0.022f;

                    // --- Wear variation ---
                    // FBM noise for surface irregularities (aggregate bumps)
                    real_Num wear =
                        ( mNoise->fbm3( x * 0.55f + 3.0f, 2.2f, z * 0.35f, 3 ) - 0.5f ) * 0.07f;

                    // --- Potholes (rare, on older roads) ---
                    real_Num pothole = 0.0f;
                    if( spec.surface == RoadSurface::Asphalt )
                    {
                        real_Num p = mNoise->worley2( x * 0.3f, z * 0.3f );
                        if( p < 0.08f )
                            pothole = -0.04f * ( 0.08f - p ) / 0.08f;
                    }

                    real_Num y = camber + rutL + rutR + wear + pothole;

                    // --- Color mask (PBR variation) ---
                    // r = wear (exposed substrate), g = grime, b = AO
                    real_Num baseNoise = mNoise->fbm3( x * 0.7f + 4.4f, 0, z * 0.7f + 4, 3 );
                    real_Num edgeWear = std::max( 0.0f, ( std::abs( normX ) - 0.7f ) / 0.3f ) * 0.4f;
                    real_Num dustAccum = std::max( 0.0f, std::abs( normX ) - 0.5f ) * 0.3f;

                    Vector3<real_Num> color(
                        edgeWear + baseNoise * 0.2f,           // r: wear
                        0.1f + dustAccum + baseNoise * 0.18f,  // g: grime
                        0.2f + std::abs( pothole ) * 5.0f      // b: AO (darker in potholes)
                    );

                    RoadVertex vertex;
                    vertex.position = Vector3<real_Num>( x, y, z );
                    vertex.normal = Vector3<real_Num>( 0, 1, 0 );  // recomputed
                    vertex.uv = Vector2<real_Num>( u * 4.0f, v * 8.0f );
                    vertex.color = color;
                    (void)mesh.addVertex( vertex );
                }
            }

            // Generate indices (quads between consecutive grid cells)
            for( u32 zi = 0; zi < lengthSegs; ++zi )
            {
                for( u32 xi = 0; xi < segments; ++xi )
                {
                    u32 a = zi * ( segments + 1 ) + xi;
                    u32 b = a + 1;
                    u32 c = a + ( segments + 1 );
                    u32 d = c + 1;
                    mesh.addTriangle( a, c, b );
                    mesh.addTriangle( b, c, d );
                }
            }

            mesh.computeNormals();
            return mesh;
        }

        // ===================================================================
        // BUILD SIDEWALK - individual slabs with gaps, broken corners
        // ===================================================================
        // Reference: "The pavements are individual slabs with gaps, broken
        // corners and sand drifts" - Claude-of-Duty ground.js
        // ===================================================================
        RoadMesh WPRoadSystem::buildSidewalk( const RoadSegmentSpec &spec, real_Num sidewalkW,
                                              real_Num length, bool isLeftSide )
        {
            RoadMesh mesh;
            real_Num sideSign = isLeftSide ? -1.0f : 1.0f;
            real_Num roadHalfW = getRoadWidth( spec.roadClass ) * 0.5f;
            real_Num sidewalkH = 0.15f;  // 15cm above road surface

            // Generate sidewalk as a series of slabs (not one continuous strip)
            // Slab length ~3-6m with small gaps between them
            real_Num z = 0.0f;
            u32 slabIndex = 0;
            while( z < length )
            {
                // Slab length varies (3.2 - 6.5m)
                real_Num slabLen =
                    3.2f + ( mNoise->noise3( slabIndex * 7.3f, 0, spec.seed ) * 0.5f + 0.5f ) * 3.3f;
                if( z + slabLen > length )
                    slabLen = length - z;

                // Gap between slabs (sometimes a larger gap for a driveway)
                real_Num gap = 0.06f;
                real_Num gapTest = mNoise->noise3( slabIndex * 11.1f, 0, spec.seed + 1 );
                if( gapTest < 0.12f )
                    gap = 0.6f + ( gapTest + 0.12f ) * 4.0f;  // driveway ramp gap

                // Slab height variation (some slabs are slightly sunken)
                real_Num slabH =
                    sidewalkH + ( mNoise->noise3( slabIndex * 3.7f, 0, spec.seed + 2 ) - 0.5f ) * 0.024f;

                // Build slab as a chamfered box
                real_Num slabX = roadHalfW + sidewalkW * 0.5f;
                real_Num slabZCenter = z + slabLen * 0.5f;

                // 4 corners
                Vector3<real_Num> p0( sideSign * ( slabX - sidewalkW * 0.5f ), 0,
                                      slabZCenter - slabLen * 0.5f );
                Vector3<real_Num> p1( sideSign * ( slabX + sidewalkW * 0.5f ), 0,
                                      slabZCenter - slabLen * 0.5f );
                Vector3<real_Num> p2( sideSign * ( slabX + sidewalkW * 0.5f ), 0,
                                      slabZCenter + slabLen * 0.5f );
                Vector3<real_Num> p3( sideSign * ( slabX - sidewalkW * 0.5f ), 0,
                                      slabZCenter + slabLen * 0.5f );

                // Color mask: broken corners get more wear
                real_Num cornerWear = 0.0f;
                real_Num cornerTest = mNoise->noise3( slabIndex * 5.5f, 0, spec.seed + 3 );
                if( cornerTest < 0.15f )
                    cornerWear = 0.8f;

                Vector3<real_Num> colorMask(
                    0.5f + cornerWear * 0.3f,                         // r: wear
                    0.4f + mNoise->noise3( slabIndex, 0, 4 ) * 0.2f,  // g: grime
                    0.15f                                             // b: AO
                );

                // Top face
                RoadVertex v0;
                v0.position = p0 + Vector3<real_Num>( 0, slabH, 0 );
                v0.normal = { 0, 1, 0 };
                v0.color = colorMask;
                RoadVertex v1;
                v1.position = p1 + Vector3<real_Num>( 0, slabH, 0 );
                v1.normal = { 0, 1, 0 };
                v1.color = colorMask;
                RoadVertex v2;
                v2.position = p2 + Vector3<real_Num>( 0, slabH, 0 );
                v2.normal = { 0, 1, 0 };
                v2.color = colorMask;
                RoadVertex v3;
                v3.position = p3 + Vector3<real_Num>( 0, slabH, 0 );
                v3.normal = { 0, 1, 0 };
                v3.color = colorMask;
                v0.uv = { 0, 0 };
                v1.uv = { 1, 0 };
                v2.uv = { 1, 1 };
                v3.uv = { 0, 1 };

                u32 i0 = mesh.addVertex( v0 );
                u32 i1 = mesh.addVertex( v1 );
                u32 i2 = mesh.addVertex( v2 );
                u32 i3 = mesh.addVertex( v3 );
                mesh.addTriangle( i0, i1, i2 );
                mesh.addTriangle( i0, i2, i3 );

                // Side faces (just front and back for simplicity)
                RoadVertex v4;
                v4.position = p0;
                v4.normal = { 0, -1, 0 };
                v4.color = colorMask;
                v4.uv = { 0, 0 };
                RoadVertex v5;
                v5.position = p1;
                v5.normal = { 0, -1, 0 };
                v5.color = colorMask;
                v5.uv = { 1, 0 };
                RoadVertex v6;
                v6.position = p2;
                v6.normal = { 0, -1, 0 };
                v6.color = colorMask;
                v6.uv = { 1, 1 };
                RoadVertex v7;
                v7.position = p3;
                v7.normal = { 0, -1, 0 };
                v7.color = colorMask;
                v7.uv = { 0, 1 };
                u32 i4 = mesh.addVertex( v4 );
                u32 i5 = mesh.addVertex( v5 );
                u32 i6 = mesh.addVertex( v6 );
                u32 i7 = mesh.addVertex( v7 );

                // Front face
                mesh.addTriangle( i4, i5, i0 );
                mesh.addTriangle( i5, i1, i0 );
                // Right face
                mesh.addTriangle( i5, i6, i1 );
                mesh.addTriangle( i6, i2, i1 );
                // Back face
                mesh.addTriangle( i6, i7, i2 );
                mesh.addTriangle( i7, i3, i2 );
                // Left face
                mesh.addTriangle( i7, i4, i3 );
                mesh.addTriangle( i4, i0, i3 );

                z += slabLen + gap;
                ++slabIndex;
            }

            mesh.computeNormals();
            return mesh;
        }

        // ===================================================================
        // BUILD KERB - stone kerb with worn top edge
        // ===================================================================
        RoadMesh WPRoadSystem::buildKerb( const RoadSegmentSpec &spec, real_Num kerbH, real_Num length,
                                          bool isLeftSide )
        {
            RoadMesh mesh;
            real_Num sideSign = isLeftSide ? -1.0f : 1.0f;
            real_Num roadHalfW = getRoadWidth( spec.roadClass ) * 0.5f;
            real_Num kerbW = 0.22f;
            real_Num kerbY = kerbH * 0.5f;

            // Kerb is a long thin box at the edge of the road
            real_Num x = sideSign * ( roadHalfW + kerbW * 0.5f );

            // Add some variation to kerb height (worn sections)
            u32 segs = static_cast<u32>( length / 4.0f ) + 1;
            real_Num segLen = length / segs;

            for( u32 i = 0; i < segs; ++i )
            {
                real_Num z0 = i * segLen;
                real_Num z1 = z0 + segLen;
                real_Num segH = kerbH + ( mNoise->noise3( i * 2.3f, 0, spec.seed ) - 0.5f ) * 0.04f;

                // 8 corners of the kerb segment
                Vector3<real_Num> p0( x - kerbW * 0.5f, 0, z0 );
                Vector3<real_Num> p1( x + kerbW * 0.5f, 0, z0 );
                Vector3<real_Num> p2( x + kerbW * 0.5f, 0, z1 );
                Vector3<real_Num> p3( x - kerbW * 0.5f, 0, z1 );
                Vector3<real_Num> p4( x - kerbW * 0.5f, segH, z0 );
                Vector3<real_Num> p5( x + kerbW * 0.5f, segH, z0 );
                Vector3<real_Num> p6( x + kerbW * 0.5f, segH, z1 );
                Vector3<real_Num> p7( x - kerbW * 0.5f, segH, z1 );

                Vector3<real_Num> mask( 0.9f, 0.35f, 0.1f );  // High wear on kerb top

                RoadVertex v[8];
                for( int j = 0; j < 8; ++j )
                {
                    v[j].color = mask;
                    v[j].uv = { 0, 0 };
                }
                v[0].position = p0;
                v[1].position = p1;
                v[2].position = p2;
                v[3].position = p3;
                v[4].position = p4;
                v[5].position = p5;
                v[6].position = p6;
                v[7].position = p7;
                v[4].normal = v[5].normal = v[6].normal = v[7].normal = { 0, 1, 0 };

                u32 idx[8];
                for( int j = 0; j < 8; ++j )
                    idx[j] = mesh.addVertex( v[j] );

                // Top face
                mesh.addTriangle( idx[4], idx[5], idx[6] );
                mesh.addTriangle( idx[4], idx[6], idx[7] );
                // Front face (road side)
                mesh.addTriangle( idx[1], idx[0], idx[4] );
                mesh.addTriangle( idx[1], idx[4], idx[5] );
                // Back face
                mesh.addTriangle( idx[3], idx[2], idx[6] );
                mesh.addTriangle( idx[3], idx[6], idx[7] );
                // End caps
                mesh.addTriangle( idx[0], idx[3], idx[7] );
                mesh.addTriangle( idx[0], idx[7], idx[4] );
                mesh.addTriangle( idx[2], idx[1], idx[5] );
                mesh.addTriangle( idx[2], idx[5], idx[6] );
            }

            mesh.computeNormals();
            return mesh;
        }

        // ===================================================================
        // BUILD MARKINGS - center dash, edge lines, crosswalks
        // ===================================================================
        RoadMesh WPRoadSystem::buildMarkings( const RoadSegmentSpec &spec, real_Num roadWidth,
                                              real_Num length )
        {
            RoadMesh mesh;
            real_Num hw = roadWidth * 0.5f;

            // White road marking material (high reflectance, low roughness)
            Vector3<real_Num> mask( 0.0f, 0.0f, 0.0f );  // No wear on markings

            if( spec.roadClass == RoadClass::Highway || spec.roadClass == RoadClass::Arterial )
            {
                // --- Center dashed line ---
                real_Num dashLen = 2.0f;
                real_Num gapLen = 3.0f;
                real_Num cycle = dashLen + gapLen;
                u32 numDashes = static_cast<u32>( length / cycle ) + 1;

                for( u32 i = 0; i < numDashes; ++i )
                {
                    real_Num z0 = i * cycle;
                    real_Num z1 = z0 + dashLen;
                    if( z1 > length )
                        break;

                    // 4 vertices for the dash quad
                    RoadVertex v0;
                    v0.position = { -0.075f, 0.061f, z0 };
                    v0.normal = { 0, 1, 0 };
                    v0.color = mask;
                    RoadVertex v1;
                    v1.position = { 0.075f, 0.061f, z0 };
                    v1.normal = { 0, 1, 0 };
                    v1.color = mask;
                    RoadVertex v2;
                    v2.position = { 0.075f, 0.061f, z1 };
                    v2.normal = { 0, 1, 0 };
                    v2.color = mask;
                    RoadVertex v3;
                    v3.position = { -0.075f, 0.061f, z1 };
                    v3.normal = { 0, 1, 0 };
                    v3.color = mask;
                    v0.uv = { 0, 0 };
                    v1.uv = { 1, 0 };
                    v2.uv = { 1, 1 };
                    v3.uv = { 0, 1 };
                    u32 i0 = mesh.addVertex( v0 );
                    u32 i1 = mesh.addVertex( v1 );
                    u32 i2 = mesh.addVertex( v2 );
                    u32 i3 = mesh.addVertex( v3 );
                    mesh.addTriangle( i0, i1, i2 );
                    mesh.addTriangle( i0, i2, i3 );
                }

                // --- Edge lines (solid) ---
                for( real_Num side : { -1.0f, 1.0f } )
                {
                    real_Num edgeX = side * ( hw - 0.3f );
                    RoadVertex v0;
                    v0.position = { edgeX - 0.1f, 0.061f, 0 };
                    v0.normal = { 0, 1, 0 };
                    v0.color = mask;
                    RoadVertex v1;
                    v1.position = { edgeX + 0.1f, 0.061f, 0 };
                    v1.normal = { 0, 1, 0 };
                    v1.color = mask;
                    RoadVertex v2;
                    v2.position = { edgeX + 0.1f, 0.061f, length };
                    v2.normal = { 0, 1, 0 };
                    v2.color = mask;
                    RoadVertex v3;
                    v3.position = { edgeX - 0.1f, 0.061f, length };
                    v3.normal = { 0, 1, 0 };
                    v3.color = mask;
                    v0.uv = { 0, 0 };
                    v1.uv = { 1, 0 };
                    v2.uv = { 1, 1 };
                    v3.uv = { 0, 1 };
                    u32 i0 = mesh.addVertex( v0 );
                    u32 i1 = mesh.addVertex( v1 );
                    u32 i2 = mesh.addVertex( v2 );
                    u32 i3 = mesh.addVertex( v3 );
                    mesh.addTriangle( i0, i1, i2 );
                    mesh.addTriangle( i0, i2, i3 );
                }
            }

            if( spec.roadClass == RoadClass::Residential )
            {
                // Just edge lines for residential (no center line)
                for( real_Num side : { -1.0f, 1.0f } )
                {
                    real_Num edgeX = side * ( hw - 0.25f );
                    RoadVertex v0;
                    v0.position = { edgeX - 0.08f, 0.061f, 0 };
                    v0.normal = { 0, 1, 0 };
                    v0.color = mask;
                    RoadVertex v1;
                    v1.position = { edgeX + 0.08f, 0.061f, 0 };
                    v1.normal = { 0, 1, 0 };
                    v1.color = mask;
                    RoadVertex v2;
                    v2.position = { edgeX + 0.08f, 0.061f, length };
                    v2.normal = { 0, 1, 0 };
                    v2.color = mask;
                    RoadVertex v3;
                    v3.position = { edgeX - 0.08f, 0.061f, length };
                    v3.normal = { 0, 1, 0 };
                    v3.color = mask;
                    v0.uv = { 0, 0 };
                    v1.uv = { 1, 0 };
                    v2.uv = { 1, 1 };
                    v3.uv = { 0, 1 };
                    u32 i0 = mesh.addVertex( v0 );
                    u32 i1 = mesh.addVertex( v1 );
                    u32 i2 = mesh.addVertex( v2 );
                    u32 i3 = mesh.addVertex( v3 );
                    mesh.addTriangle( i0, i1, i2 );
                    mesh.addTriangle( i0, i2, i3 );
                }
            }

            // --- Pedestrian crosswalk at both ends ---
            // (only for residential and arterial)
            if( spec.roadClass == RoadClass::Residential || spec.roadClass == RoadClass::Arterial )
            {
                for( real_Num zEnd : { 1.0f, length - 1.0f } )
                {
                    // 6 stripes across the road
                    u32 numStripes = 6;
                    real_Num stripeW = ( roadWidth - 1.0f ) / numStripes;
                    for( u32 i = 0; i < numStripes; ++i )
                    {
                        real_Num xCenter = -hw + 0.5f + ( i + 0.5f ) * stripeW;
                        RoadVertex v0;
                        v0.position = { xCenter - stripeW * 0.35f, 0.062f, zEnd - 0.75f };
                        v0.normal = { 0, 1, 0 };
                        v0.color = mask;
                        RoadVertex v1;
                        v1.position = { xCenter + stripeW * 0.35f, 0.062f, zEnd - 0.75f };
                        v1.normal = { 0, 1, 0 };
                        v1.color = mask;
                        RoadVertex v2;
                        v2.position = { xCenter + stripeW * 0.35f, 0.062f, zEnd + 0.75f };
                        v2.normal = { 0, 1, 0 };
                        v1.color = mask;
                        RoadVertex v3;
                        v3.position = { xCenter - stripeW * 0.35f, 0.062f, zEnd + 0.75f };
                        v3.normal = { 0, 1, 0 };
                        v3.color = mask;
                        v0.uv = { 0, 0 };
                        v1.uv = { 1, 0 };
                        v2.uv = { 1, 1 };
                        v3.uv = { 0, 1 };
                        u32 i0 = mesh.addVertex( v0 );
                        u32 i1 = mesh.addVertex( v1 );
                        u32 i2 = mesh.addVertex( v2 );
                        u32 i3 = mesh.addVertex( v3 );
                        mesh.addTriangle( i0, i1, i2 );
                        mesh.addTriangle( i0, i2, i3 );
                    }
                }
            }

            return mesh;
        }

        // ===================================================================
        // BUILD DRESSING - sand drifts, gutter debris, pebbles
        // ===================================================================
        // Reference: "Wind-blown sand and grit piles against the kerb on the
        // road side, which is what actually hides the value step where the
        // road surface meets the pavement" - Claude-of-Duty ground.js
        // ===================================================================
        RoadMesh WPRoadSystem::buildDressing( const RoadSegmentSpec &spec, real_Num roadWidth,
                                              real_Num length )
        {
            RoadMesh mesh;
            real_Num hw = roadWidth * 0.5f;
            u32 seedOffset = spec.seed * 7 + 13;

            // --- Sand drifts against both kerbs ---
            for( real_Num side : { -1.0f, 1.0f } )
            {
                u32 numDrifts = static_cast<u32>( length / 1.5f );
                for( u32 i = 0; i < numDrifts; ++i )
                {
                    real_Num z = ( real_Num( i ) / numDrifts ) * length +
                                 ( mNoise->noise3( i * 3.1f, 0, seedOffset ) * 0.5f + 0.5f ) * 1.5f;
                    if( z > length )
                        continue;

                    // Position: just inside the kerb
                    real_Num x = side * ( hw - 0.3f - mNoise->noise3( i, 0, seedOffset + 1 ) * 0.5f );
                    real_Num driftR =
                        0.35f + mNoise->noise3( i * 5.5f, 0, seedOffset + 2 ) * 0.5f + 0.4f;
                    real_Num driftH = 0.03f + mNoise->noise3( i * 2.7f, 0, seedOffset + 3 ) * 0.02f;

                    // Build a small flattened dome
                    u32 segs = 8;
                    u32 baseIdx = static_cast<u32>( mesh.vertices.size() );
                    for( u32 s = 0; s <= segs; ++s )
                    {
                        real_Num a = ( real_Num( s ) / segs ) * 2.0f * PI;
                        real_Num r =
                            driftR *
                            ( 0.7f + mNoise->noise3( std::cos( a ) * 3, 0, std::sin( a ) * 3 ) * 0.3f );
                        real_Num px = x + std::cos( a ) * r;
                        real_Num pz = z + std::sin( a ) * r;

                        RoadVertex v;
                        v.position = { px, driftH, pz };
                        v.normal = { 0, 1, 0 };
                        v.color = { 0.15f, 0.5f, 0.3f };  // Sand color mask
                        v.uv = { 0, 0 };
                        (void)mesh.addVertex( v );
                    }
                    // Center vertex
                    RoadVertex cv;
                    cv.position = { x, driftH, z };
                    cv.normal = { 0, 1, 0 };
                    cv.color = { 0.15f, 0.5f, 0.3f };
                    cv.uv = { 0.5f, 0.5f };
                    u32 centerIdx = mesh.addVertex( cv );

                    // Fan triangles
                    for( u32 s = 0; s < segs; ++s )
                    {
                        mesh.addTriangle( baseIdx + s, centerIdx, baseIdx + s + 1 );
                    }
                }
            }

            // --- Pebbles scattered along the gutters ---
            u32 numPebbles = static_cast<u32>( length * 2.0f );
            for( u32 i = 0; i < numPebbles; ++i )
            {
                real_Num z = ( mNoise->noise3( i * 7.7f, 0, seedOffset + 10 ) * 0.5f + 0.5f ) * length;
                real_Num side = ( mNoise->noise3( i * 3.3f, 0, seedOffset + 11 ) > 0.5f ) ? 1.0f : -1.0f;
                real_Num x = side * ( hw - 0.05f - mNoise->noise3( i, 0, seedOffset + 12 ) * 0.55f );
                real_Num r = 0.03f + mNoise->noise3( i * 11, 0, seedOffset + 13 ) * 0.03f;

                // Small octahedron for each pebble
                u32 baseIdx = static_cast<u32>( mesh.vertices.size() );
                Vector3<real_Num> positions[6] = {
                    { x, 0.012f, z + r }, { x + r, 0.012f, z },        { x, 0.012f, z - r },
                    { x - r, 0.012f, z }, { x, 0.012f + r * 0.7f, z }, { x, 0.012f - r * 0.3f, z }
                };
                for( int j = 0; j < 6; ++j )
                {
                    RoadVertex v;
                    v.position = positions[j];
                    v.normal = { 0, 1, 0 };
                    v.color = { 0.2f, 0.7f, 0.5f };  // Rock mask
                    v.uv = { 0, 0 };
                    (void)mesh.addVertex( v );
                }
                // 8 triangles for the octahedron
                mesh.addTriangle( baseIdx + 4, baseIdx + 0, baseIdx + 1 );
                mesh.addTriangle( baseIdx + 4, baseIdx + 1, baseIdx + 2 );
                mesh.addTriangle( baseIdx + 4, baseIdx + 2, baseIdx + 3 );
                mesh.addTriangle( baseIdx + 4, baseIdx + 3, baseIdx + 0 );
                mesh.addTriangle( baseIdx + 5, baseIdx + 1, baseIdx + 0 );
                mesh.addTriangle( baseIdx + 5, baseIdx + 2, baseIdx + 1 );
                mesh.addTriangle( baseIdx + 5, baseIdx + 3, baseIdx + 2 );
                mesh.addTriangle( baseIdx + 5, baseIdx + 0, baseIdx + 3 );
            }

            mesh.computeNormals();
            return mesh;
        }

        // ===================================================================
        // BUILD COLLISION - simplified flat boxes
        // ===================================================================
        // Reference: "Collision is a handful of flat boxes rather than the
        // visual triangles, which keeps the BVH tiny" - Claude-of-Duty ground.js
        // ===================================================================
        RoadMesh WPRoadSystem::buildCollision( const RoadSegmentSpec &spec, real_Num roadWidth,
                                               real_Num length )
        {
            RoadMesh mesh;
            real_Num hw = roadWidth * 0.5f;
            real_Num sidewalkW = getSidewalkWidth( spec.roadClass );

            // Road collision box (flat)
            RoadVertex v0;
            v0.position = { -hw, 0, 0 };
            v0.normal = { 0, 1, 0 };
            RoadVertex v1;
            v1.position = { hw, 0, 0 };
            v1.normal = { 0, 1, 0 };
            RoadVertex v2;
            v2.position = { hw, 0, length };
            v2.normal = { 0, 1, 0 };
            RoadVertex v3;
            v3.position = { -hw, 0, length };
            v3.normal = { 0, 1, 0 };
            u32 i0 = mesh.addVertex( v0 );
            u32 i1 = mesh.addVertex( v1 );
            u32 i2 = mesh.addVertex( v2 );
            u32 i3 = mesh.addVertex( v3 );
            mesh.addTriangle( i0, i1, i2 );
            mesh.addTriangle( i0, i2, i3 );

            // Sidewalk collision boxes
            if( sidewalkW > 0.0f )
            {
                for( real_Num side : { -1.0f, 1.0f } )
                {
                    real_Num x = side * ( hw + sidewalkW * 0.5f );
                    RoadVertex s0;
                    s0.position = { x - sidewalkW * 0.5f, 0.15f, 0 };
                    RoadVertex s1;
                    s1.position = { x + sidewalkW * 0.5f, 0.15f, 0 };
                    RoadVertex s2;
                    s2.position = { x + sidewalkW * 0.5f, 0.15f, length };
                    RoadVertex s3;
                    s3.position = { x - sidewalkW * 0.5f, 0.15f, length };
                    // (sidewalk top face vertices s0..s3 added below)
                    u32 j0 = mesh.addVertex( s0 );
                    u32 j1 = mesh.addVertex( s1 );
                    u32 j2 = mesh.addVertex( s2 );
                    u32 j3 = mesh.addVertex( s3 );
                    mesh.addTriangle( j0, j1, j2 );
                    mesh.addTriangle( j0, j2, j3 );
                }
            }

            return mesh;
        }

        // ===================================================================
        // GENERATE INTERSECTION (top level)
        // ===================================================================
        IntersectionResult WPRoadSystem::generateIntersection( const IntersectionSpec &spec )
        {
            switch( spec.type )
            {
            case IntersectionType::X_Crossing:
                return buildXCrossing( spec );
            case IntersectionType::T_Junction:
                return buildTJunction( spec );
            case IntersectionType::Roundabout:
                return buildRoundabout( spec );
            case IntersectionType::L_Corner:
                return buildLCorner( spec );
            default:
                return buildXCrossing( spec );
            }
        }

        // ===================================================================
        // BUILD X CROSSING (4-way intersection)
        // ===================================================================
        IntersectionResult WPRoadSystem::buildXCrossing( const IntersectionSpec &spec )
        {
            IntersectionResult result;
            real_Num width = getRoadWidth( spec.roadClass );
            real_Num hw = width * 0.5f;
            u32 segments = 16;

            // Build the intersection surface as a square patch
            for( u32 zi = 0; zi <= segments; ++zi )
            {
                for( u32 xi = 0; xi <= segments; ++xi )
                {
                    real_Num u = real_Num( xi ) / segments;
                    real_Num v = real_Num( zi ) / segments;
                    real_Num x = ( u - 0.5f ) * width;
                    real_Num z = ( v - 0.5f ) * width;

                    // Slight crown toward center for drainage
                    real_Num r = std::hypot( x, z ) / hw;
                    real_Num y = ( 1.0f - r * r * 0.5f ) * 0.04f;

                    // Asphalt variation
                    real_Num baseNoise = mNoise->fbm3( x * 0.7f, 0, z * 0.7f, 3 );
                    y += ( baseNoise - 0.5f ) * 0.02f;

                    Vector3<real_Num> mask( 0.3f, 0.1f + baseNoise * 0.2f, 0.2f );

                    RoadVertex vertex;
                    vertex.position = { x, y, z };
                    vertex.normal = { 0, 1, 0 };
                    vertex.uv = { u * 2, v * 2 };
                    vertex.color = mask;
                    (void)result.surface.addVertex( vertex );
                }
            }

            // Triangulate
            for( u32 zi = 0; zi < segments; ++zi )
            {
                for( u32 xi = 0; xi < segments; ++xi )
                {
                    u32 a = zi * ( segments + 1 ) + xi;
                    u32 b = a + 1;
                    u32 c = a + ( segments + 1 );
                    u32 d = c + 1;
                    result.surface.addTriangle( a, c, b );
                    result.surface.addTriangle( b, c, d );
                }
            }
            result.surface.computeNormals();

            // Crosswalk stripes on all 4 sides
            Vector3<real_Num> mask( 0, 0, 0 );
            for( u32 side = 0; side < 4; ++side )
            {
                real_Num angle = side * PI * 0.5f;
                real_Num cs = std::cos( angle ), sn = std::sin( angle );

                // 6 stripes per crosswalk
                u32 numStripes = 6;
                real_Num stripeW = ( width - 1.0f ) / numStripes;
                for( u32 i = 0; i < numStripes; ++i )
                {
                    real_Num along = -hw + 0.5f + ( i + 0.5f ) * stripeW;
                    real_Num across = hw - 0.75f;  // Just inside the intersection edge

                    // 4 corners of the stripe
                    real_Num sx0 = along - stripeW * 0.35f;
                    real_Num sx1 = along + stripeW * 0.35f;
                    real_Num sz0 = across - 0.4f;
                    real_Num sz1 = across + 0.4f;

                    // Rotate by the side angle
                    auto rot = [cs, sn]( real_Num x, real_Num z ) {
                        return Vector3<real_Num>( x * cs + z * sn, 0.062f, -x * sn + z * cs );
                    };

                    RoadVertex v0;
                    v0.position = rot( sx0, sz0 );
                    v0.normal = { 0, 1, 0 };
                    v0.color = mask;
                    RoadVertex v1;
                    v1.position = rot( sx1, sz0 );
                    v1.normal = { 0, 1, 0 };
                    v1.color = mask;
                    RoadVertex v2;
                    v2.position = rot( sx1, sz1 );
                    v2.normal = { 0, 1, 0 };
                    v2.color = mask;
                    RoadVertex v3;
                    v3.position = rot( sx0, sz1 );
                    v3.normal = { 0, 1, 0 };
                    v3.color = mask;
                    v0.uv = { 0, 0 };
                    v1.uv = { 1, 0 };
                    v2.uv = { 1, 1 };
                    v3.uv = { 0, 1 };
                    u32 i0 = result.markings.addVertex( v0 );
                    u32 i1 = result.markings.addVertex( v1 );
                    u32 i2 = result.markings.addVertex( v2 );
                    u32 i3 = result.markings.addVertex( v3 );
                    result.markings.addTriangle( i0, i1, i2 );
                    result.markings.addTriangle( i0, i2, i3 );
                }
            }

            // Offset to center
            for( auto &v : result.surface.vertices )
                v.position = v.position + spec.center;
            for( auto &v : result.markings.vertices )
                v.position = v.position + spec.center;

            return result;
        }

        // ===================================================================
        // BUILD T JUNCTION
        // ===================================================================
        IntersectionResult WPRoadSystem::buildTJunction( const IntersectionSpec &spec )
        {
            // A T-junction is an X-crossing with one approach missing.
            // For simplicity, build an X-crossing and the caller can handle
            // the missing road visually.
            return buildXCrossing( spec );
        }

        // ===================================================================
        // BUILD ROUNDABOUT
        // ===================================================================
        IntersectionResult WPRoadSystem::buildRoundabout( const IntersectionSpec &spec )
        {
            IntersectionResult result;
            real_Num r = spec.radius;
            u32 ringSegs = 32;
            real_Num ringW = 3.0f;  // Circular road width
            real_Num rOuter = r + ringW * 0.5f;
            real_Num rInner = r - ringW * 0.5f;

            // Build the ring road surface
            for( u32 i = 0; i < ringSegs; ++i )
            {
                real_Num a0 = ( real_Num( i ) / ringSegs ) * 2.0f * PI;
                real_Num a1 = ( real_Num( i + 1 ) / ringSegs ) * 2.0f * PI;

                // Outer and inner radii
                rOuter = r + ringW * 0.5f;
                rInner = r - ringW * 0.5f;

                // 4 corners of the ring segment
                RoadVertex v0;
                v0.position = { std::cos( a0 ) * rOuter, 0.04f, std::sin( a0 ) * rOuter };
                RoadVertex v1;
                v1.position = { std::cos( a1 ) * rOuter, 0.04f, std::sin( a1 ) * rOuter };
                RoadVertex v2;
                v2.position = { std::cos( a1 ) * rInner, 0.04f, std::sin( a1 ) * rInner };
                RoadVertex v3;
                v3.position = { std::cos( a0 ) * rInner, 0.04f, std::sin( a0 ) * rInner };

                for( int j = 0; j < 4; ++j )
                {
                    ( &v0 )[j].normal = { 0, 1, 0 };
                    ( &v0 )[j].color = { 0.3f, 0.15f, 0.2f };
                    ( &v0 )[j].uv = { 0, 0 };
                }

                u32 i0 = result.surface.addVertex( v0 );
                u32 i1 = result.surface.addVertex( v1 );
                u32 i2 = result.surface.addVertex( v2 );
                u32 i3 = result.surface.addVertex( v3 );
                result.surface.addTriangle( i0, i1, i2 );
                result.surface.addTriangle( i0, i2, i3 );
            }

            // Center island
            u32 islandSegs = 24;
            u32 islandBase = static_cast<u32>( result.centerIsland.vertices.size() );
            for( u32 i = 0; i < islandSegs; ++i )
            {
                real_Num a = ( real_Num( i ) / islandSegs ) * 2.0f * PI;
                real_Num ir = rInner * 0.95f;
                RoadVertex v;
                v.position = { std::cos( a ) * ir, 0.1f, std::sin( a ) * ir };
                v.normal = { 0, 1, 0 };
                v.color = { 0.4f, 0.5f, 0.3f };  // Foliage-ish
                v.uv = { std::cos( a ), std::sin( a ) };
                (void)result.centerIsland.addVertex( v );
            }
            // Center vertex
            RoadVertex cv;
            cv.position = { 0, 0.1f, 0 };
            cv.normal = { 0, 1, 0 };
            cv.color = { 0.4f, 0.5f, 0.3f };
            cv.uv = { 0.5f, 0.5f };
            u32 centerIdx = result.centerIsland.addVertex( cv );

            for( u32 i = 0; i < islandSegs; ++i )
            {
                result.centerIsland.addTriangle( islandBase + i, centerIdx,
                                                 islandBase + ( i + 1 ) % islandSegs );
            }

            // Offset to center
            for( auto &v : result.surface.vertices )
                v.position = v.position + spec.center;
            for( auto &v : result.centerIsland.vertices )
                v.position = v.position + spec.center;

            result.surface.computeNormals();
            result.centerIsland.computeNormals();

            return result;
        }

        // ===================================================================
        // BUILD L CORNER (bend in road)
        // ===================================================================
        IntersectionResult WPRoadSystem::buildLCorner( const IntersectionSpec &spec )
        {
            // An L-corner is a bend. Build a quarter-arc that smoothly connects
            // two perpendicular road directions.
            IntersectionResult result;
            real_Num width = getRoadWidth( spec.roadClass );
            real_Num hw = width * 0.5f;
            real_Num radius = 8.0f;
            u32 arcSegs = 16;

            for( u32 i = 0; i < arcSegs; ++i )
            {
                real_Num a0 = ( real_Num( i ) / arcSegs ) * PI * 0.5f;
                real_Num a1 = ( real_Num( i + 1 ) / arcSegs ) * PI * 0.5f;

                real_Num rOuter = radius + hw;
                real_Num rInner = radius - hw;

                RoadVertex v0;
                v0.position = { std::cos( a0 ) * rOuter, 0.05f, std::sin( a0 ) * rOuter };
                RoadVertex v1;
                v1.position = { std::cos( a1 ) * rOuter, 0.05f, std::sin( a1 ) * rOuter };
                RoadVertex v2;
                v2.position = { std::cos( a1 ) * rInner, 0.05f, std::sin( a1 ) * rInner };
                RoadVertex v3;
                v3.position = { std::cos( a0 ) * rInner, 0.05f, std::sin( a0 ) * rInner };

                for( int j = 0; j < 4; ++j )
                {
                    ( &v0 )[j].normal = { 0, 1, 0 };
                    ( &v0 )[j].color = { 0.3f, 0.15f, 0.2f };
                    ( &v0 )[j].uv = { 0, 0 };
                }

                u32 i0 = result.surface.addVertex( v0 );
                u32 i1 = result.surface.addVertex( v1 );
                u32 i2 = result.surface.addVertex( v2 );
                u32 i3 = result.surface.addVertex( v3 );
                result.surface.addTriangle( i0, i1, i2 );
                result.surface.addTriangle( i0, i2, i3 );
            }

            for( auto &v : result.surface.vertices )
                v.position = v.position + spec.center;

            result.surface.computeNormals();
            return result;
        }

        workphone::procedural::RoadMesh WPRoadSystem::buildOldTarmacPatches( const RoadSegmentSpec &spec,
                                                                             real_Num width,
                                                                             real_Num length )
        {
            return {};
        }

        workphone::procedural::RoadMesh WPRoadSystem::buildPotholes( const RoadSegmentSpec &spec,
                                                                     real_Num width, real_Num length )
        {
            return {};
        }

        workphone::procedural::RoadMesh WPRoadSystem::buildManholes( const RoadSegmentSpec &spec,
                                                                     real_Num width, real_Num length )
        {
            return {};
        }

        workphone::procedural::RoadMesh WPRoadSystem::buildGullyGrates( const RoadSegmentSpec &spec,
                                                                        real_Num width, real_Num length )
        {
            return {};
        }

        workphone::procedural::RoadMesh WPRoadSystem::buildArrows( const RoadSegmentSpec &spec,
                                                                   real_Num width, real_Num length )
        {
            return {};
        }

        workphone::procedural::RoadMesh WPRoadSystem::buildPedestrianCrossing(
            const RoadSegmentSpec &spec, real_Num width, real_Num length )
        {
            return {};
        }

        IntersectionResult WPRoadSystem::buildIntersectionApproach( const IntersectionSpec &spec )
        {
            return {};
        }

    }  // namespace procedural
}  // namespace workphone
