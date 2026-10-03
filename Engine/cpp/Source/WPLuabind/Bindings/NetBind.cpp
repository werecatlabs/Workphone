#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/NetBind.hpp"
#include <luabind/luabind.hpp>
#include "WPLuabind/SmartPtrConverter.hpp"
#include "WPLuabind/ParamConverter.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace
    {
        template <class T>
        T packetRead( IPacket *packet )
        {
            T value{};
            packet->read( value );
            return value;
        }

        template <class T>
        T streamRead( INetworkStream *stream )
        {
            T value{};
            stream->read( value );
            return value;
        }

        void sendPacket( INetworkManager *manager, SmartPtr<IPacket> packet )
        {
            manager->sendPacket( packet );
        }

        void sendPacketToPlayer( INetworkManager *manager, SmartPtr<IPacket> packet, u16 playerId )
        {
            manager->sendPacket( packet, playerId );
        }

        void sendPacketToAddress( INetworkManager *manager, SmartPtr<IPacket> packet,
                                  SmartPtr<ISystemAddress> address )
        {
            manager->sendPacket( packet, address );
        }

        void sendPacketToAllExcept( INetworkManager *manager, SmartPtr<IPacket> packet,
                                    SmartPtr<ISystemAddress> address )
        {
            manager->sendPacketToAllExcept( packet, address );
        }

        void sendPacketUnreliable( INetworkManager *manager, SmartPtr<IPacket> packet )
        {
            manager->sendPacketUnreliable( packet );
        }

        void sendPacketUnreliableToPlayer( INetworkManager *manager, SmartPtr<IPacket> packet,
                                           u16 playerId )
        {
            manager->sendPacketUnreliable( packet, playerId );
        }

        int getConnectionStatus( INetworkManager *manager )
        {
            return static_cast<int>( manager->getConnectionStatus() );
        }
    } // namespace

    void bindNet( lua_State *L )
    {
        using namespace luabind;

        module(
            L )[class_<IPacket, ISharedObject, SmartPtr<IPacket>>( "IPacket" )
                    .def( "readInt8", &packetRead<s8> )
                    .def( "readUInt8", &packetRead<u8> )
                    .def( "readInt16", &packetRead<s16> )
                    .def( "readUInt16", &packetRead<u16> )
                    .def( "readInt32", &packetRead<s32> )
                    .def( "readUInt32", &packetRead<u32> )
                    .def( "readFloat", &packetRead<f32> )
                    .def( "readBool", &packetRead<bool> )
                    .def( "readString", &packetRead<String> )
                    .def( "readVector2I", &packetRead<Vector2I> )
                    .def( "readVector2", &packetRead<Vector2<real_Num>> )
                    .def( "readVector3I", &packetRead<Vector3I> )
                    .def( "readVector3", &packetRead<Vector3<real_Num>> )
                    .def( "writeInt8", static_cast<void ( IPacket::* )( s8 )>( &IPacket::write ) )
                    .def( "writeUInt8", static_cast<void ( IPacket::* )( u8 )>( &IPacket::write ) )
                    .def( "writeInt16", static_cast<void ( IPacket::* )( s16 )>( &IPacket::write ) )
                    .def( "writeUInt16", static_cast<void ( IPacket::* )( u16 )>( &IPacket::write ) )
                    .def( "writeInt32", static_cast<void ( IPacket::* )( s32 )>( &IPacket::write ) )
                    .def( "writeUInt32", static_cast<void ( IPacket::* )( u32 )>( &IPacket::write ) )
                    .def( "writeFloat", static_cast<void ( IPacket::* )( f32 )>( &IPacket::write ) )
                    .def( "writeBool",
                          static_cast<void ( IPacket::* )( const bool & )>( &IPacket::write ) )
                    .def( "writeString",
                          static_cast<void ( IPacket::* )( const String & )>( &IPacket::write ) )
                    .def( "writeVector2I",
                          static_cast<void ( IPacket::* )( const Vector2I & )>( &IPacket::write ) )
                    .def( "writeVector2", static_cast<void ( IPacket::* )( const Vector2<real_Num> & )>(
                                              &IPacket::write ) )
                    .def( "writeVector3I",
                          static_cast<void ( IPacket::* )( const Vector3I & )>( &IPacket::write ) )
                    .def( "writeVector3", static_cast<void ( IPacket::* )( const Vector3<real_Num> & )>(
                                              &IPacket::write ) )
                    .def( "ignoreMessageId", &IPacket::ignoreMessageId )
                    .def( "getDataLength", &IPacket::getDataLength )
                    .def( "resetReadPointer", &IPacket::resetReadPointer )
                    .def( "getSystemAddress", &IPacket::getSystemAddress )
                    .scope[def( "typeInfo", IPacket::typeInfo )]];

        module(
            L )[class_<INetworkListener, ISharedObject, SmartPtr<INetworkListener>>( "INetworkListener" )
                    .def( "handlePacket", &INetworkListener::handlePacket )
                    .def( "connect", &INetworkListener::connect )
                    .def( "disconnect", &INetworkListener::disconnect )
                    .scope[def( "typeInfo", INetworkListener::typeInfo )]];

        module(
            L )[class_<INetworkManager, ISharedObject, SmartPtr<INetworkManager>>( "INetworkManager" )
                    .def( "setServer", &INetworkManager::setServer )
                    .def( "isServer", &INetworkManager::isServer )
                    .def( "setPeer", &INetworkManager::setPeer )
                    .def( "setVerbose", &INetworkManager::setVerbose )
                    .def( "setListener", &INetworkManager::setListener )
                    .def( "sendPacket", sendPacket )
                    .def( "sendPacketToPlayer", sendPacketToPlayer )
                    .def( "sendPacketToAddress", sendPacketToAddress )
                    .def( "sendPacketToAllExcept", sendPacketToAllExcept )
                    .def( "sendPacketUnreliable", sendPacketUnreliable )
                    .def( "sendPacketUnreliableToPlayer", sendPacketUnreliableToPlayer )
                    .def( "getPeerCount", &INetworkManager::getPeerCount )
                    .def( "getPlayerNumber", &INetworkManager::getPlayerNumber )
                    .def( "getClientAddress", &INetworkManager::getClientAddress )
                    .def( "getLocalRakNetGUID", &INetworkManager::getLocalRakNetGUID )
                    .def( "kickClient", &INetworkManager::kickClient )
                    .def( "getConnectionStatus", getConnectionStatus )
                    .def( "connect", &INetworkManager::connect )
                    .def( "getPing", &INetworkManager::getPing )
                    .def( "setNetIterations", &INetworkManager::setNetIterations )
                    .def( "setGlobalPacketRelay", &INetworkManager::setGlobalPacketRelay )
                    .def( "createPacket", &INetworkManager::createPacket )
                    .def( "getServerTime", &INetworkManager::getServerTime )
                    .enum_( "ConnectionStatus" )
                        [value( "Pending", INetworkManager::ConnectionStatus::NCS_PENDING ),
                         value( "Established", INetworkManager::ConnectionStatus::NCS_ESTABLISHED ),
                         value( "Failed", INetworkManager::ConnectionStatus::NCS_FAILED )]
                    .scope[def( "typeInfo", INetworkManager::typeInfo )]];

        module( L )[class_<INetworkPlayer, ISharedObject, SmartPtr<INetworkPlayer>>( "INetworkPlayer" )
                        .def( "getActorNumber", &INetworkPlayer::getActorNumber )
                        .def( "setActorNumber", &INetworkPlayer::setActorNumber )
                        .def( "getNickName", &INetworkPlayer::getNickName )
                        .def( "setNickName", &INetworkPlayer::setNickName )
                        .def( "getUserId", &INetworkPlayer::getUserId )
                        .def( "setUserId", &INetworkPlayer::setUserId )
                        .def( "getPing", &INetworkPlayer::getPing )
                        .def( "setPing", &INetworkPlayer::setPing )
                        .def( "isLocal", &INetworkPlayer::isLocal )
                        .def( "setLocal", &INetworkPlayer::setLocal )
                        .def( "isMasterClient", &INetworkPlayer::isMasterClient )
                        .def( "setMasterClient", &INetworkPlayer::setMasterClient )
                        .def( "getCustomProperties", &INetworkPlayer::getCustomProperties )
                        .def( "setCustomProperties", &INetworkPlayer::setCustomProperties )
                        .scope[def( "typeInfo", INetworkPlayer::typeInfo )]];

        module(
            L )[class_<INetworkStream, ISharedObject, SmartPtr<INetworkStream>>( "INetworkStream" )
                    .def( "isWriting", &INetworkStream::isWriting )
                    .def( "isReading", &INetworkStream::isReading )
                    .def( "readInt8", &streamRead<s8> )
                    .def( "readUInt8", &streamRead<u8> )
                    .def( "readInt16", &streamRead<s16> )
                    .def( "readUInt16", &streamRead<u16> )
                    .def( "readInt32", &streamRead<s32> )
                    .def( "readUInt32", &streamRead<u32> )
                    .def( "readFloat", &streamRead<f32> )
                    .def( "readBool", &streamRead<bool> )
                    .def( "readString", &streamRead<String> )
                    .def( "readVector2I", &streamRead<Vector2I> )
                    .def( "readVector2", &streamRead<Vector2<real_Num>> )
                    .def( "readVector3I", &streamRead<Vector3I> )
                    .def( "readVector3", &streamRead<Vector3<real_Num>> )
                    .def( "writeInt8",
                          static_cast<void ( INetworkStream::* )( s8 )>( &INetworkStream::write ) )
                    .def( "writeUInt8",
                          static_cast<void ( INetworkStream::* )( u8 )>( &INetworkStream::write ) )
                    .def( "writeInt16",
                          static_cast<void ( INetworkStream::* )( s16 )>( &INetworkStream::write ) )
                    .def( "writeUInt16",
                          static_cast<void ( INetworkStream::* )( u16 )>( &INetworkStream::write ) )
                    .def( "writeInt32",
                          static_cast<void ( INetworkStream::* )( s32 )>( &INetworkStream::write ) )
                    .def( "writeUInt32",
                          static_cast<void ( INetworkStream::* )( u32 )>( &INetworkStream::write ) )
                    .def( "writeFloat",
                          static_cast<void ( INetworkStream::* )( f32 )>( &INetworkStream::write ) )
                    .def( "writeBool",
                          static_cast<void ( INetworkStream::* )( bool )>( &INetworkStream::write ) )
                    .def( "writeString", static_cast<void ( INetworkStream::* )( const String & )>(
                                             &INetworkStream::write ) )
                    .def( "writeVector2I", static_cast<void ( INetworkStream::* )( const Vector2I & )>(
                                               &INetworkStream::write ) )
                    .def( "writeVector2", static_cast<void ( INetworkStream::* )(
                                              const Vector2<real_Num> & )>( &INetworkStream::write ) )
                    .def( "writeVector3I", static_cast<void ( INetworkStream::* )( const Vector3I & )>(
                                               &INetworkStream::write ) )
                    .def( "writeVector3", static_cast<void ( INetworkStream::* )(
                                              const Vector3<real_Num> & )>( &INetworkStream::write ) )
                    .def( "getSize", &INetworkStream::getSize )
                    .def( "getPosition", &INetworkStream::getPosition )
                    .def( "reset", &INetworkStream::reset )
                    .scope[def( "typeInfo", INetworkStream::typeInfo )]];

        module( L )[class_<INetworkView, ISharedObject, SmartPtr<INetworkView>>( "INetworkView" )
                        .def( "RPC", &INetworkView::RPC )
                        .def( "serializeView", &INetworkView::serializeView )
                        .def( "deserializeView", &INetworkView::deserializeView )
                        .def( "onSerializeView", &INetworkView::onSerializeView )
                        .def( "getViewId", &INetworkView::getViewId )
                        .def( "setViewId", &INetworkView::setViewId )
                        .def( "getOwnerId", &INetworkView::getOwnerId )
                        .def( "setOwnerId", &INetworkView::setOwnerId )
                        .def( "isMine", &INetworkView::isMine )
                        .def( "transferOwnership", &INetworkView::transferOwnership )
                        .def( "requestOwnership", &INetworkView::requestOwnership )
                        .def( "isSceneView", &INetworkView::isSceneView )
                        .scope[def( "typeInfo", INetworkView::typeInfo )]];

        module( L )[class_<ISystemAddress, ISharedObject, SmartPtr<ISystemAddress>>( "ISystemAddress" )
                        .def( "setBinaryAddress", &ISystemAddress::setBinaryAddress )
                        .def( "getBinaryAddress", &ISystemAddress::getBinaryAddress )
                        .def( "setPort", &ISystemAddress::setPort )
                        .def( "getPort", &ISystemAddress::getPort )
                        .def( "toString", &ISystemAddress::toString )
                        .def( "isValid", &ISystemAddress::isValid )
                        .scope[def( "typeInfo", ISystemAddress::typeInfo )]];
    }
} // namespace workphone
