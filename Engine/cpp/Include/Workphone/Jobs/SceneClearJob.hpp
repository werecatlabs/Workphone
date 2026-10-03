#ifndef SceneClearJob_h__
#define SceneClearJob_h__

#include <Workphone/System/Job.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>

namespace workphone
{
    /**
     * @brief Job used to clear (destroy/unload) a scene.
     *
     * This Job encapsulates the operation of clearing a scene from the engine.
     * It stores a smart pointer to the scene that should be cleared and will
     * perform the actual clear operation when executed on a job worker thread.
     *
     * Typical usage:
     * - Create a SceneClearJob instance.
     * - Set the target scene via `setScene`.
     * - Submit the job to the job system; `execute` will be invoked by the worker.
     *
     * Threading: execution will occur on whatever thread the job system schedules
     * the job on. Ensure any scene pointers passed are safe to operate on from
     * that thread or are properly synchronized.
     */
    class WPCore_API SceneClearJob : public Job
    {
    public:
        /**
         * @brief Construct a new SceneClearJob.
         *
         * Initializes internal state. The scene pointer is initially empty and
         * should be set with `setScene` before scheduling the job.
         */
        SceneClearJob();

        /**
         * @brief Destroy the SceneClearJob.
         *
         * Default destructor ensures proper cleanup of smart pointers.
         */
        ~SceneClearJob() override;

        /**
         * @brief Execute the clear operation on the stored scene.
         *
         * This method is called by the job system when the job runs. It should
         * perform all steps necessary to clear the scene (destroy actors,
         * release resources, unregister objects, etc.). Implementations should
         * be robust to a null scene pointer.
         */
        void execute() override;

        /**
         * @brief Get the scene associated with this job.
         * @return SmartPtr<scene::IScene> The scene that will be cleared, or an empty pointer.
         */
        SmartPtr<scene::IGameScene> getScene() const;

        /**
         * @brief Set the scene to be cleared by this job.
         * @param scene Smart pointer to the scene that should be cleared.
         *
         * The job takes a reference to the scene via SmartPtr to ensure the
         * scene remains alive for the duration of the job (subject to the
         * semantics of SmartPtr).
         */
        void setScene( SmartPtr<scene::IGameScene> scene );

        /**
         * @brief Get the actors that will be removed from the scene.
         * @return Array<SmartPtr<scene::IGameActor>> The list of actors to be removed.
         */
        Array<SmartPtr<scene::IGameActor>> getActors() const;

        /**
         * @brief Set the actors that will be removed from the scene.
         * @param actors The list of actors to remove.
         */
        void setActors( const Array<SmartPtr<scene::IGameActor>> &actors );

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief The scene that will be cleared when `execute` runs.
         * Stored as a SmartPtr to manage lifetime across threads and job execution.
         */
        SmartPtr<scene::IGameScene> m_scene;

        /**
         * @brief The list of actors to be removed from the scene.
         */
        ConcurrentArray<SmartPtr<scene::IGameActor>> m_actors;
    };
}  // namespace workphone

#endif  // SceneClearJob_h__
