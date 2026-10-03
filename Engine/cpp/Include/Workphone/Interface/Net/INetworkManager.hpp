#ifndef _INetworkManager_H
#define _INetworkManager_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Triangle3.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /**
     * @brief Network manager interface for handling client-server communications.
     *
     * The INetworkManager provides a comprehensive interface for managing network connections,
     * packet transmission, and client management in both server and client modes. It supports
     * reliable and unreliable packet delivery, connection status monitoring, and various
     * network configuration options.
     *
     * @note This interface abstracts the underlying network implementation and provides
     * a unified API for network operations regardless of the networking library used.
     *
     * @see INetworkListener, IPacket, ISystemAddress
     */
    class WPCore_API INetworkManager : public ISharedObject
    {
    public:
        /**
         * @brief Connection status enumeration.
         *
         * Represents the current state of the network connection.
         */
        enum class ConnectionStatus
        {
            NCS_PENDING,      ///< The connection is still in progress
            NCS_ESTABLISHED,  ///< The connection has been successfully established
            NCS_FAILED,       ///< The connection has failed or the NetManager is invalid

            NCS_COUNT  ///< Number of connection status values
        };

        /// @brief Virtual destructor
        ~INetworkManager() override;

        /**
         * @brief Sets whether this instance operates as a server.
         *
         * @param isServer True to operate as server, false for client mode
         *
         * @note Server mode allows accepting connections from multiple clients,
         * while client mode connects to a single server.
         */
        virtual void setServer( bool isServer ) = 0;

        /**
         * @brief Checks if this instance is operating as a server.
         *
         * @return True if operating as server, false if client
         */
        virtual bool isServer() const = 0;

        /**
         * @brief Configures this instance to operate in peer-to-peer mode.
         *
         * @note In peer mode, the instance can both accept connections and
         * connect to other peers simultaneously.
         */
        virtual void setPeer() = 0;

        /**
         * @brief Enables or disables verbose debug output.
         *
         * When enabled, the network manager will output debug information to the console,
         * including packet transmission details and connection events.
         *
         * Example output:
         * - "irrNetLite: A packet of length 50 was received."
         * - "irrNetLite: Player number 23 disconnected."
         *
         * @param isverbose True to enable verbose output, false to disable
         *
         * @note Useful for debugging network issues and monitoring traffic
         */
        virtual void setVerbose( bool isverbose ) = 0;

        /**
         * @brief Sets the network event listener.
         *
         * The listener will receive callbacks for network events such as
         * packet reception, client connections, and disconnections.
         *
         * @param netCallback Smart pointer to the network listener implementation
         *
         * @see INetworkListener
         */
        virtual void setListener( SmartPtr<INetworkListener> netCallback ) = 0;

        /**
         * @brief Adds a listener without replacing existing callback targets.
         *

         * * This is the preferred registration path for actor components because
         * several
         * NetworkView instances may be active at the same time.
         * Duplicate registrations are
         * ignored.
         */
        virtual void addListener( SmartPtr<INetworkListener> netCallback ) = 0;

        /**
         * @brief Removes one previously registered callback target.
         *
         *
         * Has no effect when the listener is null or is not registered.
         */
        virtual void removeListener( SmartPtr<INetworkListener> netCallback ) = 0;

        /**
         * @brief Sends a packet reliably to connected peers.
         *
         * This function provides reliable packet delivery with automatic retransmission
         * and ordering guarantees. For servers, the packet is sent to all connected clients.
         * For clients, the packet is sent directly to the server.
         *
         * @param outpacket Smart pointer to the packet to send
         *
         * @note Reliable transmission is recommended for critical game data
         * but has higher overhead than unreliable transmission.
         *
         * @see sendPacketUnreliable(), IPacket
         */
        virtual void sendPacket( SmartPtr<IPacket> &outpacket ) = 0;

        /**
         * @brief Sends a packet reliably to a specific player.
         *
         * This function sends a packet to a specific client identified by player ID.
         * Players are automatically assigned IDs as they connect to the server.
         *
         * @param outpacket Smart pointer to the packet to send
         * @param playerId ID of the target player (0-based)
         *
         * \warning This function is only valid when operating as a server.
         * Clients can only send packets to the server using sendPacket().
         *
         * \see getPeerCount(), getPlayerNumber()
         */
        virtual void sendPacket( SmartPtr<IPacket> &outpacket, u16 playerId ) = 0;

        /**
         * \brief Sends a packet to a specific network address.
         *
         * This function allows sending packets to a specific network endpoint
         * identified by system address, providing more granular control over
         * packet destinations.
         *
         * \param outpacket Smart pointer to the packet to send
         * \param systemAddress Target system address
         *
         * \see ISystemAddress
         */
        virtual void sendPacket( SmartPtr<IPacket> &outpacket,
                                 SmartPtr<ISystemAddress> systemAddress ) = 0;

        /**
         * \brief Sends a packet to all connected clients except one.
         *
         * This function is useful for broadcasting information to all clients
         * except the sender or a specific client that should be excluded.
         *
         * \param outpacket Smart pointer to the packet to send
         * \param systemAddress System address to exclude from broadcast
         *
         * \note Only valid when operating as a server
         */
        virtual void sendPacketToAllExcept( SmartPtr<IPacket> &outpacket,
                                            SmartPtr<ISystemAddress> systemAddress ) = 0;

        /**
         * \brief Sends a packet unreliably to connected peers.
         *
         * Unreliable transmission provides faster delivery with lower overhead
         * but without delivery guarantees or automatic retransmission.
         *
         * \param outpacket Smart pointer to the packet to send
         *
         * \warning Not recommended for critical information as packets may be lost.
         * Use for frequently updated data like position updates where occasional
         * loss is acceptable.
         *
         * \see sendPacket()
         */
        virtual void sendPacketUnreliable( SmartPtr<IPacket> &outpacket ) = 0;

        /**
         * \brief Sends a packet unreliably to a specific player.
         *
         * Combines unreliable transmission with targeted delivery to a specific client.
         *
         * \param outpacket Smart pointer to the packet to send
         * \param playerId ID of the target player
         *
         * \warning Only valid for servers. Not recommended for critical information.
         *
         * \see sendPacketUnreliable(), sendPacket()
         */
        virtual void sendPacketUnreliable( SmartPtr<IPacket> &outpacket, u16 playerId ) = 0;

        /**
         * \brief Gets the number of connected peers.
         *
         * \return Number of connected clients
         *
         * \note This function is only valid for servers. For clients, use
         * getConnectionStatus() to check if connected to a server.
         */
        virtual const u32 getPeerCount() = 0;

        /**
         * \brief Gets the local player ID.
         *
         * \return The player ID assigned to this client by the server
         *
         * \note This function is only valid for clients. Server instances
         * don't have player IDs.
         */
        virtual u16 getPlayerNumber() const = 0;

        /**
         * \brief Gets a client's IP address by player ID.
         *
         * \param playerId ID of the target player
         * \return IP address in 32-bit integer format
         *
         * \note Only valid for servers. The address is in network byte order.
         */
        virtual const u32 getClientAddress( u16 playerId ) = 0;

        /**
         * \brief Gets the local RakNet GUID.
         *
         * \return String representation of the local RakNet globally unique identifier
         *
         * \note The GUID is used for peer identification in RakNet-based implementations
         */
        virtual String getLocalRakNetGUID() = 0;

        /**
         * \brief Kicks a client from the server.
         *
         * \param playerId ID of the client to disconnect
         * \param hardKick If true, forcefully disconnects immediately without generating events
         *
         * \note When hardKick is false, a disconnect event will be generated for proper cleanup.
         * Hard kicks bypass normal disconnection procedures.
         *
         * \warning Only valid for servers
         */
        virtual void kickClient( u16 playerId, bool hardKick = false ) = 0;

        /**
         * \brief Gets the current connection status.
         *
         * \return Current connection state
         *
         * The returned status indicates:
         * - NCS_PENDING: Connection attempt in progress
         * - NCS_ESTABLISHED: Successfully connected
         * - NCS_FAILED: Connection failed or disconnected
         *
         * \note Primarily useful for clients, but servers can also check for setup failures
         *
         * \see ConnectionStatus
         */
        virtual ConnectionStatus getConnectionStatus() = 0;

        /**
         * \brief Initiates a connection to a remote server.
         *
         * \param address Server IP address or hostname
         * \param port Server port number
         * \param password Connection password (empty string if no password required)
         *
         * \note This function is asynchronous. Use getConnectionStatus() to monitor
         * the connection progress.
         *
         * \see getConnectionStatus(), ConnectionStatus
         */
        virtual void connect( const String &address, unsigned short port, const String &password ) = 0;

        /**
         * \brief Gets the network ping time.
         *
         * \return Ping time in milliseconds
         *
         * For clients, returns the ping to the server.
         * For servers, returns the ping to the first connected player.
         *
         * \note Useful for monitoring connection quality and implementing
         * lag compensation systems.
         */
        virtual const u32 getPing() = 0;

        /**
         * \brief Sets the number of packet processing iterations per update.
         *
         * \param iterations Number of packets to process per update cycle
         *
         * Increase this value if packets are not being processed quickly enough
         * due to infrequent updates. Higher values provide better throughput
         * but may impact performance.
         *
         * \note Default value is typically 10000
         */
        virtual void setNetIterations( u16 iterations ) = 0;

        /**
         * \brief Enables or disables global packet relay.
         *
         * When enabled, the server automatically relays all received packets
         * to all connected clients. This simplifies client-to-client communication
         * but may not be suitable for large-scale applications due to bandwidth usage.
         *
         * \param relay True to enable global packet relay, false to disable
         *
         * \note Default is false. Recommended for applications with fewer than
         * 10 peers. For larger applications, implement custom packet routing.
         *
         * \warning Only affects server behavior
         */
        virtual void setGlobalPacketRelay( bool relay ) = 0;

        /**
         * \brief Creates a new packet instance.
         *
         * \return Smart pointer to a new packet object
         *
         * Factory method for creating packet objects compatible with this
         * network manager implementation.
         *
         * \see IPacket
         */
        virtual SmartPtr<IPacket> createPacket() = 0;

        /**
         * \brief Gets the server's synchronized time.
         *
         * \return Server time as a time interval
         *
         * This function provides access to synchronized time information
         * from the server, useful for time-critical applications and
         * synchronization between clients.
         *
         * \note The time format and epoch depend on the specific implementation
         */
        virtual time_interval getServerTime() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif
