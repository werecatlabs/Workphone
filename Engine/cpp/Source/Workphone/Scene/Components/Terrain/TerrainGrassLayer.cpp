#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainGrassLayer.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Scene/IGamePrefab.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, TerrainGrassLayer, SubComponent );

    const String TerrainGrassLayer::BaseTextureStr = "baseTexture";
    const String TerrainGrassLayer::IndexStr = "index";
    const String TerrainGrassLayer::PrefabPathStr = "prefabPath";
    const String TerrainGrassLayer::DensityStr = "density";

    TerrainGrassLayer::TerrainGrassLayer() = default;

    TerrainGrassLayer::~TerrainGrassLayer() = default;

    void TerrainGrassLayer::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TerrainGrassLayer::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            m_baseTexture = nullptr;
            m_prefab = nullptr;
            SubComponent::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto TerrainGrassLayer::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = SubComponent::getProperties();

        properties->setProperty( BaseTextureStr, getBaseTexture() );
        properties->setProperty( IndexStr, getIndex() );
        properties->setProperty( PrefabPathStr, getPrefabPath() );
        properties->setProperty( DensityStr, getDensity() );

        return properties;
    }

    void TerrainGrassLayer::setProperties( SmartPtr<Properties> properties )
    {
        auto baseTexture = getBaseTexture();
        auto index = getIndex();
        auto prefabPath = getPrefabPath();
        auto density = getDensity();

        properties->getPropertyValue( BaseTextureStr, baseTexture );
        properties->getPropertyValue( IndexStr, index );
        properties->getPropertyValue( PrefabPathStr, prefabPath );
        properties->getPropertyValue( DensityStr, density );

        setIndex( index );
        setPrefabPath( prefabPath );
        setDensity( density );

        if( baseTexture != getBaseTexture() )
        {
            setBaseTexture( baseTexture );

            if( auto parent = getParentComponent() )
            {
                auto terrain = workphone::static_pointer_cast<TerrainSystem>( parent );
                terrain->updateLayers();
            }
        }
    }

    auto TerrainGrassLayer::getBaseTexture() const -> SmartPtr<render::ITexture>
    {
        return m_baseTexture;
    }

    void TerrainGrassLayer::setBaseTexture( SmartPtr<render::ITexture> baseTexture )
    {
        m_baseTexture = baseTexture;
    }

    auto TerrainGrassLayer::getIndex() const -> s32
    {
        return m_index;
    }

    void TerrainGrassLayer::setIndex( s32 index )
    {
        m_index = index;
    }

    SmartPtr<IGamePrefab> TerrainGrassLayer::getPrefab() const
    {
        return m_prefab;
    }

    void TerrainGrassLayer::setPrefab( SmartPtr<IGamePrefab> prefab )
    {
        m_prefab = prefab;
    }

    String TerrainGrassLayer::getPrefabPath() const
    {
        return m_prefabPath;
    }

    void TerrainGrassLayer::setPrefabPath( const String &prefabPath )
    {
        m_prefabPath = prefabPath;
    }

    s32 TerrainGrassLayer::getDensity() const
    {
        return m_density;
    }

    void TerrainGrassLayer::setDensity( s32 density )
    {
        m_density = Math<s32>::max( 0, density );
    }

}  // namespace workphone::scene
