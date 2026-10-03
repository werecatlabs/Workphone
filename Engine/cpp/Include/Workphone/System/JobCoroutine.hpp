#ifndef JobCoroutine_h__
#define JobCoroutine_h__

#include <Workphone/System/Job.hpp>
#include <Workphone/Interface/System/ICoroutineData.hpp>

namespace workphone
{

    /**
     * @brief Job implementation that runs a coroutine.
     *
     * JobCoroutine wraps a callable coroutine function and drives its execution
     * via Boost.Coroutines2. The supplied function should accept an
     * `ICoroutineData::PullType &` and use it to yield data back to the
     * caller. This class integrates that coroutine with the engine's
     * Job interface so coroutines can be scheduled and stepped like other
     * jobs.
     *
     * Usage:
     * - Provide the coroutine body via `setFunction(...)`.
     * - `coroutine_execute()` will run the coroutine to completion.
     * - `coroutine_execute_step(...)` will resume the coroutine one or more
     *   steps and return yielded data via the `yield` parameter.
     *
     * Note: The exact resume/yield semantics depend on the coroutine body
     * and the `ICoroutineData` implementation.
     */
    class JobCoroutine : public Job
    {
    public:
        /**
         * @brief Construct a new JobCoroutine.
         *
         * Initializes internal state. No coroutine function is set by default.
         */
        JobCoroutine();

        /**
         * @brief Destroy the JobCoroutine.
         *
         * Ensures any coroutine resources are released. If a coroutine is
         * currently active it will be destroyed when this object is destroyed.
         */
        ~JobCoroutine() override;

        /**
         * @brief Execute the coroutine to completion.
         *
         * This overrides `Job::coroutine_execute`. When called, the stored
         * coroutine function (if any) is driven until it finishes. Any yielded
         * values are handled according to the Job/Coroutine integration.
         *
         * If no function has been set, this call is a no-op.
         */
        void coroutine_execute() override;

        /**
         * @brief Execute the coroutine for a single step (or until the next yield).
         *
         * @param[in,out] yield Smart pointer used to receive data yielded by the coroutine.
         *
         * This overrides `Job::coroutine_execute_step`. The coroutine is resumed
         * and may populate `yield` with an `ICoroutineData` object. Callers can
         * inspect the yielded data and schedule further steps as needed.
         */
        void coroutine_execute_step( SmartPtr<ICoroutineData> &yield ) override;

        /**
         * @brief Get the stored coroutine function.
         *
         * @return std::function<void(ICoroutineData::PullType &)> A copy of the
         *         currently stored coroutine callable. If no function has been
         *         set this will be an empty `std::function`.
         */
        std::function<void( ICoroutineData::PullType & )> getFunction() const;

        /**
         * @brief Set the coroutine function to execute.
         *
         * @param[in] function Callable that implements the coroutine body. It
         *                     receives an `ICoroutineData::PullType &` which it
         *                     can use to yield control/data back to the caller.
         *
         * The provided function will be used to create and drive a Boost
         * coroutine when execution begins. Passing an empty `function` will
         * clear any existing coroutine body.
         */
        void setFunction( std::function<void( ICoroutineData::PullType & )> &function );

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Internal helper invoked by the Boost coroutine to run a step.
         *
         * @param[in] pull The coroutine pull type supplied by Boost.Coroutines2.
         *
         * This method is used as the coroutine entry point and forwards control
         * to the user-provided `m_function`. It is declared protected because
         * it is an implementation detail and should not be called externally.
         */
        void coroutine_step( ICoroutineData::PullType &pull );

        /**
         * @brief The user-supplied coroutine body.
         *
         * The callable should accept an `ICoroutineData::PullType &` and use
         * it to yield data. This is copied by `setFunction` and returned by
         * `getFunction`.
         */
        std::function<void( ICoroutineData::PullType & )> m_function;

        /**
         * @brief Boost push-type coroutine used to resume the coroutine from outside.
         *
         * This object controls execution of the coroutine. It is constructed
         * when a function is set and drives calls into `coroutine_step`.
         */
        boost::coroutines2::coroutine<void>::push_type m_coroutine;

        /**
         * @brief Count of how many times the coroutine has been resumed / stepped.
         *
         * Useful for diagnostics or to detect re-entrancy/looping behaviour.
         * Initialized to 0.
         */
        u32 m_coroutineCount = 0;
    };
}  // namespace workphone

#endif  // JobCoroutine_h__
