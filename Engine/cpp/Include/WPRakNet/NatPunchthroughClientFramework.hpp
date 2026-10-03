#ifndef _NatPunchthoughClientFramework_H
#define _NatPunchthoughClientFramework_H

#define _WINSOCKAPI_  // stops windows.h including winsock.h

#include "NATSampleFramework.hpp"
#include "NatPunchthroughClient.h"
#include "GetTime.h"

namespace workphone
{
    class NatPunchthroughClientFramework : public NATSampleFramework,
                                           public RakNet::NatPunchthroughDebugInterface_Printf
    {
    public:
        NatPunchthroughClientFramework();

        void SetPrintMessage( IPrintMessage *pm );

        SampleResult GetSampleResult() override;

        void SetSampleResult( SampleResult sampleResult );

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

        RakNet::SystemAddress GetOtherClientAddress();

        void SetOtherClientAddress( RakNet::SystemAddress value );

        String GetPartenerGuid();

        void SetPartenerGuid( String value );

        bool GetIsListening();

        void SetIsListening( bool value );

    protected:
        RakNet::NatPunchthroughClient *m_npClient;
        RakNet::TimeMS m_timeout;
        bool m_isListening;
        RakNet::SystemAddress m_serverAddress;
        String m_partenerGuid;
        RakNet::SystemAddress m_otherClientAddress;
    };
}  // namespace workphone

#endif  // _NatPunchthoughClientFramework_H
