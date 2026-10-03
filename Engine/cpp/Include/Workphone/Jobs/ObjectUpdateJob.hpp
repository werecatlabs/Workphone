#ifndef ObjectUpdateJob_h__
#define ObjectUpdateJob_h__

#include <Workphone/System/Job.hpp>

namespace workphone
{

    /**
     * @file ObjectUpdateJob.hpp
     * @brief Job used to update a shared object.
     *
     * This header declares the `ObjectUpdateJob` class which is a lightweight job wrapper
     * that stores a smart pointer to an `ISharedObject` and performs an update operation
     * on that object when the job is executed.
     */

    /**
     * @brief A job that is used to update objects.
     *
     * The `ObjectUpdateJob` holds a `SmartPtr<ISharedObject>` referred to as the owner.
     * When the job is executed via `execute()`, the job should perform whatever update
     * semantics are appropriate for the owner (for example, calling an update method
     * on the `ISharedObject` if one exists).
     *
     * The job does not assume ownership semantics beyond the `SmartPtr` lifetime;
     * callers should ensure the `owner` remains valid for the duration the job may run.
     */
    class WPCore_API ObjectUpdateJob : public Job
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes an empty `ObjectUpdateJob` with no owner set.
         */
        ObjectUpdateJob();

        /**
         * @brief Virtual destructor.
         *
         * Cleans up job resources. The `SmartPtr<ISharedObject>` destructor will release
         * the reference to the owner if one is set.
         */
        ~ObjectUpdateJob() override;

        /**
         * @brief Execute the job.
         *
         * @copydetails Job::execute
         *
         * When executed this job will attempt to update the stored owner object if it
         * has been set. Implementations should be safe to call from worker threads;
         * any thread-safety requirements belong to the owner object and caller.
         */
        void execute() override;

        /**
         * @brief Get the owner object this job will update.
         *
         * @return SmartPtr<ISharedObject> The current owner pointer (may be null).
         */
        ISharedObject *getOwner() const;

        /**
         * @brief Set the owner object this job will update.
         *
         * The provided `owner` will be stored in the job and used when `execute()` is run.
         * Passing a null `SmartPtr` clears the owner.
         *
         * @param owner SmartPtr<ISharedObject> The object to update when this job runs.
         */
        void setOwner( ISharedObject *owner );

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief The object that will be updated when the job runs.
         *
         * Stored as a `SmartPtr` to manage lifetime. May be null if no owner is set.
         */
        SmartPtr<ISharedObject> m_owner;
    };

}  // namespace workphone

#endif  // ObjectUpdateJob_h__
