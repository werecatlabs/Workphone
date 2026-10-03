#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Net/IPacket.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IPacket, ISharedObject );

    IPacket::~IPacket() = default;

}  // namespace workphone
