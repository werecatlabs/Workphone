#include "WPRakNet/NatPunchthroughClientFramework.hpp"
#include <BitStream.h>
#include <MessageIdentifiers.h>
#include <Workphone/Core/Handle.hpp>

using namespace RakNet;

namespace workphone
{
    NatPunchthroughClientFramework::NatPunchthroughClientFramework()
    {
        m_sampleResult = SUPPORT_NAT_PUNCHTHROUGH;
        m_npClient = nullptr;
        m_serverAddress = UNASSIGNED_SYSTEM_ADDRESS;
    }

    void NatPunchthroughClientFramework::SetPrintMessage( IPrintMessage *pm )
    {
        m_pm = pm;
    }

    workphone::SampleResult NatPunchthroughClientFramework::GetSampleResult()
    {
        return m_sampleResult;
    }

    void NatPunchthroughClientFramework::SetSampleResult( SampleResult sampleResult )
    {
        m_sampleResult = sampleResult;
    }

    const String NatPunchthroughClientFramework::QueryName( void )
    {
        return "NatPunchthoughClientFramework";
    }

    bool NatPunchthroughClientFramework::QueryRequiresServer( void )
    {
        return true;
    }

    const String NatPunchthroughClientFramework::QueryFunction( void )
    {
        return "Causes two systems to try to connect to each other at the same\ntime, to get through "
               "routers.";
    }

    const String NatPunchthroughClientFramework::QuerySuccess( void )
    {
        return "We can now communicate with the other system, including connecting.";
    }

    bool NatPunchthroughClientFramework::QueryQuitOnSuccess( void )
    {
        return true;
    }

    void NatPunchthroughClientFramework::Init( RakPeerInterface *rakPeer )
    {
        if( m_sampleResult == FAILED )
            return;

        // SystemAddress serverAddress = SelectAmongConnectedSystems(rakPeer, "NatPunchthroughServer");
        if( m_serverAddress == UNASSIGNED_SYSTEM_ADDRESS )
        {
            return;
        }

        m_npClient = new NatPunchthroughClient;
        m_npClient->SetDebugInterface( this );
        rakPeer->AttachPlugin( m_npClient );

        if( !m_isListening )
        {
            RakNetGUID remoteSystemGuid;
            remoteSystemGuid.FromString( m_partenerGuid.c_str() );
            bool res = m_npClient->OpenNAT( remoteSystemGuid, m_serverAddress );

            m_timeout = GetTimeMS() + 10000;
        }
        /*else
        {
        m_pm->PrintMessage("Listening\n");
        m_pm->PrintMessage("My GUID is %s\n", rakPeer->GetMyGUID().ToString());
        isListening=true;
        }*/
    }

    void NatPunchthroughClientFramework::ProcessPacket( Packet *packet )
    {
        if( packet->data[0] == ID_NAT_TARGET_NOT_CONNECTED ||
            packet->data[0] == ID_NAT_TARGET_UNRESPONSIVE ||
            packet->data[0] == ID_NAT_CONNECTION_TO_TARGET_LOST ||
            packet->data[0] == ID_NAT_PUNCHTHROUGH_FAILED )
        {
            RakNetGUID guid;
            if( packet->data[0] == ID_NAT_PUNCHTHROUGH_FAILED )
            {
                guid = packet->guid;
            }
            else
            {
                BitStream bs( packet->data, packet->length, false );
                bs.IgnoreBytes( 1 );
                bool b = bs.Read( guid );
                RakAssert( b );
            }

            switch( packet->data[0] )
            {
            case ID_NAT_TARGET_NOT_CONNECTED:
                m_pm->PrintMessage( "Failed: ID_NAT_TARGET_NOT_CONNECTED" );
                break;
            case ID_NAT_TARGET_UNRESPONSIVE:
                m_pm->PrintMessage( "Failed: ID_NAT_TARGET_UNRESPONSIVE" );
                break;
            case ID_NAT_CONNECTION_TO_TARGET_LOST:
                m_pm->PrintMessage( "Failed: ID_NAT_CONNECTION_TO_TARGET_LOST" );
                break;
            case ID_NAT_PUNCHTHROUGH_FAILED:
                m_pm->PrintMessage( "Failed: ID_NAT_PUNCHTHROUGH_FAILED" );
                break;
            }

            m_sampleResult = FAILED;
            return;
        }
        if( packet->data[0] == ID_NAT_PUNCHTHROUGH_SUCCEEDED )
        {
            // here we can find the address of the other client (Packet::SystemAddress)
            m_otherClientAddress = packet->systemAddress;

            /*unsigned char weAreTheSender = packet->data[1];
            if (weAreTheSender)
                m_pm->PrintMessage("NAT punch success to remote system %s.\n",
            packet->systemAddress.ToString(true)); else
                m_pm->PrintMessage("NAT punch success from remote system %s.\n",
            packet->systemAddress.ToString(true));*/
            m_sampleResult = SUCCEEDED;
        }
    }

    void NatPunchthroughClientFramework::Update( RakPeerInterface *rakPeer )
    {
        if( m_sampleResult == FAILED )
            return;

        if( m_sampleResult == PENDING && GetTimeMS() > m_timeout && m_isListening == false )
        {
            // m_pm->PrintMessage("No response from the server, probably not running
            // NatPunchthroughServer plugin.\n");
            m_sampleResult = FAILED;
        }
    }

    void NatPunchthroughClientFramework::Shutdown( RakPeerInterface *rakPeer )
    {
        delete m_npClient;
        m_npClient = nullptr;
    }

    RakNet::SystemAddress NatPunchthroughClientFramework::GetServerAddress()
    {
        return m_serverAddress;
    }

    void NatPunchthroughClientFramework::SetServerAddress( RakNet::SystemAddress value )
    {
        m_serverAddress = value;
    }

    RakNet::SystemAddress NatPunchthroughClientFramework::GetOtherClientAddress()
    {
        return m_otherClientAddress;
    }

    void NatPunchthroughClientFramework::SetOtherClientAddress( RakNet::SystemAddress value )
    {
        m_otherClientAddress = value;
    }

    workphone::String NatPunchthroughClientFramework::GetPartenerGuid()
    {
        return m_partenerGuid;
    }

    void NatPunchthroughClientFramework::SetPartenerGuid( String value )
    {
        m_partenerGuid = value;
    }

    bool NatPunchthroughClientFramework::GetIsListening()
    {
        return m_isListening;
    }

    void NatPunchthroughClientFramework::SetIsListening( bool value )
    {
        m_isListening = value;
    }

}  // namespace workphone
