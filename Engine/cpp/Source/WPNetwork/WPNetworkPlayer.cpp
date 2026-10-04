#include <WPNetwork/WPNetworkPlayer.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPNetworkPlayer, INetworkPlayer );

    WPNetworkPlayer::WPNetworkPlayer() = default;
    WPNetworkPlayer::~WPNetworkPlayer() = default;

    s32 WPNetworkPlayer::getActorNumber() const
    {
        return m_actorNumber;
    }

    void WPNetworkPlayer::setActorNumber( s32 actorNumber )
    {
        m_actorNumber = actorNumber;
    }

    String WPNetworkPlayer::getNickName() const
    {
        return m_nickName;
    }

    void WPNetworkPlayer::setNickName( const String &nickName )
    {
        m_nickName = nickName;
    }

    String WPNetworkPlayer::getUserId() const
    {
        return m_userId;
    }

    void WPNetworkPlayer::setUserId( const String &userId )
    {
        m_userId = userId;
    }

    u32 WPNetworkPlayer::getPing() const
    {
        return m_ping;
    }

    void WPNetworkPlayer::setPing( u32 ping )
    {
        m_ping = ping;
    }

    bool WPNetworkPlayer::isLocal() const
    {
        return m_local;
    }

    void WPNetworkPlayer::setLocal( bool local )
    {
        m_local = local;
    }

    bool WPNetworkPlayer::isMasterClient() const
    {
        return m_masterClient;
    }

    void WPNetworkPlayer::setMasterClient( bool master )
    {
        m_masterClient = master;
    }

    SmartPtr<Properties> WPNetworkPlayer::getCustomProperties() const
    {
        return m_customProperties;
    }

    void WPNetworkPlayer::setCustomProperties( SmartPtr<Properties> properties )
    {
        m_customProperties = properties;
    }
}  // namespace workphone
