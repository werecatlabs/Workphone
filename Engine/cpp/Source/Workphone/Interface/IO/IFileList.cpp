#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/IO/IFileList.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IFileList, ISharedObject );

    IFileList::IFileList() : ISharedObject( IFileList::typeInfo() )
    {
    }

    IFileList::~IFileList() = default;
}  // namespace workphone
