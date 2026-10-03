#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/ITerrainBlendMap.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ITerrainBlendMap, ISharedObject );

    ITerrainBlendMap::~ITerrainBlendMap() = default;

}  // namespace workphone::render
