#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Net/ISystemAddress.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ISystemAddress, ISharedObject );

    ISystemAddress::~ISystemAddress() = default;

}  // namespace workphone
