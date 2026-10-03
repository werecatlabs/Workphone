#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Sound/ISoundEvent.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ISoundEvent, ISharedObject );

    ISoundEvent::~ISoundEvent() = default;

}  // namespace workphone
