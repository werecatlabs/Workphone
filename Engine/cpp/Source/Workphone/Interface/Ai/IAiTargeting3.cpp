#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IAiTargeting3.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAiTargeting3, ISharedObject );

    IAiTargeting3::~IAiTargeting3() = default;

}  // namespace workphone
