#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Mesh/IMeshLoader.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IMeshLoader, ISharedObject );

    IMeshLoader::~IMeshLoader() = default;
}  // namespace workphone
