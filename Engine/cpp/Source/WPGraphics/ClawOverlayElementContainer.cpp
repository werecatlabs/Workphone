#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawOverlayElementContainer.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawOverlayElementContainer,
                               IOverlayElementContainer );

    ClawOverlayElementContainer::ClawOverlayElementContainer() = default;
    ClawOverlayElementContainer::~ClawOverlayElementContainer() = default;
}  // namespace workphone::render
