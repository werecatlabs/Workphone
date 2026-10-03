#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/ColourI.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Core/ColourUtil.hpp>

namespace workphone
{

    ColourI::ColourI() = default;

    ColourI::ColourI( u32 a, u32 r, u32 g, u32 b ) :
        m_color( ( ( a & 0xff ) << 24 ) | ( ( r & 0xff ) << 16 ) | ( ( g & 0xff ) << 8 ) | ( b & 0xff ) )
    {
    }

    ColourI::ColourI( u32 clr ) : m_color( clr )
    {
    }

    auto ColourI::getAlpha() const -> u32
    {
        return m_color >> 24;
    }

    auto ColourI::getRed() const -> u32
    {
        return ( m_color >> 16 ) & 0xff;
    }

    auto ColourI::getGreen() const -> u32
    {
        return ( m_color >> 8 ) & 0xff;
    }

    auto ColourI::operator+( const ColourI &other ) const -> ColourI
    {
        s32 a = getAlpha() + other.getAlpha();
        if( a > 255 )
        {
            a = 255;
        }

        s32 r = getRed() + other.getRed();
        if( r > 255 )
        {
            r = 255;
        }

        s32 g = getGreen() + other.getGreen();
        if( g > 255 )
        {
            g = 255;
        }

        s32 b = getBlue() + other.getBlue();
        if( b > 255 )
        {
            b = 255;
        }

        return { (u32)a, (u32)r, (u32)g, (u32)b };
    }

    auto ColourI::getInterpolated( const ColourI &other, f32 d ) const -> ColourI
    {
        const auto inv = 1.0f - d;
        return { static_cast<u32>( other.getAlpha() * inv + getAlpha() * d ),
                 static_cast<u32>( other.getRed() * inv + getRed() * d ),
                 static_cast<u32>( other.getGreen() * inv + getGreen() * d ),
                 static_cast<u32>( other.getBlue() * inv + getBlue() * d ) };
    }

    auto ColourI::operator!=( const ColourI &other ) const -> bool
    {
        return other.m_color != m_color;
    }

    auto ColourI::operator==( const ColourI &other ) const -> bool
    {
        return other.m_color == m_color;
    }

    void ColourI::set( u32 col )
    {
        m_color = col;
    }

    void ColourI::set( u32 a, u32 r, u32 g, u32 b )
    {
        m_color =
            ( ( ( a & 0xff ) << 24 ) | ( ( r & 0xff ) << 16 ) | ( ( g & 0xff ) << 8 ) | ( b & 0xff ) );
    }

    void ColourI::toOpenGLColor( u8 *dest ) const
    {
        *dest = static_cast<u8>( getRed() );
        *++dest = static_cast<u8>( getGreen() );
        *++dest = static_cast<u8>( getBlue() );
        *++dest = static_cast<u8>( getAlpha() );
    }

    auto ColourI::toA1R5G5B5() const -> u16
    {
        return ColourUtil::A8R8G8B8toA1R5G5B5( m_color );
    }

    void ColourI::setBlue( u32 b )
    {
        m_color = ( b & 0xff ) | ( m_color & 0xffffff00 );
    }

    void ColourI::setGreen( u32 g )
    {
        m_color = ( ( g & 0xff ) << 8 ) | ( m_color & 0xffff00ff );
    }

    void ColourI::setRed( u32 r )
    {
        m_color = ( ( r & 0xff ) << 16 ) | ( m_color & 0xff00ffff );
    }

    void ColourI::setAlpha( u32 a )
    {
        m_color = ( ( a & 0xff ) << 24 ) | ( m_color & 0x00ffffff );
    }

    auto ColourI::getAverage() const -> u32
    {
        return ( getRed() + getGreen() + getBlue() ) / 3;
    }

    auto ColourI::getLuminance() const -> f32
    {
        return 0.3f * getRed() + 0.59f * getGreen() + 0.11f * getBlue();
    }

    auto ColourI::getBlue() const -> u32
    {
        return m_color & 0xff;
    }

    auto ColourI::getInterpolated_quadratic( const ColourI &c1, const ColourI &c2, const f32 d ) const
        -> ColourI
    {
        // this*(1-d)*(1-d) + 2 * c1 * (1-d) + c2 * d * d;
        const f32 inv = 1.f - d;
        const f32 mul0 = inv * inv;
        const f32 mul1 = 2.f * d * inv;
        const f32 mul2 = d * d;

        return { static_cast<u32>( MathI::clamp(
                     MathI::Floor( static_cast<s32>( static_cast<f32>( getAlpha() ) * mul0 +
                                                     static_cast<f32>( c1.getAlpha() ) * mul1 +
                                                     static_cast<f32>( c2.getAlpha() ) * mul2 ) ),
                     0, 255 ) ),
                 static_cast<u32>( MathI::clamp(
                     MathI::Floor( static_cast<s32>( static_cast<f32>( getRed() ) * mul0 +
                                                     static_cast<f32>( c1.getRed() ) * mul1 +
                                                     static_cast<f32>( c2.getRed() ) * mul2 ) ),
                     0, 255 ) ),
                 static_cast<u32>( MathI::clamp(
                     MathI::Floor( static_cast<s32>( static_cast<f32>( getGreen() ) * mul0 +
                                                     static_cast<f32>( c1.getGreen() ) * mul1 +
                                                     static_cast<f32>( c2.getGreen() ) * mul2 ) ),
                     0, 255 ) ),
                 static_cast<u32>( MathI::clamp(
                     MathI::Floor( static_cast<s32>( static_cast<f32>( getBlue() ) * mul0 +
                                                     static_cast<f32>( c1.getBlue() ) * mul1 +
                                                     static_cast<f32>( c2.getBlue() ) * mul2 ) ),
                     0, 255 ) ) };
    }

}  // namespace workphone
