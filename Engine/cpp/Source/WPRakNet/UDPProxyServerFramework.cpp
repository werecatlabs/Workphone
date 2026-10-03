#include "WPRakNet/UDPProxyServerFramework.hpp"
#include <BitStream.h>
#include <MessageIdentifiers.h>
#include <Workphone/Core/Handle.hpp>

using namespace RakNet;

namespace workphone
{
    UDPProxyServerFramework::UDPProxyServerFramework()
    {
        m_isSupported = SUPPORTED;
        m_udpps = nullptr;
        m_pm = nullptr;
        m_password = "Firebird";
    }

    void UDPProxyServerFramework::SetPrintMessage( IPrintMessage *pm )
    {
        m_pm = pm;
    }

    const String UDPProxyServerFramework::QueryName( void )
    {
        return "UDPProxyServer";
    }

    const String UDPProxyServerFramework::QueryRequirements( void )
    {
        return "Bandwidth to handle forwarded game traffic.";
    }

    const String UDPProxyServerFramework::QueryFunction( void )
    {
        return "Allows game clients to forward network traffic transparently.\nOne or more instances "
               "required, can be added at runtime.";
    }

    void UDPProxyServerFramework::Init( RakPeerInterface *rakPeer )
    {
        if( m_isSupported == SUPPORTED )
        {
            /*if (m_coordinatorAddress == UNASSIGNED_SYSTEM_ADDRESS)
            {
            return;
            }*/

            m_udpps = new UDPProxyServer;
            m_udpps->SetResultHandler( this );
            rakPeer->AttachPlugin( m_udpps );
            if( m_udpps->LoginToCoordinator(
                    m_password.c_str(), rakPeer->GetInternalID( UNASSIGNED_SYSTEM_ADDRESS ) ) == false )
            {
                if( m_pm )
                    m_pm->PrintMessage( "LoginToCoordinator call failed.\n" );
                m_isSupported = UNSUPPORTED;
                rakPeer->DetachPlugin( m_udpps );
                delete m_udpps;
                m_udpps = nullptr;
            }
        }
    }

    void UDPProxyServerFramework::ProcessPacket( RakPeerInterface *rakPeer, Packet *packet )
    {
    }

    void UDPProxyServerFramework::Shutdown( RakPeerInterface *rakPeer )
    {
        if( m_udpps )
        {
            rakPeer->DetachPlugin( m_udpps );
            delete m_udpps;
            m_udpps = nullptr;
        }
    }

    void UDPProxyServerFramework::OnLoginSuccess( RakString usedPassword,
                                                  UDPProxyServer *proxyServerPlugin )
    {
        // printf("%s logged into UDPProxyCoordinator.\n", QueryName());
        char text[256] = "";
        // sprintf_s(text, "%s logged into UDPProxyCoordinator.", QueryName().c_str());
        if( m_pm )
        {
            m_pm->PrintMessage( text );
        }
    }

    void UDPProxyServerFramework::OnAlreadyLoggedIn( RakString usedPassword,
                                                     UDPProxyServer *proxyServerPlugin )
    {
        // printf("%s already logged into UDPProxyCoordinator.\n", QueryName());
        char text[256] = "";
        // sprintf_s(text, "%s already logged into UDPProxyCoordinator.", QueryName().c_str());
        if( m_pm )
            m_pm->PrintMessage( text );
    }

    void UDPProxyServerFramework::OnNoPasswordSet( RakString usedPassword,
                                                   UDPProxyServer *proxyServerPlugin )
    {
        // printf("%s failed login to UDPProxyCoordinator. No password set.\n", QueryName());
        char text[256] = "";
        // sprintf_s(text, "%s failed login to UDPProxyCoordinator. No password set.",
        // QueryName().c_str());
        if( m_pm )
            m_pm->PrintMessage( text );

        m_isSupported = QUERY;
        delete m_udpps;
        m_udpps = nullptr;
    }

    void UDPProxyServerFramework::OnWrongPassword( RakString usedPassword,
                                                   UDPProxyServer *proxyServerPlugin )
    {
        // printf("%s failed login to UDPProxyCoordinator. %s was the wrong password.\n", QueryName(),
        // usedPassword.C_String());
        char text[256] = "";
        // sprintf_s(text, "%s failed login to UDPProxyCoordinator. %s was the wrong password.",
        // QueryName().c_str(), usedPassword.C_String());
        if( m_pm )
            m_pm->PrintMessage( text );

        m_isSupported = QUERY;
        delete m_udpps;
        m_udpps = nullptr;
    }
}  // namespace workphone
