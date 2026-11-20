#pragma once

namespace cityPitchedRoofCladding
{
    struct Surfaces
    {
        Geometry slate, clay, slateCaps, clayCaps, eaves, flashing;
    };

    struct Materials
    {
        SmartPtr<render::IMaterial> slate, clay, slateCaps, clayCaps, eaves, flashing;
    };

    struct Counts
    {
        u32 slopes = 0, courses = 0, caps = 0, eaves = 0;
    };

    constexpr u32 atlasRepeats = 8;
    constexpr float skin = .026f;

    struct CourseSize
    {
        float width, rise;
    };

    CourseSize courseSize( bool clay )
    {
        return clay ? CourseSize{ .255f, .315f } : CourseSize{ .295f, .225f };
    }

    float unit( u64 key )
    {
        return float( render::foliageStableHash( key ) >> 40 ) / 16777216.f;
    }

    SmartPtr<render::IMaterial> tiles( SceneAssets &assets, u32 seed, bool clay )
    {
        constexpr u32 size = 512;
        const auto pitch = courseSize( clay );
        TextureBuffer colour( size, size ), normal( size, size ), roughness( size, size );
        SourceBudgetVector<float> relief( size_t( size ) * size );
        const u64 base =
            ( u64( seed ) << 32 ) ^ ( clay ? 0x7465727261636c61ull : 0x736c617465726f6full );
        const auto pack = []( float v ) { return u8( std::clamp( std::round( v ), 0.f, 255.f ) ); };
        const auto smooth = []( float v ) {
            const float t = std::clamp( v, 0.f, 1.f );
            return t * t * ( 3.f - 2.f * t );
        };
        for( u32 y = 0; y < size; ++y )
        {
            for( u32 x = 0; x < size; ++x )
            {
                const float v = ( y + .5f ) * atlasRepeats / size;
                const auto course = u32( v );
                const float lapMetres = ( v - course ) * pitch.rise;
                const float u =
                    ( x + .5f ) * atlasRepeats / size + ( clay ? 0.f : float( course & 1u ) * .5f );
                const auto tile = u32( u );
                const float across = u - tile;
                const float joint = std::min( across, 1.f - across ) * pitch.width;
                const auto tileKey =
                    base + 19u * ( course % atlasRepeats ) + 137u * ( tile % atlasRepeats );
                const float shade = unit( tileKey + 3 ) - .5f, warm = unit( tileKey + 5 ) - .5f;
                const float grit = unit( base + size_t( y ) * size + x + 11771u ) - .5f;
                const float deposit = unit( base + size_t( y / 3 ) * size + x / 3 + 719u ) - .5f;
                const float lap = smooth( lapMetres / ( clay ? .019f : .013f ) );
                const float seam = smooth( joint / ( clay ? .012f : .008f ) );
                const float arch =
                    std::pow( .5f - .5f * std::cos( 2.f * pi * across ), clay ? .82f : 1.7f );
                const float dwell = shade * ( clay ? 18.f : 19.f ) + grit * ( clay ? 8.f : 5.f ) +
                                    deposit * ( clay ? 10.f : 6.f );
                const float shadow = 24.f * ( 1.f - lap ) + ( clay ? 12.f : 22.f ) * ( 1.f - seam );
                const float red = clay ? 161.f + warm * 12.f : 93.f + warm * 3.f;
                const float green = clay ? 100.f + warm * 7.f : 99.f + warm * 2.f;
                const float blue = clay ? 75.f + warm * 4.f : 101.f - warm * 4.f;
                auto pixel = colour.pixel( x, y );
                pixel[0] = pack( red + dwell - shadow );
                pixel[1] = pack( green + dwell - shadow * .84f );
                pixel[2] = pack( blue + dwell - shadow * .73f );
                pixel[3] = 255;
                pixel = roughness.pixel( x, y );
                pixel[0] = pixel[1] = pixel[2] =
                    pack( 255.f * ( ( clay ? .81f : .84f ) + deposit * .060f + .074f * ( 1.f - lap ) ) );
                pixel[3] = 255;
                relief[size_t( y ) * size + x] = ( clay ? .015f : .0011f ) * arch +
                                                 ( clay ? .0029f : .0018f ) * lap + .0015f * seam +
                                                 .00083f * deposit + .00054f * grit;
            }
        }
        const auto h = [&]( u32 x, u32 y ) { return relief[size_t( y ) * size + x]; };
        for( u32 y = 0; y < size; ++y )
        {
            for( u32 x = 0; x < size; ++x )
            {
                const float dx = ( h( ( x + 1 ) % size, y ) - h( ( x + size - 1 ) % size, y ) ) * size /
                                 ( 2.f * atlasRepeats * pitch.width );
                const float dy = ( h( x, ( y + 1 ) % size ) - h( x, ( y + size - 1 ) % size ) ) * size /
                                 ( 2.f * atlasRepeats * pitch.rise );
                const auto n = Vector3F( -dx, -dy, 1.f ).normaliseCopy();
                const auto pixel = normal.pixel( x, y );
                pixel[0] = pack( 127.5f * ( 1.f + n.x ) );
                pixel[1] = pack( 127.5f * ( 1.f + n.y ) );
                pixel[2] = pack( 127.5f * ( 1.f + n.z ) );
                pixel[3] = 255;
            }
        }
        auto surface = material( assets, ColourF::White, 1.f );
        surface->setTexture( upload( assets, colour ), 0 );
        surface->setTexture( upload( assets, normal, render::TextureMipFilter::Normal ), 1 );
        surface->setTexture( upload( assets, roughness, render::TextureMipFilter::Roughness ), 3 );
        surface->setUVProjection( 0 );
        surface->setNormalStrength( clay ? .88f : .50f );
        surface->setEditorUInt( "uvFilter", 3 );
        surface->setEditorFloat( "uvAniso", 8.f );
        return surface;
    }

