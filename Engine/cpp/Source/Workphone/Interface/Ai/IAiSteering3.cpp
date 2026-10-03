#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IAiSteering3.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAiSteering3, ISharedObject );

    IAiSteering3::~IAiSteering3() = default;

}  // namespace workphone
