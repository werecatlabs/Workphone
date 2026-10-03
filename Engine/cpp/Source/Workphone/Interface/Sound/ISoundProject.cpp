#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Sound/ISoundProject.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ISoundProject, ISharedObject );

    ISoundProject::~ISoundProject() = default;

}  // namespace workphone
