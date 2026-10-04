#include <WPNetwork/WPNetworkStream.hpp>
#include <Workphone/Workphone.hpp>
#include <cstring>
#include <stdexcept>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPNetworkStream, INetworkStream );

    WPNetworkStream::WPNetworkStream( bool writing ) : m_writing( writing )
    {
    }

    WPNetworkStream::~WPNetworkStream() = default;

    bool WPNetworkStream::isWriting() const
    {
        return m_writing;
    }

    bool WPNetworkStream::isReading() const
    {
        return !m_writing;
    }

    void WPNetworkStream::appendBytes( const void *data, size_t size )
    {
        if( !data || size == 0 )
            return;

        const auto *bytes = static_cast<const u8 *>( data );
        m_buffer.insert( m_buffer.end(), bytes, bytes + size );
        m_position += size;
    }

    void WPNetworkStream::extractBytes( void *data, size_t size )
    {
        if( size == 0 )
            return;

        if( !data || m_position + size > m_buffer.size() )
            throw std::out_of_range( "WPNetworkStream: read past end of stream" );

        std::memcpy( data, m_buffer.data() + m_position, size );
        m_position += size;
    }

    size_t WPNetworkStream::read( void *buffer, size_t size )
    {
        const auto available = m_position < m_buffer.size() ? m_buffer.size() - m_position : 0;
        const auto toRead = size < available ? size : available;

        if( toRead == 0 )
            return 0;

        std::memcpy( buffer, m_buffer.data() + m_position, toRead );
        m_position += toRead;
        return toRead;
    }

    size_t WPNetworkStream::write( const void *buffer, size_t size )
    {
        appendBytes( buffer, size );
        return size;
    }

    void WPNetworkStream::read( s8 &value )
    {
        extractBytes( &value, sizeof( value ) );
    }
    void WPNetworkStream::read( u8 &value )
    {
        extractBytes( &value, sizeof( value ) );
    }
    void WPNetworkStream::read( s16 &value )
    {
        extractBytes( &value, sizeof( value ) );
    }
    void WPNetworkStream::read( u16 &value )
    {
        extractBytes( &value, sizeof( value ) );
    }
    void WPNetworkStream::read( s32 &value )
    {
        extractBytes( &value, sizeof( value ) );
    }
    void WPNetworkStream::read( u32 &value )
    {
        extractBytes( &value, sizeof( value ) );
    }
    void WPNetworkStream::read( f32 &value )
    {
        extractBytes( &value, sizeof( value ) );
    }

    void WPNetworkStream::read( bool &value )
    {
        u8 raw = 0;
        read( raw );
        value = raw != 0;
    }

    void WPNetworkStream::read( String &value )
    {
        u32 length = 0;
        read( length );

        if( m_position + length > m_buffer.size() )
            throw std::out_of_range( "WPNetworkStream: string read past end of stream" );

        value.assign( reinterpret_cast<const char *>( m_buffer.data() + m_position ), length );
        m_position += length;
    }

    void WPNetworkStream::read( Vector2I &value )
    {
        read( value.x );
        read( value.y );
    }

    void WPNetworkStream::read( Vector2<real_Num> &value )
    {
        read( value.x );
        read( value.y );
    }

    void WPNetworkStream::read( Vector3I &value )
    {
        read( value.x );
        read( value.y );
        read( value.z );
    }

    void WPNetworkStream::read( Vector3<real_Num> &value )
    {
        read( value.x );
        read( value.y );
        read( value.z );
    }

    void WPNetworkStream::write( s8 value )
    {
        appendBytes( &value, sizeof( value ) );
    }
    void WPNetworkStream::write( u8 value )
    {
        appendBytes( &value, sizeof( value ) );
    }
    void WPNetworkStream::write( s16 value )
    {
        appendBytes( &value, sizeof( value ) );
    }
    void WPNetworkStream::write( u16 value )
    {
        appendBytes( &value, sizeof( value ) );
    }
    void WPNetworkStream::write( s32 value )
    {
        appendBytes( &value, sizeof( value ) );
    }
    void WPNetworkStream::write( u32 value )
    {
        appendBytes( &value, sizeof( value ) );
    }
    void WPNetworkStream::write( f32 value )
    {
        appendBytes( &value, sizeof( value ) );
    }

    void WPNetworkStream::write( bool value )
    {
        const u8 raw = value ? 1 : 0;
        write( raw );
    }

    void WPNetworkStream::write( const String &value )
    {
        const auto length = static_cast<u32>( value.size() );
        write( length );
        appendBytes( value.data(), length );
    }

    void WPNetworkStream::write( const Vector2I &value )
    {
        write( value.x );
        write( value.y );
    }

    void WPNetworkStream::write( const Vector2<real_Num> &value )
    {
        write( value.x );
        write( value.y );
    }

    void WPNetworkStream::write( const Vector3I &value )
    {
        write( value.x );
        write( value.y );
        write( value.z );
    }

    void WPNetworkStream::write( const Vector3<real_Num> &value )
    {
        write( value.x );
        write( value.y );
        write( value.z );
    }

    size_t WPNetworkStream::getSize() const
    {
        return m_buffer.size();
    }

    size_t WPNetworkStream::getPosition() const
    {
        return m_position;
    }

    void WPNetworkStream::reset()
    {
        m_position = 0;
    }

    const u8 *WPNetworkStream::getData() const
    {
        return m_buffer.empty() ? nullptr : m_buffer.data();
    }

    void WPNetworkStream::setData( const void *data, size_t size )
    {
        m_buffer.clear();
        if( data && size > 0 )
        {
            const auto *bytes = static_cast<const u8 *>( data );
            //m_buffer.assign( bytes, bytes + size );
        }

        m_position = 0;
    }
}  // namespace workphone
