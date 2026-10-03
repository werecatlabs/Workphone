#ifndef _CNetworkManager_H
#define _CNetworkManager_H

#define _WINSOCKAPI_  // stops windows.h including winsock.h

#include <Workphone/Interface/Net/INetworkManager.hpp>
#include <RakPeerInterface.h>

namespace workphone
{
    namespace scene
    {
        class NetworkListener;
    }

    /**
     * @brief RakNet-backed implementation of INetworkManager.
     *
     * CNetworkManager wraps a @c RakNet::RakPeerInterface and maps the engine's
     * networking API onto RakNet's client/server/peer model.  It can operate in
     * three roles:
     *
     * - **Server** – calls @c setServer(true) or @c setServer; binds a UDP
     *   socket on the configured port and accepts up to @c maxClients connections.
     * - **Client** – calls @c setServer(false); starts a single outbound socket
     *   and connects to a server with @c connect().
     * - **Peer** – calls @c setPeer(); binds a socket that can both accept
     *   incoming connections and initiate outgoing ones simultaneously.
     *
     * Packet dispatch is driven by @c update(), which must be called every
     * game/simulation tick.  All RakNet system messages (connect, disconnect,
     * failure) are translated into @c INetworkListener callbacks so that higher-
     * level code stays transport-agnostic.
     *
     * ### Typical server setup
     * @code
     * auto net = make_ptr<CNetworkManager>();
     * net->setPort( 15822 );
     * net->SetMaxClients( 16 );
     * net->setListener( myListener );
     * net->setServer( true );
     * @endcode
     *
     * ### Typical client setup
     * @code
     * auto net = make_ptr<CNetworkManager>( true );  // isClient = true
     * net->setListener( myListener );
     * net->setServer( false );
     * net->connect( "192.168.1.10", 15822, "" );
     * @endcode
     *
     * @see INetworkManager, INetworkListener, CPacket, CSystemAddress
     */
    class CNetworkManager : public INetworkManager
    {
    public:
        /// @brief Creates a manager that defaults to client mode.
        CNetworkManager();

        /**
         * @brief Creates a manager and immediately sets the client/server role.
         *
         * @param isClient  Pass @c true to start in client mode, @c false for server mode.
         *                  The underlying @c RakPeerInterface is created but @e not started
         *                  yet; call @c setServer() or @c connect() to activate it.
         */
        explicit CNetworkManager( bool isClient );

        ~CNetworkManager() override;

        // ------------------------------------------------------------------
        // INetworkManager overrides
        // ------------------------------------------------------------------

        /// @copydoc INetworkManager::setServer
        void setServer( bool isServer ) override;

        /// @copydoc INetworkManager::isServer
        bool isServer() const override;

        /// @copydoc INetworkManager::setPeer
        void setPeer() override;

        /// @copydoc INetworkManager::update
        void update() override;

        /// @copydoc INetworkManager::setVerbose
        void setVerbose( bool isverbose ) override;

        /// @copydoc INetworkManager::setNetIterations
        void setNetIterations( u16 iterations ) override;

        /// @copydoc INetworkManager::setListener
        void setListener( SmartPtr<INetworkListener> netCallback ) override;

        /// @copydoc INetworkManager::addListener
        void addListener( SmartPtr<INetworkListener> netCallback ) override;

        /// @copydoc INetworkManager::removeListener
        void removeListener( SmartPtr<INetworkListener> netCallback ) override;

        /// @copydoc INetworkManager::setGlobalPacketRelay
        void setGlobalPacketRelay( bool relay ) override;

        /// @copydoc INetworkManager::sendPacket(SmartPtr<IPacket>&)
        void sendPacket( SmartPtr<IPacket> &outpacket ) override;

        /// @copydoc INetworkManager::sendPacket(SmartPtr<IPacket>&,u16)
        void sendPacket( SmartPtr<IPacket> &outpacket, u16 playerId ) override;

        /// @copydoc INetworkManager::sendPacket(SmartPtr<IPacket>&,SmartPtr<ISystemAddress>)
        void sendPacket( SmartPtr<IPacket> &outpacket, SmartPtr<ISystemAddress> systemAddress ) override;

        /// @copydoc INetworkManager::sendPacketToAllExcept
        void sendPacketToAllExcept( SmartPtr<IPacket> &outpacket,
                                    SmartPtr<ISystemAddress> systemAddress ) override;

        /// @copydoc INetworkManager::sendPacketUnreliable(SmartPtr<IPacket>&)
        void sendPacketUnreliable( SmartPtr<IPacket> &outpacket ) override;

        /// @copydoc INetworkManager::sendPacketUnreliable(SmartPtr<IPacket>&,u16)
        void sendPacketUnreliable( SmartPtr<IPacket> &outpacket, u16 playerId ) override;

        /// @copydoc INetworkManager::kickClient
        void kickClient( u16 playerId, bool hardKick ) override;

