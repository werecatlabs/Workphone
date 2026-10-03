#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Database/IResourceReference.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IResourceReference, ISharedObject );

    IResourceReference::~IResourceReference() = default;

}  // namespace workphone
