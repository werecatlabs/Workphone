#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/TextureSamplerState.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, TextureSamplerState, StateData );

    TextureSamplerState::TextureSamplerState() : StateData( TextureSamplerState::typeInfo() )
    {
    }

    TextureSamplerState::~TextureSamplerState() = default;

}  // namespace workphone
