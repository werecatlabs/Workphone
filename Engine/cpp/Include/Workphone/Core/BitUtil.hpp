#ifndef __BitUtil_H__
#define __BitUtil_H__

#include <Workphone/WorkphoneTypes.hpp>

namespace workphone
{

    /**
     * Class for manipulating bit patterns.
     */
    class WPCore_API BitUtil
    {
    public:
        /**
         * Determines whether the number is power-of-two or not.
         * @note 0 and 1 are tread as power of two.
         * @tparam T The type of the number.
         * @param n The number to test.
         * @return True if n is a power of two, false otherwise.
         */
        template <typename T>
        static bool isPO2( T n );

        /**
         * Updates flags stored in the type and returns a value holding the updated flags.
         * @tparam T The type of the flags.
         * @param flags The flags to update.
         * @param flag The flag to update.
         * @param value The new value of the flag.
         * @return The value holding the updated flags.
         */
        template <class T>
        static T setFlagValue( T flags, T flag, bool value );

        /**
         * Gets the value of a flag.
         * @tparam T The type of the flags.
         * @param flags The flags to read.
         * @param flag The flag to read.
         * @return The value of the flag.
         */
        template <class T>
        static bool getFlagValue( T flags, T flag );

        template <class T>
        static bool hasAllFlags( T flags, T flag );

        /**
         * Converts fixed pos32 value to float.
         * @param value The fixed pos32 value.
         * @param bits The number of bits used to represent the fractional part.
         * @return The floating pos32 value.
         */
        static f32 fixedToFloat( u32 value, u32 bits );

        /** Convert floating pos32 color channel value between 0.0 and 1.0 (otherwise clamped)
         * to an integer of a certain number of bits. Works for any value of bits between 0 and 31.
         *
         * \param value The value to convert, between 0.0 and 1.0.
         * \param bits The number of bits to use in the output integer.
         * \return The converted integer.
         */
        static u32 floatToFixed( f32 value, u32 bits );

        /** Convert a float32 to a float16 (NV_half_float).
         * Courtesy of OpenEXR.
         *
         * \param i The float value to convert.
         * \return The converted float16 value.
         */
        static u16 floatToHalf( float i );

        /** Converts a float in u32 format to a half in u16 format.
         *
         * \param i The float value in u32 format.
         * \return The converted float value in u16 format.
         */
        static u16 floatToHalfI( u32 i );

        /**
         * Convert a float16 (NV_half_float) to a float32.
         * Courtesy of OpenEXR.
         *
         * \param y The float16 value to convert.
         * \return The converted float32 value.
         */
        static f32 halfToFloat( u16 y );

        /** Converts a half in u16 format to a float in u32 format.
         *
         * \param y The half value in u16 format.
         * \return The converted float value in u32 format.
         */
        static u32 halfToFloatI( u16 y );
    };

    template <typename T>
    bool BitUtil::isPO2( T n )
    {
        return ( n & ( n - 1 ) ) == 0;
    }

    template <class T>
    WPForceInline T BitUtil::setFlagValue( T flags, T flag, bool value )
    {
        return value ? ( flags | flag ) : ( flags & ~flag );
    }

    template <class T>
    WPForceInline bool BitUtil::getFlagValue( T flags, T flag )
    {
        return ( flags & flag ) != 0;
    }

    template <class T>
    WPForceInline bool BitUtil::hasAllFlags( T flags, T flag )
    {
        return ( flags & flag ) == flag;
    }

    inline f32 BitUtil::fixedToFloat( u32 value, u32 bits )
    {
        return static_cast<f32>( value ) / static_cast<f32>( ( 1 << bits ) - 1 );
    }

    inline u32 BitUtil::floatToFixed( f32 value, u32 bits )
    {
        return static_cast<u32>( value * ( 1 << bits ) );
    }

    inline u16 BitUtil::floatToHalf( float i )
    {
        union
        {
            float f;
            u32 i;
        } v;
        v.f = i;
        return floatToHalfI( v.i );
    }

    inline u16 BitUtil::floatToHalfI( u32 i )
    {
        s32 s = ( i >> 16 ) & 0x00008000;
        s32 e = ( ( i >> 23 ) & 0x000000ff ) - ( 127 - 15 );
        s32 m = i & 0x007fffff;

        if( e <= 0 )
        {
            if( e < -10 )
            {
                return 0;
            }
            m = ( m | 0x00800000 ) >> ( 1 - e );

            return static_cast<u16>( s | ( m >> 13 ) );
        }
        if( e == 0xff - ( 127 - 15 ) )
        {
            if( m == 0 )  // Inf
            {
                return static_cast<u16>( s | 0x7c00 );
            }
            // NAN
            m >>= 13;
            return static_cast<u16>( s | 0x7c00 | m | ( m == 0 ) );
        }
        if( e > 30 )  // Overflow
        {
            return static_cast<u16>( s | 0x7c00 );
        }

        return static_cast<u16>( s | ( e << 10 ) | ( m >> 13 ) );
    }

    inline f32 BitUtil::halfToFloat( u16 y )
    {
        union
        {
            f32 f;
            u32 i;
        } v;
        v.i = halfToFloatI( y );
        return v.f;
    }

    inline u32 BitUtil::halfToFloatI( u16 y )
    {
        s32 s = ( y >> 15 ) & 0x00000001;
        s32 e = ( y >> 10 ) & 0x0000001f;
        s32 m = y & 0x000003ff;

        if( e == 0 )
        {
            if( m == 0 )  // Plus or minus zero
            {
                return s << 31;
            }
            // Denormalized number -- renormalize it
            while( !( m & 0x00000400 ) )
            {
                m <<= 1;
                e -= 1;
            }

            e += 1;
            m &= ~0x00000400;
        }
        else if( e == 31 )
        {
            if( m == 0 )  // Inf
            {
                return ( s << 31 ) | 0x7f800000;
            }
            // NaN
            return ( s << 31 ) | 0x7f800000 | ( m << 13 );
        }

        e = e + ( 127 - 15 );
        m = m << 13;

        return ( s << 31 ) | ( e << 23 ) | m;
    }

}  // namespace workphone

#endif
