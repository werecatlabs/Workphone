#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <vector>

namespace
{
    using Clock = std::chrono::steady_clock;

    void check( bool condition, const char *message )
    {
        if( !condition )
        {
            throw std::runtime_error( message );
        }
    }

    struct Runtime
    {
        Runtime()
        {
            SteamDatagramErrMsg error{};
            if( !GameNetworkingSockets_Init( nullptr, error ) )
            {
                throw std::runtime_error( error );
            }
            SteamNetworkingUtils()->SetDebugOutputFunction(
                k_ESteamNetworkingSocketsDebugOutputType_Warning,
                []( ESteamNetworkingSocketsDebugOutputType, const char *text ) {
                    std::fprintf( stderr, "GNS: %s\n", text );
                } );
        }

        ~Runtime()
        {
            GameNetworkingSockets_Kill();
        }
    };

    struct Pair
    {
        static Pair *active;
        ISteamNetworkingSockets *sockets = SteamNetworkingSockets();
        HSteamListenSocket listener = k_HSteamListenSocket_Invalid;
        HSteamNetConnection client = k_HSteamNetConnection_Invalid;
        HSteamNetConnection server = k_HSteamNetConnection_Invalid;
        bool connected = false, failed = false;
        int reason = 0;

        static void changed( SteamNetConnectionStatusChangedCallback_t *event )
        {
            if( !active )
                return;
            auto &pair = *active;
            if( pair.listener != k_HSteamListenSocket_Invalid &&
                event->m_info.m_hListenSocket == pair.listener &&
                event->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting )
            {
                pair.server = event->m_hConn;
                pair.sockets->AcceptConnection( event->m_hConn );
            }
            if( event->m_hConn == pair.client )
            {
                if( event->m_info.m_eState == k_ESteamNetworkingConnectionState_Connected )
                {
                    pair.connected = true;
                }
                if( event->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally ||
                    event->m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer )
                {
                    pair.failed = true;
                    pair.reason = event->m_info.m_eEndReason;
                    std::fprintf( stderr, "Connection ended (%d): %s\n", pair.reason, event->m_info.m_szEndDebug );
                }
            }
        }

        explicit Pair( bool requireIdentity )
        {
            active = this;
            SteamNetworkingConfigValue_t options[5];
            options[0].SetPtr( k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
                               reinterpret_cast<void *>( &changed ) );
            // The fixture server permits development identities. The strict
            // case below changes only the client's identity requirements.
            options[1].SetInt32( k_ESteamNetworkingConfig_IP_AllowWithoutAuth, 1 );
            options[2].SetInt32( k_ESteamNetworkingConfig_IPLocalHost_AllowWithoutAuth,
                                 1 );
            options[3].SetInt32( k_ESteamNetworkingConfig_Unencrypted, 0 );
            options[4].SetInt32( k_ESteamNetworkingConfig_TimeoutInitial, 1500 );
            SteamNetworkingIPAddr address;
            address.Clear();
            address.SetIPv6LocalHost();
            // GNS requires an explicit port; unlike native UDP, port zero does
            // not ask the OS for an ephemeral listener. Bind and retry directly
            // so a separate probe socket cannot race the actual listener.
            const auto first = static_cast<unsigned>( Clock::now().time_since_epoch().count() % 20000 );
            for( unsigned attempt = 0; attempt < 64 && listener == k_HSteamListenSocket_Invalid; ++attempt )
            {
                address.m_port = static_cast<uint16>( 30000 + (first + attempt) % 20000 );
                listener = sockets->CreateListenSocketIP( address, 5, options );
            }
            check( listener != k_HSteamListenSocket_Invalid, "Cannot bind IPv6 fixture listener" );
            if( !sockets->GetListenSocketAddress( listener, &address ) )
            {
                failed = true;
                return;
            }
            options[1].SetInt32( k_ESteamNetworkingConfig_IP_AllowWithoutAuth, requireIdentity ? 0 : 1 );
            options[2].SetInt32( k_ESteamNetworkingConfig_IPLocalHost_AllowWithoutAuth, requireIdentity ? 0 : 1 );
            client = sockets->ConnectByIPAddress( address, 5, options );
            if( client == k_HSteamNetConnection_Invalid )
            {
                failed = true;
            }
        }

