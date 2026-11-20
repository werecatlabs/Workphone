#pragma once

namespace cityMansardRoof
{
    constexpr float panWidth = .59f;
    constexpr float sheetRepeat = 4.f * panWidth;
    constexpr float sheetLength = 3.25f;

    SmartPtr<render::IMaterial> zincMaterial( SceneAssets &assets, u32 seed )
    {
        constexpr u32 size = 256;
        TextureBuffer colour( size, size ), normals( size, size ), roughness( size, size );
        const auto byte = []( float v ) { return u8( std::clamp( v, 0.f, 255.f ) + .5f ); };
        const auto fold = []( float u ) {
            const float d = std::abs( u - std::round( u ) ) * panWidth;
            const float t = std::min( 1.f, d / .026f );
            return .024f * ( 1.f - t * t * ( 3.f - 2.f * t ) );
        };
        for( u32 x = 0; x < size; ++x )
        {
            const float pan = ( x + .5f ) * 4.f / size;
            const float ripple = .0013f * std::sin( 2.f * pi * pan );
            const float slope = ( fold( pan + 4.f / size ) - fold( pan - 4.f / size ) ) /
                                ( 2.f * sheetRepeat / size ) + ripple;
            const auto n = Vector3F( -slope, 0, 1 ).normaliseCopy();
            const float patina = 8.f * ( cityPitchedRoofCladding::unit( u64( seed ) + u32( pan ) ) - .5f );
            for( u32 y = 0; y < size; ++y )
            {
                const float v = ( y + .5f ) / size;
                const float grain = cityPitchedRoofCladding::unit( ( u64( seed ) << 32 ) + y * size + x );
                const float weather = 2.9f * std::sin( pi * ( pan + 2.f * v ) ) +
                                      2.1f * std::cos( pi * ( 2.f * pan - 4.f * v ) ) + 5.f * ( grain - .5f );
                const float tone = 126.f + patina + weather + fold( pan ) * 360.f;
                auto *p = colour.pixel( x, y );
                p[0] = byte( tone - 6.f );
                p[1] = byte( tone );
                p[2] = byte( tone + 3.f );
                p[3] = 255;
                p = normals.pixel( x, y );
                p[0] = byte( ( .5f + .5f * n.x ) * 255.f );
                p[1] = 128;
                p[2] = byte( ( .5f + .5f * n.z ) * 255.f );
                p[3] = 255;
                p = roughness.pixel( x, y );
                p[0] = p[1] = p[2] = byte( 184.f + .9f * weather + patina );
                p[3] = 255;
            }
        }
        auto surface = material( assets, ColourF::White, 1.f, .22f );
        surface->setTexture( upload( assets, colour ), 0 );
        surface->setTexture( upload( assets, normals, render::TextureMipFilter::Normal ), 1 );
        surface->setTexture( upload( assets, roughness, render::TextureMipFilter::Roughness ), 3 );
        surface->setUVProjection( 0 );
        surface->setNormalStrength( .45f );
        surface->setEditorUInt( "uvFilter", 3 );
        surface->setEditorFloat( "uvAniso", 8.f );
        return surface;
    }

    void zincFace( Geometry &zinc, const cityPitchedRoofCladding::Face &f, float phase )
    {
        const auto along = ( f.eaveB - f.eaveA ).normaliseCopy();
        auto climb = ( f.crownA + f.crownB - f.eaveA - f.eaveB ) * .5f;
        climb = ( climb - along * climb.dotProduct( along ) ).normaliseCopy();
        const auto uv = [&]( Vector3F p ) {
            const auto local = p - f.eaveA;
            return Vector3F( phase + local.dotProduct( along ) / sheetRepeat,
                             local.dotProduct( climb ) / sheetLength, 0 );
        };
        cityPitchedRoofCladding::triangle( zinc, f.eaveA, f.eaveB, f.crownB, Vector3F::unitY(), along, uv );
        cityPitchedRoofCladding::triangle( zinc, f.eaveA, f.crownB, f.crownA, Vector3F::unitY(), along, uv );
    }

