#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IAiAgent.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IAiAgent, ISharedObject );

    IAiAgent::~IAiAgent() = default;

}  // namespace workphone
