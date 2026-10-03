#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ITask, ISharedObject );

    const u32 ITask::recycle_flag = ( 1 << 1 );
    const u32 ITask::use_accurate_time_flag = ( 1 << 2 );
    const u32 ITask::enabled_flag = ( 1 << 3 );
    const u32 ITask::auto_target_fps_flag = ( 1 << 4 );
    const u32 ITask::fixed_time_flag = ( 1 << 5 );
    const u32 ITask::updating_flag = ( 1 << 6 );
    const u32 ITask::queue_messages_flag = ( 1 << 7 );
    const u32 ITask::use_thread_flag = ( 1 << 8 );
    const u32 ITask::smooth_time_flag = ( 1 << 9 );
    const u32 ITask::primary_flag = ( 1 << 10 );
    const u32 ITask::parallel_flag = ( 1 << 11 );
    const u32 ITask::executing_flag = ( 1 << 12 );
    const u32 ITask::paused_flag = ( 1 << 13 );

    ITask::~ITask() = default;

}  // namespace workphone
