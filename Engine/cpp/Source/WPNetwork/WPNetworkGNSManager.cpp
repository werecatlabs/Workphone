#if WP_NETWORK_GNS
#    include <WPNetwork/WPNetworkGNSManager.hpp>
#    include <WPNetwork/WPNetworkPacket.hpp>
#    include <WPNetwork/WPNetworkSystemAddress.hpp>
#    include <Workphone/Scene/Components/NetworkListener.hpp>
#    include <Workphone/Interface/Net/NetworkCodec.hpp>
#    include <Workphone/Workphone.hpp>
#    include <steam/steamnetworkingsockets.h>
#    include <steam/isteamnetworkingutils.h>
#    include <map>
#    include <deque>
#    include <mutex>
#    include <stdexcept>
#    include <algorithm>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPNetworkGNSManager, INetworkManager );

    struct WPNetworkGNSManager::Impl : std::enable_shared_from_this<Impl>
    {
        struct Runtime
        {
            std::recursive_mutex mutex;
            unsigned users = 0;
            std::map<HSteamListenSocket, std::weak_ptr<Impl>> listeners;
            std::map<HSteamNetConnection, std::weak_ptr<Impl>> connections;
        };

        static Runtime &runtime()
        {
            static Runtime value;
            return value;
        }

        struct Endpoint : WPNetworkSystemAddress
        {
            std::weak_ptr<Impl> owner;
            HSteamNetConnection connection;
            SteamNetworkingIPAddr ip;

            Endpoint( std::shared_ptr<Impl> value, HSteamNetConnection handle,
                      SteamNetworkingIPAddr address ) :
                owner( value ),
                connection( handle ),
                ip( address )
            {
            }

            String toString() const override
            {
                char text[128];
                ip.ToString( text, sizeof( text ), true );
                return text;
            }

            u32 getBinaryAddress() const override
            {
                return ip.IsIPv4() ? ip.GetIPv4() : 0;
            }

            u16 getPort() const override
            {
                return ip.m_port;
            }

            bool isValid() const override
            {
                auto value = owner.lock();
                if( !value )
                {
                    return false;
                }
                std::lock_guard<std::recursive_mutex> lock( runtime().mutex );
                auto peer = value->peers.find( connection );
                return peer != value->peers.end() && peer->second.ready &&
                       peer->second.endpoint.get() == this;
            }

            void setBinaryAddress( u32 ) override
            {
                throw std::logic_error( "Received GNS endpoint is immutable" );
            }

            void setPort( u16 ) override
            {
                throw std::logic_error( "Received GNS endpoint is immutable" );
            }
        };

        struct Peer
        {
            u16 player = 0xffffu;
            bool transportConnected = false;
            bool ready = false;
            SmartPtr<Endpoint> endpoint;
        };

        WPNetworkGNSConfig config;
        SmartPtr<scene::NetworkListener> listener =
            SmartPtr<scene::NetworkListener>( new scene::NetworkListener() );
        ISteamNetworkingSockets *sockets = nullptr;
        HSteamListenSocket listening = k_HSteamListenSocket_Invalid;
        HSteamNetPollGroup group = k_HSteamNetPollGroup_Invalid;
        std::map<HSteamNetConnection, Peer> peers;
        std::deque<std::pair<bool, u16>> events;
        u16 local = 0xffffu;
        u32 nextPlayer = 1;
        bool server = false, started = false, polling = false, fatal = false;
        String error;
        static constexpr size_t MaxPayload = 65536;

        explicit Impl( const WPNetworkGNSConfig &value ) : config( value )
        {
            if( config.allowedPeerIdentities.size() > 4096 )
            {
                throw std::invalid_argument( "GNS identity allowlist exceeds limit" );
            }
            for( const auto &text : config.allowedPeerIdentities )
            {
                SteamNetworkingIdentity identity{};
                if( !identity.ParseString( text.c_str() ) )
                {
                    throw std::invalid_argument( "Invalid GNS peer identity" );
                }
            }
            if( config.maxClients == 0 || config.maxClients > 64 || config.eventsPerPoll == 0 ||
                config.eventsPerPoll > 256 )
            {
                throw std::invalid_argument( "Invalid GNS session limits" );
            }
        }

        void acquire()
        {
            if( sockets )
            {
                return;
            }
            auto &r = runtime();
            std::lock_guard<std::recursive_mutex> lock( r.mutex );
            if( !r.users )
            {
                SteamDatagramErrMsg message{};
                if( !GameNetworkingSockets_Init( nullptr, message ) )
                {
                    throw std::runtime_error( message );
                }
            }
            ++r.users;
            sockets = SteamNetworkingSockets();
        }

        ~Impl()
        {
            auto &r = runtime();
            std::lock_guard<std::recursive_mutex> lock( r.mutex );
            stop();
        }

        bool authorized( const SteamNetworkingIdentity &identity )
        {
            for( const auto &text : config.allowedPeerIdentities )
            {
                SteamNetworkingIdentity expected{};
                if( expected.ParseString( text.c_str() ) && expected == identity )
                {
                    return true;
                }
            }
            return false;
        }

        static void callback( SteamNetConnectionStatusChangedCallback_t *event )
        {
            auto &r = runtime();
            std::lock_guard<std::recursive_mutex> lock( r.mutex );
            std::shared_ptr<Impl> self;
            auto found = r.connections.find( event->m_hConn );
            if( found != r.connections.end() )
            {
                self = found->second.lock();
            }
            if( !self )
            {
                auto listen = r.listeners.find( event->m_info.m_hListenSocket );
                if( listen != r.listeners.end() )
                {
                    self = listen->second.lock();
                }
            }
            if( !self || !self->started )
            {
                return;
            }
            auto &s = *self;
            const auto handle = event->m_hConn;
            if( event->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting && s.server )
            {
                if( s.peers.count( handle ) )
                {
                    return;
                }
                if( s.fatal || s.events.size() >= 512 || s.peers.size() >= s.config.maxClients ||
                    s.nextPlayer > 65534 ||
                    ( s.config.allowUnverifiedLoopback
                          ? !event->m_info.m_addrRemote.IsLocalHost()
                          : !s.authorized( event->m_info.m_identityRemote ) ) )
                {
                    s.sockets->CloseConnection( handle, 1001, "Admission rejected", false );
                    return;
                }
                Peer peer;
                peer.player = static_cast<u16>( s.nextPlayer++ );
                peer.endpoint =
                    SmartPtr<Endpoint>( new Endpoint( self, handle, event->m_info.m_addrRemote ) );
                s.peers.emplace( handle, peer );
                r.connections[handle] = self;
                if( !s.sockets->SetConnectionPollGroup( handle, s.group ) ||
                    s.sockets->AcceptConnection( handle ) != k_EResultOK )
                {
                    s.close( handle );
                }
            }
            else if( event->m_info.m_eState == k_ESteamNetworkingConnectionState_Connected )
            {
                auto peer = s.peers.find( handle );
                if( peer == s.peers.end() )
                {
                    return;
                }
                if( !s.config.allowUnverifiedLoopback &&
                    !s.authorized( event->m_info.m_identityRemote ) )
                {
                    s.close( handle );
                    return;
                }
                peer->second.transportConnected = true;
                if( s.server )
                {
                    const u16 id = peer->second.player;
                    const u8 hello[] = { 0, 1, static_cast<u8>( id ), static_cast<u8>( id >> 8 ) };
                    if( s.sockets->SendMessageToConnection( handle, hello, sizeof( hello ),
                                                            k_nSteamNetworkingSend_Reliable,
                                                            nullptr ) != k_EResultOK )
                    {
                        s.close( handle );
                        return;
                    }
                    peer->second.ready = true;
                    s.notify( true, id );
                }
            }
            else if( event->m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer ||
                     event->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally )
            {
                if( s.peers.count( handle ) )
                {
                    s.error = event->m_info.m_szEndDebug;
                    s.close( handle );
                }
            }
        }

        void notify( bool joined, u16 player )
        {
            if( events.size() >= 512 )
            {
                fatal = true;
                error = "GNS control-event queue exceeded limit";
            }
            else
            {
                events.emplace_back( joined, player );
            }
        }

        void close( HSteamNetConnection handle )
        {
            auto p = peers.find( handle );
            if( p == peers.end() )
            {
                return;
            }
            if( p->second.ready )
            {
                notify( false, p->second.player );
            }
            sockets->CloseConnection( handle, 0, nullptr, false );
            runtime().connections.erase( handle );
            peers.erase( p );
            if( !server )
            {
                local = 0xffffu;
            }
        }

        void stop()
        {
            if( !sockets )
            {
                listener->clearListeners();
                return;
            }
            while( !peers.empty() )
            {
                close( peers.begin()->first );
            }
            if( listening != k_HSteamListenSocket_Invalid )
            {
                runtime().listeners.erase( listening );
                sockets->CloseListenSocket( listening );
                listening = k_HSteamListenSocket_Invalid;
            }
            if( group != k_HSteamNetPollGroup_Invalid )
            {
                sockets->DestroyPollGroup( group );
                group = k_HSteamNetPollGroup_Invalid;
            }
            listener->clearListeners();
            events.clear();
            started = false;
            local = 0xffffu;
            sockets = nullptr;
            if( --runtime().users == 0 )
            {
                GameNetworkingSockets_Kill();
            }
        }

        void start( bool asServer )
        {
            if( started )
            {
                throw std::logic_error( "Stop GNS session before changing role" );
            }
            acquire();
            if( !config.allowUnverifiedLoopback )
            {
                SteamNetAuthenticationStatus_t auth{};
                if( config.allowedPeerIdentities.empty() ||
                    sockets->GetAuthenticationStatus( &auth ) != k_ESteamNetworkingAvailability_Current )
                {
                    throw std::logic_error(
                        "GNS strict mode requires a provisioned certificate and explicit peer "
                        "identities" );
                }
            }
            server = asServer;
            error.clear();
            nextPlayer = 1;
            fatal = false;
            group = sockets->CreatePollGroup();
            if( group == k_HSteamNetPollGroup_Invalid )
            {
                throw std::runtime_error( "Cannot create GNS poll group" );
            }
            SteamNetworkingConfigValue_t options[9];
            optionsFor( options );
            if( server )
            {
                SteamNetworkingIPAddr ip;
                ip.Clear();
                if( !ip.ParseString( config.bindAddress.c_str() ) || !ip.m_port ||
                    ( config.allowUnverifiedLoopback && !ip.IsLocalHost() ) )
                {
                    sockets->DestroyPollGroup( group );
                    group = k_HSteamNetPollGroup_Invalid;
                    throw std::invalid_argument(
                        "Invalid GNS bind endpoint or non-loopback development endpoint" );
                }
                listening = sockets->CreateListenSocketIP( ip, 9, options );
                if( listening == k_HSteamListenSocket_Invalid )
                {
                    sockets->DestroyPollGroup( group );
                    group = k_HSteamNetPollGroup_Invalid;
                    throw std::runtime_error( "GNS listener startup failed" );
                }
                runtime().listeners[listening] = shared_from_this();
                local = 0;
            }
            started = true;
        }

        void optionsFor( SteamNetworkingConfigValue_t ( &options )[9] )
        {
            options[0].SetPtr( k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
                               reinterpret_cast<void *>( &callback ) );
            options[1].SetInt32( k_ESteamNetworkingConfig_IP_AllowWithoutAuth,
                                 config.allowUnverifiedLoopback ? 1 : 0 );
            options[2].SetInt32( k_ESteamNetworkingConfig_IPLocalHost_AllowWithoutAuth,
                                 config.allowUnverifiedLoopback ? 1 : 0 );
            options[3].SetInt32( k_ESteamNetworkingConfig_Unencrypted, 0 );
            options[4].SetInt32( k_ESteamNetworkingConfig_TimeoutInitial, 5000 );
            options[5].SetInt32( k_ESteamNetworkingConfig_SendBufferSize, 1024 * 1024 );
            options[6].SetInt32( k_ESteamNetworkingConfig_RecvBufferSize, 1024 * 1024 );
            options[7].SetInt32( k_ESteamNetworkingConfig_RecvBufferMessages, 256 );
            options[8].SetInt32( k_ESteamNetworkingConfig_RecvMaxMessageSize,
                                 static_cast<int>( MaxPayload + 1 ) );
        }

        void send( SmartPtr<IPacket> packet, int flags, int player = -1,
                   HSteamNetConnection except = k_HSteamNetConnection_Invalid )
        {
            if( !started || ( !server && local == 0xffffu ) )
            {
                throw std::logic_error( "GNS session is not ready" );
            }
            auto data = packet ? dynamic_cast<WPNetworkPacket *>( packet.get() ) : nullptr;
            if( !data )
            {
                throw std::invalid_argument( "GNS requires a WPNetwork packet" );
            }
            if( data->getDataLength() > MaxPayload )
            {
                throw std::length_error( "GNS payload exceeds 64KiB limit" );
            }
            std::vector<u8> bytes{ 1 };
            if( data->getDataLength() )
            {
                bytes.insert( bytes.end(), data->getData(), data->getData() + data->getDataLength() );
            }
            bool sent = false;
            for( const auto &[handle, peer] : peers )
            {
                if( !peer.ready || handle == except || ( player >= 0 && peer.player != player ) )
                {
                    continue;
                }
                if( sockets->SendMessageToConnection( handle, bytes.data(),
                                                      static_cast<uint32>( bytes.size() ), flags,
                                                      nullptr ) != k_EResultOK )
                {
                    throw std::runtime_error( "GNS send rejected or send queue full" );
                }
                sent = true;
            }
            if( player >= 0 && !sent )
            {
                throw std::invalid_argument( "GNS target player is not connected" );
            }
        }

        HSteamNetConnection endpoint( SmartPtr<ISystemAddress> address )
        {
            auto p = address ? dynamic_cast<Endpoint *>( address.get() ) : nullptr;
            if( !p || p->owner.lock().get() != this || !p->isValid() )
            {
                throw std::invalid_argument( "Stale or foreign GNS endpoint" );
            }
            return p->connection;
        }
    };

    namespace
    {
        [[noreturn]] void unsupported()
        {
            throw std::logic_error( "Operation is unsupported by the GNS facade" );
        }
    }  // namespace

