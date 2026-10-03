#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/ICommand.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ICommand, ISharedObject );

    ICommand::~ICommand() = default;
}  // namespace workphone
