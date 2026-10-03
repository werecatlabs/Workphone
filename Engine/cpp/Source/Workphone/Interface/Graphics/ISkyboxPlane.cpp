#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/ISkyboxPlane.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, ISkyboxPlane, ISkybox );

    ISkyboxPlane::~ISkyboxPlane() = default;

}  // namespace workphone::render
