#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Graphics/IGraphicsTerrain.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    class RecordingTexture : public render::ITexture
    {
    public:
        SmartPtr<core::IPrototype> getParentPrototype() const override
        {
            return nullptr;
        }
        void setParentPrototype( SmartPtr<core::IPrototype> prototype ) override
        {
        }
        SmartPtr<Properties> getProperties() const override
        {
            return nullptr;
        }
        void setProperties( SmartPtr<Properties> properties ) override
        {
        }

        void saveToFile( const String &filePath ) override
        {
        }
        void loadFromFile( const String &filePath ) override
        {
            this->filePath = filePath;
        }
        void save() override
        {
        }
        void import() override
        {
        }
        void reimport() override
        {
        }

        workphone::UUID getFileSystemId() const override
        {
            return fileSystemId;
        }
        void setFileSystemId( workphone::UUID id ) override
        {
            fileSystemId = id;
        }
        String getFilePath() const override
        {
            return filePath;
        }
        void setFilePath( const String &filePath ) override
        {
            this->filePath = filePath;
        }
        workphone::UUID getSettingsFileSystemId() const override
        {
            return settingsFileSystemId;
        }
        void setSettingsFileSystemId( workphone::UUID id ) override
        {
            settingsFileSystemId = id;
        }
        void _getObject( void **ppObject ) const override
        {
            if( ppObject )
            {
                *ppObject = nullptr;
            }
        }
        Array<SmartPtr<IResource>> getDependencies() const override
        {
            return {};
        }
        IResourceManager *getResourceManagerPtr() const override
        {
            return nullptr;
        }
        SmartPtr<IResourceManager> getResourceManager() const override
        {
            return nullptr;
        }
        void setResourceManager( SmartPtr<IResourceManager> resourceManager ) override
        {
        }
        IStateContext *getStateContextPtr() const override
        {
            return nullptr;
        }
        SmartPtr<IStateContext> getStateContext() const override
        {
            return nullptr;
        }
        bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override
        {
            return false;
        }
        bool handleStateChanged( SmartPtr<IState> &state ) override
        {
            return false;
        }

        SmartPtr<render::IRenderTarget> getRenderTarget() const override
        {
            return nullptr;
        }
        void setRenderTarget( SmartPtr<render::IRenderTarget> rt ) override
        {
        }
        void copyToTexture( SmartPtr<render::ITexture> &target ) override
        {
            target = this;
        }
        void copyData( void *data, const Vector2I &size ) override
        {
            this->size = size;
        }
        Vector2I getSize() const override
        {
            return size;
        }
        void setSize( const Vector2I &size ) override
        {
            this->size = size;
        }
        Vector2I getActualSize() const override
        {
            return size;
        }
        void getTextureGPU( void **ppTexture ) const override
        {
            if( ppTexture )
            {
                *ppTexture = nullptr;
            }
        }
        void getTextureFinal( void **ppTexture ) const override
        {
            if( ppTexture )
            {
                *ppTexture = nullptr;
            }
        }
        size_t getTextureHandle() const override
        {
            return 0u;
        }
        u32 getUsageFlags() const override
        {
            return usageFlags;
        }
        void setUsageFlags( u32 usageFlags ) override
        {
            this->usageFlags = usageFlags;
        }

        workphone::UUID fileSystemId;
        workphone::UUID settingsFileSystemId;
        String filePath;
        Vector2I size = Vector2I( 1, 1 );
        u32 usageFlags = 0u;
    };

    class RecordingTerrain : public render::IGraphicsTerrain
    {
    public:
        Transform3<real_Num> getWorldTransform() const override
        {
            return {};
        }
        void setWorldTransform( const Transform3<real_Num> &worldTransform ) override
        {
            this->worldTransform = worldTransform;
        }

        Vector3<real_Num> getPosition() const override
        {
            return position;
        }
        void setPosition( const Vector3<real_Num> &position ) override
        {
            this->position = position;
        }
        f32 getHeightAtWorldPosition( const Vector3<real_Num> &position ) const override
        {
            return 0.0f;
        }
        u16 getSize() const override
        {
            return static_cast<u16>( heightMapSize.x );
        }
        Vector3<real_Num> getTerrainSpacePosition( const Vector3<real_Num> &worldSpace ) const override
        {
            return worldSpace;
        }

        Array<f32> getHeightData() const override
        {
            return heightData;
        }
        void setHeightData( const Array<f32> &heightData ) override
        {
            this->heightData = heightData;
        }

        bool isVisible() const override
        {
            return visible;
        }
        void setVisible( bool visible ) override
        {
            this->visible = visible;
        }
        bool getShowWireframe() const override
        {
            return showWireframe;
        }
        void setShowWireframe( bool showWireframe ) override
        {
            this->showWireframe = showWireframe;
        }

        String getMaterialName() const override
        {
            return materialName;
        }
        void setMaterialName( const String &materialName ) override
        {
            this->materialName = materialName;
        }

        SmartPtr<render::ITerrainBlendMap> getBlendMap( u32 index ) override
        {
            return nullptr;
        }
        u16 getLayerBlendMapSize() const override
        {
            return 0;
        }
        SmartPtr<render::ITerrainRayResult> intersects( const Ray3F &ray ) const override
        {
            return nullptr;
        }
        SmartPtr<IMesh> getMesh() const override
        {
            return nullptr;
        }

        Vector2I getHeightMapSize() const override
        {
            return heightMapSize;
        }
        void setHeightMapSize( const Vector2I &heightMapSize ) override
        {
            this->heightMapSize = heightMapSize;
        }

        f32 getHeightScale() const override
        {
            return heightScale;
        }
        void setHeightScale( f32 heightScale ) override
        {
            this->heightScale = heightScale;
        }

        SmartPtr<render::IGraphicsScene> getSceneManager() const override
        {
            return nullptr;
        }
        void setSceneManager( SmartPtr<render::IGraphicsScene> sceneManager ) override
        {
        }
        void _getObject( void **ppObject ) const override
        {
            if( ppObject )
            {
                *ppObject = nullptr;
            }
        }

        SmartPtr<render::ITexture> getHeightMap() const override
        {
            return heightMap;
        }
        void setHeightMap( SmartPtr<render::ITexture> heightMap ) override
        {
            this->heightMap = heightMap;
        }

        void setTextureLayer( s32 layer, const String &textureName ) override
        {
            textureLayerCalls.push_back( layer );
            textureLayerNames.push_back( textureName );
        }

        Array<SmartPtr<render::ITexture>> getTextures() const override
        {
            return textures;
        }
        void setTextures( const Array<SmartPtr<render::ITexture>> &textures ) override
        {
            this->textures = textures;
        }

        SmartPtr<render::ITexture> getTexture( u32 index ) const override
        {
            return index < textures.size() ? textures[index] : nullptr;
        }

        void setTexture( u32 index, SmartPtr<render::ITexture> texture ) override
        {
            if( textures.size() <= index )
            {
                textures.resize( index + 1u );
            }

            textures[index] = texture;
            textureSetSlots.push_back( index );
            textureSetValues.push_back( texture );
        }

        void updateMaterial() override
        {
            ++updateMaterialCalls;
        }

        Transform3<real_Num> worldTransform;
        Vector3<real_Num> position = Vector3<real_Num>::zero();
        Vector2I heightMapSize = Vector2I( 256, 256 );
        f32 heightScale = 50.0f;
        bool visible = true;
        bool showWireframe = false;
        String materialName;
        SmartPtr<render::ITexture> heightMap;
        Array<f32> heightData;
        Array<SmartPtr<render::ITexture>> textures;
        Array<u32> textureSetSlots;
        Array<SmartPtr<render::ITexture>> textureSetValues;
        Array<s32> textureLayerCalls;
        Array<String> textureLayerNames;
        u32 updateMaterialCalls = 0u;
    };

    bool isUiRuntimeAvailable()
    {
        auto applicationManager = core::IApplicationManager::instance();
        return applicationManager && applicationManager->getRenderUI();
    }

    bool skipWhenUiRuntimeUnavailable()
    {
        if( isUiRuntimeAvailable() )
        {
            return false;
        }

        BOOST_TEST_MESSAGE( "Skipping UI component test because no UI runtime plugin is available." );
        return true;
    }
}  // namespace

