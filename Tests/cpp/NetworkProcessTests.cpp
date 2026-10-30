#include <WPNetwork/WPNetworkManager.hpp>
#include <Workphone/Interface/Net/INetworkListener.hpp>
#include <Workphone/Interface/Net/IPacket.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#if defined( _WIN32 )
#    include <process.h>
#else
#    include <unistd.h>
#endif

// A loopback development fixture using separate OS processes and the concrete
// manager. Retries below are application test traffic, not reliable transport.
using namespace workphone;

namespace
{
    constexpr u32 Magic = 0x4c4e5057u;
    enum class Message : u8
    {
        Hello = 1,
        HelloAck,
        Finished,
        FinishedAck
    };
    using Clock = std::chrono::steady_clock;

    unsigned processId()
    {
#if defined( _WIN32 )
        return static_cast<unsigned>( _getpid() );
#else
        return static_cast<unsigned>( getpid() );
#endif
    }

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
        const auto result = std::stoul( text );
        require( result >= minimum && result <= maximum, "Argument out of range" );
        return static_cast<unsigned>( result );
    }

    void publish( const std::filesystem::path &path, const std::string &text )
    {
        const auto temporary = path.string() + ".tmp";
        {
            std::ofstream output( temporary, std::ios::binary | std::ios::trunc );
            output << text;
            output.close();
            require( static_cast<bool>( output ), "Cannot write process report" );
        }
        std::filesystem::rename( temporary, path );
    }

    SmartPtr<IPacket> message( WPNetworkManager &manager, Message type, u32 token, u16 player )
    {
        auto packet = manager.createPacket();
        packet->write( Magic );
        packet->write( static_cast<u8>( type ) );
        packet->write( token );
        packet->write( player );
        return packet;
    }

    struct PeerResult
    {
        u16 id = 0;
        bool finished = false;
        bool disconnected = false;
    };

    class ServerListener final : public INetworkListener
    {
    public:
        ServerListener( WPNetworkManager &manager, unsigned expected ) :
            manager( manager ),
            expected( expected )
        {
        }

        void connect( u32 player ) override
        {
            // Also validate the associated sender, independently of the
            // listener's explicit loopback bind configuration.
            if( manager.getClientAddress( static_cast<u16>( player ) ) != 0x7f000001u )
            {
                failure = "Non-loopback peer reached local fixture";
                manager.kickClient( static_cast<u16>( player ) );
                return;
            }
            if( !joined.insert( player ).second )
            {
                failure = "Duplicate connection callback";
            }
        }

        void disconnect( u32 player ) override
        {
            for( auto &[token, peer] : peers )
            {
                if( peer.id == player )
                {
                    peer.disconnected = true;
                }
            }
        }

        void handlePacket( SmartPtr<IPacket> packet ) override
        {
            try
            {
                const auto sender = manager.getPacketSenderId( packet );
                require( sender > 0 && sender <= NET_MAX_PLAYER_ID, "Unassociated client packet" );
                require( packet && packet->getDataLength() == 11, "Invalid local fixture packet size" );
                u32 magic = 0, token = 0;
                u8 type = 0;
                u16 claimed = 0;
                packet->read( magic );
                packet->read( type );
                packet->read( token );
                packet->read( claimed );
                require( magic == Magic && token > 0 && token <= expected && claimed == sender,
                         "Client token or assigned identity mismatch" );
                if( type == static_cast<u8>( Message::Hello ) )
                {
                    for( const auto &[otherToken, peer] : peers )
                    {
                        require( peer.id != sender || otherToken == token,
                                 "Client identity reused by another process" );
                    }
                    auto &peer = peers[token];
                    require( peer.id == 0 || peer.id == sender,
                             "Client token reused by another identity" );
                    peer.id = static_cast<u16>( sender );
                    auto reply = message( manager, Message::HelloAck, token, peer.id );
                    manager.sendPacketUnreliable( reply, peer.id );
                }
                else if( type == static_cast<u8>( Message::Finished ) )
                {
                    const auto found = peers.find( token );
                    require( found != peers.end() && found->second.id == sender,
                             "Finish before verified hello" );
                    found->second.finished = true;
                    auto reply = message( manager, Message::FinishedAck, token, found->second.id );
                    manager.sendPacketUnreliable( reply, found->second.id );
                }
                else
                {
                    throw std::runtime_error( "Unexpected client message" );
                }
            }
            catch( const std::exception &error )
            {
                failure = error.what();
            }
        }

        bool complete() const
        {
            if( peers.size() != expected || joined.size() != expected )
            {
                return false;
            }
            for( const auto &[token, peer] : peers )
            {
                if( !peer.finished || !peer.disconnected )
                {
                    return false;
                }
            }
            return true;
        }

        WPNetworkManager &manager;
        unsigned expected;
        std::set<u32> joined;
        std::map<u32, PeerResult> peers;
        std::string failure;
    };

    class ClientListener final : public INetworkListener
    {
    public:
        ClientListener( WPNetworkManager &manager, unsigned token ) : manager( manager ), token( token )
        {
        }

        void connect( u32 ) override
        {
            ++connections;
        }

        void disconnect( u32 ) override
        {
        }

        void handlePacket( SmartPtr<IPacket> packet ) override
        {
            try
            {
                require( manager.getPacketSenderId( packet ) == 0,
                         "Reply did not come from connected server" );
                require( packet && packet->getDataLength() == 11, "Invalid local fixture reply size" );
                u32 magic = 0, echoedToken = 0;
                u8 type = 0;
                u16 assigned = 0;
                packet->read( magic );
                packet->read( type );
                packet->read( echoedToken );
                packet->read( assigned );
                require( magic == Magic && echoedToken == token && assigned == manager.getPlayerNumber(),
                         "Server reply token or identity mismatch" );
                if( type == static_cast<u8>( Message::HelloAck ) )
                {
                    hello = true;
                }
                else if( type == static_cast<u8>( Message::FinishedAck ) && hello )
                {
                    finished = true;
                }
                else
                {
                    throw std::runtime_error( "Unexpected server reply" );
                }
            }
            catch( const std::exception &error )
            {
                failure = error.what();
            }
        }

        WPNetworkManager &manager;
        unsigned token;
        unsigned connections = 0;
        bool hello = false, finished = false;
        std::string failure;
    };

    void server( const std::filesystem::path &directory, unsigned clients, unsigned timeout )
    {
        auto manager = make_ptr<WPNetworkManager>();
        manager->setBindAddress( 0x7f000001u );
        manager->setPort( 0 );
        manager->setMaxClients( clients );
        auto listener = make_ptr<ServerListener>( *manager, clients );
        manager->addListener( listener );
        manager->setServer( true );
        const auto bound = manager->getDiagnostics();
        const auto port = bound.boundPort;
        require( bound.boundHost == 0x7f000001u, "Server did not bind exclusively to loopback" );
        require( port != 0, "Server has no bound port" );
        publish( directory / "server.ready", std::to_string( port ) );
        const auto deadline = Clock::now() + std::chrono::seconds( timeout );
        while( !listener->complete() && listener->failure.empty() && Clock::now() < deadline )
        {
            manager->poll();
            std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
        }
        require( listener->failure.empty(), listener->failure.c_str() );
        require( listener->complete(), "Server timed out waiting for verified clients and disconnects" );
        manager->unload( nullptr );
        const auto stopped = manager->getDiagnostics();
        require( stopped.state == WPNetworkSessionState::Stopped && stopped.boundPort == 0,
                 "Server did not close its socket" );
        std::string report =
            "{\"version\":1,\"role\":\"server\",\"pid\":" + std::to_string( processId() ) +
            ",\"boundHost\":" + std::to_string( bound.boundHost ) +
            ",\"loopback\":true,\"closed\":true,\"clients\":[";
        bool first = true;
        for( const auto &[token, peer] : listener->peers )
        {
            if( !first )
            {
                report += ',';
            }
            first = false;
            report += "{\"token\":" + std::to_string( token ) +
                      ",\"playerId\":" + std::to_string( peer.id ) +
                      ",\"finished\":true,\"disconnected\":true}";
        }
        report += "]}";
        publish( directory / "server.json", report );
        std::puts(
            "PASS: all client processes exchanged validated messages with distinct identities and "
            "disconnected" );
    }

    void client( const std::filesystem::path &directory, unsigned port, unsigned token,
                 unsigned timeout )
    {
        auto manager = make_ptr<WPNetworkManager>();
        manager->setBindAddress( 0x7f000001u );
        auto listener = make_ptr<ClientListener>( *manager, token );
        manager->addListener( listener );
        manager->connect( "127.0.0.1", static_cast<unsigned short>( port ), "" );
        const auto deadline = Clock::now() + std::chrono::seconds( timeout );
        auto nextSend = Clock::now();
        while( !listener->finished && listener->failure.empty() && Clock::now() < deadline )
        {
            manager->poll();
            if( manager->getConnectionStatus() == INetworkManager::ConnectionStatus::NCS_ESTABLISHED &&
                Clock::now() >= nextSend )
            {
                auto packet = message( *manager, listener->hello ? Message::Finished : Message::Hello,
                                       token, manager->getPlayerNumber() );
                manager->sendPacketUnreliable( packet );
                nextSend = Clock::now() + std::chrono::milliseconds( 50 );
            }
            std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
        }
        require( listener->failure.empty(), listener->failure.c_str() );
        require( listener->finished && listener->connections == 1,
                 "Client timed out or connected more than once" );
        const auto player = manager->getPlayerNumber();
        const auto bound = manager->getDiagnostics();
        require( bound.boundHost == 0x7f000001u, "Client did not bind exclusively to loopback" );
        require( player > 0 && player <= NET_MAX_PLAYER_ID, "Invalid assigned client identity" );
        manager->disconnect();
        manager->poll();
        manager->unload( nullptr );
        const auto stopped = manager->getDiagnostics();
        require( stopped.state == WPNetworkSessionState::Stopped && stopped.boundPort == 0,
                 "Client did not close its socket" );
        publish( directory / ( "client-" + std::to_string( token ) + ".json" ),
                 "{\"version\":1,\"role\":\"client\",\"pid\":" + std::to_string( processId() ) +
                     ",\"token\":" + std::to_string( token ) + ",\"playerId\":" +
                     std::to_string( player ) + ",\"boundHost\":" + std::to_string( bound.boundHost ) +
                     ",\"helloAcknowledged\":true,\"finishAcknowledged\":true,\"closed\":true}" );
        std::printf( "PASS: client %u assigned player %u exchanged messages and closed\n", token,
                     static_cast<unsigned>( player ) );
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
        std::map<std::string, std::string> options;
        require( argc > 1 && argc % 2 == 1,
                 "Expected --mode server/client --output directory and mode options" );
        for( int i = 1; i < argc; i += 2 )
        {
            require( options.emplace( argv[i], argv[i + 1] ).second, "Duplicate argument" );
        }
        const auto mode = options.at( "--mode" );
        const auto directory = std::filesystem::absolute( options.at( "--output" ) );
        const auto timeout = integer( options.at( "--timeout" ), 5, 30 );
        require( std::filesystem::is_directory( directory ), "Output directory must already exist" );
        if( mode == "server" )
        {
            require( options.size() == 4, "Unknown server argument" );
            server( directory, integer( options.at( "--clients" ), 2, 8 ), timeout );
        }
        else
        {
            require( mode == "client" && options.size() == 5, "Unknown mode or client argument" );
            client( directory, integer( options.at( "--port" ), 1, 65535 ),
                    integer( options.at( "--client" ), 1, 8 ), timeout );
        }
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
