#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/RunCommandJob.hpp>
#include <Workphone/Interface/System/ICommand.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, RunCommandJob, Job );

    RunCommandJob::RunCommandJob() = default;

    RunCommandJob::~RunCommandJob() = default;

    void RunCommandJob::execute()
    {
        if( auto command = getCommand() )
        {
            command->execute();
        }
    }

    auto RunCommandJob::getCommand() const -> SmartPtr<ICommand>
    {
        return m_command;
    }

    void RunCommandJob::setCommand( SmartPtr<ICommand> command )
    {
        m_command = command;
    }
}  // namespace workphone
