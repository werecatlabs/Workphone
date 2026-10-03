#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/IGameInput.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IGameInput, ISharedObject );
    const u32 IGameInput::UNASSIGNED = std::numeric_limits<u32>::max();

    IGameInput::~IGameInput() = default;

}  // namespace workphone
