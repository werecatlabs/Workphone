#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/ByteSwap.hpp>

#ifdef WP_PLATFORM_IOS
#    define bswap_16( X ) X
#    define bswap_32( X ) X
#else
#    define bswap_16( X ) X
#    define bswap_32( X ) X
#endif

namespace workphone
{

    auto Byteswap::byteswap( u16 num ) -> u16
    {
        return bswap_16( num );
    }

    auto Byteswap::byteswap( s16 num ) -> s16
    {
        return bswap_16( num );
    }

    auto Byteswap::byteswap( u32 num ) -> u32
    {
        return bswap_32( num );
    }

    auto Byteswap::byteswap( s32 num ) -> s32
    {
        return bswap_32( num );
    }

    auto Byteswap::byteswap( f32 num ) -> f32
    {
        float retVal;
        auto floatToConvert = reinterpret_cast<char *>( &num );
        auto returnFloat = reinterpret_cast<char *>( &retVal );

        // swap the bytes into a temporary buffer
        returnFloat[0] = floatToConvert[3];
        returnFloat[1] = floatToConvert[2];
        returnFloat[2] = floatToConvert[1];
        returnFloat[3] = floatToConvert[0];

        return retVal;
    }

    auto Byteswap::byteswap( u8 num ) -> u8
    {
        return num;
    }

    auto Byteswap::byteswap( c8 num ) -> c8
    {
        return num;
    }
}  // namespace workphone
