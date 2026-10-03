#ifndef SceneLoadJob_h__
#define SceneLoadJob_h__

#include <Workphone/System/Job.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>

namespace workphone
{

    /**
     * @file SceneLoadJob.hpp
     * @brief Asynchronous job that loads a scene resource.
     *
     * @class SceneLoadJob
     * @ingroup Jobs
     *
     * `SceneLoadJob` represents a unit of work that loads a `scene::IGameScene`
     * instance (for example, from a file) on a worker thread managed by the
     * engine job system. The loader publishes the resulting scene via an
     * atomic smart pointer so other threads may observe the result safely.
     *
     * Responsibilities:
     * - Store the scene resource identifier (file path or serialized data).
     * - Optionally accept a `scene::LightingDirector` to configure the scene
     *   after creation.
     * - Perform the load on a background thread inside `execute()` and publish
     *   the resulting `scene::IGameScene` using `setScene()`.
     *
     * Thread-safety:
     * Members that are accessed from multiple threads use atomic wrappers
     * (`AtomicSmartPtr`, `AtomicObject`, or `atomic_bool`) so callers may set
     * inputs and read outputs from other threads without additional locking.
     *
     * Typical usage:
     * @code
     * auto job = fb::make_shared<SceneLoadJob>();
     * job->setFilePath("Assets/Scenes/my_scene.scene");
     * job->setLightingDirector(lightingDirector);
     * jobSystem->schedule(job);
     * // later (may be called from any thread):
     * auto scene = job->getScene(); // may be null until the job completes
     * @endcode
     *
     * See also: `Job`, `scene::IGameScene`, `scene::LightingDirector`.
     */
    class WPCore_API SceneLoadJob : public Job
    {
    public:
        /**
         * @brief Default constructs an empty SceneLoadJob.
         *
         * The newly constructed job contains no file path, no scene and no
         * lighting director. Configure inputs (for example `setFilePath()`
         * and `setLightingDirector()`) before scheduling the job.
         */
        SceneLoadJob();

        /**
         * @brief Virtual destructor.
         *
         * Ensures derived classes are destroyed correctly when referenced via
         * a base `Job` pointer.
         */
        ~SceneLoadJob() override;

        /**
         * @brief Perform the scene load on the worker thread.
         *
         * Called by the engine job system when this job runs. The method
         * should:
         * - Read inputs such as `m_filePath`, `m_dataStr` and
         *   `m_lightingDirector`.
         * - Create or load a `scene::IGameScene` instance for the resource.
         * - Publish the created scene via `setScene()` (or publish `nullptr`
         *   on failure).
         * - Avoid performing main-thread-only work; if needed, schedule
         *   follow-up tasks back to the main thread after the load completes.
         *
         * Thread-safety: runs on a background thread and must use the provided
         * atomic accessors to communicate results and read inputs.
         */
        void execute() override;

        /**
         * @brief Retrieve the loaded scene.
         *
         * Returns a copy of the atomic smart pointer holding the loaded
         * `scene::IScene`. If the job has not completed or loading failed,
         * the returned pointer may be `nullptr`.
         *
         * Thread-safety: safe to call from any thread; returns an atomic copy.
         *
         * @return SmartPtr<scene::IScene> Copy of the stored scene pointer
         *         (may be null).
         */
        SmartPtr<scene::IGameScene> getScene() const;

        /**
         * @brief Atomically set the scene pointer stored by this job.
         *
         * Typically called by the job implementation to publish the loaded scene.
         * Can also be used by tests or other producers to inject a scene.
         *
         * Thread-safety: safe to call from any thread.
         *
         * @param scene Smart pointer to the scene to store (may be nullptr).
         */
        void setScene( SmartPtr<scene::IGameScene> scene );

        /**
         * @brief Get the file path used by this job to load the scene.
         *
         * Returns a copy of the file path string stored in the job.
         *
         * Thread-safety: safe to call from any thread; returns a copy.
         *
         * @return String Copy of the file path (may be empty).
         */
        String getFilePath() const;

        /**
         * @brief Set the file path for the scene to load.
         *
         * The path should identify a scene resource understood by the engine's
         * scene/resource loader. This value is stored atomically so it can be
         * set from a different thread than the one that executes the job.
         *
         * @param filePath Path to the scene resource (copy is stored).
         */
        void setFilePath( const String &filePath );

        /**
         * @brief Get the optional lighting director assigned to this job.
         *
         * The lighting director, if provided, can be used by the loader to
         * configure scene lighting after the scene instance is created.
         *
         * Thread-safety: returns a copy of the atomic smart pointer.
         *
         * @return SmartPtr<scene::LightingDirector> Copy of the lighting director pointer.
         */
        SmartPtr<scene::LightingDirector> getLightingDirector() const;

        /**
         * @brief Atomically set the lighting director for the scene being loaded.
         *
         * If set prior to execution, the loader may attach or configure this
         * lighting director on the created scene instance.
         *
         * Thread-safety: safe to call from any thread.
         *
         * @param lightingDirector Smart pointer to a `LightingDirector` (may be nullptr).
         */
        void setLightingDirector( SmartPtr<scene::LightingDirector> lightingDirector );

        /**
         * @brief Query whether the loader should spawn actor creation jobs.
         *
         * When true (default) the loader may create additional jobs to construct
         * scene actors or components in parallel. When false, actor creation
         * is expected to be performed synchronously by the loader.
         *
         * Thread-safety: atomic read.
         *
         * @return bool True if actor creation jobs should be created.
         */
        bool getCreateActorJobs() const;

        /**
         * @brief Enable or disable creation of actor creation jobs.
         *
         * Thread-safety: atomic write.
         *
         * @param createActorJobs True to allow actor-creation jobs, false to
         *                        force synchronous actor creation.
         */
        void setCreateActorJobs( bool createActorJobs );

        /**
         * @brief Get the data string used by this job to load the scene.
         * Returns a copy of the data string stored in the job.
         * Thread-safety: safe to call from any thread; returns a copy.
         * @return String Copy of the data string (may be empty).
         */
        String getDataStr() const;

        /**
         * @brief Set the data string for the scene to load.
         * @param dataStr String containing scene data (copy is stored).
         */
        void setDataStr( const String &dataStr );

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Atomic smart pointer holding the loaded scene instance.
         * Set by `execute()` (via `setScene()`) on successful load. Clients must
         * use `getScene()` to obtain a copy.
         */
        AtomicSmartPtr<scene::IGameScene> m_scene;

        /**
         * @brief Atomic smart pointer to an optional lighting director.
         * If non-null when `execute()` runs, the loader may attach or configure
         * this director on the loaded scene.
         */
        AtomicSmartPtr<scene::LightingDirector> m_lightingDirector;

        /**
         * @brief Atomic container holding the path to the scene resource to load.
         * Access via `getFilePath()` / `setFilePath()`. Stored atomically so that
         * the path may be set from another thread before scheduling the job.
         */
        AtomicObject<String> m_filePath;

        /** A string containing file data. */
        AtomicObject<String> m_dataStr;

        /**
         * @brief When true, loader may spawn additional jobs to create actors.
         *
         * Default: false.
         *
         * Note: plain `atomic_bool` is used (not wrapped by `AtomicObject`). Use
         * the accessors to read/write to ensure correct atomic semantics.
         */
        atomic_bool m_createActorJobs = false;
    };
}  // namespace workphone

#endif  // SceneLoadJob_h__
