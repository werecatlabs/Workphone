#include "WPRakNet/NatTypeDetectionServerFramework.hpp"
#include <BitStream.h>
#include <MessageIdentifiers.h>
#include <SocketLayer.h>
#include <Workphone/Core/Handle.hpp>

using namespace RakNet;

namespace workphone
{
    NatTypeDetectionServerFramework::NatTypeDetectionServerFramework()
    {
        m_isSupported = SUPPORTED;
        m_ntds = nullptr;
        m_pm = nullptr;
    }

    void NatTypeDetectionServerFramework::SetPrintMessage( IPrintMessage *pm )
    {
        m_pm = pm;
    }

    const String NatTypeDetectionServerFramework::QueryName( void )
    {
        return "NatTypeDetectionServer";
    }

    const String NatTypeDetectionServerFramework::QueryRequirements( void )
    {
        return "Requires 4 IP addresses.";
    }

    const String NatTypeDetectionServerFramework::QueryFunction( void )
    {
        return "Determines router type to filter by connectable systems.\nOne instance needed, multiple "
               "instances may exist to spread workload.";
    }

    void NatTypeDetectionServerFramework::Init( RakPeerInterface *rakPeer )
    {
        if( m_isSupported == SUPPORTED )
        {
            m_ntds = new NatTypeDetectionServer;
            rakPeer->AttachPlugin( m_ntds );

            SystemAddress ipList[MAXIMUM_NUMBER_OF_INTERNAL_IDS];
            SocketLayer::GetMyIP( ipList );
            for( int i = 0; i < 4; i++ )
            {
                if( ipList[i] == UNASSIGNED_SYSTEM_ADDRESS && i < MAXIMUM_NUMBER_OF_INTERNAL_IDS )
                {
                    if( m_pm )
                        m_pm->PrintMessage( "Failed. Not enough IP addresses to bind to." );
                    rakPeer->DetachPlugin( m_ntds );
                    delete m_ntds;
                    m_ntds = nullptr;
                    m_isSupported = UNSUPPORTED;
                    return;
                }
            }
            char ipListStr1[128], ipListStr2[128], ipListStr3[128];
            ipList[1].ToString( false, ipListStr1 );
            ipList[2].ToString( false, ipListStr2 );
            ipList[3].ToString( false, ipListStr3 );
            char text[1024] = "";
            // sprintf_s(text, "Starting %s on %s, %s, %s.", QueryName(), ipListStr1, ipListStr2,
            // ipListStr3);
            if( m_pm )
                m_pm->PrintMessage( text );
            m_ntds->Startup( ipListStr1, ipListStr2, ipListStr3 );
        }
    }

    void NatTypeDetectionServerFramework::ProcessPacket( RakPeerInterface *rakPeer, Packet *packet )
    {
    }

    void NatTypeDetectionServerFramework::Shutdown( RakPeerInterface *rakPeer )
    {
        if( m_ntds )
        {
            rakPeer->DetachPlugin( m_ntds );
            delete m_ntds;
            m_ntds = nullptr;
        }
    }
}  // namespace workphone
