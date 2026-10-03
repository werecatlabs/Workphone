#ifndef WPNetworkView_h__
#define WPNetworkView_h__

#include <WPNetwork/WPNetworkPrerequisites.hpp>
#include <Workphone/Interface/Net/INetworkView.hpp>

namespace workphone
{
    class WPNetwork_API WPNetworkView : public INetworkView
    {
    public:
        WPNetworkView();
        ~WPNetworkView() override;

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

        SmartPtr<INetworkManager> getNetworkManager() const;
        void setNetworkManager( SmartPtr<INetworkManager> networkManager );
        void setSceneView( bool sceneView );

        WP_CLASS_REGISTER_DECL;

    private:
        SmartPtr<INetworkManager> m_networkManager;
        s32 m_viewId = -1;
        u32 m_ownerId = 0;
        bool m_sceneView = false;
    };
}  // namespace workphone

#endif  // WPNetworkView_h__
