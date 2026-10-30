#include <WPNetwork/WPNetworkManager.hpp>
#include <WPNetwork/WPNetworkPacket.hpp>
#include <WPNetwork/WPNetworkSystemAddress.hpp>
#include <Workphone/Interface/Net/INetworkListener.hpp>
#include <Workphone/Scene/Components/NetworkListener.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <memory>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPNetworkManager, INetworkManager );

    WPNetworkManager::WPNetworkManager()
    {
        m_listener = SmartPtr<scene::NetworkListener>( new scene::NetworkListener() );
        net_context_init( &m_context );
    }

    WPNetworkManager::WPNetworkManager( bool isClient ) : WPNetworkManager()
    {
        m_isServer = !isClient;
    }

    WPNetworkManager::~WPNetworkManager()
    {
        unload( nullptr );
    }

    void WPNetworkManager::unload( SmartPtr<ISharedObject> data )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        net_context_shutdown( &m_context );
        m_started = m_connected = false;
        m_terminalState = WPNetworkSessionState::Stopped;
        m_listener->clearListeners();
        if( m_socketRuntimeInitialized )
        {
            net_shutdown();
            m_socketRuntimeInitialized = false;
        }
        INetworkManager::unload( data );
    }

    void WPNetworkManager::poll()
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        update();
    }

    void WPNetworkManager::disconnect()
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        if( m_started && m_context.mode == NET_MODE_SERVER )
        {
            throw std::logic_error( "WPNetwork: use unload to stop a server" );
        }
        if( !m_started )
        {
            return;
        }
        if( !m_context.connecting && m_context.server_peer_id == NET_INVALID_PEER )
        {
            return;  // Preserve any queued terminal callback on repeated calls.
        }
        // Reserve a fresh endpoint while the old socket is still bound. A late
        // accept/disconnect from the old attempt must not enter the new session.
        auto next = std::make_unique<NetContext>();
        net_context_init( next.get() );
        if( net_start_client_on( next.get(), m_bindAddress ) != NET_RESULT_OK )
        {
            throw std::runtime_error( "WPNetwork: cannot reserve reconnect socket" );
        }
        const auto oldPeer = m_context.server_peer_id;
        net_disconnect( &m_context );
        net_context_shutdown( &m_context );
        m_context = *next;
        if( oldPeer != NET_INVALID_PEER )
        {
            auto &event = m_context.events[0];
            event.type = NET_EVENT_DISCONNECTED;
            event.peer_id = oldPeer;
            m_context.event_tail = 1;
        }
        m_connected = false;
        m_terminalState = WPNetworkSessionState::Disconnected;
        m_lastError.clear();
    }

    WPNetworkDiagnostics WPNetworkManager::getDiagnostics() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        WPNetworkDiagnostics result;
        result.state = m_terminalState;
        result.server = m_isServer;
        result.capabilities = getCapabilities();
        result.lastError = m_lastError;
        result.boundPort = net_get_bound_port( &m_context );
        NetAddress bound{};
        if( net_get_bound_address( &m_context, &bound ) == NET_RESULT_OK )
        {
            result.boundHost = bound.host;
        }
        if( m_context.local_player_id >= 0 )
        {
            result.localPlayerId = static_cast<u16>( m_context.local_player_id );
        }
        result.queuedEvents =
            ( m_context.event_tail + NET_MAX_EVENTS - m_context.event_head ) % NET_MAX_EVENTS;
        result.droppedEvents = m_context.events_dropped;
        result.receivedDatagrams = m_context.datagrams_received;
        result.malformedDatagrams = m_context.malformed_datagrams;
        for( int i = 0; i < m_context.max_peers; ++i )
        {
            result.peerCount += m_context.peers[i].active ? 1 : 0;
        }
        if( m_started )
        {
            if( m_isServer )
            {
                result.state = WPNetworkSessionState::Listening;
            }
            else if( m_context.server_peer_id != NET_INVALID_PEER )
            {
                result.state = WPNetworkSessionState::Connected;
            }
            else if( m_context.connecting )
            {
                result.state = WPNetworkSessionState::Connecting;
            }
            else if( result.state == WPNetworkSessionState::Connecting )
            {
                result.state = WPNetworkSessionState::Failed;
            }
            else if( result.state == WPNetworkSessionState::Connected )
            {
                result.state = WPNetworkSessionState::Disconnected;
            }
        }
        return result;
    }

    u32 WPNetworkManager::getCapabilities() const
    {
        return UnreliableDelivery;
    }

    s32 WPNetworkManager::getPacketSenderId( SmartPtr<IPacket> packet ) const
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        if( !packet || !m_started )
        {
            return -1;
        }
        auto packetAddress = packet->getSystemAddress();
        auto address = dynamic_cast<WPNetworkSystemAddress *>( packetAddress.get() );
        if( !address )
        {
            return -1;
        }
        for( int i = 0; i < m_context.max_peers; ++i )
        {
            const auto &peer = m_context.peers[i];
            if( peer.active && net_address_equal( &peer.address, &address->getNetAddress() ) )
            {
                return m_context.mode == NET_MODE_CLIENT ? 0 : peer.id;
            }
        }
        return -1;
    }

    void WPNetworkManager::setServer( bool isServer )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        if( !m_socketRuntimeInitialized )
        {
            if( net_init() != NET_RESULT_OK )
            {
                m_terminalState = WPNetworkSessionState::Failed;
                m_lastError = "Socket runtime startup failed";
                throw std::runtime_error( "WPNetwork: socket runtime startup failed" );
            }
            m_socketRuntimeInitialized = true;
        }
        net_context_shutdown( &m_context );
        net_context_init( &m_context );

        m_isServer = isServer;
        m_connected = false;
        m_lastError.clear();
        m_terminalState = WPNetworkSessionState::Idle;

        if( m_isServer )
        {
            m_started =
                net_start_server_on(
                    &m_context, net_make_address( m_bindAddress, static_cast<unsigned short>( m_port ) ),
                    static_cast<int>( m_maxClients ) ) == NET_RESULT_OK;
            m_connected = m_started;
        }
        else
        {
            m_started = net_start_client_on( &m_context, m_bindAddress ) == NET_RESULT_OK;
        }
        if( !m_started )
        {
            m_terminalState = WPNetworkSessionState::Failed;
            m_lastError = "Socket startup or bind failed";
            throw std::runtime_error( "WPNetwork: socket startup or bind failed" );
        }
    }

    bool WPNetworkManager::isServer() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        return m_isServer;
    }

    void WPNetworkManager::setPeer()
    {
        throw std::logic_error( "WPNetwork: peer-to-peer mode is unsupported by native UDP" );
    }

    void WPNetworkManager::setVerbose( bool isverbose )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        m_verbose = isverbose;
    }

    void WPNetworkManager::setListener( SmartPtr<INetworkListener> netCallback )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        m_listener->clearListeners();
        m_listener->addListener( netCallback );
    }

    void WPNetworkManager::addListener( SmartPtr<INetworkListener> netCallback )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        m_listener->addListener( netCallback );
    }

    void WPNetworkManager::removeListener( SmartPtr<INetworkListener> netCallback )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        m_listener->removeListener( netCallback );
    }

    const u8 *WPNetworkManager::getPacketData( SmartPtr<IPacket> &packet, u32 &size ) const
    {
        size = 0;

        auto wpPacket = packet ? dynamic_cast<WPNetworkPacket *>( packet.get() ) : nullptr;
        if( !wpPacket )
        {
            throw std::invalid_argument( "WPNetwork: incompatible or null packet" );
        }

        size = wpPacket->getDataLength();
        return wpPacket->getData();
    }

    void WPNetworkManager::sendPacket( SmartPtr<IPacket> &outpacket )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        (void)outpacket;
        throw std::logic_error( "WPNetwork: reliable delivery requires a certified transport backend" );
    }

    void WPNetworkManager::sendPacketUnreliable( SmartPtr<IPacket> &outpacket )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        if( !m_started )
        {
            throw std::logic_error( "WPNetwork: session is not started" );
        }
        u32 size = 0;
        const auto *data = getPacketData( outpacket, size );
        if( !data || size == 0 )
        {
            return;
        }

        if( m_context.mode == NET_MODE_CLIENT )
        {
            if( net_send_to_server( &m_context, data, size ) != NET_RESULT_OK )
            {
                throw std::runtime_error( "WPNetwork: unreliable send to server failed" );
            }
            return;
        }

        for( int i = 0; i < m_context.max_peers; ++i )
        {
            if( m_context.peers[i].active )
            {
                if( net_send( &m_context, m_context.peers[i].id, data, size ) != NET_RESULT_OK )
                {
                    throw std::runtime_error( "WPNetwork: unreliable broadcast failed" );
                }
            }
        }
    }

    void WPNetworkManager::sendPacket( SmartPtr<IPacket> &outpacket, u16 playerId )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        (void)outpacket;
        (void)playerId;
        throw std::logic_error( "WPNetwork: reliable delivery requires a certified transport backend" );
    }

    void WPNetworkManager::sendPacketUnreliable( SmartPtr<IPacket> &outpacket, u16 playerId )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        u32 size = 0;
        const auto *data = getPacketData( outpacket, size );
        if( data && size > 0 )
        {
            const auto peer = m_context.mode == NET_MODE_CLIENT && playerId == 0
                                  ? m_context.server_peer_id
                                  : static_cast<NetPeerId>( playerId );
            if( net_send( &m_context, peer, data, size ) != NET_RESULT_OK )
            {
                throw std::runtime_error( "WPNetwork: unreliable send to player failed" );
            }
        }
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
        {
            return nullptr;
        }

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
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        (void)outpacket;
        (void)systemAddress;
        throw std::logic_error( "WPNetwork: reliable delivery requires a certified transport backend" );
    }

    void WPNetworkManager::sendPacketToAllExcept( SmartPtr<IPacket> &outpacket,
                                                  SmartPtr<ISystemAddress> systemAddress )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        (void)outpacket;
        (void)systemAddress;
        throw std::logic_error( "WPNetwork: reliable delivery requires a certified transport backend" );
    }

    const u32 WPNetworkManager::getPeerCount()
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        u32 count = 0;
        for( int i = 0; i < m_context.max_peers; ++i )
        {
            if( m_context.peers[i].active )
            {
                ++count;
            }
        }

        return count;
    }

    u16 WPNetworkManager::getPlayerNumber() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        return m_context.local_player_id < 0 ? 0xffffu : static_cast<u16>( m_context.local_player_id );
    }

    const u32 WPNetworkManager::getClientAddress( u16 playerId )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        for( int i = 0; i < m_context.max_peers; ++i )
        {
            if( m_context.peers[i].active &&
                m_context.peers[i].id == static_cast<NetPeerId>( playerId ) )
            {
                return m_context.peers[i].address.host;
            }
        }

        return 0;
    }

    String WPNetworkManager::getLocalRakNetGUID()
    {
        throw std::logic_error( "WPNetwork: RakNet GUID is unsupported by native UDP" );
    }

    void WPNetworkManager::kickClient( u16 playerId, bool hardKick )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        (void)hardKick;
        net_disconnect_peer( &m_context, static_cast<NetPeerId>( playerId ) );
    }

    WPNetworkManager::ConnectionStatus WPNetworkManager::getConnectionStatus()
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        if( !m_started )
        {
            return ConnectionStatus::NCS_FAILED;
        }

        if( m_isServer || m_context.server_peer_id != NET_INVALID_PEER )
        {
            return ConnectionStatus::NCS_ESTABLISHED;
        }

        return m_context.connecting ? ConnectionStatus::NCS_PENDING : ConnectionStatus::NCS_FAILED;
    }

    void WPNetworkManager::connect( const String &address, unsigned short port, const String &password )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        if( !password.empty() )
        {
            throw std::logic_error( "WPNetwork: native UDP cannot authenticate a password" );
        }

        if( !m_started || m_context.mode != NET_MODE_CLIENT )
        {
            setServer( false );
        }

        if( !m_context.connecting && m_context.server_peer_id == NET_INVALID_PEER )
        {
            // Error text belongs to the previous attempt; retain terminal peer
            // callbacks while preventing a retry from inheriting stale failures.
            auto write = m_context.event_head;
            for( auto read = m_context.event_head; read != m_context.event_tail;
                 read = ( read + 1 ) % NET_MAX_EVENTS )
            {
                if( m_context.events[read].type != NET_EVENT_ERROR )
                {
                    m_context.events[write] = m_context.events[read];
                    write = ( write + 1 ) % NET_MAX_EVENTS;
                }
            }
            m_context.event_tail = write;
        }
        if( net_connect( &m_context, address.c_str(), port ) != NET_RESULT_OK )
        {
            m_lastError = "Connection request failed";
            if( !m_context.connecting && m_context.server_peer_id == NET_INVALID_PEER )
            {
                m_terminalState = WPNetworkSessionState::Failed;
            }
            throw std::runtime_error( "WPNetwork: connection request failed" );
        }
        m_terminalState = WPNetworkSessionState::Connecting;
        m_lastError.clear();
    }

    const u32 WPNetworkManager::getPing()
    {
        throw std::logic_error( "WPNetwork: native UDP has no measured RTT" );
    }

    void WPNetworkManager::setNetIterations( u16 iterations )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        m_netIterations = iterations == 0 ? 1 : iterations;
    }

    void WPNetworkManager::setGlobalPacketRelay( bool relay )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        if( relay )
        {
            throw std::logic_error( "WPNetwork: blind packet relay is unsupported" );
        }
        m_globalRelay = false;
    }

    SmartPtr<IPacket> WPNetworkManager::createPacket()
    {
        return SmartPtr<IPacket>( new WPNetworkPacket() );
    }

    time_interval WPNetworkManager::getServerTime() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        if( !m_isServer )
        {
            throw std::logic_error( "WPNetwork: native UDP has no synchronized server clock" );
        }
        return static_cast<time_interval>( net_time_ms() ) * 0.001;
    }

    void WPNetworkManager::update()
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        if( !m_started || m_polling )
        {
            return;
        }
        m_polling = true;

        struct PollScope
        {
            bool &polling;

            ~PollScope()
            {
                polling = false;
            }
        } scope{ m_polling };

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
            if( !m_isServer )
            {
                m_terminalState = WPNetworkSessionState::Connected;
            }
            if( m_verbose )
            {
                std::printf( "WPNetworkManager: peer connected %d\n", event.peer_id );
            }
            if( m_listener )
            {
                m_listener->connect( static_cast<u32>( event.peer_id ) );
            }
        }
        else if( event.type == NET_EVENT_DISCONNECTED )
        {
            if( !m_isServer && !m_context.connecting && m_context.server_peer_id == NET_INVALID_PEER )
            {
                m_connected = false;
                m_terminalState = WPNetworkSessionState::Disconnected;
            }
            if( m_verbose )
            {
                std::printf( "WPNetworkManager: peer disconnected %d\n", event.peer_id );
            }
            if( m_listener )
            {
                m_listener->disconnect( static_cast<u32>( event.peer_id ) );
            }
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
        else if( event.type == NET_EVENT_ERROR )
        {
            m_lastError = event.message;
            if( !m_isServer && !m_context.connecting && m_context.server_peer_id == NET_INVALID_PEER &&
                m_terminalState != WPNetworkSessionState::Disconnected )
            {
                m_terminalState = WPNetworkSessionState::Failed;
            }
            if( m_verbose )
            {
                std::printf( "WPNetworkManager: %s\n", event.message );
            }
        }
    }

    u32 WPNetworkManager::getPort() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        return m_port;
    }

    void WPNetworkManager::setPort( u32 port )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        if( port > 65535u || m_started )
        {
            throw std::invalid_argument( "WPNetwork: invalid port or session already started" );
        }
        m_port = port;
    }

    u32 WPNetworkManager::getMaxClients() const
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        return m_maxClients;
    }

    void WPNetworkManager::setBindAddress( u32 address )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        if( m_started )
        {
            throw std::logic_error( "WPNetwork: bind interface cannot change while started" );
        }
        m_bindAddress = address;
    }

    void WPNetworkManager::setMaxClients( u32 maxClients )
    {
        std::lock_guard<std::recursive_mutex> lock( m_contextMutex );
        if( maxClients == 0 || maxClients > NET_MAX_PEERS || m_started )
        {
            throw std::invalid_argument( "WPNetwork: invalid capacity or session already started" );
        }
        m_maxClients = maxClients;
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