#    define GNS_LOCK       \
        auto &s = *m_impl; \
        std::lock_guard<std::recursive_mutex> lock( Impl::runtime().mutex )

    WPNetworkGNSManager::WPNetworkGNSManager( const WPNetworkGNSConfig &config ) :
        m_impl( std::make_shared<Impl>( config ) )
    {
    }

    WPNetworkGNSManager::~WPNetworkGNSManager()
    {
        unload( nullptr );
    }

    void WPNetworkGNSManager::unload( SmartPtr<ISharedObject> data )
    {
        GNS_LOCK;
        s.stop();
        INetworkManager::unload( data );
    }

    void WPNetworkGNSManager::setServer( bool server )
    {
        GNS_LOCK;
        s.start( server );
    }

    bool WPNetworkGNSManager::isServer() const
    {
        GNS_LOCK;
        return s.server;
    }

    void WPNetworkGNSManager::setPeer()
    {
        unsupported();
    }

    void WPNetworkGNSManager::setVerbose( bool verbose )
    {
        if( verbose )
        {
            unsupported();
        }
    }  // Retained errors are available through getLastError().

    void WPNetworkGNSManager::setListener( SmartPtr<INetworkListener> value )
    {
        GNS_LOCK;
        s.listener->clearListeners();
        s.listener->addListener( value );
    }

    void WPNetworkGNSManager::addListener( SmartPtr<INetworkListener> value )
    {
        GNS_LOCK;
        s.listener->addListener( value );
    }

    void WPNetworkGNSManager::removeListener( SmartPtr<INetworkListener> value )
    {
        GNS_LOCK;
        s.listener->removeListener( value );
    }

    void WPNetworkGNSManager::sendPacket( SmartPtr<IPacket> &p )
    {
        GNS_LOCK;
        s.send( p, k_nSteamNetworkingSend_Reliable );
    }

    void WPNetworkGNSManager::sendPacket( SmartPtr<IPacket> &p, u16 id )
    {
        GNS_LOCK;
        s.send( p, k_nSteamNetworkingSend_Reliable, id );
    }

    void WPNetworkGNSManager::sendPacketUnreliable( SmartPtr<IPacket> &p )
    {
        GNS_LOCK;
        s.send( p, k_nSteamNetworkingSend_Unreliable );
    }

    void WPNetworkGNSManager::sendPacketUnreliable( SmartPtr<IPacket> &p, u16 id )
    {
        GNS_LOCK;
        s.send( p, k_nSteamNetworkingSend_Unreliable, id );
    }

    void WPNetworkGNSManager::sendPacket( SmartPtr<IPacket> &p, SmartPtr<ISystemAddress> a )
    {
        GNS_LOCK;
        s.send( p, k_nSteamNetworkingSend_Reliable, s.peers.at( s.endpoint( a ) ).player );
    }

    void WPNetworkGNSManager::sendPacketToAllExcept( SmartPtr<IPacket> &p, SmartPtr<ISystemAddress> a )
    {
        GNS_LOCK;
        s.send( p, k_nSteamNetworkingSend_Reliable, -1, s.endpoint( a ) );
    }

    const u32 WPNetworkGNSManager::getPeerCount()
    {
        GNS_LOCK;
        return static_cast<u32>( std::count_if( s.peers.begin(), s.peers.end(),
                                                []( const auto &p ) { return p.second.ready; } ) );
    }

    u16 WPNetworkGNSManager::getPlayerNumber() const
    {
        GNS_LOCK;
        return s.local;
    }

    const u32 WPNetworkGNSManager::getClientAddress( u16 id )
    {
        GNS_LOCK;
        for( const auto &[h, p] : s.peers )
        {
            (void)h;
            if( p.player == id )
            {
                return p.endpoint->getBinaryAddress();
            }
        }
        return 0;
    }

    String WPNetworkGNSManager::getLocalRakNetGUID()
    {
        unsupported();
    }

    void WPNetworkGNSManager::kickClient( u16 id, bool )
    {
        GNS_LOCK;
        for( const auto &[h, p] : s.peers )
        {
            if( p.player == id )
            {
                s.close( h );
                return;
            }
        }
    }

    INetworkManager::ConnectionStatus WPNetworkGNSManager::getConnectionStatus()
    {
        GNS_LOCK;
        return !s.started ? ConnectionStatus::NCS_FAILED
                          : ( s.local != 0xffffu ? ConnectionStatus::NCS_ESTABLISHED
                                                 : ( !s.peers.empty() ? ConnectionStatus::NCS_PENDING
                                                                      : ConnectionStatus::NCS_FAILED ) );
    }

    void WPNetworkGNSManager::connect( const String &address, unsigned short port,
                                       const String &password )
    {
        GNS_LOCK;
        if( !password.empty() )
        {
            throw std::invalid_argument( "Use certificate admission, not plaintext GNS passwords" );
        }
        if( !s.started )
        {
            s.start( false );
        }
        if( s.server || !s.peers.empty() )
        {
            throw std::logic_error( "GNS client already started or connecting" );
        }
        SteamNetworkingIPAddr ip;
        ip.Clear();
        if( !ip.ParseString( address.c_str() ) || !port ||
            ( s.config.allowUnverifiedLoopback && !ip.IsLocalHost() ) )
        {
            throw std::invalid_argument( "Invalid GNS IP endpoint; development is loopback only" );
        }
        ip.m_port = port;
        SteamNetworkingConfigValue_t options[9];
        s.optionsFor( options );
        auto handle = s.sockets->ConnectByIPAddress( ip, 9, options );
        if( handle == k_HSteamNetConnection_Invalid )
        {
            throw std::runtime_error( "GNS connect failed" );
        }
        Impl::Peer peer;
        peer.player = 0;
        peer.endpoint = SmartPtr<Impl::Endpoint>( new Impl::Endpoint( m_impl, handle, ip ) );
        s.peers.emplace( handle, peer );
        Impl::runtime().connections[handle] = m_impl;
        if( !s.sockets->SetConnectionPollGroup( handle, s.group ) )
        {
            s.close( handle );
            throw std::runtime_error( "GNS poll-group assignment failed" );
        }
    }

    void WPNetworkGNSManager::disconnect()
    {
        GNS_LOCK;
        if( s.server )
        {
            throw std::logic_error( "Use unload to stop GNS server" );
        }
        while( !s.peers.empty() )
        {
            s.close( s.peers.begin()->first );
        }
    }

    const u32 WPNetworkGNSManager::getPing()
    {
        GNS_LOCK;
        if( s.server || s.local == 0xffffu )
        {
            throw std::logic_error( "GNS client RTT is unavailable" );
        }
        SteamNetConnectionRealTimeStatus_t status{};
        if( s.sockets->GetConnectionRealTimeStatus( s.peers.begin()->first, &status, 0, nullptr ) !=
                k_EResultOK ||
            status.m_nPing < 0 )
        {
            throw std::runtime_error( "GNS RTT is unavailable" );
        }
        return static_cast<u32>( status.m_nPing );
    }

    void WPNetworkGNSManager::setNetIterations( u16 value )
    {
        GNS_LOCK;
        if( !value || value > 256 )
        {
            throw std::invalid_argument( "Invalid GNS poll budget" );
        }
        s.config.eventsPerPoll = value;
    }

    void WPNetworkGNSManager::setGlobalPacketRelay( bool relay )
    {
        if( relay )
        {
            unsupported();
        }
    }

    SmartPtr<IPacket> WPNetworkGNSManager::createPacket()
    {
        return SmartPtr<IPacket>( new WPNetworkPacket() );
    }

    time_interval WPNetworkGNSManager::getServerTime() const
    {
        GNS_LOCK;
        if( !s.server )
        {
            unsupported();
        }
        return SteamNetworkingUtils()->GetLocalTimestamp() * 0.000001;
    }

    u32 WPNetworkGNSManager::getCapabilities() const
    {
        return UnreliableDelivery | ReliableOrderedDelivery;
    }

    String WPNetworkGNSManager::getLastError() const
    {
        GNS_LOCK;
        return s.error;
    }

    String WPNetworkGNSManager::getBoundAddress() const
    {
        GNS_LOCK;
        if( !s.sockets || s.listening == k_HSteamListenSocket_Invalid )
        {
            return {};
        }
        SteamNetworkingIPAddr ip;
        ip.Clear();
        if( !s.sockets->GetListenSocketAddress( s.listening, &ip ) )
        {
            return {};
        }
        char text[128];
        ip.ToString( text, sizeof( text ), true );
        return text;
    }

    s32 WPNetworkGNSManager::getPacketSenderId( SmartPtr<IPacket> packet ) const
    {
        GNS_LOCK;
        if( !packet )
        {
            return -1;
        }
        auto address = packet->getSystemAddress();
        auto p = address ? dynamic_cast<Impl::Endpoint *>( address.get() ) : nullptr;
        if( !p || p->owner.lock() != m_impl )
        {
            return -1;
        }
        auto it = s.peers.find( p->connection );
        return it != s.peers.end() && it->second.ready && it->second.endpoint.get() == p
                   ? it->second.player
                   : -1;
    }

    std::vector<u8> WPNetworkGNSManager::getCertificateRequest()
    {
        GNS_LOCK;
        s.acquire();
        if( s.started || Impl::runtime().users != 1 )
        {
            throw std::logic_error( "Certificate setup requires one stopped manager" );
        }
        int size = 0;
        SteamNetworkingErrMsg message{};
        if( !s.sockets->GetCertificateRequest( &size, nullptr, message ) || size <= 0 || size > 65536 )
        {
            throw std::runtime_error( "Cannot create GNS certificate request" );
        }
        std::vector<u8> bytes( size );
        if( !s.sockets->GetCertificateRequest( &size, bytes.data(), message ) )
        {
            throw std::runtime_error( "Cannot serialize GNS certificate request" );
        }
        bytes.resize( size );
        return bytes;
    }

    void WPNetworkGNSManager::installCertificate( const std::vector<u8> &bytes )
    {
        GNS_LOCK;
        s.acquire();
        if( s.started || Impl::runtime().users != 1 )
        {
            throw std::logic_error( "Certificate setup requires one stopped manager" );
        }
        if( bytes.empty() || bytes.size() > 65536 )
        {
            throw std::invalid_argument( "Invalid GNS certificate size" );
        }
        SteamNetworkingErrMsg message{};
        if( !s.sockets->SetCertificate( bytes.data(), static_cast<int>( bytes.size() ), message ) )
        {
            throw std::runtime_error( "GNS certificate rejected" );
        }
    }

    void WPNetworkGNSManager::poll()
    {
        auto lifetime = m_impl;
        auto &s = *lifetime;
        std::lock_guard<std::recursive_mutex> lock( Impl::runtime().mutex );
        if( !s.started || s.polling )
        {
            return;
        }
        s.polling = true;

        struct Guard
        {
            bool &value;

            ~Guard()
            {
                value = false;
            }
        } guard{ s.polling };

        s.sockets->RunCallbacks();
        if( s.fatal )
        {
            s.stop();
            return;
        }
        // Connection notifications precede packet callbacks, with bounded work.
        unsigned work = 0;
        while( s.started && !s.events.empty() && work++ < s.config.eventsPerPoll )
        {
            auto event = s.events.front();
            s.events.pop_front();
            if( event.first )
            {
                s.listener->connect( event.second );
            }
            else
            {
                s.listener->disconnect( event.second );
            }
        }
        // The SDK receive thread can queue data after RunCallbacks has drained its current
        // batch. Do not consume a client assignment before the Connected identity check.
        if( !s.server && ( s.peers.empty() || !s.peers.begin()->second.transportConnected ) )
        {
            return;
        }
        size_t bytes = 0;
        while( s.started && work++ < s.config.eventsPerPoll &&
               bytes <= 1024 * 1024 - ( Impl::MaxPayload + 1 ) )
        {
            SteamNetworkingMessage_t *raw = nullptr;
            const int count = s.sockets->ReceiveMessagesOnPollGroup( s.group, &raw, 1 );
            if( count == 0 )
            {
                break;
            }
            if( count < 0 )
            {
                s.error = "GNS receive failed";
                break;
            }

            struct Release
            {
                SteamNetworkingMessage_t *p;

                ~Release()
                {
                    if( p )
                    {
                        p->Release();
                    }
                }
            } release{ raw };

            if( raw->m_cbSize < 1 || raw->m_cbSize > static_cast<int>( Impl::MaxPayload + 1 ) )
            {
                s.error = "Invalid GNS frame size";
                continue;
            }
            bytes += raw->m_cbSize;
            auto peer = s.peers.find( raw->m_conn );
            if( peer == s.peers.end() )
            {
                continue;
            }
            const auto *data = static_cast<const u8 *>( raw->m_pData );
            const bool reliable = ( raw->m_nFlags & k_nSteamNetworkingSend_Reliable ) != 0;
            if( data[0] == 0 && !s.server && !peer->second.ready && reliable && raw->m_cbSize == 4 &&
                data[1] == 1 )
            {
                const u16 id = static_cast<u16>( data[2] | ( static_cast<u16>( data[3] ) << 8 ) );
                if( !id || id == 0xffffu )
                {
                    s.close( raw->m_conn );
                    continue;
                }
                s.local = id;
                peer->second.ready = true;
                s.notify( true, 0 );
                break;
            }
            else if( data[0] == 1 && peer->second.ready )
            {
                auto packet = SmartPtr<WPNetworkPacket>( new WPNetworkPacket() );
                packet->setData( data + 1, raw->m_cbSize - 1 );
                packet->setSystemAddress( peer->second.endpoint );
                raw->Release();
                release.p = nullptr;
                s.listener->handlePacket( packet );
            }
            else if( data[0] == 1 && !s.server && !reliable )
            {
                // Unreliable gameplay data can overtake the reliable player assignment.
                continue;
            }
            else
            {
                s.error = "Unexpected GNS session frame";
                s.close( raw->m_conn );
            }
        }
    }

#    undef GNS_LOCK
}  // namespace workphone
#endif
