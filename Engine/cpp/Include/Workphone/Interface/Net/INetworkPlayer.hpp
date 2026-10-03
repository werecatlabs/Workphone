#ifndef INetworkPlayer_h__
#define INetworkPlayer_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /**
     * @brief Represents a single participant in a network session.
     *
     * INetworkPlayer models one connected peer, mirroring Unity PUN's
     * @c PhotonPlayer / @c Player concept.  Each player is identified by a
     * unique @e actor number assigned by the server at connect time, carries an
     * optional human-readable @e nick name, and exposes connection-quality
     * information (ping) and ownership flags (local, master client).
     *
     * Custom per-player state (e.g. score, team, skin) can be stored in an
     * opaque @c Properties bag and synchronised via the network manager.
     *
     * ### Typical usage
     * @code
     * // Server creates a player record when a peer connects:
     * auto player = factoryManager->make_ptr<NetworkPlayer>();
     * player->setActorNumber( assignedId );
     * player->setNickName( "Player_" + std::to_string( assignedId ) );
     * player->setLocal( false );
     *
     * // Querying the local client's own record:
     * if ( player->isLocal() )
     *     sendWelcome( player->getNickName() );
     * @endcode
     *
     * @see INetworkManager, INetworkListener
     */
    class WPCore_API INetworkPlayer : public ISharedObject
    {
    public:
        ~INetworkPlayer() override;

        // ------------------------------------------------------------------
        // Identity
        // ------------------------------------------------------------------

        /**
         * @brief Returns the server-assigned actor number for this player.
         *
         * Actor numbers start at 0 and are unique within a session.  They
         * match the @c playerId values delivered by
         * INetworkListener::connect() / disconnect().
         *
         * @return Player actor number, or @c -1 if not yet assigned.
         */
        virtual s32 getActorNumber() const = 0;

        /**
         * @brief Sets the actor number for this player.
         *
         * @param actorNumber Server-assigned player index (>= 0).
         */
        virtual void setActorNumber( s32 actorNumber ) = 0;

        /**
         * @brief Returns the human-readable display name of this player.
         *
         * The nick name is set by the client before joining and broadcast to
         * all peers.  It is not guaranteed to be unique.
         *
         * @return Display name string, may be empty.
         */
        virtual String getNickName() const = 0;

        /**
         * @brief Sets the display name of this player.
         *
         * @param nickName Human-readable name (UTF-8).
         */
        virtual void setNickName( const String &nickName ) = 0;

        /**
         * @brief Returns a unique string identifier for this player.
         *
         * The user ID is typically a persistent account identifier (e.g. a GUID
         * or platform user ID) supplied by the client and forwarded by the server.
         * It survives reconnects unlike the actor number.
         *
         * @return Persistent user ID string, may be empty.
         */
        virtual String getUserId() const = 0;

        /**
         * @brief Sets the persistent user ID.
         *
         * @param userId Persistent identifier string.
         */
        virtual void setUserId( const String &userId ) = 0;

        // ------------------------------------------------------------------
        // Connection quality
        // ------------------------------------------------------------------

        /**
         * @brief Returns the round-trip time to this player in milliseconds.
         *
         * For the local player this is 0.  For remote players this reflects
         * the most recently measured ping value obtained from the network layer.
         *
         * @return Ping in milliseconds.
         */
        virtual u32 getPing() const = 0;

        /**
         * @brief Updates the cached ping value for this player.
         *
         * Called by the network manager whenever a fresh measurement is
         * available.
         *
         * @param ping Round-trip time in milliseconds.
         */
        virtual void setPing( u32 ping ) = 0;

        // ------------------------------------------------------------------
        // Ownership / role flags
        // ------------------------------------------------------------------

        /**
         * @brief Returns whether this player record represents the local peer.
         *
         * The local player is the one running the current process.  Only one
         * player in a session can be local.
         *
         * @return @c true if this is the local player.
         */
        virtual bool isLocal() const = 0;

        /**
         * @brief Marks this player record as local or remote.
         *
         * @param local @c true to mark as the local player.
         */
        virtual void setLocal( bool local ) = 0;

        /**
         * @brief Returns whether this player is currently the master client.
         *
         * The master client is the authoritative peer responsible for game-
         * state decisions (e.g. spawning, scene loading).  In a server-hosted
         * session this is always the server itself; in peer-to-peer sessions
         * the role migrates to the next connected peer if the current master
         * disconnects.
         *
         * @return @c true if this player is the master client.
         */
        virtual bool isMasterClient() const = 0;

        /**
         * @brief Sets the master-client flag for this player.
         *
         * @param master @c true to designate this player as master client.
         */
        virtual void setMasterClient( bool master ) = 0;

        // ------------------------------------------------------------------
        // Custom properties
        // ------------------------------------------------------------------

        /**
         * @brief Returns the custom property bag for this player.
         *
         * Custom properties allow arbitrary key-value data (score, team,
         * avatar colour, etc.) to be associated with a player and synchronised
         * across the session.
         *
         * @return Shared pointer to the Properties container (never null after
         *         construction).
         */
        virtual SmartPtr<Properties> getCustomProperties() const = 0;

        /**
         * @brief Replaces the custom property bag for this player.
         *
         * @param properties New property set to assign.
         */
        virtual void setCustomProperties( SmartPtr<Properties> properties ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // INetworkPlayer_h__
