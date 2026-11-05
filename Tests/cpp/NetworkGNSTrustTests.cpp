#include <WPNetwork/WPNetworkGNSManager.hpp>
#include <Workphone/Interface/Net/INetworkListener.hpp>
#include <Workphone/Interface/Net/IPacket.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <process.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace workphone;

namespace
{
    using Clock = std::chrono::steady_clock;
    using Status = INetworkManager::ConnectionStatus;
    constexpr std::size_t MaxCertificateBytes = 65536;
    constexpr u32 Request = 0x57505451u;
    constexpr u32 Response = 0x57505452u;

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
                 "Invalid numeric argument" );
        const auto value = std::stoul( text );
        require( value >= minimum && value <= maximum, "Numeric argument out of range" );
        return static_cast<unsigned>( value );
    }

    struct Options
    {
        bool server = false;
        bool expectRejection = false;
        bool withoutCertificate = false;
        bool unverifiedClient = false;
        u16 port = 0;
        std::chrono::milliseconds timeout{ 10000 };
        std::string peer;
        std::filesystem::path request, certificate, ready, connectGate, result;
    };

    Options parse( int argc, char **argv )
    {
        std::map<std::string, std::string> args;
        for( int i = 1; i < argc; ++i )
        {
            const std::string key = argv[i];
            const bool flag = key == "--expect-rejection" || key == "--without-certificate" ||
                              key == "--unverified-client";
            require( flag || i + 1 < argc, "Option requires a value" );
            require( args.emplace( key, flag ? "true" : argv[++i] ).second, "Duplicate option" );
        }
        auto take = [&]( const char *name ) {
            auto it = args.find( name );
            if( it == args.end() )
            {
                return std::string{};
            }
            auto value = it->second;
            args.erase( it );
            return value;
        };
        Options result;
        const auto mode = take( "--mode" );
        require( mode == "server" || mode == "client", "Expected --mode server or client" );
        result.server = mode == "server";
        result.expectRejection = !take( "--expect-rejection" ).empty();
        result.withoutCertificate = !take( "--without-certificate" ).empty();
        result.unverifiedClient = !take( "--unverified-client" ).empty();
        require( !( result.withoutCertificate && result.unverifiedClient ),
                 "Certificate test modes are mutually exclusive" );
        if( result.withoutCertificate || result.unverifiedClient )
        {
            require( !result.server && result.expectRejection,
                     "Credentialless mode is a client rejection test" );
        }
        result.port = static_cast<u16>( integer( take( "--port" ), 1, 65535 ) );
        const auto timeout = take( "--timeout-ms" );
        if( !timeout.empty() )
        {
            result.timeout = std::chrono::milliseconds( integer( timeout, 1000, 30000 ) );
        }
        result.peer = take( "--peer" );
        result.result = take( "--result" );
        result.ready = take( "--ready" );
        result.connectGate = take( "--connect-gate" );
        result.request = take( "--request" );
        result.certificate = take( "--certificate" );
        require( args.empty() && !result.peer.empty() && !result.result.empty(),
                 "Expected --peer and --result, with no unknown options" );
        const bool gated = !result.connectGate.empty();
        require( ( result.server || gated ) == !result.ready.empty(),
                 "Server and gated client modes require --ready" );
        if( gated )
        {
            require( !result.server && !result.withoutCertificate && !result.unverifiedClient,
                     "Only a certificate-provisioned client can wait for --connect-gate" );
            require( !std::filesystem::exists( result.connectGate ),
                     "The connect gate must be unused before provisioning" );
        }
        if( result.withoutCertificate || result.unverifiedClient )
        {
            require( result.request.empty() && result.certificate.empty(),
                     "Credentialless mode takes no certificate paths" );
        }
        else
        {
            require( !result.request.empty() && !result.certificate.empty(),
                     "Expected --request and --certificate" );
            require( !std::filesystem::exists( result.certificate ),
                     "The certificate path must be unused before the fixture issues its request" );
        }
        return result;
    }

    void publish( const std::filesystem::path &path, const char *data, std::size_t size )
    {
        auto temporary = path;
        temporary += ".tmp";
        require( !std::filesystem::exists( path ) && !std::filesystem::exists( temporary ),
                 "Fixture output path is already in use" );
        std::ofstream output( temporary, std::ios::binary );
        output.write( data, static_cast<std::streamsize>( size ) );
        output.close();
        require( static_cast<bool>( output ), "Could not write fixture output" );
        std::filesystem::rename( temporary, path );
    }

    void publish( const std::filesystem::path &path, const std::string &text )
    {
        publish( path, text.data(), text.size() );
    }

    void waitForFile( const std::filesystem::path &path, std::chrono::milliseconds timeout,
                      const char *failure )
    {
        const auto deadline = Clock::now() + timeout;
        while( !std::filesystem::exists( path ) && Clock::now() < deadline )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
        }
        require( std::filesystem::is_regular_file( path ), failure );
    }

    std::vector<u8> getCertificate( WPNetworkGNSManager &manager, const Options &options )
    {
        const auto request = manager.getCertificateRequest();
        require( !request.empty() && request.size() <= MaxCertificateBytes,
                 "GNS returned an invalid certificate request" );
        publish( options.request, reinterpret_cast<const char *>( request.data() ), request.size() );
        waitForFile( options.certificate, options.timeout, "No issued certificate arrived" );
        std::ifstream input( options.certificate, std::ios::binary | std::ios::ate );
        require( static_cast<bool>( input ), "No issued certificate arrived" );
        const auto size = input.tellg();
        require( size > 0 && size <= static_cast<std::streamoff>( MaxCertificateBytes ),
                 "Issued certificate has invalid size" );
        std::vector<u8> certificate( static_cast<std::size_t>( size ) );
        input.seekg( 0 );
        require( static_cast<bool>( input.read( reinterpret_cast<char *>( certificate.data() ),
                                                static_cast<std::streamsize>( size ) ) ),
                 "Could not read issued certificate" );
        return certificate;
    }

    struct Recorder final : INetworkListener
    {
        Recorder( WPNetworkGNSManager &value, bool isServer ) : manager( value ), server( isServer )
        {
        }

        void connect( u32 player ) override
        {
            ++admissions;
            peer = player;
            if( admissions != 1 || ( server ? player == 0 || player >= 65535 : player != 0 ) )
            {
                failure = "Unexpected peer admission";
            }
        }

        void disconnect( u32 player ) override
        {
            ++disconnects;
            if( admissions != 1 || disconnects != 1 || player != peer )
            {
                failure = "Unexpected peer disconnect";
            }
        }

        void handlePacket( SmartPtr<IPacket> packet ) override
        {
            ++packets;
            try
            {
                require( admissions == 1 && packets == 1 && packet && packet->getDataLength() == 6,
                         "Unexpected gameplay probe" );
                const auto sender = manager.getPacketSenderId( packet );
                require( sender == static_cast<s32>( peer ),
                         "Gameplay probe sender was not the admitted peer" );
                u32 message = 0;
                u16 client = 0;
                packet->read( message );
                packet->read( client );
                require( message == ( server ? Request : Response ) &&
                             client == ( server ? peer : manager.getPlayerNumber() ),
                         "Gameplay probe did not match the assigned client" );
            }
            catch( const std::exception &error )
            {
                failure = error.what();
            }
        }

        WPNetworkGNSManager &manager;
        bool server;
        unsigned admissions = 0, disconnects = 0, packets = 0;
        u32 peer = 65535;
        std::string failure;
    };

    void sendProbe( WPNetworkGNSManager &manager, u32 message, u16 client, u16 destination )
    {
        auto packet = manager.createPacket();
        packet->write( message );
        packet->write( client );
        manager.sendPacket( packet, destination );
    }

    const char *runSession( WPNetworkGNSManager &manager, Recorder &recorder, const Options &options )
    {
        if( options.server )
        {
            manager.setServer( true );
            require( manager.getBoundAddress() == "127.0.0.1:" + std::to_string( options.port ),
                     "Server did not bind the requested loopback endpoint" );
            publish( options.ready, std::to_string( options.port ) );
        }
        else
        {
            try
            {
                manager.connect( "127.0.0.1", options.port, "" );
            }
            catch( const std::logic_error &error )
            {
                if( !options.withoutCertificate &&
                    ( options.connectGate.empty() || !options.expectRejection ) )
                {
                    throw;
                }
                std::fprintf( stdout, "Strict startup rejected: %s\n", error.what() );
                require( manager.getConnectionStatus() == Status::NCS_FAILED,
                         "Rejected strict client entered a connecting state" );
                return "startup";
            }
            require( !options.withoutCertificate, "Credentialless strict client started" );
        }

        bool sent = false;
        const auto deadline = Clock::now() + options.timeout;
        while( Clock::now() < deadline )
        {
            manager.poll();
            require( recorder.failure.empty(), recorder.failure.c_str() );
            if( options.expectRejection )
            {
                require( recorder.admissions == 0 && recorder.packets == 0 && !manager.getPeerCount(),
                         "Rejected case admitted a peer or delivered gameplay" );
                if( !options.server && manager.getConnectionStatus() == Status::NCS_FAILED )
                {
                    const auto error = manager.getLastError();
                    std::fprintf( stdout, "Connection rejected: %s\n", error.c_str() );
                    return "handshake";
                }
            }
            else if( options.server )
            {
                if( recorder.packets == 1 && !sent )
                {
                    const auto client = static_cast<u16>( recorder.peer );
                    sendProbe( manager, Response, client, client );
                    sent = true;
                }
                if( sent && recorder.disconnects == 1 )
                {
                    return "gameplay";
                }
            }
            else
            {
                const auto status = manager.getConnectionStatus();
                require( status != Status::NCS_FAILED, "Trusted client connection failed" );
                if( !sent && recorder.admissions == 1 && status == Status::NCS_ESTABLISHED )
                {
                    sendProbe( manager, Request, manager.getPlayerNumber(), 0 );
                    sent = true;
                }
                if( sent && recorder.packets == 1 )
                {
                    manager.disconnect();
                    manager.poll();
                    require( recorder.failure.empty() && recorder.disconnects == 1,
                             "Trusted client did not disconnect cleanly" );
                    return "gameplay";
                }
            }
            std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
        }
        if( options.server && options.expectRejection )
        {
            return "observation";
        }
        throw std::runtime_error( "Trust fixture session timed out" );
    }

    void run( const Options &options )
    {
        WPNetworkGNSConfig config;
        config.bindAddress = "127.0.0.1:" + std::to_string( options.port );
        config.maxClients = 1;
        config.allowUnverifiedLoopback = options.unverifiedClient;
        config.allowedPeerIdentities.push_back( options.peer );
        auto manager = make_ptr<WPNetworkGNSManager>( config );
        auto recorder = make_ptr<Recorder>( *manager, options.server );
        manager->addListener( recorder );
        const char *stage = nullptr;
        if( !options.withoutCertificate && !options.unverifiedClient )
        {
            const auto certificate = getCertificate( *manager, options );
            try
            {
                manager->installCertificate( certificate );
            }
            catch( const std::runtime_error &error )
            {
                if( options.server || !options.expectRejection )
                {
                    throw;
                }
                std::fprintf( stdout, "Certificate import rejected: %s\n", error.what() );
                stage = "certificate";
            }
        }
        if( !stage )
        {
            if( !options.connectGate.empty() )
            {
                publish( options.ready, "certificate-installed" );
                waitForFile( options.connectGate, options.timeout, "Connect gate did not open" );
            }
            stage = runSession( *manager, *recorder, options );
        }
        manager->unload( nullptr );
        require( manager->getBoundAddress().empty() && manager->getPlayerNumber() == 65535 &&
                     !manager->getPeerCount(),
                 "Fixture transport remained active after teardown" );
        const std::string role = options.server ? "server" : "client";
        const std::string outcome = options.expectRejection ? "rejected" : "accepted";
        const auto report =
            "{\"version\":1,\"role\":\"" + role + "\",\"pid\":" + std::to_string( _getpid() ) +
            ",\"outcome\":\"" + outcome + "\",\"stage\":\"" + stage +
            "\",\"admissions\":" + std::to_string( recorder->admissions ) +
            ",\"packets\":" + std::to_string( recorder->packets ) +
            ",\"disconnects\":" + std::to_string( recorder->disconnects ) +
            ",\"peerId\":" + std::to_string( recorder->peer ) + ",\"closed\":true}";
        publish( options.result, report );
    }
}  // namespace

int main( int argc, char **argv )
{
    TypeManager types;
    types.load();
    TypeManager::setInstance( &types );
    auto application = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance( application );
    int result = 1;
    try
    {
        run( parse( argc, argv ) );
        result = 0;
    }
    catch( const std::exception &error )
    {
        std::fprintf( stderr, "FAIL: %s\n", error.what() );
    }
    core::IApplicationManager::setInstance( nullptr );
    application = nullptr;
    TypeManager::setInstance( nullptr );
    types.unload();
    return result;
}
