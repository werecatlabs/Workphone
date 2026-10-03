// ============================================================================
// WPBuildingGenerator.cpp - Implementation of the AAA building generator
// ============================================================================

#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPBuildingGenerator.hpp"
#include <cmath>
#include <algorithm>

namespace workphone
{
    namespace procedural
    {
        namespace
        {
            inline real_Num clamp01( real_Num v )
            {
                return v < 0.0f ? 0.0f : ( v > 1.0f ? 1.0f : v );
            }
        }  // namespace

        WindowState WPBuildingGenerator::pickWindowState( u32 seed, s32 floor, real_Num damage )
        {
            WPNoise n( seed );
            real_Num r = n.noise3( floor * 17.0f, seed & 0xFFFF, 0 ) * 0.5f + 0.5f;
            // Lower floors more likely to be shopfronts; upper floors have varied states.
            bool upperFloor = floor > 0;
            if( r < 0.07f + damage * 0.25f )
                return WindowState::Boarded;
            if( r < 0.20f + damage * 0.5f )
                return WindowState::Open;
            if( upperFloor && r < 0.42f )
                return WindowState::Shuttered;
            if( upperFloor && r < 0.52f )
                return WindowState::Ajar;
            if( r < 0.60f )
                return WindowState::Curtain;
            if( r < 0.66f )
                return WindowState::Lit;
            return WindowState::Glazed;
        }

        WPBuildingGenerator::WPBuildingGenerator( u32 seed /*= 0xc0de */ ) : mSeed( seed )
        {
        }

