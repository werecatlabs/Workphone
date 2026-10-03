#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/IO/IFolderExplorer.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IFolderExplorer, ISharedObject );

    IFolderExplorer::~IFolderExplorer() = default;

}  // namespace workphone
