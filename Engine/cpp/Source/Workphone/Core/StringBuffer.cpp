#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/StringBuffer.hpp>

#include <stdexcept>
#include <cstdint>
#include <utility>

namespace workphone
{
    StringBuffer::StringBuffer() :
        m_buffer( m_smallBuffer ),
        m_length( 0 ),
        m_capacity( SMALL_BUFFER_SIZE - 1 ),
        m_usingSmallBuffer( true )
    {
        m_smallBuffer[0] = '\0';
    }

    StringBuffer::StringBuffer( size_t capacity ) : StringBuffer()
    {
        reserve( capacity );
    }

    StringBuffer::StringBuffer( const char *str ) : StringBuffer()
    {
        append( str );
    }

    StringBuffer::StringBuffer( const std::string &str ) : StringBuffer()
    {
        append( str );
    }

    StringBuffer::StringBuffer( const StringBuffer &other ) : StringBuffer()
    {
        ensureCapacity( other.m_length );
        std::memcpy( m_buffer, other.m_buffer, other.m_length + 1 );
        m_length = other.m_length;
    }

    StringBuffer::StringBuffer( StringBuffer &&other ) noexcept : StringBuffer()
    {
        if( other.m_usingSmallBuffer )
        {
            std::memcpy( m_smallBuffer, other.m_smallBuffer, other.m_length + 1 );
            m_length = other.m_length;
        }
        else
        {
            m_buffer = other.m_buffer;
            m_length = other.m_length;
            m_capacity = other.m_capacity;
            m_usingSmallBuffer = false;
        }

        other.m_buffer = other.m_smallBuffer;
        other.m_length = 0;
        other.m_capacity = SMALL_BUFFER_SIZE - 1;
        other.m_usingSmallBuffer = true;
        other.m_smallBuffer[0] = '\0';
    }

    StringBuffer::~StringBuffer()
    {
        destroy();
    }

    StringBuffer &StringBuffer::operator=( const StringBuffer &other )
    {
        if( this != &other )
        {
            StringBuffer copy( other );
            *this = std::move( copy );
        }
        return *this;
    }

    StringBuffer &StringBuffer::operator=( StringBuffer &&other ) noexcept
    {
        if( this == &other )
            return *this;

        destroy();
        m_buffer = m_smallBuffer;
        m_length = 0;
        m_capacity = SMALL_BUFFER_SIZE - 1;
        m_usingSmallBuffer = true;

        if( other.m_usingSmallBuffer )
        {
            std::memcpy( m_smallBuffer, other.m_smallBuffer, other.m_length + 1 );
            m_length = other.m_length;
        }
        else
        {
            m_buffer = other.m_buffer;
            m_length = other.m_length;
            m_capacity = other.m_capacity;
            m_usingSmallBuffer = false;
        }

        other.m_buffer = other.m_smallBuffer;
        other.m_length = 0;
        other.m_capacity = SMALL_BUFFER_SIZE - 1;
        other.m_usingSmallBuffer = true;
        other.m_smallBuffer[0] = '\0';
        return *this;
    }

    StringBuffer &StringBuffer::append( const char *str )
    {
        if( str == nullptr )
            return *this;
        return append( str, 0, std::strlen( str ) );
    }

    StringBuffer &StringBuffer::append( const std::string &str )
    {
        return append( str.data(), 0, str.size() );
    }

    StringBuffer &StringBuffer::append( const StringBuffer &other )
    {
        if( this == &other )
        {
            const std::string copy = toString();
            return append( copy );
        }
        return append( other.m_buffer, 0, other.m_length );
    }

    StringBuffer &StringBuffer::append( char ch )
    {
        if( m_length == static_cast<size_t>( -1 ) )
            throw std::length_error( "StringBuffer capacity overflow" );
        ensureCapacity( m_length + 1 );
        m_buffer[m_length++] = ch;
        m_buffer[m_length] = '\0';
        return *this;
    }

    StringBuffer &StringBuffer::append( const char *str, size_t pos, size_t len )
    {
        if( str == nullptr )
            return *this;

        const size_t sourceLength = std::strlen( str );
        if( pos > sourceLength )
            throw std::out_of_range( "StringBuffer append position is out of range" );

        const size_t count = len == npos ? sourceLength - pos : std::min( len, sourceLength - pos );
        if( count == 0 )
            return *this;

        // Preserve aliased input if growth would invalidate it.
        const auto sourceAddress = reinterpret_cast<std::uintptr_t>( str );
        const auto bufferAddress = reinterpret_cast<std::uintptr_t>( m_buffer );
        const bool aliases = sourceAddress >= bufferAddress && sourceAddress - bufferAddress <= m_length;
        if( aliases )
        {
            const std::string copy( str + pos, count );
            return append( copy );
        }

        if( count > static_cast<size_t>( -1 ) - m_length )
            throw std::length_error( "StringBuffer capacity overflow" );

        ensureCapacity( m_length + count );
        std::memcpy( m_buffer + m_length, str + pos, count );
        m_length += count;
        m_buffer[m_length] = '\0';
        return *this;
    }

    void StringBuffer::clear()
    {
        m_length = 0;
        m_buffer[0] = '\0';
    }

    void StringBuffer::reserve( size_t capacity )
    {
        ensureCapacity( capacity );
    }

    size_t StringBuffer::length() const
    {
        return m_length;
    }
    size_t StringBuffer::size() const
    {
        return m_length;
    }
    size_t StringBuffer::capacity() const
    {
        return m_capacity;
    }
    bool StringBuffer::empty() const
    {
        return m_length == 0;
    }
    const char *StringBuffer::c_str() const
    {
        return m_buffer;
    }
    const char *StringBuffer::data() const
    {
        return m_buffer;
    }
    std::string StringBuffer::toString() const
    {
        return std::string( m_buffer, m_length );
    }

    StringBuffer &StringBuffer::operator+=( const char *str )
    {
        return append( str );
    }
    StringBuffer &StringBuffer::operator+=( const std::string &str )
    {
        return append( str );
    }
    StringBuffer &StringBuffer::operator+=( char ch )
    {
        return append( ch );
    }

    char StringBuffer::operator[]( size_t index ) const
    {
        if( index >= m_length )
            throw std::out_of_range( "StringBuffer index is out of range" );
        return m_buffer[index];
    }

    bool StringBuffer::operator==( const StringBuffer &other ) const
    {
        return m_length == other.m_length && std::memcmp( m_buffer, other.m_buffer, m_length ) == 0;
    }

    bool StringBuffer::operator!=( const StringBuffer &other ) const
    {
        return !( *this == other );
    }

    void StringBuffer::ensureCapacity( size_t requiredCapacity )
    {
        if( requiredCapacity <= m_capacity )
            return;

        size_t newCapacity = m_capacity;
        while( newCapacity < requiredCapacity )
        {
            if( newCapacity > ( static_cast<size_t>( -1 ) - 1 ) / 2 )
            {
                newCapacity = requiredCapacity;
                break;
            }
            newCapacity = newCapacity * 2 + 1;
        }

        char *newBuffer = new char[newCapacity + 1];
        std::memcpy( newBuffer, m_buffer, m_length + 1 );
        if( !m_usingSmallBuffer )
            delete[] m_buffer;

        m_buffer = newBuffer;
        m_capacity = newCapacity;
        m_usingSmallBuffer = false;
    }

    void StringBuffer::destroy()
    {
        if( !m_usingSmallBuffer )
            delete[] m_buffer;
    }
}  // namespace workphone