        // ===================================================================
        // TOP-LEVEL GENERATE
        // ===================================================================
        BuildingResult WPBuildingGenerator::generate( const BuildingSpec &spec,
                                                      const Vector3<real_Num> &origin )
        {
            BuildingResult result;
            result.position = origin;

            u32 seed = spec.seed ? spec.seed : mSeed;

            // Main body
            result.body = buildBody( spec, seed );
            // Roof slab + parapet
            result.roof = buildRoof( spec, seed );
            // Build windows per floor and per side
            real_Num totalH = spec.floors * spec.floorH;
            for( s32 f = 0; f < spec.floors; ++f )
            {
                real_Num fy = f * spec.floorH + spec.floorH * 0.5f;
                WindowState ws = pickWindowState( seed, f );

                s32 sides = 4;
                for( s32 side = 0; side < sides; ++side )
                {
                    bool isLength = ( side == 0 || side == 2 );
                    real_Num sideLen = isLength ? spec.width : spec.depth;
                    s32 nWindows = std::max( 1, static_cast<s32>( sideLen / 3.0f ) );

                    for( s32 i = 0; i < nWindows; ++i )
                    {
                        ProceduralMesh w = buildWindow( ws, seed + f * 4 + side * 32 + i );
                        // Position window on the wall
                        real_Num u = -sideLen * 0.5f + ( i + 0.5f ) * ( sideLen / nWindows );
                        real_Num offset = spec.wallThick * 0.5f + 0.01f;
                        Vector3<real_Num> p;
                        real_Num rot = 0;
                        switch( side )
                        {
                        case 0:
                            p = Vector3<real_Num>( u, fy, spec.depth * 0.5f + offset );
                            break;
                        case 1:
                            p = Vector3<real_Num>( spec.width * 0.5f + offset, fy, u );
                            rot = -1.5707963f;
                            break;
                        case 2:
                            p = Vector3<real_Num>( u, fy, -spec.depth * 0.5f - offset );
                            rot = 3.1415927f;
                            break;
                        case 3:
                            p = Vector3<real_Num>( -spec.width * 0.5f - offset, fy, u );
                            rot = 1.5707963f;
                            break;
                        }
                        for( auto &v : w.vertices )
                        {
                            real_Num cs = std::cos( rot ), sn = std::sin( rot );
                            Vector3<real_Num> q( v.position.x * cs + v.position.z * sn, v.position.y,
                                                 -v.position.x * sn + v.position.z * cs );
                            v.position = q + p + origin;
                        }
                        result.windows.push_back( w );
                    }
                }
            }

            // Roof clutter (AC units) - chance
            if( ( seed % 7 ) > 2 )
            {
                ProceduralMesh ac = buildACUnit( seed );
                Vector3<real_Num> acPos =
                    origin + Vector3<real_Num>( spec.width * 0.25f, totalH + 0.5f, spec.depth * 0.25f );
                for( auto &v : ac.vertices )
                    v.position = v.position + acPos;
                result.details.push_back( ac );
            }

            // Roof access penthouse if high-rise
            if( spec.floors >= 5 )
            {
                ProceduralMesh penthouse = buildRoofAccess( spec, seed );
                for( auto &v : penthouse.vertices )
                    v.position = v.position + origin + Vector3<real_Num>( 0, totalH, 0 );
                result.details.push_back( penthouse );
            }

            // Setback terrace if specified
            for( const auto &sb : spec.setbacks )
            {
                if( sb.from < 0 || sb.from >= spec.floors )
                    continue;
                ProceduralMesh terrace = buildTerrace( spec, seed + sb.from );
                real_Num y = ( sb.from + 1 ) * spec.floorH;
                for( auto &v : terrace.vertices )
                    v.position = v.position + origin + Vector3<real_Num>( 0, y, 0 );
                result.details.push_back( terrace );
            }

            // Drainpipe at one corner
            {
                ProceduralMesh drain = buildDrainpipe( totalH + 0.6f, seed );
                Vector3<real_Num> dp = origin + Vector3<real_Num>( spec.width * 0.5f + 0.04f, 0,
                                                                   spec.depth * 0.5f + 0.04f );
                for( auto &v : drain.vertices )
                    v.position = v.position + dp;
                result.details.push_back( drain );
            }

            // Optional interior partitions
            if( spec.generateInterior )
            {
                for( s32 f = 0; f < spec.floors; ++f )
                {
                    ProceduralMesh interior = buildInterior( spec, f, seed + f );
                    for( auto &v : interior.vertices )
                        v.position = v.position + origin + Vector3<real_Num>( 0, f * spec.floorH, 0 );
                    result.interiors.push_back( interior );
                }
            }

            // Move main body to origin
            for( auto &v : result.body.vertices )
                v.position = v.position + origin;
            for( auto &v : result.roof.vertices )
                v.position = v.position + origin + Vector3<real_Num>( 0, totalH, 0 );

            // Wall key variant for tinting
            result.wallKeyVariant = mNoise.noise3( real_Num( seed ) * 0.01f, 0, 0 ) * 0.5f + 0.5f;

            return result;
        }

        // ===================================================================
        // BUILD BODY - chamfered box with subtle warp (no perfectly flat walls)
        // ===================================================================
        ProceduralMesh WPBuildingGenerator::buildBody( const BuildingSpec &spec, u32 seed )
        {
            real_Num totalH = spec.floors * spec.floorH;
            ProceduralMesh m =
                WPGeometryKit::chamferedBox( spec.width, totalH, spec.depth, 0.022f, 1, 1, 1 );

            // Subtle warp on the outer faces - nothing perfectly flat
            WPGeometryKit::warpMesh( m, 0.018f, 0.5f, seed );

            return m;
        }

