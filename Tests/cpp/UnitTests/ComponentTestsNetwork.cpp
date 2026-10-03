#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    class RecordingNetworkListener : public INetworkListener
    {
    public:
        void handlePacket( SmartPtr<IPacket> packet ) override
        {
            ++packetCount;
            lastPacket = packet;
        }

        void connect( u32 playerId ) override
        {
            ++connectCount;
            lastPlayerId = playerId;
        }

        void disconnect( u32 playerId ) override
        {
            ++disconnectCount;
            lastPlayerId = playerId;
        }

        u32 packetCount = 0;
        u32 connectCount = 0;
        u32 disconnectCount = 0;
        u32 lastPlayerId = 0;
        SmartPtr<IPacket> lastPacket;
    };
}  // namespace

BOOST_AUTO_TEST_SUITE( ComponentNetworkTests )

BOOST_AUTO_TEST_CASE( network_stream_round_trips_supported_types )
{
    NetworkStream writer( true );
    BOOST_CHECK( writer.isWriting() );
    BOOST_CHECK( !writer.isReading() );

    writer.write( static_cast<s8>( -8 ) );
    writer.write( static_cast<u8>( 8 ) );
    writer.write( static_cast<s16>( -1600 ) );
    writer.write( static_cast<u16>( 1600 ) );
    writer.write( static_cast<s32>( -32000 ) );
    writer.write( static_cast<u32>( 32000 ) );
    writer.write( 12.5f );
    writer.write( true );
    writer.write( String( "network stream" ) );
    writer.write( Vector2I( 2, -3 ) );
    writer.write( Vector2<real_Num>( 1.25f, -2.5f ) );
    writer.write( Vector3I( 4, -5, 6 ) );
    writer.write( Vector3<real_Num>( 7.5f, -8.25f, 9.0f ) );

    BOOST_REQUIRE_GT( writer.getSize(), 0u );
    BOOST_CHECK_EQUAL( writer.getPosition(), writer.getSize() );
    BOOST_REQUIRE( writer.getData() );

    NetworkStream reader( false );
    reader.setData( writer.getData(), writer.getSize() );
    BOOST_CHECK( reader.isReading() );
    BOOST_CHECK( !reader.isWriting() );

    s8 s8Value{};
    u8 u8Value{};
    s16 s16Value{};
    u16 u16Value{};
    s32 s32Value{};
    u32 u32Value{};
    f32 floatValue{};
    bool boolValue = false;
    String stringValue;
    Vector2I vector2i;
    Vector2<real_Num> vector2;
    Vector3I vector3i;
    Vector3<real_Num> vector3;

    reader.read( s8Value );
    reader.read( u8Value );
    reader.read( s16Value );
    reader.read( u16Value );
    reader.read( s32Value );
    reader.read( u32Value );
    reader.read( floatValue );
    reader.read( boolValue );
    reader.read( stringValue );
    reader.read( vector2i );
    reader.read( vector2 );
    reader.read( vector3i );
    reader.read( vector3 );

    BOOST_CHECK_EQUAL( s8Value, -8 );
    BOOST_CHECK_EQUAL( u8Value, 8u );
    BOOST_CHECK_EQUAL( s16Value, -1600 );
    BOOST_CHECK_EQUAL( u16Value, 1600u );
    BOOST_CHECK_EQUAL( s32Value, -32000 );
    BOOST_CHECK_EQUAL( u32Value, 32000u );
    BOOST_CHECK_CLOSE( floatValue, 12.5f, 0.001f );
    BOOST_CHECK( boolValue );
    BOOST_CHECK_EQUAL( stringValue, "network stream" );
    BOOST_CHECK_EQUAL( vector2i.x, 2 );
    BOOST_CHECK_EQUAL( vector2i.y, -3 );
    BOOST_CHECK_CLOSE( vector2.x, 1.25f, 0.001f );
    BOOST_CHECK_CLOSE( vector2.y, -2.5f, 0.001f );
    BOOST_CHECK_EQUAL( vector3i.x, 4 );
    BOOST_CHECK_EQUAL( vector3i.y, -5 );
    BOOST_CHECK_EQUAL( vector3i.z, 6 );
    BOOST_CHECK_CLOSE( vector3.x, 7.5f, 0.001f );
    BOOST_CHECK_CLOSE( vector3.y, -8.25f, 0.001f );
    BOOST_CHECK_CLOSE( vector3.z, 9.0f, 0.001f );
    BOOST_CHECK_EQUAL( reader.getPosition(), reader.getSize() );
}

BOOST_AUTO_TEST_CASE( network_stream_reset_raw_io_and_bounds )
{
    const u8 input[] = { 1, 2, 3, 4 };
    NetworkStream stream( true );
    BOOST_CHECK_EQUAL( stream.write( input, sizeof( input ) ), sizeof( input ) );
    stream.reset();
    BOOST_CHECK_EQUAL( stream.getPosition(), 0u );

    u8 output[6] = {};
    BOOST_CHECK_EQUAL( stream.read( output, sizeof( output ) ), sizeof( input ) );
    BOOST_CHECK_EQUAL_COLLECTIONS( input, input + sizeof( input ), output, output + sizeof( input ) );
    BOOST_CHECK_EQUAL( stream.read( output, 1 ), 0u );

    NetworkStream typedReader( false );
    typedReader.setData( input, 1 );
    u32 value = 0;
    BOOST_CHECK_THROW( typedReader.read( value ), std::out_of_range );
}

