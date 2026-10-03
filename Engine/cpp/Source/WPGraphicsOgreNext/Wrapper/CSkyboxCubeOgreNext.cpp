#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CSkyboxCubeOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreCamera.h>
#include <OgreException.h>
#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgrePass.h>
#include <OgreRectangle2D2.h>
#include <OgreRenderSystem.h>
#include <OgreResourceGroupManager.h>
#include <OgreRoot.h>
#include <OgreSceneManager.h>
#include <OgreStringConverter.h>
#include <OgreTechnique.h>
#include <OgreTextureGpu.h>
#include <OgreTextureGpuManager.h>
#include <OgreTextureUnitState.h>
#include <OgreViewport.h>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CSkyboxCubeOgreNext, SkyboxCube );

    namespace
    {
        constexpr Ogre::uint8 SkyRenderQueueGroup = 212u;
        const Ogre::String SkyCubemapMaterialName = "Ogre/Sky/Cubemap";

        Ogre::TextureGpu *getOgreTextureGpu( const SmartPtr<ITexture> &texture )
        {
            if( !texture )
            {
                return nullptr;
            }

            void *textureObject = nullptr;
            texture->getTextureGPU( &textureObject );
            return static_cast<Ogre::TextureGpu *>( textureObject );
        }

        Ogre::TextureGpu *loadOgreCubemapByName( const String &textureName )
        {
            if( StringUtil::isNullOrEmpty( textureName ) )
            {
                return nullptr;
            }

            auto root = Ogre::Root::getSingletonPtr();
            if( !root )
            {
                return nullptr;
            }

            auto renderSystem = root->getRenderSystem();
            if( !renderSystem )
            {
                return nullptr;
            }

            auto textureManager = renderSystem->getTextureGpuManager();
            if( !textureManager )
            {
                return nullptr;
            }

            auto texture = textureManager->createOrRetrieveTexture(
                textureName.c_str(), Ogre::GpuPageOutStrategy::Discard, Ogre::CommonTextureTypes::EnvMap,
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

            if( texture )
            {
                texture->scheduleTransitionTo( Ogre::GpuResidency::Resident );
            }

            return texture;
        }

        Ogre::Vector3 safeDirection( const Ogre::Vector3 &direction )
        {
            auto result = direction;
            result.normalise();
            return result;
        }
    }  // namespace

    CSkyboxCubeOgreNext::CSkyboxCubeOgreNext()
    {
        setupStateObject();
    }

    CSkyboxCubeOgreNext::~CSkyboxCubeOgreNext()
    {
        unload( nullptr );
        destroyStateObject();
    }

    void CSkyboxCubeOgreNext::load( Ogre::SceneManager *sceneMgr, Ogre::Camera *camera, f32 size )
    {
        unregisterSceneListener();
        destroySky();
        destroySkyMaterial();

        mSceneMgr = sceneMgr;
        mCamera = camera;
        mSize = size;
        mHasRenderableMaterial = false;

        if( !mSceneMgr )
        {
            WP_LOG_ERROR( "CSkyboxCubeOgreNext::load - scene manager is null." );
            return;
        }

        registerSceneListener();

        mSky = mSceneMgr->createRectangle2D( Ogre::SCENE_STATIC );
        mSky->initialize( Ogre::BT_DEFAULT,
                          Ogre::Rectangle2D::GeometryFlagQuad | Ogre::Rectangle2D::GeometryFlagNormals );
        mSky->setGeometry( -Ogre::Vector2::UNIT_SCALE, Ogre::Vector2( 2.0f ) );
        mSky->setRenderQueueGroup( SkyRenderQueueGroup );
        mSky->setVisible( false );

        mSceneMgr->getRootSceneNode( Ogre::SCENE_STATIC )->attachObject( mSky );

        updateSkyFrustum();

        if( auto material = getMaterial() )
        {
            applyMaterial( material );
        }
    }

    void CSkyboxCubeOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            SkyboxCube::load( data );

            auto scene = getScene();
            if( scene )
            {
                scene->_getObject( reinterpret_cast<void **>( &mSceneMgr ) );
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            if( !graphicsSystem->isValid() )
            {
                WP_LOG_ERROR( "Graphics system is not valid." );
                setLoadingState( LoadingState::Error );
                return;
            }

            Ogre::Camera *camera = nullptr;
            if( scene )
            {
                auto graphicsCamera = scene->getActiveCamera();
                if( !graphicsCamera )
                {
                    graphicsCamera = scene->getDefaultCamera();
                }

                if( graphicsCamera )
                {
                    graphicsCamera->_getObject( reinterpret_cast<void **>( &camera ) );
                }
            }

            const auto distance = getDistance();
            load( mSceneMgr, camera, distance > 0.0f ? distance : 5000.0f );

            setLoadingState( LoadingState::Loaded );
        }
        catch( Ogre::Exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Error );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Error );
        }
    }

    void CSkyboxCubeOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            unregisterSceneListener();
            destroySky();
            destroySkyMaterial();

            mSceneMgr = nullptr;
            mCamera = nullptr;
            mSize = 0.0f;
            mHasRenderableMaterial = false;

            SkyboxCube::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
        catch( Ogre::Exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Error );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Error );
        }
    }

    void CSkyboxCubeOgreNext::applyMaterial( SmartPtr<IMaterial> material )
    {
        if( !mSky )
        {
            return;
        }

        if( !material )
        {
            mHasRenderableMaterial = false;
            mSky->setVisible( false );
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        auto textureManager = graphicsSystem->getTextureManager();
        auto resourceDatabase = applicationManager->getResourceDatabasePtr();

        auto cubeTexture = material->getCubeTexture();

        auto textures = material->getCubicTextures();
        if( textures.size() != 6 )
        {
            textures.resize( 6 );
        }

        if( m_textures.size() != 6 )
        {
            m_textures.resize( 6 );
        }

        auto dirty = !cubeTexture ||
                     !std::equal( m_textures.begin(), m_textures.end(), textures.begin(),
                                  []( const SmartPtr<ITexture> &a, const SmartPtr<ITexture> &b ) {
                                      // Both are null.
                                      if( !a && !b )
                                      {
                                          return true;
                                      }

                                      // Only one is null.
                                      if( !a || !b )
                                      {
                                          return false;
                                      }

                                      return a == b;
                                  } );

        if( dirty )
        {
            // Cache the requested faces, before substituting defaults, so missing faces
            // do not cause the same cubemap to be rebuilt on every state update.
            const auto sourceTextures = textures;
            static const String defaultSkyboxTextureName = "checker.png";
            for( auto &texture : textures )
            {
                if( !texture )
                {
                    texture = resourceDatabase->loadResourceByType<render::ITexture>(
                        defaultSkyboxTextureName );
                }
            }

            // Skybox stores faces as Front, Back, Left, Right, Up, Down, while Ogre
            // expects cubemap slices in +X, -X, +Y, -Y, +Z, -Z order.
            Array<SmartPtr<ITexture>> ogreTextures( 6 );
            ogreTextures[0] = textures[static_cast<u32>( SkyboxTextureTypes::Right )];
            ogreTextures[1] = textures[static_cast<u32>( SkyboxTextureTypes::Left )];
            ogreTextures[2] = textures[static_cast<u32>( SkyboxTextureTypes::Up )];
            ogreTextures[3] = textures[static_cast<u32>( SkyboxTextureTypes::Down )];
            ogreTextures[4] = textures[static_cast<u32>( SkyboxTextureTypes::Front )];
            ogreTextures[5] = textures[static_cast<u32>( SkyboxTextureTypes::Back )];

            if( auto generatedCubeTexture = textureManager->createCubeMap( ogreTextures ) )
            {
                cubeTexture = generatedCubeTexture;
                m_textures = sourceTextures;
                material->setCubeTexture( cubeTexture );
            }
            // Leave the cache unchanged on failure so the next material update retries.
        }

        if( cubeTexture )
        {
            mHasRenderableMaterial = bindCubeTexture( cubeTexture );
            mSky->setVisible( isVisible() && mHasRenderableMaterial );
            return;
        }

        const auto &materialName = material->getName();
        if( !StringUtil::isNullOrEmpty( materialName ) )
        {
            auto ogreMaterialName = Ogre::String( materialName.c_str(), materialName.length() );
            auto ogreMaterial = Ogre::MaterialManager::getSingleton().getByName(
                ogreMaterialName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
            if( ogreMaterial )
            {
                mSky->setMaterial( ogreMaterial );
                mHasRenderableMaterial = true;
                mSky->setVisible( isVisible() );
                updateSkyFrustum();
                return;
            }
        }

        mHasRenderableMaterial = false;
        mSky->setVisible( false );
    }

    void CSkyboxCubeOgreNext::applyState( SmartPtr<SkyStateData> skyStateData )
    {
        if( !skyStateData )
        {
            return;
        }

        if( skyStateData->distance > 0.0f )
        {
            mSize = skyStateData->distance;
        }

        applyMaterial( skyStateData->material );

        if( mSky )
        {
            mSky->setVisible( skyStateData->visible && mHasRenderableMaterial );
            updateSkyFrustum();
        }
    }

    void CSkyboxCubeOgreNext::update()
    {
        updateSkyFrustum();
    }

    bool CSkyboxCubeOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( SkyboxCube::handleStateMessage( message ) )
        {
            return true;
        }

        return false;
    }

    bool CSkyboxCubeOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        if( !isLoaded() )
        {
            return false;
        }

        if( state && state->getOwnerPtr() == this )
        {
            if( auto stateData = state->getData() )
            {
                if( stateData->isDerived<SkyStateData>() )
                {
                    auto skyStateData = workphone::static_pointer_cast<SkyStateData>( stateData );
                    applyState( skyStateData );

                    auto &material = skyStateData->material;
                    applyMaterial( material );

                    auto visible = isVisible();
                    if( mSky )
                    {
                        mSky->setVisible( visible && mHasRenderableMaterial );
                        updateSkyFrustum();
                    }

                    // work around for now, since the above code is not working as expected. The skybox is not updating correctly when the state changes.
                    //mSceneMgr->setSky( true, mSkyMaterial, mSize, SkyRenderQueueGroup );

                    return true;
                }
            }
        }

        return false;
    }

    const Ogre::Camera *CSkyboxCubeOgreNext::getActiveCamera() const
    {
        auto scene = getScene();
        if( scene )
        {
            auto graphicsCamera = scene->getActiveCamera();
            if( !graphicsCamera )
            {
                graphicsCamera = scene->getDefaultCamera();
            }

            if( graphicsCamera )
            {
                Ogre::Camera *camera = nullptr;
                graphicsCamera->_getObject( reinterpret_cast<void **>( &camera ) );

                if( camera )
                {
                    return camera;
                }
            }
        }

        return mCamera;
    }

    void CSkyboxCubeOgreNext::updateSkyFrustum()
    {
        updateSkyFrustum( getActiveCamera() );
    }

    void CSkyboxCubeOgreNext::updateSkyFrustum( const Ogre::Camera *camera )
    {
        if( !mSky )
        {
            return;
        }

        if( camera )
        {
            // OgreNext derives the camera transform through its parent scene node.
            if( !camera->isAttached() )
            {
                return;
            }

            const auto *corners = camera->getWorldSpaceCorners();
            const auto &cameraPos = camera->getDerivedPosition();

            // Near-plane corners also work with an infinite far clip (distance zero).
            const auto invNearPlane = Ogre::Real( 1.0 ) / camera->getNearClipDistance();
            Ogre::Vector3 cameraDirs[4];
            cameraDirs[0] = ( corners[1] - cameraPos ) * invNearPlane;
            cameraDirs[1] = ( corners[2] - cameraPos ) * invNearPlane;
            cameraDirs[2] = ( corners[0] - cameraPos ) * invNearPlane;
            cameraDirs[3] = ( corners[3] - cameraPos ) * invNearPlane;

            mSky->setNormals( cameraDirs[0], cameraDirs[1], cameraDirs[2], cameraDirs[3] );
            mSky->update();
            return;
        }

        mSky->setNormals( safeDirection( Ogre::Vector3( -1.0f, 1.0f, -1.0f ) ),
                          safeDirection( Ogre::Vector3( -1.0f, -1.0f, -1.0f ) ),
                          safeDirection( Ogre::Vector3( 1.0f, 1.0f, -1.0f ) ),
                          safeDirection( Ogre::Vector3( 1.0f, -1.0f, -1.0f ) ) );
        mSky->update();
    }

    void CSkyboxCubeOgreNext::preFindVisibleObjects( Ogre::SceneManager *source,
                                                     Ogre::SceneManager::IlluminationRenderStage irs,
                                                     Ogre::Viewport *viewport )
    {
        WP_UNUSED( viewport );

        if( source != mSceneMgr || irs == Ogre::SceneManager::IRS_RENDER_TO_TEXTURE )
        {
            return;
        }

        const auto cameras = source->getCamerasInProgress();
        updateSkyFrustum( cameras.renderingCamera );
    }

    void CSkyboxCubeOgreNext::destroySky()
    {
        if( !mSky )
        {
            return;
        }

        if( mSceneMgr )
        {
            mSky->detachFromParent();
            mSceneMgr->destroyRectangle2D( mSky );
        }

        mSky = nullptr;
    }

    void CSkyboxCubeOgreNext::destroySkyMaterial()
    {
        if( mSkyMaterial )
        {
            if( mOwnsSkyMaterial )
            {
                if( auto materialManager = Ogre::MaterialManager::getSingletonPtr() )
                {
                    materialManager->remove( mSkyMaterial );
                }
            }

            mSkyMaterial.reset();
        }

        mOwnsSkyMaterial = false;
    }

    void CSkyboxCubeOgreNext::registerSceneListener()
    {
        if( mSceneMgr && !mSceneListenerRegistered )
        {
            mSceneMgr->addListener( this );
            mSceneListenerRegistered = true;
        }
    }

    void CSkyboxCubeOgreNext::unregisterSceneListener()
    {
        if( mSceneMgr && mSceneListenerRegistered )
        {
            mSceneMgr->removeListener( this );
        }

        mSceneListenerRegistered = false;
    }

    Ogre::MaterialPtr CSkyboxCubeOgreNext::getOrCreateSkyMaterial()
    {
        if( mSkyMaterial )
        {
            return mSkyMaterial;
        }

        if( !mSceneMgr )
        {
            return Ogre::MaterialPtr();
        }

        auto &materialManager = Ogre::MaterialManager::getSingleton();
        const auto materialName =
            SkyCubemapMaterialName + "/Workphone" +
            Ogre::StringConverter::toString( static_cast<Ogre::uint64>( mSceneMgr->getId() ) ) + "_" +
            Ogre::StringConverter::toString( static_cast<Ogre::uint64>( getId() ) );

        mSkyMaterial = materialManager.getByName( materialName );
        if( !mSkyMaterial )
        {
            auto baseMaterial = materialManager.getByName(
                SkyCubemapMaterialName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

            if( !baseMaterial )
            {
                WP_LOG_ERROR(
                    "CSkyboxCubeOgreNext requires Ogre/Sky/Cubemap. Make sure the Ogre sky resources "
                    "are loaded." );
                return Ogre::MaterialPtr();
            }

            baseMaterial->load();
            mSkyMaterial = baseMaterial->clone( materialName );
        }

        mOwnsSkyMaterial = true;
        return mSkyMaterial;
    }

    bool CSkyboxCubeOgreNext::bindCubeTexture( SmartPtr<ITexture> cubeTexture )
    {
        if( !mSky || !cubeTexture )
        {
            return false;
        }

        auto texture = getOgreTextureGpu( cubeTexture );
        if( !texture )
        {
            texture = loadOgreCubemapByName( cubeTexture->getName() );
        }

        if( !texture )
        {
            WP_LOG_ERROR( "CSkyboxCubeOgreNext could not resolve the cubemap texture." );
            return false;
        }

        if( texture->getTextureType() != Ogre::TextureTypes::TypeCube )
        {
            WP_LOG_ERROR( "CSkyboxCubeOgreNext requires an Ogre TypeCube texture." );
            return false;
        }

        auto skyMaterial = getOrCreateSkyMaterial();
        if( !skyMaterial )
        {
            return false;
        }

        auto technique = skyMaterial->getTechnique( 0 );
        if( !technique || technique->getNumPasses() == 0 )
        {
            WP_LOG_ERROR( "CSkyboxCubeOgreNext sky material has no render pass." );
            return false;
        }

        auto pass = technique->getPass( 0 );
        if( !pass || pass->getNumTextureUnitStates() == 0 )
        {
            WP_LOG_ERROR( "CSkyboxCubeOgreNext sky material has no texture unit." );
            return false;
        }

        auto textureUnit = pass->getTextureUnitState( 0 );
        textureUnit->setAutomaticBatching( texture->hasAutomaticBatching() );
        textureUnit->setTexture( texture );

        mSky->setMaterial( skyMaterial );
        updateSkyFrustum();
        return true;
    }

    void CSkyboxCubeOgreNext::setupStateObject()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManagerPtr();
        WP_ASSERT( stateManager );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = graphicsSystem->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto stateContext = graphicsSystem->getStateContext();
        WP_ASSERT( stateContext );
        setStateContext( stateContext );

        auto state = factoryManager->make_ptr<State>();
        state->setId( getId() );
        state->setOwner( this );
        stateContext->addState( state );

        auto stateData = factoryManager->make_ptr<SkyStateData>();
        state->setData( stateData );
    }

    void CSkyboxCubeOgreNext::destroyStateObject()
    {
        if( auto stateContext = getStateContext() )
        {
            stateContext->removeStatesById( getId() );
        }
    }

}  // namespace workphone::render