        // ===================================================================
        // BUILD WINDOW - state-dependent geometry
        // ===================================================================
        ProceduralMesh WPBuildingGenerator::buildWindow( WindowState state, u32 seed, real_Num width,
                                                         real_Num height )
        {
            ProceduralMesh m;
            real_Num fD = 0.15f;

            switch( state )
            {
            case WindowState::Glazed:
            case WindowState::Lit:
            case WindowState::Curtain:
            {
                // Frame box
                ProceduralMesh frame =
                    WPGeometryKit::chamferedBox( width + 0.15f, height + 0.15f, fD, 0.012f, 1, 1, 1 );
                m.append( frame, Vector3<real_Num>( 0, 0, 0 ) );

                // Glass / curtain pane
                ProceduralMesh pane = WPGeometryKit::quad( width, height );
                Vector3<real_Num> pn( 0, 0, fD * 0.5f );
                m.append( pane, pn );

                // Interior glow for "lit" windows
                if( state == WindowState::Lit )
                {
                    ProceduralMesh glow = WPGeometryKit::quad( width * 0.7f, height * 0.7f );
                    m.append( glow, Vector3<real_Num>( 0, 0, fD * 0.55f ) );
                }
                break;
            }
            case WindowState::Open:
            case WindowState::Ajar:
            {
                // Just the frame
                ProceduralMesh frame =
                    WPGeometryKit::chamferedBox( width + 0.15f, height + 0.15f, fD, 0.012f, 1, 1, 1 );
                m.append( frame, Vector3<real_Num>( 0, 0, 0 ) );

                if( state == WindowState::Ajar )
                {
                    // Pane rotated open
                    ProceduralMesh pane = WPGeometryKit::quad( width, height );
                    Vector3<real_Num> pn( width * 0.25f, 0, fD * 0.5f + 0.05f );
                    for( auto &v : pane.vertices )
                        v.position = v.position + pn;
                    m.append( pane, Vector3<real_Num>( 0, 0, 0 ) );
                }
                break;
            }
            case WindowState::Boarded:
            {
                // Frame outline only
                ProceduralMesh frame =
                    WPGeometryKit::chamferedBox( width + 0.15f, height + 0.15f, fD, 0.012f, 1, 1, 1 );
                m.append( frame, Vector3<real_Num>( 0, 0, 0 ) );

                // Three horizontal boards across the opening
                real_Num boardH = height / 3.0f - 0.02f;
                for( s32 i = 0; i < 3; ++i )
                {
                    real_Num y = -height * 0.5f + ( i + 0.5f ) * ( height / 3.0f );
                    ProceduralMesh board =
                        WPGeometryKit::chamferedBox( width + 0.2f, boardH, 0.04f, 0.008f, 1, 1, 1 );
                    Vector3<real_Num> bp( 0, y, fD * 0.5f + 0.02f );
                    m.append( board, bp );
                }
                break;
            }
            case WindowState::Shuttered:
            {
                // Two louvered shutters hinged to each side
                ProceduralMesh frame =
                    WPGeometryKit::chamferedBox( width + 0.15f, height + 0.15f, fD, 0.012f, 1, 1, 1 );
                m.append( frame, Vector3<real_Num>( 0, 0, 0 ) );

                real_Num sh = height * 0.5f - 0.02f;
                for( s32 i = 0; i < 2; ++i )
                {
                    real_Num side = ( i == 0 ) ? 1.0f : -1.0f;
                    ProceduralMesh shutter =
                        WPGeometryKit::chamferedBox( width * 0.45f, sh, 0.04f, 0.008f, 1, 1, 1 );
                    Vector3<real_Num> sp( side * width * 0.27f, 0, fD * 0.5f + 0.02f );
                    m.append( shutter, sp );
                }
                break;
            }
            }

            return m;
        }