    Materials materials( SceneAssets &assets, u32 seed )
    {
        return { tiles( assets, seed, false ),
                 tiles( assets, seed, true ),
                 material( assets, { .35f, .38f, .39f, 1 }, .82f ),
                 material( assets, { .64f, .38f, .28f, 1 }, .81f ),
                 material( assets, { .27f, .25f, .23f, 1 }, .83f ),
                 material( assets, { .375f, .38f, .377f, 1 }, .76f, .22f ) };
    }

    struct Face
    {
        Vector3F eaveA, eaveB, crownB, crownA;
    };

    template <class UV>
    void triangle( Geometry &geometry, Vector3F a, Vector3F b, Vector3F c, Vector3F facing,
                   Vector3F tangent, const UV &uv )
    {
        auto normal = ( b - a ).crossProduct( c - a );
        if( normal.lengthSquared() < 1e-10f )
        {
            return;
        }
        if( normal.dotProduct( facing ) < 0.f )
        {
            std::swap( b, c );
            normal = -normal;
        }
        normal.normalise();
        const std::array coordinates{ uv( a ), uv( b ), uv( c ) };
        const auto deltaU = coordinates[1] - coordinates[0], deltaV = coordinates[2] - coordinates[0];
        const float det = deltaU.x * deltaV.y - deltaU.y * deltaV.x;
        Vector3F bitangent = normal.crossProduct( tangent );
        if( std::abs( det ) > 1e-8f )
        {
            const auto ab = b - a, ac = c - a;
            tangent = ( ab * deltaV.y - ac * deltaU.y ) / det;
            bitangent = ( ac * deltaU.x - ab * deltaV.x ) / det;
        }
        tangent = ( tangent - normal * tangent.dotProduct( normal ) ).normaliseCopy();
        const float handedness =
            normal.crossProduct( tangent ).dotProduct( bitangent ) < 0.f ? -1.f : 1.f;
        const std::array points{ a, b, c };
        for( size_t i = 0; i < points.size(); ++i )
        {
            geometry.indices.push_back( u32( geometry.positions.size() ) );
            geometry.positions.push_back( points[i] );
            geometry.normals.push_back( normal );
            geometry.tangents.push_back( { tangent.x, tangent.y, tangent.z, handedness } );
            geometry.uv.push_back( coordinates[i] );
        }
    }

