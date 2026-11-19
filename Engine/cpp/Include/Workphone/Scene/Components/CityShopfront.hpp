#pragma once

namespace cityShopfront
{
    enum class Style : u32
    {
        Market,
        Cafe,
        Studio,
        Count
    };
    constexpr float glassDepth = .192f;
    constexpr size_t faceBudget = 288, displayBudget = 96;

    struct Materials
    {
        std::array<SmartPtr<render::IMaterial>, static_cast<u32>( Style::Count )> fascia;
        SmartPtr<render::IMaterial> lettering, display;
    };

    Materials materials( SceneAssets &assets )
    {
        return { { material( assets, { .13f, .21f, .27f, 1 }, .56f ),
                   material( assets, { .16f, .25f, .21f, 1 }, .73f ),
                   material( assets, { .27f, .20f, .15f, 1 }, .58f ) },
                 material( assets, { .79f, .73f, .63f, 1 }, .62f ),
                 material( assets, { .52f, .38f, .26f, 1 }, .78f ) };
    }

    float fasciaWidth( float span, float door )
    {
        const float requested = std::clamp( span * .63f, 2.65f, 6.20f );
        return std::min( requested, span - 2.f * std::abs( door ) - .47f );
    }

    Style choose( u64 lot, u32 seed, bool warehouse, bool artdeco )
    {
        const u64 key = render::foliageStableHash( lot ^ ( u64( seed ) << 32 ) ^ 0x73686f7066726f6eull );
        if( warehouse )
        {
            return key % 4u == 0 ? Style::Studio : Style::Market;
        }
        if( artdeco )
        {
            return key % 4u == 0 ? Style::Cafe : Style::Studio;
        }
        return static_cast<Style>( key % static_cast<u32>( Style::Count ) );
    }

    template <class Line>
    void strokes( char letter, const Line &line )
    {
        switch( letter )
        {
        case 'A':
            line( 0, 0, .5f, 1 );
            line( .5f, 1, 1, 0 );
            line( .22f, .43f, .78f, .43f );
            break;
        case 'C':
            line( .9f, 1, .18f, 1 );
            line( .18f, 1, 0, .79f );
            line( 0, .79f, 0, .21f );
            line( 0, .21f, .18f, 0 );
            line( .18f, 0, .9f, 0 );
            break;
        case 'D':
            line( 0, 0, 0, 1 );
            line( 0, 1, .62f, 1 );
            line( .62f, 1, 1, .72f );
            line( 1, .72f, 1, .28f );
            line( 1, .28f, .62f, 0 );
            line( .62f, 0, 0, 0 );
            break;
        case 'E':
            line( 0, 0, 0, 1 );
            line( 0, 1, 1, 1 );
            line( 0, .51f, .77f, .51f );
            line( 0, 0, 1, 0 );
            break;
        case 'F':
            line( 0, 0, 0, 1 );
            line( 0, 1, 1, 1 );
            line( 0, .51f, .77f, .51f );
            break;
        case 'I':
            line( .5f, 0, .5f, 1 );
            break;
        case 'K':
            line( 0, 0, 0, 1 );
            line( 0, .45f, 1, 1 );
            line( .31f, .61f, 1, 0 );
            break;
        case 'M':
            line( 0, 0, 0, 1 );
            line( 0, 1, .50f, .38f );
            line( .50f, .38f, 1, 1 );
            line( 1, 1, 1, 0 );
            break;
        case 'O':
            line( .18f, 0, .82f, 0 );
            line( .82f, 0, 1, .20f );
            line( 1, .20f, 1, .80f );
            line( 1, .80f, .82f, 1 );
            line( .82f, 1, .18f, 1 );
            line( .18f, 1, 0, .80f );
            line( 0, .80f, 0, .20f );
            line( 0, .20f, .18f, 0 );
            break;
        case 'R':
            line( 0, 0, 0, 1 );
            line( 0, 1, .83f, 1 );
            line( .83f, 1, 1, .80f );
            line( 1, .80f, 1, .64f );
            line( 1, .64f, .83f, .48f );
            line( .83f, .48f, 0, .48f );
            line( .45f, .48f, 1, 0 );
            break;
        case 'S':
            line( 1, 1, .18f, 1 );
            line( .18f, 1, 0, .80f );
            line( 0, .80f, 0, .66f );
            line( 0, .66f, .18f, .51f );
            line( .18f, .51f, .82f, .51f );
            line( .82f, .51f, 1, .35f );
            line( 1, .35f, 1, .18f );
            line( 1, .18f, .82f, 0 );
            line( .82f, 0, 0, 0 );
            break;
        case 'T':
            line( 0, 1, 1, 1 );
            line( .5f, 1, .5f, 0 );
            break;
        case 'U':
            line( 0, 1, 0, .20f );
            line( 0, .20f, .20f, 0 );
            line( .20f, 0, .80f, 0 );
            line( .80f, 0, 1, .20f );
            line( 1, .20f, 1, 1 );
            break;
        }
    }

