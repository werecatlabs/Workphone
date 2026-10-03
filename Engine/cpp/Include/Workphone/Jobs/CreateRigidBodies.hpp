#ifndef CreateRigidBodies_h__
#define CreateRigidBodies_h__

#include <Workphone/System/Job.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>

namespace workphone
{
    /**
     * @file CreateRigidBodies.hpp
     * @brief Job that creates physics rigid bodies for an actor and optionally its children.
     *
     * This job is intended to be scheduled on the engine job system to create and configure
     * physics rigid bodies for a scene actor. It supports creating bodies only for the
     * specified actor or cascading the creation to all child actors. The job also allows
     * setting flags for creating static bodies and using convex collision shapes.
     */
    class WPCore_API CreateRigidBodies : public Job
    {
    public:
        /**
         * @brief Constructs a new CreateRigidBodies job.
         *
         * Creates a job instance with default options (non-cascading, dynamic, non-convex).
         */
        CreateRigidBodies();

        /**
         * @brief Destructor.
         *
         * Ensures any resources owned by the job are released. The destructor is virtual
         * because this class derives from Job.
         */
        ~CreateRigidBodies() override;

        /**
         * @copydoc Job::execute
         *
         * Executes the rigid body creation using the configured actor and options.
         * If no actor is set, this method will return without performing any work.
         */
        void execute() override;

        /**
         * @brief Gets the actor for which rigid bodies will be created.
         * @return SmartPtr to the actor. May be null if no actor has been set.
         */
        SmartPtr<scene::IGameActor> getActor() const;

        /**
         * @brief Sets the actor for which rigid bodies will be created.
         * @param actor SmartPtr to the target actor. Passing a null pointer clears the actor.
         */
        void setActor( SmartPtr<scene::IGameActor> actor );

        /**
         * @brief Returns whether rigid bodies should be created for all child actors.
         * @return True if rigid bodies should be created for the actor's children as well.
         */
        bool getCascade() const;

        /**
         * @brief Sets the cascade option.
         * @param cascade True to create rigid bodies for the actor and all descendants.
         */
        void setCascade( bool cascade );

        /**
         * @brief Returns whether created rigid bodies should be static.
         * @return True if rigid bodies will be made static (non-simulated).
         */
        bool getMakeStatic() const;

        /**
         * @brief Sets whether created rigid bodies should be static.
         * @param makeStatic True to make created rigid bodies static.
         */
        void setMakeStatic( bool makeStatic );

        /**
         * @brief Returns whether convex collision shapes should be used.
         * @return True if convex collision shapes are requested.
         */
        bool isConvex() const;

        /**
         * @brief Sets whether to use convex collision shapes for created bodies.
         * @param convex True to request convex collision shapes.
         */
        void setConvex( bool convex );

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Internal helper that creates rigid bodies for an actor.
         *
         * This method performs the actual creation and configuration of rigid bodies
         * for the supplied actor. If @p cascade is true, the function will recurse
         * into the actor's children and create bodies for them as well.
         *
         * @param actor The actor to process. Must be a valid pointer.
         * @param cascade If true, create rigid bodies for child actors recursively.
         */
        void createRigidBodies( SmartPtr<scene::IGameActor> actor, bool cascade );

        /** @brief The actor to create rigid bodies for. May be null. */
        AtomicSmartPtr<scene::IGameActor> m_actor;

        /** @brief If true, create rigid bodies for all children of the actor. */
        atomic_bool m_cascade = false;

        /** @brief If true, created rigid bodies will be static (non-dynamic). */
        atomic_bool m_makeStatic = false;

        /** @brief If true, convex collision shapes will be used when possible. */
        atomic_bool m_isConvex = false;
    };
}  // namespace workphone

#endif  // CreateRigidBodies_h__
