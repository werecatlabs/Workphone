#ifndef Workphone_ImageGenerationValidation_hpp
#define Workphone_ImageGenerationValidation_hpp

#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <algorithm>
#include <cstring>
#include <vector>
#include <zlib.h>

namespace workphone::image_generation
{
    constexpr size_t maximumResponseSize = 32 * 1024 * 1024;
    constexpr size_t maximumImageSize = 24 * 1024 * 1024;
    constexpr size_t maximumPromptSize = 32000;
    constexpr int timeoutMilliseconds = 300000;

    inline int base64Digit( unsigned char value )
    {
        if( value >= 'A' && value <= 'Z' ) return value - 'A';
        if( value >= 'a' && value <= 'z' ) return value - 'a' + 26;
        if( value >= '0' && value <= '9' ) return value - '0' + 52;
        if( value == '+' ) return 62;
        if( value == '/' ) return 63;
        return -1;
    }

    inline bool decodeBase64( const char *data, size_t size, Array<u8> &bytes )
    {
        if( size == 0 || size % 4 != 0 || size / 4 * 3 > maximumImageSize )
            return false;

        bytes.reserve( size / 4 * 3 );
        for( size_t offset = 0; offset < size; offset += 4 )
        {
            const auto a = base64Digit( data[offset] );
            const auto b = base64Digit( data[offset + 1] );
            const auto c = base64Digit( data[offset + 2] );
            const auto d = base64Digit( data[offset + 3] );
            const bool padC = data[offset + 2] == '=';
            const bool padD = data[offset + 3] == '=';
            if( a < 0 || b < 0 || ( c < 0 && !padC ) || ( d < 0 && !padD ) ||
                ( padC && !padD ) || ( ( padC || padD ) && offset + 4 != size ) ||
                ( padC && ( b & 15 ) != 0 ) || ( padD && !padC && ( c & 3 ) != 0 ) )
                return false;

            bytes.push_back( static_cast<u8>( ( a << 2 ) | ( b >> 4 ) ) );
            if( !padC ) bytes.push_back( static_cast<u8>( ( b << 4 ) | ( c >> 2 ) ) );
            if( !padD ) bytes.push_back( static_cast<u8>( ( c << 6 ) | d ) );
        }
        return true;
    }

    inline u32 readBigEndian( const u8 *data )
    {
        return ( u32( data[0] ) << 24 ) | ( u32( data[1] ) << 16 ) |
               ( u32( data[2] ) << 8 ) | u32( data[3] );
    }