        // ===================================================================
        // BUILD ROOF - slab + parapet
        // ===================================================================
        ProceduralMesh WPBuildingGenerator::buildRoof( const BuildingSpec &spec, u32 seed )
        {
            ProceduralMesh m;

            // Roof screed slab
            ProceduralMesh slab = WPGeometryKit::chamferedBox( spec.width + 0.1f, 0.2f,
                                                               spec.depth + 0.1f, 0.04f, 1, 1, 1 );
            m.append( slab, Vector3<real_Num>( 0, 0.1f, 0 ) );

            // Coping (raised concrete edge along the parapet top)
            ProceduralMesh coping =
                WPGeometryKit::chamferedBox( spec.width + 0.2f, 0.1f, 0.22f, 0.04f, 1, 1, 1 );
            m.append( coping, Vector3<real_Num>( 0, spec.floors * spec.floorH + 0.2f + 0.05f,
                                                 spec.depth * 0.5f + 0.05f ) );
            m.append( coping, Vector3<real_Num>( 0, spec.floors * spec.floorH + 0.2f + 0.05f,
                                                 -spec.depth * 0.5f - 0.05f ) );

            ProceduralMesh copingX =
                WPGeometryKit::chamferedBox( 0.22f, 0.1f, spec.depth + 0.2f, 0.04f, 1, 1, 1 );
            m.append( copingX, Vector3<real_Num>( spec.width * 0.5f + 0.05f,
                                                  spec.floors * spec.floorH + 0.2f + 0.05f, 0 ) );
            m.append( copingX, Vector3<real_Num>( -spec.width * 0.5f - 0.05f,
                                                  spec.floors * spec.floorH + 0.2f + 0.05f, 0 ) );

            // Parapet (4 walls)
            real_Num ph = 0.92f;
            real_Num py = spec.floors * spec.floorH + 0.2f + ph * 0.5f;
            ProceduralMesh pLong =
                WPGeometryKit::chamferedBox( spec.width + 0.2f, ph, 0.22f, 0.04f, 1, 1, 1 );
            m.append( pLong, Vector3<real_Num>( 0, py, spec.depth * 0.5f + 0.05f ) );
            m.append( pLong, Vector3<real_Num>( 0, py, -spec.depth * 0.5f - 0.05f ) );

            ProceduralMesh pShort =
                WPGeometryKit::chamferedBox( 0.22f, ph, spec.depth + 0.2f, 0.04f, 1, 1, 1 );
            m.append( pShort, Vector3<real_Num>( spec.width * 0.5f + 0.05f, py, 0 ) );
            m.append( pShort, Vector3<real_Num>( -spec.width * 0.5f - 0.05f, py, 0 ) );

            return m;
        }

        // ===================================================================
        // ROOF ACCESS PENTHOUSE
        // ===================================================================
        ProceduralMesh WPBuildingGenerator::buildRoofAccess( const BuildingSpec &spec, u32 seed )
        {
            ProceduralMesh m;
            real_Num w = 2.4f, d = 2.6f, h = 2.5f;
            ProceduralMesh body = WPGeometryKit::chamferedBox( w, h, d, 0.022f, 1, 1, 1 );
            m.append( body, Vector3<real_Num>( spec.width * 0.3f, h * 0.5f, 0 ) );

            // Door opening on +Z face
            ProceduralMesh door = buildWindow( WindowState::Open, seed, 1.05f, 2.16f );
            Vector3<real_Num> dp( spec.width * 0.3f, 1.08f, d * 0.5f + 0.05f );
            m.append( door, dp );

            // Roof slab
            ProceduralMesh top = WPGeometryKit::chamferedBox( w + 0.1f, 0.2f, d + 0.1f, 0.04f, 1, 1, 1 );
            m.append( top, Vector3<real_Num>( spec.width * 0.3f, h + 0.1f, 0 ) );

            return m;
        }

        // ===================================================================
        // AC UNIT
        // ===================================================================
        ProceduralMesh WPBuildingGenerator::buildACUnit( u32 seed )
        {
            ProceduralMesh m = WPGeometryKit::chamferedBox( 1.0f, 0.6f, 0.8f, 0.04f, 1, 1, 1 );

            // Fan grille on the side
            ProceduralMesh grille = WPGeometryKit::quad( 0.6f, 0.4f );
            m.append( grille, Vector3<real_Num>( 0.5f + 0.01f, 0, 0 ) );

            return m;
        }

