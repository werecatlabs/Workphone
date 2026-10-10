#include <WPNetwork/WPNetworkManager.hpp>
#include <WPNetwork/WPNetworkPacket.hpp>
#include <WPNetwork/WPNetworkStream.hpp>
#include <WPNetwork/WPNetworkSystemAddress.hpp>
#include <WPNetwork/WPNetwork.hpp>
#include <WPNetwork/WPNetworkConfig.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Scene/GameActor.hpp>
#include <Workphone/Scene/Transform.hpp>
#include <Workphone/Scene/Components/NetworkStream.hpp>
#include <Workphone/Scene/Components/NetworkView.hpp>
#include <Workphone/Scene/Components/NetworkListener.hpp>
#include <Workphone/Interface/Net/NetworkMessage.hpp>
#include <Workphone/Interface/Net/NetworkActorSnapshot.hpp>
#include <Workphone/Interface/Net/INetworkListener.hpp>
#include <chrono>
#include <thread>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <atomic>

using namespace workphone;
#define CHECK( condition )                                                      \
    do                                                                          \
    {                                                                           \
        if( !( condition ) )                                                    \
        {                                                                       \
            std::fprintf( stderr, "FAIL line %d: %s\n", __LINE__, #condition ); \
            return false;                                                       \
        }                                                                       \
    } while( false )
template <class Error, class Fn>
bool throws( Fn fn )
{
    try
    {
        fn();
    }
    catch( const Error & )
    {
        return true;
    }
    return false;
}

class Recorder : public INetworkListener
{
public:
    void connect( u32 ) override
    {
        ++joined;
    }
    void disconnect( u32 ) override
    {
        ++left;
    }
    void handlePacket( SmartPtr<IPacket> packet ) override
    {
        received = packet;
        ++messages;
    }
    unsigned joined = 0, left = 0, messages = 0;
    SmartPtr<IPacket> received;
};

class ProbeView : public scene::NetworkView
{
public:
    void bind( SmartPtr<INetworkManager> manager, SmartPtr<scene::IGameActor> actor )
    {
        m_networkManager = manager;
        setActor( actor );
        setEnabled( true );
    }
    using NetworkView::handlePacket;
    bool hasSnapshot() const
    {
        return m_hasReceivedSnapshot;
    }
};

class FailingListener : public INetworkListener
{
public:
    void connect( u32 ) override
    {
        throw std::runtime_error( "intentional callback failure" );
    }
    void disconnect( u32 ) override
    {
        throw std::runtime_error( "intentional callback failure" );
    }
    void handlePacket( SmartPtr<IPacket> ) override
    {
        throw std::runtime_error( "intentional callback failure" );
    }
};

class ReentrantListener : public INetworkListener
{
public:
    explicit ReentrantListener( WPNetworkManager *manager ) : manager( manager )
    {
    }
    void connect( u32 ) override
    {
        ++depth;
        maxDepth = std::max( maxDepth, depth );
        manager->poll();
        --depth;
        ++joined;
    }
    void disconnect( u32 ) override
    {
    }
    void handlePacket( SmartPtr<IPacket> ) override
    {
    }
    WPNetworkManager *manager;
    int depth = 0, maxDepth = 0, joined = 0;
};

bool profiles()
{
    struct TemporaryProfile
    {
        std::filesystem::path path =
            std::filesystem::temp_directory_path() /
            ( "wp-network-" +
              std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) + ".json" );
        ~TemporaryProfile()
        {
            std::error_code error;
            std::filesystem::remove( path, error );
        }
        void write( const std::string &text )
        {
            std::ofstream output( path );
            output << text;
            if( !output )
                throw std::runtime_error( "profile fixture write failed" );
        }
    } fixture;
    const String path = fixture.path.string().c_str();
    const std::string prefix =
        R"({"version":1,"backend":"native_udp_development","environment":"development")";
    fixture.write( prefix + R"(,"port":0,"maxClients":4,"eventsPerPoll":32,"verbose":true})" );
    const auto config = WPNetworkConfig::load( path );
    CHECK( config.port == 0 && config.maxClients == 4 && config.eventsPerPoll == 32 && config.verbose );
    auto server = createWPNetworkServer( path );
    auto native = dynamic_cast<WPNetworkManager *>( server.get() );
    CHECK( native && native->isServer() && native->getContext()->running );
    const auto port = net_get_bound_port( native->getContext() );
    fixture.write( prefix + ",\"port\":" + std::to_string( port ) + "}" );
    CHECK( throws<std::runtime_error>( [&] { createWPNetworkServer( path ); } ) );
    server->unload( nullptr );
    auto client = createWPNetworkClient( path );
    CHECK( !client->isServer() &&
           client->getConnectionStatus() == INetworkManager::ConnectionStatus::NCS_FAILED );
    client->unload( nullptr );
    for( const auto &suffix :
         { ",\"port\":65536}", ",\"maxClients\":0}", ",\"eventsPerPoll\":1.5}", ",\"verbose\":\"true\"}",
           ",\"password\":\"secret\"}", ",\"version\":1}", "} garbage", ",\"port\" 12}" } )
    {
        fixture.write( prefix + suffix );
        CHECK( throws<std::invalid_argument>( [&] { WPNetworkConfig::load( path ); } ) );
    }
    fixture.write( R"({"version":1,"backend":"native_udp_development","environment":"production"})" );
    CHECK( throws<std::invalid_argument>( [&] { WPNetworkConfig::load( path ); } ) );
    fixture.write( "{}" );
    CHECK( throws<std::invalid_argument>( [&] { WPNetworkConfig::load( path ); } ) );
    fixture.write( std::string( 8193, ' ' ) );
    CHECK( throws<std::length_error>( [&] { WPNetworkConfig::load( path ); } ) );
    CHECK( throws<std::runtime_error>( [&] { WPNetworkConfig::load( path + ".missing" ); } ) );
    return true;
}

