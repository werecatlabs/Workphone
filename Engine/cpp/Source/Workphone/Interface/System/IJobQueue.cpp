#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IJobQueue, ISharedObject );

    IJobQueue::~IJobQueue() = default;
}  // namespace workphone