        ~Pair()
        {
            if( client != k_HSteamNetConnection_Invalid )
            {
                sockets->CloseConnection( client, 0, nullptr, false );
            }
            if( server != k_HSteamNetConnection_Invalid )
            {
                sockets->CloseConnection( server, 0, nullptr, false );
            }
            if( listener != k_HSteamListenSocket_Invalid )
            {
                sockets->CloseListenSocket( listener );
            }
            // Deliver pending callbacks before revoking their owner.
            sockets->RunCallbacks();
            active = nullptr;
        }

        void pump()
        {
            sockets->RunCallbacks();
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
    };

    Pair *Pair::active = nullptr;

    void delivery()
    {
        Pair pair( false );  // Encryption required, peer identity deliberately unverified.
        const auto deadline = Clock::now() + std::chrono::seconds( 8 );
        while( !pair.connected && !pair.failed && Clock::now() < deadline )
        {
            pair.pump();
        }
        check( pair.connected && pair.server != k_HSteamNetConnection_Invalid,
               "IPv6 development connection failed" );

        struct LossScope
        {
            LossScope()
            {
                check( SteamNetworkingUtils()->SetGlobalConfigValueFloat(
                           k_ESteamNetworkingConfig_FakePacketLoss_Send, 15.0f ),
                       "Cannot configure packet loss" );
            }

            ~LossScope()
            {
                SteamNetworkingUtils()->SetGlobalConfigValueFloat(
                    k_ESteamNetworkingConfig_FakePacketLoss_Send, 0.0f );
            }
        } loss;

        for( std::uint32_t sequence = 0; sequence < 33; ++sequence )
        {
            std::vector<unsigned char> bytes( sequence == 32 ? 65536 : 1024,
                                              static_cast<unsigned char>( sequence ) );
            std::memcpy( bytes.data(), &sequence, sizeof( sequence ) );
            check( pair.sockets->SendMessageToConnection(
                       pair.client, bytes.data(), static_cast<uint32>( bytes.size() ),
                       k_nSteamNetworkingSend_Reliable, nullptr ) == k_EResultOK,
                   "Reliable message was not accepted" );
        }
        std::uint32_t received = 0;
        while( received < 33 && Clock::now() < deadline )
        {
            pair.pump();
            SteamNetworkingMessage_t *message = nullptr;
            const int count = pair.sockets->ReceiveMessagesOnConnection( pair.server, &message, 1 );
            check( count >= 0, "Receive failed" );
            if( count == 0 )
            {
                continue;
            }
            const auto expectedSize = received == 32 ? 65536 : 1024;
            bool valid = message->m_cbSize == expectedSize;
            std::uint32_t sequence = 0xffffffffu;
            if( message->m_cbSize >= 4 )
            {
                std::memcpy( &sequence, message->m_pData, 4 );
            }
            valid = valid && sequence == received;
            const auto *bytes = static_cast<const unsigned char *>( message->m_pData );
            for( int i = 4; i < message->m_cbSize; ++i )
            {
                valid = valid && bytes[i] == received;
            }
            message->Release();
            check( valid, "Reliable order, size or fragmented payload mismatch" );
            ++received;
        }
        check( received == 33, "Reliable delivery deadline expired" );
    }

    void rejectMissingIdentity()
    {
        Pair pair( true );
        SteamNetAuthenticationStatus_t authentication{};
        const auto availability = pair.sockets->GetAuthenticationStatus( &authentication );
        check( availability != k_ESteamNetworkingAvailability_Current,
               "Negative fixture unexpectedly has provisioned authentication" );
        const auto deadline = Clock::now() + std::chrono::seconds( 5 );
        while( !pair.failed && !pair.connected && Clock::now() < deadline )
        {
            pair.pump();
        }
        check( !pair.connected && pair.failed,
               "Strict policy admitted a peer without credentials or did not terminate" );
        std::printf( "Strict credentialless client failed closed (reason %d, auth availability %d).\n",
                     pair.reason, static_cast<int>( availability ) );
    }
}  // namespace

int main()
{
    try
    {
        Runtime runtime;
        delivery();
        rejectMissingIdentity();
        std::puts(
            "PASS: GNS IPv6, reliable ordering with 15% simulated send loss, 64KiB reassembly, teardown "
            "and credentialless client fail-closed policy." );
        std::puts(
            "Experimental conformance only: development delivery has no verified peer identity; not "
            "engine integration or release certification." );
        return 0;
    }
    catch( const std::exception &error )
    {
        std::fprintf( stderr, "FAIL: %s\n", error.what() );
        return 1;
    }
}
