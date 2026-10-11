#ifndef WP_DDS_TEXTURE_HPP
#define WP_DDS_TEXTURE_HPP
#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace workphone::dds
{
    // First-party deterministic encoder v1. No platform SDK or image decoder is
    // needed to inspect a cooked artifact. Only single 2D DX10 textures are accepted.
    constexpr std::uint64_t byteLimit = 256ull * 1024 * 1024;
    struct Mip
    {
        std::uint32_t width, height, rowPitch, size;
        std::size_t offset;
    };
    struct Image
    {
        std::uint32_t format = 0, alphaMode = 0;
        std::vector<Mip> mips;
    };
    inline void require( bool ok, const char *message )
    {
        if( !ok )
            throw std::runtime_error( message );
    }
    inline std::uint32_t word( const std::vector<std::uint8_t> &b, std::size_t p )
    {
        require( p <= b.size() && b.size() - p >= 4, "Truncated DDS header" );
        return std::uint32_t( b[p] ) | std::uint32_t( b[p + 1] ) << 8 | std::uint32_t( b[p + 2] ) << 16 |
               std::uint32_t( b[p + 3] ) << 24;
    }
    inline void put( std::vector<std::uint8_t> &b, std::size_t p, std::uint32_t v )
    {
        for( unsigned i = 0; i < 4; ++i )
            b[p + i] = std::uint8_t( v >> ( 8 * i ) );
    }
    inline unsigned blockBytes( std::uint32_t f )
    {
        return f == 71 || f == 72 ? 8 : f == 77 || f == 78 ? 16 : 0;
    }
    inline bool supported( std::uint32_t f )
    {
        return blockBytes( f ) || f == 87 || f == 91;
    }
    inline Image inspect( const std::vector<std::uint8_t> &b )
    {
        require( b.size() >= 148 && b.size() <= byteLimit + 148, "Invalid DDS byte count" );
        require( word( b, 0 ) == 0x20534444 && word( b, 4 ) == 124 && word( b, 76 ) == 32 &&
                     word( b, 80 ) == 4 && word( b, 84 ) == 0x30315844,
                 "DDS requires a DX10 header" );
        const auto flags = word( b, 8 ), caps = word( b, 108 );
        require( ( flags & 0x1007 ) == 0x1007 && !( flags & ~( 0x1007u | 8u | 0x80000u | 0x20000u ) ) &&
                     ( caps & 0x1000 ) && !( caps & ~( 0x1000u | 8u | 0x400000u ) ) &&
                     word( b, 24 ) == 0 && word( b, 112 ) == 0 && word( b, 116 ) == 0 &&
                     word( b, 120 ) == 0,
                 "Unsupported DDS flags or topology" );
        require(
            word( b, 132 ) == 3 && word( b, 136 ) == 0 && word( b, 140 ) == 1 && word( b, 144 ) <= 3,
            "DDS arrays, cubes, volumes or alpha mode are unsupported" );
        Image result;
        result.format = word( b, 128 );
        result.alphaMode = word( b, 144 );
        require( supported( result.format ), "Unsupported DDS GPU format" );
        auto w = word( b, 16 ), h = word( b, 12 ), count = word( b, 28 );
        require( w && h && w <= 16384 && h <= 16384 && count && count <= 15,
                 "Invalid DDS dimensions or mip count" );
        require( count == 1 || ( ( flags & 0x20000 ) && ( caps & 0x400008 ) == 0x400008 ),
                 "DDS mip chain flags are missing" );
        const auto block = blockBytes( result.format );
        require( block ? ( flags & 0x80000 ) && !( flags & 8 ) : ( flags & 8 ) && !( flags & 0x80000 ),
                 "DDS pitch flags do not match the GPU format" );
        std::uint64_t offset = 148;
        for( unsigned i = 0; i < count; ++i )
        {
            const std::uint64_t row = block ? ( ( w + 3 ) / 4 ) * block : w * 4;
            const auto size = row * ( block ? ( h + 3 ) / 4 : h );
            require( size <= byteLimit && offset + size <= b.size(), "Truncated or oversized DDS mip" );
            if( i == 0 )
                require( word( b, 20 ) == ( block ? size : row ), "DDS declared pitch is inconsistent" );
            result.mips.push_back(
                { w, h, std::uint32_t( row ), std::uint32_t( size ), std::size_t( offset ) } );
            offset += size;
            require( w != 1 || h != 1 || i + 1 == count, "DDS has extra mip levels" );
            w = std::max( w / 2, 1u );
            h = std::max( h / 2, 1u );
        }
        require( offset == b.size(), "DDS has trailing bytes" );
        return result;
    }

    // RGB bounding endpoints, nearest palette selection and edge replication.
    // Integer-only arithmetic and fixed tie breaks make encoder v1 reproducible.
    inline void encodeBlock( const std::array<std::array<unsigned, 4>, 16> &p, bool alpha,
                             std::vector<std::uint8_t> &out )
    {
        if( alpha )
        {
            unsigned lo = 255, hi = 0;
            for( auto &v : p )
            {
                lo = std::min( lo, v[3] );
                hi = std::max( hi, v[3] );
            }
            std::array<unsigned, 8> a{ hi, lo };
            if( hi > lo )
                for( unsigned i = 2; i < 8; ++i )
                    a[i] = ( ( 8 - i ) * hi + ( i - 1 ) * lo ) / 7;
            else
            {
                for( unsigned i = 2; i < 6; ++i )
                    a[i] = ( ( 6 - i ) * hi + ( i - 1 ) * lo ) / 5;
                a[6] = 0;
                a[7] = 255;
            }
            std::uint64_t bits = 0;
            for( unsigned i = 0; i < 16; ++i )
            {
                unsigned best = 0, err = ~0u;
                for( unsigned j = 0; j < 8; ++j )
                {
                    int d = int( p[i][3] ) - int( a[j] );
                    unsigned e = d * d;
                    if( e < err )
                    {
                        best = j;
                        err = e;
                    }
                }
                bits |= std::uint64_t( best ) << ( 3 * i );
            }
            out.push_back( std::uint8_t( hi ) );
            out.push_back( std::uint8_t( lo ) );
            for( unsigned i = 0; i < 6; ++i )
                out.push_back( std::uint8_t( bits >> ( 8 * i ) ) );
        }
        std::array<unsigned, 3> lo{ 255, 255, 255 }, hi{};
        for( auto &v : p )
            for( unsigned c = 0; c < 3; ++c )
            {
                lo[c] = std::min( lo[c], v[c] );
                hi[c] = std::max( hi[c], v[c] );
            }
        auto pack = []( auto v ) { return ( v[2] >> 3 ) << 11 | ( v[1] >> 2 ) << 5 | ( v[0] >> 3 ); };
        unsigned c0 = pack( hi ), c1 = pack( lo );
        if( c0 == c1 )
        {
            if( c0 < 65535 )
                ++c0;
            else
                --c1;
        }
        if( c0 < c1 )
            std::swap( c0, c1 );
        auto unpack = []( unsigned c ) {
            return std::array<unsigned, 3>{ ( c & 31 ) * 255 / 31, ( ( c >> 5 ) & 63 ) * 255 / 63,
                                            ( ( c >> 11 ) & 31 ) * 255 / 31 };
        };
        std::array<std::array<unsigned, 3>, 4> palette{ unpack( c0 ), unpack( c1 ) };
        for( unsigned c = 0; c < 3; ++c )
        {
            palette[2][c] = ( 2 * palette[0][c] + palette[1][c] ) / 3;
            palette[3][c] = ( palette[0][c] + 2 * palette[1][c] ) / 3;
        }
        std::uint32_t bits = 0;
        for( unsigned i = 0; i < 16; ++i )
        {
            unsigned best = 0, err = ~0u;
            for( unsigned j = 0; j < 4; ++j )
            {
                unsigned e = 0;
                for( unsigned c = 0; c < 3; ++c )
                {
                    int d = int( p[i][c] ) - int( palette[j][c] );
                    e += d * d;
                }
                if( e < err )
                {
                    best = j;
                    err = e;
                }
            }
            bits |= best << ( 2 * i );
        }
        out.push_back( std::uint8_t( c0 ) );
        out.push_back( std::uint8_t( c0 >> 8 ) );
        out.push_back( std::uint8_t( c1 ) );
        out.push_back( std::uint8_t( c1 >> 8 ) );
        for( unsigned i = 0; i < 4; ++i )
            out.push_back( std::uint8_t( bits >> ( 8 * i ) ) );
    }

    template <class Levels>
    std::vector<std::uint8_t> encode( const Levels &levels, std::uint32_t format )
    {
        require( supported( format ) && !levels.empty() && levels.size() <= 15,
                 "Invalid DDS encoder input" );
        std::vector<std::uint8_t> bytes( 148, 0 );
        const auto block = blockBytes( format );
        auto w = levels.front().width, h = levels.front().height;
        require( w && h && w <= 16384 && h <= 16384, "Invalid DDS encoder dimensions" );
        put( bytes, 0, 0x20534444 );
        put( bytes, 4, 124 );
        put( bytes, 8, 0x1007 | ( block ? 0x80000 : 8 ) | ( levels.size() > 1 ? 0x20000 : 0 ) );
        put( bytes, 12, h );
        put( bytes, 16, w );
        put( bytes, 20, block ? ( ( w + 3 ) / 4 ) * block * ( ( h + 3 ) / 4 ) : w * 4 );
        put( bytes, 28, std::uint32_t( levels.size() ) );
        put( bytes, 76, 32 );
        put( bytes, 80, 4 );
        put( bytes, 84, 0x30315844 );
        put( bytes, 108, 0x1000 | ( levels.size() > 1 ? 0x400008 : 0 ) );
        put( bytes, 128, format );
        put( bytes, 132, 3 );
        put( bytes, 140, 1 );
        put( bytes, 144, block == 8 ? 3 : 1 );  // opaque BC1; straight alpha otherwise
        for( const auto &level : levels )
        {
            require(
                level.width == w && level.height == h && level.bgra.size() == std::uint64_t( w ) * h * 4,
                "Invalid DDS encoder mip layout" );
            const std::uint64_t size = block ? std::uint64_t( ( w + 3 ) / 4 ) * ( ( h + 3 ) / 4 ) * block
                                             : std::uint64_t( w ) * h * 4;
            require( size <= byteLimit && bytes.size() + size <= byteLimit + 148,
                     "DDS encoder byte limit exceeded" );
            if( !block )
                bytes.insert( bytes.end(), level.bgra.begin(), level.bgra.end() );
            else
                for( unsigned y = 0; y < h; y += 4 )
                    for( unsigned x = 0; x < w; x += 4 )
                    {
                        std::array<std::array<unsigned, 4>, 16> pixels{};
                        for( unsigned j = 0; j < 4; ++j )
                            for( unsigned i = 0; i < 4; ++i )
                            {
                                const auto pos = ( std::size_t( std::min( y + j, h - 1 ) ) * w +
                                                   std::min( x + i, w - 1 ) ) *
                                                 4;
                                for( unsigned c = 0; c < 4; ++c )
                                    pixels[j * 4 + i][c] = level.bgra[pos + c];
                                require( block != 8 || pixels[j * 4 + i][3] == 255,
                                         "BC1 profile requires opaque pixels; choose BC3" );
                            }
                        encodeBlock( pixels, block == 16, bytes );
                    }
            w = std::max( w / 2, 1u );
            h = std::max( h / 2, 1u );
        }
        inspect( bytes );
        return bytes;
    }
}  // namespace workphone::dds
#endif
