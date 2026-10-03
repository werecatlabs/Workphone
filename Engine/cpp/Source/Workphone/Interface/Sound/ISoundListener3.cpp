#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Sound/ISoundListener3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ISoundListener3, ISharedObject );

    ISoundListener3::~ISoundListener3() = default;

}  // namespace workphone
