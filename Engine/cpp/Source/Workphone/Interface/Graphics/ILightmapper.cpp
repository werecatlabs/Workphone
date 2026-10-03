#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/ILightmapper.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ILightmapper, ISharedObject );

    ILightmapper::~ILightmapper() = default;

}  // namespace workphone::render
