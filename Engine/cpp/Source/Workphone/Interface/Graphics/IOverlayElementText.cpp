#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IOverlayElementText.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IOverlayElementText, IOverlayElement );

    IOverlayElementText::~IOverlayElementText() = default;
}  // namespace workphone::render
