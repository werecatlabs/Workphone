#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/IGameInputState.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IGameInputState, ISharedObject );

    IGameInputState::~IGameInputState() = default;

}  // namespace workphone
