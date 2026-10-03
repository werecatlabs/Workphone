#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Core/SkyBoxRenderer.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreException.h>
#include <OgreHlmsUnlit.h>
#include <OgreHlmsUnlitDatablock.h>
#include <OgreManualObject2.h>
#include <OgreTextureGpuManager.h>
#include <OgreResourceGroupManager.h>
#include <OgreGpuProgramParams.h>
#include <OgreHlmsPbsDatablock.h>
#include <OgreHlms.h>
#include <OgreHlmsPbs.h>
#include <OgreHlmsUnlitDatablock.h>
#include <OgreHlmsManager.h>
#include <OgreHlmsPbsDatablock.h>
#include <OgreHlmsUnlitDatablock.h>
#include <OgreHlms.h>
#include <OgreHlmsPbs.h>
#include <OgreHlmsManager.h>
#include <OgreTextureGpuManager.h>
#include <OgreRectangle2D2.h>

namespace workphone::render
{

    SkyBoxRenderer::~SkyBoxRenderer()
    {
        unload( nullptr );
    }

    void SkyBoxRenderer::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

#if 1
            using namespace Ogre;

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto smgr = graphicsSystem->getGraphicsScene();
            WP_ASSERT( smgr );

            auto root = Ogre::Root::getSingletonPtr();
            auto renderSystem = root->getRenderSystem();
            auto textureManager = renderSystem->getTextureGpuManager();

            Ogre::SceneManager *sceneManager = nullptr;
            smgr->_getObject( reinterpret_cast<void **>( &sceneManager ) );

            auto &entityMemoryManager = sceneManager->_getEntityMemoryManager( SCENE_STATIC );

            if( !mSky )
            {
                mSky = OGRE_NEW Ogre::Rectangle2D( Id::generateNewId<MovableObject>(),
                                                   &entityMemoryManager, sceneManager );
                // We can't use BT_DYNAMIC_* because the scene may be rendered from multiple cameras
                // in the same frame, and dynamic supports only one set of values per frame
                mSky->initialize( BT_DEFAULT,
                                  Rectangle2D::GeometryFlagQuad | Rectangle2D::GeometryFlagNormals );
                mSky->setGeometry( -Ogre::Vector2::UNIT_SCALE, Ogre::Vector2( 2.0f ) );
                mSky->setRenderQueueGroup( 212u );  // Render after most stuff
                sceneManager->getRootSceneNode( SCENE_STATIC )->attachObject( mSky );
            }

            static const Ogre::String datablockUnlitName = "Test/SceneSkyBox1";

            const c8 *baseMatNames[] = {
                "Ogre/Sky/Cubemap",
                "Ogre/Sky/Equirectangular",
            };

            auto datablockName = datablockUnlitName;

            MaterialManager &materialManager = MaterialManager::getSingleton();

            u32 skyMethod = 0;
            const IdType sceneManagerId = sceneManager->getId();
            const String matName = baseMatNames[skyMethod] + StringUtil::toString( sceneManagerId );

            m_skyMaterial = materialManager.getByName( matName.c_str() );
            if( !m_skyMaterial )
            {
#    if 1
                m_skyMaterial = materialManager.getByName(
                    baseMatNames[skyMethod], ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                if( !m_skyMaterial )
                {
                    // OGRE_EXCEPT(
                    //     Exception::ERR_FILE_NOT_FOUND,
                    //     "To use the sky bundle the resources included in Samples/Media/Common",
                    //     "SceneManager::setSky" );
                }

                m_skyMaterial->load();
                m_skyMaterial = m_skyMaterial->clone( matName.c_str() );
#    else

                auto res = materialManager.createOrRetrieve(
                    matName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                m_skyMaterial = res.first.dynamicCast<Ogre::Material>();
#    endif
            }

            auto resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

            //auto texName = "SaintPetersBasilica.dds";
            //auto texName = "specular_bk.jpg";
            //auto texName = "specular.jpg";
            auto texName = "checker.png";

            TextureGpu *texture;
            texture = textureManager->createOrRetrieveTexture(
                texName, GpuPageOutStrategy::Discard, CommonTextureTypes::Diffuse, resourceGroup );
            texture->scheduleTransitionTo( GpuResidency::Resident );

            Pass *pass = m_skyMaterial->getTechnique( 0 )->getPass( 0 );
            auto datablock = pass->_getDatablock();
            auto datablockNameId = datablock->getName();
            datablockName = datablockNameId.getFriendlyText();

            if( skyMethod == SceneManager::SkyEquirectangular )
            {
                GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
                psParams->setNamedConstant( "sliceIdx",
                                            static_cast<float>( texture->getInternalSliceStart() ) );
            }

            TextureUnitState *tu = pass->getTextureUnitState( 0 );
            // Ensure we don't accidentally clone the texture (minimize
            // mem consumption by being Automatic Batching aware)
            //tu->setAutomaticBatching( texture->hasAutomaticBatching() );
            //tu->setTexture( texture );

            TextureGpu *texs[6];
            for( auto &tex : texs )
            {
                tex = texture;
            }

            tu->setCubicTexture( texs );

            mSky->setMaterial( m_skyMaterial );
            mSky->setVisible( true );
#elif 0
            using namespace Ogre;

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto smgr = graphicsSystem->getGraphicsScene();
            WP_ASSERT( smgr );

            auto root = Ogre::Root::getSingletonPtr();
            auto renderSystem = root->getRenderSystem();
            auto textureManager = renderSystem->getTextureGpuManager();

            Ogre::SceneManager *sceneManager = nullptr;
            smgr->_getObject( (void **)&sceneManager );

            static const Ogre::String datablockUnlitName = "Test/SceneSkyBox1";

            auto datablockName = datablockUnlitName;

            auto hlmsManager = root->getHlmsManager();

            auto hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_UNLIT ) );
            if( hlmsPbs )
            {
                Ogre::HlmsMacroblock macroblock;
                macroblock.mDepthCheck = false;

                Ogre::HlmsBlendblock blendblock;
                Ogre::HlmsParamVec paramVec;

                auto datablock = (Ogre::HlmsUnlitDatablock *)hlmsPbs->getDatablock( datablockName );
                if( !datablock )
                {
                    datablock = static_cast<Ogre::HlmsUnlitDatablock *>( hlmsPbs->createDatablock(
                        datablockName, datablockName, macroblock, blendblock, paramVec ) );

                    //auto texName = "SaintPetersBasilica.dds";
                    //datablock->setTexture( 0, texName );
                }

                // panel->setDatablock( datablock );
                // panel->setMaterialName( datablockName );
            }

