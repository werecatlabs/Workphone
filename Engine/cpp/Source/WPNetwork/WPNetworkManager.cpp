#include <WPNetwork/WPNetworkManager.hpp>
#include <WPNetwork/WPNetworkPacket.hpp>
#include <WPNetwork/WPNetworkSystemAddress.hpp>
#include <Workphone/Interface/Net/INetworkListener.hpp>
#include <Workphone/Scene/Components/NetworkListener.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <cstdio>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPNetworkManager, INetworkManager );

    WPNetworkManager::WPNetworkManager()
    {
        m_listener = SmartPtr<scene::NetworkListener>( new scene::NetworkListener() );
        net_init();
        net_context_init( &m_context );
    }

    WPNetworkManager::WPNetworkManager( bool isClient ) : WPNetworkManager()
    {
        m_isServer = !isClient;
    }

    WPNetworkManager::~WPNetworkManager()
    {
        net_context_shutdown( &m_context );
        net_shutdown();
    }

    void WPNetworkManager::setServer( bool isServer )
    {
        net_context_shutdown( &m_context );
        net_context_init( &m_context );

        m_isServer = isServer;
        m_connected = false;

        if( m_isServer )
        {
            m_started = net_start_server( &m_context, static_cast<unsigned short>( m_port ),
                                          static_cast<int>( m_maxClients ) ) == NET_RESULT_OK;
            m_connected = m_started;
        }
        else
        {
            m_started = net_start_client( &m_context ) == NET_RESULT_OK;
        }
    }

    bool WPNetworkManager::isServer() const
    {
        return m_isServer;
    }

    void WPNetworkManager::setPeer()
    {
        setServer( true );
        m_isServer = false;
    }

    void WPNetworkManager::setVerbose( bool isverbose )
    {
        m_verbose = isverbose;
    }

    void WPNetworkManager::setListener( SmartPtr<INetworkListener> netCallback )
    {
        m_listener->clearListeners();
        m_listener->addListener( netCallback );
    }

    void WPNetworkManager::addListener( SmartPtr<INetworkListener> netCallback )
    {
        m_listener->addListener( netCallback );
    }

    void WPNetworkManager::removeListener( SmartPtr<INetworkListener> netCallback )
    {
        m_listener->removeListener( netCallback );
    }

    const u8 *WPNetworkManager::getPacketData( SmartPtr<IPacket> &packet, u32 &size ) const
    {
        size = 0;

        auto wpPacket = packet ? dynamic_cast<WPNetworkPacket *>( packet.get() ) : nullptr;
        if( !wpPacket )
            return nullptr;

        size = wpPacket->getDataLength();
        return wpPacket->getData();
    }

    void WPNetworkManager::sendPacket( SmartPtr<IPacket> &outpacket )
    {
        u32 size = 0;
        const auto *data = getPacketData( outpacket, size );
        if( !data || size == 0 )
            return;

        if( m_context.mode == NET_MODE_CLIENT )
        {
            net_send_to_server( &m_context, data, size );
            return;
        }

        for( int i = 0; i < m_context.max_peers; ++i )
        {
            if( m_context.peers[i].active )
                net_send( &m_context, m_context.peers[i].id, data, size );
        }
    }

    void WPNetworkManager::sendPacket( SmartPtr<IPacket> &outpacket, u16 playerId )
    {
        u32 size = 0;
        const auto *data = getPacketData( outpacket, size );
        if( data && size > 0 )
            net_send( &m_context, static_cast<NetPeerId>( playerId ), data, size );
    }

    NetPeer *WPNetworkManager::findPeerByAddress( SmartPtr<ISystemAddress> systemAddress )
    {
        return const_cast<NetPeer *>(
            static_cast<const WPNetworkManager *>( this )->findPeerByAddress( systemAddress ) );
    }

    const NetPeer *WPNetworkManager::findPeerByAddress( SmartPtr<ISystemAddress> systemAddress ) const
    {
        auto address =
            systemAddress ? dynamic_cast<WPNetworkSystemAddress *>( systemAddress.get() ) : nullptr;
        if( !address )
            return nullptr;

        for( int i = 0; i < m_context.max_peers; ++i )
        {
            if( m_context.peers[i].active &&
                net_address_equal( &m_context.peers[i].address, &address->getNetAddress() ) )
            {
                return &m_context.peers[i];
            }
        }

        return nullptr;
    }

    void WPNetworkManager::sendPacket( SmartPtr<IPacket> &outpacket,
                                       SmartPtr<ISystemAddress> systemAddress )
    {
        auto peer = findPeerByAddress( systemAddress );
        if( peer )
            sendPacket( outpacket, static_cast<u16>( peer->id ) );
    }

    void WPNetworkManager::sendPacketToAllExcept( SmartPtr<IPacket> &outpacket,
                                                  SmartPtr<ISystemAddress> systemAddress )
    {
        auto excluded = findPeerByAddress( systemAddress );
        u32 size = 0;
        const auto *data = getPacketData( outpacket, size );
        if( !data || size == 0 )
            return;

        for( int i = 0; i < m_context.max_peers; ++i )
        {
            const auto &peer = m_context.peers[i];
            if( peer.active && ( !excluded || peer.id != excluded->id ) )
                net_send( &m_context, peer.id, data, size );
        }
    }

    void WPNetworkManager::sendPacketUnreliable( SmartPtr<IPacket> &outpacket )
    {
        sendPacket( outpacket );
    }

    void WPNetworkManager::sendPacketUnreliable( SmartPtr<IPacket> &outpacket, u16 playerId )
    {
        sendPacket( outpacket, playerId );
    }

    const u32 WPNetworkManager::getPeerCount()
    {
        u32 count = 0;
        for( int i = 0; i < m_context.max_peers; ++i )
        {
            if( m_context.peers[i].active )
                ++count;
        }

        return count;
    }

    u16 WPNetworkManager::getPlayerNumber() const
    {
        return static_cast<u16>( m_context.server_peer_id );
    }

    const u32 WPNetworkManager::getClientAddress( u16 playerId )
    {
        for( int i = 0; i < m_context.max_peers; ++i )
        {
            if( m_context.peers[i].active &&
                m_context.peers[i].id == static_cast<NetPeerId>( playerId ) )
                return m_context.peers[i].address.host;
        }

        return 0;
    }

    String WPNetworkManager::getLocalRakNetGUID()
    {
        return String();
    }

    void WPNetworkManager::kickClient( u16 playerId, bool hardKick )
    {
        (void)hardKick;
        net_disconnect_peer( &m_context, static_cast<NetPeerId>( playerId ) );
    }

    WPNetworkManager::ConnectionStatus WPNetworkManager::getConnectionStatus()
    {
        if( !m_started )
            return ConnectionStatus::NCS_FAILED;

        if( m_isServer || m_connected || m_context.server_peer_id != NET_INVALID_PEER )
            return ConnectionStatus::NCS_ESTABLISHED;

        return ConnectionStatus::NCS_PENDING;
    }

    void WPNetworkManager::connect( const String &address, unsigned short port, const String &password )
    {
        (void)password;

        if( !m_started || m_context.mode != NET_MODE_CLIENT )
            setServer( false );

        if( net_connect( &m_context, address.c_str(), port ) != NET_RESULT_OK )
            m_connected = false;
    }

    const u32 WPNetworkManager::getPing()
    {
        return 0;
    }

    void WPNetworkManager::setNetIterations( u16 iterations )
    {
        m_netIterations = iterations == 0 ? 1 : iterations;
    }

    void WPNetworkManager::setGlobalPacketRelay( bool relay )
    {
        m_globalRelay = relay;
    }

    SmartPtr<IPacket> WPNetworkManager::createPacket()
    {
        return SmartPtr<IPacket>( new WPNetworkPacket() );
    }

    time_interval WPNetworkManager::getServerTime() const
    {
        return static_cast<time_interval>( net_time_ms() ) * 0.001;
    }

    void WPNetworkManager::update()
    {
        if( !m_started )
            return;

        net_update( &m_context );

        NetEvent event;
        u16 processed = 0;
        while( processed < m_netIterations && net_poll_event( &m_context, &event ) )
        {
            dispatchEvent( event );
            ++processed;
        }
    }

    void WPNetworkManager::dispatchEvent( const NetEvent &event )
    {
        if( event.type == NET_EVENT_CONNECTED )
        {
            m_connected = true;
            if( m_verbose )
                std::printf( "WPNetworkManager: peer connected %d\n", event.peer_id );
            if( m_listener )
                m_listener->connect( static_cast<u32>( event.peer_id ) );
        }
        else if( event.type == NET_EVENT_DISCONNECTED )
        {
            if( !m_isServer )
                m_connected = false;
            if( m_verbose )
                std::printf( "WPNetworkManager: peer disconnected %d\n", event.peer_id );
            if( m_listener )
                m_listener->disconnect( static_cast<u32>( event.peer_id ) );
        }
        else if( event.type == NET_EVENT_PACKET )
        {
            if( m_listener )
            {
                auto packet = SmartPtr<WPNetworkPacket>( new WPNetworkPacket( event ) );
                for( int i = 0; i < m_context.max_peers; ++i )
                {
                    if( m_context.peers[i].active && m_context.peers[i].id == event.peer_id )
                    {
                        packet->setSystemAddress( SmartPtr<ISystemAddress>(
                            new WPNetworkSystemAddress( m_context.peers[i].address ) ) );
                        break;
                    }
                }

                m_listener->handlePacket( packet );
            }

            if( m_isServer && m_globalRelay )
            {
                auto packet = SmartPtr<WPNetworkPacket>( new WPNetworkPacket( event ) );
                SmartPtr<ISystemAddress> address;
                for( int i = 0; i < m_context.max_peers; ++i )
                {
                    if( m_context.peers[i].active && m_context.peers[i].id == event.peer_id )
                    {
                        address = SmartPtr<ISystemAddress>(
                            new WPNetworkSystemAddress( m_context.peers[i].address ) );
                        packet->setSystemAddress( address );
                        break;
                    }
                }

                SmartPtr<IPacket> relayPacket = packet;
                sendPacketToAllExcept( relayPacket, address );
            }
        }
        else if( event.type == NET_EVENT_ERROR && m_verbose )
        {
            std::printf( "WPNetworkManager: %s\n", event.message );
        }
    }

    u32 WPNetworkManager::getPort() const
    {
        return m_port;
    }

    void WPNetworkManager::setPort( u32 port )
    {
        m_port = std::min<u32>( port, 65535u );
    }

    u32 WPNetworkManager::getMaxClients() const
    {
        return m_maxClients;
    }

    void WPNetworkManager::setMaxClients( u32 maxClients )
    {
        m_maxClients = std::max<u32>( 1u, std::min<u32>( maxClients, NET_MAX_PEERS ) );
    }

    NetContext *WPNetworkManager::getContext()
    {
        return &m_context;
    }

    const NetContext *WPNetworkManager::getContext() const
    {
        return &m_context;
    }
}  // namespace workphone
