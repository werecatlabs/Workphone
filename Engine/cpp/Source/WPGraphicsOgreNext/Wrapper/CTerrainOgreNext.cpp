#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTerrainOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSystemOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CCameraOgreNext.hpp>
#include <WPGraphicsOgreNext/CompositorManager.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CLightOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <Utils/MeshUtils.hpp>
#include <Terra/Terra.h>
#include <Terra/TerraShadowMapper.h>
#include <Terra/Hlms/PbsListener/OgreHlmsPbsTerraShadows.h>
#include <Terra/Hlms/OgreHlmsTerraDatablock.h>
#include <Terra/Hlms/OgreHlmsTerra.h>
#include <Ogre.h>
#include <OgreTextureGpuManager.h>
#include <OgreTextureBox.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, CTerrainOgreNext, Terrain );
    WP_CLASS_REGISTER_DERIVED( workphone::render, CTerrainOgreNext::StateListener, IStateListener );

    u32 CTerrainOgreNext::m_cameraNameExt = 0;

    namespace
    {
        constexpr s32 DetailLayerCount = 4;
        constexpr Ogre::Real RaycastEpsilon = Ogre::Real( 1e-4f );
        constexpr u32 RaycastMinSteps = 32u;
        constexpr u32 RaycastMaxSteps = 512u;
        constexpr const char *DefaultTerraMaterialName = "TerraExampleMaterial";

        struct TerrainTextureProperty
        {
            IGraphicsTerrain::TextureTypes textureType;
            const String &propertyName;
        };

        class TerrainRayResult final : public ITerrainRayResult
        {
        public:
            TerrainRayResult() = default;
            ~TerrainRayResult() override = default;

            bool hasIntersected() const override
            {
                return m_intersected;
            }

            void setIntersected( bool intersected ) override
            {
                m_intersected = intersected;
            }

            SmartPtr<IGraphicsTerrain> getTerrain() const override
            {
                return m_terrain;
            }

            void setTerrain( SmartPtr<IGraphicsTerrain> terrain ) override
            {
                m_terrain = terrain;
            }

            Vector3F getPosition() const override
            {
                return m_position;
            }

            void setPosition( const Vector3F &position ) override
            {
                m_position = position;
            }

        private:
            Vector3F m_position = Vector3F::zero();
            SmartPtr<IGraphicsTerrain> m_terrain;
            bool m_intersected = false;
        };

        bool isFinite( const Ogre::Vector3 &value )
        {
            return std::isfinite( value.x ) && std::isfinite( value.y ) && std::isfinite( value.z );
        }

        bool isPositiveFinite( const Ogre::Vector3 &value )
        {
            return isFinite( value ) && value.x > Ogre::Real( 0.0f ) && value.y > Ogre::Real( 0.0f ) &&
                   value.z > Ogre::Real( 0.0f );
        }

        bool isValidTerraTextureSlot( u32 index )
        {
            return index < static_cast<u32>( Ogre::TerraTextureTypes::NUM_TERRA_TEXTURE_TYPES );
        }

        bool isValidHeightMapSize( const Vector2I &size )
        {
            return size.x >= 2 && size.y >= 2;
        }

        bool getHeightDataElementCount( const Vector2I &size, size_t &elementCount )
        {
            if( !isValidHeightMapSize( size ) )
            {
                elementCount = 0;
                return false;
            }

            const auto width = static_cast<size_t>( size.x );
            const auto height = static_cast<size_t>( size.y );
            if( width > std::numeric_limits<size_t>::max() / height )
            {
                elementCount = 0;
                return false;
            }

            elementCount = width * height;
            return true;
        }

        bool isValidHeightData( const Array<f32> &heightData, const Vector2I &size )
        {
            size_t expectedSize = 0;
            if( !getHeightDataElementCount( size, expectedSize ) || heightData.size() != expectedSize )
            {
                return false;
            }

            for( auto heightValue : heightData )
            {
                if( !MathF::isFinite( heightValue ) )
                {
                    return false;
                }
            }

            return true;
        }

        u32 getTerraDetailSlot( s32 layer )
        {
            WP_ASSERT( layer >= 0 );
            WP_ASSERT( layer < DetailLayerCount );
            return static_cast<u32>( Ogre::TerraTextureTypes::TERRA_DETAIL0 ) +
                   static_cast<u32>( layer );
        }

        void resizeTerrainTextures( Array<SmartPtr<ITexture>> &textures )
        {
            auto textureCount = static_cast<size_t>( Ogre::TerraTextureTypes::NUM_TERRA_TEXTURE_TYPES );
            if( textures.size() < textureCount )
            {
                textures.resize( textureCount );
            }
        }

        void normaliseLightDirection( Ogre::Vector3 &lightDirection, Ogre::Real epsilon )
        {
            const auto minLengthSquared = epsilon * epsilon;
            if( !isFinite( lightDirection ) || lightDirection.squaredLength() <= minLengthSquared )
            {
                lightDirection = CTerrainOgreNext::DEFAULT_LIGHT_DIRECTION;
                return;
            }

            lightDirection.normalise();
        }

        bool updateRaycastSlab( Ogre::Real origin, Ogre::Real direction, Ogre::Real minimum,
                                Ogre::Real maximum, Ogre::Real &tMin, Ogre::Real &tMax )
        {
            if( std::fabs( direction ) <= RaycastEpsilon )
            {
                return origin >= minimum && origin <= maximum;
            }

            Ogre::Real nearT = ( minimum - origin ) / direction;
            Ogre::Real farT = ( maximum - origin ) / direction;
            if( nearT > farT )
            {
                std::swap( nearT, farT );
            }

            tMin = std::max( tMin, nearT );
            tMax = std::min( tMax, farT );
            return tMin <= tMax;
        }

        SmartPtr<ITerrainRayResult> makeRayResult( const Ogre::Vector3 &position )
        {
            SmartPtr<ITerrainRayResult> result( new TerrainRayResult );
            result->setIntersected( true );
            result->setPosition( Vector3F( position.x, position.y, position.z ) );
            return result;
        }
    }  // namespace

    const Ogre::Vector3 CTerrainOgreNext::DEFAULT_DIMENSIONS =
        Ogre::Vector3( 256.0f, 83.26562f, 256.0f );
    const Ogre::Vector3 CTerrainOgreNext::DEFAULT_LIGHT_DIRECTION =
        Ogre::Vector3( 0.01f, -0.9958f, 0.3f );
    const time_interval CTerrainOgreNext::DEFAULT_UPDATE_INTERVAL = 1.0 / 30.0;
    const time_interval CTerrainOgreNext::DEFAULT_INITIAL_UPDATE_DELAY = 3.0;
    const f32 CTerrainOgreNext::DEFAULT_LIGHT_EPSILON = 1e-6f;
    const u32 CTerrainOgreNext::DEFAULT_RENDER_QUEUE_ID = 11u;
    const bool CTerrainOgreNext::DEFAULT_CAST_SHADOWS = false;

    const String CTerrainOgreNext::TerrainDimensionsStr = "terrainDimensions";
    const String CTerrainOgreNext::LightDirectionStr = "lightDirection";
    const String CTerrainOgreNext::LightEpsilonStr = "lightEpsilon";
    const String CTerrainOgreNext::UpdateIntervalStr = "updateInterval";
    const String CTerrainOgreNext::InitialUpdateDelayStr = "initialUpdateDelay";
    const String CTerrainOgreNext::RenderQueueIdStr = "renderQueueId";
    const String CTerrainOgreNext::CastShadowsStr = "castShadows";
    const String CTerrainOgreNext::ShowWireframeStr = "showWireframe";
    const String CTerrainOgreNext::PositionStr = "position";
    const String CTerrainOgreNext::NextUpdateTimeStr = "nextUpdateTime";
    const String CTerrainOgreNext::HeightMapStr = "heightMap";
    const String CTerrainOgreNext::HeightMapPathStr = "heightMapPath";
    const String CTerrainOgreNext::TexturesStr = "Textures";
    const String CTerrainOgreNext::DiffuseTextureStr = "diffuseTexture";
    const String CTerrainOgreNext::DetailWeightTextureStr = "detailWeightTexture";
    const String CTerrainOgreNext::DetailTexture0Str = "detailTexture0";
    const String CTerrainOgreNext::DetailTexture1Str = "detailTexture1";
    const String CTerrainOgreNext::DetailTexture2Str = "detailTexture2";
    const String CTerrainOgreNext::DetailTexture3Str = "detailTexture3";
    const String CTerrainOgreNext::DetailTexture0NMStr = "detailTexture0NM";
    const String CTerrainOgreNext::DetailTexture1NMStr = "detailTexture1NM";
    const String CTerrainOgreNext::DetailTexture2NMStr = "detailTexture2NM";
    const String CTerrainOgreNext::DetailTexture3NMStr = "detailTexture3NM";
    const String CTerrainOgreNext::DetailRoughness0Str = "detailRoughness0";
    const String CTerrainOgreNext::DetailRoughness1Str = "detailRoughness1";
    const String CTerrainOgreNext::DetailRoughness2Str = "detailRoughness2";
    const String CTerrainOgreNext::DetailRoughness3Str = "detailRoughness3";
    const String CTerrainOgreNext::DetailMetalness0Str = "detailMetalness0";
    const String CTerrainOgreNext::DetailMetalness1Str = "detailMetalness1";
    const String CTerrainOgreNext::DetailMetalness2Str = "detailMetalness2";
    const String CTerrainOgreNext::DetailMetalness3Str = "detailMetalness3";
    const String CTerrainOgreNext::ReflectionTextureStr = "reflectionTexture";

    static const TerrainTextureProperty TerrainTextureProperties[] = {
        { IGraphicsTerrain::TextureTypes::DIFFUSE, CTerrainOgreNext::DiffuseTextureStr },
        { IGraphicsTerrain::TextureTypes::DETAIL_WEIGHT, CTerrainOgreNext::DetailWeightTextureStr },
        { IGraphicsTerrain::TextureTypes::DETAIL0, CTerrainOgreNext::DetailTexture0Str },
        { IGraphicsTerrain::TextureTypes::DETAIL1, CTerrainOgreNext::DetailTexture1Str },
        { IGraphicsTerrain::TextureTypes::DETAIL2, CTerrainOgreNext::DetailTexture2Str },
        { IGraphicsTerrain::TextureTypes::DETAIL3, CTerrainOgreNext::DetailTexture3Str },
        { IGraphicsTerrain::TextureTypes::DETAIL0_NM, CTerrainOgreNext::DetailTexture0NMStr },
        { IGraphicsTerrain::TextureTypes::DETAIL1_NM, CTerrainOgreNext::DetailTexture1NMStr },
        { IGraphicsTerrain::TextureTypes::DETAIL2_NM, CTerrainOgreNext::DetailTexture2NMStr },
        { IGraphicsTerrain::TextureTypes::DETAIL3_NM, CTerrainOgreNext::DetailTexture3NMStr },
        { IGraphicsTerrain::TextureTypes::DETAIL_ROUGHNESS0, CTerrainOgreNext::DetailRoughness0Str },
        { IGraphicsTerrain::TextureTypes::DETAIL_ROUGHNESS1, CTerrainOgreNext::DetailRoughness1Str },
        { IGraphicsTerrain::TextureTypes::DETAIL_ROUGHNESS2, CTerrainOgreNext::DetailRoughness2Str },
        { IGraphicsTerrain::TextureTypes::DETAIL_ROUGHNESS3, CTerrainOgreNext::DetailRoughness3Str },
        { IGraphicsTerrain::TextureTypes::DETAIL_METALNESS0, CTerrainOgreNext::DetailMetalness0Str },
        { IGraphicsTerrain::TextureTypes::DETAIL_METALNESS1, CTerrainOgreNext::DetailMetalness1Str },
        { IGraphicsTerrain::TextureTypes::DETAIL_METALNESS2, CTerrainOgreNext::DetailMetalness2Str },
        { IGraphicsTerrain::TextureTypes::DETAIL_METALNESS3, CTerrainOgreNext::DetailMetalness3Str },
        { IGraphicsTerrain::TextureTypes::REFLECTION, CTerrainOgreNext::ReflectionTextureStr }
    };

    CTerrainOgreNext::CTerrainOgreNext()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto stateManager = applicationManager->getStateManager();
        WP_ASSERT( stateManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto stateContext = stateManager->addStateContext();
        WP_ASSERT( stateContext );

        stateContext->setOwner( this );
        setStateContext( stateContext );
        stateContext->setTaskId( TaskId::Render );

        auto graphicsObjectState = factoryManager->make_ptr<State>();
        graphicsObjectState->setId( getId() );
        graphicsObjectState->setOwner( this );
        stateContext->addState( graphicsObjectState );

        auto graphicsObjectStateData = factoryManager->make_ptr<GraphicsObjectData>();
        graphicsObjectState->setData( graphicsObjectStateData );

        auto transformState = factoryManager->make_ptr<State>();
        transformState->setId( getId() );
        transformState->setOwner( this );
        stateContext->addState( transformState );

        auto transformData = factoryManager->make_ptr<TransformStateData>();
        transformState->setData( transformData );

        auto terrainState = factoryManager->make_ptr<State>();
        terrainState->setId( getId() );
        terrainState->setOwner( this );
        stateContext->addState( terrainState );

        auto terrainData = factoryManager->make_ptr<TerrainStateData>();
        terrainState->setData( terrainData );

        m_center = Ogre::Vector3( 0.0f, getHeightScale() * 0.5f, 0.0f );

        auto stateListener = workphone::make_ptr<StateListener>();
        stateListener->setOwner( this );
        setStateListener( stateListener );
        stateContext->addStateListener( stateListener );

        //m_heightImage = new Ogre::Image2();

        size_t expectedHeightDataSize = 0;
        auto heightMapSize = getHeightMapSize();
        if( !getHeightDataElementCount( heightMapSize, expectedHeightDataSize ) )
        {
            heightMapSize = Vector2I( 2, 2 );
            setHeightMapSize( heightMapSize );
            getHeightDataElementCount( heightMapSize, expectedHeightDataSize );
        }

        auto heightData = getHeightData();
        heightData.resize( expectedHeightDataSize, 0.0f );
        setHeightData( heightData );
    }

    CTerrainOgreNext::~CTerrainOgreNext()
    {
        unload( nullptr );

        if( m_heightImage )
        {
            delete m_heightImage;
            m_heightImage = nullptr;
        }
    }

    void CTerrainOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            using namespace Ogre;

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto timer = applicationManager->getTimerPtr();
            WP_ASSERT( timer );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            auto pGraphicsSystem = (CGraphicsSystemOgreNext *)graphicsSystem;
            WP_ASSERT( pGraphicsSystem );

            auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
            WP_ASSERT( resourceGroupManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto pSceneManager =
                workphone::static_pointer_cast<CGraphicsSceneOgreNext>( getSceneManager() );
            WP_ASSERT( pSceneManager );
            if( !pSceneManager )
            {
                WP_LOG_ERROR( "Invalid scene manager type for terrain" );
                setLoadingState( LoadingState::Error );
                return;
            }

            auto renderTask = graphicsSystem->getRenderTask();
            auto stateTask = graphicsSystem->getStateTask();
            auto task = Thread::getCurrentTask();

            auto textureCount = (u32)Ogre::TerraTextureTypes::NUM_TERRA_TEXTURE_TYPES;
            //m_textures.resize( textureCount );

            m_nextUpdateTime = timer->getTime() + getInitialUpdateDelay();

            auto root = Root::getSingletonPtr();
            WP_ASSERT( root );
            if( !root )
            {
                setLoadingState( LoadingState::Error );
                return;
            }

            auto hlmsManager = root->getHlmsManager();
            WP_ASSERT( hlmsManager );
            if( !hlmsManager )
            {
                setLoadingState( LoadingState::Error );
                return;
            }
            auto hlmsCompute = hlmsManager->getComputeHlms();

            auto sceneManager = pSceneManager->getSceneManager();
            WP_ASSERT( sceneManager );
            if( !sceneManager )
            {
                setLoadingState( LoadingState::Error );
                return;
            }

            createTerraInstance();
            WP_ASSERT( m_terra );
            if( !m_terra )
            {
                setLoadingState( LoadingState::Error );
                return;
            }

            auto heightMapSize = getHeightMapSize();
            WP_ASSERT( isValidHeightMapSize( heightMapSize ) );
            if( !isValidHeightMapSize( heightMapSize ) )
            {
                WP_LOG_ERROR( "Invalid height map size for terrain" );
                heightMapSize.x = Math<s32>::max( 2, heightMapSize.x );
                heightMapSize.y = Math<s32>::max( 2, heightMapSize.y );
                setHeightMapSize( heightMapSize );
            }

            if( m_dimensions.x <= Ogre::Real( 0.0f ) || m_dimensions.z <= Ogre::Real( 0.0f ) )
            {
                m_dimensions.x = static_cast<Ogre::Real>( heightMapSize.x );
                m_dimensions.z = static_cast<Ogre::Real>( heightMapSize.y );
            }

            m_dimensions.y = getHeightScale();
            if( m_dimensions.y <= Ogre::Real( 0.0f ) )
            {
                m_dimensions.y = DEFAULT_DIMENSIONS.y;
                setHeightScale( static_cast<f32>( m_dimensions.y ) );
            }

            WP_ASSERT( isFinite( m_center ) );
            if( !isFinite( m_center ) )
            {
                WP_LOG_WARNING( "CTerrainOgreNext::load: invalid terrain position; using origin." );
                m_center = Ogre::Vector3( 0.0f, m_dimensions.y * Ogre::Real( 0.5f ), 0.0f );
            }

            WP_ASSERT( isPositiveFinite( m_dimensions ) );
            if( !isPositiveFinite( m_dimensions ) )
            {
                WP_LOG_ERROR( "CTerrainOgreNext::load: invalid terrain dimensions." );
                setLoadingState( LoadingState::Error );
                return;
            }

            auto heightData = getHeightData();
            if( !isValidHeightData( heightData, heightMapSize ) )
            {
                size_t expectedSize = 0;
                if( !getHeightDataElementCount( heightMapSize, expectedSize ) )
                {
                    WP_LOG_ERROR( "CTerrainOgreNext::load: invalid height data dimensions." );
                    setLoadingState( LoadingState::Error );
                    return;
                }

                WP_LOG_WARNING(
                    "CTerrainOgreNext::load: height data missing or invalid; using a flat terrain." );
                heightData.assign( expectedSize, 0.0f );
                Terrain::setHeightData( heightData );
            }

            if( !m_heightImage )
            {
                m_heightImage = new Ogre::Image2();
            }
            WP_ASSERT( m_heightImage );
            createImageDataFromHeightData();

            if( !m_heightImage || m_heightImage->getWidth() == 0 || m_heightImage->getHeight() == 0 )
            {
                WP_LOG_ERROR( "CTerrainOgreNext::load: failed to create height image." );
                setLoadingState( LoadingState::Error );
                return;
            }

            m_terra->load( *m_heightImage, m_center, m_dimensions, false, false );

            auto rootNode = sceneManager->getRootSceneNode( SCENE_STATIC );
            WP_ASSERT( rootNode );
            auto sceneNode = rootNode->createChildSceneNode( SCENE_STATIC );
            WP_ASSERT( sceneNode );
            sceneNode->attachObject( m_terra );
            WP_ASSERT( m_terra->isAttached() );
            setSceneNode( sceneNode );

            updateMaterial();

            pGraphicsSystem->setTerra( m_terra );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            auto message = String( e.what() );
            WP_LOG_ERROR( message );

            setLoadingState( LoadingState::Error );
        }
    }

    void CTerrainOgreNext::createTerraInstance()
    {
        auto root = Ogre::Root::getSingletonPtr();
        WP_ASSERT( root );
        if( !root )
        {
            return;
        }

        auto pSceneManager = workphone::static_pointer_cast<CGraphicsSceneOgreNext>( getSceneManager() );
        WP_ASSERT( pSceneManager );
        if( !pSceneManager )
        {
            return;
        }
        auto sceneManager = pSceneManager->getSceneManager();
        WP_ASSERT( sceneManager );
        if( !sceneManager )
        {
            return;
        }
        WP_ASSERT( !m_terra );

        if( !m_terrainCameraNode )
        {
            auto cameraName = String( "TerrainCamera" ) + StringUtil::toString( m_cameraNameExt++ );
            m_terrainCamera = sceneManager->createCamera( cameraName );
            WP_ASSERT( m_terrainCamera );
            m_terrainCameraNode = m_terrainCamera->getParentSceneNode();
            WP_ASSERT( m_terrainCameraNode );
        }
        else
        {
            WP_ASSERT( m_terrainCamera );
            WP_ASSERT( m_terrainCamera->getParentSceneNode() == m_terrainCameraNode );
        }

        auto compositorManager = root->getCompositorManager2();
        WP_ASSERT( compositorManager );
        if( !compositorManager )
        {
            return;
        }

        auto entityMemoryManager = &sceneManager->_getEntityMemoryManager( Ogre::SCENE_STATIC );
        WP_ASSERT( entityMemoryManager );

        WP_ASSERT( getRenderQueueId() < 256u );
        auto renderQueueId = static_cast<u8>( std::min( getRenderQueueId(), static_cast<u32>( 255u ) ) );

        auto id = Ogre::Id::generateNewId<Ogre::MovableObject>();

        // Render terrain after most objects, to improve performance by taking advantage of early Z
        m_terra = new Ogre::Terra( id, entityMemoryManager, sceneManager, renderQueueId,
                                   compositorManager, m_terrainCamera, false );
        WP_ASSERT( m_terra );
        m_terra->setCastShadows( getCastShadows() );
    }

    void CTerrainOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Unloaded )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            auto root = Ogre::Root::getSingletonPtr();
            WP_ASSERT( root );
            auto hlmsManager = root ? root->getHlmsManager() : nullptr;
            WP_ASSERT( hlmsManager );

            if( m_hlmsPbsTerraShadows )
            {
                if( hlmsManager )
                {
                    auto hlmsPbs = hlmsManager->getHlms( Ogre::HLMS_PBS );
                    WP_ASSERT( hlmsPbs );
                    if( hlmsPbs )
                    {
                        hlmsPbs->setListener( nullptr );
                    }
                }

                m_hlmsPbsTerraShadows->setTerra( nullptr );
                delete m_hlmsPbsTerraShadows;
                m_hlmsPbsTerraShadows = nullptr;
            }

            if( m_terra )
            {
                if( m_terra->isAttached() )
                {
                    WP_ASSERT( m_sceneNode );
                    m_terra->detachFromParent();
                    WP_ASSERT( !m_terra->isAttached() );
                }

                if( auto applicationManager = core::IApplicationManager::instancePtr() )
                {
                    auto graphicsSystem =
                        (CGraphicsSystemOgreNext *)applicationManager->getGraphicsSystemPtr();
                    if( graphicsSystem )
                    {
                        if( graphicsSystem->getTerra() == m_terra )
                        {
                            graphicsSystem->setTerra( nullptr );
                            WP_ASSERT( !graphicsSystem->getTerra() );
                        }
                    }
                }

                delete m_terra;
                m_terra = nullptr;
            }

            if( m_sceneNode )
            {
                auto parentSceneNode = m_sceneNode->getParentSceneNode();
                if( parentSceneNode )
                {
                    parentSceneNode->removeAndDestroyChild( m_sceneNode );
                    m_sceneNode = nullptr;
                }
            }

            if( m_terrainCamera )
            {
                auto pSceneManager =
                    workphone::static_pointer_cast<CGraphicsSceneOgreNext>( getSceneManager() );
                if( pSceneManager )
                {
                    auto sceneManager = pSceneManager->getSceneManager();
                    if( sceneManager )
                        sceneManager->destroyCamera( m_terrainCamera );
                }
                m_terrainCamera = nullptr;
                m_terrainCameraNode = nullptr;
            }

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CTerrainOgreNext::reload( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        ScopedLock lock( graphicsSystem );

        unload( data );
        load( data );
    }

    void CTerrainOgreNext::postUpdate()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto timer = applicationManager->getTimerPtr();
            WP_ASSERT( timer );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            auto renderTask = graphicsSystem->getRenderTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();

            if( loadingState == LoadingState::Loaded && task == renderTask )
            {
                if( m_nextUpdateTime < timer->getTime() )
                {
                    auto window = graphicsSystem->getDefaultWindow();
                    if( window )
                    {
                        if( window->isVisible() )
                        {
                            if( m_terra )
                            {
                                auto pSceneManager =
                                    workphone::static_pointer_cast<CGraphicsSceneOgreNext>(
                                        getSceneManager() );
                                WP_ASSERT( pSceneManager );
                                if( !pSceneManager )
                                {
                                    return;
                                }

                                auto sceneManager = pSceneManager->getSceneManager();
                                WP_ASSERT( sceneManager );

                                auto pCamera = pSceneManager->getActiveCamera();
                                if( !pCamera )
                                {
                                    pCamera = pSceneManager->getCamera();
                                }

                                // copy camera properties to terrain camera
                                if( pCamera )
                                {
                                    auto cameraOgreNext =
                                        workphone::static_pointer_cast<CCameraOgreNext>( pCamera );
                                    WP_ASSERT( cameraOgreNext );

                                    Ogre::Camera *cameraOgre = nullptr;
                                    if( cameraOgreNext )
                                    {
                                        cameraOgreNext->_getObject( (void **)&cameraOgre );
                                    }

                                    WP_ASSERT( cameraOgre );

                                    if( cameraOgre )
                                    {
                                        auto cameraNodeOgreNext = cameraOgre->getParentSceneNode();
                                        WP_ASSERT( cameraNodeOgreNext );
                                        WP_ASSERT( m_terrainCamera );
                                        WP_ASSERT( m_terrainCameraNode );

                                        if( cameraNodeOgreNext && m_terrainCamera &&
                                            m_terrainCameraNode )
                                        {
                                            auto cameraPosition = cameraNodeOgreNext->getPosition();
                                            auto cameraOrientation =
                                                cameraNodeOgreNext->getOrientation();
                                            m_terrainCameraNode->setPosition( cameraPosition );
                                            m_terrainCameraNode->setOrientation( cameraOrientation );

                                            m_terrainCamera->synchroniseBaseSettingsWith( cameraOgre );
                                            m_terrainCamera->setAutoAspectRatio(
                                                cameraOgre->getAutoAspectRatio() );
                                        }
                                    }
                                }

                                WP_ASSERT( m_terrainCamera );
                                m_terra->setCamera( m_terrainCamera );

                                auto lights = pSceneManager->getObjectByType<render::IGraphicsLight>();
                                for( auto light : lights )
                                {
                                    auto type = light->getType();
                                    if( type == LightTypes::LT_DIRECTIONAL )
                                    {
                                        auto pLightOgreNext =
                                            workphone::static_pointer_cast<CLightOgreNext>( light );
                                        WP_ASSERT( pLightOgreNext );
                                        auto lightDirection = pLightOgreNext->getDerivedDirection();
                                        m_lightDirection = Ogre::Vector3(
                                            lightDirection.x, lightDirection.y, lightDirection.z );
                                    }
                                }

                                normaliseLightDirection( m_lightDirection,
                                                         static_cast<Ogre::Real>( getLightEpsilon() ) );

                                WP_ASSERT( m_terra->getCamera() == m_terrainCamera );
                                m_terra->update( m_lightDirection, m_lightEpsilon );
                            }
                        }
                    }

                    m_nextUpdateTime = timer->getTime() + getUpdateInterval();
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<Properties> CTerrainOgreNext::getProperties() const
    {
        try
        {
            auto properties = Terrain::getProperties();
            if( !properties )
            {
                WP_LOG_WARNING(
                    "CTerrainOgreNext::getProperties: base class returned null properties." );
                return {};
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                WP_LOG_WARNING( "CTerrainOgreNext::getProperties: null application manager." );
                return properties;
            }

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            if( !factoryManager )
            {
                WP_LOG_WARNING( "CTerrainOgreNext::getProperties: null factory manager." );
                return properties;
            }

            properties->setProperty( PositionStr, getPosition() );
            properties->setProperty( TerrainDimensionsStr, getTerrainDimensions() );
            properties->setProperty( LightDirectionStr, getLightDirection() );
            properties->setProperty( LightEpsilonStr, getLightEpsilon() );
            properties->setProperty( UpdateIntervalStr, static_cast<f32>( getUpdateInterval() ) );
            properties->setProperty( InitialUpdateDelayStr,
                                     static_cast<f32>( getInitialUpdateDelay() ) );
            properties->setProperty( NextUpdateTimeStr, static_cast<f32>( getNextUpdateTime() ) );
            properties->setProperty( RenderQueueIdStr, getRenderQueueId() );
            properties->setProperty( CastShadowsStr, getCastShadows() );
            properties->setProperty( ShowWireframeStr, getShowWireframe() );
            properties->setProperty( HeightMapStr, getHeightMap() );
            //properties->setProperty( HeightMapPathStr, getHeightMapPath() );

            auto textureProperties = factoryManager->make_ptr<Properties>();
            textureProperties->setName( TexturesStr );
            properties->addChild( textureProperties );

            auto textures = getTextures();
            resizeTerrainTextures( textures );
            for( const auto &textureProperty : TerrainTextureProperties )
            {
                auto textureIndex = static_cast<size_t>( textureProperty.textureType );
                textureProperties->setProperty( textureProperty.propertyName, textures[textureIndex] );
            }

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    void CTerrainOgreNext::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            if( !properties )
            {
                WP_LOG_WARNING( "CTerrainOgreNext::setProperties: null properties supplied." );
                return;
            }

            auto oldDimensions = getTerrainDimensions();
            auto oldPosition = getPosition();
            auto oldHeightMapSize = getHeightMapSize();
            auto oldRenderQueueId = getRenderQueueId();
            auto oldCastShadows = getCastShadows();
            auto oldHeightMap = getHeightMap();

            Terrain::setProperties( properties );

            auto position = getPosition();
            auto terrainDimensions = getTerrainDimensions();
            auto lightDirection = getLightDirection();
            auto lightEpsilon = getLightEpsilon();
            auto updateInterval = static_cast<f32>( getUpdateInterval() );
            auto initialUpdateDelay = static_cast<f32>( getInitialUpdateDelay() );
            auto nextUpdateTime = static_cast<f32>( getNextUpdateTime() );
            auto renderQueueId = getRenderQueueId();
            auto castShadows = getCastShadows();
            auto showWireframe = getShowWireframe();
            auto heightMap = getHeightMap();
            //auto heightMapPath = getHeightMapPath();

            properties->getPropertyValue( PositionStr, position );
            auto hasTerrainDimensions =
                properties->getPropertyValue( TerrainDimensionsStr, terrainDimensions );
            properties->getPropertyValue( LightDirectionStr, lightDirection );
            properties->getPropertyValue( LightEpsilonStr, lightEpsilon );
            properties->getPropertyValue( UpdateIntervalStr, updateInterval );
            properties->getPropertyValue( InitialUpdateDelayStr, initialUpdateDelay );
            properties->getPropertyValue( NextUpdateTimeStr, nextUpdateTime );
            properties->getPropertyValue( RenderQueueIdStr, renderQueueId );
            properties->getPropertyValue( CastShadowsStr, castShadows );
            properties->getPropertyValue( ShowWireframeStr, showWireframe );
            properties->getPropertyValue( HeightMapStr, heightMap );
            //properties->getPropertyValue( HeightMapPathStr, heightMapPath );

            setPosition( position );

            if( hasTerrainDimensions )
            {
                setTerrainDimensions( terrainDimensions );
            }
            else
            {
                auto heightMapSize = getHeightMapSize();
                m_dimensions.x = static_cast<Ogre::Real>( heightMapSize.x );
                m_dimensions.y = getHeightScale();
                m_dimensions.z = static_cast<Ogre::Real>( heightMapSize.y );
            }

            setLightDirection( lightDirection );
            setLightEpsilon( lightEpsilon );
            setUpdateInterval( updateInterval );
            setInitialUpdateDelay( initialUpdateDelay );
            setNextUpdateTime( nextUpdateTime );
            setRenderQueueId( renderQueueId );
            setCastShadows( castShadows );
            setShowWireframe( showWireframe );
            //setHeightMapPath( heightMapPath );

            if( heightMap != oldHeightMap )
            {
                Terrain::setHeightMap( heightMap );
                if( heightMap )
                {
                    //setHeightMapPath( heightMap->getFilePath() );
                }
            }

            if( auto textureProperties = properties->getChild( TexturesStr ) )
            {
                auto textures = getTextures();
                resizeTerrainTextures( textures );

                for( const auto &textureProperty : TerrainTextureProperties )
                {
                    auto textureIndex = static_cast<size_t>( textureProperty.textureType );
                    textureProperties->getPropertyValue( textureProperty.propertyName,
                                                         textures[textureIndex] );
                }

                setTextures( textures );
            }

            auto heightMapSize = getHeightMapSize();
            auto reloadRequired = oldDimensions != getTerrainDimensions() ||
                                  oldPosition != getPosition() || oldHeightMapSize != heightMapSize ||
                                  oldRenderQueueId != getRenderQueueId() ||
                                  oldCastShadows != getCastShadows() || oldHeightMap != getHeightMap();

            if( oldHeightMapSize != heightMapSize )
            {
                size_t expectedSize = 0;
                if( getHeightDataElementCount( heightMapSize, expectedSize ) )
                {
                    auto heightData = getHeightData();
                    heightData.resize( expectedSize, 0.0f );
                    Terrain::setHeightData( heightData );
                }
                createImageDataFromHeightData();
            }

            if( reloadRequired && isLoaded() )
            {
                reload( nullptr );
            }
            else if( isLoaded() )
            {
                updateMaterial();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Vector3F CTerrainOgreNext::getPosition() const
    {
        WP_ASSERT( isFinite( m_center ) );
        return { m_center.x, m_center.y, m_center.z };
    }

    void CTerrainOgreNext::setPosition( const Vector3F &position )
    {
        WP_ASSERT( position.isFinite() );
        if( !position.isFinite() )
        {
            WP_LOG_WARNING( "CTerrainOgreNext::setPosition: invalid position ignored." );
            return;
        }

        m_center = Ogre::Vector3( position.x, position.y, position.z );
        WP_ASSERT( isFinite( m_center ) );
    }

    f32 CTerrainOgreNext::getHeightAtWorldPosition( const Vector3F &position ) const
    {
        WP_ASSERT( position.isFinite() );
        if( !position.isFinite() )
        {
            WP_LOG_WARNING( "CTerrainOgreNext::getHeightAtWorldPosition: invalid position supplied." );
            return 0.0f;
        }

        if( m_terra )
        {
            Ogre::Vector3 ogrePosition( position.x, position.y, position.z );
            if( m_terra->getHeightAt( ogrePosition ) )
            {
                WP_ASSERT( isFinite( ogrePosition ) );
                return static_cast<f32>( ogrePosition.y );
            }
        }

        const auto height = Terrain::getHeightAtWorldPosition( position ) * getHeightScale();
        WP_ASSERT( MathF::isFinite( height ) );
        return height;
    }

    u16 CTerrainOgreNext::getSize() const
    {
        const auto size = Terrain::getSize();
        WP_ASSERT( size >= 2u );
        return size;
    }

    Vector3F CTerrainOgreNext::getTerrainSpacePosition( const Vector3F &worldSpace ) const
    {
        WP_ASSERT( worldSpace.isFinite() );

        const auto heightMapSize = getHeightMapSize();
        WP_ASSERT( heightMapSize.x >= 2 );
        WP_ASSERT( heightMapSize.y >= 2 );
        WP_ASSERT( isPositiveFinite( m_dimensions ) );

        if( heightMapSize.x < 2 || heightMapSize.y < 2 || !isPositiveFinite( m_dimensions ) )
        {
            return Vector3F::zero();
        }

        const auto origin = m_terra ? m_terra->getTerrainOrigin() : m_center - m_dimensions * 0.5f;
        WP_ASSERT( isFinite( origin ) );

        const auto maxX = static_cast<Ogre::Real>( heightMapSize.x - 1 );
        const auto maxZ = static_cast<Ogre::Real>( heightMapSize.y - 1 );
        const auto terrainX = Ogre::Math::Clamp( ( ( worldSpace.x - origin.x ) / m_dimensions.x ) * maxX,
                                                 Ogre::Real( 0.0f ), maxX );
        const auto terrainZ = Ogre::Math::Clamp( ( ( worldSpace.z - origin.z ) / m_dimensions.z ) * maxZ,
                                                 Ogre::Real( 0.0f ), maxZ );

        const Vector3F terrainSpace( static_cast<f32>( terrainX ), worldSpace.y,
                                     static_cast<f32>( terrainZ ) );
        WP_ASSERT( terrainSpace.isFinite() );
        return terrainSpace;
    }

    Ogre::SceneNode *CTerrainOgreNext::getSceneNode() const
    {
        return m_sceneNode;
    }

    void CTerrainOgreNext::setSceneNode( Ogre::SceneNode *sceneNode )
    {
        m_sceneNode = sceneNode;
    }

    Ogre::SceneNode *CTerrainOgreNext::getTerrainCameraNode() const
    {
        return m_terrainCameraNode;
    }

    void CTerrainOgreNext::setTerrainCameraNode( Ogre::SceneNode *terrainCameraNode )
    {
        m_terrainCameraNode = terrainCameraNode;
    }

    Ogre::Camera *CTerrainOgreNext::getTerrainCamera() const
    {
        return m_terrainCamera;
    }

    void CTerrainOgreNext::setTerrainCamera( Ogre::Camera *terrainCamera )
    {
        m_terrainCamera = terrainCamera;
    }

    void CTerrainOgreNext::setTextureLayer( s32 layer, const String &textureName )
    {
        WP_ASSERT( layer >= 0 );
        WP_ASSERT( layer < DetailLayerCount );
        if( layer < 0 || layer >= DetailLayerCount )
        {
            WP_LOG_WARNING( "CTerrainOgreNext::setTextureLayer: layer index out of range." );
            return;
        }

        SmartPtr<ITexture> texture;
        if( !textureName.empty() )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            if( !applicationManager )
            {
                return;
            }

            auto resourceDatabase = applicationManager->getResourceDatabase();
            WP_ASSERT( resourceDatabase );
            if( !resourceDatabase )
            {
                return;
            }

            texture = resourceDatabase->loadResourceByType<ITexture>( textureName );
            WP_ASSERT( texture );
        }

        setTexture( getTerraDetailSlot( layer ), texture );
    }

    void CTerrainOgreNext::setTextures( const Array<SmartPtr<ITexture>> &textures )
    {
        auto resizedTextures = textures;
        resizeTerrainTextures( resizedTextures );
        Terrain::setTextures( resizedTextures );

        if( isLoaded() )
        {
            updateMaterial();
        }
    }

    void CTerrainOgreNext::setTexture( u32 index, SmartPtr<ITexture> texture )
    {
        WP_ASSERT( isValidTerraTextureSlot( index ) );
        if( !isValidTerraTextureSlot( index ) )
        {
            WP_LOG_WARNING( "CTerrainOgreNext::setTexture: texture index out of range." );
            return;
        }

        auto textures = getTextures();
        resizeTerrainTextures( textures );
        textures[index] = texture;
        Terrain::setTextures( textures );

        if( isLoaded() )
        {
            updateMaterial();
        }
    }

    Ogre::Image2 *CTerrainOgreNext::getHeightImage() const
    {
        return m_heightImage;
    }

    void CTerrainOgreNext::setHeightImage( Ogre::Image2 *heightImage )
    {
        if( m_heightImage && m_heightImage != heightImage )
        {
            delete m_heightImage;
        }

        m_heightImage = heightImage;
    }

    Ogre::HlmsPbsTerraShadows *CTerrainOgreNext::getHlmsPbsTerraShadows() const
    {
        return m_hlmsPbsTerraShadows;
    }

    void CTerrainOgreNext::setHlmsPbsTerraShadows( Ogre::HlmsPbsTerraShadows *hlmsPbsTerraShadows )
    {
        m_hlmsPbsTerraShadows = hlmsPbsTerraShadows;
    }

    void CTerrainOgreNext::_setHeightData( const Array<f32> &heightData )
    {
        const auto heightMapSize = getHeightMapSize();
        WP_ASSERT( isValidHeightMapSize( heightMapSize ) );
        if( !isValidHeightMapSize( heightMapSize ) )
        {
            WP_LOG_ERROR( "Invalid terrain height map size." );
            return;
        }

        size_t expectedSize = 0;
        if( !getHeightDataElementCount( heightMapSize, expectedSize ) ||
            heightData.size() != expectedSize )
        {
            WP_LOG_ERROR( "Invalid terrain height data size." );
            return;
        }

        for( auto heightValue : heightData )
        {
            WP_ASSERT( MathF::isFinite( heightValue ) );
            if( !MathF::isFinite( heightValue ) )
            {
                WP_LOG_ERROR( "Invalid terrain height data value." );
                return;
            }
        }

        Terrain::setHeightData( heightData );

        createImageDataFromHeightData();

        if( isLoaded() )
        {
            reload( nullptr );
        }
    }

    void CTerrainOgreNext::createImageDataFromHeightData()
    {
        ScopedLock lock( this );

        auto heightData = getHeightData();
        if( heightData.empty() )
            return;

        if( !m_heightImage )
        {
            m_heightImage = new Ogre::Image2();
        }
        WP_ASSERT( m_heightImage );

        auto heightMapSize = getHeightMapSize();
        WP_ASSERT( isValidHeightMapSize( heightMapSize ) );
        if( !isValidHeightMapSize( heightMapSize ) )
        {
            WP_LOG_WARNING( "CTerrainOgreNext::createImageDataFromHeightData: invalid size; skipping." );
            return;
        }

        const u32 width = static_cast<u32>( heightMapSize.x );
        const u32 height = static_cast<u32>( heightMapSize.y );
        const size_t expectedSize = static_cast<size_t>( width ) * static_cast<size_t>( height );

        WP_ASSERT( heightData.size() == expectedSize );
        if( !isValidHeightData( heightData, heightMapSize ) )
        {
            WP_LOG_WARNING(
                "CTerrainOgreNext::createImageDataFromHeightData: invalid height data; skipping." );
            return;
        }

        // PFG_R32_FLOAT: Terra reads raw float values directly scaled by m_height.
        // No per-pixel normalization needed; Terra applies height scale at load time.
        m_heightImage->createEmptyImage( width, height,
                                         1u,  // depthOrSlices
                                         Ogre::TextureTypes::Type2D, Ogre::PFG_R32_FLOAT,
                                         1u  // numMipmaps
        );

        // Write directly into the image buffer; avoids the overhead of setColourAt.
        const Ogre::TextureBox box = m_heightImage->getData( 0 );
        WP_ASSERT( box.data );
        for( u32 y = 0; y < height; ++y )
        {
            float *row = reinterpret_cast<float *>( box.at( 0, y, 0 ) );
            WP_ASSERT( row );
            for( u32 x = 0; x < width; ++x )
            {
                const auto heightValue = heightData[y * width + x];
                WP_ASSERT( MathF::isFinite( heightValue ) );
                row[x] = heightValue;
            }
        }
    }

    SmartPtr<ITerrainBlendMap> CTerrainOgreNext::getBlendMap( u32 index )
    {
        WP_ASSERT( index < static_cast<u32>( Ogre::TerraTextureTypes::NUM_TERRA_TEXTURE_TYPES ) );
        if( !isValidTerraTextureSlot( index ) )
        {
            WP_LOG_WARNING( "CTerrainOgreNext::getBlendMap: blend map index out of range." );
        }

        return nullptr;
    }

    u16 CTerrainOgreNext::getLayerBlendMapSize() const
    {
        WP_ASSERT( !m_terra || m_terra->getHeightMapTex() );

        const auto heightMapSize = getHeightMapSize();
        if( !isValidHeightMapSize( heightMapSize ) )
        {
            return 0;
        }

        return static_cast<u16>( std::min( Math<s32>::min( heightMapSize.x, heightMapSize.y ),
                                           static_cast<s32>( std::numeric_limits<u16>::max() ) ) );
    }

    SmartPtr<ITerrainRayResult> CTerrainOgreNext::intersects( const Ray3F &ray ) const
    {
        WP_ASSERT( ray.isValid() );
        if( !ray.isValid() || !m_terra || !isPositiveFinite( m_dimensions ) )
        {
            return nullptr;
        }

        auto origin = Ogre::Vector3( ray.getOrigin().x, ray.getOrigin().y, ray.getOrigin().z );
        auto direction =
            Ogre::Vector3( ray.getDirection().x, ray.getDirection().y, ray.getDirection().z );
        if( !isFinite( origin ) || !isFinite( direction ) ||
            direction.squaredLength() <= RaycastEpsilon * RaycastEpsilon )
        {
            return nullptr;
        }

        direction.normalise();

        const auto terrainOrigin = m_terra->getTerrainOrigin();
        const auto minX = terrainOrigin.x;
        const auto maxX = terrainOrigin.x + m_dimensions.x;
        const auto minZ = terrainOrigin.z;
        const auto maxZ = terrainOrigin.z + m_dimensions.z;

        Ogre::Real tMin = Ogre::Real( 0.0f );
        Ogre::Real tMax = std::numeric_limits<Ogre::Real>::max();
        if( !updateRaycastSlab( origin.x, direction.x, minX, maxX, tMin, tMax ) ||
            !updateRaycastSlab( origin.z, direction.z, minZ, maxZ, tMin, tMax ) ||
            tMax < Ogre::Real( 0.0f ) )
        {
            return nullptr;
        }

        tMin = std::max( tMin, Ogre::Real( 0.0f ) );
        if( tMax == std::numeric_limits<Ogre::Real>::max() )
        {
            if( std::fabs( direction.y ) <= RaycastEpsilon )
            {
                return nullptr;
            }

            Ogre::Vector3 samplePosition( origin.x, origin.y, origin.z );
            if( !m_terra->getHeightAt( samplePosition ) )
            {
                return nullptr;
            }

            const auto hitT = ( samplePosition.y - origin.y ) / direction.y;
            if( hitT < Ogre::Real( 0.0f ) )
            {
                return nullptr;
            }

            return makeRayResult( origin + direction * hitT );
        }

        auto sampleHeightDelta = [this, &origin, &direction]( Ogre::Real t, Ogre::Real &delta ) {
            auto samplePosition = origin + direction * t;
            if( !m_terra->getHeightAt( samplePosition ) )
            {
                return false;
            }

            delta = ( origin + direction * t ).y - samplePosition.y;
            return true;
        };

        Ogre::Real previousT = tMin;
        Ogre::Real previousDelta = Ogre::Real( 0.0f );
        if( !sampleHeightDelta( previousT, previousDelta ) )
        {
            return nullptr;
        }

        if( std::fabs( previousDelta ) <= RaycastEpsilon )
        {
            return makeRayResult( origin + direction * previousT );
        }

        const auto heightMapSize = getHeightMapSize();
        const auto terrainSteps = static_cast<u32>( Math<s32>::max( heightMapSize.x, heightMapSize.y ) );
        const auto stepCount = std::min( RaycastMaxSteps, std::max( RaycastMinSteps, terrainSteps ) );
        const auto stepSize = ( tMax - tMin ) / static_cast<Ogre::Real>( stepCount );

        for( u32 i = 1u; i <= stepCount; ++i )
        {
            const auto currentT = i == stepCount ? tMax : tMin + stepSize * i;

            Ogre::Real currentDelta = Ogre::Real( 0.0f );
            if( !sampleHeightDelta( currentT, currentDelta ) )
            {
                continue;
            }

            if( std::fabs( currentDelta ) <= RaycastEpsilon )
            {
                return makeRayResult( origin + direction * currentT );
            }

            if( ( previousDelta < Ogre::Real( 0.0f ) && currentDelta > Ogre::Real( 0.0f ) ) ||
                ( previousDelta > Ogre::Real( 0.0f ) && currentDelta < Ogre::Real( 0.0f ) ) )
            {
                auto lowT = previousT;
                auto highT = currentT;
                auto lowDelta = previousDelta;

                for( u32 iteration = 0u; iteration < 24u; ++iteration )
                {
                    const auto midT = ( lowT + highT ) * Ogre::Real( 0.5f );
                    Ogre::Real midDelta = Ogre::Real( 0.0f );
                    if( !sampleHeightDelta( midT, midDelta ) )
                    {
                        break;
                    }

                    if( std::fabs( midDelta ) <= RaycastEpsilon )
                    {
                        lowT = midT;
                        highT = midT;
                        break;
                    }

                    if( ( lowDelta < Ogre::Real( 0.0f ) && midDelta < Ogre::Real( 0.0f ) ) ||
                        ( lowDelta > Ogre::Real( 0.0f ) && midDelta > Ogre::Real( 0.0f ) ) )
                    {
                        lowT = midT;
                        lowDelta = midDelta;
                    }
                    else
                    {
                        highT = midT;
                    }
                }

                return makeRayResult( origin + direction * ( lowT + highT ) * Ogre::Real( 0.5f ) );
            }

            previousT = currentT;
            previousDelta = currentDelta;
        }

        return nullptr;
    }

    SmartPtr<IMesh> CTerrainOgreNext::getMesh() const
    {
        const auto heightMapSize = getHeightMapSize();
        WP_ASSERT( isValidHeightMapSize( heightMapSize ) );

        const auto heightData = getHeightData();
        if( !isValidHeightData( heightData, heightMapSize ) )
        {
            return nullptr;
        }

        const auto tileSize = static_cast<u32>( getSize() );
        WP_ASSERT( tileSize >= 2u );

        if( heightMapSize.x == heightMapSize.y )
        {
            return MeshUtil::getMesh( heightData, 1.0f, getHeightScale(), tileSize );
        }

        Array<f32> squareHeightData;
        squareHeightData.resize( static_cast<size_t>( tileSize ) * static_cast<size_t>( tileSize ) );

        for( u32 z = 0; z < tileSize; ++z )
        {
            for( u32 x = 0; x < tileSize; ++x )
            {
                squareHeightData[x + z * tileSize] =
                    heightData[x + z * static_cast<u32>( heightMapSize.x )];
            }
        }

        return MeshUtil::getMesh( squareHeightData, 1.0f, getHeightScale(), tileSize );
    }

    void CTerrainOgreNext::_getObject( void **ppObject ) const
    {
        WP_ASSERT( ppObject );
        *ppObject = m_terra;
    }

    void CTerrainOgreNext::setHeightMap( SmartPtr<ITexture> heightMap )
    {
        Terrain::setHeightMap( heightMap );
    }

    void CTerrainOgreNext::updateMaterial()
    {
        ScopedLock lock( this );

        if( !m_terra )
        {
            return;
        }

        auto showWireframe = getShowWireframe();
        if( showWireframe )
        {
            auto root = Ogre::Root::getSingletonPtr();
            WP_ASSERT( root );
            if( !root )
            {
                return;
            }

            auto hlmsManager = root->getHlmsManager();
            WP_ASSERT( hlmsManager );
            if( !hlmsManager )
            {
                return;
            }
            auto hlmsCompute = hlmsManager->getComputeHlms();

            //wireframe
            auto hlms = hlmsManager->getHlms( Ogre::HLMS_USER3 );
            WP_ASSERT( hlms );
            if( !hlms )
            {
                return;
            }
            auto datablock = hlms->getDefaultDatablock();
            WP_ASSERT( datablock );
            if( !datablock )
            {
                return;
            }
            Ogre::HlmsMacroblock macroblock;
            macroblock.mPolygonMode = Ogre::PM_WIREFRAME;
            datablock->setMacroblock( macroblock );

            m_terra->setDatablock( datablock );
        }
        else
        {
            auto root = Ogre::Root::getSingletonPtr();
            WP_ASSERT( root );
            if( !root )
            {
                return;
            }

            auto hlmsManager = root->getHlmsManager();
            WP_ASSERT( hlmsManager );
            if( !hlmsManager )
            {
                return;
            }
            auto hlmsCompute = hlmsManager->getComputeHlms();

            auto materialName = getMaterialName();
            if( materialName.empty() )
            {
                materialName = DefaultTerraMaterialName;
            }

            auto pDatablock = hlmsManager->getDatablock( Ogre::IdString( materialName.c_str() ) );
            WP_ASSERT( pDatablock );
            if( !pDatablock )
            {
                WP_LOG_ERROR( "CTerrainOgreNext::updateMaterial: missing Terra datablock." );
                return;
            }

            auto datablock = dynamic_cast<Ogre::HlmsTerraDatablock *>( pDatablock );
            WP_ASSERT( datablock );
            if( !datablock )
            {
                WP_LOG_ERROR( "CTerrainOgreNext::updateMaterial: invalid Terra datablock." );
                return;
            }

            datablock->setDiffuse( Ogre::Vector3( 1.0f, 1.0f, 1.0f ) );
            for( Ogre::uint8 detailIndex = 0; detailIndex < 4; ++detailIndex )
            {
                datablock->setRoughness( detailIndex, 1.0f );
                datablock->setMetalness( detailIndex, 0.0f );
            }
            datablock->setShadowConstantBias( 0.03f );

            auto creator = datablock->getCreator();
            WP_ASSERT( creator );
            if( !creator )
            {
                WP_LOG_ERROR( "CTerrainOgreNext::updateMaterial: datablock has no creator." );
                return;
            }

            auto renderSystem = creator->getRenderSystem();
            WP_ASSERT( renderSystem );
            if( !renderSystem )
            {
                WP_LOG_ERROR(
                    "CTerrainOgreNext::updateMaterial: datablock creator has no render system." );
                return;
            }

            auto textureManager = renderSystem->getTextureGpuManager();
            WP_ASSERT( textureManager );

            auto textures = getTextures();
            const auto textureCount =
                static_cast<size_t>( Ogre::TerraTextureTypes::NUM_TERRA_TEXTURE_TYPES );
            if( textures.size() != textureCount )
            {
                WP_ASSERT( textures.size() <= textureCount );
                textures.resize( textureCount );
            }

            for( size_t textureIndex = 0; textureIndex < textures.size(); ++textureIndex )
            {
                auto textureType = static_cast<Ogre::TerraTextureTypes>( textureIndex );
                auto texture = textures[textureIndex];
                if( !texture )
                {
                    datablock->setTexture( textureType, nullptr );
                    continue;
                }

                auto ogreNextTexture = workphone::dynamic_pointer_cast<CTextureOgreNext>( texture );
                if( !ogreNextTexture )
                {
                    datablock->setTexture( textureType, nullptr );
                    continue;
                }

                if( !ogreNextTexture->isLoaded() )
                {
                    ogreNextTexture->load( nullptr );
                }

                auto ogreTexture = ogreNextTexture->getTexture();
                WP_ASSERT( ogreTexture );
                if( !ogreTexture )
                {
                    datablock->setTexture( textureType, nullptr );
                    continue;
                }

                datablock->setTexture( textureType, ogreTexture );
            }

            if( m_terra )
            {
                m_terra->setDatablock( datablock );
                WP_ASSERT( datablock );

                auto hlmsPbs = hlmsManager->getHlms( Ogre::HLMS_PBS );
                WP_ASSERT( hlmsPbs );

                if( getCastShadows() )
                {
                    if( !m_hlmsPbsTerraShadows )
                    {
                        m_hlmsPbsTerraShadows = new Ogre::HlmsPbsTerraShadows();
                    }

                    WP_ASSERT( m_hlmsPbsTerraShadows );
                    m_hlmsPbsTerraShadows->setTerra( m_terra );

                    // Set the PBS listener so regular objects also receive terrain shadows
                    if( hlmsPbs )
                    {
                        hlmsPbs->setListener( m_hlmsPbsTerraShadows );
                    }
                }
                else if( m_hlmsPbsTerraShadows )
                {
                    m_hlmsPbsTerraShadows->setTerra( nullptr );
                    if( hlmsPbs )
                    {
                        hlmsPbs->setListener( nullptr );
                    }
                    delete m_hlmsPbsTerraShadows;
                    m_hlmsPbsTerraShadows = nullptr;
                }
            }
        }
    }

    void CTerrainOgreNext::setTerra( Ogre::Terra *terra )
    {
        WP_ASSERT( !terra || !m_terra || terra == m_terra );
        m_terra = terra;
    }

    Ogre::Terra *CTerrainOgreNext::getTerra() const
    {
        return m_terra;
    }

    Vector3F CTerrainOgreNext::getTerrainDimensions() const
    {
        return Vector3F( m_dimensions.x, m_dimensions.y, m_dimensions.z );
    }

    void CTerrainOgreNext::setTerrainDimensions( const Vector3F &dimensions )
    {
        WP_ASSERT( dimensions.isFinite() );
        WP_ASSERT( dimensions.x > 0.0f );
        WP_ASSERT( dimensions.y > 0.0f );
        WP_ASSERT( dimensions.z > 0.0f );

        if( !dimensions.isFinite() || dimensions.x <= 0.0f || dimensions.y <= 0.0f ||
            dimensions.z <= 0.0f )
        {
            WP_LOG_WARNING( "CTerrainOgreNext::setTerrainDimensions: invalid dimensions ignored." );
            return;
        }

        m_dimensions = Ogre::Vector3( dimensions.x, dimensions.y, dimensions.z );
        setHeightScale( dimensions.y );
    }

    Vector3F CTerrainOgreNext::getLightDirection() const
    {
        return Vector3F( m_lightDirection.x, m_lightDirection.y, m_lightDirection.z );
    }

    void CTerrainOgreNext::setLightDirection( const Vector3F &lightDirection )
    {
        WP_ASSERT( lightDirection.isFinite() );
        m_lightDirection = Ogre::Vector3( lightDirection.x, lightDirection.y, lightDirection.z );
        normaliseLightDirection( m_lightDirection, static_cast<Ogre::Real>( getLightEpsilon() ) );
    }

    f32 CTerrainOgreNext::getLightEpsilon() const
    {
        return m_lightEpsilon;
    }

    void CTerrainOgreNext::setLightEpsilon( f32 lightEpsilon )
    {
        WP_ASSERT( MathF::isFinite( lightEpsilon ) );
        m_lightEpsilon = lightEpsilon > 0.0f ? lightEpsilon : 0.0f;
    }

    time_interval CTerrainOgreNext::getUpdateInterval() const
    {
        return m_updateInterval;
    }

    void CTerrainOgreNext::setUpdateInterval( time_interval updateInterval )
    {
        WP_ASSERT( Math<time_interval>::isFinite( updateInterval ) );
        m_updateInterval = updateInterval > 0.0 ? updateInterval : 0.0;
    }

    time_interval CTerrainOgreNext::getInitialUpdateDelay() const
    {
        return m_initialUpdateDelay;
    }

    void CTerrainOgreNext::setInitialUpdateDelay( time_interval initialUpdateDelay )
    {
        WP_ASSERT( Math<time_interval>::isFinite( initialUpdateDelay ) );
        m_initialUpdateDelay = initialUpdateDelay > 0.0 ? initialUpdateDelay : 0.0;
    }

    time_interval CTerrainOgreNext::getNextUpdateTime() const
    {
        return m_nextUpdateTime;
    }

    void CTerrainOgreNext::setNextUpdateTime( time_interval nextUpdateTime )
    {
        WP_ASSERT( Math<time_interval>::isFinite( nextUpdateTime ) );
        m_nextUpdateTime = nextUpdateTime > 0.0 ? nextUpdateTime : 0.0;
    }

    u32 CTerrainOgreNext::getRenderQueueId() const
    {
        return m_renderQueueId;
    }

    void CTerrainOgreNext::setRenderQueueId( u32 renderQueueId )
    {
        WP_ASSERT( renderQueueId < 256u );
        m_renderQueueId = renderQueueId < 256u ? renderQueueId : 255u;
    }

    bool CTerrainOgreNext::getCastShadows() const
    {
        return m_castShadows;
    }

    void CTerrainOgreNext::setCastShadows( bool castShadows )
    {
        m_castShadows = castShadows;

        if( m_terra )
        {
            m_terra->setCastShadows( m_castShadows );
            updateMaterial();
        }
    }

    bool CTerrainOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( message->isExactly<StateMessageVisible>() )
        {
            auto visibleMessage = workphone::static_pointer_cast<StateMessageVisible>( message );
            WP_ASSERT( visibleMessage );

            auto visible = visibleMessage->isVisible();
            auto cascade = visibleMessage->getCascade();

            setVisible( visible );
            return true;
        }

        return false;
    }

    bool CTerrainOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        if( !state )
            return false;

        auto stateData = state->getData();
        if( !stateData )
            return false;

        if( stateData->isExactly<TransformStateData>() )
        {
            auto transformData = workphone::static_pointer_cast<TransformStateData>( stateData );
            WP_ASSERT( transformData );

            const auto &pos = transformData->worldTransform.getPosition();
            const Vector3F newPos( static_cast<f32>( pos.x ), static_cast<f32>( pos.y ),
                                   static_cast<f32>( pos.z ) );
            WP_ASSERT( newPos.isFinite() );

            const auto oldPos = getPosition();
            setPosition( newPos );  // stores into m_center via base implementation
            if( oldPos != newPos && isLoaded() )
            {
                reload( nullptr );
            }

            return true;
        }
        else if( stateData->isExactly<GraphicsObjectData>() )
        {
            auto terrainState = workphone::static_pointer_cast<GraphicsObjectData>( stateData );

            // --- Visibility flag ---
            const auto visible =
                BitUtil::getFlagValue( terrainState->flags, IGraphicsObject::visibleFlag );

            // --- Cast-shadows flag ---
            const auto castShadows =
                BitUtil::getFlagValue( terrainState->flags, IGraphicsObject::castShadowsFlag );

            if( auto terra = getTerra() )
            {
                if( terra->isAttached() )
                {
                    terra->setVisible( visible );
                    terra->setCastShadows( castShadows );
                    //terra->setRenderQueueGroup( static_cast<u8>( terrainState->renderQueueGroup ) );
                    //terra->setVisibilityFlags( terrainState->visibilityMask );
                }
            }

            if( m_castShadows != castShadows )
            {
                m_castShadows = castShadows;
                if( isLoaded() )
                {
                    updateMaterial();
                }
            }

            return true;
        }
        else if( stateData->isDerived<TerrainStateData>() )
        {
            auto terrainState = workphone::static_pointer_cast<TerrainStateData>( stateData );

            bool materialDirty = false;
            bool reloadRequired = false;

            // --- Wireframe toggle (triggers material rebuild) ---
            if( terrainState->showWireframe != m_showWireframe )
            {
                setShowWireframe( terrainState->showWireframe );
                materialDirty = true;
            }

            // --- Material name ---
            if( !terrainState->materialName.empty() && terrainState->materialName != m_materialName )
            {
                setMaterialName( terrainState->materialName );
                materialDirty = true;
            }

            // --- Heightmap size (must be applied before height data / scale) ---
            if( terrainState->heightMapSize != m_heightMapSize )
            {
                auto newSize = terrainState->heightMapSize;
                WP_ASSERT( isValidHeightMapSize( newSize ) );
                if( !isValidHeightMapSize( newSize ) )
                {
                    WP_LOG_WARNING(
                        "CTerrainOgreNext::handleStateChanged: invalid height map size; clamping." );
                    newSize.x = Math<s32>::max( 2, newSize.x );
                    newSize.y = Math<s32>::max( 2, newSize.y );
                }

                setHeightMapSize( newSize );
                m_dimensions.x = static_cast<Ogre::Real>( newSize.x );
                m_dimensions.z = static_cast<Ogre::Real>( newSize.y );

                if( !isValidHeightData( terrainState->heightData, newSize ) )
                {
                    size_t expectedSize = 0;
                    if( getHeightDataElementCount( newSize, expectedSize ) )
                    {
                        auto heightData = terrainState->heightData;
                        heightData.resize( expectedSize, 0.0f );
                        for( auto &heightValue : heightData )
                        {
                            if( !MathF::isFinite( heightValue ) )
                            {
                                heightValue = 0.0f;
                            }
                        }

                        Terrain::setHeightData( heightData );
                        createImageDataFromHeightData();
                    }
                }

                reloadRequired = true;
            }

            // --- Height scale ---
            if( !Math<f32>::equals( terrainState->heightScale, m_heightScale ) )
            {
                WP_ASSERT( MathF::isFinite( terrainState->heightScale ) );
                WP_ASSERT( terrainState->heightScale > 0.0f );
                if( !MathF::isFinite( terrainState->heightScale ) || terrainState->heightScale <= 0.0f )
                {
                    WP_LOG_WARNING(
                        "CTerrainOgreNext::handleStateChanged: invalid height scale; skipping." );
                    return true;
                }

                setHeightScale( terrainState->heightScale );
                m_dimensions.y = static_cast<Ogre::Real>( terrainState->heightScale );
                reloadRequired = true;
            }

            // --- Heightmap texture ---
            if( terrainState->heightMap && terrainState->heightMap != getHeightMap() )
            {
                setHeightMap( terrainState->heightMap );
                reloadRequired = true;
            }

            // --- Raw height data (only if non-empty and correctly sized) ---
            if( !terrainState->heightData.empty() )
            {
                const auto sz = getHeightMapSize();
                if( isValidHeightData( terrainState->heightData, sz ) )
                {
                    Terrain::setHeightData( terrainState->heightData );
                    createImageDataFromHeightData();
                    reloadRequired = true;
                }
                else
                {
                    WP_LOG_WARNING(
                        "CTerrainOgreNext::handleStateChanged: height data size "
                        "mismatch; skipping height update." );
                }
            }

            // --- Textures (triggers material rebuild) ---
            if( !terrainState->textures.empty() )
            {
                auto textures = terrainState->textures;
                resizeTerrainTextures( textures );
                Terrain::setTextures( textures );
                materialDirty = true;
            }

            if( isLoaded() && reloadRequired )
            {
                reload( nullptr );
            }
            else if( materialDirty && isLoaded() )
            {
                updateMaterial();
            }

            return true;
        }

        return false;
    }

    bool CTerrainOgreNext::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto owner = getOwner() )
        {
            return owner->handleStateChanged( state );
        }

        return false;
    }

    bool CTerrainOgreNext::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( auto owner = getOwner() )
        {
            return owner->handleStateMessage( message );
        }

        return false;
    }

    SmartPtr<CTerrainOgreNext> CTerrainOgreNext::StateListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void CTerrainOgreNext::StateListener::setOwner( SmartPtr<CTerrainOgreNext> owner )
    {
        m_owner = owner;
    }

    CTerrainOgreNext::StateListener::StateListener() = default;

    CTerrainOgreNext::StateListener::~StateListener() = default;
}  // namespace workphone::render
