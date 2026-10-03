#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IWorkerThread.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IWorkerThread, ISharedObject );

    IWorkerThread::~IWorkerThread() = default;

}  // namespace workphone
