#include "WPRakNet/CNetworkManager.hpp"
#include "WPRakNet/CPacket.hpp"
#include "WPRakNet/CSystemAddress.hpp"

#include <BitStream.h>
#include <GetTime.h>
#include <MessageIdentifiers.h>
#include <RakPeerInterface.h>

#include <Workphone/Workphone.hpp>

#include <algorithm>
#include <array>
#include <limits>

namespace workphone
{
    using namespace RakNet;

    namespace
    {
        struct ConnectionStateMapping
        {
            ConnectionState state;
            INetworkManager::ConnectionStatus status;
        };

        constexpr std::array<ConnectionStateMapping, 3> ConnectionStateMappings = {
            { { IS_CONNECTED, INetworkManager::ConnectionStatus::NCS_ESTABLISHED },
              { IS_CONNECTING, INetworkManager::ConnectionStatus::NCS_PENDING },
              { IS_PENDING, INetworkManager::ConnectionStatus::NCS_PENDING } }
        };

        constexpr std::array<DefaultMessageIDTypes, 4> ConnectionFailureMessages = {
            { ID_CONNECTION_ATTEMPT_FAILED, ID_CONNECTION_BANNED, ID_INVALID_PASSWORD,
              ID_ALREADY_CONNECTED }
        };

        constexpr std::array<DefaultMessageIDTypes, 2> DisconnectMessages = {
            { ID_DISCONNECTION_NOTIFICATION, ID_CONNECTION_LOST }
        };

        template <size_t N>
        [[nodiscard]] bool containsMessageId( const std::array<DefaultMessageIDTypes, N> &messages,
                                              DefaultMessageIDTypes messageId )
        {
            return std::find( messages.begin(), messages.end(), messageId ) != messages.end();
        }

        [[nodiscard]] u16 clampToRakConnectionCount( u32 value )
        {
            return static_cast<u16>(
                std::min<u32>( value, static_cast<u32>( std::numeric_limits<u16>::max() ) ) );
        }

        [[nodiscard]] bool isUsableAddress( const SystemAddress &address )
        {
            return address != UNASSIGNED_SYSTEM_ADDRESS;
        }
    }  // namespace

    CNetworkManager::CNetworkManager()
    {
        m_netCallback = SmartPtr<scene::NetworkListener>( new scene::NetworkListener() );
        m_rakInterface = RakPeerInterface::GetInstance();
    }

    CNetworkManager::CNetworkManager( bool isClient ) : CNetworkManager()
    {
        m_isServer = !isClient;
    }

    CNetworkManager::~CNetworkManager()
    {
        if( m_rakInterface )
        {
            m_rakInterface->Shutdown( 300 );
            RakPeerInterface::DestroyInstance( m_rakInterface );
            m_rakInterface = nullptr;
        }
    }

    void CNetworkManager::setServer( bool isServer )
    {
        m_isServer = isServer;
        m_connected = false;

        if( m_isServer )
            m_connected = startPeer( true, m_maxClients );
        else
            m_started = startPeer( false, 1 );
    }

    bool CNetworkManager::isServer() const
    {
        return m_isServer;
    }

    void CNetworkManager::setPeer()
    {
        m_isServer = false;
        m_connected = false;
        m_started = startPeer( true, m_maxClients );
    }

    void CNetworkManager::setVerbose( bool isverbose )
    {
        m_verbose = isverbose;
    }

    void CNetworkManager::setNetIterations( u16 iterations )
    {
        m_netIterations = iterations == 0 ? 1 : iterations;
    }

    void CNetworkManager::setListener( SmartPtr<INetworkListener> netCallback )
    {
        m_netCallback->clearListeners();
        m_netCallback->addListener( netCallback );
    }

    void CNetworkManager::addListener( SmartPtr<INetworkListener> netCallback )
    {
        m_netCallback->addListener( netCallback );
    }

    void CNetworkManager::removeListener( SmartPtr<INetworkListener> netCallback )
    {
        m_netCallback->removeListener( netCallback );
    }

    void CNetworkManager::setGlobalPacketRelay( bool relay )
    {
        m_globalRelay = relay;
    }

