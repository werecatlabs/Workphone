#ifndef WPNetworkManager_h__
#define WPNetworkManager_h__

#include <WPNetwork/WPNetworkPrerequisites.hpp>
#include <Workphone/Interface/Net/INetworkManager.hpp>
#include <mutex>

extern "C" {
#include <WorkphoneNetwork/workphone_network.h>
}

namespace workphone
{
    namespace scene
    {
        class NetworkListener;
    }

    /**
     * @class WPNetworkManager
     * @brief Implementation of the network manager handling the communication between clients and server.
     *
     * This class manages the network context, packet transmission, and peer connections using the underlying
     * network library.
     */
    class WPNetwork_API WPNetworkManager : public INetworkManager
    {
    public:
        /** @brief Default constructor. */
        WPNetworkManager();

        /** @brief Constructor that initializes the manager as either a client or server. @param isClient True if this instance should act as a client. */
        explicit WPNetworkManager( bool isClient );
        ~WPNetworkManager() override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void poll() override;
        u32 getCapabilities() const override;
        s32 getPacketSenderId( SmartPtr<IPacket> packet ) const override;

        /** @brief Sets whether this instance acts as a server. @param isServer True to enable server mode. */
        void setServer( bool isServer ) override;

        /** @brief Checks if this instance is acting as a server. @return True if server, false otherwise. */
        bool isServer() const override;

        /** @brief Sets the current instance as a peer. */
        void setPeer() override;

        /** @brief Enables or disables verbose logging for network events. @param isverbose True to enable verbose output. */
        void setVerbose( bool isverbose ) override;

        /** @brief Sets the network listener callback for receiving events. @param netCallback Smart pointer to the network listener. */
        void setListener( SmartPtr<INetworkListener> netCallback ) override;

        /** @copydoc INetworkManager::addListener */
        void addListener( SmartPtr<INetworkListener> netCallback ) override;

        /** @copydoc INetworkManager::removeListener */
        void removeListener( SmartPtr<INetworkListener> netCallback ) override;

        /** @brief Sends a reliable packet to all connected peers. @param outpacket The packet to be sent. */
        void sendPacket( SmartPtr<IPacket> &outpacket ) override;

        /** @brief Sends a reliable packet to a specific player. @param outpacket The packet to be sent. @param playerId The ID of the target player. */
        void sendPacket( SmartPtr<IPacket> &outpacket, u16 playerId ) override;

        /** @brief Sends a reliable packet to a specific system address. @param outpacket The packet to be sent. @param systemAddress The target system address. */
        void sendPacket( SmartPtr<IPacket> &outpacket, SmartPtr<ISystemAddress> systemAddress ) override;

        /** @brief Sends a reliable packet to all connected peers except the one specified. @param outpacket The packet to be sent. @param systemAddress The address of the peer to exclude. */
        void sendPacketToAllExcept( SmartPtr<IPacket> &outpacket,
                                    SmartPtr<ISystemAddress> systemAddress ) override;

        /** @brief Sends an unreliable packet to all connected peers. @param outpacket The packet to be sent. */
        void sendPacketUnreliable( SmartPtr<IPacket> &outpacket ) override;

        /** @brief Sends an unreliable packet to a specific player. @param outpacket The packet to be sent. @param playerId The ID of the target player. */
        void sendPacketUnreliable( SmartPtr<IPacket> &outpacket, u16 playerId ) override;

        /** @brief Returns the current number of connected peers. @return Total peer count. */
        const u32 getPeerCount() override;
        
        /** @brief Returns the local player's network ID. @return Player number. */
        u16 getPlayerNumber() const override;

        /** @brief Gets the network address of a specific client. @param playerId The ID of the client. @return The system address. */
        const u32 getClientAddress( u16 playerId ) override;

        /** @brief Returns the local RakNet GUID. @return The GUID string. */
        String getLocalRakNetGUID() override;

        /** @brief Kicks a client from the session. @param playerId The ID of the client to kick. @param hardKick Whether to perform a hard disconnect. */
        void kickClient( u16 playerId, bool hardKick = false ) override;

        /** @brief Gets the current connection status. @return The current ConnectionStatus. */
        ConnectionStatus getConnectionStatus() override;

        /** @brief Connects to a server. @param address The server address. @param port The server port. @param password The connection password. */
        void connect( const String &address, unsigned short port, const String &password ) override;

        /** @brief Returns the current ping to the server. @return Ping in milliseconds. */
        const u32 getPing() override;

        /** @brief Sets the number of network iterations per update. @param iterations Number of events to process. */
        void setNetIterations( u16 iterations ) override;

        /** @brief Enables or disables global packet relay. @param relay True to enable relay. */
        void setGlobalPacketRelay( bool relay ) override;

        /** @brief Creates a new empty packet. @return A smart pointer to a new IPacket. */
        SmartPtr<IPacket> createPacket() override;

        /** @brief Gets the current server time. @return Current time interval. */
        time_interval getServerTime() const override;

        /** @brief Processes network events and updates connection states. */
        void update();

        /** @brief Gets the current listener port. @return The port number. */
        u32 getPort() const;

        /** @brief Sets the listener port. @param port The port number to use. */
        void setPort( u32 port );

        /** @brief Gets the maximum number of allowed clients. @return Max clients. */
        u32 getMaxClients() const;

        /** @brief Sets the maximum number of allowed clients. @param maxClients The maximum limit. */
        void setMaxClients( u32 maxClients );

        /** Development inspection only. Raw context access requires exclusive
         * application-task ownership; do not retain across unload or poll.
         * @return Pointer to NetContext. */
        NetContext *getContext();

        /** @brief Gets the internal network context (const). @return Const pointer to NetContext. */
        const NetContext *getContext() const;

        WP_CLASS_REGISTER_DECL;

    private:
        /** @brief Dispatches a network event to the registered listener. @param event The event to dispatch. */
        void dispatchEvent( const NetEvent &event );

        /** @brief Extracts raw data from a packet. @param packet The packet to read from. @param size Output parameter for the data size. @return Pointer to raw byte data. */
        const u8 *getPacketData( SmartPtr<IPacket> &packet, u32 &size ) const;

        /** @brief Finds a peer by their system address. @param systemAddress The address to search for. @return Pointer to the peer if found. */
        NetPeer *findPeerByAddress( SmartPtr<ISystemAddress> systemAddress );

        /** @brief Finds a peer by their system address (const). @param systemAddress The address to search for. @return Const pointer to the peer if found. */
        const NetPeer *findPeerByAddress( SmartPtr<ISystemAddress> systemAddress ) const;

        NetContext m_context;                   ///< Internal network context
        mutable std::recursive_mutex m_contextMutex;
        SmartPtr<scene::NetworkListener> m_listener;  ///< Multiplexed network event listeners
        bool m_isServer = false;                ///< Flag indicating if the manager is acting as a server
        bool m_started = false;                 ///< Flag indicating if the network has been started
        bool m_connected = false;          ///< Flag indicating if the client is connected to a server
        bool m_verbose = false;            ///< Flag for verbose logging
        bool m_globalRelay = false;        ///< Flag for global packet relay
        bool m_socketRuntimeInitialized = false;
        bool m_polling = false;
        u32 m_port = 15822;                ///< The network port used for listening/connecting
        u32 m_maxClients = NET_MAX_PEERS;  ///< Maximum number of clients allowed on the server
        u16 m_netIterations =
            NET_MAX_EVENTS;  ///< Maximum number of network events to process per update
    };
}  // namespace workphone

#endif  // WPNetworkManager_h__
