#ifndef ActorEnableJob_h__
#define ActorEnableJob_h__

#include <Workphone/System/Job.hpp>

namespace workphone
{

    /**
     * \brief Job that enables or disables a scene actor.
     *
     * This job is intended to be dispatched to the engine's job/worker system to
     * change an actor's enabled state in a thread-safe manner. The job stores an
     * atomic reference to the target actor and an atomic boolean representing
     * the requested enabled state. When executed on the main/render thread (or
     * an appropriate thread that can safely modify scene objects), the job will
     * set the actor's enabled state accordingly.
     *
     * Usage notes:
     * - Use `setActor(...)` to provide the target actor before scheduling the job.
     * - Use `setEnable(true)` to enable the actor or `setEnable(false)` to disable it.
     * - The job itself does not perform lifetime management beyond the SmartPtr
     *   stored in `m_actor`. Ensure the actor remains valid for the lifetime of
     *   the job or update the actor reference appropriately.
     *
     * Thread-safety:
     * - The actor reference is stored in an `AtomicSmartPtr` and the enabled flag
     *   is stored in an `atomic_bool` so callers may set these from other threads.
     * - Actual modifications to the actor should occur on the thread that owns
     *   scene updates; `execute()` should be scheduled accordingly.
     */
    class WPCore_API ActorEnableJob : public Job
    {
    public:
        /**
         * \brief Constructs an ActorEnableJob.
         *
         * The job is initialised with no actor set and the enabled flag defaulting
         * to true. Callers should set the actor and desired enabled state before
         * scheduling the job.
         */
        ActorEnableJob();

        /**
         * \brief Virtual destructor.
         *
         * Cleans up job resources. If the job is still referenced by a worker
         * queue, ensure it is not destroyed while queued.
         */
        ~ActorEnableJob() override;

        /**
         * \brief Execute the job action.
         *
         * This overrides `Job::execute()` and will apply the stored enabled state
         * to the stored actor (if present). The method checks whether the actor
         * pointer is valid before attempting to change its state.
         *
         * \note Execution should be performed on the appropriate thread for
         *       modifying scene/actor state to avoid race conditions.
         *
         * \copydoc Job::execute
         */
        void execute() override;

        /**
         * \brief Get the actor targeted by this job.
         *
         * \return SmartPtr to the actor, may be null if no actor has been set.
         */
        SmartPtr<scene::IGameActor> getActor() const;

        /**
         * \brief Set the actor to enable/disable.
         *
         * The actor is stored using an atomic smart pointer allowing safe updates
         * from other threads. If the job has already been scheduled, updating
         * the actor after scheduling may have no effect on that scheduled run.
         *
         * \param actor SmartPtr to the target actor (may be null).
         */
        void setActor( SmartPtr<scene::IGameActor> actor );

        /**
         * \brief Get the desired enabled state.
         *
         * \return true if the job will enable the actor; false to disable it.
         */
        bool getEnable() const;

        /**
         * \brief Set the desired enabled state for the actor.
         *
         * This flag is stored atomically so it can be changed from other threads
         * prior to the job being executed.
         *
         * \param enable true to enable the actor, false to disable.
         */
        void setEnable( bool enable );

        WP_CLASS_REGISTER_DECL;

    protected:
        /** Atomic smart pointer to the actor this job will operate on. */
        AtomicSmartPtr<scene::IGameActor> m_actor;

        /** Atomic flag indicating whether the actor should be enabled (true)
         * or disabled (false) when the job is executed. Defaults to true. */
        atomic_bool m_enable = true;
    };
}  // namespace workphone

#endif  // ActorEnableJob_h__
