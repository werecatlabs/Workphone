#ifndef _NatTypeDetectionFramework_H
#define _NatTypeDetectionFramework_H

#define _WINSOCKAPI_  // stops windows.h including winsock.h

#include "WPRakNet/NATSampleFramework.hpp"
#include "NatTypeDetectionClient.h"
#include "GetTime.h"

namespace workphone
{
    class NatTypeDetectionFramework : public NATSampleFramework
    {
    public:
        NatTypeDetectionFramework();

        void SetPrintMessage( IPrintMessage *pm );
        bool HasRooter();
        SampleResult GetSampleResult() override;

        const String QueryName( void ) override;
        bool QueryRequiresServer( void ) override;
        const String QueryFunction( void ) override;
        const String QuerySuccess( void ) override;
        bool QueryQuitOnSuccess( void ) override;
        void Init( RakNet::RakPeerInterface *rakPeer ) override;
        void ProcessPacket( RakNet::Packet *packet ) override;
        void Update( RakNet::RakPeerInterface *rakPeer ) override;
        void Shutdown( RakNet::RakPeerInterface *rakPeer ) override;

        RakNet::SystemAddress GetServerAddress();
        void SetServerAddress( RakNet::SystemAddress value );

    protected:
        RakNet::NatTypeDetectionClient *m_ntdc;
        RakNet::TimeMS m_timeout;
        RakNet::SystemAddress m_serverAddress;
        bool m_hasRooter;
    };
}  // namespace workphone

#endif  // _NatTypeDetectionFramework_H
