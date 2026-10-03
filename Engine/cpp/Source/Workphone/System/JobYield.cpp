#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/JobYield.hpp>
#include <Workphone/System/Job.hpp>
#include <utility>

namespace workphone::core
{

    JobYield::JobYield() = default;

    JobYield::JobYield( SmartPtr<IJob> job ) : m_job( std::move( job ) )
    {
    }

    JobYield::JobYield( SmartPtr<ICoroutineData> &jobYield )
    {
    }

    void JobYield::stop()
    {
        m_job->setState( Job::State::Finish );
    }

    JobYield::~JobYield() = default;

}  // namespace workphone::core
