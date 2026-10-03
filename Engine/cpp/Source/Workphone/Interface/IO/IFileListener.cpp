#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/IO/IFileListener.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IFileListener, ISharedObject );

    const u32 IFileListener::FILE_LISTENER_ACTION_ADD = 1;
    const u32 IFileListener::FILE_LISTENER_ACTION_DELETE = 2;
    const u32 IFileListener::FILE_LISTENER_ACTION_MODIFIED = 4;

    IFileListener::~IFileListener() = default;

}  // namespace workphone
