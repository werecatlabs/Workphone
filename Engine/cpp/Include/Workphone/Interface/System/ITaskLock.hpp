#ifndef ITaskLock_h__
#define ITaskLock_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** Interface for a task lock. */
    class WPCore_API ITaskLock : public ISharedObject
    {
    public:
        /** Destructor. */
        ~ITaskLock() override;

        /** Returns the task. */
        virtual SmartPtr<ITask> getTask() const = 0;

        /** Sets the task. */
        virtual void setTask( SmartPtr<ITask> task ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ITaskLock_h__