    template <class RoofPoint, class Admit>
    bool appendFlashing( Surfaces &out, const RoofPoint &roofPoint, float halfLong, float halfShort,
                         float rise, bool hipped, float chimneyU, float chimneyV, float width,
                         float depth, const Admit &admit )
    {
        for( const float value : { halfLong, halfShort, rise, chimneyU, chimneyV, width, depth } )
        {
            if( !std::isfinite( value ) )
            {
                return false;
            }
        }
        if( halfLong < .55f || halfShort < .55f || rise < .12f || width < .12f || depth < .12f )
        {
            return false;
        }
        const float x = width * .5f + .004f, z = depth * .5f + .004f;
        constexpr float apronX = .183f, apronZ = .207f;
        if( std::abs( chimneyU ) + x + apronX >= halfLong - .025f ||
            std::abs( chimneyV ) + z + apronZ >= halfShort - .025f )
        {
            return false;
        }
        const float ridge = hipped ? std::max( .10f, halfLong - .75f * halfShort ) : halfLong;
        const u32 planes = hipped ? 4u : 2u;
        // The roof is the lower envelope of these four planes. Splitting
        // on their intersections keeps each lead skirt flush across a crease.
        const auto height = [&]( u32 plane, Vector2F p ) {
            if( plane < 2u )
            {
                return rise * ( 1.f + ( plane ? p.y : -p.y ) / halfShort );
            }
            return rise * ( halfLong + ( plane == 3u ? p.x : -p.x ) ) / ( halfLong - ridge );
        };
        const auto corners = [&]( float a, float b ) {
            return std::array{ Vector2F( chimneyU - a, chimneyV - b ),
                               Vector2F( chimneyU + a, chimneyV - b ),
                               Vector2F( chimneyU + a, chimneyV + b ),
                               Vector2F( chimneyU - a, chimneyV + b ) };
        };
        const auto inner = corners( x, z ), outer = corners( x + apronX, z + apronZ );
        const auto centre = roofPoint( chimneyU, 0.f, chimneyV );
        const auto uv = []( Vector3F p ) { return Vector3F( .67f * p.x, .67f * p.z, 0 ); };
        // The built-in masonry penetrates the deck. At the downhill side of a
        // gable, continue the vertical lead far enough to cover the masonry toe.
        const float masonryBase = hipped ? rise * .63f - .09f : rise - .075f;
        const Vector3F up( 0, 1, 0 );
        Geometry candidate;
        for( size_t i = 0; i < inner.size(); ++i )
        {
            const size_t j = ( i + 1u ) % inner.size();
            const auto a = inner[i], b = inner[j];
            const auto tangent =
                ( roofPoint( b.x, 0.f, b.y ) - roofPoint( a.x, 0.f, a.y ) ).normaliseCopy();
            const auto middle = ( a + b ) * .5f;
            const auto outward = ( roofPoint( middle.x, 0.f, middle.y ) - centre ).normaliseCopy();
            for( u32 plane = 0; plane < planes; ++plane )
            {
                std::array<Vector2F, 8> polygon{ a, b, outer[j], outer[i] }, next{};
                size_t count = 4;
                float start = 0.f, stop = 1.f;
                for( u32 other = 0; other < planes; ++other )
                {
                    if( plane == other )
                    {
                        continue;
                    }
                    const auto delta = [&]( Vector2F p ) {
                        return height( plane, p ) - height( other, p );
                    };
                    size_t used = 0;
                    for( size_t k = 0; k < count; ++k )
                    {
                        const auto p = polygon[k], q = polygon[( k + 1u ) % count];
                        const float dp = delta( p ), dq = delta( q );
                        if( dp <= 0.f )
                        {
                            next[used++] = p;
                        }
                        if( ( dp < 0.f && dq > 0.f ) || ( dp > 0.f && dq < 0.f ) )
                        {
                            next[used++] = p + ( q - p ) * ( dp / ( dp - dq ) );
                        }
                    }
                    polygon.swap( next );
                    count = used;
                    const float da = delta( a ), db = delta( b );
                    if( ( da > 0.f && db > 0.f ) || ( da == 0.f && db == 0.f && other < plane ) )
                    {
                        stop = 0.f;
                    }
                    else if( da > 0.f )
                    {
                        start = std::max( start, da / ( da - db ) );
                    }
                    else if( db > 0.f )
                    {
                        stop = std::min( stop, da / ( da - db ) );
                    }
                }
                const auto deck = [&]( Vector2F p ) {
                    return roofPoint( p.x, height( plane, p ) + skin + .023f, p.y );
                };
                for( size_t k = 1; k + 1u < count; ++k )
                {
                    triangle( candidate, deck( polygon[0] ), deck( polygon[k] ), deck( polygon[k + 1u] ),
                              up, tangent, uv );
                }
                if( stop - start < .0001f )
                {
                    continue;
                }
                const auto p = a + ( b - a ) * start, q = a + ( b - a ) * stop;
                const float y0 = height( plane, p ), y1 = height( plane, q );
                const auto lowP = roofPoint( p.x, y0 + skin + .008f, p.y );
                const auto lowQ = roofPoint( q.x, y1 + skin + .008f, q.y );
                const auto highP = roofPoint( p.x, std::max( y0 + .142f, masonryBase + .10f ), p.y );
                const auto highQ = roofPoint( q.x, std::max( y1 + .142f, masonryBase + .10f ), q.y );
                triangle( candidate, lowP, lowQ, highQ, outward, tangent, uv );
                triangle( candidate, lowP, highQ, highP, outward, tangent, uv );
            }
        }
        if( candidate.indices.empty() || !admit( candidate.positions.size() ) )
        {
            return false;
        }
        auto &target = out.flashing;
        const auto base = u32( target.positions.size() );
        for( const auto index : candidate.indices )
        {
            target.indices.push_back( base + index );
        }
        target.positions.insert( target.positions.end(), candidate.positions.begin(),
                                 candidate.positions.end() );
        target.normals.insert( target.normals.end(), candidate.normals.begin(),
                               candidate.normals.end() );
        target.tangents.insert( target.tangents.end(), candidate.tangents.begin(),
                                candidate.tangents.end() );
        target.uv.insert( target.uv.end(), candidate.uv.begin(), candidate.uv.end() );
        return true;
    }

