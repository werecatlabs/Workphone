#ifndef NetworkPlayer_h__
#define NetworkPlayer_h__

#include <Workphone/Interface/Net/INetworkPlayer.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Concrete implementation of INetworkPlayer.
         *
         * NetworkPlayer stores identity, connection-quality, and role data for
         * one participant in a network session.  It mirrors Unity PUN's
         * @c Player / @c PhotonPlayer object: every connected peer has exactly
         * one record on every other peer, and the local client also holds its
         * own record (marked with @c isLocal() == @c true).
         *
         * ### Lifecycle
         * Player records are created by the session controller when
         * INetworkListener::connect() fires, and destroyed when
         * INetworkListener::disconnect() fires.
         *
         * ### Custom properties
         * Arbitrary per-player data (score, team, skin, etc.) can be stored in
         * the Properties bag returned by getCustomProperties().  The session
         * controller is responsible for serialising and broadcasting property
         * changes to other peers.
         *
         * @see INetworkPlayer, INetworkManager, INetworkListener
         */
        class WPCore_API NetworkPlayer : public INetworkPlayer
        {
        public:
            NetworkPlayer();
            ~NetworkPlayer() override;

            // ------------------------------------------------------------------
            // INetworkPlayer — Identity
            // ------------------------------------------------------------------

            /** @copydoc INetworkPlayer::getActorNumber */
            s32 getActorNumber() const override;

            /** @copydoc INetworkPlayer::setActorNumber */
            void setActorNumber( s32 actorNumber ) override;

            /** @copydoc INetworkPlayer::getNickName */
            String getNickName() const override;

            /** @copydoc INetworkPlayer::setNickName */
            void setNickName( const String &nickName ) override;

            /** @copydoc INetworkPlayer::getUserId */
            String getUserId() const override;

            /** @copydoc INetworkPlayer::setUserId */
            void setUserId( const String &userId ) override;

            // ------------------------------------------------------------------
            // INetworkPlayer — Connection quality
            // ------------------------------------------------------------------

            /** @copydoc INetworkPlayer::getPing */
            u32 getPing() const override;

            /** @copydoc INetworkPlayer::setPing */
            void setPing( u32 ping ) override;

            // ------------------------------------------------------------------
            // INetworkPlayer — Ownership / role flags
            // ------------------------------------------------------------------

            /** @copydoc INetworkPlayer::isLocal */
            bool isLocal() const override;

            /** @copydoc INetworkPlayer::setLocal */
            void setLocal( bool local ) override;

            /** @copydoc INetworkPlayer::isMasterClient */
            bool isMasterClient() const override;

            /** @copydoc INetworkPlayer::setMasterClient */
            void setMasterClient( bool master ) override;

            // ------------------------------------------------------------------
            // INetworkPlayer — Custom properties
            // ------------------------------------------------------------------

            /** @copydoc INetworkPlayer::getCustomProperties */
            SmartPtr<Properties> getCustomProperties() const override;

            /** @copydoc INetworkPlayer::setCustomProperties */
            void setCustomProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        private:
            s32 m_actorNumber = -1;
            String m_nickName;
            String m_userId;
            u32 m_ping = 0;
            bool m_isLocal = false;
            bool m_isMaster = false;

            SmartPtr<Properties> m_customProperties;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // NetworkPlayer_h__
