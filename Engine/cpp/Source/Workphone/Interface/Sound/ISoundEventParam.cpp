#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Sound/ISoundEventParam.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ISoundEventParam, ISharedObject );

    ISoundEventParam::~ISoundEventParam() = default;

}  // namespace workphone
