#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainLayer.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainTreeLayer.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainGrassLayer.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/IGraphicsTerrain.hpp>
#include <Workphone/Interface/Graphics/ITerrainBlendMap.hpp>
#include <Workphone/Interface/Graphics/ITextureManager.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/Scene/IGamePrefabManager.hpp>
#include <Workphone/Interface/Scene/IGamePrefab.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <cJSON.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, TerrainSystem, Component );
    WP_CLASS_REGISTER_DERIVED( workphone::scene, TerrainSystem::TerrainObjectListener, IEventListener );

    namespace
    {
        constexpr u32 MinHeightMapDimension = 2u;
        constexpr u32 MaxGeneratedHeightMapDimension = 4096u;
        const String DefaultTerrainTextureName = "checker.png";
        const String DefaultTerrainTexturePath = "G:/lioncat/Bin/Media/checker.png";
        const String DefaultTerrainDetailWeightTextureName = "TerrainDefaultDetailWeight_R1000";
        constexpr u32 DefaultTerrainDetailWeightTextureSize = 4u;

        f32 clamp01( f32 value )
        {
            if( value < 0.0f )
            {
                return 0.0f;
            }

            if( value > 1.0f )
            {
                return 1.0f;
            }

            return value;
        }

        f32 smoothstep( f32 edge0, f32 edge1, f32 value )
        {
            if( MathF::equals( edge0, edge1 ) )
            {
                return value < edge0 ? 0.0f : 1.0f;
            }

            auto t = clamp01( ( value - edge0 ) / ( edge1 - edge0 ) );
            return t * t * ( 3.0f - 2.0f * t );
        }

        u32 clampHeightMapDimension( u32 value )
        {
            if( value < MinHeightMapDimension )
            {
                return MinHeightMapDimension;
            }

            if( value > MaxGeneratedHeightMapDimension )
            {
                return MaxGeneratedHeightMapDimension;
            }

            return value;
        }

        bool getHeightDataElementCount( u32 width, u32 height, size_t &elementCount )
        {
            width = clampHeightMapDimension( width );
            height = clampHeightMapDimension( height );

            const auto widthAsSize = static_cast<size_t>( width );
            const auto heightAsSize = static_cast<size_t>( height );
            if( widthAsSize > std::numeric_limits<size_t>::max() / heightAsSize )
            {
                elementCount = 0;
                return false;
            }

            elementCount = widthAsSize * heightAsSize;
            return true;
        }

        f32 hashNoise( s32 x, s32 y )
        {
            auto n = static_cast<u32>( x ) * 374761393u + static_cast<u32>( y ) * 668265263u;
            n = ( n ^ ( n >> 13u ) ) * 1274126177u;
            n = n ^ ( n >> 16u );
            return static_cast<f32>( n ) / static_cast<f32>( 0xffffffffu );
        }

        f32 valueNoise( f32 x, f32 y )
        {
            const auto ix = static_cast<s32>( std::floor( x ) );
            const auto iy = static_cast<s32>( std::floor( y ) );
            const auto fx = x - static_cast<f32>( ix );
            const auto fy = y - static_cast<f32>( iy );

            const auto sx = fx * fx * ( 3.0f - 2.0f * fx );
            const auto sy = fy * fy * ( 3.0f - 2.0f * fy );

            const auto n00 = hashNoise( ix, iy );
            const auto n10 = hashNoise( ix + 1, iy );
            const auto n01 = hashNoise( ix, iy + 1 );
            const auto n11 = hashNoise( ix + 1, iy + 1 );

            const auto nx0 = n00 + ( n10 - n00 ) * sx;
            const auto nx1 = n01 + ( n11 - n01 ) * sx;
            return nx0 + ( nx1 - nx0 ) * sy;
        }

        f32 fractalNoise( f32 x, f32 y )
        {
            auto amplitude = 0.5f;
            auto frequency = 1.0f;
            auto value = 0.0f;
            auto normalizer = 0.0f;

            for( u32 i = 0; i < 5u; ++i )
            {
                value += valueNoise( x * frequency, y * frequency ) * amplitude;
                normalizer += amplitude;
                amplitude *= 0.5f;
                frequency *= 2.0f;
            }

            return normalizer > 0.0f ? value / normalizer : 0.0f;
        }

        f32 ridgeNoise( f32 x, f32 y )
        {
            auto n = fractalNoise( x, y );
            n = 1.0f - std::abs( n * 2.0f - 1.0f );
            return n * n;
        }

        SmartPtr<render::ITexture> loadDefaultTerrainTexture()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return nullptr;
            }

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto textureManager = graphicsSystem ? graphicsSystem->getTextureManager() : nullptr;
            if( !textureManager )
            {
                return nullptr;
            }

            auto resourceDatabase = applicationManager->getResourceDatabase();
            if( !resourceDatabase )
            {
                return nullptr;
            }

            Array<String> textureCandidates;
            textureCandidates.push_back( DefaultTerrainTextureName );

            auto mediaPath = applicationManager->getMediaPath();
            if( !StringUtil::isNullOrEmpty( mediaPath ) )
            {
                textureCandidates.push_back( mediaPath + String( "/" ) + DefaultTerrainTextureName );
            }

            textureCandidates.push_back( DefaultTerrainTexturePath );

            for( auto &textureName : textureCandidates )
            {
                if( auto texture =
                        resourceDatabase->loadResourceByType<render::ITexture>( textureName ) )
                {
                    return texture;
                }
            }

            WP_LOG_WARNING( "TerrainSystem: failed to load default terrain texture checker.png." );
            return nullptr;
        }

        SmartPtr<render::ITexture> loadDefaultTerrainDetailWeightTexture()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return nullptr;
            }

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto textureManager = graphicsSystem ? graphicsSystem->getTextureManager() : nullptr;
            if( !textureManager )
            {
                return nullptr;
            }

            if( auto texture = workphone::dynamic_pointer_cast<render::ITexture>(
                    textureManager->getByName( DefaultTerrainDetailWeightTextureName ) ) )
            {
                return texture;
            }

            auto texture = textureManager->createManual(
                DefaultTerrainDetailWeightTextureName, "General",
                static_cast<u8>( TextureType::TEX_TYPE_2D ), DefaultTerrainDetailWeightTextureSize,
                DefaultTerrainDetailWeightTextureSize, 1u, 1,
                static_cast<u8>( PixelFormat::PF_R8G8B8A8 ),
                static_cast<s32>( TextureUsage::TU_STATIC_WRITE_ONLY ) );
            if( !texture )
            {
                return nullptr;
            }

            Array<u8> pixels( static_cast<size_t>( DefaultTerrainDetailWeightTextureSize ) *
                                  static_cast<size_t>( DefaultTerrainDetailWeightTextureSize ) * 4u,
                              0u );
            for( size_t i = 0u; i < pixels.size(); i += 4u )
            {
                pixels[i] = 255u;
            }

            texture->copyData( pixels.data(),
                               Vector2I( static_cast<s32>( DefaultTerrainDetailWeightTextureSize ),
                                         static_cast<s32>( DefaultTerrainDetailWeightTextureSize ) ) );
            return texture;
        }

        void ensureDefaultTerrainTextures( Array<SmartPtr<render::ITexture>> &textures,
                                           bool forceDefault )
        {
            const auto textureCount =
                static_cast<size_t>( render::IGraphicsTerrain::TextureTypes::COUNT );
            if( textures.size() < textureCount )
            {
                textures.resize( textureCount );
            }

            const auto diffuseSlot =
                static_cast<size_t>( render::IGraphicsTerrain::TextureTypes::DIFFUSE );
            const auto detailWeightSlot =
                static_cast<size_t>( render::IGraphicsTerrain::TextureTypes::DETAIL_WEIGHT );
            const auto detail0Slot =
                static_cast<size_t>( render::IGraphicsTerrain::TextureTypes::DETAIL0 );
            if( forceDefault )
            {
                for( size_t i = 0u; i < textureCount; ++i )
                {
                    textures[i] = nullptr;
                }
            }

            auto defaultTexture = loadDefaultTerrainTexture();
            if( !defaultTexture )
            {
                return;
            }

            if( forceDefault || !textures[diffuseSlot] )
            {
                textures[diffuseSlot] = defaultTexture;
            }

            if( forceDefault || !textures[detail0Slot] )
            {
                textures[detail0Slot] = defaultTexture;
            }

            auto detailWeightTexture = loadDefaultTerrainDetailWeightTexture();
            if( detailWeightTexture && ( forceDefault || !textures[detailWeightSlot] ) )
            {
                textures[detailWeightSlot] = detailWeightTexture;
            }
        }

        void applyTerrainTextures( SmartPtr<render::IGraphicsTerrain> terrain,
                                   const Array<SmartPtr<render::ITexture>> &textures )
        {
            if( !terrain )
            {
                return;
            }

            for( size_t i = 0u; i < textures.size(); ++i )
            {
                terrain->setTexture( static_cast<u32>( i ), textures[i] );
            }

            terrain->updateMaterial();
        }
    }  // namespace

    // Property key string definitions
    const String TerrainSystem::UpdateMaterialStr = "updateMaterial";
    const String TerrainSystem::LayersStr = "layers";
    const String TerrainSystem::HeightMapStr = "heightMap";
    const String TerrainSystem::HeightMapSizeStr = "heightMapSize";
    const String TerrainSystem::HeightScaleStr = "heightScale";
    const String TerrainSystem::ShowWireframeStr = "showWireframe";
    const String TerrainSystem::GenerateStr = "Generate";
    const String TerrainSystem::TexturesStr = "Textures";
    const String TerrainSystem::DiffuseTextureStr = "diffuseTexture";
    const String TerrainSystem::DetailWeightTextureStr = "detailWeightTexture";
    const String TerrainSystem::DetailTexture0Str = "detailTexture0";
    const String TerrainSystem::DetailTexture1Str = "detailTexture1";
    const String TerrainSystem::DetailTexture2Str = "detailTexture2";
    const String TerrainSystem::DetailTexture3Str = "detailTexture3";
    const String TerrainSystem::DetailTexture0NMStr = "detailTexture0NM";
    const String TerrainSystem::DetailTexture1NMStr = "detailTexture1NM";
    const String TerrainSystem::DetailTexture2NMStr = "detailTexture2NM";
    const String TerrainSystem::DetailTexture3NMStr = "detailTexture3NM";
    const String TerrainSystem::DetailRoughness0Str = "detailRoughness0";
    const String TerrainSystem::DetailRoughness1Str = "detailRoughness1";
    const String TerrainSystem::DetailRoughness2Str = "detailRoughness2";
    const String TerrainSystem::DetailRoughness3Str = "detailRoughness3";
    const String TerrainSystem::DetailMetalness0Str = "detailMetalness0";
    const String TerrainSystem::DetailMetalness1Str = "detailMetalness1";
    const String TerrainSystem::DetailMetalness2Str = "detailMetalness2";
    const String TerrainSystem::DetailMetalness3Str = "detailMetalness3";
    const String TerrainSystem::ReflectionTextureStr = "reflectionTexture";
    const String TerrainSystem::MetalnessStr = "Metalness";
    const String TerrainSystem::DetailMetalnessValue0Str = "detailMetalnessValue0";
    const String TerrainSystem::DetailMetalnessValue1Str = "detailMetalnessValue1";
    const String TerrainSystem::DetailMetalnessValue2Str = "detailMetalnessValue2";
    const String TerrainSystem::DetailMetalnessValue3Str = "detailMetalnessValue3";
    const String TerrainSystem::DefaultMetalnessValueStr = "defaultMetalnessValue";
    const String TerrainSystem::TreeSettingsStr = "Tree Settings";
    const String TerrainSystem::TreesEnableStr = "treesEnable";
    const String TerrainSystem::TreeDensityStr = "treeDensity";
    const String TerrainSystem::GrassSettingsStr = "Grass Settings";
    const String TerrainSystem::GrassEnableStr = "grassEnable";
    const String TerrainSystem::GrassDensityStr = "grassDensity";
    const String TerrainSystem::TreesStr = "Trees";
    const String TerrainSystem::GenerateTreesStr = "Generate Trees";
    const String TerrainSystem::AddTreesStr = "Add Trees";
    const String TerrainSystem::RemoveTreesStr = "Remove Trees";
    const String TerrainSystem::TreeLayerStr = "Tree Layer";
    const String TerrainSystem::TreeTextureStr = "treeTexture";
    const String TerrainSystem::GeneratedHeightMapWidthStr = "generatedHeightMapWidth";
    const String TerrainSystem::GeneratedHeightMapHeightStr = "generatedHeightMapHeight";
    const String TerrainSystem::GeneratedHeightMapValueScaleStr = "generatedHeightMapValueScale";
    const String TerrainSystem::GeneratedHeightMapTypeStr = "generatedHeightMapType";
    const String TerrainSystem::PreviewTreeCountStr = "previewTreeCount";
    const String TerrainSystem::GeneratedTreeCountStr = "generatedTreeCount";
    const String TerrainSystem::TreePrefabStr = "treePrefab";

    const Array<String> TerrainSystem::terrainTypes = { "island", "mountains", "gradient" };

    TerrainSystem::TerrainSystem()
    {
        auto data = std::make_shared<render::TerrainData>();
        data->dimensions = m_heightMapSize;
        data->origin = Vector2F( -128, -128 );  // Preserve the legacy sample-zero placement.
        data->heightScale = m_heightScale;
        data->heights.assign( 256 * 256, 0.0f );
        data->revision = 1;
        m_terrainData = data;
        m_textures.resize( static_cast<size_t>( render::IGraphicsTerrain::TextureTypes::COUNT ) );
        m_metalnessValues.resize( 4, m_defaultMetalnessValue );
    }

    TerrainSystem::~TerrainSystem() = default;

    void TerrainSystem::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto textureCount = static_cast<size_t>( render::IGraphicsTerrain::TextureTypes::COUNT );
            m_textures.resize( textureCount );
            ensureDefaultTerrainTextures( m_textures, false );

            m_metalnessValues.resize( 4 );

            for( size_t i = 0; i < m_metalnessValues.size(); ++i )
            {
                m_metalnessValues[i] = getDefaultMetalnessValue();
            }

            Component::load( data );

            auto terrainLayer = addSubComponentByType<SubComponent>();
            auto treesLayer = addSubComponentByType<SubComponent>();
            auto grassLayer = addSubComponentByType<SubComponent>();

            createTerrain();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TerrainSystem::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                // Authoring references, like height samples, survive runtime recreation.

                if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
                {
                    auto smgr = graphicsSystem->getGraphicsScene();
                    WP_ASSERT( smgr );

                    if( m_terrainListener )
                    {
                        if( m_terrain )
                        {
                            m_terrain->removeObjectListener( m_terrainListener );
                        }

                        m_terrainListener->unload( data );
                        m_terrainListener = nullptr;
                    }

                    if( m_terrain )
                    {
                        m_terrain->setVisible( false );

                        smgr->removeGraphicsObject( m_terrain );
                        m_terrain = nullptr;
                    }
                }

                Component::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TerrainSystem::updateFlags( u32 flags, u32 oldFlags )
    {
        if( auto actor = getActor() )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();

            if( graphicsSystem )
            {
                auto smgr = graphicsSystem->getGraphicsScene();
                WP_ASSERT( smgr );

                auto rootNode = smgr->getRootSceneNode();
                auto visible = isEnabled() && actor->isEnabledInScene();

                if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagInScene ) !=
                    BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagInScene ) )
                {
                    if( auto terrain = getTerrain() )
                    {
                        terrain->setVisible( visible );
                        terrain->updateMaterial();
                    }
                }
                else if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagEnabled ) !=
                         BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagEnabled ) )
                {
                    if( auto terrain = getTerrain() )
                    {
                        terrain->setVisible( visible );
                        terrain->updateMaterial();
                    }
                }
            }
        }
    }

    SmartPtr<Properties> TerrainSystem::getProperties() const
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();

        Array<String> terrainTypes = { "island", "mountains", "gradient" };

        if( auto properties = Component::getProperties() )
        {
            properties->setButtonPressed( UpdateMaterialStr );

            properties->setProperty( LayersStr, getNumLayers() );
            properties->setProperty( HeightMapStr, m_heightMap );
            properties->setProperty( HeightMapSizeStr, getHeightMapSize() );
            properties->setProperty( HeightScaleStr, m_heightScale );
            properties->setProperty( ShowWireframeStr, getShowWireframe() );
            properties->setProperty( GeneratedHeightMapWidthStr, getGeneratedHeightMapWidth() );
            properties->setProperty( GeneratedHeightMapHeightStr, getGeneratedHeightMapHeight() );
            properties->setProperty( GeneratedHeightMapValueScaleStr,
                                     getGeneratedHeightMapValueScale() );
            properties->setPropertyAsEnum( GeneratedHeightMapTypeStr, getGeneratedHeightMapType(),
                                           terrainTypes );

            properties->setButtonPressed( GenerateStr );

            auto textureProperties = factoryManager->make_ptr<Properties>();
            textureProperties->setName( TexturesStr );
            properties->addChild( textureProperties );

            textureProperties->setProperty(
                DiffuseTextureStr,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DIFFUSE )] );
            textureProperties->setProperty(
                DetailWeightTextureStr,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL_WEIGHT )] );
            textureProperties->setProperty(
                DetailTexture0Str,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL0 )] );
            textureProperties->setProperty(
                DetailTexture1Str,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL1 )] );
            textureProperties->setProperty(
                DetailTexture2Str,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL2 )] );
            textureProperties->setProperty(
                DetailTexture3Str,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL3 )] );
            textureProperties->setProperty(
                DetailTexture0NMStr,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL0_NM )] );
            textureProperties->setProperty(
                DetailTexture1NMStr,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL1_NM )] );
            textureProperties->setProperty(
                DetailTexture2NMStr,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL2_NM )] );
            textureProperties->setProperty(
                DetailTexture3NMStr,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL3_NM )] );
            textureProperties->setProperty(
                DetailRoughness0Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_ROUGHNESS0 )] );
            textureProperties->setProperty(
                DetailRoughness1Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_ROUGHNESS1 )] );
            textureProperties->setProperty(
                DetailRoughness2Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_ROUGHNESS2 )] );
            textureProperties->setProperty(
                DetailRoughness3Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_ROUGHNESS3 )] );
            textureProperties->setProperty(
                DetailMetalness0Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_METALNESS0 )] );
            textureProperties->setProperty(
                DetailMetalness1Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_METALNESS1 )] );
            textureProperties->setProperty(
                DetailMetalness2Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_METALNESS2 )] );
            textureProperties->setProperty(
                DetailMetalness3Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_METALNESS3 )] );
            textureProperties->setProperty(
                ReflectionTextureStr,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::REFLECTION )] );

            auto metalnessProperties = factoryManager->make_ptr<Properties>();
            metalnessProperties->setName( MetalnessStr );
            properties->addChild( metalnessProperties );

            metalnessProperties->setProperty( DefaultMetalnessValueStr, getDefaultMetalnessValue() );
            metalnessProperties->setProperty( DetailMetalnessValue0Str, m_metalnessValues[0] );
            metalnessProperties->setProperty( DetailMetalnessValue1Str, m_metalnessValues[1] );
            metalnessProperties->setProperty( DetailMetalnessValue2Str, m_metalnessValues[2] );
            metalnessProperties->setProperty( DetailMetalnessValue3Str, m_metalnessValues[3] );

            auto treesProperties = factoryManager->make_ptr<Properties>();
            treesProperties->setName( TreeSettingsStr );
            properties->addChild( treesProperties );

            treesProperties->setProperty( TreesEnableStr, getTreesEnabled() );
            treesProperties->setProperty( TreeDensityStr, getTreeDensity() );
            treesProperties->setProperty( PreviewTreeCountStr, getPreviewTreeCount() );
            treesProperties->setProperty( GeneratedTreeCountStr, getGeneratedTreeCount() );
            treesProperties->setProperty( TreePrefabStr, getTreePrefabName() );

            auto grassProperties = factoryManager->make_ptr<Properties>();
            grassProperties->setName( GrassSettingsStr );
            properties->addChild( grassProperties );

            grassProperties->setProperty( GrassEnableStr, getGrassEnabled() );
            grassProperties->setProperty( GrassDensityStr, getGrassDensity() );

            auto treeLayerProperties = factoryManager->make_ptr<Properties>();
            treeLayerProperties->setName( TreesStr );
            properties->addChild( treeLayerProperties );

            treeLayerProperties->setButtonPressed( GenerateTreesStr );
            treeLayerProperties->setButtonPressed( AddTreesStr );
            treeLayerProperties->setButtonPressed( RemoveTreesStr );

            auto terrainTreeLayers = getSubComponentsByType<TerrainTreeLayer>();
            for( auto terrainTreeLayer : terrainTreeLayers )
            {
                auto treeLayer = factoryManager->make_ptr<Properties>();
                treeLayer->setName( TreeLayerStr );
                treeLayerProperties->addChild( treeLayer );

                treeLayer->setProperty( TreeDensityStr, terrainTreeLayer->getDensity() );
                treeLayer->setProperty( TreePrefabStr, terrainTreeLayer->getPrefabPath() );
                treeLayer->setProperty( TreeTextureStr, terrainTreeLayer->getBaseTexture() );
            }

            return properties;
        }

        return nullptr;
    }

    void TerrainSystem::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            WP_LOG_WARNING( "TerrainSystem::setProperties: null properties supplied." );
            return;
        }

        String serialized;
        const bool hasSerializedData = properties->hasProperty( "terrainDataV1" );
        properties->getPropertyValue( "terrainDataV1", serialized );
        if( hasSerializedData )
        {
            String error;
            if( !importTerrainData( serialized, error ) )
            {
                WP_LOG_WARNING( "TerrainSystem: rejected saved terrain: " + error );
                return;
            }
        }
        else
        {
            // Legacy scenes contain only dimensions and scale. Preserve their
            // render-space anchor and initialize flat samples until generated.
            auto size = getHeightMapSize();
            auto scale = getHeightScale();
            properties->getPropertyValue( HeightMapSizeStr, size );
            properties->getPropertyValue( HeightScaleStr, scale );
            setHeightMapSize( size );
            setHeightScale( scale );
        }

        Component::setProperties( properties );

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            WP_LOG_WARNING( "TerrainSystem::setProperties: null application manager." );
            return;
        }

        auto sceneManager = applicationManager->getGameManagerPtr();
        auto scene = sceneManager ? sceneManager->getCurrentScenePtr() : nullptr;

        auto actor = getActorPtr();
        if( !actor )
        {
            return;
        }

        const auto generateHeightMapRequested = properties->isButtonPressed( GenerateStr );

        properties->getPropertyValue( HeightMapStr, m_heightMap );

        auto heightMapSize = getHeightMapSize();
        auto heightScale = getHeightScale();
        auto showWireframe = getShowWireframe();
        if( !hasSerializedData )
        {
            properties->getPropertyValue( HeightMapSizeStr, heightMapSize );
            properties->getPropertyValue( HeightScaleStr, heightScale );
        }
        properties->getPropertyValue( ShowWireframeStr, showWireframe );
        setHeightMapSize( heightMapSize );
        setHeightScale( heightScale );
        setShowWireframe( showWireframe );

        auto generatedHeightMapWidth = getGeneratedHeightMapWidth();
        auto generatedHeightMapHeight = getGeneratedHeightMapHeight();
        auto generatedHeightMapValueScale = getGeneratedHeightMapValueScale();
        auto generatedHeightMapType = getGeneratedHeightMapType();
        properties->getPropertyValue( GeneratedHeightMapWidthStr, generatedHeightMapWidth );
        properties->getPropertyValue( GeneratedHeightMapHeightStr, generatedHeightMapHeight );
        properties->getPropertyValue( GeneratedHeightMapValueScaleStr, generatedHeightMapValueScale );
        properties->getPropertyValue( GeneratedHeightMapTypeStr, generatedHeightMapType );
        setGeneratedHeightMapWidth( generatedHeightMapWidth );
        setGeneratedHeightMapHeight( generatedHeightMapHeight );
        setGeneratedHeightMapValueScale( generatedHeightMapValueScale );
        setGeneratedHeightMapType( generatedHeightMapType );

        if( auto textureProperties = properties->getChild( TexturesStr ) )
        {
            textureProperties->getPropertyValue(
                DiffuseTextureStr,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DIFFUSE )] );
            textureProperties->getPropertyValue(
                DetailWeightTextureStr,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL_WEIGHT )] );
            textureProperties->getPropertyValue(
                DetailTexture0Str,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL0 )] );
            textureProperties->getPropertyValue(
                DetailTexture1Str,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL1 )] );
            textureProperties->getPropertyValue(
                DetailTexture2Str,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL2 )] );
            textureProperties->getPropertyValue(
                DetailTexture3Str,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL3 )] );
            textureProperties->getPropertyValue(
                DetailTexture0NMStr,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL0_NM )] );
            textureProperties->getPropertyValue(
                DetailTexture1NMStr,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL1_NM )] );
            textureProperties->getPropertyValue(
                DetailTexture2NMStr,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL2_NM )] );
            textureProperties->getPropertyValue(
                DetailTexture3NMStr,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL3_NM )] );
            textureProperties->getPropertyValue(
                DetailRoughness0Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_ROUGHNESS0 )] );
            textureProperties->getPropertyValue(
                DetailRoughness1Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_ROUGHNESS1 )] );
            textureProperties->getPropertyValue(
                DetailRoughness2Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_ROUGHNESS2 )] );
            textureProperties->getPropertyValue(
                DetailRoughness3Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_ROUGHNESS3 )] );
            textureProperties->getPropertyValue(
                DetailMetalness0Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_METALNESS0 )] );
            textureProperties->getPropertyValue(
                DetailMetalness1Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_METALNESS1 )] );
            textureProperties->getPropertyValue(
                DetailMetalness2Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_METALNESS2 )] );
            textureProperties->getPropertyValue(
                DetailMetalness3Str, m_textures[static_cast<u32>(
                                         render::IGraphicsTerrain::TextureTypes::DETAIL_METALNESS3 )] );
            textureProperties->getPropertyValue(
                ReflectionTextureStr,
                m_textures[static_cast<u32>( render::IGraphicsTerrain::TextureTypes::REFLECTION )] );
        }

        ensureDefaultTerrainTextures( m_textures, false );

        if( auto terrain = getTerrain() )
        {
            applyTerrainTextures( terrain, m_textures );
        }

        if( auto metalnessProperties = properties->getChild( MetalnessStr ) )
        {
            auto defaultMetalnessValue = getDefaultMetalnessValue();
            metalnessProperties->getPropertyValue( DefaultMetalnessValueStr, defaultMetalnessValue );
            setDefaultMetalnessValue( defaultMetalnessValue );

            metalnessProperties->getPropertyValue( DetailMetalnessValue0Str, m_metalnessValues[0] );
            metalnessProperties->getPropertyValue( DetailMetalnessValue1Str, m_metalnessValues[1] );
            metalnessProperties->getPropertyValue( DetailMetalnessValue2Str, m_metalnessValues[2] );
            metalnessProperties->getPropertyValue( DetailMetalnessValue3Str, m_metalnessValues[3] );
        }

        if( auto treesProperties = properties->getChild( TreeSettingsStr ) )
        {
            auto treesEnabled = getTreesEnabled();
            auto treeDensity = getTreeDensity();
            auto previewTreeCount = getPreviewTreeCount();
            auto generatedTreeCount = getGeneratedTreeCount();
            auto treePrefabName = getTreePrefabName();

            treesProperties->getPropertyValue( TreesEnableStr, treesEnabled );
            treesProperties->getPropertyValue( TreeDensityStr, treeDensity );
            treesProperties->getPropertyValue( PreviewTreeCountStr, previewTreeCount );
            treesProperties->getPropertyValue( GeneratedTreeCountStr, generatedTreeCount );
            treesProperties->getPropertyValue( TreePrefabStr, treePrefabName );

            setTreesEnabled( treesEnabled );
            setTreeDensity( treeDensity );
            setPreviewTreeCount( previewTreeCount );
            setGeneratedTreeCount( generatedTreeCount );
            setTreePrefabName( treePrefabName );
        }

        if( auto grassProperties = properties->getChild( GrassSettingsStr ) )
        {
            auto grassEnabled = getGrassEnabled();
            auto grassDensity = getGrassDensity();

            grassProperties->getPropertyValue( GrassEnableStr, grassEnabled );
            grassProperties->getPropertyValue( GrassDensityStr, grassDensity );

            setGrassEnabled( grassEnabled );
            setGrassDensity( grassDensity );
        }

        if( generateHeightMapRequested )
        {
            generateHeightMap();

            auto treePrefab = getTreePrefab( 0 );

            for( size_t i = 0; treePrefab && i < getPreviewTreeCount(); ++i )
            {
                auto treeInstance = treePrefab->createActor();

                auto treePosition = Vector3<real_Num>( 0, 0, 0 );
                treeInstance->setPosition( treePosition );

                actor->addChild( treeInstance );
            }

            applicationManager->triggerEvent( EventType::Object, IEvent::sceneChanged,
                                              Array<Parameter>(), this, this, nullptr );
        }

        if( auto treeLayerProperties = properties->getChild( TreesStr ) )
        {
            if( treeLayerProperties->isButtonPressed( GenerateTreesStr ) )
            {
                auto treePrefab = getTreePrefab( 0 );

                for( size_t i = 0; treePrefab && i < getGeneratedTreeCount(); ++i )
                {
                    auto treeInstance = treePrefab->createActor();

                    auto treePosition = Vector3<real_Num>( 0, 0, 0 );
                    treeInstance->setPosition( treePosition );

                    if( scene )
                    {
                        scene->addActor( treeInstance );
                    }
                }
            }
            else if( properties->isButtonPressed( AddTreesStr ) ||
                     treeLayerProperties->isButtonPressed( AddTreesStr ) )
            {
                addTreeLayer();
            }
            else if( properties->isButtonPressed( RemoveTreesStr ) ||
                     treeLayerProperties->isButtonPressed( RemoveTreesStr ) )
            {
                removeSubComponentByType<TerrainTreeLayer>();
            }
        }

        updateVisibility();

        if( auto terrain = getTerrain() )
        {
            WP_ASSERT( terrain->getLoadingState() != LoadingState::Error );

            auto heightMapScale = getHeightScale();
            terrain->setHeightScale( heightMapScale );
            terrain->setHeightMapSize( getHeightMapSize() );
            terrain->setShowWireframe( getShowWireframe() );

            terrain->setHeightMap( m_heightMap );
            terrain->updateMaterial();
        }

        if( properties->isButtonPressed( UpdateMaterialStr ) )
        {
            if( auto terrain = getTerrain() )
            {
                terrain->setHeightMapSize( getHeightMapSize() );
                terrain->setHeightScale( getHeightScale() );
                terrain->setShowWireframe( getShowWireframe() );
                terrain->setHeightMap( m_heightMap );
                ensureDefaultTerrainTextures( m_textures, false );
                applyTerrainTextures( terrain, m_textures );
            }
        }
    }

    void TerrainSystem::updateTransform()
    {
        if( auto actor = getActor() )
        {
            if( auto actorTransform = actor->getTransform() )
            {
                auto worldTransform = actorTransform->getWorldTransform();

                if( m_terrain )
                {
                    m_terrain->setWorldTransform( worldTransform );
                }
            }
        }
    }

    s32 TerrainSystem::calculateNumLayers() const
    {
        auto terrainLayers = getSubComponentsByType<TerrainLayer>();
        return static_cast<s32>( terrainLayers.size() );
    }

    s32 TerrainSystem::getNumLayers() const
    {
        return m_numLayers;
    }

    SmartPtr<TerrainLayer> TerrainSystem::addLayer()
    {
        auto numLayers = calculateNumLayers();
        auto newNumLayers = numLayers + 1;

        setNumLayers( newNumLayers );
        resizeLayermap();

        auto layers = getSubComponentsByType<TerrainLayer>();
        return layers[numLayers];
    }

    void TerrainSystem::removeLayer( s32 index )
    {
        if( index < 0 || index >= m_numLayers )
        {
            return;
        }

        m_layers.erase( m_layers.begin() + index );
    }

    void TerrainSystem::removeLayer( SmartPtr<TerrainLayer> layer )
    {
        auto layers = getSubComponentsByType<TerrainLayer>();
        auto it = std::find( layers.begin(), layers.end(), layer );
        if( it != layers.end() )
        {
            layers.erase( it );
        }
    }

    void TerrainSystem::setNumLayers( s32 numLayers )
    {
        if( m_numLayers != numLayers )
        {
            m_numLayers = numLayers;
            resizeLayermap();
        }
    }

    void TerrainSystem::createTerrain()
    {
        if( m_terrain )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        // The terrain graphics object can only be created when a graphics
        // system and an active graphics scene are available. In headless
        // environments (such as the unit tests) there is no graphics system,
        // so the component must remain valid without one rather than
        // crashing.
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            return;
        }

        auto smgr = graphicsSystem->getGraphicsScene();
        if( !smgr )
        {
            return;
        }

        auto terrainListener = workphone::make_ptr<TerrainObjectListener>();
        terrainListener->setOwner( this );
        m_terrainListener = terrainListener;

        auto candidate = smgr->addGraphicsObjectByType<render::IGraphicsTerrain>();
        setTerrain( candidate );
        if( candidate && m_terrain != candidate )
            smgr->removeGraphicsObject( candidate );
        if( !m_terrain )
        {
            m_terrainListener = nullptr;
            return;
        }

        m_terrain->addObjectListener( m_terrainListener );
        m_terrain->setHeightMapSize( getHeightMapSize() );
        m_terrain->setHeightScale( getHeightScale() );
        m_terrain->setShowWireframe( getShowWireframe() );

        ensureDefaultTerrainTextures( m_textures, false );
        applyTerrainTextures( m_terrain, m_textures );
    }

    void TerrainSystem::rebuild()
    {
        createTerrain();

        if( auto terrain = getTerrain() )
        {
            terrain->setHeightMapSize( getHeightMapSize() );
            terrain->setHeightScale( getHeightScale() );
            terrain->setShowWireframe( getShowWireframe() );
            terrain->setHeightMap( m_heightMap );
            terrain->updateMaterial();
        }

        updateTransform();
        updateVisibility();
        resizeLayermap();
        updateLayers();
    }

    void TerrainSystem::updateVisibility()
    {
        if( auto actor = getActor() )
        {
            auto visible = isEnabled() && actor->isEnabledInScene();

            if( m_terrain )
            {
                m_terrain->setVisible( visible );
            }
        }
    }

    FSMReturnType TerrainSystem::handleComponentEvent( u32 state, FSMEvent eventType )
    {
        switch( eventType )
        {
        case FSMEvent::Change:
            break;
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                createTerrain();
                updateVisibility();

                if( auto terrain = getTerrain() )
                {
                    auto heightMapScale = getHeightScale();
                    terrain->setHeightScale( heightMapScale );
                    terrain->setHeightMapSize( getHeightMapSize() );
                    terrain->setShowWireframe( getShowWireframe() );

                    terrain->setHeightMap( m_heightMap );
                    terrain->updateMaterial();
                }
            }
            break;
            default:
            {
            }
            }
        }
        break;
        case FSMEvent::Leave:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Play:
            {
            }
            break;
            default:
            {
            }
            }
        }
        break;
        case FSMEvent::Pending:
            break;
        case FSMEvent::Complete:
            break;
        case FSMEvent::NewState:
            break;
        case FSMEvent::WaitForChange:
            break;
        default:
        {
        }
        break;
        }

        return FSMReturnType::Ok;
    }

    SmartPtr<IGamePrefab> TerrainSystem::getTreePrefab( s32 idx ) const
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto prefabManager = applicationManager->getPrefabManager();

        auto treePrefab = prefabManager->loadPrefab( getTreePrefabName() );

        return treePrefab;
    }

    void TerrainSystem::generateHeightMap()
    {
        try
        {
            auto terrain = getTerrain();

            const auto width = clampHeightMapDimension( getGeneratedHeightMapWidth() );
            const auto height = clampHeightMapDimension( getGeneratedHeightMapHeight() );
            const auto valueScale = getGeneratedHeightMapValueScale();
            const auto heightMapType = getGeneratedHeightMapType();

            size_t heightDataSize = 0;
            if( !getHeightDataElementCount( width, height, heightDataSize ) ||
                heightDataSize > render::terrainMaximumSamples || !std::isfinite( valueScale ) )
            {
                WP_LOG_WARNING( "TerrainSystem::generateHeightMap: invalid generated height map size." );
                return;
            }

            Array<f32> heightData( heightDataSize );
            for( u32 y = 0; y < height; ++y )
            {
                for( u32 x = 0; x < width; ++x )
                {
                    const auto u = static_cast<f32>( x ) / static_cast<f32>( width - 1u );
                    const auto v = static_cast<f32>( y ) / static_cast<f32>( height - 1u );
                    const auto centeredX = u * 2.0f - 1.0f;
                    const auto centeredY = v * 2.0f - 1.0f;
                    const auto distanceFromCenter =
                        std::sqrt( centeredX * centeredX + centeredY * centeredY );

                    auto normalizedHeight = v;
                    if( heightMapType == "island" )
                    {
                        const auto islandMask = 1.0f - smoothstep( 0.35f, 0.98f, distanceFromCenter );
                        const auto beachShelf =
                            0.18f * ( 1.0f - smoothstep( 0.72f, 0.92f, distanceFromCenter ) );
                        const auto hills = fractalNoise( u * 5.0f + 17.0f, v * 5.0f + 11.0f );
                        const auto peak =
                            0.55f * ( 1.0f - smoothstep( 0.0f, 0.55f, distanceFromCenter ) );

                        normalizedHeight = clamp01( ( beachShelf + hills * 0.55f + peak ) * islandMask );
                    }
                    else if( heightMapType == "mountains" )
                    {
                        const auto ridges = ridgeNoise( u * 7.0f + 31.0f, v * 7.0f + 23.0f );
                        const auto broadForms = fractalNoise( u * 2.0f + 5.0f, v * 2.0f + 19.0f );
                        const auto valley =
                            0.5f + 0.5f * std::sin( ( u * 2.0f + v * 1.2f ) * 3.14159265f );

                        normalizedHeight =
                            clamp01( broadForms * 0.35f + ridges * 0.65f + valley * 0.15f );
                    }

                    heightData[y * width + x] = normalizedHeight * valueScale;
                }
            }

            render::TerrainData data;
            data.dimensions = Vector2I( static_cast<s32>( width ), static_cast<s32>( height ) );
            data.origin = Vector2F( -static_cast<f32>( width - 1 ) * 0.5f,
                                    -static_cast<f32>( height - 1 ) * 0.5f );
            data.heightScale = getHeightScale();
            data.heights = std::move( heightData );
            String error;
            if( !applyTerrainData( data, error ) )
            {
                WP_LOG_WARNING( "TerrainSystem::generateHeightMap: " + error );
                return;
            }
            if( terrain )
            {
                ensureDefaultTerrainTextures( m_textures, true );
                applyTerrainTextures( terrain, m_textures );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TerrainSystem::resizeLayermap()
    {
        //m_layermap.resize( m_num_layers );
        m_layers.resize( getNumLayers() );

        auto numLayers = getNumLayers();
        auto subComponents = getSubComponentsByType<TerrainLayer>();
        if( numLayers >= subComponents.size() )
        {
            auto diff = numLayers - subComponents.size();
            for( size_t i = 0; i < diff; ++i )
            {
                auto terrainLayer = addSubComponentByType<TerrainLayer>();
            }
        }
        else if( numLayers < subComponents.size() )
        {
            auto diff = subComponents.size() - numLayers;
            for( size_t i = 0; i < diff; ++i )
            {
                auto index = getNumSubComponents() - 1;
                removeSubComponentByIndex( index );
            }
        }

        auto index = 0;
        auto layers = getSubComponentsByType<TerrainLayer>();
        for( auto &layer : layers )
        {
            layer->setIndex( index );
            ++index;
        }
    }

    void TerrainSystem::updateLayers()
    {
        if( m_terrain )
        {
            auto layers = getSubComponentsByType<TerrainLayer>();
            constexpr s32 maxTerraDetailLayers = 4;
            for( auto layer : layers )
            {
                auto index = layer->getIndex();
                WP_ASSERT( index >= 0 );
                WP_ASSERT( index < maxTerraDetailLayers );
                if( index < 0 || index >= maxTerraDetailLayers )
                {
                    continue;
                }

                const auto textureSlot =
                    static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL0 ) +
                    static_cast<u32>( index );
                m_terrain->setTexture( textureSlot, layer->getBaseTexture() );
            }

            m_terrain->updateMaterial();
        }
    }

    SmartPtr<TerrainTreeLayer> TerrainSystem::addTreeLayer()
    {
        auto terrainTreeLayer = addSubComponentByType<TerrainTreeLayer>();
        return terrainTreeLayer;
    }

    SmartPtr<TerrainGrassLayer> TerrainSystem::addGrassLayer()
    {
        auto terrainGrassLayer = addSubComponentByType<TerrainGrassLayer>();
        return terrainGrassLayer;
    }

    SmartPtr<render::ITexture> TerrainSystem::getHeightMap() const
    {
        return m_heightMap;
    }

    void TerrainSystem::setHeightMap( SmartPtr<render::ITexture> heightMap )
    {
        m_heightMap = heightMap;

        if( m_terrain )
        {
            m_terrain->setHeightMap( m_heightMap );
        }
    }

    void TerrainSystem::setTerrain( SmartPtr<render::IGraphicsTerrain> terrain )
    {
        if( terrain )
        {
            auto concrete = dynamic_cast<render::Terrain *>( terrain.get() );
            String error;
            if( !concrete )
            {
                WP_LOG_WARNING( "TerrainSystem: renderer cannot accept authoritative height data." );
                return;
            }
            const auto snapshot = getTerrainSnapshot();
            auto placement = concrete->getWorldTransform();
            if( auto actor = getActor() )
                if( auto transform = actor->getTransform() )
                    placement = transform->getWorldTransform();
            // A recreated renderer must receive the actor's existing placement even
            // when no new transform event follows attachment. Validate both source
            // and placement before touching the candidate or retiring a valid runtime.
            if( !render::validateTerrainPlacement( *snapshot, placement, error ) ||
                !concrete->applyTerrainData( *snapshot, error ) )
            {
                WP_LOG_WARNING( "TerrainSystem: renderer cannot accept authoritative height data: " +
                                error );
                return;
            }
            concrete->setWorldTransform( placement );
        }
        m_terrain = terrain;
    }

    SmartPtr<render::IGraphicsTerrain> TerrainSystem::getTerrain() const
    {
        return m_terrain;
    }

    render::TerrainSnapshot TerrainSystem::getTerrainSnapshot() const
    {
        std::lock_guard<std::mutex> guard( m_terrainDataMutex );
        return m_terrainData;
    }

    SmartPtr<ISharedObject> TerrainSystem::toData() const
    {
        auto data = Component::toData();
        if( auto properties = dynamic_cast<Properties *>( data.get() ) )
        {
            const auto samples = exportTerrainData();
            if( samples.empty() )
                throw std::runtime_error( "Unable to serialize terrain height samples." );
            properties->setProperty( "terrainDataV1", samples );
        }
        return data;
    }

    u64 TerrainSystem::getTerrainRevision() const
    {
        return getTerrainSnapshot()->revision;
    }

    bool TerrainSystem::applyTerrainData( const render::TerrainData &data, String &error,
                                          u64 expectedRevision )
    {
        if( !render::validateTerrainData( data, error ) )
            return false;
        try
        {
            auto candidate = std::make_shared<render::TerrainData>( data );
            std::lock_guard<std::mutex> guard( m_terrainDataMutex );
            if( expectedRevision && expectedRevision != m_terrainData->revision )
            {
                error = "Terrain changed while the edit was being prepared.";
                return false;
            }
            if( candidate->dimensions == m_terrainData->dimensions &&
                candidate->spacing == m_terrainData->spacing &&
                candidate->origin == m_terrainData->origin &&
                candidate->heightScale == m_terrainData->heightScale &&
                candidate->heights == m_terrainData->heights )
            {
                error.clear();
                return true;
            }
            if( m_terrainData->revision == std::numeric_limits<u64>::max() )
            {
                error = "Terrain revision exhausted.";
                return false;
            }
            candidate->revision = m_terrainData->revision + 1;
            if( m_terrain )
            {
                auto concrete = dynamic_cast<render::Terrain *>( m_terrain.get() );
                if( !concrete )
                {
                    error = "This renderer does not support authoritative terrain data.";
                    return false;
                }
                if( !concrete->applyTerrainData( *candidate, error ) )
                    return false;
            }
            m_terrainData = candidate;
            m_heightMapSize = candidate->dimensions;
            m_heightScale = candidate->heightScale;
            error.clear();
            return true;
        }
        catch( const std::exception &exception )
        {
            error = String( "Unable to prepare terrain: " ) + exception.what();
            return false;
        }
    }

    String TerrainSystem::exportTerrainData() const
    {
        const auto data = getTerrainSnapshot();
        using Json = std::unique_ptr<cJSON, decltype( &cJSON_Delete )>;
        Json root( cJSON_CreateObject(), cJSON_Delete );
        const float spacing[] = { data->spacing.x, data->spacing.y };
        const float origin[] = { data->origin.x, data->origin.y };
        if( !root || !cJSON_AddStringToObject( root.get(), "format", "workphone.terrain" ) ||
            !cJSON_AddNumberToObject( root.get(), "version", render::terrainDataFormatVersion ) ||
            !cJSON_AddNumberToObject( root.get(), "width", data->dimensions.x ) ||
            !cJSON_AddNumberToObject( root.get(), "depth", data->dimensions.y ) ||
            !cJSON_AddNumberToObject( root.get(), "heightScale", data->heightScale ) )
            return {};
        const auto addArray = [&]( const char *name, const float *values, int count ) {
            Json array( cJSON_CreateFloatArray( values, count ), cJSON_Delete );
            if( !array || !cJSON_AddItemToObject( root.get(), name, array.get() ) )
                return false;
            array.release();
            return true;
        };
        if( !addArray( "spacing", spacing, 2 ) || !addArray( "origin", origin, 2 ) ||
            !addArray( "heights", data->heights.data(), static_cast<int>( data->heights.size() ) ) )
            return {};
        std::unique_ptr<char, decltype( &cJSON_free )> json( cJSON_PrintUnformatted( root.get() ),
                                                             cJSON_free );
        return json ? String( json.get() ) : String();
    }

    bool TerrainSystem::importTerrainData( const String &json, String &error )
    {
        // Bound input before the parser allocates its object tree. No partial changes.
        if( json.empty() || json.size() > 128ull * 1024ull * 1024ull )
        {
            error = "Terrain JSON is empty or exceeds the 128 MiB limit.";
            return false;
        }
        const auto expectedRevision = getTerrainRevision();
        try
        {
            using Json = std::unique_ptr<cJSON, decltype( &cJSON_Delete )>;
            const char *end = nullptr;
            Json root( cJSON_ParseWithLengthOpts( json.c_str(), json.size() + 1, &end, 1 ),
                       cJSON_Delete );
            if( !root || !cJSON_IsObject( root.get() ) || end != json.c_str() + json.size() )
            {
                error = "Invalid terrain JSON.";
                return false;
            }
            const auto field = [&]( const char *name ) {
                return cJSON_GetObjectItemCaseSensitive( root.get(), name );
            };
            const auto number = []( const cJSON *value ) {
                return cJSON_IsNumber( value ) && std::isfinite( value->valuedouble ) &&
                       std::abs( value->valuedouble ) <= std::numeric_limits<float>::max();
            };
            const auto format = field( "format" );
            const auto version = field( "version" );
            const auto width = field( "width" );
            const auto depth = field( "depth" );
            const auto scale = field( "heightScale" );
            const auto spacing = field( "spacing" );
            const auto origin = field( "origin" );
            const auto heights = field( "heights" );
            if( !cJSON_IsString( format ) || String( format->valuestring ) != "workphone.terrain" ||
                !number( version ) || version->valuedouble != render::terrainDataFormatVersion ||
                !number( width ) || !number( depth ) || width->valuedouble < 2 ||
                depth->valuedouble < 2 || width->valuedouble > render::terrainMaximumDimension ||
                depth->valuedouble > render::terrainMaximumDimension ||
                std::floor( width->valuedouble ) != width->valuedouble ||
                std::floor( depth->valuedouble ) != depth->valuedouble ||
                width->valuedouble * depth->valuedouble > render::terrainMaximumSamples ||
                !number( scale ) || !cJSON_IsArray( spacing ) || cJSON_GetArraySize( spacing ) != 2 ||
                !cJSON_IsArray( origin ) || cJSON_GetArraySize( origin ) != 2 ||
                !cJSON_IsArray( heights ) )
            {
                error = "Unsupported terrain schema, dimensions or metadata.";
                return false;
            }
            render::TerrainData data;
            data.dimensions = Vector2I( static_cast<s32>( width->valuedouble ),
                                        static_cast<s32>( depth->valuedouble ) );
            const auto count = static_cast<size_t>( data.dimensions.x ) * data.dimensions.y;
            if( static_cast<size_t>( cJSON_GetArraySize( heights ) ) != count ||
                !number( cJSON_GetArrayItem( spacing, 0 ) ) ||
                !number( cJSON_GetArrayItem( spacing, 1 ) ) ||
                !number( cJSON_GetArrayItem( origin, 0 ) ) ||
                !number( cJSON_GetArrayItem( origin, 1 ) ) )
            {
                error = "Terrain arrays have invalid sizes or numbers.";
                return false;
            }
            data.spacing = Vector2F( static_cast<f32>( cJSON_GetArrayItem( spacing, 0 )->valuedouble ),
                                     static_cast<f32>( cJSON_GetArrayItem( spacing, 1 )->valuedouble ) );
            data.origin = Vector2F( static_cast<f32>( cJSON_GetArrayItem( origin, 0 )->valuedouble ),
                                    static_cast<f32>( cJSON_GetArrayItem( origin, 1 )->valuedouble ) );
            data.heightScale = static_cast<f32>( scale->valuedouble );
            data.heights.clear();
            data.heights.reserve( count );
            for( auto value = heights->child; value; value = value->next )
            {
                if( !number( value ) )
                {
                    error = "Terrain contains a nonfinite or invalid sample.";
                    return false;
                }
                data.heights.push_back( static_cast<f32>( value->valuedouble ) );
            }
            return applyTerrainData( data, error, expectedRevision );
        }
        catch( const std::exception &exception )
        {
            error = String( "Unable to read terrain: " ) + exception.what();
            return false;
        }
    }

    void TerrainSystem::setHeightScale( f32 heightScale )
    {
        auto before = getTerrainSnapshot();
        if( before->heightScale == heightScale )
            return;
        auto candidate = *before;
        candidate.heightScale = heightScale;
        String error;
        if( !applyTerrainData( candidate, error, before->revision ) )
        {
            WP_LOG_WARNING( "TerrainSystem: " + error );
        }
    }

    f32 TerrainSystem::getHeightScale() const
    {
        return m_heightScale;
    }

    Vector2I TerrainSystem::getHeightMapSize() const
    {
        return m_heightMapSize;
    }

    void TerrainSystem::setHeightMapSize( const Vector2I &heightMapSize )
    {
        auto before = getTerrainSnapshot();
        if( before->dimensions == heightMapSize )
            return;
        if( heightMapSize.x < 2 || heightMapSize.y < 2 ||
            heightMapSize.x > static_cast<s32>( render::terrainMaximumDimension ) ||
            heightMapSize.y > static_cast<s32>( render::terrainMaximumDimension ) ||
            static_cast<u64>( heightMapSize.x ) * heightMapSize.y > render::terrainMaximumSamples )
        {
            WP_LOG_WARNING( "TerrainSystem: invalid height-map dimensions." );
            return;
        }
        auto candidate = *before;
        candidate.dimensions = heightMapSize;
        candidate.origin = Vector2F( -heightMapSize.x * candidate.spacing.x * 0.5f,
                                     -heightMapSize.y * candidate.spacing.y * 0.5f );
        candidate.heights.assign( static_cast<size_t>( heightMapSize.x ) * heightMapSize.y, 0.0f );
        String error;
        if( !applyTerrainData( candidate, error, before->revision ) )
        {
            WP_LOG_WARNING( "TerrainSystem: " + error );
        }
    }

    bool TerrainSystem::getShowWireframe() const
    {
        return m_showWireframe;
    }

    void TerrainSystem::setShowWireframe( bool showWireframe )
    {
        m_showWireframe = showWireframe;

        if( m_terrain )
        {
            m_terrain->setShowWireframe( m_showWireframe );
            m_terrain->updateMaterial();
        }
    }

    u32 TerrainSystem::getGeneratedHeightMapWidth() const
    {
        return m_generatedHeightMapWidth;
    }

    void TerrainSystem::setGeneratedHeightMapWidth( u32 width )
    {
        m_generatedHeightMapWidth = clampHeightMapDimension( width );
    }

    u32 TerrainSystem::getGeneratedHeightMapHeight() const
    {
        return m_generatedHeightMapHeight;
    }

    void TerrainSystem::setGeneratedHeightMapHeight( u32 height )
    {
        m_generatedHeightMapHeight = clampHeightMapDimension( height );
    }

    f32 TerrainSystem::getGeneratedHeightMapValueScale() const
    {
        return m_generatedHeightMapValueScale;
    }

    void TerrainSystem::setGeneratedHeightMapValueScale( f32 valueScale )
    {
        if( !MathF::isFinite( valueScale ) )
        {
            m_generatedHeightMapValueScale = 0.0f;
            return;
        }

        m_generatedHeightMapValueScale = Math<f32>::max( 0.0f, valueScale );
    }

    String TerrainSystem::getGeneratedHeightMapType() const
    {
        return m_generatedHeightMapType;
    }

    void TerrainSystem::setGeneratedHeightMapType( const String &heightMapType )
    {
        auto value = StringUtil::trim( heightMapType );
        if( value != "island" && value != "mountains" && value != "gradient" )
        {
            value = "gradient";
        }

        m_generatedHeightMapType = value;
    }

    f32 TerrainSystem::getDefaultMetalnessValue() const
    {
        return m_defaultMetalnessValue;
    }

    void TerrainSystem::setDefaultMetalnessValue( f32 defaultMetalnessValue )
    {
        m_defaultMetalnessValue = Math<f32>::max( 0.0f, defaultMetalnessValue );
    }

    bool TerrainSystem::getTreesEnabled() const
    {
        return m_treesEnabled;
    }

    void TerrainSystem::setTreesEnabled( bool enabled )
    {
        m_treesEnabled = enabled;
    }

    f32 TerrainSystem::getTreeDensity() const
    {
        return m_treeDensity;
    }

    void TerrainSystem::setTreeDensity( f32 density )
    {
        m_treeDensity = Math<f32>::max( 0.0f, density );
    }

    bool TerrainSystem::getGrassEnabled() const
    {
        return m_grassEnabled;
    }

    void TerrainSystem::setGrassEnabled( bool enabled )
    {
        m_grassEnabled = enabled;
    }

    f32 TerrainSystem::getGrassDensity() const
    {
        return m_grassDensity;
    }

    void TerrainSystem::setGrassDensity( f32 density )
    {
        m_grassDensity = Math<f32>::max( 0.0f, density );
    }

    u32 TerrainSystem::getPreviewTreeCount() const
    {
        return m_previewTreeCount;
    }

    void TerrainSystem::setPreviewTreeCount( u32 count )
    {
        m_previewTreeCount = count;
    }

    u32 TerrainSystem::getGeneratedTreeCount() const
    {
        return m_generatedTreeCount;
    }

    void TerrainSystem::setGeneratedTreeCount( u32 count )
    {
        m_generatedTreeCount = count;
    }

    String TerrainSystem::getTreePrefabName() const
    {
        return m_treePrefabName;
    }

    void TerrainSystem::setTreePrefabName( const String &treePrefabName )
    {
        m_treePrefabName = StringUtil::trim( treePrefabName );
    }

    TerrainSystem::TerrainObjectListener::TerrainObjectListener() = default;

    TerrainSystem::TerrainObjectListener::~TerrainObjectListener() = default;

    Parameter TerrainSystem::TerrainObjectListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        auto newState = static_cast<LoadingState>( arguments[1].getS32() );
        if( newState == LoadingState::Loaded )
        {
            if( auto owner = getOwner() )
            {
                owner->updateLayers();
            }
        }

        return {};
    }

    void TerrainSystem::TerrainObjectListener::loadingStateChanged( ISharedObject *sharedObject,
                                                                    LoadingState oldState,
                                                                    LoadingState newState )
    {
        if( newState == LoadingState::Loaded )
        {
            if( auto owner = getOwner() )
            {
                owner->updateLayers();
            }
        }
    }

    bool TerrainSystem::TerrainObjectListener::destroy( void *ptr )
    {
        return false;
    }

    SmartPtr<TerrainSystem> TerrainSystem::TerrainObjectListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void TerrainSystem::TerrainObjectListener::setOwner( SmartPtr<TerrainSystem> owner )
    {
        m_owner = owner;
    }

    void TerrainSystem::TerrainObjectListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

}  // namespace workphone::scene
