#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/IO/MemoryFile.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, MemoryFile, DataStream );

    MemoryFile::MemoryFile() = default;

    MemoryFile::MemoryFile( void *memory, long len, const String &fileName, bool d ) :
        DataStream( fileName, static_cast<u16>( AccessMode::Read ) ),
        m_buffer( static_cast<u8 *>( memory ) ),
        m_size( len > 0 ? static_cast<size_Num>( len ) : 0 ),
        m_freeMemory( d )
    {
        WP_ASSERT( len >= 0 );
        WP_ASSERT( memory != nullptr || len == 0 );

        m_position = m_buffer;
        m_end = m_buffer + m_size;
    }

    MemoryFile::~MemoryFile()
    {
        if( m_freeMemory )
        {
            delete[] reinterpret_cast<c8 *>( m_buffer );
        }
    }

    auto MemoryFile::read( void *buffer, size_t sizeToRead ) -> size_t
    {
        if( !m_buffer )
        {
            return 0;
        }

        size_t cnt = sizeToRead;
        // Read over end of memory?
        if( m_position + cnt > m_end )
        {
            cnt = m_end - m_position;
        }

        if( cnt == 0 )
        {
            return 0;
        }

        assert( cnt <= sizeToRead );

        memcpy( buffer, m_position, cnt );
        m_position += cnt;
        return cnt;
    }

    auto MemoryFile::write( const void *buffer, size_t sizeToWrite ) -> size_t
    {
        if( !m_buffer )
        {
            return 0;
        }

        size_t written = 0;

        if( isWriteable() )
        {
            written = sizeToWrite;

            // we only allow writing within the extents of allocated memory
            // check for buffer overrun & disallow
            if( m_position + written > m_end )
            {
                written = m_end - m_position;
            }

            if( written == 0 )
            {
                return 0;
            }

            memcpy( m_position, buffer, written );
            m_position += written;
        }

        return written;
    }

    auto MemoryFile::seek( size_Num finalPos ) -> bool
    {
        if( !m_buffer || finalPos > m_size )
        {
            return false;
        }

        m_position = m_buffer + finalPos;
        return true;
    }

    auto MemoryFile::size() const -> size_Num
    {
        return m_size;
    }

    auto MemoryFile::tell() const -> size_Num
    {
        if( !m_buffer )
        {
            return 0;
        }

        return m_position - m_buffer;
    }

    auto MemoryFile::getBuffer() const -> void *
    {
        return m_buffer;
    }

    void MemoryFile::setBuffer( void *buffer )
    {
        m_buffer = static_cast<u8 *>( buffer );
    }

    auto MemoryFile::getFreeMemory() const -> bool
    {
        return m_freeMemory;
    }

    void MemoryFile::setFreeMemory( bool freeMemory )
    {
        m_freeMemory = freeMemory;
    }

    auto MemoryFile::getData() const -> void *
    {
        return m_buffer;
    }

    auto MemoryFile::getCharPtr() const -> const c8 *
    {
        return reinterpret_cast<char *>( m_buffer );
    }

    auto MemoryFile::eof() const -> bool
    {
        return m_position >= m_end;
    }

    auto MemoryFile::isOpen() const -> bool
    {
        return m_buffer != nullptr;
    }

    void MemoryFile::close()
    {
        if( m_freeMemory && m_buffer )
        {
            delete[] reinterpret_cast<c8 *>( m_buffer );
            m_freeMemory = false;
        }

        m_buffer = nullptr;
        m_position = nullptr;
        m_end = nullptr;
        m_size = 0;
    }

    auto MemoryFile::skipLine( const String &delim ) -> size_Num
    {
        size_Num pos = 0;

        // Make sure pos can never go past the end of the data
        while( m_position < m_end )
        {
            ++pos;
            if( delim.find( *m_position++ ) != String::npos )
            {
                // Found terminator, break out
                break;
            }
        }

        return pos;
    }

    void MemoryFile::skip( size_Num count )
    {
        if( !m_buffer )
        {
            return;
        }

        auto newpos = static_cast<size_t>( ( m_position - m_buffer ) + count );

        if( m_buffer + newpos > m_end )
        {
            newpos = static_cast<size_t>( m_end - m_buffer );
        }

        m_position = m_buffer + newpos;
    }
}  // namespace workphone
