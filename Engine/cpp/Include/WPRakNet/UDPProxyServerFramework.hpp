#ifndef _UDPProxyServerFramework_H
#define _UDPProxyServerFramework_H

#define _WINSOCKAPI_  // stops windows.h including winsock.h

#include "NATSampleFrameworkServer.hpp"
#include "UDPProxyServer.h"
#include "GetTime.h"

#include "IPrintMessage.hpp"

namespace workphone
{
    struct UDPProxyServerFramework : public NATSampleFrameworkServer,
                                     public RakNet::UDPProxyServerResultHandler
    {
    public:
        UDPProxyServerFramework();

        void SetPrintMessage( IPrintMessage *pm );

        const String QueryName( void ) override;
        const String QueryRequirements( void ) override;
        const String QueryFunction( void ) override;
        void Init( RakNet::RakPeerInterface *rakPeer ) override;
        void ProcessPacket( RakNet::RakPeerInterface *rakPeer, RakNet::Packet *packet ) override;
        void Shutdown( RakNet::RakPeerInterface *rakPeer ) override;
        void OnLoginSuccess( RakNet::RakString usedPassword,
                             RakNet::UDPProxyServer *proxyServerPlugin ) override;
        void OnAlreadyLoggedIn( RakNet::RakString usedPassword,
                                RakNet::UDPProxyServer *proxyServerPlugin ) override;
        void OnNoPasswordSet( RakNet::RakString usedPassword,
                              RakNet::UDPProxyServer *proxyServerPlugin ) override;
        void OnWrongPassword( RakNet::RakString usedPassword,
                              RakNet::UDPProxyServer *proxyServerPlugin ) override;

    protected:
        RakNet::UDPProxyServer *m_udpps;
        RakNet::SystemAddress m_coordinatorAddress;
        String m_password;
        IPrintMessage *m_pm;
    };
}  // namespace workphone

#endif  // _UDPProxyServerFramework_H
