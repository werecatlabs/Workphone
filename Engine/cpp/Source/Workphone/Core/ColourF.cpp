#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Math.hpp>

namespace workphone
{
    const ColourF ColourF::ZERO( 0, 0, 0, 0 );
    const ColourF ColourF::Black( 0, 0, 0 );
    const ColourF ColourF::White( 1, 1, 1 );
    const ColourF ColourF::Red( 1, 0, 0 );
    const ColourF ColourF::Green( 0, 1, 0 );
    const ColourF ColourF::Blue( 0, 0, 1 );

    ColourF::ColourF() = default;

    ColourF::ColourF( ColourI c )
    {
        const auto inv = 1.0f / 255.0f;
        r = static_cast<f32>( c.getRed() ) * inv;
        g = static_cast<f32>( c.getGreen() ) * inv;
        b = static_cast<f32>( c.getBlue() ) * inv;
        a = static_cast<f32>( c.getAlpha() ) * inv;
    }

    ColourF::ColourF( f32 r, f32 g, f32 b, f32 a ) : r( r ), g( g ), b( b ), a( a )
    {
    }

    ColourF::ColourF( f32 r, f32 g, f32 b ) : r( r ), g( g ), b( b ), a( 1.0f )
    {
    }

    ColourF::~ColourF() = default;

    void ColourF::setColorComponentValue( s32 index, f32 value )
    {
        switch( index )
        {
        case 0:
            r = value;
            break;
        case 1:
            g = value;
            break;
        case 2:
            b = value;
            break;
        case 3:
            a = value;
            break;
        }
    }

    auto ColourF::getInterpolated_quadratic( const ColourF &c1, const ColourF &c2, const f32 d ) const
        -> ColourF
    {
        // this*(1-d)*(1-d) + 2 * c1 * (1-d) + c2 * d * d;
        const auto inv = 1.0f - d;
        const auto mul0 = inv * inv;
        const auto mul1 = 2.0f * d * inv;
        const auto mul2 = d * d;

        return { r * mul0 + c1.r * mul1 + c2.r * mul2, g * mul0 + c1.g * mul1 + c2.g * mul2,
                 g * mul0 + c1.b * mul1 + c2.b * mul2, a * mul0 + c1.a * mul1 + c2.a * mul2 };
    }

    auto ColourF::getInterpolated( const ColourF &other, f32 d ) const -> ColourF
    {
        const auto inv = 1.0f - d;
        return { other.r * inv + r * d, other.g * inv + g * d, other.b * inv + b * d,
                 other.a * inv + a * d };
    }

    void ColourF::set( f32 aa, f32 rr, f32 gg, f32 bb )
    {
        a = aa;
        r = rr;
        g = gg;
        b = bb;
    }

    void ColourF::set( f32 rr, f32 gg, f32 bb )
    {
        r = rr;
        g = gg;
        b = bb;
    }

    auto ColourF::toSColor() const -> ColourI
    {
        return { static_cast<u32>( static_cast<f32>( a ) * 255.0f ),
                 static_cast<u32>( static_cast<f32>( r ) * 255.0f ),
                 static_cast<u32>( static_cast<f32>( g ) * 255.0f ),
                 static_cast<u32>( static_cast<f32>( b ) * 255.0f ) };
    }

#if WP_ENDIAN == WP_ENDIAN_BIG
    ABGR ColourF::getAsABGR( void ) const
#else
    auto ColourF::getAsRGBA() const -> RGBA
#endif
    {
        u8 val8;
        u32 val32 = 0;

        // Convert to 32bit pattern
        // (RGBA = 8888)

        // Red
        val8 = static_cast<u8>( r * 255 );
        val32 = static_cast<u32>( val8 << 24 );

        // Green
        val8 = static_cast<u8>( g * 255 );
        val32 += static_cast<u32>( val8 << 16 );

        // Blue
        val8 = static_cast<u8>( b * 255 );
        val32 += static_cast<u32>( val8 << 8 );

        // Alpha
        val8 = static_cast<u8>( a * 255 );
        val32 += static_cast<u32>( val8 );

        return val32;
    }

#if WP_ENDIAN == WP_ENDIAN_BIG
    BGRA ColourF::getAsBGRA() const
#else
    auto ColourF::getAsARGB() const -> ARGB
#endif
    {
        u8 val8;
        u32 val32 = 0;

        // Convert to 32bit pattern
        // (ARGB = 8888)

        // Alpha
        val8 = static_cast<u8>( a * 255 );
        val32 = static_cast<u32>( val8 << 24 );

        // Red
        val8 = static_cast<u8>( r * 255 );
        val32 += static_cast<u32>( val8 << 16 );

        // Green
        val8 = static_cast<u8>( g * 255 );
        val32 += static_cast<u32>( val8 << 8 );

        // Blue
        val8 = static_cast<u8>( b * 255 );
        val32 += static_cast<u32>( val8 );

        return val32;
    }

#if WP_ENDIAN == WP_ENDIAN_BIG
    ARGB ColourF::getAsARGB() const
#else
    auto ColourF::getAsBGRA() const -> BGRA
#endif
    {
        u8 val8;
        u32 val32 = 0;

        // Convert to 32bit pattern
        // (ARGB = 8888)

        // Blue
        val8 = static_cast<u8>( b * 255 );
        val32 = static_cast<u32>( val8 << 24 );

        // Green
        val8 = static_cast<u8>( g * 255 );
        val32 += static_cast<u32>( val8 << 16 );

        // Red
        val8 = static_cast<u8>( r * 255 );
        val32 += static_cast<u32>( val8 << 8 );

        // Alpha
        val8 = static_cast<u8>( a * 255 );
        val32 += static_cast<u32>( val8 );

        return val32;
    }

#if WP_ENDIAN == WP_ENDIAN_BIG
    RGBA ColourF::getAsRGBA() const
#else
    auto ColourF::getAsABGR() const -> ABGR
#endif
    {
        u8 val8;
        u32 val32 = 0;

        // Convert to 32bit pattern
        // (ABRG = 8888)

        // Alpha
        val8 = static_cast<u8>( a * 255 );
        val32 = static_cast<u32>( val8 << 24 );

        // Blue
        val8 = static_cast<u8>( b * 255 );
        val32 += static_cast<u32>( val8 << 16 );

        // Green
        val8 = static_cast<u8>( g * 255 );
        val32 += static_cast<u32>( val8 << 8 );

        // Red
        val8 = static_cast<u8>( r * 255 );
        val32 += static_cast<u32>( val8 );

        return val32;
    }

