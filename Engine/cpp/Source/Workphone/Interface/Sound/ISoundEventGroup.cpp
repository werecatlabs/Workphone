#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Sound/ISoundEventGroup.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ISoundEventGroup, ISharedObject );

    ISoundEventGroup::~ISoundEventGroup() = default;

}  // namespace workphone
