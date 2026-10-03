#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/ILightmap.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ILightmap, ISharedObject );

    ILightmap::~ILightmap() = default;

}  // namespace workphone::render
