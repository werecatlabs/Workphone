#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IResource, IPrototype );
    const String IResource::nameStr = String( "name" );

    IResource::IResource( u32 typeId ) : core::IPrototype( typeId )
    {
    }

    IResource::IResource()
    {
    }

    IResource::~IResource() = default;

}  // namespace workphone
