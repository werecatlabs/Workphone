#include <WPNetwork/WPNetworkGNSManager.hpp>
#include <WPNetwork/WPNetwork.hpp>
#include <WPNetwork/WPNetworkPacket.hpp>
#include <Workphone/Interface/Net/INetworkListener.hpp>
#include <Workphone/Interface/Net/ISystemAddress.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/TimerChrono.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/Scene/Components/NetworkView.hpp>
#include <chrono>
#include <thread>
#include <vector>
#include <cstdio>
#include <stdexcept>
#include <functional>
using namespace workphone;

static void require( bool value, const char *error )
{
    if( !value )
    {
        throw std::runtime_error( error );
    }
}

template <class Exception = std::exception, class F>
static bool rejects( F f )
{
    try
    {
        f();
    }
    catch( const Exception & )
    {
        return true;
    }
    return false;
}

struct Recorder : INetworkListener
{
    unsigned joins = 0, leaves = 0;
    std::vector<SmartPtr<IPacket>> packets;
    std::function<void( u32 )> onJoin;
    std::function<void()> onPacket;
    bool packetBeforeJoin = false;

    void connect( u32 player ) override
    {
        ++joins;
        if( onJoin )
        {
            onJoin( player );
        }
    }

    void disconnect( u32 ) override
    {
        ++leaves;
    }

    void handlePacket( SmartPtr<IPacket> packet ) override
    {
        packets.push_back( packet );
        packetBeforeJoin |= joins == 0;
        if( onPacket )
        {
            onPacket();
        }
    }
};

struct View : scene::NetworkView
{
    void bind( SmartPtr<INetworkManager> manager, SmartPtr<scene::IGameActor> actor )
    {
        m_networkManager = manager;
        setActor( actor );
        setEnabled( true );
    }
};

