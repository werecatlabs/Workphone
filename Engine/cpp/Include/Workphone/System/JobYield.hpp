#ifndef JobYield_h__
#define JobYield_h__

#include <Workphone/Memory/CoroutineData.hpp>

/**
 * @file JobYield.hpp
 * @brief Coroutine data type used when a coroutine yields execution to a job.
 *
 * This header declares the JobYield class which derives from
 * `CoroutineData` and carries a `SmartPtr<IJob>` describing the job
 * the coroutine is waiting on. The scheduler inspects this object to
 * know how to resume the coroutine when the job completes.
 */

namespace workphone
{
    namespace core
    {

        /**
         * @class JobYield
         * @brief Represents a coroutine yield that is waiting on an `IJob`.
         *
         * `JobYield` wraps a `SmartPtr<IJob>` and is used by the coroutine
         * scheduler to represent a coroutine that has yielded because it is
         * waiting for a background job to finish. When the job completes the
         * scheduler can resume the coroutine associated with this object.
         */
        class WPCore_API JobYield : public CoroutineData
        {
        public:
            /**
             * @brief Construct an empty JobYield (no job associated).
             *
             * This can be used to represent a yield point that does not
             * currently reference an active job.
             */
            JobYield();

            /**
             * @brief Construct a JobYield that wraps the provided job.
             * @param job Smart pointer to the job the coroutine yields to.
             */
            JobYield( SmartPtr<IJob> job );

            /**
             * @brief Construct from another coroutine data object.
             * @param jobYield Reference to an existing coroutine data instance
             *        to initialize from. Useful when adapting or forwarding
             *        coroutine data objects.
             */
            JobYield( SmartPtr<ICoroutineData> &jobYield );

            /**
             * @brief Virtual destructor.
             */
            ~JobYield() override;

            /**
             * @brief Stop any work associated with this yield and release
             *        held resources.
             *
             * Implementations should ensure the associated job is stopped
             * (if applicable) and the internal pointer is cleared so the
             * coroutine will not be resumed unexpectedly.
             */
            void stop() override;

            /**
             * @brief Smart pointer to the job the coroutine is waiting on.
             *
             * When non-null this points to the job instance that must
             * complete before the coroutine can be resumed.
             */
            SmartPtr<IJob> m_job;
        };

    }  // namespace core
}  // namespace workphone

#endif  // JobYield_h__
