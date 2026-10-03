#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IAiTargetingSystem.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAiTargetingSystem, ISharedObject );

    IAiTargetingSystem::~IAiTargetingSystem() = default;

}  // namespace workphone
