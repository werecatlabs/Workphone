#include <WPNetwork/WPNetworkPacket.hpp>
#include <WPNetwork/WPNetworkSystemAddress.hpp>
#include <Workphone/Workphone.hpp>
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
        setData( event.data, event.size );
    }

    void WPNetworkPacket::appendBytes( const void *data, size_t size )
    {
        if( !data || size == 0 )
            return;

        const auto *bytes = static_cast<const u8 *>( data );
        m_buffer.insert( m_buffer.end(), bytes, bytes + size );
    }

    void WPNetworkPacket::extractBytes( void *data, size_t size )
    {
        if( size == 0 )
            return;

        if( !data || m_readPosition + size > m_buffer.size() )
            throw std::out_of_range( "WPNetworkPacket: read past end of packet" );

        std::memcpy( data, m_buffer.data() + m_readPosition, size );
        m_readPosition += size;
    }

    void WPNetworkPacket::read( s8 &value )
    {
        extractBytes( &value, sizeof( value ) );
    }
    void WPNetworkPacket::read( u8 &value )
    {
        extractBytes( &value, sizeof( value ) );
    }
    void WPNetworkPacket::read( u16 &value )
    {
        extractBytes( &value, sizeof( value ) );
    }
    void WPNetworkPacket::read( s16 &value )
    {
        extractBytes( &value, sizeof( value ) );
    }
    void WPNetworkPacket::read( u32 &value )
    {
        extractBytes( &value, sizeof( value ) );
    }
    void WPNetworkPacket::read( s32 &value )
    {
        extractBytes( &value, sizeof( value ) );
    }
    void WPNetworkPacket::read( f32 &value )
    {
        extractBytes( &value, sizeof( value ) );
    }

    void WPNetworkPacket::read( Vector2I &value )
    {
        read( value.x );
        read( value.y );
    }

    void WPNetworkPacket::read( Vector2<real_Num> &value )
    {
        read( value.x );
        read( value.y );
    }

    void WPNetworkPacket::read( Vector3I &value )
    {
        read( value.x );
        read( value.y );
        read( value.z );
    }

    void WPNetworkPacket::read( Vector3<real_Num> &value )
    {
        read( value.x );
        read( value.y );
        read( value.z );
    }

    void WPNetworkPacket::read( String &value )
    {
        u32 length = 0;
        read( length );

        if( m_readPosition + length > m_buffer.size() )
            throw std::out_of_range( "WPNetworkPacket: string read past end of packet" );

        value.assign( reinterpret_cast<const char *>( m_buffer.data() + m_readPosition ), length );
        m_readPosition += length;
    }

    void WPNetworkPacket::read( bool &value )
    {
        u8 raw = 0;
        read( raw );
        value = raw != 0;
    }

    void WPNetworkPacket::write( s8 value )
    {
        appendBytes( &value, sizeof( value ) );
    }
    void WPNetworkPacket::write( u8 value )
    {
        appendBytes( &value, sizeof( value ) );
    }
    void WPNetworkPacket::write( u16 value )
    {
        appendBytes( &value, sizeof( value ) );
    }
    void WPNetworkPacket::write( s16 value )
    {
        appendBytes( &value, sizeof( value ) );
    }
    void WPNetworkPacket::write( u32 value )
    {
        appendBytes( &value, sizeof( value ) );
    }
    void WPNetworkPacket::write( s32 value )
    {
        appendBytes( &value, sizeof( value ) );
    }
    void WPNetworkPacket::write( f32 value )
    {
        appendBytes( &value, sizeof( value ) );
    }

    void WPNetworkPacket::write( const Vector2I &value )
    {
        write( value.x );
        write( value.y );
    }

    void WPNetworkPacket::write( const Vector2<real_Num> &value )
    {
        write( value.x );
        write( value.y );
    }

    void WPNetworkPacket::write( const Vector3I &value )
    {
        write( value.x );
        write( value.y );
        write( value.z );
    }

    void WPNetworkPacket::write( const Vector3<real_Num> &value )
    {
        write( value.x );
        write( value.y );
        write( value.z );
    }

    void WPNetworkPacket::write( const String &value )
    {
        const auto length = static_cast<u32>( value.size() );
        write( length );
        appendBytes( value.data(), length );
    }

    void WPNetworkPacket::write( const bool &value )
    {
        const u8 raw = value ? 1 : 0;
        write( raw );
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
        m_buffer.clear();
        m_readPosition = 0;

        if( data && size > 0 )
        {
            m_buffer.resize( size );
            std::memcpy( m_buffer.data(), data, size );
        }
    }

    void WPNetworkPacket::setSystemAddress( SmartPtr<ISystemAddress> systemAddress )
    {
        m_systemAddress = systemAddress;
    }
}  // namespace workphone
