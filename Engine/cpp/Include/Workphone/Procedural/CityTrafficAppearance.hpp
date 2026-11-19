#pragma once

#include <array>
#include <algorithm>
#include <cmath>

namespace workphone::scene::procedural_scene::cityTrafficAppearance
{
    // Local +Z is the direction of travel. The origin is the chassis centre,
    // 0.76 m above the fitted road contact. Three shared silhouettes are enough
    // to make the small moving fleet read as ordinary mixed city traffic.
    inline constexpr float chassisLift = .76f;
    inline constexpr std::array<float, 3> lengths{ 4.08f, 4.58f, 4.78f };

    inline QuaternionF roadOrientation( Vector3F tangent )
    {
        // Keep the lateral axle horizontal while allowing the chassis to pitch
        // with the fitted road. +Z is forward and +X is the right-hand side.
        if( !tangent.isFinite() || tangent.lengthSquared() < 1e-8f )
        {
            return QuaternionF::identity();
        }
        tangent.normalise();
        const Vector3F horizontal( tangent.x, 0, tangent.z );
        if( horizontal.lengthSquared() < 1e-6f )
        {
            return QuaternionF::identity();
        }
        auto right = Vector3F( horizontal.z, 0, -horizontal.x ).normaliseCopy();
        auto up = tangent.crossProduct( right ).normaliseCopy();
        QuaternionF orientation;
        orientation.fromAxes( right, up, tangent );
        orientation.normalise();
        return orientation;
    }

    struct Cabin
    {
        float frontBase, frontRoof, rearRoof, rearBase, roofHeight;
    };

    inline Cabin cabin( u32 style )
    {
        switch( style % 3 )
        {
        case 0:
            return { .89f, .43f, -.88f, -1.68f, .81f };  // sloping hatch door
        case 1:
            return { 1.08f, .59f, -.75f, -1.29f, .78f };  // separate boot
        default:
            return { 1.26f, 1.04f, -2.07f, -2.28f, 1.09f };  // high van roof
        }
    }

    template <class G>
    void body( G &g, u32 style )
    {
        const auto kind = style % 3;
        const float half = lengths[kind] * .5f;
        const auto c = cabin( kind );
        constexpr float sideX = .91f, lower = -.50f, shoulder = .27f;
        constexpr float archRadius = .43f, axle = 1.27f;

        // Continuous shoulders and fitted side panels surround actual open
        // wheel arches. The former box shell filled the arch and hid the tires.
        const auto sidePanel = [&]( float side, float z0, float z1 ) {
            const float x = side * sideX;
            g.quad( { x, lower, z0 }, { x, lower, z1 }, { x, shoulder, z1 }, { x, shoulder, z0 },
                    { side, 0, 0 } );
        };
        for( const auto side : { -1.f, 1.f } )
        {
            sidePanel( side, -half + .16f, -axle - archRadius );
            sidePanel( side, -axle + archRadius, axle - archRadius );
            sidePanel( side, axle + archRadius, half - .16f );
            for( const auto wheel : { -axle, axle } )
            {
                constexpr int segments = 8;
                for( int n = 0; n < segments; ++n )
                {
                    const float z0 = wheel - archRadius + n * 2 * archRadius / segments;
                    const float z1 = wheel - archRadius + ( n + 1 ) * 2 * archRadius / segments;
                    const auto archY = [&]( float z ) {
                        const float d = z - wheel;
                        return -.43f + std::sqrt( std::max( 0.f, archRadius * archRadius - d * d ) );
                    };
                    const float x = side * sideX;
                    g.quad( { x, archY( z0 ), z0 }, { x, archY( z1 ), z1 }, { x, shoulder, z1 },
                            { x, shoulder, z0 }, { side, 0, 0 } );
                }
            }
            // Door sill and modest mirrors silhouette at three-quarter view.
            g.box( { side * .88f, -.46f, 0 }, { .075f, .075f, .77f } );
            g.box( { side * 1.02f, .23f, .67f }, { .24f, .085f, .27f } );
        }

        // Chamfered nose and tail, and deck surfaces that rise toward the
        // greenhouse. These replace the previous flat rectangular upper boxes.
        g.quad( { -.82f, lower, half }, { .82f, lower, half }, { .84f, .17f, half },
                { -.84f, .17f, half }, { 0, 0, 1 } );
        g.quad( { .82f, lower, -half }, { -.82f, lower, -half }, { -.84f, .17f, -half },
                { .84f, .17f, -half }, { 0, 0, -1 } );
        for( const auto side : { -1.f, 1.f } )
        {
            g.quad( { side * sideX, lower, half - .16f }, { side * .82f, lower, half },
                    { side * .84f, .17f, half }, { side * sideX, shoulder, half - .16f },
                    { side, 0, .25f } );
            g.quad( { side * .82f, lower, -half }, { side * sideX, lower, -half + .16f },
                    { side * sideX, shoulder, -half + .16f }, { side * .84f, .17f, -half },
                    { side, 0, -.25f } );
        }
        g.quad( { -.84f, .17f, half }, { .84f, .17f, half }, { .88f, shoulder, c.frontBase },
                { -.88f, shoulder, c.frontBase }, { 0, 1, .1f } );
        g.quad( { .88f, shoulder, c.rearBase }, { -.88f, shoulder, c.rearBase }, { -.84f, .17f, -half },
                { .84f, .17f, -half }, { 0, 1, -.1f } );
        g.quad( { -.78f, lower, -half }, { .78f, lower, -half }, { .78f, lower, half },
                { -.78f, lower, half }, { 0, -1, 0 } );

        // A sloped four-plane greenhouse, with a slight roof crown. The van
        // retains cargo volume but has eased front pillars and rear shoulders.
        constexpr float baseX = .80f, roofX = .72f;
        for( const auto side : { -1.f, 1.f } )
        {
            g.quad( { side * sideX, shoulder, c.rearBase }, { side * sideX, shoulder, c.frontBase },
                    { side * .80f, shoulder, c.frontBase }, { side * .80f, shoulder, c.rearBase },
                    { 0, 1, 0 } );
            g.quad( { side * baseX, shoulder, c.rearBase }, { side * baseX, shoulder, c.frontBase },
                    { side * roofX, c.roofHeight, c.frontRoof },
                    { side * roofX, c.roofHeight, c.rearRoof }, { side, .14f, 0 } );
        }
        g.quad( { -baseX, shoulder, c.frontBase }, { baseX, shoulder, c.frontBase },
                { roofX, c.roofHeight, c.frontRoof }, { -roofX, c.roofHeight, c.frontRoof },
                { 0, .32f, 1 } );
        g.quad( { baseX, shoulder, c.rearBase }, { -baseX, shoulder, c.rearBase },
                { -roofX, c.roofHeight, c.rearRoof }, { roofX, c.roofHeight, c.rearRoof },
                { 0, .3f, -1 } );
        g.quad( { -roofX, c.roofHeight, c.rearRoof }, { 0, c.roofHeight + .035f, c.rearRoof },
                { 0, c.roofHeight + .035f, c.frontRoof }, { -roofX, c.roofHeight, c.frontRoof },
                { 0, 1, 0 } );
        g.quad( { 0, c.roofHeight + .035f, c.rearRoof }, { roofX, c.roofHeight, c.rearRoof },
                { roofX, c.roofHeight, c.frontRoof }, { 0, c.roofHeight + .035f, c.frontRoof },
                { 0, 1, 0 } );
        if( kind == 2 )
        {
            // Faint cargo-door embossing also breaks the broad flat side.
            for( const auto side : { -1.f, 1.f } )
            {
                g.box( { side * .81f, .25f, -.85f }, { .025f, .10f, 1.75f } );
            }
        }
    }

