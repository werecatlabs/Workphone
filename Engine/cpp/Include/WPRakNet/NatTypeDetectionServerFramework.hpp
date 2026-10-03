#ifndef _NatTypeDetectionServerFramework_H
#define _NatTypeDetectionServerFramework_H

#define _WINSOCKAPI_  // stops windows.h including winsock.h

#include "NATSampleFrameworkServer.hpp"
#include "NatTypeDetectionServer.h"
#include "GetTime.h"

#include "IPrintMessage.hpp"

namespace workphone
{
    class NatTypeDetectionServerFramework : public NATSampleFrameworkServer
    {
    public:
        NatTypeDetectionServerFramework();

        void SetPrintMessage( IPrintMessage *pm );

        const String QueryName( void ) override;
        const String QueryRequirements( void ) override;
        const String QueryFunction( void ) override;
        void Init( RakNet::RakPeerInterface *rakPeer ) override;
        void ProcessPacket( RakNet::RakPeerInterface *rakPeer, RakNet::Packet *packet ) override;
        void Shutdown( RakNet::RakPeerInterface *rakPeer ) override;

    protected:
        RakNet::NatTypeDetectionServer *m_ntds;
        IPrintMessage *m_pm;
    };
}  // namespace workphone

#endif  // _NatTypeDetectionServerFramework_H
