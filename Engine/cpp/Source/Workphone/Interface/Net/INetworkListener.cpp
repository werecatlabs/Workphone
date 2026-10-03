#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Net/INetworkListener.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, INetworkListener, ISharedObject );

    INetworkListener::~INetworkListener() = default;

}  // namespace workphone
