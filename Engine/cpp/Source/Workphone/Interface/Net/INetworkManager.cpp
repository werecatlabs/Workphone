#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Net/INetworkManager.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, INetworkManager, ISharedObject );

    INetworkManager::~INetworkManager() = default;

}  // namespace workphone
