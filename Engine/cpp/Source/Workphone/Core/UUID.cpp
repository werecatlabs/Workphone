#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/UUID.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

namespace workphone
{

    uuid::uuid() = default;

    uuid::uuid( const storage_type &bytes ) : m_bytes( bytes )
    {
    }

    uuid::uuid( std::initializer_list<value_type> bytes )
    {
        assert( bytes.size() == m_bytes.size() );
        if( bytes.size() != m_bytes.size() )
        {
            throw std::invalid_argument( "uuid requires exactly 16 bytes" );
        }

        std::copy( bytes.begin(), bytes.end(), m_bytes.begin() );
    }

    uuid uuid::generate()
    {
        std::random_device randomDevice;
        std::mt19937 generator( randomDevice() );
        std::uniform_int_distribution<u32> distribution( 0, 0xFFFFFFFFu );

        uuid result;
        for( size_t i = 0; i < result.m_bytes.size(); i += 4 )
        {
            const auto value = distribution( generator );
            result.m_bytes[i + 0] = static_cast<u8>( ( value >> 24 ) & 0xFF );
            result.m_bytes[i + 1] = static_cast<u8>( ( value >> 16 ) & 0xFF );
            result.m_bytes[i + 2] = static_cast<u8>( ( value >> 8 ) & 0xFF );
            result.m_bytes[i + 3] = static_cast<u8>( value & 0xFF );
        }

        result.m_bytes[6] = static_cast<u8>( ( result.m_bytes[6] & 0x0F ) | 0x40 );
        result.m_bytes[8] = static_cast<u8>( ( result.m_bytes[8] & 0x3F ) | 0x80 );
        return result;
    }

    uuid uuid::from_string( const std::string &str )
    {
        if( str.empty() )
        {
            return uuid();
        }

        const auto hasOpeningBrace = str.front() == '{';
        const auto hasClosingBrace = str.back() == '}';
        if( hasOpeningBrace != hasClosingBrace )
        {
            throw std::invalid_argument( "Invalid uuid string braces" );
        }

        const auto first = hasOpeningBrace ? 1u : 0u;
        const auto last = hasClosingBrace ? str.size() - 1u : str.size();

        if( last <= first )
        {
            throw std::invalid_argument( "Invalid uuid string" );
        }

        std::string compact;
        compact.reserve( 32 );
        for( size_t i = first; i < last; ++i )
        {
            const auto ch = str[i];
            if( ch == '-' )
            {
                continue;
            }

            const auto value = hexValue( ch );
            if( value < 0 )
            {
                throw std::invalid_argument( "Invalid uuid string" );
            }

            compact.push_back( ch );
        }

        if( compact.size() != 32 )
        {
            throw std::invalid_argument( "Invalid uuid string length" );
        }

        uuid result;
        for( size_t i = 0; i < result.m_bytes.size(); ++i )
        {
            const auto high = hexValue( compact[i * 2] );
            const auto low = hexValue( compact[i * 2 + 1] );
            assert( high >= 0 && low >= 0 );
            result.m_bytes[i] = static_cast<u8>( ( high << 4 ) | low );
        }

        return result;
    }

    uuid uuid::from_string( const std::wstring &str )
    {
        std::string narrow;
        narrow.reserve( str.size() );
        for( const auto ch : str )
        {
            if( ch > 0x7F )
            {
                throw std::invalid_argument( "Invalid uuid string" );
            }

            narrow.push_back( static_cast<char>( ch ) );
        }

        return from_string( narrow );
    }

    String uuid::to_string() const
    {
        std::ostringstream stream;
        stream << std::hex << std::nouppercase << std::setfill( '0' );

        for( size_t i = 0; i < m_bytes.size(); ++i )
        {
            stream << std::setw( 2 ) << static_cast<int>( m_bytes[i] );
            if( i == 3 || i == 5 || i == 7 || i == 9 )
            {
                stream << '-';
            }
        }

        return stream.str().c_str();
    }

    StringW uuid::to_wstring() const
    {
        const auto value = to_string();
        std::wstring wide;
        wide.reserve( value.size() );
        for( const auto ch : value )
        {
            wide.push_back( static_cast<wchar_t>( ch ) );
        }

        return wide.c_str();
    }

    bool uuid::is_nil() const
    {
        for( const auto byte : m_bytes )
        {
            if( byte != 0 )
            {
                return false;
            }
        }

        return true;
    }

    s32 uuid::variant() const
    {
        if( ( m_bytes[8] & 0x80 ) == 0x00 )
            return 0;
        if( ( m_bytes[8] & 0xC0 ) == 0x80 )
            return 1;
        if( ( m_bytes[8] & 0xE0 ) == 0xC0 )
            return 2;
        return 3;
    }

    s32 uuid::version() const
    {
        return static_cast<s32>( ( m_bytes[6] & 0xF0 ) >> 4 );
    }

    const uuid::storage_type &uuid::bytes() const
    {
        return m_bytes;
    }

    uuid::value_type *uuid::data()
    {
        return m_bytes.data();
    }

    const uuid::value_type *uuid::data() const
    {
        return m_bytes.data();
    }

    size_t uuid::size() const
    {
        return m_bytes.size();
    }

    uuid::iterator uuid::begin()
    {
        return m_bytes.begin();
    }

    uuid::const_iterator uuid::begin() const
    {
        return m_bytes.begin();
    }

    uuid::const_iterator uuid::cbegin() const
    {
        return m_bytes.cbegin();
    }

    uuid::iterator uuid::end()
    {
        return m_bytes.end();
    }

    uuid::const_iterator uuid::end() const
    {
        return m_bytes.end();
    }

    uuid::const_iterator uuid::cend() const
    {
        return m_bytes.cend();
    }

    uuid::value_type &uuid::operator[]( size_t index )
    {
        assert( index < m_bytes.size() );
        return m_bytes[index];
    }

    const uuid::value_type &uuid::operator[]( size_t index ) const
    {
        assert( index < m_bytes.size() );
        return m_bytes[index];
    }

    bool uuid::operator==( const uuid &other ) const
    {
        return m_bytes == other.m_bytes;
    }

    bool uuid::operator!=( const uuid &other ) const
    {
        return !( *this == other );
    }

    bool uuid::operator<( const uuid &other ) const
    {
        return m_bytes < other.m_bytes;
    }

    bool uuid::operator<=( const uuid &other ) const
    {
        return !( other < *this );
    }

    bool uuid::operator>( const uuid &other ) const
    {
        return other < *this;
    }

    bool uuid::operator>=( const uuid &other ) const
    {
        return !( *this < other );
    }

    s32 uuid::hexValue( char ch )
    {
        if( ch >= '0' && ch <= '9' )
            return ch - '0';
        if( ch >= 'a' && ch <= 'f' )
            return 10 + ( ch - 'a' );
        if( ch >= 'A' && ch <= 'F' )
            return 10 + ( ch - 'A' );
        return -1;
    }

    String uuid::to_string( const uuid &value )
    {
        return value.to_string();
    }

    StringW uuid::to_wstring( const uuid &value )
    {
        return value.to_wstring();
    }

}  // namespace workphone