BOOST_AUTO_TEST_CASE( network_player_stores_identity_role_and_custom_properties )
{
    NetworkPlayer player;
    BOOST_CHECK_EQUAL( player.getActorNumber(), -1 );
    BOOST_CHECK_EQUAL( player.getPing(), 0u );
    BOOST_CHECK( !player.isLocal() );
    BOOST_CHECK( !player.isMasterClient() );
    BOOST_CHECK( !player.getCustomProperties() );

    player.setActorNumber( 42 );
    player.setNickName( "Lion" );
    player.setUserId( "user-42" );
    player.setPing( 27 );
    player.setLocal( true );
    player.setMasterClient( true );
    auto properties = make_ptr<Properties>();
    properties->setProperty( "score", 99 );
    player.setCustomProperties( properties );

    BOOST_CHECK_EQUAL( player.getActorNumber(), 42 );
    BOOST_CHECK_EQUAL( player.getNickName(), "Lion" );
    BOOST_CHECK_EQUAL( player.getUserId(), "user-42" );
    BOOST_CHECK_EQUAL( player.getPing(), 27u );
    BOOST_CHECK( player.isLocal() );
    BOOST_CHECK( player.isMasterClient() );
    BOOST_CHECK( player.getCustomProperties() == properties );
}

BOOST_AUTO_TEST_CASE( network_listener_manages_and_dispatches_children )
{
    NetworkListener dispatcher;
    auto first = make_ptr<RecordingNetworkListener>();
    auto second = make_ptr<RecordingNetworkListener>();

    dispatcher.addListener( nullptr );
    dispatcher.addListener( first );
    dispatcher.addListener( first );
    dispatcher.addListener( second );
    BOOST_CHECK_EQUAL( dispatcher.getListenerCount(), 2u );
    BOOST_CHECK( dispatcher.hasListener( first ) );
    BOOST_CHECK( dispatcher.hasListener( second ) );

    dispatcher.handlePacket( nullptr );
    dispatcher.connect( 17 );
    dispatcher.disconnect( 23 );
    BOOST_CHECK_EQUAL( first->packetCount, 1u );
    BOOST_CHECK_EQUAL( first->connectCount, 1u );
    BOOST_CHECK_EQUAL( first->disconnectCount, 1u );
    BOOST_CHECK_EQUAL( first->lastPlayerId, 23u );
    BOOST_CHECK_EQUAL( second->packetCount, 1u );

    dispatcher.removeListener( first );
    dispatcher.connect( 99 );
    BOOST_CHECK_EQUAL( first->connectCount, 1u );
    BOOST_CHECK_EQUAL( second->connectCount, 2u );
    BOOST_CHECK( !dispatcher.hasListener( first ) );

    dispatcher.clearListeners();
    BOOST_CHECK_EQUAL( dispatcher.getListenerCount(), 0u );
}

BOOST_AUTO_TEST_CASE( network_view_properties_and_actor_attachment )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.isAvailable );

    auto actor = guard.sceneManager->createActor();
    BOOST_REQUIRE( actor );
    auto view = actor->addComponent<NetworkView>();
    BOOST_REQUIRE( view );
    BOOST_CHECK( actor->getComponent<NetworkView>().get() == view.get() );
    BOOST_CHECK( view->getActor().get() == actor.get() );

    view->setViewId( 123 );
    view->setOwnerId( 456 );
    view->setReplicationEnabled( true );
    view->setAuthorityMode( NetworkView::AuthorityMode::Owner );
    view->setDeliveryMode( NetworkView::DeliveryMode::Reliable );
    view->setSendRate( 30.0f );
    view->setSyncPosition( true );
    view->setSyncRotation( false );
    view->setSyncScale( true );
    view->setAllowOwnershipRequests( true );
    BOOST_CHECK_EQUAL( view->getViewId(), 123 );
    BOOST_CHECK_EQUAL( view->getOwnerId(), 456u );
    BOOST_CHECK( !view->isSceneView() );

    auto properties = view->getProperties();
    BOOST_REQUIRE( properties );
    auto restored = make_ptr<NetworkView>();
    restored->setProperties( properties );
    BOOST_CHECK_EQUAL( restored->getViewId(), 123 );
    BOOST_CHECK_EQUAL( restored->getOwnerId(), 456u );
    BOOST_CHECK( !restored->isSceneView() );
    BOOST_CHECK( restored->isReplicationEnabled() );
    BOOST_CHECK( restored->getAuthorityMode() == NetworkView::AuthorityMode::Owner );
    BOOST_CHECK( restored->getDeliveryMode() == NetworkView::DeliveryMode::Reliable );
    BOOST_CHECK_CLOSE( restored->getSendRate(), 30.0f, 0.001f );
    BOOST_CHECK( restored->getSyncPosition() );
    BOOST_CHECK( !restored->getSyncRotation() );
    BOOST_CHECK( restored->getSyncScale() );
    BOOST_CHECK( restored->getAllowOwnershipRequests() );

    auto sendRateProperty = properties->getPropertyObject( NetworkView::sendRateStr );
    BOOST_CHECK_EQUAL( sendRateProperty.getAttribute( "category" ), "Replication" );
    BOOST_CHECK_EQUAL( sendRateProperty.getAttribute( "min" ), "1" );
    BOOST_CHECK_EQUAL( sendRateProperty.getAttribute( "max" ), "120" );

    guard.sceneManager->destroyActor( actor );
}

BOOST_AUTO_TEST_SUITE_END()