    template <class G>
    void glazing( G &g, u32 style )
    {
        const auto kind = style % 3;
        const auto c = cabin( kind );
        const float glassTop = c.roofHeight - .085f;
        // Insets sit just outside the painted pillar planes. Broad front and
        // rear panes keep the new cabin angles legible at street distance.
        g.quad( { -.68f, .345f, c.frontBase + .018f }, { .68f, .345f, c.frontBase + .018f },
                { .65f, glassTop, c.frontRoof + .020f }, { -.65f, glassTop, c.frontRoof + .020f },
                { 0, .36f, 1 } );
        if( kind != 2 )
        {
            g.quad( { .68f, .345f, c.rearBase - .020f }, { -.68f, .345f, c.rearBase - .020f },
                    { -.65f, glassTop, c.rearRoof - .020f }, { .65f, glassTop, c.rearRoof - .020f },
                    { 0, .34f, -1 } );
        }
        for( const auto side : { -1.f, 1.f } )
        {
            const float bottomX = side * .812f, topX = side * .734f;
            const float split = kind == 2 ? .23f : ( c.frontRoof + c.rearRoof ) * .50f;
            g.quad( { bottomX, .34f, split + .065f }, { bottomX, .34f, c.frontBase - .08f },
                    { topX, glassTop, c.frontRoof - .07f }, { topX, glassTop, split + .065f },
                    { side, .1f, 0 } );
            if( kind != 2 )
            {
                g.quad( { bottomX, .34f, c.rearBase + .09f }, { bottomX, .34f, split - .065f },
                        { topX, glassTop, split - .065f }, { topX, glassTop, c.rearRoof + .08f },
                        { side, .1f, 0 } );
            }

            // Thin dark seams, handles and arch liners give the moving cars
            // useful small-scale cues without allocating per-car resources.
            const float seam = kind == 2 ? .02f : -.20f;
            g.box( { side * .924f, -.06f, seam }, { .012f, .48f, .012f } );
            g.box( { side * .936f, .095f, seam + .31f }, { .022f, .033f, .17f } );
            if( kind == 2 )
            {
                g.box( { side * .926f, -.07f, -1.12f }, { .012f, .55f, .012f } );
                g.box( { side * .936f, .13f, -1.05f }, { .022f, .035f, .16f } );
            }
            for( const auto wheel : { -1.27f, 1.27f } )
            {
                constexpr float radius = .445f;
                for( int n = 0; n < 8; ++n )
                {
                    const float z0 = wheel - radius + n * 2 * radius / 8;
                    const float z1 = wheel - radius + ( n + 1 ) * 2 * radius / 8;
                    const auto edge = [&]( float z ) {
                        const float d = z - wheel;
                        return -.43f + std::sqrt( std::max( 0.f, radius * radius - d * d ) );
                    };
                    const float x = side * .932f;
                    g.quad( { x, edge( z0 ) - .024f, z0 }, { x, edge( z1 ) - .024f, z1 },
                            { x, edge( z1 ) + .016f, z1 }, { x, edge( z0 ) + .016f, z0 },
                            { side, 0, 0 } );
                }
            }
        }
        const float half = lengths[kind] * .5f;
        g.box( { 0, -.24f, half + .014f }, { .94f, .095f, .035f } );  // front grille
        g.box( { 0, -.19f, -half - .014f }, { .43f, .15f, .035f } );  // inset plate
        if( kind == 2 )
        {
            // Two door seams follow the rear panel instead of floating off
            // its inclined surface; the lower seam continues onto the bumper.
            g.box( { 0, -.13f, -half - .019f }, { .018f, .57f, .028f } );
            const auto rearAt = [&]( float y ) {
                return c.rearBase +
                       ( c.rearRoof - c.rearBase ) * ( y - .27f ) / ( c.roofHeight - .27f ) - .022f;
            };
            g.quad( { -.010f, .32f, rearAt( .32f ) }, { .010f, .32f, rearAt( .32f ) },
                    { .010f, 1.01f, rearAt( 1.01f ) }, { -.010f, 1.01f, rearAt( 1.01f ) },
                    { 0, .28f, -1 } );
            g.box( { 0, -.32f, -half - .019f }, { 1.40f, .065f, .030f } );
            g.box( { .27f, .57f, rearAt( .57f ) }, { .16f, .042f, .032f } );
        }
    }