        // ===================================================================
        // SETBACK TERRACE
        // ===================================================================
        ProceduralMesh WPBuildingGenerator::buildTerrace( const BuildingSpec &spec, u32 seed )
        {
            ProceduralMesh m;

            // The exposed strip left by the setback: a screed slab + coping + parapet
            real_Num depth = 1.5f;
            real_Num sideLen = spec.width;
            ProceduralMesh screed =
                WPGeometryKit::chamferedBox( sideLen + 0.08f, 0.26f, depth + 0.08f, 0.04f, 1, 1, 1 );
            m.append( screed, Vector3<real_Num>( 0, 0.13f, depth * 0.5f ) );

            ProceduralMesh par =
                WPGeometryKit::chamferedBox( sideLen + 0.2f, 0.92f, 0.22f, 0.04f, 1, 1, 1 );
            m.append( par, Vector3<real_Num>( 0, 0.59f, depth + 0.06f ) );

            ProceduralMesh coping =
                WPGeometryKit::chamferedBox( sideLen + 0.2f, 0.1f, 0.22f, 0.04f, 1, 1, 1 );
            m.append( coping, Vector3<real_Num>( 0, 1.12f, depth + 0.06f ) );

            // Returns at each end of the terrace
            for( real_Num sign : { -1.0f, 1.0f } )
            {
                ProceduralMesh ret = WPGeometryKit::chamferedBox( depth, 0.92f, 0.22f, 0.04f, 1, 1, 1 );
                m.append( ret,
                          Vector3<real_Num>( sign * ( sideLen * 0.5f - 0.11f ), 0.59f, depth * 0.5f ) );
            }

            return m;
        }

        // ===================================================================
        // DRAINPIPE
        // ===================================================================
        ProceduralMesh WPBuildingGenerator::buildDrainpipe( real_Num height, u32 seed )
        {
            // A thin cylinder at the wall corner
            return WPGeometryKit::cylinder( 0.04f, height, 8, 1, false, 1.0f );
        }

        // ===================================================================
        // INTERIOR PARTITIONS
        // ===================================================================
        ProceduralMesh WPBuildingGenerator::buildInterior( const BuildingSpec &spec, s32 floor,
                                                           u32 seed )
        {
            ProceduralMesh m;
            real_Num w = spec.width - spec.wallThick * 2.0f;
            real_Num d = spec.depth - spec.wallThick * 2.0f;
            real_Num h = spec.floorH;
            if( floor == 0 )
                h -= 0.13f;

            // Two simple perpendicular partition walls creating 4 rooms
            // Wall 1: along Z axis, off-center
            real_Num wx1 = -w * 0.5f + w * 0.4f;
            ProceduralMesh p1 =
                WPGeometryKit::chamferedBox( spec.wallThick, h, d * 0.85f, 0.012f, 1, 1, 1 );
            m.append( p1, Vector3<real_Num>( wx1, h * 0.5f, d * 0.5f * 0.85f ) );

            // Wall 2: along X axis, off-center
            real_Num wz2 = -d * 0.5f + d * 0.6f;
            ProceduralMesh p2 =
                WPGeometryKit::chamferedBox( w * 0.4f, h, spec.wallThick, 0.012f, 1, 1, 1 );
            m.append( p2, Vector3<real_Num>( -w * 0.5f + w * 0.2f, h * 0.5f, wz2 ) );

            return m;
        }

        // ===================================================================
        // STAIRS
        // ===================================================================
        ProceduralMesh WPBuildingGenerator::buildStairs( const BuildingSpec &spec, s32 fromFloor,
                                                         u32 seed )
        {
            real_Num climb = spec.floorH;
            s32 steps = std::max( 6, static_cast<s32>( climb / 0.19f ) );
            real_Num run = 0.275f * steps;
            return WPGeometryKit::stairRun( 1.2f, steps, climb, run, "right" );
        }
    }  // namespace procedural
}  // namespace workphone