    // A fold uses a flat part of the zinc pan. Its two metal faces supply the relief.
    void metalStrip( Geometry &zinc, Vector3F a, Vector3F b, Vector3F c, Vector3F d )
    {
        auto n = ( b - a ).crossProduct( d - a ).normaliseCopy();
        zinc.quad( a, b, c, d, n.y < 0 ? -n : n, .10f, .10f, .30f, .35f );
    }

    template <class Admit>
    void upperPitch( Geometry &zinc, const cityPitchedRoofCladding::Face &f, float phase,
                     const Admit &admit, bool detailed )
    {
        zincFace( zinc, f, phase );
        if( !detailed ) return;
        const auto edge = f.eaveB - f.eaveA;
        const float width = edge.length();
        const auto along = edge / width;
        const auto climb = ( f.crownA + f.crownB - f.eaveA - f.eaveB ) * .5f;
        const float left = ( f.crownA - f.eaveA ).dotProduct( along );
        const float right = ( f.crownB - f.eaveA ).dotProduct( along );
        const float offset = phase * sheetRepeat;
        const float first = std::ceil( ( offset + .08f ) / panWidth ) * panWidth - offset;
        if( first > width - .08f ) return;
        const auto count = u32( ( width - .08f - first ) / panWidth ) + 1;
        if( !admit( 8u * count ) ) return;
        auto normal = along.crossProduct( climb ).normaliseCopy();
        if( normal.y < 0 ) normal = -normal;
        const auto halfFold = along * .017f, crest = normal * .024f;
        for( u32 i = 0; i < count; ++i )
        {
            const float x = first + i * panWidth;
            // Clip both margins of the fold at the hip, including a pointed crown.
            const float end = std::min( { 1.f, ( x - .022f ) / left, ( width - x - .022f ) / ( width - right ) } );
            const auto start = f.eaveA + along * x + normal * .002f;
            const auto stop = start + climb * end;
            metalStrip( zinc, start - halfFold, stop - halfFold, stop + crest, start + crest );
            metalStrip( zinc, start + crest, stop + crest, stop + halfFold, start + halfFold );
        }
    }

