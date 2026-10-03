#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IOverlayElementContainer.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IOverlayElementContainer, IOverlayElement );

    IOverlayElementContainer::~IOverlayElementContainer() = default;
}  // namespace workphone::render