int main()
{
    TypeManager types;
    types.load();
    TypeManager::setInstance( &types );
    auto app = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance( app );
    app->setTimer( make_ptr<TimerChrono>() );
    int result = 1;
    try
    {
        {
            WPNetworkGNSConfig strictConfig;
            strictConfig.allowedPeerIdentities = { "ip:127.0.0.1" };
            require( rejects<std::invalid_argument>(
                         [&] { make_ptr<WPNetworkGNSManager>( strictConfig ); } ),
                     "An IP address was allowed as a strict GNS identity" );
            strictConfig.allowedPeerIdentities = { "str:wpnetwork-test", "str:alternate-server" };
            auto multiServerClient = make_ptr<WPNetworkGNSManager>( strictConfig );
            require( rejects<std::logic_error>( [&] { multiServerClient->setAdmissionTicket( {} ); } ),
                     "A client staged a ticket with an ambiguous server identity" );
            strictConfig.allowedPeerIdentities = { "str:wpnetwork-test" };
            auto strict = make_ptr<WPNetworkGNSManager>( strictConfig );
            require( strict->getBoundAddress().empty(), "Construction opened a listener" );
            require( rejects<std::logic_error>( [&] { strict->setServer( true ); } ),
                     "Strict mode started without a certificate" );
            require( rejects<std::logic_error>( [&] { strict->installCertificate( { 1, 2, 3 } ); } ),
                     "Strict mode allowed certificate import before generating its request" );
            require( !strict->getCertificateRequest().empty(), "Certificate request missing" );
            WPNetworkGNSConfig development;
            development.allowUnverifiedLoopback = true;
            auto dev = make_ptr<WPNetworkGNSManager>( development );
            require( rejects<std::logic_error>( [&] { dev->setServer( false ); } ),
                     "Development manager shared the reserved certificate identity" );
            require( rejects<std::runtime_error>( [&] { strict->installCertificate( { 1, 2, 3 } ); } ),
                     "Malformed certificate accepted" );
            strict->unload( nullptr );
            strict->unload( nullptr );
        }
        WPNetworkGNSConfig config;
        config.allowUnverifiedLoopback = true;
        auto a = workphone::dynamic_pointer_cast<WPNetworkGNSManager>(
            createWPNetworkClient( WP_GNS_LOOPBACK_PROFILE ) );
        auto b = make_ptr<WPNetworkGNSManager>( config );
        require( a != nullptr, "Profile did not select GNS" );
        SmartPtr<WPNetworkGNSManager> server;
        unsigned port = 0;
        for( unsigned attempt = 0; attempt < 64 && !server; ++attempt )
        {
            port = 32000 + attempt;
            config.bindAddress = "[::1]:" + std::to_string( port );
            auto candidate = make_ptr<WPNetworkGNSManager>( config );
            try
            {
                candidate->setServer( true );
                server = candidate;
            }
            catch( const std::runtime_error & )
            {
            }
        }
        require( server != nullptr, "Cannot bind loopback GNS server" );
        require( server->getCapabilities() ==
                     ( INetworkManager::ReliableOrderedDelivery | INetworkManager::UnreliableDelivery ),
                 "Capability overstatement" );
        auto received = make_ptr<Recorder>(), ra = make_ptr<Recorder>(), rb = make_ptr<Recorder>();
        received->onJoin = [&]( u32 player ) {
            auto welcome = server->createPacket();
            welcome->write( u32( 0x1234 ) );
            server->sendPacket( welcome, static_cast<u16>( player ) );
        };
        server->addListener( received );
        a->addListener( ra );
        b->addListener( rb );
        require( rejects( [&] { a->connect( "192.0.2.1", 1234, "" ); } ),
                 "Unverified mode permitted external IP" );
        a->connect( "::1", static_cast<u16>( port ), "" );
        b->connect( "::1", static_cast<u16>( port ), "" );
        auto pump = [&] {
            server->poll();
            a->poll();
            b->poll();
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        };
        auto until = [&]( auto predicate ) {
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
            while( !predicate() && std::chrono::steady_clock::now() < deadline )
            {
                pump();
            }
            require( predicate(), "GNS integration deadline" );
        };
        until( [&] {
            return a->getPlayerNumber() != 0xffffu && b->getPlayerNumber() != 0xffffu &&
                   ra->joins == 1 && rb->joins == 1 && ra->packets.size() == 1 &&
                   rb->packets.size() == 1;
        } );
        require( !ra->packetBeforeJoin && !rb->packetBeforeJoin,
                 "Payload arrived before client connection callback" );
        received->onJoin = {};
        ra->packets.clear();
        rb->packets.clear();
        require( a->getPlayerNumber() != b->getPlayerNumber() && server->getPeerCount() == 2,
                 "Nonunique peer identity" );
        for( u32 seq = 0; seq < 32; ++seq )
        {
            auto p = a->createPacket();
            p->write( seq );
            a->sendPacket( p );
        }
        std::vector<u8> bytes( 60000, 37 );
        auto large = a->createPacket();
        dynamic_cast<WPNetworkPacket *>( large.get() )->setData( bytes.data(), bytes.size() );
        a->sendPacket( large );
        until( [&] { return received->packets.size() == 33; } );
        for( u32 seq = 0; seq < 32; ++seq )
        {
            u32 value = 99;
            received->packets[seq]->read( value );
            require( value == seq, "Reliable order violated" );
            require( server->getPacketSenderId( received->packets[seq] ) == a->getPlayerNumber(),
                     "Sender association wrong" );
        }
        auto big = dynamic_cast<WPNetworkPacket *>( received->packets.back().get() );
        require( big->getDataLength() == bytes.size() &&
                     std::equal( bytes.begin(), bytes.end(), big->getData() ),
                 "Fragmented engine packet corrupt" );
        auto address = received->packets.front()->getSystemAddress();
        auto reply = server->createPacket();
        reply->write( u32( 71 ) );
        server->sendPacket( reply, address );
        until( [&] { return ra->packets.size() == 1; } );
        require( rb->packets.empty(), "Targeted packet broadcast unexpectedly" );

        // Real scene components use the same engine interface and reliable path.
        auto source = make_ptr<scene::GameActor>(), remote = make_ptr<scene::GameActor>();
        auto st = make_ptr<scene::Transform>(), rt = make_ptr<scene::Transform>();
        source->setTransform( st );
        st->setActor( source );
        remote->setTransform( rt );
        rt->setActor( remote );
        auto sv = make_ptr<View>(), cv = make_ptr<View>();
        sv->bind( server, source );
        cv->bind( b, remote );
        sv->setViewId( 42 );
        cv->setViewId( 42 );
        sv->setDeliveryMode( scene::NetworkView::DeliveryMode::Reliable );
        auto subscription = make_ptr<scene::NetworkView::Listener>( cv.get() );
        b->addListener( subscription );
        app->setPlaying( true );
        st->setPosition( Vector3<real_Num>( 4, 5, 6 ) );
        sv->serializeView();
        until( [&] { return rt->getPosition() == Vector3<real_Num>( 4, 5, 6 ); } );
        sv->transferOwnership( b->getPlayerNumber() );
        until( [&] { return cv->getOwnerId() == b->getPlayerNumber(); } );
        subscription->detach();
        b->removeListener( subscription );
        sv->unload( nullptr );
        cv->unload( nullptr );
        source->setTransform( nullptr );
        st->setActor( nullptr );
        remote->setTransform( nullptr );
        rt->setActor( nullptr );
        app->setPlaying( false );
        const auto old = a->getPlayerNumber();
        a->disconnect();
        a->disconnect();
        until( [&] { return server->getPeerCount() == 1 && ra->leaves == 1; } );
        require( server->getPacketSenderId( received->packets.front() ) == -1,
                 "Stale sender remained valid" );
        require( !address->isValid(), "Stale GNS address advertised itself as valid" );
        require( rejects( [&] { server->sendPacket( reply, address ); } ), "Stale endpoint accepted" );
        a->connect( "::1", static_cast<u16>( port ), "" );
        until( [&] { return a->getPlayerNumber() != 0xffffu && ra->joins == 2; } );
        require( a->getPlayerNumber() != old, "Reconnect reused player identity" );
        rb->onPacket = [&] { b->unload( nullptr ); };
        server->sendPacket( reply, b->getPlayerNumber() );
        until(
            [&] { return b->getConnectionStatus() == INetworkManager::ConnectionStatus::NCS_FAILED; } );
        rb->onPacket = {};
        a->unload( nullptr );
        b->unload( nullptr );
        server->unload( nullptr );
        server->unload( nullptr );
        require( server->getBoundAddress().empty(), "Listener survived unload" );
        std::puts(
            "PASS: real GNS engine facade, two clients, identity, reliable ordering, large packets, "
            "scene transforms/ownership, stale endpoints and lifecycle. Development identities only." );
        result = 0;
    }
    catch( const std::exception &error )
    {
        std::fprintf( stderr, "FAIL: %s\n", error.what() );
    }
    core::IApplicationManager::setInstance( nullptr );
    app = nullptr;
    TypeManager::setInstance( nullptr );
    types.unload();
    return result;
}
