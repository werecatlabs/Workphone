#ifndef _UDPProxyCoordinatorFramework_H
#define _UDPProxyCoordinatorFramework_H

#define _WINSOCKAPI_  // stops windows.h including winsock.h

#include "NATSampleFrameworkServer.hpp"
#include "UDPProxyCoordinator.h"

namespace workphone
{
    struct UDPProxyCoordinatorFramework : public NATSampleFrameworkServer
    {
    public:
        UDPProxyCoordinatorFramework();

        const String QueryName( void ) override;
        const String QueryRequirements( void ) override;
        const String QueryFunction( void ) override;
        void Init( RakNet::RakPeerInterface *rakPeer ) override;
        void ProcessPacket( RakNet::RakPeerInterface *rakPeer, RakNet::Packet *packet ) override;
        void Shutdown( RakNet::RakPeerInterface *rakPeer ) override;

    protected:
        RakNet::UDPProxyCoordinator *m_udppc;
        String m_password;
    };
}  // namespace workphone

#endif  // _UDPProxyCoordinatorFramework_H