    template <class Point>
    void dormer( Geometry &stone, Geometry &zinc, Geometry &glass, const MansardProfile &p,
                 const Point &point, Vector3F normal, u32 pane, bool arched, bool detailed )
    {
        constexpr float half = .655f, light = .47f, sill = .65f, head = 1.80f, shoulder = 1.95f;
        const float front = p.inset * .15f, glazing = front + .082f;
        const float base = p.breakHeight * .15f + cityPitchedRoofCladding::skin;
        const auto along = ( point( 1, 0, 0 ) - point( 0, 0, 0 ) ).normaliseCopy();
        const auto face = [&]( float left, float low, float right, float top, float depth ) {
            stone.quad( point( left, low, depth ), point( right, low, depth ),
                        point( right, top, depth ), point( left, top, depth ), normal );
        };
        face( -half, base, half, sill, front );
        face( -half, sill, -light, head, front );
        face( light, sill, half, head, front );
        face( -half, head, half, shoulder, front );
        const float paneDepth = detailed ? glazing : front;
        const auto a = point( -light, sill, paneDepth ), b = point( light, sill, paneDepth );
        const auto c = point( light, head, paneDepth ), d = point( -light, head, paneDepth );
        if( detailed )
        {
            cityBuildingGlazing::pane( glass, cityBuildingGlazing::domesticStart +
                cityBuildingGlazing::paneVariants + pane, a, b, c, d, normal );
            stone.quad( point( -light, sill, front ), a, d, point( -light, head, front ), along );
            stone.quad( b, point( light, sill, front ), point( light, head, front ), c, -along );
            stone.quad( point( -light, sill, front ), point( light, sill, front ), b, a, Vector3F::unitY() );
            stone.quad( d, c, point( light, head, front ), point( -light, head, front ), -Vector3F::unitY() );
        }
        else
        {
            glass.quad( a, b, c, d, normal );
        }

        // Both the cheek and the back of the hood terminate inside the slate pitch.
        const float capFront = front - .105f, eave = shoulder + .045f;
        const auto depthUV = [&]( Vector3F v ) {
            const auto local = v - point( 0, base, front );
            return Vector3F( -local.dotProduct( normal ) / sheetRepeat, local.y / sheetLength, 0 );
        };
        for( const float side : { -1.f, 1.f } )
        {
            const float x = side * half;
            cityPitchedRoofCladding::triangle( zinc, point( x, base - .012f, front ),
                point( x, eave, front ), point( x, eave, p.hoodBack( eave ) ), along * side, -normal, depthUV );
        }
        const int spans = arched && detailed ? 4 : 2;
        const auto height = [&]( float t ) {
            return eave + ( arched ? .23f * ( 1.f - t * t ) : ( p.hoodPeak - eave ) * ( 1.f - std::abs( t ) ) );
        };
        for( int i = 0; i < spans; ++i )
        {
            const float t0 = 2.f * i / spans - 1.f, t1 = 2.f * ( i + 1 ) / spans - 1.f;
            const float x0 = t0 * p.hoodHalfWidth, x1 = t1 * p.hoodHalfWidth;
            const float h0 = height( t0 ), h1 = height( t1 );
            const auto low = point( x0, h0, capFront ), high = point( x1, h1, capFront );
            zincFace( zinc, { low, point( x0, h0, p.hoodBack( h0 ) ),
                              point( x1, h1, p.hoodBack( h1 ) ), high }, .25f );
            stone.quad( point( x0, shoulder, front ), point( x1, shoulder, front ),
                        point( x1, h1 - .018f, front ), point( x0, h0 - .018f, front ), normal );
            const auto lip = Vector3F::unitY() * .046f;
            zinc.quad( low - lip, high - lip, high, low, normal );
            if( detailed )
            {
                auto underside = ( high - low ).crossProduct( normal ).normaliseCopy();
                if( underside.y > 0 ) underside = -underside;
                zinc.quad( low - lip, high - lip, point( x1, h1, front ) - lip,
                           point( x0, h0, front ) - lip, underside );
            }
        }
        if( detailed )
        {
            const float toe = front * .40f;
            const auto level = [&]( float depth ) {
                return p.breakHeight * depth / p.inset + cityPitchedRoofCladding::skin + .013f;
            };
            zincFace( zinc, { point( -half - .07f, level( toe ), toe ), point( half + .07f, level( toe ), toe ),
                              point( half + .07f, level( front ), front ), point( -half - .07f, level( front ), front ) }, 0 );
        }
    }

