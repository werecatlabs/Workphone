#ifndef _UDPProxyClientFramework_H
#define _UDPProxyClientFramework_H

#define _WINSOCKAPI_  // stops windows.h including winsock.h

#include "NATSampleFramework.hpp"
#include "UDPProxyClient.h"
#include "GetTime.h"

namespace workphone
{
    class UDPProxyClientFramework : public NATSampleFramework, public RakNet::UDPProxyClientResultHandler
    {
    public:
        UDPProxyClientFramework();

        void SetPrintMessage( IPrintMessage *pm );

        String GetOtherClientAddress();

        unsigned short GetOtherClientPort();

        SampleResult GetSampleResult();

        const String QueryName( void ) override;
        bool QueryRequiresServer( void ) override;
        const String QueryFunction( void ) override;
        const String QuerySuccess( void ) override;
        bool QueryQuitOnSuccess( void ) override;
        void Init( RakNet::RakPeerInterface *rakPeer ) override;
        void ProcessPacket( RakNet::Packet *packet ) override;
        void Update( RakNet::RakPeerInterface *rakPeer ) override;
        void Shutdown( RakNet::RakPeerInterface *rakPeer ) override;
        void OnForwardingSuccess( const char *proxyIPAddress, unsigned short proxyPort,
                                  RakNet::SystemAddress proxyCoordinator,
                                  RakNet::SystemAddress sourceAddress,
                                  RakNet::SystemAddress targetAddress,
                                  RakNet::UDPProxyClient *proxyClientPlugin );
        void OnForwardingNotification( const char *proxyIPAddress, unsigned short proxyPort,
                                       RakNet::SystemAddress proxyCoordinator,
                                       RakNet::SystemAddress sourceAddress,
                                       RakNet::SystemAddress targetAddress,
                                       RakNet::UDPProxyClient *proxyClientPlugin );
        void OnNoServersOnline( RakNet::SystemAddress proxyCoordinator,
                                RakNet::SystemAddress sourceAddress, RakNet::SystemAddress targetAddress,
                                RakNet::UDPProxyClient *proxyClientPlugin );
        void OnRecipientNotConnected( RakNet::SystemAddress proxyCoordinator,
                                      RakNet::SystemAddress sourceAddress,
                                      RakNet::SystemAddress targetAddress, RakNet::RakNetGUID targetGuid,
                                      RakNet::UDPProxyClient *proxyClientPlugin ) override;
        void OnAllServersBusy( RakNet::SystemAddress proxyCoordinator,
                               RakNet::SystemAddress sourceAddress, RakNet::SystemAddress targetAddress,
                               RakNet::UDPProxyClient *proxyClientPlugin );
        void OnForwardingInProgress( RakNet::SystemAddress proxyCoordinator,
                                     RakNet::SystemAddress sourceAddress,
                                     RakNet::SystemAddress targetAddress,
                                     RakNet::UDPProxyClient *proxyClientPlugin );

        RakNet::SystemAddress GetServerAddress();

        void SetServerAddress( RakNet::SystemAddress value );

        String GetPartenerGuid();

        void SetPartenerGuid( String value );

        bool GetIsListening();

        void SetIsListening( bool value );

    protected:
        RakNet::UDPProxyClient *m_udpProxy;
        RakNet::TimeMS m_timeout;
        bool m_isListening;
        RakNet::SystemAddress m_serverAddress;
        String m_partenerGuid;
        String m_otherClientAddress;
        unsigned short m_otherClientPort;
    };
}  // namespace workphone

#endif  // _UDPProxyClientFramework_H
