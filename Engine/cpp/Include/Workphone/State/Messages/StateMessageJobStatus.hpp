#ifndef StateMessageJobStatus_h__
#define StateMessageJobStatus_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{

    class WPCore_API StateMessageJobStatus : public StateMessage
    {
    public:
        enum
        {
            JOB_STATUS_QUEUED,
            JOB_STATUS_START,
            JOB_STATUS_END,
            JOB_STATUS_DESTROY,

            JOB_STATUS_COUNT
        };

        StateMessageJobStatus();
        explicit StateMessageJobStatus( u32 status );
        ~StateMessageJobStatus() override;

        SmartPtr<IJob> getJob() const;
        void setJob( SmartPtr<IJob> job );

        u32 getJobStatus() const;
        void setJobStatus( u32 value );

        WP_CLASS_REGISTER_DECL;

    protected:
        SmartPtr<IJob> m_job;
        u32 m_jobStatus;
    };
}  // namespace workphone

#endif  // StateMessageJobStatus_h__
