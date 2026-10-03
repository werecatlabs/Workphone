#ifndef _INetworkListener_H
#define _INetworkListener_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @class INetworkListener
     * @brief Observer interface for network events dispatched by INetworkManager.
     *
     * Register an implementation with INetworkManager::setListener() to receive
     * callbacks for incoming packets, peer connections, and peer disconnections.
     *
     * This is the C++17 equivalent of Unity PUN's @c IPunCallbacks /
     * @c MonoBehaviourPunCallbacks pattern. Concrete classes (e.g. NetworkView::Listener,
     * game-level network controllers) implement only the methods they require.
     *
     * A NetworkListener multiplexer can be registered as the single INetworkManager
     * callback and will fan out events to every registered child listener.
     *
     * @see INetworkManager::setListener, NetworkListener
     */
    class WPCore_API INetworkListener : public ISharedObject
    {
    public:
        ~INetworkListener() override;

        /**
         * @brief Called for every application-level packet received from a peer.
         *
         * System-level RakNet messages (connect / disconnect etc.) are handled by
         * INetworkManager and never forwarded here. This method receives only
         * packets that were transmitted by the remote application layer.
         *
         * @param packet  Incoming packet. The read cursor is positioned at byte 0
         *                (i.e. including the message-type byte written by the sender).
         */
        virtual void handlePacket( SmartPtr<IPacket> packet ) = 0;

        /**
         * @brief Called when a peer successfully connects.
         *
         * On the server, this fires for each new client. On the client, it fires
         * once when the connection to the server is accepted.
         *
         * @param playerId  Zero-based index assigned to the connecting peer.
         *                  On the client side this is always 0 (the server).
         */
        virtual void connect( u32 playerId ) = 0;

        /**
         * @brief Called when a peer disconnects or is lost.
         *
         * On the server, this fires for each disconnecting client. On the client,
         * it fires once when the connection to the server is dropped.
         *
         * @param playerId  Index of the peer that disconnected.
         */
        virtual void disconnect( u32 playerId ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // _INetworkListener_H
