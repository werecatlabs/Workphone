#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainLayer.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, TerrainLayer, SubComponent );

    const String TerrainLayer::BaseTextureStr = "baseTexture";
    const String TerrainLayer::IndexStr = "index";

    TerrainLayer::TerrainLayer() = default;

    TerrainLayer::~TerrainLayer() = default;

    void TerrainLayer::load( SmartPtr<ISharedObject> data )
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

    void TerrainLayer::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            m_baseTexture = nullptr;
            SubComponent::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto TerrainLayer::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = SubComponent::getProperties();

        properties->setProperty( BaseTextureStr, getBaseTexture() );
        properties->setProperty( IndexStr, getIndex() );

        return properties;
    }

    void TerrainLayer::setProperties( SmartPtr<Properties> properties )
    {
        auto baseTexture = getBaseTexture();
        auto index = getIndex();

        properties->getPropertyValue( BaseTextureStr, baseTexture );
        properties->getPropertyValue( IndexStr, index );

        setIndex( index );

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

    auto TerrainLayer::getBaseTexture() const -> SmartPtr<render::ITexture>
    {
        return m_baseTexture;
    }

    void TerrainLayer::setBaseTexture( SmartPtr<render::ITexture> baseTexture )
    {
        m_baseTexture = baseTexture;
    }

    auto TerrainLayer::getIndex() const -> s32
    {
        return m_index;
    }

    void TerrainLayer::setIndex( s32 index )
    {
        m_index = index;
    }

}  // namespace workphone::scene
