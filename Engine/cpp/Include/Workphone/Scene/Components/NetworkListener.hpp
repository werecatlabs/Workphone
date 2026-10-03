#ifndef NetworkListener_h__
#define NetworkListener_h__

#include <Workphone/Interface/Net/INetworkListener.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class NetworkListener
         * @brief Thread-safe multiplexing dispatcher for INetworkListener events.
         *
         * A single NetworkListener instance is registered with INetworkManager via
         * INetworkManager::setListener(). Any number of INetworkListener children can
         * be added or removed at runtime; every incoming event is forwarded to each of
         * them in registration order.
         *
         * This mirrors Unity PUN's callback-target system
         * (@c PhotonNetwork.AddCallbackTarget / @c RemoveCallbackTarget), where
         * multiple MonoBehaviours can independently receive network events.
         *
         * ### Typical setup
         * @code
         * // At initialisation
         * auto listener = factoryManager->make_ptr<NetworkListener>();
         * networkManager->setListener( listener );
         *
         * // When a NetworkView is loaded
         * listener->addListener( networkViewListener );
         *
         * // When a NetworkView is unloaded
         * listener->removeListener( networkViewListener );
         * @endcode
         *
         * @note Adding or removing listeners while a dispatch is in progress is safe
         *       because the internal list is a ConcurrentArray snapshot-copy on read.
         *
         * @see INetworkListener, INetworkManager::setListener
         */
        class WPCore_API NetworkListener : public INetworkListener
        {
        public:
            NetworkListener();
            ~NetworkListener() override;

            // ------------------------------------------------------------------
            // INetworkListener
            // ------------------------------------------------------------------

            /**
             * @brief Forwards the packet to every registered child listener.
             * @param packet  Received application-layer packet.
             */
            void handlePacket( SmartPtr<IPacket> packet ) override;

            /**
             * @brief Notifies every registered child listener of a new connection.
             * @param playerId  Index of the newly connected peer.
             */
            void connect( u32 playerId ) override;

            /**
             * @brief Notifies every registered child listener of a disconnection.
             * @param playerId  Index of the disconnected peer.
             */
            void disconnect( u32 playerId ) override;

            // ------------------------------------------------------------------
            // Listener management
            // ------------------------------------------------------------------

            /**
             * @brief Registers a child listener to receive future network events.
             *
             * Has no effect if @p listener is null or already registered.
             *
             * @param listener  Listener to add.
             */
            void addListener( SmartPtr<INetworkListener> listener );

            /**
             * @brief Unregisters a previously added child listener.
             *
             * Has no effect if @p listener is null or not currently registered.
             *
             * @param listener  Listener to remove.
             */
            void removeListener( SmartPtr<INetworkListener> listener );

            /**
             * @brief Removes all registered child listeners.
             */
            void clearListeners();

            /**
             * @brief Returns whether the given listener is currently registered.
             * @param listener  Listener to query.
             * @return True if @p listener is in the registered list.
             */
            bool hasListener( SmartPtr<INetworkListener> listener ) const;

            /**
             * @brief Returns the number of currently registered child listeners.
             */
            u32 getListenerCount() const;

            WP_CLASS_REGISTER_DECL;

        private:
            ConcurrentArray<SmartPtr<INetworkListener>> m_listeners;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // NetworkListener_h__
