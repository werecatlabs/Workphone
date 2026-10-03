#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IOverlayElementVector.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IOverlayElementVector, IOverlayElement );

    IOverlayElementVector::~IOverlayElementVector() = default;
}  // namespace workphone::render
