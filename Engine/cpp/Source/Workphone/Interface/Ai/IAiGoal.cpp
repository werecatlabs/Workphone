#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IAiGoal.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IAiGoal, ISharedObject );

    IAiGoal::~IAiGoal() = default;

}  // namespace workphone
