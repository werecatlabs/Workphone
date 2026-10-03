#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainBlendMap.hpp>
#include <Workphone/Interface/Graphics/ITerrainBlendMap.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone, TerrainBlendMap, SubComponent );

    TerrainBlendMap::TerrainBlendMap() = default;

    TerrainBlendMap::~TerrainBlendMap() = default;

    void TerrainBlendMap::load( SmartPtr<ISharedObject> data )
    {
        try
        {
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TerrainBlendMap::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto TerrainBlendMap::getBlendMap() const -> SmartPtr<render::ITerrainBlendMap>
    {
        return m_blendMap;
    }

    void TerrainBlendMap::setBlendMap( SmartPtr<render::ITerrainBlendMap> blendMap )
    {
        m_blendMap = blendMap;
    }
}  // namespace workphone::scene
