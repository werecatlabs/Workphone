#ifndef __WP_TaskLock_h__
#define __WP_TaskLock_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone
{

    /** The task lock. */
    class WPCore_API TaskLock
    {
    public:
        /** Constructor. */
        TaskLock();

        /** Constructor. */
        TaskLock( SmartPtr<ITask> task );

        /** Destructor. */
        ~TaskLock();

        /** Get the task. */
        SmartPtr<ITask> getTask() const;

        /** Set the task. */
        void setTask( SmartPtr<ITask> task );

    private:
        /** The task. */
        SmartPtr<ITask> m_task;
    };

}  // namespace workphone

#endif  // TaskLock_h__