        /// @copydoc INetworkManager::getPeerCount
        [[nodiscard]] const u32 getPeerCount() override;

        /// @copydoc INetworkManager::getPing
        [[nodiscard]] const u32 getPing() override;

        /// @copydoc INetworkManager::getClientAddress
        [[nodiscard]] const u32 getClientAddress( u16 playerId ) override;

        /// @copydoc INetworkManager::getLocalRakNetGUID
        [[nodiscard]] String getLocalRakNetGUID() override;

        /// @copydoc INetworkManager::getPlayerNumber
        [[nodiscard]] u16 getPlayerNumber() const override;

        /// @copydoc INetworkManager::connect
        void connect( const String &address, unsigned short port, const String &password ) override;

        /// @copydoc INetworkManager::getConnectionStatus
        [[nodiscard]] ConnectionStatus getConnectionStatus() override;

        /// @copydoc INetworkManager::createPacket
        [[nodiscard]] SmartPtr<IPacket> createPacket() override;

        /// @copydoc INetworkManager::getServerTime
        [[nodiscard]] time_interval getServerTime() const override;

        // ------------------------------------------------------------------
        // RakNet-specific accessors (not part of INetworkManager)
        // ------------------------------------------------------------------

        /**
         * @brief Returns the UDP port the peer interface is bound to.
         *
         * Set before calling @c setServer() or @c setPeer().  Changing the port
         * after startup has no effect on the running socket.
         */
        [[nodiscard]] u32 getPort();

        /**
         * @brief Sets the UDP port that will be used when starting the peer interface.
         *
         * @param value  Port number (1–65535).  Default is @c 15822.
         */
        void setPort( u32 value );

        /**
         * @brief Returns the maximum number of simultaneous client connections.
         *
         * Only meaningful in server or peer mode.
         */
        [[nodiscard]] u32 GetMaxClients();

        /**
         * @brief Sets the maximum number of simultaneous client connections.
         *
         * @param value  Maximum client count.  Default is @c 15.
         *               Must be set before calling @c setServer() or @c setPeer().
         */
        void SetMaxClients( u32 value );

        [[nodiscard]] bool getVerbose() const;
        [[nodiscard]] bool getGlobalPacketRelay() const;
        [[nodiscard]] u16 getNetIterations() const;
        [[nodiscard]] SmartPtr<INetworkListener> getListener() const;
        [[nodiscard]] bool getConnected() const;
        void setConnected( bool connected );
        [[nodiscard]] bool getStarted() const;
        void setStarted( bool started );
        [[nodiscard]] RakNet::SystemAddress getServerAddress() const;
        void setServerAddress( const RakNet::SystemAddress &serverAddress );
        void setPlayerNumber( u16 playerNumber );

        /**
         * @brief Returns the underlying @c RakNet::RakPeerInterface.
         *
         * Exposes the raw RakNet interface for advanced operations not covered
         * by INetworkManager (e.g. NAT punch-through, statistics queries).
         * Handle with care — bypassing the manager's packet loop can break
         * listener dispatching.
         */
        [[nodiscard]] RakNet::RakPeerInterface *GetRakPeerInterface();
        void SetRakPeerInterface( RakNet::RakPeerInterface *rakInterface );

    private:
        enum class SendReliability
        {
            Reliable,
            Unreliable
        };

        [[nodiscard]] bool startPeer( bool serverMode, u32 maxConnections );
        bool sendPacketInternal( SmartPtr<IPacket> &outpacket, const RakNet::SystemAddress &address,
                                 bool broadcast, SendReliability reliability );
        [[nodiscard]] RakNet::SystemAddress getAddressForPlayer( u16 playerId ) const;
        [[nodiscard]] RakNet::SystemAddress getDefaultSendAddress() const;

        /// Underlying RakNet peer interface; owns its lifetime via DestroyInstance.
        RakNet::RakPeerInterface *m_rakInterface = nullptr;

        /// Registered event listener for packet, connect, and disconnect callbacks.
        SmartPtr<scene::NetworkListener> m_netCallback;

        /// Cached system address of the server (client-side only).
        RakNet::SystemAddress m_serverAddress;

        bool m_isServer = false;      ///< True when operating as a server.
        bool m_verbose = false;       ///< True to emit debug output.
        bool m_globalRelay = false;   ///< True to relay all data packets to all peers.
        u32 m_port = 15822;           ///< Bound UDP port.
        u32 m_maxClients = 15;        ///< Maximum simultaneous connections.
        u16 m_playerNumber = 0;       ///< Server-assigned player index (client-side).
        u16 m_netIterations = 10000;  ///< Max packets processed per @c update() call.

        /// True once the client has received @c ID_CONNECTION_REQUEST_ACCEPTED.
        bool m_connected = false;

        bool m_started = false;
    };

}  // namespace workphone

#endif
