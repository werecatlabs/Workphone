#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IFileSystem, ISharedObject );

    IFileSystem::~IFileSystem() = default;
}  // namespace workphone