bool codecs()
{
    WPNetworkPacket packet;
    packet.write( u32( 0x12345678 ) );
    packet.write( s16( -42 ) );
    packet.write( f32( 1.25f ) );
    packet.write( String( "bytes" ) );
    packet.write( true );
    packet.write( Vector3<real_Num>( 1, 2, 3 ) );
    CHECK( packet.getData()[0] == 0x78 && packet.getData()[3] == 0x12 );
    WPNetworkStream wrapper( false );
    scene::NetworkStream component( false );
    wrapper.setData( packet.getData(), packet.getDataLength() );
    component.setData( packet.getData(), packet.getDataLength() );
    for( INetworkStream *reader :
         { static_cast<INetworkStream *>( &wrapper ), static_cast<INetworkStream *>( &component ) } )
    {
        u32 a = 0;
        s16 b = 0;
        f32 c = 0;
        String string;
        bool flag = false;
        Vector3<real_Num> vector;
        reader->read( a );
        reader->read( b );
        reader->read( c );
        reader->read( string );
        reader->read( flag );
        reader->read( vector );
        CHECK( a == 0x12345678 && b == -42 && c == 1.25f && string == "bytes" && flag );
        CHECK( vector == Vector3<real_Num>( 1, 2, 3 ) );
        CHECK( reader->getPosition() == reader->getSize() );
        CHECK( throws<std::out_of_range>( [&] { reader->read( a ); } ) );
        reader->reset();
        CHECK( throws<std::invalid_argument>( [&] { reader->read( nullptr, 1 ); } ) );
        CHECK( reader->getPosition() == 0 );
    }
    const auto size = wrapper.getSize();
    CHECK( throws<std::invalid_argument>( [&] { wrapper.setData( nullptr, 1 ); } ) );
    CHECK( wrapper.getSize() == size );
    wrapper.setData( wrapper.getData(), wrapper.getSize() );
    CHECK( wrapper.getSize() == size );
    NetEvent invalid{};
    invalid.size = NET_MAX_PACKET_SIZE + 1;
    CHECK( throws<std::length_error>( [&] { WPNetworkPacket malformed( invalid ); } ) );
    CHECK( network::isNewerSequence( 0, 0xffffffffu ) );
    CHECK( !network::isNewerSequence( 0xffffffffu, 0 ) );
    CHECK( !network::isNewerSequence( 7, 7 ) );
    return true;
}

SmartPtr<WPNetworkPacket> snapshot( u32 sequence, u8 channels, SmartPtr<ISystemAddress> address )
{
    auto packet = make_ptr<WPNetworkPacket>();
    NetworkMessageHeader header;
    header.objectId = 42;
    header.type = NetworkMessageType::ActorState;
    header.write( packet );
    packet->write( sequence );
    packet->write( channels );
    if( channels & 1 )
        packet->write( Vector3<real_Num>( 10, 20, 30 ) );
    if( channels & 2 )
        packet->write( Vector3<real_Num>( 0, 45, 0 ) );
    if( channels & 4 )
        packet->write( Vector3<real_Num>( 2, 2, 2 ) );
    packet->setSystemAddress( address );
    return packet;
}

