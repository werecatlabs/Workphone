#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/TerrainStateData.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, TerrainStateData, StateData );

    TerrainStateData::TerrainStateData() : StateData( TerrainStateData::typeInfo() )
    {
        textures.resize( 24 );
        metalnessValues.resize( 24 );
        heightData.resize( heightMapSize.x * heightMapSize.y );
    }

    TerrainStateData::~TerrainStateData() = default;

}  // namespace workphone
