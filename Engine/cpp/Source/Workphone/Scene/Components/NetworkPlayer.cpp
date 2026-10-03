#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/NetworkPlayer.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone
{
    namespace scene
    {

        WP_CLASS_REGISTER_DERIVED( workphone, NetworkPlayer, INetworkPlayer );

        NetworkPlayer::NetworkPlayer() = default;

        NetworkPlayer::~NetworkPlayer() = default;

        // ------------------------------------------------------------------
        // Identity
        // ------------------------------------------------------------------

        s32 NetworkPlayer::getActorNumber() const
        {
            return m_actorNumber;
        }

        void NetworkPlayer::setActorNumber( s32 actorNumber )
        {
            m_actorNumber = actorNumber;
        }

        String NetworkPlayer::getNickName() const
        {
            return m_nickName;
        }

        void NetworkPlayer::setNickName( const String &nickName )
        {
            m_nickName = nickName;
        }

        String NetworkPlayer::getUserId() const
        {
            return m_userId;
        }

        void NetworkPlayer::setUserId( const String &userId )
        {
            m_userId = userId;
        }

        // ------------------------------------------------------------------
        // Connection quality
        // ------------------------------------------------------------------

        u32 NetworkPlayer::getPing() const
        {
            return m_ping;
        }

        void NetworkPlayer::setPing( u32 ping )
        {
            m_ping = ping;
        }

        // ------------------------------------------------------------------
        // Ownership / role flags
        // ------------------------------------------------------------------

        bool NetworkPlayer::isLocal() const
        {
            return m_isLocal;
        }

        void NetworkPlayer::setLocal( bool local )
        {
            m_isLocal = local;
        }

        bool NetworkPlayer::isMasterClient() const
        {
            return m_isMaster;
        }

        void NetworkPlayer::setMasterClient( bool master )
        {
            m_isMaster = master;
        }

        // ------------------------------------------------------------------
        // Custom properties
        // ------------------------------------------------------------------

        SmartPtr<Properties> NetworkPlayer::getCustomProperties() const
        {
            return m_customProperties;
        }

        void NetworkPlayer::setCustomProperties( SmartPtr<Properties> properties )
        {
            m_customProperties = properties;
        }

    }  // namespace scene
}  // namespace workphone
