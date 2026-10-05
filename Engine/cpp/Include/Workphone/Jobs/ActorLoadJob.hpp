#ifndef ActorLoadJob_h__
#define ActorLoadJob_h__

#include <Workphone/System/Job.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>

namespace workphone
{
    /**
     * @brief Job responsible for loading or initializing an actor (scene node).
     *
     * ActorLoadJob wraps the work required to create/configure a scene::IActor, optionally
     * attach it to a parent actor and create child jobs to load child actors. It is intended
     * to be executed by the engine's job/task system by calling \c execute().
     *
     * Usage notes:
     * - Set the actor to be initialized via \c setActor() before queuing the job.
     * - Optionally set a parent actor via \c setParent() to attach the created actor to a parent.
     * - Use \c setCreateChildJobs(false) to prevent the job from creating/queuing child jobs.
     *
     * Threading:
     * - This job commits on the primary queue. Configure its pinned target before submission.
     * - Hierarchies are loaded synchronously; the child-job flag is retained for API compatibility.
     */
    class WPCore_API ActorLoadJob : public Job
    {
    public:
        /**
         * @brief Construct a new ActorLoadJob.
         *
         * The default constructor sets sensible defaults. After construction, configure the job
         * (actor, parent, properties, etc.) before scheduling it with the task system.
         */
        ActorLoadJob();
        SmartPtr<scene::IGameScene> getScene() const;
        void setScene( SmartPtr<scene::IGameScene> scene );

        /**
         * @brief Destroy the ActorLoadJob.
         *
         * Cleans up any resources owned by the job. The destructor is virtual (override) to allow
         * safe polymorphic deletion through Job pointers.
         */
        ~ActorLoadJob() override;

        /**
         * @brief Execute the job.
         *
         * This method contains the logic required to load/initialize the actor. It is called by
         * the job system when the job is run. Implementations should:
         * - Create or initialize the actor referenced by \c m_actor.
         * - Attach the actor to \c m_parent if provided.
         * - Use \c m_properties to configure the actor.
         * - Optionally create child ActorLoadJob instances for any children if
         *   \c m_createChildJobs is true.
         *
         * Implementations must consider thread-safety of any scene system operations.
         */
        void execute() override;

        /**
         * @brief Get the actor that this job will (or has) loaded/initialized.
         * @return SmartPtr<scene::IGameActor> Smart pointer to the actor instance.
         */
        SmartPtr<scene::IGameActor> getActor() const;

        /**
         * @brief Set the actor instance that this job should load/initialize.
         * @param actor Smart pointer to the actor.
         */
        void setActor( SmartPtr<scene::IGameActor> actor );

        /**
         * @brief Get the parent actor to which the loaded actor should be attached.
         * @return SmartPtr<scene::IGameActor> Smart pointer to the parent actor, or null if none.
         */
        SmartPtr<scene::IGameActor> getParent() const;

        /**
         * @brief Set the parent actor that the loaded actor should be attached to.
         * @param parent Smart pointer to the parent actor.
         */
        void setParent( SmartPtr<scene::IGameActor> parent );

        /**
         * @brief Get any child ActorLoadJob instances created by this job.
         *
         * Child jobs represent work required to load child actors of the actor managed by
         * this job. They can be scheduled independently by the caller or returned for inspection.
         *
         * @return Array<SmartPtr<ActorLoadJob>> Array of child jobs.
         */
        Array<SmartPtr<ActorLoadJob>> getChildJobs() const;

        /**
         * @brief Set child jobs for this job.
         * @param childJobs Reference to an array of child ActorLoadJob smart pointers.
         */
        void setChildJobs( const Array<SmartPtr<ActorLoadJob>> &childJobs );

        /**
         * @brief Get properties used to configure the actor during load.
         * @return SmartPtr<Properties> Smart pointer to the properties object.
         */
        SmartPtr<Properties> getProperties() const override;

        /**
         * @brief Set properties used to configure the actor during load.
         * @param properties Smart pointer to a Properties object containing configuration values.
         */
        void setProperties( SmartPtr<Properties> properties ) override;

        /**
         * @brief Query whether this job should create child jobs for child actors.
         * @return true if child jobs should be created, false otherwise.
         */
        bool getCreateChildJobs() const;

        /**
         * @brief Enable or disable creation of child jobs for child actors.
         * @param createChildJobs true to create child jobs; false to skip them.
         */
        void setCreateChildJobs( bool createChildJobs );

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Child actor load jobs created by this job.
         *
         * These jobs are typically used to load/initialize child actors of the actor referenced
         * by \c m_actor. The array may be empty if there are no children or if child-job creation
         * is disabled.
         */
        ConcurrentArray<SmartPtr<ActorLoadJob>> m_childJobs;

        /**
         * @brief Parent actor to attach the loaded actor to (optional).
         *
         * If non-null, the job should attach the created/initialized actor to this parent.
         */
        AtomicSmartPtr<scene::IGameActor> m_parent;
        AtomicSmartPtr<scene::IGameScene> m_scene;
        u64 m_sceneGeneration = 0;

        /**
         * @brief Actor instance that will be loaded/initialized by this job.
         *
         * This may hold either an actor to configure or a handle that will be populated during
         * execution.
         */
        AtomicSmartPtr<scene::IGameActor> m_actor;

        /**
         * @brief Properties used to configure the actor during loading/initialization.
         *
         * This object provides name/value configuration used by the job to set up the actor.
         */
        AtomicSmartPtr<Properties> m_properties;

        /**
         * @brief Controls whether the job should create child jobs for any child actors.
         *
         * Default is true. Set to false when callers want to manage creation/scheduling of child
         * jobs themselves.
         */
        atomic_bool m_createChildJobs = true;
    };
}  // namespace workphone

#endif  // ActorLoadJob_h__
