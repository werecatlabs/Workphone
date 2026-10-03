#include "WPRakNet/UDPProxyCoordinatorFramework.hpp"
#include <BitStream.h>
#include <MessageIdentifiers.h>
#include <Workphone/Core/Handle.hpp>

using namespace RakNet;

namespace workphone
{
    UDPProxyCoordinatorFramework::UDPProxyCoordinatorFramework()
    {
        m_isSupported = SUPPORTED;
        m_udppc = nullptr;
        m_password = "Firebird";
    }

    const String UDPProxyCoordinatorFramework::QueryName( void )
    {
        return "UDPProxyCoordinator";
    }

    const String UDPProxyCoordinatorFramework::QueryRequirements( void )
    {
        return "Bandwidth to handle a few hundred bytes per game session";
    }

    const String UDPProxyCoordinatorFramework::QueryFunction( void )
    {
        return "Coordinates UDPProxyClient to find available UDPProxyServer.\nExactly one instance "
               "required.";
    }

    void UDPProxyCoordinatorFramework::Init( RakPeerInterface *rakPeer )
    {
        if( m_isSupported == SUPPORTED )
        {
            m_udppc = new UDPProxyCoordinator;
            rakPeer->AttachPlugin( m_udppc );
            m_udppc->SetRemoteLoginPassword( RakString( m_password.c_str() ) );
        }
    }

    void UDPProxyCoordinatorFramework::ProcessPacket( RakPeerInterface *rakPeer, Packet *packet )
    {
    }

    void UDPProxyCoordinatorFramework::Shutdown( RakPeerInterface *rakPeer )
    {
        if( m_udppc )
        {
            rakPeer->DetachPlugin( m_udppc );
            delete m_udppc;
            m_udppc = nullptr;
        }
    }
}  // namespace workphone
