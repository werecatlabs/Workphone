#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CSkybox6SidedOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreCamera.h>
#include <OgreException.h>
#include <OgreHlms.h>
#include <OgreHlmsDatablock.h>
#include <OgreHlmsManager.h>
#include <OgreHlmsSamplerblock.h>
#include <OgreHlmsUnlit.h>
#include <OgreHlmsUnlitDatablock.h>
#include <OgreItem.h>
#include <OgreManualObject2.h>
#include <OgreMaterialManager.h>
#include <OgreMesh2.h>
#include <OgreMeshManager2.h>
#include <OgreResourceGroupManager.h>
#include <OgreRoot.h>
#include <OgreSceneNode.h>
#include <OgreStringConverter.h>
#include <OgreTextureGpu.h>
#include <OgreTextureGpuManager.h>
#include <OgreViewport.h>
#include <cmath>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CSkybox6SidedOgreNext, Skybox );

    namespace
    {
        constexpr Ogre::uint8 SkyRenderQueueGroup = 212u;
        const Ogre::String PlaceholderMaterialName = "BaseWhiteNoLighting";

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

        Ogre::TextureGpu *resolveOgreTextureGpu( const SmartPtr<ITexture> &texture )
        {
            if( !texture )
            {
                return nullptr;
            }

            if( auto ogreTexture = getOgreTextureGpu( texture ) )
            {
                return ogreTexture;
            }

            const auto &textureName = texture->getName();
            if( StringUtil::isNullOrEmpty( textureName ) )
            {
                return nullptr;
            }

            auto root = Ogre::Root::getSingletonPtr();
            auto renderSystem = root ? root->getRenderSystem() : nullptr;
            auto textureManager = renderSystem ? renderSystem->getTextureGpuManager() : nullptr;
            if( !textureManager )
            {
                return nullptr;
            }

            auto ogreTexture = textureManager->createOrRetrieveTexture(
                textureName.c_str(), Ogre::GpuPageOutStrategy::Discard,
                Ogre::CommonTextureTypes::Diffuse,
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
            if( ogreTexture )
            {
                ogreTexture->scheduleTransitionTo( Ogre::GpuResidency::Resident );
            }

            return ogreTexture;
        }
    }  // namespace

    CSkybox6SidedOgreNext::CSkybox6SidedOgreNext()
    {
        setupStateObject();
    }

    CSkybox6SidedOgreNext::~CSkybox6SidedOgreNext()
    {
        unload( nullptr );
        destroyStateObject();
    }

    void CSkybox6SidedOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            Skybox::load( data );

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto graphicsSystem = applicationManager ? applicationManager->getGraphicsSystemPtr() : nullptr;
            if( !graphicsSystem || !graphicsSystem->isValid() )
            {
                WP_LOG_ERROR( "CSkybox6SidedOgreNext::load - graphics system is not valid." );
                setLoadingState( LoadingState::Error );
                return;
            }

            auto scene = getScene();
            Ogre::SceneManager *sceneMgr = nullptr;
            Ogre::Camera *camera = nullptr;
            if( scene )
            {
                scene->_getObject( reinterpret_cast<void **>( &sceneMgr ) );

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
            const auto size = distance > 0.0f && std::isfinite( distance ) ? distance : 5000.0f;
            if( !load( sceneMgr, camera, size ) )
            {
                destroyOgreObjects();
                mSceneMgr = nullptr;
                mCamera = nullptr;
                mSize = 0.0f;
                setLoadingState( LoadingState::Error );
                return;
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( Ogre::Exception &e )
        {
            WP_LOG_EXCEPTION( e );
            destroyOgreObjects();
            setLoadingState( LoadingState::Error );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            destroyOgreObjects();
            setLoadingState( LoadingState::Error );
        }
    }

    bool CSkybox6SidedOgreNext::load( Ogre::SceneManager *sceneMgr, Ogre::Camera *camera, f32 size )
    {
        destroyOgreObjects();

        mSceneMgr = sceneMgr;
        mCamera = camera;
        mSize = size;

        if( !mSceneMgr )
        {
            WP_LOG_ERROR( "CSkybox6SidedOgreNext::load - scene manager is null." );
            return false;
        }

        if( !( mSize > 0.0f ) || !std::isfinite( mSize ) )
        {
            WP_LOG_ERROR( "CSkybox6SidedOgreNext::load - skybox size must be finite and positive." );
            return false;
        }

        registerSceneListener();
        mRootNode = mSceneMgr->getRootSceneNode( Ogre::SCENE_DYNAMIC )->createChildSceneNode(
            Ogre::SCENE_DYNAMIC );
        if( !mRootNode )
        {
            WP_LOG_ERROR( "CSkybox6SidedOgreNext::load - failed to create the root scene node." );
            return false;
        }

        for( size_t i = 0u; i < NumFaces; ++i )
        {
            if( !createPlane( static_cast<Face>( i ) ) )
            {
                WP_LOG_ERROR( "CSkybox6SidedOgreNext::load - failed to create a skybox face." );
                return false;
            }
        }

        const auto textures = getTextures();
        for( size_t i = 0u; i < NumFaces; ++i )
        {
            mFaceTextures[i] = i < textures.size() ? textures[i] : nullptr;
        }

        const auto distance = getDistance();
        const auto scale = distance > 0.0f && std::isfinite( distance ) ? distance / mSize : 1.0f;
        mRootNode->setScale( Ogre::Vector3( scale ) );
        updatePosition( getActiveCamera() );
        refreshMaterials();
        refreshVisibility();
        return true;
    }

    void CSkybox6SidedOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            destroyOgreObjects();
            mSceneMgr = nullptr;
            mCamera = nullptr;
            mSize = 0.0f;

            Skybox::unload( data );
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

    void CSkybox6SidedOgreNext::setMaterial( Face face, const String &materialName )
    {
        const auto faceIndex = static_cast<size_t>( face );
        if( faceIndex >= NumFaces )
        {
            WP_LOG_ERROR( "CSkybox6SidedOgreNext::setMaterial - invalid face index." );
            return;
        }

        mFaceMaterialNames[faceIndex] = materialName;
        if( mFaces[faceIndex] )
        {
            mFaceHasRenderableMaterial[faceIndex] = applyFaceMaterial( face, getMaterial() );
            refreshVisibility();
        }
    }

    void CSkybox6SidedOgreNext::update()
    {
        updatePosition( getActiveCamera() );

        // A texture wrapper can receive its Ogre texture after the skybox state was applied.
        // Rebind only when the underlying GPU object changes.
        for( size_t i = 0u; i < NumFaces; ++i )
        {
            if( StringUtil::isNullOrEmpty( mFaceMaterialNames[i] ) && mFaceTextures[i] )
            {
                auto texture = resolveOgreTextureGpu( mFaceTextures[i] );
                if( texture != mBoundFaceTextures[i] )
                {
                    mFaceHasRenderableMaterial[i] =
                        applyFaceTexture( static_cast<Face>( i ), mFaceTextures[i] );
                }
            }
        }

        refreshVisibility();
    }

    bool CSkybox6SidedOgreNext::createPlane( Face face )
    {
        if( !mSceneMgr || !mRootNode )
        {
            return false;
        }

        const auto faceIndex = static_cast<size_t>( face );
        if( faceIndex >= NumFaces )
        {
            return false;
        }

        const auto h = static_cast<Ogre::Real>( mSize * 0.5f );
        Ogre::Vector3 normal = Ogre::Vector3::ZERO;
        Ogre::Vector3 corners[4];

        switch( face )
        {
        case Front:
            normal = Ogre::Vector3::NEGATIVE_UNIT_Z;
            corners[0] = Ogre::Vector3( -h, -h, h );
            corners[1] = Ogre::Vector3( h, -h, h );
            corners[2] = Ogre::Vector3( h, h, h );
            corners[3] = Ogre::Vector3( -h, h, h );
            break;
        case Back:
            normal = Ogre::Vector3::UNIT_Z;
            corners[0] = Ogre::Vector3( h, -h, -h );
            corners[1] = Ogre::Vector3( -h, -h, -h );
            corners[2] = Ogre::Vector3( -h, h, -h );
            corners[3] = Ogre::Vector3( h, h, -h );
            break;
        case Left:
            normal = Ogre::Vector3::UNIT_X;
            corners[0] = Ogre::Vector3( -h, -h, -h );
            corners[1] = Ogre::Vector3( -h, -h, h );
            corners[2] = Ogre::Vector3( -h, h, h );
            corners[3] = Ogre::Vector3( -h, h, -h );
            break;
        case Right:
            normal = Ogre::Vector3::NEGATIVE_UNIT_X;
            corners[0] = Ogre::Vector3( h, -h, h );
            corners[1] = Ogre::Vector3( h, -h, -h );
            corners[2] = Ogre::Vector3( h, h, -h );
            corners[3] = Ogre::Vector3( h, h, h );
            break;
        case Top:
            normal = Ogre::Vector3::NEGATIVE_UNIT_Y;
            corners[0] = Ogre::Vector3( -h, h, h );
            corners[1] = Ogre::Vector3( h, h, h );
            corners[2] = Ogre::Vector3( h, h, -h );
            corners[3] = Ogre::Vector3( -h, h, -h );
            break;
        case Bottom:
            normal = Ogre::Vector3::UNIT_Y;
            corners[0] = Ogre::Vector3( -h, -h, -h );
            corners[1] = Ogre::Vector3( h, -h, -h );
            corners[2] = Ogre::Vector3( h, -h, h );
            corners[3] = Ogre::Vector3( -h, -h, h );
            break;
        default:
            return false;
        }

        auto meshManager = Ogre::MeshManager::getSingletonPtr();
        if( !meshManager )
        {
            WP_LOG_ERROR( "CSkybox6SidedOgreNext::createPlane - mesh manager is unavailable." );
            return false;
        }

        const auto meshName =
            Ogre::String( "Workphone/Skybox6Sided/" ) +
            Ogre::StringConverter::toString( static_cast<Ogre::uint64>( mSceneMgr->getId() ) ) + "/" +
            Ogre::StringConverter::toString( static_cast<Ogre::uint64>( getId() ) ) + "/" +
            Ogre::StringConverter::toString( static_cast<Ogre::uint32>( faceIndex ) );
        mFaceMeshNames[faceIndex] = meshName.c_str();

        if( meshManager->resourceExists( meshName ) )
        {
            meshManager->remove( meshName );
        }

        Ogre::ManualObject *manualObject = nullptr;
        try
        {
            manualObject = mSceneMgr->createManualObject( Ogre::SCENE_DYNAMIC );
            if( !manualObject )
            {
                return false;
            }

            manualObject->begin( PlaceholderMaterialName, Ogre::OT_TRIANGLE_LIST );
            const Ogre::Vector2 uvs[4] = {
                Ogre::Vector2( 0.0f, 1.0f ), Ogre::Vector2( 1.0f, 1.0f ),
                Ogre::Vector2( 1.0f, 0.0f ), Ogre::Vector2( 0.0f, 0.0f )
            };

            for( size_t i = 0u; i < 4u; ++i )
            {
                manualObject->position( corners[i] );
                manualObject->normal( normal );
                manualObject->textureCoord( uvs[i] );
            }

            manualObject->triangle( 0u, 2u, 1u );
            manualObject->triangle( 0u, 3u, 2u );
            manualObject->end();

            auto mesh = manualObject->convertToMesh(
                meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, false );
            mSceneMgr->destroyManualObject( manualObject );
            manualObject = nullptr;

            if( !mesh )
            {
                return false;
            }

            auto item = mSceneMgr->createItem( mesh, Ogre::SCENE_DYNAMIC );
            auto node = mRootNode->createChildSceneNode( Ogre::SCENE_DYNAMIC );
            if( !item || !node )
            {
                if( item )
                {
                    mSceneMgr->destroyItem( item );
                }
                if( node )
                {
                    mSceneMgr->destroySceneNode( node );
                }
                return false;
            }

            item->setCastShadows( false );
            item->setRenderQueueGroup( SkyRenderQueueGroup );
            item->setVisible( false );
            node->attachObject( item );

            mFaces[faceIndex] = item;
            mFaceNodes[faceIndex] = node;
            return true;
        }
        catch( ... )
        {
            if( manualObject && mSceneMgr )
            {
                mSceneMgr->destroyManualObject( manualObject );
            }
            throw;
        }
    }

    void CSkybox6SidedOgreNext::destroyOgreObjects()
    {
        unregisterSceneListener();

        if( mSceneMgr )
        {
            for( size_t i = 0u; i < NumFaces; ++i )
            {
                if( mFaces[i] )
                {
                    mFaces[i]->detachFromParent();
                    mSceneMgr->destroyItem( mFaces[i] );
                }
                mFaces[i] = nullptr;

                if( mFaceNodes[i] )
                {
                    mSceneMgr->destroySceneNode( mFaceNodes[i] );
                }
                mFaceNodes[i] = nullptr;
            }

            if( mRootNode )
            {
                mSceneMgr->destroySceneNode( mRootNode );
            }
        }
        else
        {
            mFaces.fill( nullptr );
            mFaceNodes.fill( nullptr );
        }
        mRootNode = nullptr;

        destroyTextureDatablocks();

        if( auto meshManager = Ogre::MeshManager::getSingletonPtr() )
        {
            for( auto &meshName : mFaceMeshNames )
            {
                if( !StringUtil::isNullOrEmpty( meshName ) &&
                    meshManager->resourceExists( meshName.c_str() ) )
                {
                    meshManager->remove( meshName.c_str() );
                }
                meshName.clear();
            }
        }
        else
        {
            mFaceMeshNames.fill( String() );
        }

        mFaceTextures.fill( nullptr );
        mBoundFaceTextures.fill( nullptr );
        mFaceHasRenderableMaterial.fill( false );
    }

    void CSkybox6SidedOgreNext::destroyTextureDatablocks()
    {
        auto root = Ogre::Root::getSingletonPtr();
        auto hlmsManager = root ? root->getHlmsManager() : nullptr;
        auto hlmsUnlit = hlmsManager ? hlmsManager->getHlms( Ogre::HLMS_UNLIT ) : nullptr;

        for( auto &datablockName : mTextureDatablockNames )
        {
            if( hlmsManager && hlmsUnlit && !StringUtil::isNullOrEmpty( datablockName ) )
            {
                auto datablock = hlmsManager->getDatablockNoDefault( datablockName.c_str() );
                if( datablock && datablock->getCreator() == hlmsUnlit )
                {
                    hlmsUnlit->destroyDatablock( Ogre::IdString( datablockName.c_str() ) );
                }
            }
            datablockName.clear();
        }
    }

    void CSkybox6SidedOgreNext::applyMaterial( SmartPtr<IMaterial> material )
    {
        for( size_t i = 0u; i < NumFaces; ++i )
        {
            mFaceHasRenderableMaterial[i] =
                applyFaceMaterial( static_cast<Face>( i ), material );
        }
        refreshVisibility();
    }

    bool CSkybox6SidedOgreNext::applyFaceMaterial( Face face,
                                                   const SmartPtr<IMaterial> &material )
    {
        const auto faceIndex = static_cast<size_t>( face );
        if( faceIndex >= NumFaces || !mFaces[faceIndex] )
        {
            return false;
        }

        if( !StringUtil::isNullOrEmpty( mFaceMaterialNames[faceIndex] ) )
        {
            return applyNamedMaterial( face, mFaceMaterialNames[faceIndex] );
        }

        if( mFaceTextures[faceIndex] )
        {
            return applyFaceTexture( face, mFaceTextures[faceIndex] );
        }

        if( auto ogreMaterial = workphone::dynamic_pointer_cast<CMaterialOgreNext>( material ) )
        {
            if( auto datablock = ogreMaterial->getHlmsDatablock() )
            {
                mFaces[faceIndex]->setDatablock( datablock );
                mBoundFaceTextures[faceIndex] = nullptr;
                return true;
            }

            const auto &datablockName = ogreMaterial->getDatablockName();
            if( !StringUtil::isNullOrEmpty( datablockName ) )
            {
                return applyNamedMaterial( face, datablockName );
            }
        }

        if( material && !StringUtil::isNullOrEmpty( material->getName() ) )
        {
            return applyNamedMaterial( face, material->getName() );
        }

        mFaces[faceIndex]->setVisible( false );
        mBoundFaceTextures[faceIndex] = nullptr;
        return false;
    }

    bool CSkybox6SidedOgreNext::applyNamedMaterial( Face face, const String &materialName )
    {
        const auto faceIndex = static_cast<size_t>( face );
        if( faceIndex >= NumFaces || !mFaces[faceIndex] ||
            StringUtil::isNullOrEmpty( materialName ) )
        {
            return false;
        }

        auto root = Ogre::Root::getSingletonPtr();
        auto hlmsManager = root ? root->getHlmsManager() : nullptr;
        if( hlmsManager )
        {
            if( auto datablock = hlmsManager->getDatablockNoDefault( materialName.c_str() ) )
            {
                mFaces[faceIndex]->setDatablock( datablock );
                mBoundFaceTextures[faceIndex] = nullptr;
                return true;
            }
        }

        if( auto materialManager = Ogre::MaterialManager::getSingletonPtr() )
        {
            auto ogreMaterial = materialManager->getByName(
                materialName.c_str(), Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
            if( ogreMaterial )
            {
                mFaces[faceIndex]->setDatablockOrMaterialName(
                    materialName.c_str(), Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
                mBoundFaceTextures[faceIndex] = nullptr;
                return true;
            }
        }

        WP_LOG_ERROR( "CSkybox6SidedOgreNext could not find material: " + materialName );
        mFaces[faceIndex]->setVisible( false );
        return false;
    }

    bool CSkybox6SidedOgreNext::applyFaceTexture( Face face, const SmartPtr<ITexture> &texture )
    {
        const auto faceIndex = static_cast<size_t>( face );
        if( faceIndex >= NumFaces || !mFaces[faceIndex] || !texture )
        {
            return false;
        }

        auto ogreTexture = resolveOgreTextureGpu( texture );
        if( !ogreTexture )
        {
            mBoundFaceTextures[faceIndex] = nullptr;
            return false;
        }

        if( ogreTexture->getTextureType() != Ogre::TextureTypes::Type2D )
        {
            WP_LOG_ERROR( "CSkybox6SidedOgreNext requires a 2D texture for each face." );
            mBoundFaceTextures[faceIndex] = nullptr;
            return false;
        }

        auto root = Ogre::Root::getSingletonPtr();
        auto hlmsManager = root ? root->getHlmsManager() : nullptr;
        auto hlmsUnlit = hlmsManager
                             ? dynamic_cast<Ogre::HlmsUnlit *>(
                                   hlmsManager->getHlms( Ogre::HLMS_UNLIT ) )
                             : nullptr;
        if( !hlmsManager || !hlmsUnlit )
        {
            WP_LOG_ERROR( "CSkybox6SidedOgreNext requires the Ogre HLMS Unlit implementation." );
            return false;
        }

        auto &datablockName = mTextureDatablockNames[faceIndex];
        if( StringUtil::isNullOrEmpty( datablockName ) )
        {
            const auto sceneId = mSceneMgr ? static_cast<u64>( mSceneMgr->getId() ) : 0u;
            datablockName = "Workphone/Skybox6Sided/Material/" + StringUtil::toString( sceneId ) +
                            "/" + StringUtil::toString( getId() ) + "/" +
                            StringUtil::toString( static_cast<u32>( faceIndex ) );
        }

        auto datablock = dynamic_cast<Ogre::HlmsUnlitDatablock *>(
            hlmsManager->getDatablockNoDefault( datablockName.c_str() ) );
        if( !datablock )
        {
            Ogre::HlmsMacroblock macroblock;
            macroblock.mDepthWrite = false;
            macroblock.mCullMode = Ogre::CULL_NONE;

            Ogre::HlmsBlendblock blendblock;
            Ogre::HlmsParamVec params;
            datablock = static_cast<Ogre::HlmsUnlitDatablock *>( hlmsUnlit->createDatablock(
                datablockName.c_str(), datablockName.c_str(), macroblock, blendblock, params ) );
        }

        if( !datablock )
        {
            return false;
        }

        Ogre::HlmsSamplerblock samplerblock;
        samplerblock.mU = Ogre::TAM_CLAMP;
        samplerblock.mV = Ogre::TAM_CLAMP;
        samplerblock.mW = Ogre::TAM_CLAMP;
        datablock->setTexture( 0u, ogreTexture, &samplerblock );
        mFaces[faceIndex]->setDatablock( datablock );
        mBoundFaceTextures[faceIndex] = ogreTexture;
        return true;
    }

    void CSkybox6SidedOgreNext::refreshMaterials()
    {
        applyMaterial( getMaterial() );
    }

    void CSkybox6SidedOgreNext::refreshVisibility()
    {
        if( !mRootNode )
        {
            return;
        }

        const auto visible = isVisible();
        mRootNode->setVisible( visible );
        for( size_t i = 0u; i < NumFaces; ++i )
        {
            if( mFaces[i] )
            {
                mFaces[i]->setVisible( visible && mFaceHasRenderableMaterial[i] );
            }
        }
    }

    void CSkybox6SidedOgreNext::applyState( SmartPtr<SkyStateData> skyStateData )
    {
        if( !skyStateData )
        {
            return;
        }

        for( size_t i = 0u; i < NumFaces; ++i )
        {
            mFaceTextures[i] = skyStateData->textures[i];
        }

        if( mRootNode )
        {
            const auto scale = skyStateData->distance > 0.0f &&
                                       std::isfinite( skyStateData->distance ) && mSize > 0.0f
                                   ? skyStateData->distance / mSize
                                   : 1.0f;
            mRootNode->setScale( Ogre::Vector3( scale ) );
        }

        applyMaterial( skyStateData->material );
        updatePosition( getActiveCamera() );
        refreshVisibility();
    }

    bool CSkybox6SidedOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        return Skybox::handleStateMessage( message );
    }

    bool CSkybox6SidedOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        if( !isLoaded() || !state || state->getOwnerPtr() != this )
        {
            return false;
        }

        if( auto stateData = state->getData() )
        {
            if( mRootNode )
            {
                refreshVisibility();
            }

            if( stateData->isDerived<SkyStateData>() )
            {
                applyState( workphone::static_pointer_cast<SkyStateData>( stateData ) );
                return true;
            }
        }

        return false;
    }

    const Ogre::Camera *CSkybox6SidedOgreNext::getActiveCamera() const
    {
        if( auto scene = getScene() )
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

    void CSkybox6SidedOgreNext::updatePosition( const Ogre::Camera *camera )
    {
        if( mRootNode && camera && camera->isAttached() )
        {
            mRootNode->setPosition( camera->getDerivedPosition() );
        }
    }

    void CSkybox6SidedOgreNext::registerSceneListener()
    {
        if( mSceneMgr && !mSceneListenerRegistered )
        {
            mSceneMgr->addListener( this );
            mSceneListenerRegistered = true;
        }
    }

    void CSkybox6SidedOgreNext::unregisterSceneListener()
    {
        if( mSceneMgr && mSceneListenerRegistered )
        {
            mSceneMgr->removeListener( this );
        }
        mSceneListenerRegistered = false;
    }

    void CSkybox6SidedOgreNext::preFindVisibleObjects(
        Ogre::SceneManager *source, Ogre::SceneManager::IlluminationRenderStage irs,
        Ogre::Viewport *viewport )
    {
        WP_UNUSED( viewport );

        if( source == mSceneMgr && irs != Ogre::SceneManager::IRS_RENDER_TO_TEXTURE )
        {
            const auto cameras = source->getCamerasInProgress();
            updatePosition( cameras.renderingCamera );
        }
    }

    void CSkybox6SidedOgreNext::setupStateObject()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager ? applicationManager->getGraphicsSystemPtr() : nullptr;
        auto factoryManager = graphicsSystem ? graphicsSystem->getFactoryManagerPtr() : nullptr;
        auto stateContext = graphicsSystem ? graphicsSystem->getStateContext() : nullptr;
        if( !factoryManager || !stateContext )
        {
            WP_LOG_ERROR( "CSkybox6SidedOgreNext::setupStateObject - graphics state is unavailable." );
            return;
        }

        setStateContext( stateContext );

        auto state = factoryManager->make_ptr<State>();
        auto stateData = factoryManager->make_ptr<SkyStateData>();
        if( !state || !stateData )
        {
            WP_LOG_ERROR( "CSkybox6SidedOgreNext::setupStateObject - failed to allocate state." );
            return;
        }

        state->setId( getId() );
        state->setOwner( this );
        state->setData( stateData );
        stateContext->addState( state );
    }

    void CSkybox6SidedOgreNext::destroyStateObject()
    {
        if( auto stateContext = getStateContext() )
        {
            stateContext->removeStatesById( getId() );
        }
    }

}  // namespace workphone::render