    void CNetworkManager::update()
    {
        if( !m_rakInterface )
            return;

        u16 processed = 0;
        auto packet = m_rakInterface->Receive();

        while( packet && processed < m_netIterations )
        {
            if( !packet->data || packet->length == 0 )
            {
                m_rakInterface->DeallocatePacket( packet );
                packet = m_rakInterface->Receive();
                ++processed;
                continue;
            }

            const auto messageId = static_cast<DefaultMessageIDTypes>( packet->data[0] );

            if( messageId == ID_NEW_INCOMING_CONNECTION )
            {
                const u32 newPlayerId =
                    m_rakInterface->GetIndexFromSystemAddress( packet->systemAddress );
                if( m_verbose )
                    printf( "CNetworkManager: new connection from %s (id=%u)\n",
                            packet->systemAddress.ToString(), newPlayerId );

                if( m_netCallback )
                    m_netCallback->connect( newPlayerId );
            }
            else if( messageId == ID_CONNECTION_REQUEST_ACCEPTED )
            {
                m_serverAddress = packet->systemAddress;
                m_connected = true;
                m_playerNumber = 0;

                if( m_verbose )
                    printf( "CNetworkManager: connected to server %s\n",
                            packet->systemAddress.ToString() );

                if( m_netCallback )
                    m_netCallback->connect( 0 );
            }
            else if( containsMessageId( ConnectionFailureMessages, messageId ) )
            {
                m_connected = false;

                if( m_verbose )
                    printf( "CNetworkManager: connection failed (id=%d)\n",
                            static_cast<int>( messageId ) );
            }
            else if( containsMessageId( DisconnectMessages, messageId ) )
            {
                const u32 lostId =
                    m_isServer ? m_rakInterface->GetIndexFromSystemAddress( packet->systemAddress ) : 0u;

                if( m_verbose )
                    printf( "CNetworkManager: peer disconnected %s (id=%u)\n",
                            packet->systemAddress.ToString(), lostId );

                if( !m_isServer )
                    m_connected = false;

                if( m_netCallback )
                    m_netCallback->disconnect( lostId );
            }
            else
            {
                if( m_netCallback )
                {
                    auto newPacket = workphone::make_ptr<CPacket>();
                    newPacket->initialise( packet );
                    m_netCallback->handlePacket( newPacket );
                }

                if( m_isServer && m_globalRelay )
                {
                    m_rakInterface->Send( reinterpret_cast<const char *>( packet->data ),
                                          static_cast<int>( packet->length ), HIGH_PRIORITY, RELIABLE, 0,
                                          packet->systemAddress, true );
                }
            }

            m_rakInterface->DeallocatePacket( packet );
            packet = m_rakInterface->Receive();
            ++processed;
        }

        if( packet )
            m_rakInterface->DeallocatePacket( packet );
    }

    void CNetworkManager::sendPacket( SmartPtr<IPacket> &outpacket )
    {
        sendPacketInternal( outpacket, getDefaultSendAddress(), m_isServer, SendReliability::Reliable );
    }

    void CNetworkManager::sendPacket( SmartPtr<IPacket> &outpacket, u16 playerId )
    {
        sendPacketInternal( outpacket, getAddressForPlayer( playerId ), false,
                            SendReliability::Reliable );
    }

    void CNetworkManager::sendPacket( SmartPtr<IPacket> &outpacket,
                                      SmartPtr<ISystemAddress> systemAddress )
    {
        auto cAddr = systemAddress ? dynamic_cast<CSystemAddress *>( systemAddress.get() ) : nullptr;
        if( !cAddr || !cAddr->getSystemAddress() )
            return;

        sendPacketInternal( outpacket, *cAddr->getSystemAddress(), false, SendReliability::Reliable );
    }

    void CNetworkManager::sendPacketToAllExcept( SmartPtr<IPacket> &outpacket,
                                                 SmartPtr<ISystemAddress> systemAddress )
    {
        auto cAddr = systemAddress ? dynamic_cast<CSystemAddress *>( systemAddress.get() ) : nullptr;
        if( !cAddr || !cAddr->getSystemAddress() )
            return;

        sendPacketInternal( outpacket, *cAddr->getSystemAddress(), true, SendReliability::Reliable );
    }

