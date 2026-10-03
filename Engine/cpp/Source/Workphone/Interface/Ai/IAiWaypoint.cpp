#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IAiWaypoint.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAiWaypoint, ISharedObject );

    IAiWaypoint::~IAiWaypoint() = default;

}  // namespace workphone
