#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IMeshResource, IResource );

    IMeshResource::IMeshResource() : IResource( IMeshResource::typeInfo() )
    {
    }

    IMeshResource::IMeshResource( u32 poolTypeId ) : IResource( poolTypeId )
    {
    }

    IMeshResource::~IMeshResource() = default;
}  // namespace workphone
