#ifndef INetworkView_h__
#define INetworkView_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Net/INetworkStream.hpp>

namespace workphone
{

    /**
     * @brief Pure-virtual interface for a network-synchronised view object.
     *
     * INetworkView mirrors Unity PUN's @c PhotonView.  Every networked actor
     * owns exactly one view, which is identified by a session-unique @e view ID
     * and an @e owner ID that names the authoritative peer.
     *
     * The interface provides:
     * - **State synchronisation** – @c serializeView / @c deserializeView move
     *   transform and custom state over the wire each network tick.
     * - **Remote procedure calls** – @c RPC dispatches a pre-built packet to
     *   all connected peers reliably.
     * - **Ownership management** – @c isMine, @c transferOwnership and
     *   @c requestOwnership mirror PUN's ownership-transfer API.
     * - **User callbacks** – @c onSerializeView is called each tick so that
     *   user code (e.g. a gameplay component) can push/pull its own state into
     *   the shared stream without subclassing the view directly.
     *
     * ### Typical usage
     * @code
     * // In a gameplay component attached to the same actor:
     * void MyComponent::onSerializeView( SmartPtr<INetworkStream> stream )
     * {
     *     if ( stream->isWriting() )
     *         stream->write( m_health );
     *     else
     *         stream->read( m_health );
     * }
     * @endcode
     *
     * @see INetworkManager, INetworkStream, INetworkListener, NetworkView
     */
    class WPCore_API INetworkView : public ISharedObject
    {
    public:
        ~INetworkView() override;

        // ------------------------------------------------------------------
        // Remote procedure calls
        // ------------------------------------------------------------------

        /**
         * @brief Sends a pre-built packet to all connected peers reliably.
         *
         * The implementation must write the view ID as the first field of the
         * packet so that the receiving peer can route it to the correct view.
         *
         * @param packet  Packet to transmit.  Must not be null.
         */
        virtual void RPC( SmartPtr<IPacket> packet ) = 0;

        // ------------------------------------------------------------------
        // State synchronisation
        // ------------------------------------------------------------------

        /**
         * @brief Serializes and sends the current actor state to all peers.
         *
         * Called each network-update tick by the owning system.  Implementations
         * should build a packet containing the view ID and any per-tick state
         * (position, rotation, custom fields) and dispatch it unreliably.
         */
        virtual void serializeView() = 0;

        /**
         * @brief Applies a received state packet to the local actor.
         *
         * @param packet  Incoming packet whose read cursor is positioned
         *                immediately after the view-ID field.
         */
        virtual void deserializeView( SmartPtr<IPacket> packet ) = 0;

        /**
         * @brief User-code hook called each network tick for custom state sync.
         *
         * Mirrors @c OnPhotonSerializeView in Unity PUN.  The @p stream is in
         * @e writing mode when this peer is the owner (authoritative), and in
         * @e reading mode on all other peers.
         *
         * @param stream  Typed serialization stream; never null.
         */
        virtual void onSerializeView( SmartPtr<INetworkStream> stream ) = 0;

        // ------------------------------------------------------------------
        // Identity
        // ------------------------------------------------------------------

        /**
         * @brief Returns the unique network view identifier for this view.
         *
         * View IDs must be consistent across all connected peers.  A value of
         * @c -1 indicates that no ID has been assigned yet.
         */
        virtual s32 getViewId() const = 0;

        /**
         * @brief Sets the unique network view identifier.
         *
         * @param viewId  New view ID; must be unique within the session.
         */
        virtual void setViewId( s32 viewId ) = 0;

        // ------------------------------------------------------------------
        // Ownership
        // ------------------------------------------------------------------

        /**
         * @brief Returns the player ID of the peer that owns this view.
         *
         * The owner is the authoritative peer that writes state into
         * @c serializeView.  All other peers are in reading mode.
         */
        virtual u32 getOwnerId() const = 0;

        /**
         * @brief Sets the player ID of the authoritative peer.
         *
         * @param ownerId  Player ID of the new owner.
         */
        virtual void setOwnerId( u32 ownerId ) = 0;

        /**
         * @brief Returns @c true if this peer is the current owner of the view.
         *
         * Equivalent to @c photonView.IsMine in Unity PUN.
         */
        virtual bool isMine() const = 0;

        /**
         * @brief Transfers ownership of this view to another peer.
         *
         * Mirrors @c PhotonView.TransferOwnership.  The implementation should
         * notify the network manager so that all peers update their ownership
         * records.
         *
         * @param newOwnerId  Player ID of the peer that should become the owner.
         */
        virtual void transferOwnership( u32 newOwnerId ) = 0;

        /**
         * @brief Requests ownership of this view from the current owner.
         *
         * Mirrors @c PhotonView.RequestOwnership.  The local peer sends an
         * ownership-request packet; the current owner decides whether to grant
         * it (engine-policy dependent).
         */
        virtual void requestOwnership() = 0;

        // ------------------------------------------------------------------
        // Scene view
        // ------------------------------------------------------------------

        /**
         * @brief Returns @c true if this is a scene view (not player-owned).
         *
         * Scene views are placed in the scene at design time and are not owned
         * by any particular player — they are owned by the master client.
         * Mirrors the @c PhotonView.IsSceneView concept.
         */
        virtual bool isSceneView() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // INetworkView_h__