    auto ColourF::getAsBYTE() const -> RGBA
    {
#if WP_ENDIAN == WP_ENDIAN_BIG
        return getAsRGBA();
#else
        return getAsABGR();
#endif
    }

#if WP_ENDIAN == WP_ENDIAN_BIG
    void ColourF::setAsABGR( ABGR val32 )
#else
    void ColourF::setAsRGBA( RGBA val32 )
#endif
    {
        // Convert from 32bit pattern
        // (RGBA = 8888)

        // Red
        r = static_cast<float>( ( val32 >> 24 ) & 0xFF ) / 255.0f;

        // Green
        g = static_cast<float>( ( val32 >> 16 ) & 0xFF ) / 255.0f;

        // Blue
        b = static_cast<float>( ( val32 >> 8 ) & 0xFF ) / 255.0f;

        // Alpha
        a = static_cast<float>( val32 & 0xFF ) / 255.0f;
    }

#if WP_ENDIAN == WP_ENDIAN_BIG
    void ColourF::setAsBGRA( BGRA val32 )
#else
    void ColourF::setAsARGB( ARGB val32 )
#endif
    {
        // Convert from 32bit pattern
        // (ARGB = 8888)

        // Alpha
        a = static_cast<float>( ( val32 >> 24 ) & 0xFF ) / 255.0f;

        // Red
        r = static_cast<float>( ( val32 >> 16 ) & 0xFF ) / 255.0f;

        // Green
        g = static_cast<float>( ( val32 >> 8 ) & 0xFF ) / 255.0f;

        // Blue
        b = static_cast<float>( val32 & 0xFF ) / 255.0f;
    }

#if WP_ENDIAN == WP_ENDIAN_BIG
    void ColourF::setAsARGB( ARGB val32 )
#else
    void ColourF::setAsBGRA( BGRA val32 )
#endif
    {
        // Convert from 32bit pattern
        // (ARGB = 8888)

        // Blue
        b = static_cast<float>( ( val32 >> 24 ) & 0xFF ) / 255.0f;

        // Green
        g = static_cast<float>( ( val32 >> 16 ) & 0xFF ) / 255.0f;

        // Red
        r = static_cast<float>( ( val32 >> 8 ) & 0xFF ) / 255.0f;

        // Alpha
        a = static_cast<float>( val32 & 0xFF ) / 255.0f;
    }

#if WP_ENDIAN == WP_ENDIAN_BIG
    void ColourF::setAsRGBA( RGBA val32 )
#else
    void ColourF::setAsABGR( ABGR val32 )
#endif
    {
        // Convert from 32bit pattern
        // (ABGR = 8888)

        // Alpha
        a = static_cast<float>( ( val32 >> 24 ) & 0xFF ) / 255.0f;

        // Blue
        b = static_cast<float>( ( val32 >> 16 ) & 0xFF ) / 255.0f;

        // Green
        g = static_cast<float>( ( val32 >> 8 ) & 0xFF ) / 255.0f;

        // Red
        r = static_cast<float>( val32 & 0xFF ) / 255.0f;
    }

    auto ColourF::operator*( const f32 fScalar ) const -> ColourF
    {
        ColourF kProd;

        kProd.r = fScalar * r;
        kProd.g = fScalar * g;
        kProd.b = fScalar * b;
        kProd.a = fScalar * a;

        return kProd;
    }

    auto ColourF::operator*=( const f32 fScalar ) -> ColourF &
    {
        r *= fScalar;
        g *= fScalar;
        b *= fScalar;
        a *= fScalar;
        return *this;
    }

    auto ColourF::operator==( const ColourF &other ) const -> bool
    {
        return MathF::equals( r, other.r ) && MathF::equals( g, other.g ) &&
               MathF::equals( b, other.b ) && MathF::equals( a, other.a );
    }

    auto ColourF::operator!=( const ColourF &other ) const -> bool
    {
        return !MathF::equals( r, other.r ) || !MathF::equals( g, other.g ) ||
               !MathF::equals( b, other.b ) || !MathF::equals( a, other.a );
    }

    auto ColourF::isValid() const -> bool
    {
        // A color is valid if all its components are in the range [0,1]
        return r >= 0.0f && r <= 1.0f && g >= 0.0f && g <= 1.0f && b >= 0.0f && b <= 1.0f && a >= 0.0f &&
               a <= 1.0f;
    }

}  // namespace workphone
