#include <WPNetwork/WPNetworkPacket.hpp>
#include <WPNetwork/WPNetworkSystemAddress.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Net/NetworkCodec.hpp>
#include <cstring>
#include <stdexcept>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPNetworkPacket, IPacket );

    WPNetworkPacket::WPNetworkPacket()
    {
        initialise();
    }

    WPNetworkPacket::WPNetworkPacket( const NetEvent &event )
    {
        initialise( event );
    }

    WPNetworkPacket::~WPNetworkPacket() = default;

    void WPNetworkPacket::initialise()
    {
        m_buffer.clear();
        m_readPosition = 0;
        m_systemAddress = nullptr;
    }

    void WPNetworkPacket::initialise( const NetEvent &event )
    {
        if( event.size > sizeof( event.data ) )
            throw std::length_error( "WPNetwork: invalid native event size" );
        setData( event.data, event.size );
    }

    void WPNetworkPacket::appendBytes( const void *data, size_t size )
    {
        network::appendBytes( m_buffer, data, size );
    }

    void WPNetworkPacket::extractBytes( void *data, size_t size )
    {
        network::requireBytes( m_readPosition, size, m_buffer.size() );
        if( size == 0 )
            return;
        if( !data )
            throw std::invalid_argument( "Network codec: null output" );
        std::memcpy( data, m_buffer.data() + m_readPosition, size );
        m_readPosition += size;
    }

    void WPNetworkPacket::read( s8 &value )
    {
        network::read( m_buffer, m_readPosition, value );
    }
    void WPNetworkPacket::read( u8 &value )
    {
        network::read( m_buffer, m_readPosition, value );
    }
    void WPNetworkPacket::read( u16 &value )
    {
        network::read( m_buffer, m_readPosition, value );
    }
    void WPNetworkPacket::read( s16 &value )
    {
        network::read( m_buffer, m_readPosition, value );
    }
    void WPNetworkPacket::read( u32 &value )
    {
        network::read( m_buffer, m_readPosition, value );
    }
    void WPNetworkPacket::read( s32 &value )
    {
        network::read( m_buffer, m_readPosition, value );
    }
    void WPNetworkPacket::read( f32 &value )
    {
        network::read( m_buffer, m_readPosition, value );
    }

    void WPNetworkPacket::read( Vector2I &value )
    {
        network::requireBytes( m_readPosition, 8, m_buffer.size() );
        auto decoded = Vector2I();
        read( decoded.x );
        read( decoded.y );
        value = decoded;
    }

    void WPNetworkPacket::read( Vector2<real_Num> &value )
    {
        network::requireBytes( m_readPosition, 8, m_buffer.size() );
        auto decoded = Vector2<real_Num>();
        f32 x = 0;
        read( x );
        decoded.x = static_cast<real_Num>( x );
        f32 y = 0;
        read( y );
        decoded.y = static_cast<real_Num>( y );
        value = decoded;
    }

    void WPNetworkPacket::read( Vector3I &value )
    {
        network::requireBytes( m_readPosition, 12, m_buffer.size() );
        auto decoded = Vector3I();
        read( decoded.x );
        read( decoded.y );
        read( decoded.z );
        value = decoded;
    }

    void WPNetworkPacket::read( Vector3<real_Num> &value )
    {
        network::requireBytes( m_readPosition, 12, m_buffer.size() );
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

    void WPNetworkPacket::read( String &value )
    {
        network::readString( m_buffer, m_readPosition, value );
    }

    void WPNetworkPacket::read( bool &value )
    {
        network::readBool( m_buffer, m_readPosition, value );
    }

    void WPNetworkPacket::write( s8 value )
    {
        network::write( m_buffer, value );
    }
    void WPNetworkPacket::write( u8 value )
    {
        network::write( m_buffer, value );
    }
    void WPNetworkPacket::write( u16 value )
    {
        network::write( m_buffer, value );
    }
    void WPNetworkPacket::write( s16 value )
    {
        network::write( m_buffer, value );
    }
    void WPNetworkPacket::write( u32 value )
    {
        network::write( m_buffer, value );
    }
    void WPNetworkPacket::write( s32 value )
    {
        network::write( m_buffer, value );
    }
    void WPNetworkPacket::write( f32 value )
    {
        network::write( m_buffer, value );
    }

    void WPNetworkPacket::write( const Vector2I &value )
    {
        if( m_buffer.size() > network::MaxMessageBytes - 8 )
            throw std::length_error( "Network vector exceeds message capacity" );
        write( value.x );
        write( value.y );
    }

    void WPNetworkPacket::write( const Vector2<real_Num> &value )
    {
        if( m_buffer.size() > network::MaxMessageBytes - 8 )
            throw std::length_error( "Network vector exceeds message capacity" );
        write( static_cast<f32>( value.x ) );
        write( static_cast<f32>( value.y ) );
    }

    void WPNetworkPacket::write( const Vector3I &value )
    {
        if( m_buffer.size() > network::MaxMessageBytes - 12 )
            throw std::length_error( "Network vector exceeds message capacity" );
        write( value.x );
        write( value.y );
        write( value.z );
    }

    void WPNetworkPacket::write( const Vector3<real_Num> &value )
    {
        if( m_buffer.size() > network::MaxMessageBytes - 12 )
            throw std::length_error( "Network vector exceeds message capacity" );
        write( static_cast<f32>( value.x ) );
        write( static_cast<f32>( value.y ) );
        write( static_cast<f32>( value.z ) );
    }

    void WPNetworkPacket::write( const String &value )
    {
        network::writeString( m_buffer, value );
    }

    void WPNetworkPacket::write( const bool &value )
    {
        write( static_cast<u8>( value ? 1 : 0 ) );
    }

    void WPNetworkPacket::ignoreMessageId()
    {
        if( m_readPosition < m_buffer.size() )
            ++m_readPosition;
    }

    u32 WPNetworkPacket::getDataLength() const
    {
        return static_cast<u32>( m_buffer.size() );
    }

    void WPNetworkPacket::resetReadPointer()
    {
        m_readPosition = 0;
    }

    SmartPtr<ISystemAddress> WPNetworkPacket::getSystemAddress() const
    {
        return m_systemAddress;
    }

    const u8 *WPNetworkPacket::getData() const
    {
        return m_buffer.empty() ? nullptr : m_buffer.data();
    }

    void WPNetworkPacket::setData( const void *data, size_t size )
    {
        network::assignBytes( m_buffer, data, size );
        m_readPosition = 0;
    }
    void WPNetworkPacket::setSystemAddress( SmartPtr<ISystemAddress> systemAddress )
    {
        m_systemAddress = systemAddress;
    }
}  // namespace workphone