    template <class Point>
    void flashJunctions( Geometry &zinc, const MansardProfile &p, const Point &point )
    {
        const float a = p.halfLength - p.inset, b = p.halfWidth - p.inset;
        const float ridge = p.ridgeHalfLength();
        // Tuck the ends of the zinc pans beneath the hip rolls and dress the
        // pitch break down over the top slate course.
        const auto metal = [&]( Vector2F plan, float lift ) {
            return point( plan.x, p.height( plan.x, plan.y ) + cityPitchedRoofCladding::skin + lift,
                          plan.y );
        };
        const auto flash = [&]( Vector2F from, Vector2F to, float width, float crest ) {
            const auto axis = ( to - from ).normaliseCopy();
            const Vector2F wing( -axis.y * width, axis.x * width );
            const auto start = metal( from, crest ), stop = metal( to, crest );
            metalStrip( zinc, metal( from - wing, .030f ), metal( to - wing, .030f ), stop, start );
            metalStrip( zinc, start, stop, metal( to + wing, .030f ), metal( from + wing, .030f ) );
        };
        const std::array<Vector2F, 4> signs{
            Vector2F( -1, -1 ), Vector2F( 1, -1 ), Vector2F( 1, 1 ), Vector2F( -1, 1 ) };
        const auto rim = [&]( Vector2F corner, float offset, float lift ) {
            return metal( { corner.x * ( a + offset ), corner.y * ( b + offset ) }, lift );
        };
        for( size_t i = 0; i < signs.size(); ++i )
        {
            const auto corner = signs[i], next = signs[( i + 1 ) % signs.size()];
            metalStrip( zinc, rim( corner, .047f, .010f ), rim( next, .047f, .010f ),
                        rim( next, 0, .046f ), rim( corner, 0, .046f ) );
            metalStrip( zinc, rim( corner, 0, .046f ), rim( next, 0, .046f ),
                        rim( next, -.11f, .031f ), rim( corner, -.11f, .031f ) );
            flash( { corner.x * ( a - .08f ), corner.y * ( b - .08f ) },
                   { corner.x * ( ridge + .035f ), corner.y * .035f }, .073f, .076f );
        }
        if( ridge > .12f ) flash( { -ridge, 0 }, { ridge, 0 }, .094f, .083f );
        const auto saddle = [&]( float centre, float reach ) {
            const auto tip = metal( { centre, 0 }, .103f );
            const auto corner = [&]( Vector2F sign ) {
                return metal( { centre + sign.x * reach, sign.y * .125f }, .047f );
            };
            const auto uv = []( Vector3F ) { return Vector3F( .10f, .30f, 0 ); };
            for( size_t i = 0; i < signs.size(); ++i )
                cityPitchedRoofCladding::triangle( zinc, corner( signs[i] ),
                    corner( signs[( i + 1 ) % signs.size()] ), tip, Vector3F::unitY(), Vector3F::unitX(), uv );
        };
        if( ridge > .12f )
        {
            saddle( -ridge, .12f );
            saddle( ridge, .12f );
        }
        else
        {
            saddle( 0, ridge + .12f );
        }
    }

    template <class Point, class Admit>
    void cover( cityPitchedRoofCladding::Surfaces &cladding, Geometry &zinc, const MansardProfile &p,
                const Point &point, u64 lotId, const Admit &admit, bool detailed )
    {
        const float l = p.halfLength, w = p.halfWidth, a = l - p.inset, b = w - p.inset;
        const float ridge = p.ridgeHalfLength();
        const float phase = float( render::foliageStableHash( lotId ) & 7u ) / 8.f;
        const float upper = p.breakHeight + cityPitchedRoofCladding::skin;
        const float peak = upper + p.upperRise;
        if( detailed && admit( 96 ) ) flashJunctions( zinc, p, point );
        cityPitchedRoofCladding::Counts counts;
        for( float side : { -1.f, 1.f } )
        {
            cityPitchedRoofCladding::cladFace( cladding, counts,
                { point( -l, 0, side * w ), point( l, 0, side * w ),
                  point( a, p.breakHeight, side * b ), point( -a, p.breakHeight, side * b ) },
                false, phase, admit, detailed );
            cityPitchedRoofCladding::cladFace( cladding, counts,
                { point( side * l, 0, -w ), point( side * l, 0, w ),
                  point( side * a, p.breakHeight, b ), point( side * a, p.breakHeight, -b ) },
                false, phase, admit, detailed );
            upperPitch( zinc, { point( -a, upper, side * b ), point( a, upper, side * b ),
                                point( ridge, peak, 0 ), point( -ridge, peak, 0 ) }, phase, admit, detailed );
            const auto tip = point( side * ridge, peak, 0 );
            upperPitch( zinc, { point( side * a, upper, -b ), point( side * a, upper, b ), tip, tip },
                        phase, admit, detailed );
        }
    }

