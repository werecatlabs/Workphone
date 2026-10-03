#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <Workphone/Core/DebugTrace.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>

namespace workphone
{

    TaskLock::TaskLock( SmartPtr<ITask> task )
    {
        WP_DEBUG_TRACE;

        if( task )
        {
            setTask( task );

#if !WP_FINAL
            WP_LOG( "Task stop id: " + Thread::getTaskName( task->getTask() ) );
#endif

            task->stop();
        }
    }

    TaskLock::TaskLock()
    {
        WP_DEBUG_TRACE;
    }

    TaskLock::~TaskLock()
    {
        WP_DEBUG_TRACE;

        if( auto task = getTask() )
        {
#if !WP_FINAL
            WP_LOG( "Task start id: " + Thread::getTaskName( task->getTask() ) );
#endif

            task->start();
        }
    }

    auto TaskLock::getTask() const -> SmartPtr<ITask>
    {
        return m_task;
    }

    void TaskLock::setTask( SmartPtr<ITask> task )
    {
        m_task = task;
    }

}  // namespace workphone
