#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace workphone::procedural::texture_export
{
    // Byte export is deliberately UNORM: no implicit gamma conversion, channel
    // swizzle, or alpha premultiplication (normal/roughness maps stay data).
    inline unsigned char unorm( double value )
    {
        if( !std::isfinite( value ) )
        {
            return 0;
        }
        return static_cast<unsigned char>( std::lround( std::clamp( value, 0.0, 1.0 ) * 255.0 ) );
    }

    inline void be32( std::vector<unsigned char> &out, std::uint32_t value )
    {
        for( int shift = 24; shift >= 0; shift -= 8 )
        {
            out.push_back( static_cast<unsigned char>( value >> shift ) );
        }
    }

    inline void chunk( std::vector<unsigned char> &out, const char *type,
                       const std::vector<unsigned char> &payload )
    {
        be32( out, static_cast<std::uint32_t>( payload.size() ) );
        const auto start = out.size();
        out.insert( out.end(), type, type + 4 );
        out.insert( out.end(), payload.begin(), payload.end() );
        std::uint32_t crc = 0xffffffffu;
        for( auto i = start; i < out.size(); ++i )
        {
            crc ^= out[i];
            for( int bit = 0; bit < 8; ++bit )
            {
                crc = ( crc >> 1 ) ^ ( ( crc & 1 ) ? 0xedb88320u : 0u );
            }
        }
        be32( out, crc ^ 0xffffffffu );
    }

    template <class Pixels>
    bool encode( std::uint32_t width, std::uint32_t height, const Pixels &pixels, std::string format,
                 std::vector<unsigned char> &out )
    {
        out.clear();
        std::transform( format.begin(), format.end(), format.begin(), []( unsigned char c ) {
            return c >= 'A' && c <= 'Z' ? static_cast<char>( c + 'a' - 'A' ) : static_cast<char>( c );
        } );
        if( !format.empty() && format.front() == '.' )
        {
            format.erase( 0, 1 );
        }
        if( format != "png" && format != "tga" && format != "ppm" && format != "pam" )
        {
            return false;
        }
        const auto count = static_cast<std::uint64_t>( width ) * height;
        // Bound output and all arithmetic to the stream's 32-bit byte domain.
        if( !width || !height || width > 16384 || height > 16384 || count > 16ull * 1024 * 1024 ||
            count != pixels.size() ||
            count > ( std::numeric_limits<std::uint32_t>::max() - 1024ull ) / 6 )
        {
            return false;
        }
        if( format == "tga" && ( width > 65535 || height > 65535 ) )
        {
            return false;
        }
        if( format == "ppm" )
        {
            for( const auto &p : pixels )
            {
                if( !std::isfinite( p.a ) || p.a < 1.0 )
                {
                    return false;
                }
            }
        }

        std::vector<unsigned char> rgba;
        rgba.reserve( static_cast<size_t>( count ) * 4 );
        for( const auto &p : pixels )
        {
            rgba.push_back( unorm( p.r ) );
            rgba.push_back( unorm( p.g ) );
            rgba.push_back( unorm( p.b ) );
            rgba.push_back( unorm( p.a ) );
        }
        if( format == "png" )
        {
            out = { 137, 80, 78, 71, 13, 10, 26, 10 };
            std::vector<unsigned char> ihdr;
            be32( ihdr, width );
            be32( ihdr, height );
            ihdr.insert( ihdr.end(), { 8, 6, 0, 0, 0 } );
            chunk( out, "IHDR", ihdr );
            std::vector<unsigned char> scanlines;
            scanlines.reserve( rgba.size() + height );
            const auto stride = static_cast<size_t>( width ) * 4;
            for( std::uint32_t y = 0; y < height; ++y )
            {
                scanlines.push_back( 0 );  // PNG filter None; rows remain top-down.
                scanlines.insert( scanlines.end(), rgba.begin() + y * stride,
                                  rgba.begin() + ( y + 1 ) * stride );
            }
            // Standards-compliant zlib stream using stored DEFLATE blocks.
            std::vector<unsigned char> zlib = { 0x78, 0x01 };
            for( size_t offset = 0; offset < scanlines.size(); )
            {
                const auto length = std::min<size_t>( 65535, scanlines.size() - offset );
                zlib.push_back( offset + length == scanlines.size() ? 1 : 0 );
                zlib.push_back( static_cast<unsigned char>( length ) );
                zlib.push_back( static_cast<unsigned char>( length >> 8 ) );
                zlib.push_back( static_cast<unsigned char>( ~length ) );
                zlib.push_back( static_cast<unsigned char>( ( ~length ) >> 8 ) );
                zlib.insert( zlib.end(), scanlines.begin() + offset,
                             scanlines.begin() + offset + length );
                offset += length;
            }
            std::uint32_t a = 1, b = 0;
            for( auto byte : scanlines )
            {
                a = ( a + byte ) % 65521;
                b = ( b + a ) % 65521;
            }
            be32( zlib, ( b << 16 ) | a );
            chunk( out, "IDAT", zlib );
            chunk( out, "IEND", {} );
        }
        else if( format == "tga" )
        {
            out.resize( 18, 0 );
            out[2] = 2;
            out[12] = static_cast<unsigned char>( width );
            out[13] = static_cast<unsigned char>( width >> 8 );
            out[14] = static_cast<unsigned char>( height );
            out[15] = static_cast<unsigned char>( height >> 8 );
            out[16] = 32;
            out[17] = 0x28;  // Top-left origin, eight straight-alpha bits.
            for( size_t i = 0; i < rgba.size(); i += 4 )
            {
                out.insert( out.end(), { rgba[i + 2], rgba[i + 1], rgba[i], rgba[i + 3] } );
            }
        }
        else
        {
            const auto header =
                format == "pam"
                    ? "P7\nWIDTH " + std::to_string( width ) + "\nHEIGHT " + std::to_string( height ) +
                          "\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n"
                    : "P6\n" + std::to_string( width ) + " " + std::to_string( height ) + "\n255\n";
            out.insert( out.end(), header.begin(), header.end() );
            if( format == "pam" )
            {
                out.insert( out.end(), rgba.begin(), rgba.end() );
            }
            else
            {
                for( size_t i = 0; i < rgba.size(); i += 4 )
                {
                    out.insert( out.end(), rgba.begin() + i, rgba.begin() + i + 3 );
                }
            }
        }
        return true;
    }

    // Accept the engine's CPU TextureBuffer (RGBA8). The template keeps this
    // codec usable by tools without linking the engine or requiring a GPU.
    template <class Buffer>
    bool encodeTextureBuffer( const Buffer &buffer, const std::string &format,
                              std::vector<unsigned char> &out )
    {
        out.clear();
        const auto count = static_cast<std::uint64_t>( buffer.width ) * buffer.height;
        if( count > ( std::numeric_limits<std::uint32_t>::max() - 1024ull ) / 6 ||
            count * 4 != buffer.pixels.size() )
        {
            return false;
        }

        struct Pixel
        {
            double r, g, b, a;
        };

        // A lazy view avoids expanding a full RGBA8 bake into 32-byte float
        // pixels merely to encode it. Large material exports stay bounded.
        struct View
        {
            const unsigned char *data;
            size_t count;

            struct Iterator
            {
                const unsigned char *data;

                Pixel operator*() const
                {
                    return { data[0] / 255.0, data[1] / 255.0, data[2] / 255.0, data[3] / 255.0 };
                }

                Iterator &operator++()
                {
                    data += 4;
                    return *this;
                }

                bool operator!=( const Iterator &other ) const
                {
                    return data != other.data;
                }
            };

            size_t size() const
            {
                return count;
            }

            Iterator begin() const
            {
                return { data };
            }

            Iterator end() const
            {
                return { count ? data + count * 4 : data };
            }
        };

        return encode( buffer.width, buffer.height,
                       View{ buffer.pixels.data(), static_cast<size_t>( count ) }, format, out );
    }
}  // namespace workphone::procedural::texture_export
