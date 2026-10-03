#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IProject.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IProject, IResource );

    IProject::IProject() : IResource( IProject::typeInfo() )
    {
    }

    IProject::IProject( u32 poolTypeId ) : IResource( poolTypeId )
    {
    }

    IProject::~IProject() = default;

}  // namespace workphone