    void CNetworkManager::sendPacketUnreliable( SmartPtr<IPacket> &outpacket )
    {
        sendPacketInternal( outpacket, getDefaultSendAddress(), m_isServer,
                            SendReliability::Unreliable );
    }

    void CNetworkManager::sendPacketUnreliable( SmartPtr<IPacket> &outpacket, u16 playerId )
    {
        sendPacketInternal( outpacket, getAddressForPlayer( playerId ), false,
                            SendReliability::Unreliable );
    }

    void CNetworkManager::kickClient( u16 playerId, bool hardKick )
    {
        if( !m_rakInterface )
            return;

        const auto addr = getAddressForPlayer( playerId );
        if( !isUsableAddress( addr ) )
            return;

        m_rakInterface->CloseConnection( addr, !hardKick );
    }

    const u32 CNetworkManager::getPeerCount()
    {
        return m_rakInterface ? m_rakInterface->NumberOfConnections() : 0u;
    }

    const u32 CNetworkManager::getPing()
    {
        if( !m_rakInterface )
            return 0u;

        const auto addr = m_isServer ? getAddressForPlayer( 0 ) : m_serverAddress;
        return isUsableAddress( addr ) ? static_cast<u32>( m_rakInterface->GetAveragePing( addr ) ) : 0u;
    }

    const u32 CNetworkManager::getClientAddress( u16 playerId )
    {
        const auto addr = getAddressForPlayer( playerId );
        return isUsableAddress( addr ) ? addr.address.addr4.sin_addr.s_addr : 0u;
    }

    u16 CNetworkManager::getPlayerNumber() const
    {
        return m_playerNumber;
    }

    String CNetworkManager::getLocalRakNetGUID()
    {
        if( !m_rakInterface )
            return String();

        return String(
            m_rakInterface->GetGuidFromSystemAddress( UNASSIGNED_SYSTEM_ADDRESS ).ToString() );
    }

    void CNetworkManager::connect( const String &address, unsigned short port, const String &password )
    {
        if( !m_rakInterface || address.empty() || port == 0 )
            return;

        if( !m_started )
            setServer( false );

        const char *pwd = password.empty() ? nullptr : password.c_str();
        const int pwdLen = password.empty() ? 0 : static_cast<int>( password.length() );
        const auto result = m_rakInterface->Connect( address.c_str(), port, pwd, pwdLen );

        if( result == CONNECTION_ATTEMPT_STARTED )
            m_serverAddress = SystemAddress( address.c_str(), port );
        else
            m_connected = false;
    }

    INetworkManager::ConnectionStatus CNetworkManager::getConnectionStatus()
    {
        if( !m_rakInterface )
            return ConnectionStatus::NCS_FAILED;

        if( m_isServer )
            return m_started ? ConnectionStatus::NCS_ESTABLISHED : ConnectionStatus::NCS_FAILED;

        if( m_connected )
            return ConnectionStatus::NCS_ESTABLISHED;

        const auto cs = m_rakInterface->GetConnectionState( m_serverAddress );
        const auto it = std::find_if(
            ConnectionStateMappings.begin(), ConnectionStateMappings.end(),
            [cs]( const ConnectionStateMapping &mapping ) { return mapping.state == cs; } );

        return it != ConnectionStateMappings.end() ? it->status : ConnectionStatus::NCS_FAILED;
    }

    SmartPtr<IPacket> CNetworkManager::createPacket()
    {
        auto packet = workphone::make_ptr<CPacket>();
        packet->initialise();
        return packet;
    }

    time_interval CNetworkManager::getServerTime() const
    {
        return static_cast<time_interval>( RakNet::GetTimeMS() ) * 0.001;
    }

    u32 CNetworkManager::getPort()
    {
        return m_port;
    }

    void CNetworkManager::setPort( u32 value )
    {
        if( value <= static_cast<u32>( std::numeric_limits<u16>::max() ) )
            m_port = value;
    }

    u32 CNetworkManager::GetMaxClients()
    {
        return m_maxClients;
    }

