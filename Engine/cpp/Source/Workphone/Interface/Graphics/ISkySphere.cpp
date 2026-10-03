#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/ISkySphere.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, ISkySphere, ISky );

    ISkySphere::~ISkySphere() = default;

}  // namespace workphone::render
