#include "ChatListener.hpp"
#include "FBRakNet/CPacket.hpp"
#include "FBRakNet/CNetworkManager.hpp"
#include "FBRakNet/FBRakNetMessageIdentifiers.hpp"

#if FB_BUILD_WXWIDGETS
using namespace fb;

void ChatListener::handlePacket( SmartPtr<IPacket> packet )
{
    String text;
    SmartPtr<CPacket> pachetI = packet;

    switch( pachetI->getPacket()->data[0] )
    {
    case ID_REMOTE_DISCONNECTION_NOTIFICATION:
        m_listChat->Append( "Another client has disconnected." );
        break;
    case ID_REMOTE_CONNECTION_LOST:
        m_listChat->Append( "Another client has lost the connection." );
        break;
    case ID_REMOTE_NEW_INCOMING_CONNECTION:
        m_listChat->Append( "Another client has connected." );
        break;
    case ID_CONNECTION_REQUEST_ACCEPTED:
    {
        m_listChat->Append( "Our connection request has been accepted." );

        // Use a BitStream to write a custom user message
        // Bitstreams are easier to use than sending casted structures, and handle endian swapping automatically
        /*RakNet::BitStream bsOut;
        bsOut.Write((RakNet::MessageID)ID_GAME_MESSAGE_1);
        bsOut.Write("Client connected!");
        peer->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);*/
    }
    break;
    case ID_NEW_INCOMING_CONNECTION:
        m_listChat->Append( "A connection is incoming." );
        break;
    case ID_NO_FREE_INCOMING_CONNECTIONS:
        m_listChat->Append( "The server is full." );
        break;
    case ID_DISCONNECTION_NOTIFICATION:
        /*if (isServer){
            m_listChat->Append("A client has disconnected.");
        } else*/
        {
            m_listChat->Append( "We have been disconnected." );
        }
        break;
    case ID_CONNECTION_LOST:
        /*if (isServer){
        m_listChat->Append("A client lost the connection.");
        } else */
        {
            m_listChat->Append( "Connection lost." );
        }
        break;

    case ID_GAME_MESSAGE_1:
    {
        pachetI->ignorePacketMessage();
        pachetI->read( text );
        if( text == "" )
            return;
        if( m_listChat )
            m_listChat->Append( text.c_str() );
    }
    break;

    case ID_GAME_MESSAGE_2:
    {
        int value = 0;
        pachetI->ignorePacketMessage();
        pachetI->read( value );
        if( value == 5 )
        {
            if( m_listChat )
                m_listChat->Append( "I receive 5 pushes!" );

            SmartPtr<CPacket> packet = m_parent->createPacket();
            packet->write( static_cast<unsigned char>( ID_GAME_MESSAGE_1 ) );
            packet->write( "I receive 5 pushes!" );
            //m_parent->sendPacket((SmartPtr<IPacket>)packet);
        }
    }
    break;

    default:
    {
        char str[512];
        //sprintf_s(str, "Message with identifier %i has arrived.", pachetI->getPacket()->data[0]);
        m_listChat->Append( str );
    }

    break;
    }

    /*pachetI->read(text);
    if(text == "")
        return;
    if(m_listChat)
        m_listChat->Append(text.c_str());*/
}

void ChatListener::onConnect( const u16 playerId )
{
    m_listChat->Append( "Client connected" );
}

void ChatListener::connect( u32 playerId )
{
}

void ChatListener::disconnect( u32 playerId )
{
}
#endif
