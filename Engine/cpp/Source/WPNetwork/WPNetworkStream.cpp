#include <WPNetwork/WPNetworkStream.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Net/NetworkCodec.hpp>
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
        network::appendBytes( m_buffer, data, size );
        m_position = m_buffer.size();
    }

    void WPNetworkStream::extractBytes( void *data, size_t size )
    {
        network::requireBytes( m_position, size, m_buffer.size() );
        if( size == 0 )
            return;
        if( !data )
            throw std::invalid_argument( "Network codec: null output" );
        std::memcpy( data, m_buffer.data() + m_position, size );
        m_position += size;
    }

    size_t WPNetworkStream::read( void *buffer, size_t size )
    {
        network::requireBytes( m_position, 0, m_buffer.size() );
        const auto available = m_buffer.size() - m_position;
        const auto count = std::min( size, available );
        extractBytes( buffer, count );
        return count;
    }

    size_t WPNetworkStream::write( const void *buffer, size_t size )
    {
        appendBytes( buffer, size );
        return size;
    }

    void WPNetworkStream::read( s8 &value )
    {
        network::read( m_buffer, m_position, value );
    }
    void WPNetworkStream::read( u8 &value )
    {
        network::read( m_buffer, m_position, value );
    }
    void WPNetworkStream::read( s16 &value )
    {
        network::read( m_buffer, m_position, value );
    }
    void WPNetworkStream::read( u16 &value )
    {
        network::read( m_buffer, m_position, value );
    }
    void WPNetworkStream::read( s32 &value )
    {
        network::read( m_buffer, m_position, value );
    }
    void WPNetworkStream::read( u32 &value )
    {
        network::read( m_buffer, m_position, value );
    }
    void WPNetworkStream::read( f32 &value )
    {
        network::read( m_buffer, m_position, value );
    }

    void WPNetworkStream::read( bool &value )
    {
        network::readBool( m_buffer, m_position, value );
    }

    void WPNetworkStream::read( String &value )
    {
        network::readString( m_buffer, m_position, value );
    }

    void WPNetworkStream::read( Vector2I &value )
    {
        network::requireBytes( m_position, 8, m_buffer.size() );
        auto decoded = Vector2I();
        read( decoded.x );
        read( decoded.y );
        value = decoded;
    }

    void WPNetworkStream::read( Vector2<real_Num> &value )
    {
        network::requireBytes( m_position, 8, m_buffer.size() );
        auto decoded = Vector2<real_Num>();
        f32 x = 0;
        read( x );
        decoded.x = static_cast<real_Num>( x );
        f32 y = 0;
        read( y );
        decoded.y = static_cast<real_Num>( y );
        value = decoded;
    }

    void WPNetworkStream::read( Vector3I &value )
    {
        network::requireBytes( m_position, 12, m_buffer.size() );
        auto decoded = Vector3I();
        read( decoded.x );
        read( decoded.y );
        read( decoded.z );
        value = decoded;
    }

    void WPNetworkStream::read( Vector3<real_Num> &value )
    {
        network::requireBytes( m_position, 12, m_buffer.size() );
        auto decoded = Vector3<real_Num>();
        f32 x = 0;
        read( x );
        decoded.x = static_cast<real_Num>( x );
        f32 y = 0;
        read( y );
        decoded.y = static_cast<real_Num>( y );
        f32 z = 0;
        read( z );
        decoded.z = static_cast<real_Num>( z );
        value = decoded;
    }

    void WPNetworkStream::write( s8 value )
    {
        network::write( m_buffer, value );
        m_position = m_buffer.size();
    }
    void WPNetworkStream::write( u8 value )
    {
        network::write( m_buffer, value );
        m_position = m_buffer.size();
    }
    void WPNetworkStream::write( s16 value )
    {
        network::write( m_buffer, value );
        m_position = m_buffer.size();
    }
    void WPNetworkStream::write( u16 value )
    {
        network::write( m_buffer, value );
        m_position = m_buffer.size();
    }
    void WPNetworkStream::write( s32 value )
    {
        network::write( m_buffer, value );
        m_position = m_buffer.size();
    }
    void WPNetworkStream::write( u32 value )
    {
        network::write( m_buffer, value );
        m_position = m_buffer.size();
    }
    void WPNetworkStream::write( f32 value )
    {
        network::write( m_buffer, value );
        m_position = m_buffer.size();
    }

    void WPNetworkStream::write( bool value )
    {
        write( static_cast<u8>( value ? 1 : 0 ) );
    }

    void WPNetworkStream::write( const String &value )
    {
        network::writeString( m_buffer, value );
        m_position = m_buffer.size();
    }

    void WPNetworkStream::write( const Vector2I &value )
    {
        if( m_buffer.size() > network::MaxMessageBytes - 8 )
            throw std::length_error( "Network vector exceeds message capacity" );
        write( value.x );
        write( value.y );
    }

    void WPNetworkStream::write( const Vector2<real_Num> &value )
    {
        if( m_buffer.size() > network::MaxMessageBytes - 8 )
            throw std::length_error( "Network vector exceeds message capacity" );
        write( static_cast<f32>( value.x ) );
        write( static_cast<f32>( value.y ) );
    }

    void WPNetworkStream::write( const Vector3I &value )
    {
        if( m_buffer.size() > network::MaxMessageBytes - 12 )
            throw std::length_error( "Network vector exceeds message capacity" );
        write( value.x );
        write( value.y );
        write( value.z );
    }

    void WPNetworkStream::write( const Vector3<real_Num> &value )
    {
        if( m_buffer.size() > network::MaxMessageBytes - 12 )
            throw std::length_error( "Network vector exceeds message capacity" );
        write( static_cast<f32>( value.x ) );
        write( static_cast<f32>( value.y ) );
        write( static_cast<f32>( value.z ) );
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
        network::assignBytes( m_buffer, data, size );
        m_position = 0;
    }

}  // namespace workphone