    void flue( Geometry &clay, Geometry &soot, Vector3F foot, float height, bool detailed )
    {
        const u32 sides = detailed ? 8 : 5;
        const auto band = [&]( float r0, float h0, float r1, float h1 ) {
            const auto base = u32( clay.positions.size() );
            for( u32 i = 0; i < sides; ++i )
            {
                const float angle = 2.f * pi * i / sides;
                const Vector3F radial( std::cos( angle ), 0, std::sin( angle ) );
                const auto n = ( radial * ( h1 - h0 ) - Vector3F::unitY() * ( r1 - r0 ) ).normaliseCopy();
                for( const auto ring : { Vector2F( r0, h0 ), Vector2F( r1, h1 ) } )
                {
                    clay.positions.push_back( foot + radial * ring.x + Vector3F::unitY() * ring.y );
                    clay.normals.push_back( n );
                    clay.tangents.push_back( { -radial.z, 0, radial.x, 1 } );
                    clay.uv.push_back( { float( i ) / sides, ring.y, 0 } );
                }
                const auto a = base + 2 * i, b = base + 2 * ( ( i + 1 ) % sides );
                for( const auto index : { a, a + 1, b + 1, a, b + 1, b } )
                    clay.indices.push_back( index );
            }
        };
        if( detailed )
        {
            band( .146f, 0, .119f, height - .085f );
            band( .119f, height - .085f, .147f, height - .066f );
            band( .147f, height - .066f, .147f, height );
        }
        else
        {
            band( .146f, 0, .14f, height );
        }
        band( detailed ? .147f : .14f, height, .094f, height );
        band( .094f, height, .085f, .012f );
        const auto bottom = foot + Vector3F::unitY() * .014f;
        const auto x = Vector3F::unitX() * .086f, z = Vector3F::unitZ() * .086f;
        soot.quad( bottom - x - z, bottom + x - z, bottom + x + z, bottom - x + z, Vector3F::unitY() );
    }

    template <class Point, class Box>
    void chimney( Geometry &brick, Geometry &coping, Geometry &clay, Geometry &soot, Geometry &zinc,
                  const MansardProfile &roof, const MansardProfile::Stack &s, const Point &point,
                  const Box &box, bool detailed )
    {
        const auto ring = [&]( float gap ) {
            const float x = s.halfLength + gap, z = s.halfWidth + gap;
            return std::array{ Vector2F( s.u - x, s.v - z ), Vector2F( s.u + x, s.v - z ),
                               Vector2F( s.u + x, s.v + z ), Vector2F( s.u - x, s.v + z ) };
        };
        const auto inner = ring( .006f ), outer = ring( .185f );
        float bottom = s.coping;
        for( auto corner : inner ) bottom = std::min( bottom, roof.height( corner.x, corner.y ) );
        box( brick, s.u, ( bottom + s.coping ) * .5f, s.v, 2.f * s.halfLength,
             s.coping - bottom, 2.f * s.halfWidth );
        box( coping, s.u, s.coping + .06f, s.v, 2.f * s.halfLength + .14f, .12f,
             2.f * s.halfWidth + .14f );
        if( detailed )
        {
            const auto lead = [&]( Vector2F corner, float lift ) {
                return point( corner.x, roof.height( corner.x, corner.y ) + cityPitchedRoofCladding::skin + lift,
                              corner.y );
            };
            for( size_t i = 0; i < inner.size(); ++i )
            {
                const size_t j = ( i + 1 ) % inner.size();
                const auto a = lead( inner[i], .074f ), b = lead( inner[j], .074f );
                metalStrip( zinc, a, b, lead( outer[j], .032f ), lead( outer[i], .032f ) );
                const auto mid = ( inner[i] + inner[j] ) * .5f;
                const auto outward = ( point( mid.x, 0, mid.y ) - point( s.u, 0, s.v ) ).normaliseCopy();
                const auto upstand = Vector3F::unitY() * .128f;
                zinc.quad( a, b, b + upstand, a + upstand, outward, .10f, .10f, .1f, .2f );
            }
        }
        for( int i = 0; i < s.pots; ++i )
        {
            const float v = s.v + ( i - ( s.pots - 1 ) * .5f ) * .40f;
            flue( clay, soot, point( s.u, s.coping + .12f, v ), .37f + .055f * ( i % 2 ), detailed );
        }
    }
}
