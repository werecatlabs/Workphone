#include <steam/steamnetworkingsockets.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <process.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
    using Clock = std::chrono::steady_clock;
    constexpr std::size_t NonceSize = 32;
    using Nonce = std::array<unsigned char, NonceSize>;
    constexpr int ChallengeSize = 3 + static_cast<int>( NonceSize );
    constexpr int MaxCertificateSize = 65536;
    constexpr int AdmissionRejection = 1001;

    void require( bool condition, const char *message )
    {
        if( !condition )
        {
            throw std::runtime_error( message );
        }
    }

    unsigned integer( const std::string &text, unsigned minimum, unsigned maximum )
    {
        require( !text.empty() && text.find_first_not_of( "0123456789" ) == std::string::npos,
                 "Invalid proof fixture numeric argument" );
        const auto value = std::stoul( text );
        require( value >= minimum && value <= maximum, "Proof fixture argument out of range" );
        return static_cast<unsigned>( value );
    }

    struct Options
    {
        std::string proofMode;
        SteamNetworkingIdentity expectedServer{};
        uint16 port = 0;
        std::chrono::milliseconds timeout{ 12000 };
        std::filesystem::path request, certificate, result;
    };

    Options parse( int argc, char **argv )
    {
        std::map<std::string, std::string> args;
        for( int i = 1; i < argc; ++i )
        {
            const std::string key = argv[i];
            const bool flag = key == "--expect-rejection";
            require( flag || i + 1 < argc, "Proof fixture option requires a value" );
            require( args.emplace( key, flag ? "true" : argv[++i] ).second, "Duplicate proof fixture option" );
        }
        auto take = [&]( const char *name ) {
            const auto option = args.find( name );
            if( option == args.end() )
            {
                return std::string{};
            }
            auto value = option->second;
            args.erase( option );
            return value;
        };

        require( take( "--mode" ) == "client" && take( "--expect-rejection" ) == "true",
                 "The raw proof fixture is a rejection-only client" );
        Options options;
        options.proofMode = take( "--proof-mode" );
        require( options.proofMode == "withheld" || options.proofMode == "wrong-nonce" ||
                     options.proofMode == "early-gameplay" || options.proofMode == "replay",
                 "Unknown raw proof mode" );
        const auto peer = take( "--peer" );
        require( options.expectedServer.ParseString( peer.c_str() ) && !options.expectedServer.IsInvalid() &&
                     options.expectedServer.m_eType != k_ESteamNetworkingIdentityType_IPAddress,
                 "Expected a named server identity" );
        options.port = static_cast<uint16>( integer( take( "--port" ), 1, 65535 ) );
        const auto timeout = take( "--timeout-ms" );
        if( !timeout.empty() )
        {
            options.timeout = std::chrono::milliseconds( integer( timeout, 1000, 30000 ) );
        }
        options.request = take( "--request" );
        options.certificate = take( "--certificate" );
        options.result = take( "--result" );
        require( args.empty() && !options.request.empty() && !options.certificate.empty() &&
                     !options.result.empty() && !std::filesystem::exists( options.certificate ),
                 "Expected unused request, certificate and result paths" );
        return options;
    }

    void publish( const std::filesystem::path &path, const char *data, std::size_t size )
    {
        auto temporary = path;
        temporary += ".tmp";
        require( !std::filesystem::exists( path ) && !std::filesystem::exists( temporary ),
                 "Proof fixture output path already exists" );
        std::ofstream output( temporary, std::ios::binary );
        output.write( data, static_cast<std::streamsize>( size ) );
        output.close();
        require( static_cast<bool>( output ), "Could not write proof fixture output" );
        std::filesystem::rename( temporary, path );
    }

    void installCertificate( ISteamNetworkingSockets &sockets, const Options &options )
    {
        int size = 0;
        SteamNetworkingErrMsg error{};
        require( sockets.GetCertificateRequest( &size, nullptr, error ) && size > 0 && size <= MaxCertificateSize,
                 "Could not size raw proof client certificate request" );
        std::vector<char> request( static_cast<std::size_t>( size ) );
        require( sockets.GetCertificateRequest( &size, request.data(), error ) && size > 0 &&
                     static_cast<std::size_t>( size ) <= request.size(),
                 "Could not create raw proof client certificate request" );
        publish( options.request, request.data(), static_cast<std::size_t>( size ) );

        const auto deadline = Clock::now() + options.timeout;
        std::ifstream input;
        while( Clock::now() < deadline )
        {
            // An atomic rename can expose the path while Windows still holds the file open.
            input.open( options.certificate, std::ios::binary | std::ios::ate );
            if( input.is_open() )
            {
                break;
            }
            std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
        }
        require( input.is_open(), "No readable certificate arrived for raw proof client" );
        const auto certificateSize = input.tellg();
        require( certificateSize > 0 && certificateSize <= static_cast<std::streamoff>( MaxCertificateSize ),
                 "Invalid raw proof client certificate size" );
        std::vector<char> certificate( static_cast<std::size_t>( certificateSize ) );
        input.seekg( 0 );
        require( static_cast<bool>( input.read( certificate.data(), static_cast<std::streamsize>( certificateSize ) ) ),
                 "Could not read raw proof client certificate" );
        require( sockets.SetCertificate( certificate.data(), static_cast<int>( certificate.size() ), error ) &&
                     sockets.GetAuthenticationStatus( nullptr ) == k_ESteamNetworkingAvailability_Current,
                 "Raw proof client could not install its trusted certificate" );
    }

    void attack( ISteamNetworkingSockets &sockets, HSteamNetConnection connection,
                 const Nonce &challenge, const Nonce *replay, const std::string &mode )
    {
        if( mode == "withheld" )
        {
            return;
        }
        if( mode == "early-gameplay" )
        {
            // A reliable gameplay envelope with the ordinary trust fixture's probe payload.
            const std::array<unsigned char, 7> gameplay{ 1, 0x51, 0x54, 0x50, 0x57, 1, 0 };
            require( sockets.SendMessageToConnection( connection, gameplay.data(), static_cast<uint32>( gameplay.size() ),
                                                       k_nSteamNetworkingSend_ReliableNoNagle, nullptr ) == k_EResultOK,
                     "Could not send pre-admission gameplay probe" );
            return;
        }
        std::array<unsigned char, 2 + NonceSize> proof{ 3, 1 };
        const auto &answer = replay ? *replay : challenge;
        std::copy( answer.begin(), answer.end(), proof.begin() + 2 );
        if( !replay )
        {
            proof[2] ^= 1;
        }
        require( sockets.SendMessageToConnection( connection, proof.data(), static_cast<uint32>( proof.size() ),
                                                   k_nSteamNetworkingSend_ReliableNoNagle, nullptr ) == k_EResultOK,
                 "Could not send mismatched admission nonce" );
    }

    struct Runtime
    {
        Runtime()
        {
            SteamDatagramErrMsg error{};
            require( GameNetworkingSockets_Init( nullptr, error ), "Could not initialize raw GNS proof client" );
        }

        ~Runtime()
        {
            GameNetworkingSockets_Kill();
        }
    };

    struct Connection
    {
        ISteamNetworkingSockets &sockets;
        HSteamNetConnection handle;

        ~Connection()
        {
            if( handle != k_HSteamNetConnection_Invalid )
            {
                sockets.CloseConnection( handle, 0, nullptr, false );
            }
        }
    };

    Nonce runConnection( ISteamNetworkingSockets &sockets, const Options &options, const Nonce *replay )
    {
        SteamNetworkingConfigValue_t settings[4];
        settings[0].SetInt32( k_ESteamNetworkingConfig_IP_AllowWithoutAuth, 0 );
        settings[1].SetInt32( k_ESteamNetworkingConfig_IPLocalHost_AllowWithoutAuth, 0 );
        settings[2].SetInt32( k_ESteamNetworkingConfig_Unencrypted, 0 );
        settings[3].SetInt32( k_ESteamNetworkingConfig_TimeoutInitial, 5000 );
        SteamNetworkingIPAddr endpoint{};
        endpoint.SetIPv4( 0x7f000001u, options.port );
        const Connection connection{ sockets, sockets.ConnectByIPAddress( endpoint, 4, settings ) };
        require( connection.handle != k_HSteamNetConnection_Invalid, "Could not dial proof fixture server" );

        bool authenticated = false;
        bool challengeReceived = false;
        Nonce challenge{};
        const auto deadline = Clock::now() + options.timeout;
        while( Clock::now() < deadline )
        {
            sockets.RunCallbacks();
            SteamNetConnectionInfo_t info{};
            require( sockets.GetConnectionInfo( connection.handle, &info ), "Lost raw proof connection handle" );
            if( info.m_eState == k_ESteamNetworkingConnectionState_Connected )
            {
                require( info.m_identityRemote == options.expectedServer &&
                             !( info.m_nFlags & ( k_nSteamNetworkConnectionInfoFlags_Unauthenticated |
                                                  k_nSteamNetworkConnectionInfoFlags_Unencrypted ) ),
                         "Proof fixture failed to establish an authenticated encrypted transport" );
                authenticated = true;
            }

            SteamNetworkingMessage_t *received = nullptr;
            const auto count = authenticated ? sockets.ReceiveMessagesOnConnection( connection.handle, &received, 1 ) : 0;
            require( count >= 0, "Raw proof client receive failed" );
            if( count == 1 )
            {
                const auto release = []( SteamNetworkingMessage_t *message ) { message->Release(); };
                const std::unique_ptr<SteamNetworkingMessage_t, decltype( release )> message( received, release );
                const auto *bytes = static_cast<const unsigned char *>( message->m_pData );
                require( !challengeReceived && message->m_cbSize == ChallengeSize &&
                             ( message->m_nFlags & k_nSteamNetworkingSend_Reliable ) &&
                             bytes[0] == 2 && bytes[1] == 1 && bytes[2] == 0,
                         "Received gameplay, a player assignment, or an invalid challenge before proof" );
                challengeReceived = true;
                std::copy_n( bytes + 3, NonceSize, challenge.begin() );
                require( !replay || *replay != challenge, "Server reused its challenge on a new connection" );
                attack( sockets, connection.handle, challenge, replay, options.proofMode );
                continue;
            }

            if( info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer )
            {
                const std::string expectedReason = options.proofMode == "withheld"
                                                       ? "Admission confirmation timed out"
                                                       : "Admission confirmation rejected";
                require( challengeReceived && info.m_eEndReason == AdmissionRejection &&
                             info.m_szEndDebug == expectedReason,
                         "Server closure did not demonstrate the expected proof rejection" );
                return challenge;
            }
            require( info.m_eState != k_ESteamNetworkingConnectionState_ProblemDetectedLocally,
                     "Local transport failure cannot demonstrate remote proof rejection" );
            std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
        }
        throw std::runtime_error( "Raw proof client timed out without a remote rejection" );
    }

    void run( const Options &options )
    {
        Runtime runtime;
        auto &sockets = *SteamNetworkingSockets();
        installCertificate( sockets, options );
        const auto firstChallenge = runConnection( sockets, options, nullptr );
        if( options.proofMode == "replay" )
        {
            // The first connection was rejected after a deliberately incorrect reply.
            // Replay its original challenge on a second connection with the same certificate.
            runConnection( sockets, options, &firstChallenge );
        }
    }
}  // namespace

int main( int argc, char **argv )
{
    try
    {
        const auto options = parse( argc, argv );
        run( options );
        const auto report = "{\"version\":1,\"role\":\"client\",\"pid\":" + std::to_string( _getpid() ) +
                            ",\"outcome\":\"rejected\",\"stage\":\"handshake\",\"admissions\":0,\"packets\":0"
                            ",\"disconnects\":0,\"peerId\":65535,\"closed\":true,\"challengeReceived\":true"
                            ",\"remoteClosure\":true,\"proofMode\":\"" + options.proofMode + '"' +
                            ( options.proofMode == "replay" ? ",\"freshChallengeObserved\":true}" : "}" );
        publish( options.result, report.data(), report.size() );
        return 0;
    }
    catch( const std::exception &error )
    {
        std::fprintf( stderr, "FAIL: %s\n", error.what() );
        return 1;
    }
}