    template <class G>
    void wheels( G &tire, G &hub )
    {
        constexpr float radius = .34f;
        constexpr float wheelbase = 1.27f;
        constexpr int facets = 12;
        constexpr float tau = 6.283185307179586f;
        for( const auto side : { -1.f, 1.f } )
        {
            for( const auto axle : { -wheelbase, wheelbase } )
            {
                const float inner = side * .77f, outer = side * .99f;
                const Vector3F centre( outer, -.43f, axle );
                for( int i = 0; i < facets; ++i )
                {
                    const float a = tau * i / facets, b = tau * ( i + 1 ) / facets;
                    const auto ring = [&]( float x, float angle, float r ) {
                        return Vector3F( x, -.43f + std::sin( angle ) * r,
                                         axle + std::cos( angle ) * r );
                    };
                    tire.quad( ring( inner, a, radius ), ring( inner, b, radius ),
                               ring( outer, b, radius ), ring( outer, a, radius ),
                               { 0, std::sin( ( a + b ) * .5f ), std::cos( ( a + b ) * .5f ) } );
                    tire.quad( centre, ring( outer, a, radius ), ring( outer, b, radius ), centre,
                               { side, 0, 0 } );
                    hub.quad( { outer + side * .006f, -.43f, axle },
                              ring( outer + side * .006f, a, .19f ),
                              ring( outer + side * .006f, b, .19f ),
                              { outer + side * .006f, -.43f, axle }, { side, 0, 0 } );
                }
            }
        }
    }

    template <class G>
    void lamps( G &front, G &rear, u32 style )
    {
        const auto kind = style % 3;
        const float half = lengths[kind] * .5f;
        for( const auto side : { -1.f, 1.f } )
        {
            front.box( { side * .60f, -.055f, half + .027f }, { .43f, .13f, .047f } );
            if( kind == 2 )
            {
                rear.box( { side * .74f, -.13f, -half - .028f }, { .14f, .30f, .045f } );
                const auto c = cabin( kind );
                const auto rearAt = [&]( float y ) {
                    return c.rearBase +
                           ( c.rearRoof - c.rearBase ) * ( y - .27f ) / ( c.roofHeight - .27f ) - .026f;
                };
                rear.quad( { side * .65f, .34f, rearAt( .34f ) }, { side * .76f, .34f, rearAt( .34f ) },
                           { side * .73f, .88f, rearAt( .88f ) }, { side * .64f, .88f, rearAt( .88f ) },
                           { 0, .28f, -1 } );
            }
            else
            {
                rear.box( { side * .63f, .035f, -half - .028f }, { .39f, .15f, .045f } );
            }
        }
    }
}  // namespace workphone::scene::race::cityTrafficAppearance
