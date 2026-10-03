#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ITimer, ISharedObject );

    ITimer::~ITimer() = default;

}  // namespace workphone