            auto manualObject = sceneManager->createManualObject( Ogre::SCENE_DYNAMIC );
            m_manualObject = manualObject;

            // m_manualObject->clear();

            m_manualObject->begin( datablockName,
                                   Ogre::OT_TRIANGLE_STRIP );  // Datablock DbRed works

            // rendering cube, only using 14 vertices
            const Ogre::Vector3 cube_strip[14] = {
                { -1.f, 1.f, 1.f },    // Front-top-left
                { 1.f, 1.f, 1.f },     // Front-top-right
                { -1.f, -1.f, 1.f },   // Front-bottom-left
                { 1.f, -1.f, 1.f },    // Front-bottom-right
                { 1.f, -1.f, -1.f },   // Back-bottom-right
                { 1.f, 1.f, 1.f },     // Front-top-right
                { 1.f, 1.f, -1.f },    // Back-top-right
                { -1.f, 1.f, 1.f },    // Front-top-left
                { -1.f, 1.f, -1.f },   // Back-top-left
                { -1.f, -1.f, 1.f },   // Front-bottom-left
                { -1.f, -1.f, -1.f },  // Back-bottom-left
                { 1.f, -1.f, -1.f },   // Back-bottom-right
                { -1.f, 1.f, -1.f },   // Back-top-left
                { 1.f, 1.f, -1.f }     // Back-top-right
            };

            size_t count = 0;
            for( const auto &vtx : cube_strip )
            {
                m_manualObject->position( m_orientation * ( vtx * m_distance ) );
                // Note UVs mirrored front/back
                auto uv = vtx.normalisedCopy() * Ogre::Vector3( 1, 1, -1 );
                m_manualObject->textureCoord( uv );

                m_manualObject->index( count++ );
                // m_manualObject->index( count++ );
                // m_manualObject->index( count++ );

                // count -= 2;
            }

            // m_manualObject->triangle(0, 1, 2 );

            // for( size_t i = 3; i < 14; ++i )
            //            {
            //                m_manualObject->triangle( i-2, i-1, i );
            //            }

            m_manualObject->end();

            m_sceneNode = sceneManager->createSceneNode();
            // mSceneNode->setListener( this );

            m_manualObject->setCastShadows( false );
            m_sceneNode->attachObject( m_manualObject );

#elif 1

            using namespace Ogre;

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto smgr = graphicsSystem->getGraphicsScene();
            WP_ASSERT( smgr );

            auto root = Ogre::Root::getSingletonPtr();
            auto renderSystem = root->getRenderSystem();
            auto textureManager = renderSystem->getTextureGpuManager();

            Ogre::SceneManager *sceneManager = nullptr;
            smgr->_getObject( (void **)&sceneManager );

            static const Ogre::String datablockUnlitName = "Test/SceneSkyBox1";

            const char *baseMatNames[] = {
                "Ogre/Sky/Cubemap",
                "Ogre/Sky/Equirectangular",
            };

            auto datablockName = datablockUnlitName;

            MaterialManager &materialManager = MaterialManager::getSingleton();

            u32 skyMethod = 0;
            const IdType sceneManagerId = sceneManager->getId();
            const String matName = baseMatNames[skyMethod] + StringConverter::toString( sceneManagerId );