bool sessions( SmartPtr<core::ApplicationManager> application )
{
    auto multiplex = make_ptr<scene::NetworkListener>();
    auto failing = make_ptr<FailingListener>();
    auto survivor = make_ptr<Recorder>();
    multiplex->addListener( failing );
    multiplex->addListener( survivor );
    multiplex->connect( 1 );
    multiplex->disconnect( 1 );
    multiplex->handlePacket( make_ptr<WPNetworkPacket>() );
    CHECK( survivor->joined == 1 && survivor->left == 1 && survivor->messages == 1 );
    auto server = make_ptr<WPNetworkManager>();
    auto a = make_ptr<WPNetworkManager>(), b = make_ptr<WPNetworkManager>();
    CHECK( !server->getContext()->running );
    CHECK( server->getCapabilities() == INetworkManager::UnreliableDelivery );
    server->setPort( 0 );
    server->setServer( true );
    auto record = make_ptr<Recorder>();
    server->addListener( record );
    auto reentrant = make_ptr<ReentrantListener>( server.get() );
    server->addListener( reentrant );
    const auto port = net_get_bound_port( server->getContext() );
    CHECK( port != 0 );
    a->connect( "127.0.0.1", port, "" );
    b->connect( "127.0.0.1", port, "" );
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 3 );
    while( ( a->getConnectionStatus() != INetworkManager::ConnectionStatus::NCS_ESTABLISHED ||
             b->getConnectionStatus() != INetworkManager::ConnectionStatus::NCS_ESTABLISHED ) &&
           std::chrono::steady_clock::now() < until )
    {
        server->poll();
        a->poll();
        b->poll();
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
    }
    CHECK( a->getPlayerNumber() != b->getPlayerNumber() );
    CHECK( record->joined == 2 );
    CHECK( reentrant->joined == 2 && reentrant->maxDepth == 1 );
    CHECK( server->getPlayerNumber() == 0 && a->getPlayerNumber() != 0xffffu &&
           b->getPlayerNumber() != 0xffffu );
    auto packet = a->createPacket();
    packet->write( u32( 123 ) );
    CHECK( throws<std::logic_error>( [&] { a->sendPacket( packet ); } ) );
    CHECK( throws<std::logic_error>( [&] { a->connect( "127.0.0.1", port, "ignored-password" ); } ) );
    CHECK( throws<std::logic_error>( [&] { server->setGlobalPacketRelay( true ); } ) );
    CHECK( throws<std::logic_error>( [&] { a->getServerTime(); } ) );
    a->sendPacketUnreliable( packet );
    for( int n = 0; n < 30 && !record->received; ++n )
    {
        server->poll();
        a->poll();
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
    }
    CHECK( record->received && record->messages == 1 );
    CHECK( server->getPacketSenderId( record->received ) == a->getPlayerNumber() );

    // Exercise production scene transforms and real manager sender association.
    auto actor = make_ptr<scene::GameActor>();
    auto transform = make_ptr<scene::Transform>();
    actor->setTransform( transform );
    transform->setActor( actor );
    auto view = make_ptr<ProbeView>();
    view->bind( server, actor );
    view->setViewId( 42 );
    view->setAuthorityMode( scene::NetworkView::AuthorityMode::Owner );
    view->setOwnerId( a->getPlayerNumber() );
    application->setPlaying( true );
    const auto address = record->received->getSystemAddress();
    auto valid = snapshot( 1, 3, address );
    view->handlePacket( valid );
    CHECK( transform->getPosition() == Vector3<real_Num>( 10, 20, 30 ) && view->hasSnapshot() );
    transform->setPosition( Vector3<real_Num>( 1, 2, 3 ) );
    auto stale = snapshot( 1, 3, address );
    view->handlePacket( stale );
    CHECK( transform->getPosition() == Vector3<real_Num>( 1, 2, 3 ) );
    auto broken = snapshot( 2, 3, address );
    broken->setData( broken->getData(), broken->getDataLength() - 4 );
    CHECK( throws<std::invalid_argument>( [&] { view->handlePacket( broken ); } ) );
    CHECK( transform->getPosition() == Vector3<real_Num>( 1, 2, 3 ) );
    auto corrected = snapshot( 2, 3, address );
    view->handlePacket( corrected );
    CHECK( transform->getPosition() == Vector3<real_Num>( 10, 20, 30 ) );
    transform->setPosition( Vector3<real_Num>( 1, 2, 3 ) );
    auto unknownChannel = snapshot( 3, 0x83, address );
    CHECK( throws<std::invalid_argument>( [&] { view->handlePacket( unknownChannel ); } ) );
    auto trailing = snapshot( 3, 3, address );
    trailing->write( u8( 0 ) );
    CHECK( throws<std::invalid_argument>( [&] { view->handlePacket( trailing ); } ) );
    auto nonfinite = snapshot( 3, 0, address );
    nonfinite->setData( nonfinite->getData(), NetworkMessageHeader::SerializedSize );
    nonfinite->write( u32( 3 ) );
    nonfinite->write( u8( 1 ) );
    nonfinite->write( Vector3<real_Num>( 10, std::numeric_limits<f32>::quiet_NaN(), 30 ) );
    CHECK( throws<std::invalid_argument>( [&] { view->handlePacket( nonfinite ); } ) );
    CHECK( transform->getPosition() == Vector3<real_Num>( 1, 2, 3 ) );
    application->setPlaying( false );
    view->handlePacket( snapshot( 3, 3, address ) );
    CHECK( transform->getPosition() == Vector3<real_Num>( 1, 2, 3 ) );
    application->setPlaying( true );
    view->setEnabled( false );
    view->handlePacket( snapshot( 3, 3, address ) );
    CHECK( transform->getPosition() == Vector3<real_Num>( 1, 2, 3 ) );
    view->setEnabled( true );
    view->setOwnerId( b->getPlayerNumber() );
    auto unauthorized = snapshot( 3, 3, address );
    view->handlePacket( unauthorized );
    CHECK( transform->getPosition() == Vector3<real_Num>( 1, 2, 3 ) );
    view->setAuthorityMode( scene::NetworkView::AuthorityMode::Server );
    auto forgedState = snapshot( 4, 3, address );
    view->handlePacket( forgedState );
    CHECK( transform->getPosition() == Vector3<real_Num>( 1, 2, 3 ) );
    auto ownership = make_ptr<WPNetworkPacket>();
    NetworkMessageHeader h;
    h.type = NetworkMessageType::OwnershipTransfer;
    h.objectId = 42;
    h.write( ownership );
    ownership->write( u32( a->getPlayerNumber() ) );
    ownership->setSystemAddress( address );
    view->handlePacket( ownership );
    CHECK( view->getOwnerId() == b->getPlayerNumber() );
    CHECK( throws<std::logic_error>( [&] { view->transferOwnership( a->getPlayerNumber() ); } ) );
    CHECK( view->getOwnerId() == b->getPlayerNumber() );
    CHECK( throws<std::invalid_argument>(
        [&] { view->setSendRate( std::numeric_limits<f32>::quiet_NaN() ); } ) );
    auto detached = make_ptr<scene::NetworkView::Listener>( view.get() );
    detached->detach();
    detached->handlePacket( snapshot( 5, 3, address ) );
    view->unload( nullptr );
    view = nullptr;
    detached = nullptr;
    actor->setTransform( nullptr );
    transform->setActor( nullptr );
    actor = nullptr;
    transform = nullptr;
    application->setPlaying( false );

    net_disconnect( a->getContext() );
    a->poll();
    server->poll();
    CHECK( a->getConnectionStatus() == INetworkManager::ConnectionStatus::NCS_FAILED );
    server->unload( nullptr );
    server->unload( nullptr );
    CHECK( !server->getContext()->running && net_get_bound_port( server->getContext() ) == 0 );
    server->setServer( true );
    CHECK( server->getContext()->running );
    server->unload( nullptr );
    for( int cycle = 0; cycle < 8; ++cycle )
    {
        server->setServer( true );
        std::atomic<bool> entered{ false };
        std::thread polling( [&] {
            entered.store( true );
            for( int i = 0; i < 1000; ++i )
                server->poll();
        } );
        while( !entered.load() )
            std::this_thread::yield();
        server->unload( nullptr );
        polling.join();
        CHECK( !server->getContext()->running );
    }
    a->unload( nullptr );
    b->unload( nullptr );
    return true;
}

int main()
{
    TypeManager types;
    types.load();
    TypeManager::setInstance( &types );
    auto application = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance( application );
    bool ok = false;
    try
    {
        ok = codecs() && profiles() && sessions( application );
    }
    catch( const std::exception &error )
    {
        std::fprintf( stderr, "FAIL exception: %s\n", error.what() );
    }
    core::IApplicationManager::setInstance( nullptr );
    application = nullptr;
    TypeManager::setInstance( nullptr );
    types.unload();
    if( ok )
        std::puts(
            "PASS: concrete packet/streams, sessions, sender authorization, atomic scene state and "
            "lifecycle" );
    return ok ? 0 : 1;
}