    template <class Admit>
    void cladFace( Surfaces &out, Counts &counts, const Face &face, bool clay, float phase,
                   const Admit &admit, bool detailed )
    {
        const auto pitch = courseSize( clay );
        auto &surface = clay ? out.clay : out.slate;
        const Vector3F up( 0, 1, 0 );
        const auto mix = []( Vector3F a, Vector3F b, float t ) { return a + ( b - a ) * t; };
        const auto along = ( face.eaveB - face.eaveA ).normaliseCopy();
        auto run = ( face.crownA + face.crownB - face.eaveA - face.eaveB ) * .5f;
        run -= along * run.dotProduct( along );
        const float length = run.length();
        if( length < .05f )
        {
            return;
        }
        const auto climb = run / length;
        auto outward = -climb;
        outward.y = 0;
        outward.normalise();
        const auto uv = [&]( Vector3F p ) {
            const auto local = p - face.eaveA - up * skin;
            return Vector3F( phase + local.dotProduct( along ) / ( atlasRepeats * pitch.width ),
                             local.dotProduct( climb ) / ( atlasRepeats * pitch.rise ), 0 );
        };
        const auto skinA = face.eaveA + up * skin, skinB = face.eaveB + up * skin;
        const auto skinC = face.crownB + up * skin, skinD = face.crownA + up * skin;
        triangle( surface, skinA, skinB, skinC, up, along, uv );
        triangle( surface, skinA, skinC, skinD, up, along, uv );
        ++counts.slopes;
        if( !detailed )
        {
            return;
        }
        if( admit( 4u ) )
        {
            out.eaves.quad( face.eaveA - up * .031f, face.eaveB - up * .031f, skinB, skinA,
                            outward );
            ++counts.eaves;
        }
        const u32 rows = u32( length / pitch.rise );
        const u32 stride = std::max( 1u, ( rows + 11u ) / 12u );
        const float lip = clay ? .018f : .010f;
        for( u32 course = 1; course <= rows; course += stride )
        {
            const float first = course * pitch.rise;
            const float width = clay ? .054f : .036f;
            if( first + width + .067f > length || !admit( 12u ) )
            {
                break;
            }
            const float lo = first / length, hi = ( first + width ) / length;
            const auto a = mix( skinA, skinD, lo ), b = mix( skinB, skinC, lo );
            const auto c = mix( skinB, skinC, hi ), d = mix( skinA, skinD, hi );
            triangle( surface, a + up * lip, b + up * lip, c, up, along, uv );
            triangle( surface, a + up * lip, c, d, up, along, uv );
            triangle( surface, a, b, b + up * lip, outward, along, uv );
            triangle( surface, a, b + up * lip, a + up * lip, outward, along, uv );
            ++counts.courses;
        }
    }

