#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/RenderTextureState.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, RenderTextureState, StateData );

    RenderTextureState::RenderTextureState() = default;

    RenderTextureState::~RenderTextureState() = default;

}  // namespace workphone
