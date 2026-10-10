#ifndef WORKPHONE_NETWORK_CODEC_HPP
#define WORKPHONE_NETWORK_CODEC_HPP

#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace workphone::network
{
    // Canonical little-endian integers and IEEE binary32 floats preserve the
    // existing Windows wire format without depending on host endianness.
    inline constexpr size_t MaxMessageBytes = 1024 * 1024;
    inline constexpr size_t MaxStringBytes = 64 * 1024;

    inline void requireBytes( size_t position, size_t count, size_t size )
    {
        if( position > size || count > size - position )
            throw std::out_of_range( "Network codec: truncated value" );
    }

    template <class Buffer>
    void appendBytes( Buffer &buffer, const void *data, size_t count )
    {
        if( count == 0 )
            return;
        if( !data )
            throw std::invalid_argument( "Network codec: null input" );
        if( buffer.size() > MaxMessageBytes || count > MaxMessageBytes - buffer.size() )
            throw std::length_error( "Network codec: message capacity exceeded" );
        const auto *bytes = static_cast<const unsigned char *>( data );
        const auto inputAddress = reinterpret_cast<std::uintptr_t>( data );
        const auto bufferAddress = reinterpret_cast<std::uintptr_t>( buffer.data() );
        if( !buffer.empty() && inputAddress >= bufferAddress &&
            inputAddress - bufferAddress < buffer.size() )
        {
            requireBytes( inputAddress - bufferAddress, count, buffer.size() );
            Buffer staged;
            staged.insert( staged.end(), bytes, bytes + count );
            buffer.insert( buffer.end(), staged.begin(), staged.end() );
            return;
        }
        buffer.insert( buffer.end(), bytes, bytes + count );
    }

    template <class Buffer>
    void assignBytes( Buffer &buffer, const void *data, size_t count )
    {
        // Stage first: validation/allocation failure or self-assignment must
        // not destroy the previous buffer or invalidate the input pointer.
        Buffer replacement;
        appendBytes( replacement, data, count );
        buffer = std::move( replacement );
    }

    template <class Buffer, class T>
    void write( Buffer &buffer, T value )
    {
        static_assert( std::is_integral_v<T> && !std::is_same_v<T, bool> );
        static_assert( sizeof( T ) <= 4 );
        using U = std::make_unsigned_t<T>;
        auto bits = static_cast<U>( value );
        unsigned char bytes[sizeof( T )];
        for( size_t i = 0; i < sizeof( T ); ++i )
            bytes[i] = static_cast<unsigned char>( bits >> ( i * 8 ) );
        appendBytes( buffer, bytes, sizeof( bytes ) );
    }

    template <class Buffer>
    void write( Buffer &buffer, float value )
    {
        static_assert( sizeof( float ) == 4 && std::numeric_limits<float>::is_iec559 );
        std::uint32_t bits;
        std::memcpy( &bits, &value, sizeof( bits ) );
        write( buffer, bits );
    }

    template <class Buffer, class T>
    void read( const Buffer &buffer, size_t &position, T &value )
    {
        static_assert( std::is_integral_v<T> && !std::is_same_v<T, bool> );
        static_assert( sizeof( T ) <= 4 );
        requireBytes( position, sizeof( T ), buffer.size() );
        using U = std::make_unsigned_t<T>;
        U bits = 0;
        for( size_t i = 0; i < sizeof( T ); ++i )
            bits |= static_cast<U>( static_cast<U>( buffer[position + i] ) << ( i * 8 ) );
        // Signed values use the two's-complement wire representation, with a
        // defined conversion even on implementations where U -> T is not.
        if constexpr( std::is_signed_v<T> )
        {
            const auto max = static_cast<U>( std::numeric_limits<T>::max() );
            value = bits <= max ? static_cast<T>( bits )
                                : static_cast<T>( -1 - static_cast<T>( static_cast<U>( ~bits ) ) );
        }
        else
            value = bits;
        position += sizeof( T );
    }

    template <class Buffer>
    void read( const Buffer &buffer, size_t &position, float &value )
    {
        std::uint32_t bits;
        read( buffer, position, bits );
        std::memcpy( &value, &bits, sizeof( bits ) );
    }

    template <class Buffer>
    void readBool( const Buffer &buffer, size_t &position, bool &value )
    {
        auto cursor = position;
        std::uint8_t raw;
        read( buffer, cursor, raw );
        if( raw > 1 )
            throw std::invalid_argument( "Network codec: invalid Boolean" );
        value = raw != 0;
        position = cursor;
    }

    template <class Buffer, class String>
    void writeString( Buffer &buffer, const String &value )
    {
        if( value.size() > MaxStringBytes || buffer.size() > MaxMessageBytes - 4 ||
            value.size() > MaxMessageBytes - 4 - buffer.size() )
            throw std::length_error( "Network codec: string capacity exceeded" );
        write( buffer, static_cast<std::uint32_t>( value.size() ) );
        appendBytes( buffer, value.data(), value.size() );
    }

    template <class Buffer, class String>
    void readString( const Buffer &buffer, size_t &position, String &value )
    {
        auto cursor = position;
        std::uint32_t length;
        read( buffer, cursor, length );
        if( length > MaxStringBytes )
            throw std::length_error( "Network codec: string capacity exceeded" );
        requireBytes( cursor, length, buffer.size() );
        String decoded;
        if( length != 0 )
            decoded.assign( reinterpret_cast<const char *>( buffer.data() + cursor ), length );
        value = std::move( decoded );
        position = cursor + length;
    }
}  // namespace workphone::network
#endif
