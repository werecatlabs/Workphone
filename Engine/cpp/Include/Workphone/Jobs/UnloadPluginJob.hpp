#ifndef UnloadPluginJob_h__
#define UnloadPluginJob_h__

#include <Workphone/System/Job.hpp>

namespace workphone
{

    /**
     * @file UnloadPluginJob.hpp
     * @brief Job that safely unloads a dynamically loaded plugin.
     *
     * This file declares the `UnloadPluginJob` which wraps the logic required
     * to perform a plugin unload operation on a worker thread or task system.
     * The job holds a smart pointer to an `IPlugin` instance and will perform
     * the necessary cleanup when executed.
     */

    /**
     * @class UnloadPluginJob
     * @brief Performs plugin unload operations as a Job.
     *
     * The `UnloadPluginJob` is a concrete `Job` used to properly unload and
     * release an `IPlugin` instance. It is intended to be scheduled on the
     * engine's task system so that plugin unloading happens on a controlled
     * thread context and does not interfere with other subsystems.
     *
     * Typical usage:
     * - Construct the job.
     * - Assign the plugin via `setPlugin`.
     * - Schedule the job with the engine's task/Job system.
     *
     * The implementation of `execute` should ensure any plugin-specific
     * finalization is invoked and that resources held by the underlying
     * library (DLL/shared object) are released.
     */
    class WPCore_API UnloadPluginJob : public Job
    {
    public:
        /**
         * @brief Construct a new UnloadPluginJob.
         *
         * Creates an empty job. A plugin must be provided via `setPlugin`
         * before scheduling the job; otherwise the job will be a no-op.
         */
        UnloadPluginJob();

        /**
         * @brief Destroy the UnloadPluginJob.
         *
         * Ensures any remaining references are released. The actual plugin
         * unload work should be done inside `execute` which runs on the job
         * thread.
         */
        ~UnloadPluginJob() override;

        /**
         * @brief Execute the unload operation.
         *
         * This method is invoked by the Job/Task system. It should:
         * - perform any plugin-specific shutdown/finalization,
         * - release the library handle or other resources owned by the plugin,
         * - clear the internal plugin smart pointer to decrement ref counts.
         *
         * Implementations must be thread-safe with respect to the engine's
         * job execution model.
         */
        void execute() override;

        /**
         * @brief Get the plugin this job will unload.
         * @return SmartPtr<IPlugin> The plugin instance (may be null).
         *
         * The returned smart pointer represents the plugin currently assigned
         * to the job. It is safe to call from other threads for inspection,
         * but modifications should use `setPlugin`.
         */
        SmartPtr<IPlugin> getPlugin() const;

        /**
         * @brief Assign the plugin to be unloaded by this job.
         * @param plugin SmartPtr<IPlugin> Smart pointer to the plugin to unload.
         *
         * The job takes a reference to the provided `plugin`. The actual
         * unload and resource release occurs when `execute` is run.
         */
        void setPlugin( SmartPtr<IPlugin> plugin );

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief The plugin instance that will be unloaded when the job runs.
         *
         * Stored as a smart pointer to maintain reference lifetime until the
         * job executes. Clearing this pointer in `execute` releases the plugin
         * resources.
         */
        AtomicSmartPtr<IPlugin> m_plugin;
    };

}  // namespace workphone

#endif  // UnloadPluginJob_h__