            m_skyMaterial = materialManager.getByName( matName );
            if( !m_skyMaterial )
            {
#    if 1
                m_skyMaterial = materialManager.getByName(
                    baseMatNames[skyMethod], ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                if( !m_skyMaterial )
                {
                    // OGRE_EXCEPT(
                    //     Exception::ERR_FILE_NOT_FOUND,
                    //     "To use the sky bundle the resources included in Samples/Media/Common",
                    //     "SceneManager::setSky" );
                }

                m_skyMaterial->load();
                m_skyMaterial = m_skyMaterial->clone( matName );
#    else

                auto res = materialManager.createOrRetrieve(
                    matName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                m_skyMaterial = res.first.dynamicCast<Ogre::Material>();
#    endif
            }

            Pass *pass = m_skyMaterial->getTechnique( 0 )->getPass( 0 );
            auto datablock = pass->_getDatablock();
            auto datablockNameId = datablock->getName();
            datablockName = datablockNameId.getFriendlyText();

            auto texName = "SaintPetersBasilica.dds";
            //auto texName = "specular_bk.jpg";
            //auto texName = "specular.jpg";
            //auto texName = "checker.png";

            auto resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

            TextureGpu *texture;
            texture = textureManager->createOrRetrieveTexture(
                texName, GpuPageOutStrategy::Discard, CommonTextureTypes::EnvMap, resourceGroup );
            texture->scheduleTransitionTo( GpuResidency::Resident );

            TextureUnitState *tu = pass->getTextureUnitState( 0 );
            // Ensure we don't accidentally clone the texture (minimize
            // mem consumption by being Automatic Batching aware)
            //tu->setAutomaticBatching( texture->hasAutomaticBatching() );
            tu->setTexture( texture );

            auto &entityMemoryManager = sceneManager->_getEntityMemoryManager( SCENE_STATIC );

            auto manualObject = sceneManager->createManualObject( Ogre::SCENE_DYNAMIC );
            m_manualObject = manualObject;

            // m_manualObject->clear();

            m_manualObject->begin( matName,
                                   Ogre::OT_TRIANGLE_STRIP );  // Datablock DbRed works

            // rendering cube, only using 14 vertices
            const Ogre::Vector3 cube_strip[14] = {
                { -1.f, 1.f, 1.f },    // Front-top-left
                { 1.f, 1.f, 1.f },     // Front-top-right
                { -1.f, -1.f, 1.f },   // Front-bottom-left
                { 1.f, -1.f, 1.f },    // Front-bottom-right
                { 1.f, -1.f, -1.f },   // Back-bottom-right
                { 1.f, 1.f, 1.f },     // Front-top-right
                { 1.f, 1.f, -1.f },    // Back-top-right
                { -1.f, 1.f, 1.f },    // Front-top-left
                { -1.f, 1.f, -1.f },   // Back-top-left
                { -1.f, -1.f, 1.f },   // Front-bottom-left
                { -1.f, -1.f, -1.f },  // Back-bottom-left
                { 1.f, -1.f, -1.f },   // Back-bottom-right
                { -1.f, 1.f, -1.f },   // Back-top-left
                { 1.f, 1.f, -1.f }     // Back-top-right
            };

            size_t count = 0;
            for( const auto &vtx : cube_strip )
            {
                m_manualObject->position( m_orientation * ( vtx * m_distance ) );
                // Note UVs mirrored front/back
                auto uv = vtx.normalisedCopy() * Ogre::Vector3( 1, 1, -1 );
                m_manualObject->textureCoord( uv );

                m_manualObject->index( count++ );
                // m_manualObject->index( count++ );
                // m_manualObject->index( count++ );

                // count -= 2;
            }

            // m_manualObject->triangle(0, 1, 2 );

            // for( size_t i = 3; i < 14; ++i )
            //            {
            //                m_manualObject->triangle( i-2, i-1, i );
            //            }

            m_manualObject->end();

            m_sceneNode = sceneManager->createSceneNode();
            // mSceneNode->setListener( this );

            m_manualObject->setCastShadows( false );
            m_sceneNode->attachObject( m_manualObject );
#endif

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void SkyBoxRenderer::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            using namespace Ogre;

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto smgr = graphicsSystem->getGraphicsScene();
            WP_ASSERT( smgr );

            auto root = Ogre::Root::getSingletonPtr();
            auto renderSystem = root->getRenderSystem();
            auto textureManager = renderSystem->getTextureGpuManager();

            Ogre::SceneManager *sceneManager = nullptr;
            smgr->_getObject( reinterpret_cast<void **>( &sceneManager ) );

            //if( m_manualObject )
            //{
            //    if( m_sceneNode )
            //    {
            //        m_sceneNode->detachAllObjects();
            //    }

            //    sceneManager->destroyManualObject( m_manualObject );
            //    m_manualObject = nullptr;
            //}

            if( m_sceneNode )
            {
                sceneManager->destroySceneNode( m_sceneNode );
                m_sceneNode = nullptr;
            }

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto SkyBoxRenderer::getDistance() const -> f32
    {
        return m_distance;
    }

    void SkyBoxRenderer::setDistance( f32 distance )
    {
        m_distance = distance;
    }

    auto SkyBoxRenderer::getSkyMaterial() const -> Ogre::MaterialPtr
    {
        return m_skyMaterial;
    }

    void SkyBoxRenderer::setSkyMaterial( Ogre::MaterialPtr skyMaterial )
    {
        m_skyMaterial = skyMaterial;
    }

}  // namespace workphone::render
