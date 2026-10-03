#ifndef LoadPluginJob_h__
#define LoadPluginJob_h__

#include <Workphone/System/Job.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    /**
     * @file LoadPluginJob.hpp
     * @brief Job that loads a plugin from disk and stores a reference to it.
     *
     * This header declares `LoadPluginJob`, a concrete `Job` used to load a
     * dynamically loadable plugin (an object implementing `IPlugin`) using a
     * configured file path. The job stores the resulting plugin instance in
     * `m_plugin` so other systems can access it after execution.
     */

    /**
     * @brief Job responsible for loading a plugin.
     *
     * Instances of `LoadPluginJob` encapsulate the information required to load
     * a plugin (primarily the plugin file path) and perform the load operation
     * when `execute()` is invoked. The loaded plugin instance is stored in the
     * `m_plugin` member and is accessible via `getPlugin()`.
     *
     * The class inherits from `Job` so it can be scheduled and executed by the
     * engine's task/worker system.
     */
    class WPCore_API LoadPluginJob : public Job
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes a new `LoadPluginJob` with no plugin path or plugin set.
         */
        LoadPluginJob();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup in derived classes and that any held plugin
         * smart pointer is released when the job is destroyed.
         */
        ~LoadPluginJob() override;

        /**
         * @brief Execute the job.
         *
         * Called by the job system to perform the plugin loading operation.
         * Implementations should attempt to load the plugin specified by
         * `m_pluginPath` and store the resulting `IPlugin` instance in
         * `m_plugin`. Error handling/logging should be done inside the
         * implementation as appropriate.
         */
        void execute() override;

        /**
         * @brief Get the configured plugin file path.
         * @return The plugin path as a `String`.
         *
         * This path is used by `execute()` to locate and load the plugin
         * library.
         */
        String getPluginPath() const;

        /**
         * @brief Set the plugin file path to load.
         * @param pluginPath The file path to the plugin library.
         *
         * Prefer using an absolute or engine-relative path to avoid ambiguity.
         */
        void setPluginPath( const String &pluginPath );

        /**
         * @brief Get the loaded plugin instance.
         * @return Smart pointer to the loaded `IPlugin` instance.
         *
         * Returns the plugin instance created/assigned during `execute()`.
         * The returned smart pointer may be null if the plugin has not yet
         * been loaded or if loading failed.
         */
        SmartPtr<IPlugin> getPlugin() const;

        /**
         * @brief Set the plugin instance.
         * @param plugin Smart pointer to an `IPlugin` instance.
         *
         * This setter allows assigning a plugin instance directly (for
         * testing or when the plugin is created externally) instead of
         * loading it from disk.
         */
        void setPlugin( SmartPtr<IPlugin> plugin );

        WP_CLASS_REGISTER_DECL;

    protected:
        /** @brief The loaded plugin instance (owned via SmartPtr). */
        AtomicSmartPtr<IPlugin> m_plugin;

        /** @brief The file path of the plugin to load. */
        AtomicObject<String> m_pluginPath;
    };
}  // namespace workphone

#endif  // LoadPluginJob_h__
