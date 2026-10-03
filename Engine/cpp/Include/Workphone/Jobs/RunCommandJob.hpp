#ifndef RunCommandJob_h__
#define RunCommandJob_h__

#include <Workphone/System/Job.hpp>

namespace workphone
{

    /**
     * @file RunCommandJob.hpp
     * @brief Declaration of the `RunCommandJob` class.
     */

    /**
     * @brief A job that executes an `ICommand` instance.
     *
     * `RunCommandJob` stores a command object and executes it when the job is
     * run by the job system. The stored command pointer is held in an
     * `AtomicSmartPtr` to allow safe access from multiple threads (for example
     * one thread setting the command while another thread executes the job).
     */
    class WPCore_API RunCommandJob : public Job
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Leaves the internal command pointer null. The command should be set
         * by calling `setCommand` before the job is executed.
         */
        RunCommandJob();

        /**
         * @brief Destructor.
         */
        ~RunCommandJob() override;

        /**
         * @brief Execute the job.
         *
         * This implementation retrieves the currently stored command (if any)
         * and executes it. If no command is set, the method returns without
         * performing any action.
         *
         * @copydoc Job::execute
         */
        void execute() override;

        /**
         * @brief Get the command that will be executed by this job.
         *
         * The returned `SmartPtr` may be null if no command has been set.
         * Access is thread-safe because the command is stored in an
         * `AtomicSmartPtr`.
         *
         * @return SmartPtr<ICommand> The current command, or null.
         */
        SmartPtr<ICommand> getCommand() const;

        /**
         * @brief Set the command to be executed by this job.
         *
         * Replaces any previously stored command. The operation is performed
         * atomically to allow callers to set the command from a different
         * thread than the one that will execute the job.
         *
         * @param command SmartPtr<ICommand> The command to store.
         */
        void setCommand( SmartPtr<ICommand> command );

        /**
         * @brief Register this class with the runtime type system.
         */
        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Atomic holder for the command to run.
         *
         * Stored as an `AtomicSmartPtr` because the command may be set from one
         * thread while being read/executed from another.
         */
        AtomicSmartPtr<ICommand> m_command;
    };

}  // namespace workphone

#endif  // RunCommandJob_h__