    // Validate the complete PNG envelope and its bounded decompressed scanlines before import.
    // No pixel buffers, graphics objects, file paths or provider-supplied URLs are involved.
    inline bool validatePng( const Array<u8> &bytes, const String &requestedSize )
    {
        constexpr u8 signature[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
        if( bytes.size() < 57 || bytes.size() > maximumImageSize ||
            std::memcmp( bytes.data(), signature, sizeof( signature ) ) != 0 )
            return false;

        u32 width = 0, height = 0;
        u8 depth = 0, colour = 0, interlace = 0;
        bool header = false, palette = false, imageData = false, dataEnded = false, end = false;
        std::vector<u8> compressed;
        for( size_t offset = sizeof( signature ); offset < bytes.size(); )
        {
            if( bytes.size() - offset < 12 ) return false;
            const auto length = readBigEndian( bytes.data() + offset );
            if( length > bytes.size() - offset - 12 ) return false;
            const auto type = bytes.data() + offset + 4;
            const auto data = bytes.data() + offset + 8;
            if( crc32( 0, type, length + 4 ) != readBigEndian( data + length ) ) return false;
            for( size_t i = 0; i < 4; ++i )
            {
                if( !( ( type[i] >= 'A' && type[i] <= 'Z' ) ||
                       ( type[i] >= 'a' && type[i] <= 'z' ) ) ) return false;
            }
            if( type[2] & 32 ) return false;

            if( std::memcmp( type, "IHDR", 4 ) == 0 )
            {
                if( header || offset != sizeof( signature ) || length != 13 ) return false;
                width = readBigEndian( data );
                height = readBigEndian( data + 4 );
                const auto expectedWidth = requestedSize == "1536x1024" ? 1536u : 1024u;
                const auto expectedHeight = requestedSize == "1024x1536" ? 1536u : 1024u;
                if( width != expectedWidth || height != expectedHeight ) return false;
                depth = data[8]; colour = data[9]; interlace = data[12];
                const bool validDepth =
                    ( colour == 0 && ( depth == 1 || depth == 2 || depth == 4 || depth == 8 || depth == 16 ) ) ||
                    ( colour == 3 && ( depth == 1 || depth == 2 || depth == 4 || depth == 8 ) ) ||
                    ( ( colour == 2 || colour == 4 || colour == 6 ) && ( depth == 8 || depth == 16 ) );
                if( !validDepth || data[10] != 0 || data[11] != 0 || interlace > 1 ) return false;
                header = true;
            }
            else if( !header ) return false;
            else if( std::memcmp( type, "PLTE", 4 ) == 0 )
            {
                if( palette || imageData || length == 0 || length % 3 != 0 || length > 768 ||
                    colour == 0 || colour == 4 || ( colour == 3 && length / 3 > ( 1u << depth ) ) )
                    return false;
                palette = true;
            }
            else if( std::memcmp( type, "IDAT", 4 ) == 0 )
            {
                if( dataEnded || ( colour == 3 && !palette ) ) return false;
                compressed.insert( compressed.end(), data, data + length );
                imageData = true;
            }
            else if( std::memcmp( type, "IEND", 4 ) == 0 )
            {
                if( length != 0 || !imageData || offset + 12 != bytes.size() ) return false;
                end = true;
            }
            else
            {
                // Unknown critical chunks cannot be safely interpreted by an image decoder.
                if( !( type[0] & 32 ) ) return false;
                if( imageData ) dataEnded = true;
            }
            offset += size_t( length ) + 12;
        }
        if( !end || compressed.empty() ) return false;

        const size_t channels = colour == 2 ? 3 : colour == 4 ? 2 : colour == 6 ? 4 : 1;
        const size_t bitsPerPixel = channels * depth;
        struct Pass { size_t rows; size_t rowBytes; };
        std::vector<Pass> passes;
        size_t rawSize = 0;
        const auto addPass = [&]( size_t passWidth, size_t passHeight )
        {
            if( passWidth == 0 || passHeight == 0 ) return;
            const auto rowBytes = ( passWidth * bitsPerPixel + 7 ) / 8;
            passes.push_back( { passHeight, rowBytes } );
            rawSize += passHeight * ( rowBytes + 1 );
        };
        if( interlace == 0 ) addPass( width, height );
        else
        {
            constexpr size_t x[] = { 0, 4, 0, 2, 0, 1, 0 }, y[] = { 0, 0, 4, 0, 2, 0, 1 };
            constexpr size_t dx[] = { 8, 8, 4, 4, 2, 2, 1 }, dy[] = { 8, 8, 8, 4, 4, 2, 2 };
            for( size_t i = 0; i < 7; ++i )
                addPass( ( width - x[i] + dx[i] - 1 ) / dx[i],
                         ( height - y[i] + dy[i] - 1 ) / dy[i] );
        }
        std::vector<u8> raw( rawSize );
        uLongf outputSize = static_cast<uLongf>( raw.size() );
        uLong inputSize = static_cast<uLong>( compressed.size() );
        if( uncompress2( raw.data(), &outputSize, compressed.data(), &inputSize ) != Z_OK ||
            outputSize != rawSize || inputSize != compressed.size() ) return false;
        size_t offset = 0;
        for( const auto &pass : passes )
        {
            for( size_t row = 0; row < pass.rows; ++row )
            {
                if( raw[offset] > 4 ) return false;
                offset += pass.rowBytes + 1;
            }
        }
        return true;
    }
}

#endif
