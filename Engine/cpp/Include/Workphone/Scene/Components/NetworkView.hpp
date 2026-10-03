#ifndef NetworkView_h__
#define NetworkView_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/Net/INetworkListener.hpp>
#include <Workphone/Interface/Net/INetworkView.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class NetworkView
         * @brief Scene component that enables network synchronization for an actor.
         *
         * NetworkView attaches to a scene actor and provides the plumbing for
         * state serialization (OnSerializeView / OnDeserializeView), remote
         * procedure calls (RPC), and ownership tracking in a client-server or
         * peer-to-peer session.
         *
         * Each NetworkView is identified by a unique integer view ID that is
         * assigned at runtime and must be consistent across all connected peers.
         * The component registers itself as a network listener with the active
         * INetworkManager and dispatches incoming packets to the appropriate
         * handler (state-sync or RPC).
         *
         * @note Only one NetworkView per actor is supported.
         */
        class WPCore_API NetworkView : public Component
        {
        public:
            /** Selects which peer is allowed to publish this actor's state. */
            enum class AuthorityMode
            {
                Owner,
                Server,
                AnyPeer,
                Count
            };

            /** Selects the transport delivery policy for state snapshots. */
            enum class DeliveryMode
            {
                Unreliable,
                Reliable,
                Count
            };

            static const String viewIdStr;
            static const String ownerIdStr;
            static const String sceneViewStr;
            static const String replicationEnabledStr;
            static const String authorityModeStr;
            static const String deliveryModeStr;
            static const String sendRateStr;
            static const String syncPositionStr;
            static const String syncRotationStr;
            static const String syncScaleStr;
            static const String allowOwnershipRequestsStr;

            static const Array<String> authorityModeNames;
            static const Array<String> deliveryModeNames;

            class CNetworkView : public INetworkView
            {
            public:
                CNetworkView();

                CNetworkView( NetworkView *owner );

                ~CNetworkView() override;

                void RPC( SmartPtr<IPacket> packet ) override;

                void serializeView() override;

                void deserializeView( SmartPtr<IPacket> packet ) override;

                void onSerializeView( SmartPtr<INetworkStream> stream ) override;

                s32 getViewId() const override;

                void setViewId( s32 viewId ) override;

                u32 getOwnerId() const override;

                void setOwnerId( u32 ownerId ) override;

                bool isMine() const override;

                void transferOwnership( u32 newOwnerId ) override;

                void requestOwnership() override;

                bool isSceneView() const override;

                NetworkView *getOwner() const;

                void setOwner( NetworkView *owner );

            private:
                NetworkView *m_owner = nullptr;
            };

            /**
             * @class Listener
             * @brief Internal INetworkListener that routes packets to this NetworkView.
             */
            class Listener : public INetworkListener
            {
            public:
                explicit Listener( NetworkView *owner );
                ~Listener() override;

                void handlePacket( SmartPtr<IPacket> packet ) override;
                void connect( u32 playerId ) override;
                void disconnect( u32 playerId ) override;

                WP_CLASS_REGISTER_DECL;

            private:
                NetworkView *m_owner = nullptr;
            };

            NetworkView();
            ~NetworkView() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void update() override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            // INetworkView overrides -------------------------------------------

            /**
             * @brief Issues a remote procedure call to all connected peers.
             *
             * The packet is sent reliably via the active INetworkManager.
             * The view ID is written as the first field so that the receiving
             * peer can dispatch the packet to the correct NetworkView.
             *
             * @param packet  Packet to transmit. Must not be null.
             */
            void RPC( SmartPtr<IPacket> packet );

            /**
             * @brief Sends the current serialized state to all peers.
             *
             * Called each network update tick by the owning system. Writes the
             * view ID followed by the actor's state into a new packet and
             * dispatches it unreliably.
             */
            void serializeView();

            /**
             * @brief Applies a received state packet to the local actor.
             *
             * @param packet  Incoming packet whose read cursor is positioned
             *                immediately after the view-ID field.
             */
            void deserializeView( SmartPtr<IPacket> packet );

            /**
             * @brief User-code hook called each network tick for custom state sync.
             *
             * Mirrors @c OnPhotonSerializeView in Unity PUN. Override in a
             * subclass or delegate to an attached component to push/pull
             * arbitrary per-actor state through the shared stream.
             *
             * The default implementation is a no-op so that NetworkView can be
             * used stand-alone for transform-only synchronisation.
             *
             * @param stream  Typed serialization stream in writing or reading mode.
             */
            void onSerializeView( SmartPtr<INetworkStream> stream );

            /** @return The unique network view identifier for this component. */
            s32 getViewId() const;

            /** @param viewId  New view identifier (must be unique across the session). */
            void setViewId( s32 viewId );

            /** @return The player ID of the peer that owns (has authority over) this view. */
            u32 getOwnerId() const;

            /** @param ownerId  Player ID of the authoritative peer. */
            void setOwnerId( u32 ownerId );

            /**
             * @return True if this peer is the owner of the view (i.e. the local player
             *         ID matches the stored owner ID).
             */
            bool isMine() const;

            /**
             * @brief Transfers ownership of this view to another peer.
             *
             * Sends a reliable ownership-transfer packet via the network manager
             * and updates the local owner ID.
             *
             * @param newOwnerId  Player ID of the new owner.
             */
            void transferOwnership( u32 newOwnerId );

            /**
             * @brief Requests ownership of this view from the current owner.
             *
             * Sends a reliable ownership-request packet via the network manager.
             * The current owner (or master client) decides whether to grant it.
             */
            void requestOwnership();

            /**
             * @return True if this is a scene view not associated with a specific player.
             *
             * Scene views have an owner ID of 0 (master client / scene authority).
             */
            bool isSceneView() const;

            bool isReplicationEnabled() const;
            void setReplicationEnabled( bool enabled );

            AuthorityMode getAuthorityMode() const;
            void setAuthorityMode( AuthorityMode authorityMode );

            DeliveryMode getDeliveryMode() const;
            void setDeliveryMode( DeliveryMode deliveryMode );

            f32 getSendRate() const;
            void setSendRate( f32 sendRate );

            bool getSyncPosition() const;
            void setSyncPosition( bool syncPosition );

            bool getSyncRotation() const;
            void setSyncRotation( bool syncRotation );

            bool getSyncScale() const;
            void setSyncScale( bool syncScale );

            bool getAllowOwnershipRequests() const;
            void setAllowOwnershipRequests( bool allowOwnershipRequests );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Dispatches a received packet to either deserializeView or an RPC handler. */
            void handlePacket( SmartPtr<IPacket> packet );

            /** Returns whether this peer may publish snapshots for the configured authority mode. */
            bool hasSendAuthority() const;

            SmartPtr<Listener> m_listener;
            SmartPtr<INetworkManager> m_networkManager;

            s32 m_viewId = -1;
            u32 m_ownerId = 0;
            bool m_sceneView = false;
            bool m_replicationEnabled = true;
            AuthorityMode m_authorityMode = AuthorityMode::Server;
            DeliveryMode m_deliveryMode = DeliveryMode::Unreliable;
            f32 m_sendRate = 20.0f;
            f32 m_sendAccumulator = 0.0f;
            u32 m_outgoingSequence = 0;
            u32 m_lastReceivedSequence = 0;
            bool m_hasReceivedSnapshot = false;
            bool m_syncPosition = true;
            bool m_syncRotation = true;
            bool m_syncScale = false;
            bool m_allowOwnershipRequests = false;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // NetworkView_h__
