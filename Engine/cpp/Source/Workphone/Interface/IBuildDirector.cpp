#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/IBuildDirector.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IBuildDirector, IResource );

    IBuildDirector::IBuildDirector( u32 poolTypeId ) : IResource( poolTypeId )
    {
    }

    IBuildDirector::IBuildDirector() : IResource( IBuildDirector::typeInfo() )
    {
    }

    IBuildDirector::~IBuildDirector() = default;

}  // namespace workphone
