#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/OverlayTextState.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, OverlayTextState, OverlayElementState );

    OverlayTextState::OverlayTextState() = default;
    OverlayTextState::~OverlayTextState() = default;

}  // namespace workphone
