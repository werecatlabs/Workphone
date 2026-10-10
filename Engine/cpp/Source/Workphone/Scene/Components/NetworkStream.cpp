#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/NetworkStream.hpp>

#include <Workphone/Interface/Net/NetworkCodec.hpp>
#include <cstring>
#include <stdexcept>

namespace workphone
{
    namespace scene
    {

        WP_CLASS_REGISTER_DERIVED( workphone, NetworkStream, INetworkStream );

        NetworkStream::NetworkStream( bool writing ) : m_writing( writing )
        {
        }

        NetworkStream::~NetworkStream() = default;

        // ------------------------------------------------------------------
        // Private helpers
        // ------------------------------------------------------------------

    void NetworkStream::appendBytes( const void *src, size_t count )
    {
        network::appendBytes( m_buffer, src, count );
        m_position = m_buffer.size();
    }

    void NetworkStream::extractBytes( void *dst, size_t count )
    {
        network::requireBytes( m_position, count, m_buffer.size() );
        if( count == 0 )
            return;
        if( !dst )
            throw std::invalid_argument( "Network codec: null output" );
        std::memcpy( dst, m_buffer.data() + m_position, count );
        m_position += count;
    }

        // ------------------------------------------------------------------
        // Mode
        // ------------------------------------------------------------------

        bool NetworkStream::isWriting() const
        {
            return m_writing;
        }

        bool NetworkStream::isReading() const
        {
            return !m_writing;
        }

        // ------------------------------------------------------------------
        // Raw bytes
        // ------------------------------------------------------------------

    size_t NetworkStream::read( void *buffer, size_t size )
    {
        network::requireBytes( m_position, 0, m_buffer.size() );
        const auto available = m_buffer.size() - m_position;
        const auto count = std::min( size, available );
        extractBytes( buffer, count );
        return count;
    }

        size_t NetworkStream::write( const void *buffer, size_t size )
        {
            appendBytes( buffer, size );
            return size;
        }

        // ------------------------------------------------------------------
        // Typed reads
        // ------------------------------------------------------------------

    void NetworkStream::read( s8 &value )
    {
        network::read( m_buffer, m_position, value );
    }
    void NetworkStream::read( u8 &value )
    {
        network::read( m_buffer, m_position, value );
    }
    void NetworkStream::read( s16 &value )
    {
        network::read( m_buffer, m_position, value );
    }
    void NetworkStream::read( u16 &value )
    {
        network::read( m_buffer, m_position, value );
    }
    void NetworkStream::read( s32 &value )
    {
        network::read( m_buffer, m_position, value );
    }
    void NetworkStream::read( u32 &value )
    {
        network::read( m_buffer, m_position, value );
    }
    void NetworkStream::read( f32 &value )
    {
        network::read( m_buffer, m_position, value );
    }

    void NetworkStream::read( bool &value )
    {
        network::readBool( m_buffer, m_position, value );
    }

    void NetworkStream::read( String &value )
    {
        network::readString( m_buffer, m_position, value );
    }

    void NetworkStream::read( Vector2I &value )
    {
        network::requireBytes( m_position, 8, m_buffer.size() );
        auto decoded = Vector2I();
        read( decoded.x );
        read( decoded.y );
        value = decoded;
    }

    void NetworkStream::read( Vector2<real_Num> &value )
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

    void NetworkStream::read( Vector3I &value )
    {
        network::requireBytes( m_position, 12, m_buffer.size() );
        auto decoded = Vector3I();
        read( decoded.x );
        read( decoded.y );
        read( decoded.z );
        value = decoded;
    }

    void NetworkStream::read( Vector3<real_Num> &value )
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

        // ------------------------------------------------------------------
        // Typed writes
        // ------------------------------------------------------------------

    void NetworkStream::write( s8 value )
    {
        network::write( m_buffer, value );
        m_position = m_buffer.size();
    }
    void NetworkStream::write( u8 value )
    {
        network::write( m_buffer, value );
        m_position = m_buffer.size();
    }
    void NetworkStream::write( s16 value )
    {
        network::write( m_buffer, value );
        m_position = m_buffer.size();
    }
    void NetworkStream::write( u16 value )
    {
        network::write( m_buffer, value );
        m_position = m_buffer.size();
    }
    void NetworkStream::write( s32 value )
    {
        network::write( m_buffer, value );
        m_position = m_buffer.size();
    }
    void NetworkStream::write( u32 value )
    {
        network::write( m_buffer, value );
        m_position = m_buffer.size();
    }
    void NetworkStream::write( f32 value )
    {
        network::write( m_buffer, value );
        m_position = m_buffer.size();
    }

    void NetworkStream::write( bool value )
    {
        write( static_cast<u8>( value ? 1 : 0 ) );
    }

    void NetworkStream::write( const String &value )
    {
        network::writeString( m_buffer, value );
        m_position = m_buffer.size();
    }

    void NetworkStream::write( const Vector2I &value )
    {
        if( m_buffer.size() > network::MaxMessageBytes - 8 )
            throw std::length_error( "Network vector exceeds message capacity" );
        write( value.x );
        write( value.y );
    }

    void NetworkStream::write( const Vector2<real_Num> &value )
    {
        if( m_buffer.size() > network::MaxMessageBytes - 8 )
            throw std::length_error( "Network vector exceeds message capacity" );
        write( static_cast<f32>( value.x ) );
        write( static_cast<f32>( value.y ) );
    }

    void NetworkStream::write( const Vector3I &value )
    {
        if( m_buffer.size() > network::MaxMessageBytes - 12 )
            throw std::length_error( "Network vector exceeds message capacity" );
        write( value.x );
        write( value.y );
        write( value.z );
    }

    void NetworkStream::write( const Vector3<real_Num> &value )
    {
        if( m_buffer.size() > network::MaxMessageBytes - 12 )
            throw std::length_error( "Network vector exceeds message capacity" );
        write( static_cast<f32>( value.x ) );
        write( static_cast<f32>( value.y ) );
        write( static_cast<f32>( value.z ) );
    }

        // ------------------------------------------------------------------
        // Buffer management
        // ------------------------------------------------------------------

        size_t NetworkStream::getSize() const
        {
            return m_buffer.size();
        }

        size_t NetworkStream::getPosition() const
        {
            return m_position;
        }

        void NetworkStream::reset()
        {
            m_position = 0;
        }

        const u8 *NetworkStream::getData() const
        {
            return m_buffer.empty() ? nullptr : m_buffer.data();
        }

    void NetworkStream::setData( const void *data, size_t size )
    {
        network::assignBytes( m_buffer, data, size );
        m_position = 0;
    }

    }  // namespace scene
}  // namespace workphone
