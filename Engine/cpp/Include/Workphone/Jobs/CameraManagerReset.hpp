#ifndef CameraManagerReset_h__
#define CameraManagerReset_h__

#include <Workphone/System/Job.hpp>

namespace workphone
{

    /**
     * @brief Job that resets the camera manager.
     *
     * This job encapsulates the work required to perform a reset of the camera
     * manager within the Workphone runtime. It can optionally be associated
     * with an owner object and configured with a delay before the reset is
     * applied.
     *
     * Typical usage:
     * - Create an instance of this job.
     * - Optionally call `setOwner` to attach an owner object that represents
     *   the context for the reset (for example, to prevent the reset if the
     *   owner is no longer valid).
     * - Optionally call `setDelayTime` to schedule the reset after a short
     *   pause.
     * - Submit the job to the job system so `execute` will be invoked.
     *
     * The concrete reset logic is implemented in `execute()` which overrides
     * `Job::execute`.
     */
    class WPCore_API CameraManagerReset : public Job
    {
    public:
        /**
         * @brief Construct a CameraManagerReset job.
         *
         * The job is constructed with no owner and a default delay time of
         * 0.0f (meaning the reset should be applied immediately when
         * `execute()` runs, subject to the job system's scheduling).
         */
        CameraManagerReset();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup when the job is deleted through a base
         * pointer.
         */
        ~CameraManagerReset() override;

        /**
         * @brief Execute the reset operation.
         *
         * This method contains the logic that performs the camera manager
         * reset. It overrides `Job::execute` so it will be called by the job
         * system when the job is run. Implementations should respect the
         * configured `m_delayTime` and the optional `m_owner` when deciding
         * whether and when to perform the reset.
         */
        void execute() override;

        /**
         * @brief Get the owner associated with this job.
         *
         * The owner is an optional `ISharedObject` used to represent the
         * context for the reset. The job implementation may use the owner to
         * verify that the context is still valid before performing the
         * reset.
         *
         * @return SmartPtr<ISharedObject> The current owner or a null smart
         * pointer if none is set.
         */
        ISharedObject *getOwner() const;

        /**
         * @brief Set the owner associated with this job.
         *
         * @param owner A smart pointer to an `ISharedObject` representing the
         * context for the reset. Passing a null smart pointer clears the
         * owner.
         */
        void setOwner( ISharedObject *owner );

        /**
         * @brief Get the configured delay time (in seconds).
         *
         * The delay time specifies how long the job should wait before the
         * reset is applied. A value of 0.0f indicates the reset should occur
         * immediately when `execute()` is invoked.
         *
         * @return f32 Delay time in seconds.
         */
        f32 getDelayTime() const;

        /**
         * @brief Set the delay time (in seconds) before performing the reset.
         *
         * @param delayTime The delay in seconds. Negative values are not
         * recommended and behavior for such values is unspecified.
         */
        void setDelayTime( f32 delayTime );

        WP_CLASS_REGISTER_DECL;

    protected:
        // The owner of this job.
        SmartPtr<ISharedObject> m_owner;

        // The delay time.
        f32 m_delayTime = 0.0f;
    };
}  // namespace workphone

#endif  // CameraManagerReset_h__
