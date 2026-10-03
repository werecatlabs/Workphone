#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/RenderTargetStateData.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, RenderTargetStateData, StateData );

    RenderTargetStateData::RenderTargetStateData() = default;

    RenderTargetStateData::~RenderTargetStateData() = default;

}  // namespace workphone
