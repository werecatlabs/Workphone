#include "WPRakNet/UDPProxyClientFramework.hpp"
#include <BitStream.h>
#include <MessageIdentifiers.h>
#include <UDPProxyCommon.h>
#include <Workphone/Core/Handle.hpp>

using namespace RakNet;

namespace workphone
{
    UDPProxyClientFramework::UDPProxyClientFramework()
    {
        m_sampleResult = SUPPORT_UDP_PROXY;
        m_udpProxy = nullptr;
        m_serverAddress = UNASSIGNED_SYSTEM_ADDRESS;
    }

    void UDPProxyClientFramework::SetPrintMessage( IPrintMessage *pm )
    {
        m_pm = pm;
    }

    workphone::String UDPProxyClientFramework::GetOtherClientAddress()
    {
        return m_otherClientAddress;
    }

    unsigned short UDPProxyClientFramework::GetOtherClientPort()
    {
        return m_otherClientPort;
    }

    workphone::SampleResult UDPProxyClientFramework::GetSampleResult()
    {
        return m_sampleResult;
    }

    const String UDPProxyClientFramework::QueryName( void )
    {
        return "UDPProxyClientFramework";
    }

    bool UDPProxyClientFramework::QueryRequiresServer( void )
    {
        return true;
    }

    const String UDPProxyClientFramework::QueryFunction( void )
    {
        return "Connect to a peer using a shared server connection.";
    }

    const String UDPProxyClientFramework::QuerySuccess( void )
    {
        return "We can now communicate with the other system, including connecting, within 5 seconds.";
    }

    bool UDPProxyClientFramework::QueryQuitOnSuccess( void )
    {
        return false;
    }

    void UDPProxyClientFramework::Init( RakPeerInterface *rakPeer )
    {
        if( m_sampleResult == FAILED )
            return;

        // SystemAddress serverAddress = SelectAmongConnectedSystems(rakPeer, "NatPunchthroughServer");
        if( m_serverAddress == UNASSIGNED_SYSTEM_ADDRESS )
        {
            return;
        }

        m_udpProxy = new UDPProxyClient;
        rakPeer->AttachPlugin( m_udpProxy );
        m_udpProxy->SetResultHandler( this );

        if( !m_isListening )
        {
            RakNetGUID remoteSystemGuid;
            remoteSystemGuid.FromString( m_partenerGuid.c_str() );
            bool res = m_udpProxy->RequestForwarding( m_serverAddress, UNASSIGNED_SYSTEM_ADDRESS,
                                                      remoteSystemGuid, UDP_FORWARDER_MAXIMUM_TIMEOUT,
                                                      nullptr );
        }

        m_timeout = GetTimeMS() + 5000;
        /*else
        {
        m_pm->PrintMessage("Listening\n");
        m_pm->PrintMessage("My GUID is %s\n", rakPeer->GetMyGUID().ToString());
        isListening=true;
        }*/
    }

    void UDPProxyClientFramework::ProcessPacket( Packet *packet )
    {
    }

    void UDPProxyClientFramework::Update( RakPeerInterface *rakPeer )
    {
        if( m_sampleResult == FAILED )
            return;

        if( m_sampleResult == PENDING && GetTimeMS() > m_timeout && m_isListening == false )
        {
            m_pm->PrintMessage(
                "No response from the server, probably not running UDPProxyCoordinator plugin." );
            m_sampleResult = FAILED;
        }
    }

    void UDPProxyClientFramework::Shutdown( RakPeerInterface *rakPeer )
    {
        delete m_udpProxy;
        m_udpProxy = nullptr;
    }

    void UDPProxyClientFramework::OnForwardingSuccess(
        const char *proxyIPAddress, unsigned short proxyPort, SystemAddress proxyCoordinator,
        SystemAddress sourceAddress, SystemAddress targetAddress, UDPProxyClient *proxyClientPlugin )
    {
        char text[1024] = "";
        // sprintf_s(text, "Datagrams forwarded by proxy %s:%i to target %s.", proxyIPAddress, proxyPort,
        // targetAddress.ToString(false));
        m_pm->PrintMessage( text );
        // m_pm->PrintMessage("\nConnecting to proxy, which will be received by target.");
        /*ConnectionAttemptResult car = proxyClientPlugin->GetRakPeerInterface()->Connect(proxyIPAddress,
        proxyPort, 0, 0); RakAssert(car==CONNECTION_ATTEMPT_STARTED);*/
        m_otherClientAddress = proxyIPAddress;
        m_otherClientPort = proxyPort;
        m_sampleResult = SUCCEEDED;
    }

    void UDPProxyClientFramework::OnForwardingNotification(
        const char *proxyIPAddress, unsigned short proxyPort, SystemAddress proxyCoordinator,
        SystemAddress sourceAddress, SystemAddress targetAddress, UDPProxyClient *proxyClientPlugin )
    {
        char text[1024] = "";
        // sprintf_s(text, "Source %s has setup forwarding to us through proxy %s:%i.",
        // sourceAddress.ToString(false), proxyIPAddress, proxyPort);
        m_pm->PrintMessage( text );

        m_otherClientAddress = proxyIPAddress;
        m_otherClientPort = proxyPort;
        m_sampleResult = SUCCEEDED;
    }

    void UDPProxyClientFramework::OnNoServersOnline( SystemAddress proxyCoordinator,
                                                     SystemAddress sourceAddress,
                                                     SystemAddress targetAddress,
                                                     UDPProxyClient *proxyClientPlugin )
    {
        m_pm->PrintMessage( "Failure: No servers logged into coordinator." );
        m_sampleResult = FAILED;
    }

    void UDPProxyClientFramework::OnRecipientNotConnected( SystemAddress proxyCoordinator,
                                                           SystemAddress sourceAddress,
                                                           SystemAddress targetAddress,
                                                           RakNetGUID targetGuid,
                                                           UDPProxyClient *proxyClientPlugin )
    {
        m_pm->PrintMessage( "Failure: Recipient not connected to coordinator." );
        m_sampleResult = FAILED;
    }

    void UDPProxyClientFramework::OnAllServersBusy( SystemAddress proxyCoordinator,
                                                    SystemAddress sourceAddress,
                                                    SystemAddress targetAddress,
                                                    UDPProxyClient *proxyClientPlugin )
    {
        m_pm->PrintMessage( "Failure: No servers have available forwarding ports." );
        m_sampleResult = FAILED;
    }

    void UDPProxyClientFramework::OnForwardingInProgress( SystemAddress proxyCoordinator,
                                                          SystemAddress sourceAddress,
                                                          SystemAddress targetAddress,
                                                          UDPProxyClient *proxyClientPlugin )
    {
        m_pm->PrintMessage( "Notification: Forwarding already in progress." );
    }

    RakNet::SystemAddress UDPProxyClientFramework::GetServerAddress()
    {
        return m_serverAddress;
    }

    void UDPProxyClientFramework::SetServerAddress( RakNet::SystemAddress value )
    {
        m_serverAddress = value;
    }

    workphone::String UDPProxyClientFramework::GetPartenerGuid()
    {
        return m_partenerGuid;
    }

    void UDPProxyClientFramework::SetPartenerGuid( String value )
    {
        m_partenerGuid = value;
    }

    bool UDPProxyClientFramework::GetIsListening()
    {
        return m_isListening;
    }

    void UDPProxyClientFramework::SetIsListening( bool value )
    {
        m_isListening = value;
    }

}  // namespace workphone
