#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/ITaskLock.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ITaskLock, ISharedObject );

    ITaskLock::~ITaskLock() = default;

}  // namespace workphone
