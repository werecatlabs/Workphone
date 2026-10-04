#include <WPNetwork/WPNetworkSystemAddress.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPNetworkSystemAddress, ISystemAddress );

    WPNetworkSystemAddress::WPNetworkSystemAddress()
    {
        m_address = net_make_address( 0, 0 );
    }

    WPNetworkSystemAddress::WPNetworkSystemAddress( const NetAddress &address ) :
        m_address( address ),
        m_valid( true )
    {
    }

    WPNetworkSystemAddress::~WPNetworkSystemAddress() = default;

    void WPNetworkSystemAddress::setBinaryAddress( u32 binaryAddress )
    {
        m_address.host = binaryAddress;
        m_valid = true;
    }

    u32 WPNetworkSystemAddress::getBinaryAddress() const
    {
        return m_address.host;
    }

    void WPNetworkSystemAddress::setPort( u16 port )
    {
        m_address.port = port;
        m_valid = true;
    }

    u16 WPNetworkSystemAddress::getPort() const
    {
        return m_address.port;
    }

    String WPNetworkSystemAddress::toString() const
    {
        char buffer[64] = {};
        return String( net_address_to_string( m_address, buffer, sizeof( buffer ) ) );
    }

    bool WPNetworkSystemAddress::isValid() const
    {
        return m_valid && ( m_address.host != 0 || m_address.port != 0 );
    }

    const NetAddress &WPNetworkSystemAddress::getNetAddress() const
    {
        return m_address;
    }

    void WPNetworkSystemAddress::setNetAddress( const NetAddress &address )
    {
        m_address = address;
        m_valid = true;
    }
}  // namespace workphone
