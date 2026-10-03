#ifndef WPNetworkPlayer_h__
#define WPNetworkPlayer_h__

#include <WPNetwork/WPNetworkPrerequisites.hpp>
#include <Workphone/Interface/Net/INetworkPlayer.hpp>

namespace workphone
{
    class WPNetwork_API WPNetworkPlayer : public INetworkPlayer
    {
    public:
        WPNetworkPlayer();
        ~WPNetworkPlayer() override;

        s32 getActorNumber() const override;
        void setActorNumber( s32 actorNumber ) override;
        String getNickName() const override;
        void setNickName( const String &nickName ) override;
        String getUserId() const override;
        void setUserId( const String &userId ) override;
        u32 getPing() const override;
        void setPing( u32 ping ) override;
        bool isLocal() const override;
        void setLocal( bool local ) override;
        bool isMasterClient() const override;
        void setMasterClient( bool master ) override;
        SmartPtr<Properties> getCustomProperties() const override;
        void setCustomProperties( SmartPtr<Properties> properties ) override;

        WP_CLASS_REGISTER_DECL;

    private:
        s32 m_actorNumber = -1;
        String m_nickName;
        String m_userId;
        u32 m_ping = 0;
        bool m_local = false;
        bool m_masterClient = false;
        SmartPtr<Properties> m_customProperties;
    };
}  // namespace workphone

#endif  // WPNetworkPlayer_h__