    void CNetworkManager::SetMaxClients( u32 value )
    {
        m_maxClients = value == 0 ? 1 : value;
    }

    RakNet::RakPeerInterface *CNetworkManager::GetRakPeerInterface()
    {
        return m_rakInterface;
    }

    void CNetworkManager::SetRakPeerInterface( RakNet::RakPeerInterface *rakInterface )
    {
        if( m_rakInterface == rakInterface )
            return;

        if( m_rakInterface )
        {
            m_rakInterface->Shutdown( 300 );
            RakPeerInterface::DestroyInstance( m_rakInterface );
        }

        m_rakInterface = rakInterface;
        m_started = false;
        m_connected = false;
    }

    bool CNetworkManager::getVerbose() const
    {
        return m_verbose;
    }

    bool CNetworkManager::getGlobalPacketRelay() const
    {
        return m_globalRelay;
    }

    u16 CNetworkManager::getNetIterations() const
    {
        return m_netIterations;
    }

    SmartPtr<INetworkListener> CNetworkManager::getListener() const
    {
        return SmartPtr<INetworkListener>( m_netCallback );
    }

    bool CNetworkManager::getConnected() const
    {
        return m_connected;
    }

    void CNetworkManager::setConnected( bool connected )
    {
        m_connected = connected;
    }

    bool CNetworkManager::getStarted() const
    {
        return m_started;
    }

    void CNetworkManager::setStarted( bool started )
    {
        m_started = started;
    }

    RakNet::SystemAddress CNetworkManager::getServerAddress() const
    {
        return m_serverAddress;
    }

    void CNetworkManager::setServerAddress( const RakNet::SystemAddress &serverAddress )
    {
        m_serverAddress = serverAddress;
    }

    void CNetworkManager::setPlayerNumber( u16 playerNumber )
    {
        m_playerNumber = playerNumber;
    }

    bool CNetworkManager::startPeer( bool serverMode, u32 maxConnections )
    {
        if( !m_rakInterface )
            return false;

        if( m_started )
            m_rakInterface->Shutdown( 300 );

        SocketDescriptor socketDescriptor( serverMode ? static_cast<unsigned short>( m_port ) : 0,
                                           nullptr );
        const u32 connectionCount = std::max<u32>( 1u, maxConnections );
        const auto result = m_rakInterface->Startup( connectionCount, &socketDescriptor, 1 );
        m_started = result == RAKNET_STARTED;

        if( m_started )
        {
            if( serverMode )
                m_rakInterface->SetMaximumIncomingConnections(
                    clampToRakConnectionCount( connectionCount ) );

            m_rakInterface->SetOccasionalPing( true );
        }

        return m_started;
    }

    bool CNetworkManager::sendPacketInternal( SmartPtr<IPacket> &outpacket,
                                              const RakNet::SystemAddress &address, bool broadcast,
                                              SendReliability reliability )
    {
        if( !m_rakInterface || !m_started || !outpacket )
            return false;

        auto packet = dynamic_cast<CPacket *>( outpacket.get() );
        if( !packet || !packet->getBitStream() || packet->getDataLength() == 0 )
            return false;

        if( !broadcast && !isUsableAddress( address ) )
            return false;

        const auto priority = reliability == SendReliability::Reliable ? HIGH_PRIORITY : MEDIUM_PRIORITY;
        const auto packetReliability =
            reliability == SendReliability::Reliable ? RELIABLE_ORDERED : UNRELIABLE;

        return m_rakInterface->Send( packet->getBitStream(), priority, packetReliability, 0, address,
                                     broadcast ) > 0;
    }

    RakNet::SystemAddress CNetworkManager::getAddressForPlayer( u16 playerId ) const
    {
        return m_rakInterface ? m_rakInterface->GetSystemAddressFromIndex( playerId )
                              : UNASSIGNED_SYSTEM_ADDRESS;
    }

    RakNet::SystemAddress CNetworkManager::getDefaultSendAddress() const
    {
        return m_isServer ? UNASSIGNED_SYSTEM_ADDRESS : m_serverAddress;
    }
}  // namespace workphone
