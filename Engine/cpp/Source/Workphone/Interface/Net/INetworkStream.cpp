#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Net/INetworkStream.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, INetworkStream, ISharedObject );

    INetworkStream::~INetworkStream() = default;

}  // namespace workphone
