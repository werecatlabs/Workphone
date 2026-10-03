#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IProfile.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IProfile, ISharedObject );

    IProfile::~IProfile() = default;
}  // namespace workphone
