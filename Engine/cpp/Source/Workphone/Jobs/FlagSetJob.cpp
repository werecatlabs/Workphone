#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/FlagSetJob.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, FlagSetJob, Job );

    FlagSetJob::FlagSetJob() = default;

    FlagSetJob::~FlagSetJob() = default;

    void FlagSetJob::execute()
    {
    }

    void FlagSetJob::setFlag( u32 flag )
    {
        m_flag = flag;
    }

    auto FlagSetJob::getFlag() const -> u32
    {
        return m_flag;
    }

    std::function<void( int )> FlagSetJob::getCallbackFunc() const
    {
        return m_callbackFunc;
    }

    void FlagSetJob::setCallbackFunc( std::function<void( int )> callbackFunc )
    {
        m_callbackFunc = callbackFunc;
    }

}  // namespace workphone
