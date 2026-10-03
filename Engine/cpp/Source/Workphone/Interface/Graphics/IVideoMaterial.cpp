#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IVideoMaterial.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IVideoMaterial, ISharedObject );

    IVideoMaterial::~IVideoMaterial() = default;

}  // namespace workphone::render