BOOST_AUTO_TEST_CASE( components_terrain_add )
{
    if( skipWhenUiRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto typeManager = TypeManager::instance();

        auto gameManager = applicationManager->getGameManager();
        auto gameScene = gameManager->getCurrentScene();

        auto actor = gameManager->createActor();
        auto terrain = actor->addComponent<TerrainSystem>();

        gameManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( components_terrain_remove )
{
    if( skipWhenUiRuntimeUnavailable() )
    {
        return;
    }

    try
    {
        TestGuard guard;
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto typeManager = TypeManager::instance();

        auto gameManager = applicationManager->getGameManager();
        auto gameScene = gameManager->getCurrentScene();

        auto actor = gameManager->createActor();
        auto terrain = actor->addComponent<TerrainSystem>();

        actor->removeComponentInstance( terrain );
        gameManager->destroyActor( actor );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( components_terrain_default_settings )
{
    try
    {
        TestGuard guard;
        auto terrain = workphone::make_ptr<TerrainSystem>();
        BOOST_REQUIRE( terrain );

        BOOST_CHECK_EQUAL( terrain->getNumLayers(), 0 );
        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().x, 256 );
        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().y, 256 );
        BOOST_CHECK_CLOSE( terrain->getHeightScale(), 50.0f, 0.001f );
        BOOST_CHECK_EQUAL( terrain->getShowWireframe(), false );
        BOOST_CHECK_EQUAL( terrain->getGeneratedHeightMapWidth(), 256u );
        BOOST_CHECK_EQUAL( terrain->getGeneratedHeightMapHeight(), 256u );
        BOOST_CHECK_CLOSE( terrain->getGeneratedHeightMapValueScale(), 255.0f, 0.001f );
        BOOST_CHECK_EQUAL( terrain->getGeneratedHeightMapType(), "gradient" );
        BOOST_CHECK_CLOSE( terrain->getDefaultMetalnessValue(), 0.5f, 0.001f );
        BOOST_CHECK_EQUAL( terrain->getTreesEnabled(), false );
        BOOST_CHECK_EQUAL( terrain->getGrassEnabled(), false );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_terrain_setters_round_trip )
{
    try
    {
        TestGuard guard;
        auto terrain = workphone::make_ptr<TerrainSystem>();
        BOOST_REQUIRE( terrain );

        terrain->setHeightMapSize( Vector2I( 64, 32 ) );
        terrain->setHeightScale( 125.0f );
        terrain->setShowWireframe( true );
        terrain->setGeneratedHeightMapWidth( 128u );
        terrain->setGeneratedHeightMapHeight( 96u );
        terrain->setGeneratedHeightMapValueScale( 42.0f );
        terrain->setGeneratedHeightMapType( "island" );
        terrain->setDefaultMetalnessValue( 0.25f );
        terrain->setTreesEnabled( true );
        terrain->setTreeDensity( 12.5f );
        terrain->setGrassEnabled( true );
        terrain->setGrassDensity( 77.0f );
        terrain->setPreviewTreeCount( 33u );
        terrain->setGeneratedTreeCount( 444u );
        terrain->setTreePrefabName( "oak.prefab" );

        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().x, 64 );
        BOOST_CHECK_EQUAL( terrain->getHeightMapSize().y, 32 );
        BOOST_CHECK_CLOSE( terrain->getHeightScale(), 125.0f, 0.001f );
        BOOST_CHECK_EQUAL( terrain->getShowWireframe(), true );
        BOOST_CHECK_EQUAL( terrain->getGeneratedHeightMapWidth(), 128u );
        BOOST_CHECK_EQUAL( terrain->getGeneratedHeightMapHeight(), 96u );
        BOOST_CHECK_CLOSE( terrain->getGeneratedHeightMapValueScale(), 42.0f, 0.001f );
        BOOST_CHECK_EQUAL( terrain->getGeneratedHeightMapType(), "island" );
        BOOST_CHECK_CLOSE( terrain->getDefaultMetalnessValue(), 0.25f, 0.001f );
        BOOST_CHECK_EQUAL( terrain->getTreesEnabled(), true );
        BOOST_CHECK_CLOSE( terrain->getTreeDensity(), 12.5f, 0.001f );
        BOOST_CHECK_EQUAL( terrain->getGrassEnabled(), true );
        BOOST_CHECK_CLOSE( terrain->getGrassDensity(), 77.0f, 0.001f );
        BOOST_CHECK_EQUAL( terrain->getPreviewTreeCount(), 33u );
        BOOST_CHECK_EQUAL( terrain->getGeneratedTreeCount(), 444u );
        BOOST_CHECK_EQUAL( terrain->getTreePrefabName(), "oak.prefab" );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_terrain_layers_are_indexed_after_resize )
{
    try
    {
        TestGuard guard;
        auto terrain = workphone::make_ptr<TerrainSystem>();
        BOOST_REQUIRE( terrain );

        terrain->setNumLayers( 3 );
        terrain->resizeLayermap();

        BOOST_CHECK_EQUAL( terrain->getNumLayers(), 3 );

        auto layers = terrain->getSubComponentsByType<TerrainLayer>();
        BOOST_REQUIRE_EQUAL( layers.size(), 3u );
        for( size_t i = 0; i < layers.size(); ++i )
        {
            BOOST_REQUIRE( layers[i] );
            BOOST_CHECK_EQUAL( layers[i]->getIndex(), static_cast<s32>( i ) );
        }

        terrain->setNumLayers( 1 );
        terrain->resizeLayermap();
        layers = terrain->getSubComponentsByType<TerrainLayer>();

        BOOST_CHECK_EQUAL( terrain->getNumLayers(), 1 );
        BOOST_REQUIRE_EQUAL( layers.size(), 1u );
        BOOST_CHECK_EQUAL( layers[0]->getIndex(), 0 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( components_terrain_update_layers_maps_to_detail_texture_slots )
{
    try
    {
        TestGuard guard;
        auto terrain = workphone::make_ptr<TerrainSystem>();
        BOOST_REQUIRE( terrain );

        auto recordingTerrain = workphone::make_ptr<RecordingTerrain>();
        BOOST_REQUIRE( recordingTerrain );
        terrain->setTerrain( recordingTerrain );

        auto texture0 = workphone::make_ptr<RecordingTexture>();
        auto texture1 = workphone::make_ptr<RecordingTexture>();
        auto texture2 = workphone::make_ptr<RecordingTexture>();

        terrain->setNumLayers( 3 );
        terrain->resizeLayermap();

        auto layers = terrain->getSubComponentsByType<TerrainLayer>();
        BOOST_REQUIRE_EQUAL( layers.size(), 3u );
        layers[0]->setBaseTexture( texture0 );
        layers[1]->setBaseTexture( texture1 );
        layers[2]->setBaseTexture( texture2 );

        terrain->updateLayers();

        const auto detail0 = static_cast<u32>( render::IGraphicsTerrain::TextureTypes::DETAIL0 );
        BOOST_REQUIRE_GT( recordingTerrain->textures.size(), detail0 + 2u );
        BOOST_CHECK( recordingTerrain->textures[detail0].get() == texture0.get() );
        BOOST_CHECK( recordingTerrain->textures[detail0 + 1u].get() == texture1.get() );
        BOOST_CHECK( recordingTerrain->textures[detail0 + 2u].get() == texture2.get() );
        BOOST_CHECK_EQUAL( recordingTerrain->textureLayerCalls.size(), 0u );
        BOOST_CHECK_EQUAL( recordingTerrain->updateMaterialCalls, 1u );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}
