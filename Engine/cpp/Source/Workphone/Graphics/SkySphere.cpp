#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/SkySphere.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/State/States/SkyStateData.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, SkySphere, Sky<ISkySphere> );

    SkySphere::SkySphere() = default;

    SkySphere::~SkySphere() = default;

}  // namespace workphone::render
