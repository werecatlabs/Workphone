#ifndef IJobGroup_h__
#define IJobGroup_h__

#include <Workphone/Interface/System/IJob.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /** An Interface for a job group class. Intended to control the execution of group of jobs. */
    class WPCore_API IJobGroup : public IJob
    {
    public:
        /** Virtual destructor.
         */
        ~IJobGroup() override;

        /**
         * @brief Add a job to this group.
         * @param job The job to add to the group.
         * @return bool True if the job was successfully added, false otherwise.
         */
        virtual bool addJob( SmartPtr<IJob> job ) = 0;

        /**
         * @brief Remove a job from this group.
         * @param job The job to remove from the group.
         * @return bool True if the job was successfully removed, false otherwise.
         */
        virtual bool removeJob( SmartPtr<IJob> job ) = 0;

        /**
         * @brief Get all jobs in this group.
         * @return Array<SmartPtr<IJob>> Array of all jobs in the group.
         */
        virtual Array<SmartPtr<IJob>> getJobs() const = 0;

        /**
         * @brief Get the number of jobs in this group.
         * @return u32 The number of jobs in the group.
         */
        virtual u32 getJobCount() const = 0;

        /**
         * @brief Clear all jobs from this group.
         */
        virtual void clearJobs() = 0;

        /**
         * @brief Add a dependency between two jobs in the group.
         * @param dependentJob The job that depends on the prerequisite job.
         * @param prerequisiteJob The job that must complete before the dependent job can start.
         * @return bool True if the dependency was successfully added, false otherwise.
         */
        virtual bool addDependency( SmartPtr<IJob> dependentJob, SmartPtr<IJob> prerequisiteJob ) = 0;

        /**
         * @brief Remove a dependency between two jobs in the group.
         * @param dependentJob The job that depends on the prerequisite job.
         * @param prerequisiteJob The job that was previously required to complete first.
         * @return bool True if the dependency was successfully removed, false otherwise.
         */
        virtual bool removeDependency( SmartPtr<IJob> dependentJob, SmartPtr<IJob> prerequisiteJob ) = 0;

        /**
         * @brief Get all prerequisite jobs for a given job.
         * @param job The job to get prerequisites for.
         * @return Array<SmartPtr<IJob>> Array of prerequisite jobs.
         */
        virtual Array<SmartPtr<IJob>> getPrerequisites( SmartPtr<IJob> job ) const = 0;

        /**
         * @brief Get all jobs that depend on a given job.
         * @param job The job to get dependents for.
         * @return Array<SmartPtr<IJob>> Array of dependent jobs.
         */
        virtual Array<SmartPtr<IJob>> getDependents( SmartPtr<IJob> job ) const = 0;

        /**
         * @brief Check if a job has all its prerequisites satisfied.
         * @param job The job to check.
         * @return bool True if all prerequisites are satisfied, false otherwise.
         */
        virtual bool arePrerequisitesSatisfied( SmartPtr<IJob> job ) const = 0;

        /**
         * @brief Check if there are any circular dependencies in the job group.
         * @return bool True if circular dependencies exist, false otherwise.
         */
        virtual bool hasCircularDependencies() const = 0;

        /**
         * @brief Get jobs that are ready to execute (all prerequisites satisfied).
         * @return Array<SmartPtr<IJob>> Array of jobs ready for execution.
         */
        virtual Array<SmartPtr<IJob>> getReadyJobs() const = 0;

        /**
         * @brief Get jobs that are currently executing.
         * @return Array<SmartPtr<IJob>> Array of currently executing jobs.
         */
        virtual Array<SmartPtr<IJob>> getExecutingJobs() const = 0;

        /**
         * @brief Get jobs that have finished execution.
         * @return Array<SmartPtr<IJob>> Array of finished jobs.
         */
        virtual Array<SmartPtr<IJob>> getFinishedJobs() const = 0;

        /**
         * @brief Check if all jobs in the group have finished.
         * @return bool True if all jobs are finished, false otherwise.
         */
        virtual bool areAllJobsFinished() const = 0;

        /**
         * @brief Set the execution mode for the job group.
         * @param parallel True to execute jobs in parallel when possible, false for sequential
         * execution.
         */
        virtual void setParallelExecution( bool parallel ) = 0;

        /**
         * @brief Get the current execution mode.
         * @return bool True if parallel execution is enabled, false for sequential.
         */
        virtual bool isParallelExecution() const = 0;

        /**
         * @brief Set the maximum number of jobs that can execute simultaneously.
         * @param maxConcurrentJobs The maximum number of concurrent jobs.
         */
        virtual void setMaxConcurrentJobs( u32 maxConcurrentJobs ) = 0;

        /**
         * @brief Get the maximum number of jobs that can execute simultaneously.
         * @return u32 The maximum number of concurrent jobs.
         */
        virtual u32 getMaxConcurrentJobs() const = 0;

        /**
         * @brief Cancel all jobs in the group.
         */
        virtual void cancelAllJobs() = 0;

        /**
         * @brief Pause all jobs in the group.
         */
        virtual void pauseAllJobs() = 0;

        /**
         * @brief Resume all paused jobs in the group.
         */
        virtual void resumeAllJobs() = 0;

        /**
         * @brief Get the overall progress of the job group (0-100).
         * @return u32 The progress percentage.
         */
        virtual u32 getGroupProgress() const = 0;

        /**
         * @brief Set a callback function to be called when job dependencies change.
         * @param callbackFunction The callback function to be called.
         */
        virtual void setDependencyChangedCallback(
            std::function<void( SmartPtr<IJob>, SmartPtr<IJob> )> callbackFunction ) = 0;

        /**
         * @brief Set a callback function to be called when a job becomes ready to execute.
         * @param callbackFunction The callback function to be called.
         */
        virtual void setJobReadyCallback( std::function<void( SmartPtr<IJob> )> callbackFunction ) = 0;

        /**
         * @brief Set a callback function to be called when a job finishes.
         * @param callbackFunction The callback function to be called.
         */
        virtual void setJobFinishedCallback(
            std::function<void( SmartPtr<IJob> )> callbackFunction ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IJobGroup_h__
