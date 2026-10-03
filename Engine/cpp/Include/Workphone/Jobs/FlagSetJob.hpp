#ifndef FlagSetJob_h__
#define FlagSetJob_h__

#include <Workphone/System/Job.hpp>
#include <functional>

namespace workphone
{

    /**
     * @file FlagSetJob.hpp
     * @brief Small job used to store a flag value and optionally invoke a callback.
     *
     * This job carries a 32-bit flag and an optional callback function that accepts
     * an integer. The job's execution typically invokes the callback with the stored
     * flag value (cast to int) if a callback has been supplied.
     *
     * @note This header only declares the interface. See the corresponding .cpp for
     * the concrete behaviour executed by `execute()`.
     */
    class WPCore_API FlagSetJob : public Job
    {
    public:
        /**
         * @brief Construct a FlagSetJob.
         *
         * Initializes the job with a default flag value of 0 and no callback.
         */
        FlagSetJob();

        /**
         * @brief Virtual destructor.
         *
         * Cleans up any resources held by the job. Override is declared to ensure
         * correct polymorphic destruction.
         */
        ~FlagSetJob() override;

        /**
         * @brief Execute the job.
         *
         * When executed, the job performs its work. In the typical usage for this
         * class, if a callback function has been provided via `setCallbackFunc`,
         * it will be called with the stored flag value (converted to `int`).
         *
         * This method overrides `Job::execute()`.
         */
        void execute() override;

        /**
         * @brief Set the stored flag value.
         *
         * @param flag The 32-bit unsigned flag to store in the job.
         */
        void setFlag( u32 flag );

        /**
         * @brief Get the stored flag value.
         *
         * @return The currently stored 32-bit unsigned flag.
         */
        u32 getFlag() const;

        /**
         * @brief Get the callback function.
         *
         * Returns the callback function that will be (or has been) invoked by
         * `execute()`. The callback accepts a single `int` parameter which is
         * normally the stored flag value.
         *
         * @return A std::function taking an int and returning void. May be empty.
         */
        std::function<void( int )> getCallbackFunc() const;

        /**
         * @brief Set the callback function to be invoked when the job runs.
         *
         * @param callbackFunc Function to call during job execution. The function
         *                     receives a single integer parameter (commonly the
         *                     stored flag value). Passing an empty std::function
         *                     clears the callback.
         */
        void setCallbackFunc( std::function<void( int )> callbackFunc );

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Optional callback invoked during execution.
         *
         * The callback takes a single `int` argument. Typical usage is to pass
         * the stored `m_flag` value (converted to `int`) when invoking.
         */
        std::function<void( int )> m_callbackFunc;

        /**
         * @brief Stored 32-bit flag value for the job.
         *
         * Default-initialized to 0.
         */
        u32 m_flag = 0;
    };

}  // namespace workphone

#endif  // FlagSetJob_h__
