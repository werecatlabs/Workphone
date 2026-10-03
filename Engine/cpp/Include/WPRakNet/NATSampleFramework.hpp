#ifndef _NATSampleFramework_H
#define _NATSampleFramework_H

#include <Workphone/Core/StringTypes.hpp>
#include <RakPeerInterface.h>
#include "IPrintMessage.hpp"

namespace workphone
{
#define SUPPORT_UPNP PENDING
#define SUPPORT_NAT_TYPE_DETECTION PENDING
#define SUPPORT_NAT_PUNCHTHROUGH PENDING
#define SUPPORT_ROUTER2 PENDING
#define SUPPORT_UDP_PROXY PENDING

#define RAKPEER_PORT 0
#define RAKPEER_PORT_STR "0"
#define DEFAULT_SERVER_PORT "61111"
#define DEFAULT_SERVER_ADDRESS "94.198.81.195"

    enum SampleResult
    {
        PENDING,
        FAILED,
        SUCCEEDED
    };

    class NATSampleFramework
    {
    public:
        virtual const String QueryName( void ) = 0;
        virtual bool QueryRequiresServer( void ) = 0;
        virtual const String QueryFunction( void ) = 0;
        virtual const String QuerySuccess( void ) = 0;
        virtual bool QueryQuitOnSuccess( void ) = 0;
        virtual void Init( RakNet::RakPeerInterface *rakPeer ) = 0;
        virtual void ProcessPacket( RakNet::Packet *packet ) = 0;
        virtual void Update( RakNet::RakPeerInterface *rakPeer ) = 0;
        virtual void Shutdown( RakNet::RakPeerInterface *rakPeer ) = 0;
        virtual SampleResult GetSampleResult() = 0;

    protected:
        SampleResult m_sampleResult;
        IPrintMessage *m_pm;
    };
}  // namespace workphone

#endif  // _NATSampleFramework_H