    template <class Point, class FaceBox>
    void appendFace( Geometry &facade, Geometry &metal, Geometry &letters, Style style, Vector3F normal,
                     Vector3F along, const Point &point, const FaceBox &box, float door, float halfDoor,
                     float span, float base, float high, float storeyTop )
    {
        const float width = fasciaWidth( span, door );
        const float elevation = storeyTop - .198f;
        box( facade, door, elevation, width, .288f, .075f, .15f );
        box( metal, door, elevation + .147f, width + .035f, .026f, .089f, .17f );

        const bool sloped = style == Style::Cafe;
        const float reach = sloped ? .91f : style == Style::Studio ? .55f : .71f;
        const float drop = sloped ? .184f : .050f;
        const float half = width * ( sloped ? .487f : .505f );
        const float attach = storeyTop - .402f;
        const float nose = attach - drop;
        const float lip = style == Style::Market ? .115f : .072f;
        const float left = door - half, right = door + half;
        const auto a = point( left, attach, -.022f ), b = point( right, attach, -.022f );
        const auto c = point( right, nose, -reach ), d = point( left, nose, -reach );
        const Vector3F down( 0, -lip, 0 );
        const auto topNormal =
            ( Vector3F::unitY() + normal * ( drop / ( reach - .022f ) ) ).normaliseCopy();
        facade.quad( a, b, c, d, topNormal );
        metal.quad( a + down, b + down, c + down, d + down, -topNormal );
        facade.quad( d, c, c + down, d + down, normal );
        facade.quad( a, d, d + down, a + down, -along );
        facade.quad( b, c, c + down, b + down, along );
        box( metal, door, nose - lip + .014f, 2 * half, .026f, reach + .007f, .031f );

        const float jambY = ( base + .23f + high ) * .5f;
        for( const float side : { -1.f, 1.f } )
        {
            box( metal, door + side * ( halfDoor + .035f ), jambY, .077f, high - base - .23f, -.029f,
                 .098f );
        }

        const auto title = style == Style::Market ? "MARKET" : style == Style::Cafe ? "CAFE" : "STUDIO";
        const size_t count = style == Style::Cafe ? 4u : 6u;
        const float glyph = .108f, letterHeight = .145f, advance = .175f;
        const float start = door - ( ( count - 1u ) * advance + glyph ) * .5f;
        const float sign =
            normal.dotProduct( along.crossProduct( Vector3F::unitY() ) ) > 0.f ? 1.f : -1.f;
        for( size_t i = 0; i < count; ++i )
        {
            const float origin = sign > 0 ? start + i * advance : 2.f * door - ( start + i * advance );
            strokes( title[i], [&]( float ax, float ay, float bx, float by ) {
                const Vector2F from( origin + sign * ax * glyph,
                                     elevation + ( ay - .5f ) * letterHeight );
                const Vector2F to( origin + sign * bx * glyph, elevation + ( by - .5f ) * letterHeight );
                const auto d = to - from;
                const float halfStroke = .0090f / std::sqrt( d.lengthSquared() );
                const Vector2F side( -d.y * halfStroke, d.x * halfStroke );
                const auto corner = [&]( Vector2F p ) { return point( p.x, p.y, -.154f ); };
                letters.quad( corner( from + side ), corner( to + side ), corner( to - side ),
                              corner( from - side ), normal );
            } );
        }
    }

    template <class FaceBox>
    void appendDisplay( Geometry &display, Geometry &metal, Style style, const FaceBox &box, float left,
                        float right, float low, float head, float depth, u32 variation )
    {
        const float gap = right - left;
        const float width = std::min( gap - .25f, 1.25f + .14f * float( variation % 3u ) );
        if( width < .72f || head - low < 1.38f )
        {
            return;
        }
        const float mid = ( left + right ) * .5f +
                          ( variation & 4u ? 1.f : -1.f ) * std::min( .24f, ( gap - width ) * .24f );
        const float y = low + .063f;
        if( style == Style::Studio )
        {
            // A pair of framed prints sits inside the reveal. Different aspect
            // ratios and hanging heights keep the two display bays asymmetric.
            const bool portrait = mid < 0.f;
            const float paperWidth = width * ( portrait ? .52f : .77f );
            const float paperHeight = portrait ? .83f : .40f;
            const float sideMargin = paperWidth * .5f + .089f;
            const float s = std::clamp( mid + width * ( portrait ? .11f : -.17f ), left + sideMargin,
                                        right - sideMargin );
            const float centreY =
                std::min( y + ( portrait ? .97f : .72f ), head - paperHeight * .5f - .10f );
            box( metal, s, centreY, paperWidth + .058f, paperHeight + .058f, -depth + .052f, .045f );
            box( display, s, centreY, paperWidth, paperHeight, -depth + .075f, .019f );
            if( portrait )
            {
                box( metal, s - paperWidth * .13f, centreY - .06f, paperWidth * .20f, paperHeight * .53f,
                     -depth + .086f, .004f );
                box( metal, s + paperWidth * .13f, centreY + .15f, paperWidth * .23f, paperHeight * .21f,
                     -depth + .086f, .004f );
            }
            else
            {
                box( metal, s - paperWidth * .13f, centreY - .07f, paperWidth * .47f, paperHeight * .19f,
                     -depth + .086f, .004f );
                box( metal, s + paperWidth * .15f, centreY + .05f, paperWidth * .30f, paperHeight * .28f,
                     -depth + .086f, .004f );
            }
            return;
        }
        box( display, mid, y + .17f, width, .29f, -depth + .039f, .074f );
        box( metal, mid, y + .329f, width + .072f, .031f, -depth + .048f, .095f );
        const float solid = style == Style::Market ? .29f : .19f;
        box( display, mid - width * .22f, y + .348f + solid * .5f, width * .31f, solid, -depth + .037f,
             .065f );
        box( display, mid + width * .22f, y + .348f + solid * .33f, width * .24f, solid * .66f,
             -depth + .043f, .066f );
    }
}  // namespace cityShopfront
