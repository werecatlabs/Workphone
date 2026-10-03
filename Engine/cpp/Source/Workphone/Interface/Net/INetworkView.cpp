#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Net/INetworkView.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, INetworkView, ISharedObject );

    INetworkView::~INetworkView() = default;

}  // namespace workphone
