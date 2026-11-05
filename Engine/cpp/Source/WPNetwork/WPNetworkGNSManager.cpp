#if WP_NETWORK_GNS
#    include "WPNetworkAdmissionTickets.hpp"
#    include <WPNetwork/WPNetworkGNSManager.hpp>
#    include <WPNetwork/WPNetworkPacket.hpp>
#    include <WPNetwork/WPNetworkSystemAddress.hpp>
#    include <Workphone/Scene/Components/NetworkListener.hpp>
#    include <Workphone/Interface/Net/NetworkCodec.hpp>
#    include <Workphone/Workphone.hpp>
#    include <steam/steamnetworkingsockets.h>
#    include <steam/isteamnetworkingutils.h>
#    include <openssl/crypto.h>
#    include <chrono>
#    include <map>
#    include <deque>
#    include <mutex>
#    include <optional>
#    include <stdexcept>
#    include <algorithm>

// Provided by the pinned static GNS build. The upstream default accepts expired
// certificates for another day, which is inappropriate for strict admission.
extern "C" void SteamNetworkingSockets_CertStore_SetExpiryGraceSeconds( int seconds );

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPNetworkGNSManager, INetworkManager );

    struct WPNetworkGNSManager::Impl : std::enable_shared_from_this<Impl>
    {
        using Clock = std::chrono::steady_clock;
        using Tickets = network::WPNetworkAdmissionTickets;
        static constexpr auto AdmissionTimeout = std::chrono::seconds( 5 );
        static constexpr size_t MaxPendingAdmissions = 16;
        static constexpr u8 AssignmentFrame = 0, GameplayFrame = 1, ChallengeFrame = 2,
                            ConfirmationFrame = 3, ControlVersion = 1, TicketRequired = 1;
        static constexpr size_t SecretSize = AdmissionTicket{}.size();
        static constexpr size_t ChallengeSize = 3 + SecretSize;
        static constexpr size_t ConfirmationSize = 2 + SecretSize;
        static constexpr const char *ConfirmationRejected = "Admission confirmation rejected";
        static constexpr const char *ConfirmationTimedOut = "Admission confirmation timed out";
        static constexpr const char *TicketRejected = "Admission ticket rejected";

        struct Runtime
        {
            std::recursive_mutex mutex;
            unsigned users = 0;
            const Impl *certificateOwner = nullptr;
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
            bool confirmationSent = false;
            Clock::time_point admissionDeadline{};
            AdmissionTicket challenge{};
            SteamNetworkingIdentity identity{};
            SmartPtr<Endpoint> endpoint;
        };

        WPNetworkGNSConfig config;
        std::vector<SteamNetworkingIdentity> allowedPeerIdentities;
        SmartPtr<scene::NetworkListener> listener =
            SmartPtr<scene::NetworkListener>( new scene::NetworkListener() );
        ISteamNetworkingSockets *sockets = nullptr;
        HSteamListenSocket listening = k_HSteamListenSocket_Invalid;
        HSteamNetPollGroup group = k_HSteamNetPollGroup_Invalid;
        std::map<HSteamNetConnection, Peer> peers;
        Tickets tickets;
        std::optional<AdmissionTicket> clientTicket;
        std::deque<std::pair<bool, u16>> events;
        u16 local = 0xffffu;
        u32 nextPlayer = 1;
        bool server = false, started = false, polling = false, fatal = false;
        bool installedCertificate = false;
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
                if( text.find( '\0' ) != String::npos || !identity.ParseString( text.c_str() ) ||
                    !isNamedIdentity( identity ) )
                {
                    throw std::invalid_argument( "GNS peer identity must be a recognized name" );
                }
                allowedPeerIdentities.push_back( identity );
            }
            if( config.maxClients == 0 || config.maxClients > 64 || config.eventsPerPoll == 0 ||
                config.eventsPerPoll > 256 )
            {
                throw std::invalid_argument( "Invalid GNS session limits" );
            }
            if( config.allowUnverifiedLoopback && config.requireAdmissionTickets )
            {
                throw std::invalid_argument( "GNS admission tickets require strict transport" );
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
            if( r.certificateOwner && r.certificateOwner != this )
            {
                throw std::logic_error( "A certificate provisioned GNS manager owns the runtime" );
            }
            if( !r.users )
            {
                SteamDatagramErrMsg message{};
                if( !GameNetworkingSockets_Init( nullptr, message ) )
                {
                    throw std::runtime_error( message );
                }
                SteamNetworkingSockets_CertStore_SetExpiryGraceSeconds( 0 );
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

        static bool isNamedIdentity( const SteamNetworkingIdentity &identity )
        {
            return !identity.IsInvalid() && identity.m_eType != k_ESteamNetworkingIdentityType_IPAddress &&
                   identity.m_eType != k_ESteamNetworkingIdentityType_UnknownType;
        }

        bool authorized( const SteamNetConnectionInfo_t &info ) const
        {
            return !( info.m_nFlags & k_nSteamNetworkConnectionInfoFlags_Unauthenticated ) &&
                   std::find( allowedPeerIdentities.begin(), allowedPeerIdentities.end(),
                              info.m_identityRemote ) != allowedPeerIdentities.end();
        }

        bool canAccept( const SteamNetConnectionInfo_t &info ) const
        {
            if( config.allowUnverifiedLoopback )
            {
                return info.m_addrRemote.IsLocalHost();
            }
            return authorized( info ) &&
                   std::none_of( peers.begin(), peers.end(), [&]( const auto &entry ) {
                       return entry.second.identity == info.m_identityRemote;
                   } );
        }

        size_t readyCount() const
        {
            return static_cast<size_t>( std::count_if( peers.begin(), peers.end(), []( const auto &peer ) {
                return peer.second.ready;
            } ) );
        }

        bool hasAdmissionCapacity() const
        {
            if( config.allowUnverifiedLoopback )
            {
                return peers.size() < config.maxClients;
            }
            const auto admitted = readyCount();
            return admitted < config.maxClients && peers.size() - admitted < MaxPendingAdmissions;
        }

        bool sendControl( HSteamNetConnection handle, const u8 *data, size_t size )
        {
            return sockets->SendMessageToConnection(
                       handle, data, static_cast<uint32>( size ),
                       k_nSteamNetworkingSend_Reliable | k_nSteamNetworkingSend_NoNagle, nullptr ) ==
                   k_EResultOK;
        }

        void rejectAdmission( HSteamNetConnection handle, const char *reason )
        {
            error = reason;
            close( handle, reason );
        }

        bool canAssignPlayer() const
        {
            return nextPlayer <= 65534 && readyCount() < config.maxClients && events.size() < 512;
        }

        bool assignPlayer( HSteamNetConnection handle, Peer &peer )
        {
            if( !canAssignPlayer() )
            {
                rejectAdmission( handle, ConfirmationRejected );
                return false;
            }
            const auto id = static_cast<u16>( nextPlayer );
            const u8 assignment[] = { AssignmentFrame, ControlVersion, static_cast<u8>( id ),
                                      static_cast<u8>( id >> 8 ) };
            if( !sendControl( handle, assignment, sizeof( assignment ) ) )
            {
                rejectAdmission( handle, ConfirmationRejected );
                return false;
            }
            ++nextPlayer;
            peer.player = id;
            peer.ready = true;
            notify( true, id );
            return true;
        }

        void sendChallenge( HSteamNetConnection handle, Peer &peer )
        {
            try
            {
                peer.challenge = Tickets::randomToken();
            }
            catch( const std::runtime_error & )
            {
                rejectAdmission( handle, ConfirmationRejected );
                return;
            }
            std::array<u8, ChallengeSize> frame{ ChallengeFrame, ControlVersion,
                                                config.requireAdmissionTickets ? TicketRequired : u8{ 0 } };
            std::copy( peer.challenge.begin(), peer.challenge.end(), frame.begin() + 3 );
            if( !sendControl( handle, frame.data(), frame.size() ) )
            {
                rejectAdmission( handle, ConfirmationRejected );
            }
        }

        void clearClientTicket()
        {
            if( clientTicket )
            {
                OPENSSL_cleanse( clientTicket->data(), clientTicket->size() );
                clientTicket.reset();
            }
        }

        void answerChallenge( HSteamNetConnection handle, Peer &peer, const u8 *data, size_t size,
                              bool reliable )
        {
            if( !reliable || peer.confirmationSent || size != ChallengeSize ||
                data[1] != ControlVersion || data[2] > TicketRequired )
            {
                rejectAdmission( handle, ConfirmationRejected );
                return;
            }
            const bool needsTicket = data[2] == TicketRequired;
            if( needsTicket != clientTicket.has_value() )
            {
                rejectAdmission( handle, TicketRejected );
                return;
            }

            std::array<u8, ConfirmationSize + SecretSize> frame{ ConfirmationFrame, ControlVersion };
            std::copy_n( data + 3, SecretSize, frame.begin() + 2 );
            if( needsTicket )
            {
                std::copy( clientTicket->begin(), clientTicket->end(), frame.begin() + ConfirmationSize );
            }
            const bool sent = sendControl( handle, frame.data(), ConfirmationSize + ( needsTicket ? SecretSize : 0 ) );
            if( needsTicket )
            {
                OPENSSL_cleanse( frame.data() + ConfirmationSize, SecretSize );
                clearClientTicket();
            }
            if( !sent )
            {
                rejectAdmission( handle, ConfirmationRejected );
                return;
            }
            peer.confirmationSent = true;
        }

        bool acceptConfirmation( HSteamNetConnection handle, Peer &peer, const u8 *data, size_t size,
                                 bool reliable )
        {
            const auto expectedSize = ConfirmationSize + ( config.requireAdmissionTickets ? SecretSize : 0 );
            if( !peer.transportConnected || !reliable || data[0] != ConfirmationFrame ||
                size != expectedSize || data[1] != ControlVersion ||
                CRYPTO_memcmp( peer.challenge.data(), data + 2, SecretSize ) != 0 ||
                !canAssignPlayer() )
            {
                rejectAdmission( handle, ConfirmationRejected );
                return false;
            }
            if( config.requireAdmissionTickets )
            {
                AdmissionTicket token;
                std::copy_n( data + ConfirmationSize, SecretSize, token.begin() );
                char identity[SteamNetworkingIdentity::k_cchMaxString];
                peer.identity.ToString( identity, sizeof( identity ) );
                const bool accepted = tickets.consume( token, identity );
                OPENSSL_cleanse( token.data(), token.size() );
                if( !accepted )
                {
                    rejectAdmission( handle, TicketRejected );
                    return false;
                }
            }
            return assignPlayer( handle, peer );
        }

        void expireAdmissions()
        {
            const auto now = Clock::now();
            for( auto it = peers.begin(); it != peers.end(); )
            {
                const auto &peer = it->second;
                const auto handle = ( it++ )->first;
                if( !peer.ready && admissionExpired( peer, now ) )
                {
                    rejectAdmission( handle, ConfirmationTimedOut );
                }
            }
        }

        static bool admissionExpired( const Peer &peer, Clock::time_point now )
        {
            return peer.admissionDeadline != Clock::time_point{} && now >= peer.admissionDeadline;
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
                if( s.fatal || s.events.size() >= 512 || !s.hasAdmissionCapacity() ||
                    s.nextPlayer > 65534 || !s.canAccept( event->m_info ) )
                {
                    s.sockets->CloseConnection( handle, 1001, "Admission rejected", false );
                    return;
                }
                Peer peer;
                if( !s.config.allowUnverifiedLoopback )
                {
                    peer.admissionDeadline = Clock::now() + AdmissionTimeout;
                }
                peer.identity = event->m_info.m_identityRemote;
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
                if( peer == s.peers.end() || peer->second.transportConnected )
                {
                    return;
                }
                // The inbound Connecting callback already verifies the remote cert. The
                // negotiated cipher is available only after the SDK finishes accepting it.
                if( !s.config.allowUnverifiedLoopback &&
                    ( !s.authorized( event->m_info ) ||
                      ( event->m_info.m_nFlags & k_nSteamNetworkConnectionInfoFlags_Unencrypted ) ||
                      ( s.server && !( peer->second.identity == event->m_info.m_identityRemote ) ) ) )
                {
                    s.error = "GNS peer did not satisfy strict admission";
                    s.close( handle );
                    return;
                }
                peer->second.transportConnected = true;
                if( s.config.allowUnverifiedLoopback )
                {
                    if( s.server )
                    {
                        s.assignPlayer( handle, peer->second );
                    }
                }
                else if( s.server )
                {
                    s.sendChallenge( handle, peer->second );
                }
                else
                {
                    peer->second.admissionDeadline = Clock::now() + AdmissionTimeout;
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

        void close( HSteamNetConnection handle, const char *reason = nullptr )
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
            sockets->CloseConnection( handle, reason ? 1001 : 0, reason, false );
            runtime().connections.erase( handle );
            peers.erase( p );
            if( !server )
            {
                clearClientTicket();
                local = 0xffffu;
            }
        }

        void stop()
        {
            tickets.clear();
            clearClientTicket();
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
            installedCertificate = false;
            auto &r = runtime();
            if( r.certificateOwner == this )
            {
                r.certificateOwner = nullptr;
            }
            if( --r.users == 0 )
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
            if( ( asServer && clientTicket ) || ( !asServer && config.requireAdmissionTickets ) )
            {
                throw std::logic_error( "GNS admission ticket configuration does not match the session role" );
            }
            if( !config.allowUnverifiedLoopback )
            {
                if( allowedPeerIdentities.empty() || !installedCertificate ||
                    sockets->GetAuthenticationStatus( nullptr ) != k_ESteamNetworkingAvailability_Current )
                {
                    throw std::logic_error(
                        "GNS strict mode requires a provisioned certificate and explicit peer "
                        "identities" );
                }
            }
            acquire();
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
            std::vector<u8> bytes{ GameplayFrame };
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
        return static_cast<u32>( s.readyCount() );
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
            if( p.ready && p.player == id )
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
            if( p.ready && p.player == id )
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
        s.error.clear();
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
        s.clearClientTicket();
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
        auto &runtime = Impl::runtime();
        if( s.config.allowUnverifiedLoopback || s.started ||
            runtime.users != ( s.sockets ? 1u : 0u ) )
        {
            throw std::logic_error( "Certificate setup requires one stopped strict manager" );
        }
        s.acquire();
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
        runtime.certificateOwner = &s;
        return bytes;
    }

    void WPNetworkGNSManager::installCertificate( const std::vector<u8> &bytes )
    {
        GNS_LOCK;
        if( s.started || Impl::runtime().certificateOwner != &s )
        {
            throw std::logic_error( "Request a certificate from this stopped manager before importing it" );
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
        SteamNetworkingIdentity identity{};
        s.installedCertificate = s.sockets->GetIdentity( &identity ) && Impl::isNamedIdentity( identity );
        if( !s.installedCertificate )
        {
            throw std::runtime_error( "GNS certificate requires a named local identity" );
        }
    }

    WPNetworkGNSManager::AdmissionTicket WPNetworkGNSManager::issueAdmissionTicket(
        const String &peerIdentity, u32 lifetimeSeconds )
    {
        GNS_LOCK;
        if( !s.started || !s.server || !s.config.requireAdmissionTickets )
        {
            throw std::logic_error( "Admission ticket issuance requires a running ticket-required GNS server" );
        }
        SteamNetworkingIdentity identity{};
        if( peerIdentity.find( '\0' ) != String::npos || !identity.ParseString( peerIdentity.c_str() ) ||
            std::find( s.allowedPeerIdentities.begin(), s.allowedPeerIdentities.end(), identity ) ==
                s.allowedPeerIdentities.end() )
        {
            throw std::invalid_argument( "Admission ticket identity must be allowlisted" );
        }
        char canonical[SteamNetworkingIdentity::k_cchMaxString];
        identity.ToString( canonical, sizeof( canonical ) );
        return s.tickets.issue( canonical, std::chrono::seconds( lifetimeSeconds ) );
    }

    void WPNetworkGNSManager::setAdmissionTicket( const AdmissionTicket &ticket )
    {
        GNS_LOCK;
        if( s.config.allowUnverifiedLoopback || s.config.requireAdmissionTickets ||
            ( s.started && s.server ) || !s.peers.empty() )
        {
            throw std::logic_error( "Admission tickets must be staged before a strict client connects" );
        }
        if( s.allowedPeerIdentities.size() != 1 )
        {
            throw std::logic_error( "Ticket clients must allowlist exactly one server identity" );
        }
        s.clearClientTicket();
        s.clientTicket = ticket;
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
        if( !s.config.allowUnverifiedLoopback )
        {
            s.expireAdmissions();
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
        // batch. Do not process a challenge or assignment before the client's identity check.
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

            auto peer = s.peers.find( raw->m_conn );
            if( peer == s.peers.end() )
            {
                continue;
            }
            const bool strict = !s.config.allowUnverifiedLoopback;
            if( raw->m_cbSize < 1 || raw->m_cbSize > static_cast<int>( Impl::MaxPayload + 1 ) )
            {
                if( strict && !peer->second.ready )
                {
                    s.rejectAdmission( raw->m_conn, Impl::ConfirmationRejected );
                }
                else
                {
                    s.error = "Invalid GNS frame size";
                }
                continue;
            }
            const auto size = static_cast<size_t>( raw->m_cbSize );
            bytes += size;
            const auto *data = static_cast<const u8 *>( raw->m_pData );
            const bool reliable = ( raw->m_nFlags & k_nSteamNetworkingSend_Reliable ) != 0;
            if( strict && !peer->second.ready )
            {
                if( Impl::admissionExpired( peer->second, Impl::Clock::now() ) )
                {
                    s.rejectAdmission( raw->m_conn, Impl::ConfirmationTimedOut );
                    continue;
                }
                if( s.server )
                {
                    if( s.acceptConfirmation( raw->m_conn, peer->second, data, size, reliable ) )
                    {
                        // Publish the join before delivering gameplay queued on this connection.
                        break;
                    }
                    continue;
                }
                if( data[0] == Impl::ChallengeFrame )
                {
                    s.answerChallenge( raw->m_conn, peer->second, data, size, reliable );
                    continue;
                }
            }
            if( data[0] == Impl::AssignmentFrame && !s.server && !peer->second.ready && reliable &&
                ( !strict || peer->second.confirmationSent ) && size == 4 && data[1] == Impl::ControlVersion )
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
            else if( data[0] == Impl::GameplayFrame && peer->second.ready )
            {
                auto packet = SmartPtr<WPNetworkPacket>( new WPNetworkPacket() );
                packet->setData( data + 1, raw->m_cbSize - 1 );
                packet->setSystemAddress( peer->second.endpoint );
                raw->Release();
                release.p = nullptr;
                s.listener->handlePacket( packet );
            }
            else if( data[0] == Impl::GameplayFrame && !s.server && !reliable &&
                     ( !strict || peer->second.confirmationSent ) )
            {
                // Unreliable gameplay data can overtake the reliable player assignment.
                continue;
            }
            else
            {
                s.rejectAdmission( raw->m_conn, strict && !peer->second.ready ? Impl::ConfirmationRejected
                                                                              : "Unexpected GNS session frame" );
            }
        }
    }

#    undef GNS_LOCK
}  // namespace workphone
#endif
