#include "WPRakNet/NatTypeDetectionFramework.hpp"
#include <BitStream.h>
#include <MessageIdentifiers.h>
#include <Workphone/Core/Handle.hpp>

using namespace RakNet;

namespace workphone
{
    NatTypeDetectionFramework::NatTypeDetectionFramework()
    {
        m_sampleResult = SUPPORT_NAT_TYPE_DETECTION;
        m_ntdc = nullptr;
        m_serverAddress = UNASSIGNED_SYSTEM_ADDRESS;
        m_hasRooter = true;
    }

    void NatTypeDetectionFramework::SetPrintMessage( IPrintMessage *pm )
    {
        m_pm = pm;
    }

    bool NatTypeDetectionFramework::HasRooter()
    {
        return m_hasRooter;
    }

    SampleResult NatTypeDetectionFramework::GetSampleResult()
    {
        return m_sampleResult;
    }

    const String NatTypeDetectionFramework::QueryName( void )
    {
        return "NatTypeDetectionFramework";
    }

    bool NatTypeDetectionFramework::QueryRequiresServer( void )
    {
        return true;
    }

    const String NatTypeDetectionFramework::QueryFunction( void )
    {
        return "Determines router type to avoid NAT punch attempts that cannot succeed.";
    }

    const String NatTypeDetectionFramework::QuerySuccess( void )
    {
        return "If our NAT type is Symmetric, we can skip NAT punch to other symmetric NATs.";
    }

    bool NatTypeDetectionFramework::QueryQuitOnSuccess( void )
    {
        return false;
    }

    void NatTypeDetectionFramework::Init( RakPeerInterface *rakPeer )
    {
        if( m_sampleResult == FAILED )
            return;

        // SystemAddress serverAddress = SelectAmongConnectedSystems(rakPeer, "NatPunchthroughServer");
        if( m_serverAddress == UNASSIGNED_SYSTEM_ADDRESS )
        {
            return;
        }

        m_ntdc = new NatTypeDetectionClient;
        rakPeer->AttachPlugin( m_ntdc );
        m_ntdc->DetectNATType( m_serverAddress );
        m_timeout = GetTimeMS() + 5000;
    }

    void NatTypeDetectionFramework::ProcessPacket( Packet *packet )
    {
        if( packet->data[0] == ID_NAT_TYPE_DETECTION_RESULT )
        {
            auto r = static_cast<RakNet::NATTypeDetectionResult>( packet->data[1] );

            char text[1024] = "";
            // sprintf_s(text, "NAT Type is %s (%s)", NATTypeDetectionResultToString(r),
            // NATTypeDetectionResultToStringFriendly(r));
            m_pm->PrintMessage( text );

            m_pm->PrintMessage( "Using NATPunchthrough can connect to systems using:" );

            for( int i = 0; i < static_cast<int>( RakNet::NAT_TYPE_COUNT ); i++ )
            {
                if( CanConnect( r, static_cast<RakNet::NATTypeDetectionResult>( i ) ) )
                {
                    if( i != 0 )
                        m_pm->PrintMessage( ", " );

                    char text[1024] = "";
                    // sprintf_s(text, "%s",
                    // NATTypeDetectionResultToString((RakNet::NATTypeDetectionResult)i));
                    m_pm->PrintMessage( text );
                }
            }
            // m_pm->PrintMessage("\n");
            if( r == NAT_TYPE_PORT_RESTRICTED || r == NAT_TYPE_SYMMETRIC )
            {
                // For UPNP, see Samples\UDPProxy
                m_pm->PrintMessage(
                    "Note: Your router must support UPNP or have the user manually forward ports." );
                m_pm->PrintMessage( "Otherwise not all connections may complete." );
            }

            if( r == NAT_TYPE_NONE )
            {
                m_hasRooter = false;
            }

            m_sampleResult = SUCCEEDED;
        }
    }

    void NatTypeDetectionFramework::Update( RakPeerInterface *rakPeer )
    {
        if( m_sampleResult == FAILED )
            return;

        if( m_sampleResult == PENDING && GetTimeMS() > m_timeout )
        {
            m_pm->PrintMessage(
                "No response from the server, probably not running NatTypeDetectionServer plugin." );
            m_sampleResult = FAILED;
        }
    }

    void NatTypeDetectionFramework::Shutdown( RakPeerInterface *rakPeer )
    {
        delete m_ntdc;
        m_ntdc = nullptr;
    }

    SystemAddress NatTypeDetectionFramework::GetServerAddress()
    {
        return m_serverAddress;
    }

    void NatTypeDetectionFramework::SetServerAddress( SystemAddress value )
    {
        m_serverAddress = value;
    }
}  // namespace workphone
