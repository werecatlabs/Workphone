#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/ISkyboxCube.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, ISkyboxCube, ISkybox );

    ISkyboxCube::~ISkyboxCube() = default;

}  // namespace workphone::render
