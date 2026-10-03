#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/JobFunction.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, JobFunction, Job );

    JobFunction::JobFunction() = default;

    JobFunction::~JobFunction() = default;

    void JobFunction::execute()
    {
        if( isInterrupted() )
        {
            return;
        }

        if( m_function )
        {
            m_function();
        }
    }

    std::function<void()> JobFunction::getFunction() const
    {
        return m_function;
    }

    void JobFunction::setFunction( std::function<void()> &function )
    {
        m_function = function;
    }

}  // namespace workphone
