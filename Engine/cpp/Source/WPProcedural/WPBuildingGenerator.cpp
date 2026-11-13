#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/WPBuildingGenerator.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace workphone::procedural
{
    namespace
    {
        constexpr float pi = 3.14159265358979323846f;
        constexpr s32 maximumFloors = 256;
        constexpr float slabHeight = .22f;

        struct Footprint
        {
            float minX, maxX, minZ, maxZ;
            float width() const { return maxX - minX; }
            float depth() const { return maxZ - minZ; }
            float x() const { return ( minX + maxX ) * .5f; }
            float z() const { return ( minZ + maxZ ) * .5f; }
        };

        Footprint footprint( const BuildingSpec &spec )
        {
            return { -float( spec.width ) * .5f, float( spec.width ) * .5f,
                     -float( spec.depth ) * .5f, float( spec.depth ) * .5f };
        }

        bool validSpec( const BuildingSpec &spec )
        {
            return spec.floors > 0 && spec.floors <= maximumFloors &&
                   spec.width >= .5f && spec.width <= 1000 && spec.depth >= .5f && spec.depth <= 1000 &&
                   spec.floorH >= .5f && spec.floorH <= 20 && spec.wallThick >= 0 && spec.wallThick <= 5;
        }

        bool inset( Footprint &area, const BuildingSpec::Setback &setback )
        {
            const auto d = float( setback.depth );
            if( !std::isfinite( d ) || d < .01f ) return false;
            switch( setback.side )
            {
            case 0: if( d > area.depth() - .5f ) return false; area.maxZ -= d; break;
            case 1: if( d > area.width() - .5f ) return false; area.maxX -= d; break;
            case 2: if( d > area.depth() - .5f ) return false; area.minZ += d; break;
            case 3: if( d > area.width() - .5f ) return false; area.minX += d; break;
            default: return false;
            }
            return true;
        }

        s32 setbackFloor( const BuildingSpec::Setback &setback )
        {
            return setback.fromFloor >= 0 ? setback.fromFloor : setback.from;
        }

        Footprint baseFootprint( const BuildingSpec &spec )
        {
            auto area = footprint( spec );
            for( const auto &option : spec.setbacks )
                if( setbackFloor( option ) == 0 ) inset( area, option );
            return area;
        }

        bool same( const Footprint &a, const Footprint &b )
        {
            return a.minX == b.minX && a.maxX == b.maxX && a.minZ == b.minZ && a.maxZ == b.maxZ;
        }

        void surface( ProceduralMesh &mesh, const Vector3F &a, const Vector3F &b,
                      const Vector3F &c, const Vector3F *d, const Vector3F &mask = {} )
        {
            auto n = ( b - a ).crossProduct( c - a );
            if( n.lengthSquared() == 0 ) return;
            n.normalise();
            const auto i = mesh.addVertex( ProceduralVertex{ a, n, { 0, 1 }, mask } );
            mesh.addVertex( ProceduralVertex{ b, n, { 1, 1 }, mask } );
            mesh.addVertex( ProceduralVertex{ c, n, { 1, 0 }, mask } );
            mesh.addTriangle( i, i + 1, i + 2 );
            if( d )
            {
                mesh.addVertex( ProceduralVertex{ *d, n, { 0, 0 }, mask } );
                mesh.addTriangle( i, i + 2, i + 3 );
            }
        }

        void quad( ProceduralMesh &mesh, const Vector3F &a, const Vector3F &b,
                   const Vector3F &c, const Vector3F &d, const Vector3F &mask = {} )
        {
            surface( mesh, a, b, c, &d, mask );
        }

        void prism( ProceduralMesh &mesh, const Vector3F &centre, float width, float height,
                    float depth, float bevel = 0, bool bottom = true, bool top = true )
        {
            if( width <= 0 || height <= 0 || depth <= 0 ) return;
            const auto x = width * .5f, z = depth * .5f, y = height * .5f;
            bevel = std::min( bevel, std::min( x, z ) * .5f );
            const std::array<Vector2F, 8> ring = bevel > 0
                ? std::array<Vector2F, 8>{ { { -x + bevel, -z }, { x - bevel, -z }, { x, -z + bevel },
                    { x, z - bevel }, { x - bevel, z }, { -x + bevel, z }, { -x, z - bevel }, { -x, -z + bevel } } }
                : std::array<Vector2F, 8>{ { { -x, -z }, { x, -z }, { x, z }, { -x, z } } };
            const u32 count = bevel > 0 ? 8u : 4u;
            for( u32 edge = 0; edge < count; ++edge )
            {
                const auto a = ring[edge], b = ring[( edge + 1 ) % count];
                quad( mesh, centre + Vector3F( a.x, -y, a.y ), centre + Vector3F( a.x, y, a.y ),
                      centre + Vector3F( b.x, y, b.y ), centre + Vector3F( b.x, -y, b.y ) );
            }
            // Planar UVs across each complete cap, with no shared side normals.
            for( auto sign : { -1.f, 1.f } )
            {
                if( ( sign < 0 && !bottom ) || ( sign > 0 && !top ) ) continue;
                const auto i = mesh.addVertex( ProceduralVertex{ centre + Vector3F( 0, sign * y, 0 ),
                                                                { 0, sign, 0 }, { .5f, .5f }, {} } );
                for( u32 edge = 0; edge < count; ++edge )
                {
                    const auto p = ring[edge];
                    mesh.addVertex( ProceduralVertex{ centre + Vector3F( p.x, sign * y, p.y ),
                                                     { 0, sign, 0 }, { .5f + p.x / width, .5f + p.y / depth }, {} } );
                }
                for( u32 edge = 0; edge < count; ++edge )
                {
                    const u32 a = i + 1 + edge, b = i + 1 + ( edge + 1 ) % count;
                    mesh.addTriangle( i, sign > 0 ? b : a, sign > 0 ? a : b );
                }
            }
        }

        void place( ProceduralMesh &mesh, const Vector3F &origin, float yaw = 0 )
        {
            const auto c = std::cos( yaw ), s = std::sin( yaw );
            const auto rotate = [&]( const Vector3F &v ) {
                return Vector3F( v.x * c + v.z * s, v.y, -v.x * s + v.z * c );
            };
            for( auto &v : mesh.vertices )
            {
                v.position = rotate( v.position ) + origin;
                v.normal = rotate( v.normal );
            }
            mesh.computeBoundingBox();
        }

        void panel( ProceduralMesh &mesh, float width, float height, float z,
                    const Vector3F &mask = {}, float left = 0, float yaw = 0 )
        {
            const auto c = std::cos( yaw ), s = std::sin( yaw );
            const auto a = Vector3F( left, -height * .5f, z );
            const auto b = a + Vector3F( width * c, 0, -width * s );
            const auto up = Vector3F( 0, height, 0 );
            quad( mesh, a, b, b + up, a + up, mask );
        }

        void rail( ProceduralMesh &mesh, float x, float z, float width, float depth, float base, float height )
        {
            prism( mesh, { x, base + height * .5f, z }, width, height, depth );
            prism( mesh, { x, base + height + .045f, z }, width + .055f, .09f, depth + .055f );
        }

        void roof( ProceduralMesh &mesh, const Footprint &area, BuildingSpec::Type style )
        {
            const float w = area.width(), d = area.depth(), x = area.x(), z = area.z();
            const float wall = std::min( .18f, std::min( w, d ) * .15f );
            prism( mesh, { x, slabHeight * .5f, z }, w + .10f, slabHeight, d + .10f, .045f );
            const float parapet = style == BuildingSpec::Type::Industrial ? .28f :
                                  style == BuildingSpec::Type::Commercial ? 1.02f : .82f;
            for( float sign : { -1.f, 1.f } )
            {
                rail( mesh, x, z + sign * ( d - wall ) * .5f, w, wall, slabHeight, parapet );
                rail( mesh, x + sign * ( w - wall ) * .5f, z, wall, d - wall * 2, slabHeight, parapet );
            }
            if( style == BuildingSpec::Type::Industrial && w >= 3 && d >= 2 )
            {
                // A low monitor roof and raised cap break the long shed silhouette.
                const float monitorWidth = w * .72f, monitorDepth = d * .30f;
                prism( mesh, { x, slabHeight + .27f, z }, monitorWidth, .54f, monitorDepth, .035f );
                prism( mesh, { x, slabHeight + .60f, z }, monitorWidth + .20f, .12f, monitorDepth + .20f, .04f );
                for( int i = -2; i <= 2; ++i )
                    prism( mesh, { x + i * monitorWidth / 5, slabHeight + .29f, z + monitorDepth * .5f + .025f },
                           monitorWidth * .14f, .25f, .055f );
            }
        }

        void facadeTrim( ProceduralMesh &mesh, const Footprint &area, float base,
                         float floorHeight, s32 floor, BuildingSpec::Type style )
        {
            const float width = area.width(), depth = area.depth();
            const float x = area.x(), z = area.z();
            if( floor == 0 && width >= 2 && floorHeight >= 2.2f )
            {
                if( style == BuildingSpec::Type::Commercial || style == BuildingSpec::Type::MixedUse )
                {
                    const float canopy = style == BuildingSpec::Type::Commercial ? width * .88f : width * .62f;
                    const float y = base + floorHeight * .83f;
                    prism( mesh, { x, y, area.maxZ + .59f }, canopy, .15f, 1.30f, .045f );
                    prism( mesh, { x, y - .08f, area.maxZ + 1.18f }, canopy, .11f, .14f );
                    for( float sign : { -1.f, 1.f } )
                        prism( mesh, { x + sign * canopy * .38f, y - .15f, area.maxZ + .20f }, .12f, .19f, .46f );
                }
                else if( style == BuildingSpec::Type::Industrial )
                {
                    const float y = base + floorHeight * .75f;
                    prism( mesh, { x, y, area.maxZ + .43f }, width * .48f, .20f, .94f, .025f );
                    for( float sign : { -1.f, 1.f } )
                        prism( mesh, { x + sign * width * .24f, base + floorHeight * .36f, area.maxZ + .15f },
                               .20f, floorHeight * .72f, .26f );
                }
                else
                {
                    prism( mesh, { x, base + .075f, area.maxZ + .30f }, 1.45f, .15f, .65f, .025f );
                }
            }
            if( floor == 0 ) return;
            if( style == BuildingSpec::Type::Commercial )
            {
                // Continuous projecting slab bands frame the glazed bays.
                prism( mesh, { x, base + .10f, area.maxZ + .02f }, width + .12f, .20f, .19f );
                prism( mesh, { x, base + .10f, area.minZ - .02f }, width + .12f, .20f, .19f );
                prism( mesh, { area.minX - .02f, base + .10f, z }, .19f, .20f, depth );
                prism( mesh, { area.maxX + .02f, base + .10f, z }, .19f, .20f, depth );
            }
            else if( style != BuildingSpec::Type::Industrial && width >= 3 && floorHeight >= 2.2f && floor % 2 == 1 )
            {
                const float span = std::min( 2.6f, width * .62f );
                const float y = base + floorHeight * .12f;
                prism( mesh, { x, y, area.maxZ + .50f }, span, .16f, 1.10f, .03f );
                rail( mesh, x, area.maxZ + .99f, span, .10f, y + .08f, .66f );
                for( float sign : { -1.f, 1.f } )
                    rail( mesh, x + sign * ( span - .10f ) * .5f, area.maxZ + .48f,
                          .10f, .94f, y + .08f, .66f );
            }
        }

        void terrace( ProceduralMesh &mesh, const Footprint &strip, s32 side )
        {
            const auto w = strip.width(), d = strip.depth();
            const auto thickness = std::min( .14f, std::min( w, d ) * .35f );
            prism( mesh, { strip.x(), .075f, strip.z() }, w, .15f, d );
            if( side == 0 || side == 2 )
            {
                rail( mesh, strip.x(), side == 0 ? strip.maxZ - thickness * .5f : strip.minZ + thickness * .5f,
                      w, thickness, .15f, .88f );
                for( float sign : { -1.f, 1.f } )
                    rail( mesh, strip.x() + sign * ( w - thickness ) * .5f, strip.z(),
                          thickness, d - thickness, .15f, .88f );
            }
            else
            {
                rail( mesh, side == 1 ? strip.maxX - thickness * .5f : strip.minX + thickness * .5f,
                      strip.z(), thickness, d, .15f, .88f );
                for( float sign : { -1.f, 1.f } )
                    rail( mesh, strip.x(), strip.z() + sign * ( d - thickness ) * .5f,
                          w - thickness, thickness, .15f, .88f );
            }
        }

        void roundPipe( ProceduralMesh &mesh, float y0, float y1, float radius, float x = 0, float z = 0 )
        {
            constexpr u32 sides = 12;
            for( u32 i = 0; i < sides; ++i )
            {
                const auto a = 2 * pi * i / sides, b = 2 * pi * ( i + 1 ) / sides;
                const auto pa = Vector3F( x + std::cos( a ) * radius, y0, z + std::sin( a ) * radius );
                const auto pb = Vector3F( x + std::cos( b ) * radius, y0, z + std::sin( b ) * radius );
                const auto up = Vector3F( 0, y1 - y0, 0 );
                quad( mesh, pa, pa + up, pb + up, pb );
            }
        }

        void shell( ProceduralMesh &mesh, const Footprint &area, float y0, float y1, bool top )
        {
            prism( mesh, { area.x(), ( y0 + y1 ) * .5f, area.z() }, area.width(), y1 - y0,
                   area.depth(), .08f, y0 == 0, top );
        }
    }

    WindowState WPBuildingGenerator::pickWindowState( u32 seed, s32 floor, real_Num damage )
    {
        WPNoise noise( seed );
        const auto r = noise.noise3( floor * 17.f + .37f, float( seed & 0xffff ) + .61f, .19f ) * .5f + .5f;
        if( r < .07f + damage * .25f ) return WindowState::Boarded;
        if( r < .20f + damage * .5f ) return WindowState::Open;
        if( floor > 0 && r < .42f ) return WindowState::Shuttered;
        if( floor > 0 && r < .52f ) return WindowState::Ajar;
        if( r < .60f ) return WindowState::Curtain;
        if( r < .66f ) return WindowState::Lit;
        return WindowState::Glazed;
    }

    WPBuildingGenerator::WPBuildingGenerator( u32 seed ) : mSeed( seed ) {}

    BuildingResult WPBuildingGenerator::generate( const BuildingSpec &spec, const Vector3<real_Num> &origin )
    {
        BuildingResult result;
        result.position = origin;
        if( !validSpec( spec ) || !std::isfinite( origin.x ) || !std::isfinite( origin.y ) ||
            !std::isfinite( origin.z ) ) return result;

        const u32 seed = spec.seed ? spec.seed : mSeed;
        const float floorHeight = float( spec.floorH );
        const float totalHeight = spec.floors * floorHeight;
        const auto world = Vector3F( float( origin.x ), float( origin.y ), float( origin.z ) );
        auto area = baseFootprint( spec );
        for( s32 floor = 0; floor < spec.floors; ++floor )
        {
            for( const auto &option : spec.setbacks )
            {
                if( floor == 0 || setbackFloor( option ) != floor ) continue;
                auto old = area;
                if( !inset( area, option ) ) continue;
                auto strip = old;
                switch( option.side )
                {
                case 0: strip.minZ = area.maxZ; break;
                case 1: strip.minX = area.maxX; break;
                case 2: strip.maxZ = area.minZ; break;
                case 3: strip.maxX = area.minX; break;
                }
                ProceduralMesh deck;
                terrace( deck, strip, option.side );
                place( deck, world + Vector3F( 0, floor * floorHeight, 0 ) );
                result.details.push_back( std::move( deck ) );
            }

            ProceduralMesh trim;
            facadeTrim( trim, area, floor * floorHeight, floorHeight, floor, spec.type );
            if( trim.hasGeometry() )
            {
                place( trim, world );
                result.details.push_back( std::move( trim ) );
            }
            const bool ground = floor == 0;
            const bool retail = ground && ( spec.type == BuildingSpec::Type::Commercial || spec.type == BuildingSpec::Type::MixedUse );
            const bool industrial = spec.type == BuildingSpec::Type::Industrial;
            const auto y = ( floor + .5f ) * floorHeight;
            for( s32 side = 0; side < 4; ++side )
            {
                const bool alongX = side == 0 || side == 2;
                const auto length = alongX ? area.width() : area.depth();
                const auto bays = std::clamp( int( length / ( industrial ? 4.f : 3.f ) ), 1, 32 );
                const auto bay = length / bays;
                for( s32 index = 0; index < bays; ++index )
                {
                    const u32 detailSeed = seed + u32( floor ) * 997u + u32( side ) * 101u + u32( index ) * 37u;
                    const bool entry = ground && !industrial && side == 0 && index == bays / 2;
                    const auto state = retail ? WindowState::Glazed : pickWindowState( detailSeed, floor, spec.generateDamage ? .55f : .20f );
                    const auto width = std::min( bay * ( retail ? .8f : .58f ), retail ? 3.25f : industrial ? 2.2f : 1.55f );
                    const auto height = std::min( floorHeight * ( entry ? .75f : retail ? .78f : industrial ? .42f : .60f ),
                                                  entry || retail ? 2.7f : 1.85f );
                    auto window = buildWindow( state, detailSeed, width, height );
                    const auto u = ( index + .5f ) * bay + ( alongX ? area.minX : area.minZ );
                    const float outward = .052f;
                    const float cy = entry ? height * .5f + .08f : y;
                    const Vector3F locations[] = { { u, cy, area.maxZ + outward }, { area.maxX + outward, cy, u },
                                                   { u, cy, area.minZ - outward }, { area.minX - outward, cy, u } };
                    const float rotations[] = { 0, pi * .5f, pi, -pi * .5f };
                    place( window, world + locations[side], rotations[side] );
                    result.windows.push_back( std::move( window ) );
                }
            }
            if( spec.generateInterior )
            {
                BuildingSpec room;
                room.width = area.width(); room.depth = area.depth(); room.floorH = spec.floorH;
                room.wallThick = spec.wallThick;
                auto mesh = buildInterior( room, floor, seed + u32( floor ) );
                if( mesh.hasGeometry() )
                {
                    place( mesh, world + Vector3F( area.x(), floor * floorHeight, area.z() ) );
                    result.interiors.push_back( std::move( mesh ) );
                }
            }
        }

        result.body = buildBody( spec, seed );
        place( result.body, world );
        roof( result.roof, area, spec.type );
        place( result.roof, world + Vector3F( 0, totalHeight, 0 ) );

        const auto roofOrigin = world + Vector3F( area.x(), totalHeight + slabHeight, area.z() );
        const bool hasAccess = spec.floors >= 5 && area.width() >= 3.3f && area.depth() >= 3.3f;
        if( area.width() >= ( hasAccess ? 5.f : 2.f ) && area.depth() >= 2 &&
            ( seed % 7 > 2 || spec.type == BuildingSpec::Type::Industrial ) )
        {
            auto unit = buildACUnit( seed );
            place( unit, roofOrigin + Vector3F( -area.width() * .23f, .31f, area.depth() * .20f ) );
            result.details.push_back( std::move( unit ) );
        }
        if( hasAccess )
        {
            auto access = buildRoofAccess( spec, seed );
            place( access, roofOrigin + Vector3F( area.width() * .12f, 0, 0 ) );
            result.details.push_back( std::move( access ) );
        }
        const auto base = baseFootprint( spec );
        auto drainArea = base;
        float drainHeight = totalHeight + slabHeight;
        for( s32 floor = 1; floor < spec.floors; ++floor )
        {
            for( const auto &option : spec.setbacks )
                if( setbackFloor( option ) == floor ) inset( drainArea, option );
            if( drainArea.maxZ != base.maxZ || drainArea.maxX < base.maxX - .30f )
            {
                drainHeight = floor * floorHeight + .15f;
                break;
            }
        }
        auto drain = buildDrainpipe( drainHeight, seed );
        place( drain, world + Vector3F( base.maxX - .30f, 0, base.maxZ + .09f ) );
        result.details.push_back( std::move( drain ) );
        result.wallKeyVariant = std::clamp( float( mNoise.noise3( float( seed & 0xffff ) * .0137f, .37f, .19f ) ) * .5f + .5f, 0.f, 1.f );
        return result;
    }

    ProceduralMesh WPBuildingGenerator::buildBody( const BuildingSpec &spec, u32 )
    {
        ProceduralMesh mesh;
        if( !validSpec( spec ) ) return mesh;
        auto area = baseFootprint( spec );
        s32 start = 0;
        for( s32 floor = 1; floor < spec.floors; ++floor )
        {
            auto next = area;
            for( const auto &setback : spec.setbacks )
                if( setbackFloor( setback ) == floor ) inset( next, setback );
            if( !same( next, area ) )
            {
                shell( mesh, area, start * float( spec.floorH ), floor * float( spec.floorH ), true );
                area = next;
                start = floor;
            }
        }
        shell( mesh, area, start * float( spec.floorH ), spec.floors * float( spec.floorH ), true );
        mesh.computeBoundingBox();
        return mesh;
    }

    ProceduralMesh WPBuildingGenerator::buildWindow( WindowState state, u32 seed, real_Num w, real_Num h )
    {
        ProceduralMesh mesh;
        const auto width = float( w ), height = float( h );
        if( !( width >= .08f && width <= 20 && height >= .08f && height <= 20 ) ) return mesh;
        const auto frame = std::min( .065f, std::min( width, height ) * .16f );
        for( auto sign : { -1.f, 1.f } )
        {
            prism( mesh, { sign * ( width + frame ) * .5f, 0, .014f }, frame, height + frame * 2, .105f );
            prism( mesh, { 0, sign * ( height + frame ) * .5f, .014f }, width, frame, .105f );
        }
        prism( mesh, { 0, -height * .5f - frame, .054f }, width + frame * 3, frame * .65f, .145f, .01f );
        switch( state )
        {
        case WindowState::Glazed:
        case WindowState::Lit:
        case WindowState::Curtain:
            panel( mesh, width, height, .018f,
                   state == WindowState::Lit ? Vector3F( 0, 0, .05f ) :
                   state == WindowState::Curtain ? Vector3F( 0, .22f, .28f ) : Vector3F{}, -width * .5f );
            prism( mesh, { 0, 0, .042f }, frame * .68f, height, .05f );
            if( state == WindowState::Curtain )
                for( int fold : { -2, -1, 1, 2 } )
                    prism( mesh, { fold * width * .19f, 0, .023f }, frame * .22f, height, .018f );
            break;
        case WindowState::Ajar:
            panel( mesh, width, height, .036f, {}, -width * .5f, -.26f - float( seed % 7 ) * .028f );
            break;
        case WindowState::Boarded:
            for( int board = 0; board < 3; ++board )
                prism( mesh, { 0, ( board - 1 ) * height / 3, .071f }, width + frame, height * .30f, .044f, .008f );
            break;
        case WindowState::Shuttered:
            for( auto side : { -1.f, 1.f } )
            {
                const auto x = side * width * .25f;
                for( auto edge : { -1.f, 1.f } )
                    prism( mesh, { x + edge * width * .22f, 0, .077f }, frame * .66f, height, .054f );
                for( int slat = 0; slat < 7; ++slat )
                    prism( mesh, { x, ( ( slat + .5f ) / 7.f - .5f ) * height, .078f },
                           width * .43f, height * .101f, .042f );
            }
            break;
        case WindowState::Open: break;
        }
        mesh.computeBoundingBox();
        return mesh;
    }

    ProceduralMesh WPBuildingGenerator::buildRoof( const BuildingSpec &spec, u32 )
    {
        ProceduralMesh mesh;
        if( !validSpec( spec ) ) return mesh;
        auto area = baseFootprint( spec );
        for( s32 floor = 1; floor < spec.floors; ++floor )
            for( const auto &option : spec.setbacks )
                if( setbackFloor( option ) == floor ) inset( area, option );
        roof( mesh, area, spec.type );
        mesh.computeBoundingBox();
        return mesh;
    }

    ProceduralMesh WPBuildingGenerator::buildRoofAccess( const BuildingSpec &, u32 seed )
    {
        ProceduralMesh mesh;
        constexpr float w = 2.24f, d = 2.36f, h = 2.44f;
        prism( mesh, { 0, h * .5f, 0 }, w, h, d, .035f );
        auto door = buildWindow( WindowState::Glazed, seed, .86f, 2.04f );
        mesh.append( door, { 0, 1.07f, d * .5f + .052f } );
        prism( mesh, { 0, h + .08f, 0 }, w + .17f, .16f, d + .17f, .045f );
        prism( mesh, { 0, 2.21f, d * .5f + .12f }, 1.15f, .085f, .26f );
        mesh.computeBoundingBox();
        return mesh;
    }

    ProceduralMesh WPBuildingGenerator::buildACUnit( u32 seed )
    {
        ProceduralMesh mesh;
        prism( mesh, { 0, .04f, 0 }, 1.0f, .52f, .80f, .045f );
        for( float sign : { -1.f, 1.f } )
            prism( mesh, { sign * .32f, -.267f, 0 }, .12f, .086f, .85f );
        for( int slat = 0; slat < 6; ++slat )
            prism( mesh, { 0, ( slat - 2.5f ) * .061f, .412f }, .78f, .026f, .037f );
        roundPipe( mesh, .30f, .35f, .25f, float( seed % 3 ) * .012f, 0 );
        for( float sign : { -1.f, 1.f } )
            prism( mesh, { sign * .12f, .344f, 0 }, .034f, .018f, .41f );
        mesh.computeBoundingBox();
        return mesh;
    }

    ProceduralMesh WPBuildingGenerator::buildTerrace( const BuildingSpec &spec, u32 )
    {
        ProceduralMesh mesh;
        if( !validSpec( spec ) ) return mesh;
        const float w = float( spec.width );
        terrace( mesh, { -w * .5f, w * .5f, 0, 1.5f }, 0 );
        mesh.computeBoundingBox();
        return mesh;
    }

    ProceduralMesh WPBuildingGenerator::buildDrainpipe( real_Num height, u32 )
    {
        ProceduralMesh mesh;
        if( !( height > 0 && height <= maximumFloors * 20.f + 1 ) ) return mesh;
        roundPipe( mesh, 0, float( height ), .043f );
        for( float y = .55f; y < height - .1f; y += 2.6f )
        {
            roundPipe( mesh, y, y + .045f, .054f );
            prism( mesh, { 0, y + .023f, -.069f }, .066f, .045f, .077f );
        }
        mesh.computeBoundingBox();
        return mesh;
    }

    ProceduralMesh WPBuildingGenerator::buildInterior( const BuildingSpec &spec, s32 floor, u32 )
    {
        ProceduralMesh mesh;
        if( !validSpec( spec ) ) return mesh;
        const float t = std::max( .05f, float( spec.wallThick ) );
        const float w = float( spec.width ) - 2 * t, d = float( spec.depth ) - 2 * t;
        const float h = float( spec.floorH ) - ( floor == 0 ? .13f : 0 );
        if( w <= t * 2 || d <= t * 2 ) return mesh;
        prism( mesh, { -w * .10f, h * .5f, 0 }, t, h, d );
        prism( mesh, { -w * .30f - t * .25f, h * .5f, d * .10f }, w * .40f - t * .5f, h, t );
        prism( mesh, { w * .20f + t * .25f, h * .5f, d * .10f }, w * .60f - t * .5f, h, t );
        mesh.computeBoundingBox();
        return mesh;
    }

    ProceduralMesh WPBuildingGenerator::buildStairs( const BuildingSpec &spec, s32, u32 )
    {
        ProceduralMesh mesh;
        if( !validSpec( spec ) ) return mesh;
        const auto steps = std::max( 3, int( std::ceil( spec.floorH / .19f ) ) );
        const auto rise = float( spec.floorH ) / steps;
        constexpr float run = .275f;
        for( int step = 0; step < steps; ++step )
            prism( mesh, { 0, ( step + 1 ) * rise * .5f, ( step + .5f ) * run },
                   1.2f, ( step + 1 ) * rise, run );
        mesh.computeBoundingBox();
        return mesh;
    }
}
