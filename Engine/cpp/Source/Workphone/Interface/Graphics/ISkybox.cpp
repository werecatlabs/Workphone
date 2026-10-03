#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/ISkybox.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, ISkybox, ISky );

    ISkybox::~ISkybox() = default;

}  // namespace workphone::render
