#ifndef _NatPunchthoughServerFramework_H
#define _NatPunchthoughServerFramework_H

#define _WINSOCKAPI_  // stops windows.h including winsock.h

#include "WPRakNet/NATSampleFrameworkServer.hpp"
#include "NatPunchthroughServer.h"
#include "GetTime.h"

namespace workphone
{
    class NatPunchthroughServerFramework : public NATSampleFrameworkServer,
                                           public RakNet::NatPunchthroughServerDebugInterface_Printf
    {
    public:
        NatPunchthroughServerFramework();

        const String QueryName( void ) override;
        const String QueryRequirements( void ) override;
        const String QueryFunction( void ) override;
        void Init( RakNet::RakPeerInterface *rakPeer ) override;
        void ProcessPacket( RakNet::RakPeerInterface *rakPeer, RakNet::Packet *packet ) override;
        void Shutdown( RakNet::RakPeerInterface *rakPeer ) override;

    protected:
        RakNet::NatPunchthroughServer *m_nps;
    };
}  // namespace workphone

#endif  // _NatPunchthoughServerFramework_H
