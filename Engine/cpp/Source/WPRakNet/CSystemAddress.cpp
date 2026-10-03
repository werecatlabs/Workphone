#include "WPRakNet/CSystemAddress.hpp"
#include <Workphone/Core/Handle.hpp>
#include "Workphone/Memory/Memory.hpp"

using namespace RakNet;

namespace workphone
{
    //--------------------------------------------
    CSystemAddress::CSystemAddress()
    {
        m_systemAddress = new SystemAddress;
    }

    //--------------------------------------------
    CSystemAddress::~CSystemAddress()
    {
        WP_SAFE_DELETE( m_systemAddress );
    }

    //--------------------------------------------
    void CSystemAddress::setBinaryAddress( u32 binaryAddress )
    {
        if( m_systemAddress )
            m_systemAddress->address.addr4.sin_addr.s_addr = binaryAddress;
    }

    //--------------------------------------------
    u32 CSystemAddress::getBinaryAddress() const
    {
        return m_systemAddress ? static_cast<u32>( m_systemAddress->address.addr4.sin_addr.s_addr ) : 0u;
    }

    //--------------------------------------------
    void CSystemAddress::setPort( u16 port )
    {
        if( m_systemAddress )
            m_systemAddress->SetPortHostOrder( port );
    }

    //--------------------------------------------
    u16 CSystemAddress::getPort() const
    {
        return m_systemAddress ? m_systemAddress->GetPort() : 0u;
    }

    //--------------------------------------------
    String CSystemAddress::toString() const
    {
        return m_systemAddress ? String( m_systemAddress->ToString( true ) ) : String();
    }

    //--------------------------------------------
    bool CSystemAddress::isValid() const
    {
        return m_systemAddress && *m_systemAddress != UNASSIGNED_SYSTEM_ADDRESS;
    }

    //--------------------------------------------
    SystemAddress *CSystemAddress::getSystemAddress() const
    {
        return m_systemAddress;
    }

    void CSystemAddress::setSystemAddress( const SystemAddress &systemAddress )
    {
        if( !m_systemAddress )
            m_systemAddress = new SystemAddress;

        *m_systemAddress = systemAddress;
    }
}  // namespace workphone
