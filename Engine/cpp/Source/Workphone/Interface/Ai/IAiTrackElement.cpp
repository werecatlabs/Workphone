#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IAiTrackElement.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IAiTrackElement, ISharedObject );

    IAiTrackElement::~IAiTrackElement() = default;

}  // namespace workphone
