#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/GraphicsSceneState.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, GraphicsSceneState, StateData );

    GraphicsSceneState::GraphicsSceneState() = default;

    GraphicsSceneState::~GraphicsSceneState() = default;

}  // namespace workphone
