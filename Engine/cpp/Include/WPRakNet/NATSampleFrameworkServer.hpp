#ifndef _NATSampleFrameworkServer_H
#define _NATSampleFrameworkServer_H

#define _WINSOCKAPI_  // stops windows.h including winsock.h

#include <Workphone/Core/StringTypes.hpp>
#include <RakPeerInterface.h>

namespace workphone
{
#define RAKPEER_PORT 61111

#define NatTypeDetectionServerFramework_Supported QUERY
#define NatPunchthroughServerFramework_Supported QUERY
#define UDPProxyCoordinatorFramework_Supported QUERY
#define UDPProxyServerFramework_Supported QUERY
#define CloudServerFramework_Supported QUERY

    enum FeatureSupport
    {
        SUPPORTED,
        UNSUPPORTED,
        QUERY
    };

    class NATSampleFrameworkServer
    {
    public:
        virtual const String QueryName( void ) = 0;
        virtual const String QueryRequirements( void ) = 0;
        virtual const String QueryFunction( void ) = 0;
        virtual void Init( RakNet::RakPeerInterface *rakPeer ) = 0;
        virtual void ProcessPacket( RakNet::RakPeerInterface *rakPeer, RakNet::Packet *packet ) = 0;
        virtual void Shutdown( RakNet::RakPeerInterface *rakPeer ) = 0;

    protected:
        FeatureSupport m_isSupported;
    };
}  // namespace workphone

#endif  // _NATSampleFrameworkServer_H
