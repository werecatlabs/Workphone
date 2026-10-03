#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Net/INetworkPlayer.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, INetworkPlayer, ISharedObject );

    INetworkPlayer::~INetworkPlayer() = default;

}  // namespace workphone