    template <class RoofPoint, class Admit>
    Counts append( Surfaces &out, const RoofPoint &roofPoint, float halfLong, float halfShort,
                   float rise, bool hipped, bool clay, u64 lotId, u32 seed, const Admit &admit,
                   bool detailed = true )
    {
        Counts counts;
        if( !std::isfinite( halfLong ) || !std::isfinite( halfShort ) || !std::isfinite( rise ) ||
            halfLong < .55f || halfShort < .55f || rise < .12f )
        {
            return counts;
        }
        const float ridge = hipped ? std::max( .1f, halfLong - .75f * halfShort ) : halfLong;
        const u64 key =
            render::foliageStableHash( lotId ^ ( u64( seed ) << 32 ) ^ 0x7069746368726f6full );
        const float phase = float( key & 3u ) * .25f;
        auto &caps = clay ? out.clayCaps : out.slateCaps;
        const auto appendFace = [&]( const Face &face ) {
            cladFace( out, counts, face, clay, phase, admit, detailed );
        };

        const auto capped = [&]( Vector2F a, Vector2F b, bool hip, u32 salt ) {
            if( !detailed )
            {
                return;
            }
            const auto axis = b - a;
            const float distance = axis.length();
            if( distance < .10f )
            {
                return;
            }
            const Vector2F side( -axis.y / distance, axis.x / distance );
            const float minT = hip ? std::min( .21f, .19f / distance ) : 0.f;
            const u32 panels = clay ? 6u : 4u;
            u32 pieces =
                std::clamp( u32( std::ceil( distance / ( clay ? .48f : .62f ) ) ), 1u, hip ? 8u : 12u );
            for( ; pieces > 1 && !admit( pieces * panels * 4u ); pieces /= 2u )
            {
            }
            if( pieces == 1 && !admit( panels * 4u ) )
            {
                return;
            }
            const float halfCap = clay ? .145f : .099f;
            const auto section = [&]( float t, float s, float bulge ) {
                auto local = a + axis * t + side * ( halfCap * s );
                local.x = std::clamp( local.x, -halfLong, halfLong );
                local.y = std::clamp( local.y, -halfShort, halfShort );
                const float flank = rise * ( 1.f - std::abs( local.y ) / halfShort );
                const float h = hipped ? std::min( flank, rise * ( halfLong - std::abs( local.x ) ) /
                                                              ( halfLong - ridge ) )
                                       : flank;
                const float arch = std::sqrt( std::max( 0.f, 1.f - s * s ) );
                return roofPoint( local.x, h + skin - .004f + arch * bulge, local.y );
            };
            for( u32 j = 0; j < pieces; ++j )
            {
                const float jitter = ( unit( key + salt + j ) - .5f ) * .007f;
                const float bulge = ( clay ? .136f : .086f ) + jitter;
                const float start = minT + ( 1.f - minT ) * j / pieces;
                const float stop = minT + ( 1.f - minT ) * ( j + 1u ) / pieces;
                for( u32 k = 0; k < panels; ++k )
                {
                    const float lo = -1.f + 2.f * k / panels, hi = -1.f + 2.f * ( k + 1u ) / panels;
                    const auto p = section( start, lo, bulge ), q = section( stop, lo, bulge );
                    const auto r = section( stop, hi, bulge ), s = section( start, hi, bulge );
                    auto n = ( q - p ).crossProduct( s - p ).normaliseCopy();
                    if( n.y < 0 )
                    {
                        n = -n;
                    }
                    caps.quad( p, q, r, s, n );
                }
                ++counts.caps;
            }
        };

        // Cap budget is offered first, so large roofs retain their ridge
        // and hips before spending remaining vertices on individual laps.
        capped( { -ridge, 0 }, { ridge, 0 }, false, 31u );
        for( const float side : { -1.f, 1.f } )
        {
            if( hipped )
            {
                capped( { -halfLong, side * halfShort }, { -ridge, 0 }, true, side > 0 ? 209u : 127u );
                capped( { halfLong, side * halfShort }, { ridge, 0 }, true, side > 0 ? 707u : 521u );
            }
            appendFace( { roofPoint( -halfLong, 0, side * halfShort ),
                          roofPoint( halfLong, 0, side * halfShort ), roofPoint( ridge, rise, 0 ),
                          roofPoint( -ridge, rise, 0 ) } );
            if( hipped )
            {
                const auto crown = roofPoint( side * ridge, rise, 0 );
                appendFace( { roofPoint( side * halfLong, 0, -halfShort ),
                              roofPoint( side * halfLong, 0, halfShort ), crown, crown } );
            }
        }
        return counts;
    }
}  // namespace cityPitchedRoofCladding
