#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/OverlayState.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, OverlayState, StateData );

    OverlayState::OverlayState() = default;

    OverlayState::~OverlayState() = default;

    auto OverlayState::isVisible() const -> bool
    {
        return m_visible;
    }

    void OverlayState::setVisible( bool visible )
    {
        m_visible = visible;
    }

}  // namespace workphone
