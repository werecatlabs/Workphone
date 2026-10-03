#include "WPRakNet/NatPunchthroughServerFramework.hpp"
#include <BitStream.h>
#include <MessageIdentifiers.h>
#include <Workphone/Core/Handle.hpp>

using namespace RakNet;

namespace workphone
{
    NatPunchthroughServerFramework::NatPunchthroughServerFramework()
    {
        m_isSupported = SUPPORTED;
        m_nps = nullptr;
    }

    const String NatPunchthroughServerFramework::QueryName( void )
    {
        return "NatPunchthroughServerFramework";
    }

    const String NatPunchthroughServerFramework::QueryRequirements( void )
    {
        return "None";
    }

    const String NatPunchthroughServerFramework::QueryFunction( void )
    {
        return "Coordinates NATPunchthroughClient.";
    }

    void NatPunchthroughServerFramework::Init( RakPeerInterface *rakPeer )
    {
        if( m_isSupported == SUPPORTED )
        {
            m_nps = new NatPunchthroughServer;
            rakPeer->AttachPlugin( m_nps );
            m_nps->SetDebugInterface( this );
        }
    }

    void NatPunchthroughServerFramework::ProcessPacket( RakPeerInterface *rakPeer, Packet *packet )
    {
    }

    void NatPunchthroughServerFramework::Shutdown( RakPeerInterface *rakPeer )
    {
        if( m_nps )
        {
            rakPeer->DetachPlugin( m_nps );
            delete m_nps;
        }
    }
}  // namespace workphone
