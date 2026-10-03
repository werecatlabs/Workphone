#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/State/Messages/StateMessageJobStatus.hpp"
#include <Workphone/Interface/System/IJob.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageJobStatus, StateMessage );

    //-------------------------------------------------
    StateMessageJobStatus::StateMessageJobStatus() : m_jobStatus( 0 )
    {
    }

    //-------------------------------------------------
    StateMessageJobStatus::StateMessageJobStatus( u32 status ) : m_jobStatus( status )
    {
    }

    //-------------------------------------------------
    StateMessageJobStatus::~StateMessageJobStatus() = default;

    //-------------------------------------------------
    auto StateMessageJobStatus::getJob() const -> SmartPtr<IJob>
    {
        return m_job;
    }

    //-------------------------------------------------
    void StateMessageJobStatus::setJob( SmartPtr<IJob> job )
    {
        m_job = job;
    }

    //-------------------------------------------------
    auto StateMessageJobStatus::getJobStatus() const -> u32
    {
        return m_jobStatus;
    }

    //-------------------------------------------------
    void StateMessageJobStatus::setJobStatus( u32 value )
    {
        m_jobStatus = value;
    }
}  // namespace workphone
