#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IAi.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IAi, ISharedObject );

    IAi::~IAi() = default;

}  // namespace workphone
