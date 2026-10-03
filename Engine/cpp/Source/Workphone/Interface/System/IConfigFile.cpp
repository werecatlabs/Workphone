#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IConfigFile.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IConfigFile, ISharedObject );

    IConfigFile::~IConfigFile() = default;
}  // namespace workphone
