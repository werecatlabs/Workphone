#include "WPRakNet/CPacket.hpp"
#include "WPRakNet/CSystemAddress.hpp"
#include <Workphone/Core/Handle.hpp>

using namespace RakNet;

namespace workphone
{

    CPacket::CPacket() : m_packet( nullptr ), m_bitStream( nullptr )
    {
    }

    CPacket::~CPacket()
    {
        delete m_bitStream;
    }

    void CPacket::initialise()
    {
        delete m_bitStream;
        m_packet = nullptr;
        m_bitStream = new BitStream;
        m_systemAddress = new CSystemAddress;
    }

    void CPacket::initialise( Packet *packet )
    {
        delete m_bitStream;
        m_bitStream = nullptr;
        m_packet = packet;
        if( !m_packet || !m_packet->data || m_packet->length == 0 )
        {
            m_systemAddress = new CSystemAddress;
            return;
        }

        m_bitStream = new BitStream( packet->data, packet->length, false );

        CSystemAddressPtr systemAddress = new CSystemAddress;
        *systemAddress->getSystemAddress() = m_packet->systemAddress;
        m_systemAddress = systemAddress;
    }

    void CPacket::read( s8 &value )
    {
        if( m_bitStream )
            m_bitStream->Read( value );
    }

    void CPacket::read( u8 &value )
    {
        if( m_bitStream )
            m_bitStream->Read( value );
    }

    void CPacket::read( u16 &value )
    {
        if( m_bitStream )
            m_bitStream->Read( value );
    }

    void CPacket::read( s16 &value )
    {
        if( m_bitStream )
            m_bitStream->Read( value );
    }

    void CPacket::read( u32 &value )
    {
        if( m_bitStream )
            m_bitStream->Read( value );
    }

    void CPacket::read( s32 &value )
    {
        if( m_bitStream )
            m_bitStream->Read( value );
    }

    void CPacket::read( f32 &value )
    {
        if( m_bitStream )
            m_bitStream->Read( value );
    }

    void CPacket::read( Vector2I &value )
    {
        if( !m_bitStream )
            return;

        m_bitStream->Read( value.x );
        m_bitStream->Read( value.y );
    }

    void CPacket::read( Vector2F &value )
    {
        if( !m_bitStream )
            return;

        m_bitStream->Read( value.x );
        m_bitStream->Read( value.y );
    }

    void CPacket::read( Vector3I &value )
    {
        if( !m_bitStream )
            return;

        m_bitStream->Read( value.x );
        m_bitStream->Read( value.y );
        m_bitStream->Read( value.z );
    }

    void CPacket::read( Vector3F &value )
    {
        if( !m_bitStream )
            return;

        m_bitStream->Read( value.x );
        m_bitStream->Read( value.y );
        m_bitStream->Read( value.z );
    }

    void CPacket::read( String &value )
    {
        if( !m_bitStream )
            return;

        RakString rakValue;
        m_bitStream->Read( rakValue );
        value = rakValue.C_String();
    }

    void CPacket::read( bool &value )
    {
        if( m_bitStream )
            m_bitStream->Read( value );
    }

    void CPacket::write( s8 value )
    {
        if( m_bitStream )
            m_bitStream->Write( value );
    }

    void CPacket::write( u8 value )
    {
        if( m_bitStream )
            m_bitStream->Write( value );
    }

    void CPacket::write( u16 value )
    {
        if( m_bitStream )
            m_bitStream->Write( value );
    }

    void CPacket::write( s16 value )
    {
        if( m_bitStream )
            m_bitStream->Write( value );
    }

    void CPacket::write( u32 value )
    {
        if( m_bitStream )
            m_bitStream->Write( value );
    }

    void CPacket::write( s32 value )
    {
        if( m_bitStream )
            m_bitStream->Write( value );
    }

    void CPacket::write( f32 value )
    {
        if( m_bitStream )
            m_bitStream->Write( value );
    }

    void CPacket::write( const Vector2I &value )
    {
        if( !m_bitStream )
            return;

        m_bitStream->Write( value.x );
        m_bitStream->Write( value.y );
    }

    void CPacket::write( const Vector2F &value )
    {
        if( !m_bitStream )
            return;

        m_bitStream->Write( value.x );
        m_bitStream->Write( value.y );
    }

    void CPacket::write( const Vector3I &value )
    {
        if( !m_bitStream )
            return;

        m_bitStream->Write( value.x );
        m_bitStream->Write( value.y );
        m_bitStream->Write( value.z );
    }

    void CPacket::write( const Vector3F &value )
    {
        if( !m_bitStream )
            return;

        m_bitStream->Write( value.x );
        m_bitStream->Write( value.y );
        m_bitStream->Write( value.z );
    }

    void CPacket::write( const String &value )
    {
        if( m_bitStream )
            m_bitStream->Write( RakString( value.c_str() ) );
    }

    void CPacket::write( const bool &value )
    {
        if( m_bitStream )
            m_bitStream->Write( value );
    }

    Packet *CPacket::getPacket() const
    {
        return m_packet;
    }

    void CPacket::setPacket( Packet *packet )
    {
        m_packet = packet;
    }

    BitStream *CPacket::getBitStream() const
    {
        return m_bitStream;
    }

    void CPacket::setBitStream( BitStream *bitStream )
    {
        if( m_bitStream == bitStream )
            return;

        delete m_bitStream;
        m_bitStream = bitStream;
    }

    SmartPtr<ISystemAddress> CPacket::getSystemAddress() const
    {
        return m_systemAddress;
    }

    void CPacket::setSystemAddress( SmartPtr<ISystemAddress> systemAddress )
    {
        m_systemAddress = systemAddress;
    }

    void CPacket::ignoreMessageId()
    {
        if( m_bitStream &&
            m_bitStream->GetNumberOfUnreadBits() >= BYTES_TO_BITS( sizeof( unsigned char ) ) )
            m_bitStream->IgnoreBytes( sizeof( unsigned char ) );
    }

    u32 CPacket::getDataLength() const
    {
        return m_bitStream ? static_cast<u32>( m_bitStream->GetNumberOfBytesUsed() ) : 0u;
    }

    void CPacket::resetReadPointer()
    {
        if( m_bitStream )
            m_bitStream->ResetReadPointer();
    }
}  // namespace workphone
