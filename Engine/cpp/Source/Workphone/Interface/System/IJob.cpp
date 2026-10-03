#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IJob.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IJob, ISharedObject );

    IJob::~IJob() = default;
}  // namespace workphone
