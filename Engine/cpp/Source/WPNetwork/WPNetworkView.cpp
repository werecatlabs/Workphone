#include <WPNetwork/WPNetworkView.hpp>
#include <Workphone/Interface/Net/INetworkManager.hpp>
#include <Workphone/Interface/Net/IPacket.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPNetworkView, INetworkView );

    WPNetworkView::WPNetworkView() = default;
    WPNetworkView::~WPNetworkView() = default;

    void WPNetworkView::RPC( SmartPtr<IPacket> packet )
    {
        if( m_networkManager && packet )
            m_networkManager->sendPacket( packet );
    }

    void WPNetworkView::serializeView()
    {
    }

    void WPNetworkView::deserializeView( SmartPtr<IPacket> packet )
    {
        (void)packet;
    }

    void WPNetworkView::onSerializeView( SmartPtr<INetworkStream> stream )
    {
        (void)stream;
    }

    s32 WPNetworkView::getViewId() const
    {
        return m_viewId;
    }

    void WPNetworkView::setViewId( s32 viewId )
    {
        m_viewId = viewId;
    }

    u32 WPNetworkView::getOwnerId() const
    {
        return m_ownerId;
    }

    void WPNetworkView::setOwnerId( u32 ownerId )
    {
        m_ownerId = ownerId;
    }

    bool WPNetworkView::isMine() const
    {
        auto networkManager =
            m_networkManager ? const_cast<INetworkManager *>( m_networkManager.get() ) : nullptr;
        return networkManager && static_cast<u32>( networkManager->getPlayerNumber() ) == m_ownerId;
    }

    void WPNetworkView::transferOwnership( u32 newOwnerId )
    {
        m_ownerId = newOwnerId;
    }

    void WPNetworkView::requestOwnership()
    {
    }

    bool WPNetworkView::isSceneView() const
    {
        return m_sceneView;
    }

    SmartPtr<INetworkManager> WPNetworkView::getNetworkManager() const
    {
        return m_networkManager;
    }

    void WPNetworkView::setNetworkManager( SmartPtr<INetworkManager> networkManager )
    {
        m_networkManager = networkManager;
    }

    void WPNetworkView::setSceneView( bool sceneView )
    {
        m_sceneView = sceneView;
    }
}  // namespace workphone
