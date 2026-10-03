#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CCubemapOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSystemOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreRoot.h>
#include <OgreTextureGpu.h>
#include <OgreTextureGpuManager.h>
#include <OgrePixelFormatGpuUtils.h>
#include <OgreSceneManager.h>
#include <OgreHlms.h>
#include <OgreHlmsManager.h>
#include <OgreHlmsPbs.h>
#include <OgreHlmsPbsDatablock.h>
#include <OgreHlmsSamplerblock.h>
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <Compositor/OgreCompositorWorkspaceDef.h>
#include <Compositor/OgreCompositorNodeDef.h>
#include <Compositor/OgreTextureDefinition.h>
#include <Compositor/Pass/OgreCompositorPassDef.h>
#include <Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h>
#include <Compositor/Pass/PassClear/OgreCompositorPassClearDef.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone::render
{
    u32 CCubemapOgreNext::m_nameExt = 0;

    namespace
    {
        Ogre::uint8 getFaceExecutionMask( u32 faceIndex )
        {
            return static_cast<Ogre::uint8>( 1u << faceIndex );
        }

        Ogre::HlmsSamplerblock createCubemapSamplerblock()
        {
            Ogre::HlmsSamplerblock samplerblock;
            samplerblock.mU = Ogre::TAM_CLAMP;
            samplerblock.mV = Ogre::TAM_CLAMP;
            samplerblock.mW = Ogre::TAM_CLAMP;
            return samplerblock;
        }

        f32 sanitizeFinite( f32 value, f32 fallback, f32 minValue, f32 maxValue )
        {
            if( !std::isfinite( static_cast<double>( value ) ) )
            {
                return fallback;
            }

            return std::max( minValue, std::min( value, maxValue ) );
        }

        f32 distanceBetween( const Vector3F &a, const Vector3F &b )
        {
            const auto x = a[0] - b[0];
            const auto y = a[1] - b[1];
            const auto z = a[2] - b[2];
            return std::sqrt( x * x + y * y + z * z );
        }

        f32 boxOutsideDistance( const Vector3F &position, const Vector3F &center,
                                const Vector3F &extents )
        {
            const auto dx = std::max( 0.0f, std::abs( position[0] - center[0] ) - extents[0] );
            const auto dy = std::max( 0.0f, std::abs( position[1] - center[1] ) - extents[1] );
            const auto dz = std::max( 0.0f, std::abs( position[2] - center[2] ) - extents[2] );
            return std::sqrt( dx * dx + dy * dy + dz * dz );
        }

        f32 boundaryFalloff( f32 distancePastInnerBoundary, f32 blendDistance )
        {
            if( distancePastInnerBoundary <= 0.0f )
            {
                return 1.0f;
            }

            if( blendDistance <= 0.0f )
            {
                return 0.0f;
            }

            return std::max( 0.0f, 1.0f - distancePastInnerBoundary / blendDistance );
        }
    }  // namespace

    CCubemapOgreNext::CCubemapOgreNext()
    {
        try
        {
            if( auto logMgr = Ogre::LogManager::getSingletonPtr() )
            {
                logMgr->logMessage( "CCubemapOgreNext constructed." );
            }
        }
        catch( const std::exception &e )
        {
            if( Ogre::LogManager::getSingletonPtr() )
                Ogre::LogManager::getSingletonPtr()->logMessage(
                    Ogre::String( "Exception in CCubemapOgreNext constructor: " ) + e.what() );
        }
    }

    CCubemapOgreNext::~CCubemapOgreNext()
    {
        try
        {
            unload( nullptr );

            if( auto logMgr = Ogre::LogManager::getSingletonPtr() )
            {
                logMgr->logMessage( "CCubemapOgreNext destructed." );
            }
        }
        catch( const std::exception &e )
        {
            if( Ogre::LogManager::getSingletonPtr() )
                Ogre::LogManager::getSingletonPtr()->logMessage(
                    Ogre::String( "Exception in CCubemapOgreNext destructor: " ) + e.what() );
        }
    }

    void CCubemapOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( auto logMgr = Ogre::LogManager::getSingletonPtr() )
            {
                logMgr->logMessage( "CCubemapOgreNext::load called." );
            }

            if( !m_sceneMgr )
            {
                if( auto logMgr = Ogre::LogManager::getSingletonPtr() )
                {
                    logMgr->logMessage(
                        "[CCubemapOgreNext::load] SceneManager not set. Call setSceneManager first." );
                }
                return;
            }

            const auto renderToTexture =
                m_enable || StringUtil::isNullOrEmpty( Path::getFileExtension( m_textureName ) );
            createCubemapTexture( renderToTexture );
            createCubemapCamera();
            createCubemapWorkspaces();
            registerWithScene();

            // Attach the frame listener
            if( !m_frameListener )
            {
                m_frameListener = new CubemapFrameListener( this );
            }

            const auto enable = m_enable;
            m_enable = false;
            setEnable( enable );
        }
        catch( const std::exception &e )
        {
            if( Ogre::LogManager::getSingletonPtr() )
                Ogre::LogManager::getSingletonPtr()->logMessage(
                    Ogre::String( "Exception in CCubemapOgreNext::load: " ) + e.what() );
        }
    }

    void CCubemapOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( auto logMgr = Ogre::LogManager::getSingletonPtr() )
            {
                logMgr->logMessage( "CCubemapOgreNext::unload called." );
            }

            setEnable( false );
            unregisterFromScene();

            destroyCubemapWorkspaces();

            if( m_frameListener )
            {
                delete m_frameListener;
                m_frameListener = nullptr;
            }

            if( m_camera && m_sceneMgr )
            {
                m_sceneMgr->destroyCamera( m_camera );
                m_camera = nullptr;
            }

            if( m_cubemapTexture )
            {
                destroyCubemapTexture();
            }

            m_sceneMgr = nullptr;
        }
        catch( const std::exception &e )
        {
            if( Ogre::LogManager::getSingletonPtr() )
                Ogre::LogManager::getSingletonPtr()->logMessage(
                    Ogre::String( "Exception in CCubemapOgreNext::unload: " ) + e.what() );
        }
    }

    void CCubemapOgreNext::createCubemapTexture( bool renderToTexture )
    {
        auto root = Ogre::Root::getSingletonPtr();
        auto renderSystem = root->getRenderSystem();
        auto textureManager = renderSystem->getTextureGpuManager();

        // Generate unique texture name if not already set
        if( m_textureName.empty() )
        {
            m_textureName = "DynamicCubemap_" + StringUtil::toString( m_nameExt++ );
        }

        m_renderToTexture = renderToTexture;
        if( m_renderToTexture )
        {
            m_cubemapTexture = textureManager->createOrRetrieveTexture(
                m_textureName.c_str(), Ogre::GpuPageOutStrategy::Discard,
                Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::AllowAutomipmaps,
                Ogre::TextureTypes::TypeCube );

            m_cubemapTexture->setResolution( m_resolution, m_resolution, 6u );
            m_cubemapTexture->setPixelFormat( m_pixelFormat );
            m_cubemapTexture->setNumMipmaps(
                Ogre::PixelFormatGpuUtils::getMaxMipmapCount( m_resolution, m_resolution ) );
            m_cubemapTexture->scheduleTransitionTo( Ogre::GpuResidency::Resident );
        }
        else
        {
            m_cubemapTexture = textureManager->createOrRetrieveTexture(
                m_textureName.c_str(), Ogre::GpuPageOutStrategy::Discard,
                Ogre::CommonTextureTypes::EnvMap,
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

            if( m_cubemapTexture )
            {
                m_cubemapTexture->scheduleTransitionTo( Ogre::GpuResidency::Resident );
            }
        }

        updateTextureWrapper();
        notifyAutomaticMaterialStateChanged();

        if( auto logMgr = Ogre::LogManager::getSingletonPtr() )
        {
            auto msg = "CCubemapOgreNext: Created " +
                       String( m_renderToTexture ? "render" : "file" ) + " cubemap texture '" +
                       m_textureName + "' with resolution " + StringUtil::toString( m_resolution ) +
                       "x" + StringUtil::toString( m_resolution );
            logMgr->logMessage( msg.c_str() );
        }
    }

    void CCubemapOgreNext::createCubemapCamera()
    {
        if( !m_sceneMgr )
            return;

        if( !m_camera )
        {
            String camName = "CubemapCamera_" + StringUtil::toString( m_nameExt );
            m_camera = m_sceneMgr->createCamera( camName.c_str(), true, true );
            m_camera->setFOVy( Ogre::Radian( Ogre::Math::HALF_PI ) );  // 90 degrees
            m_camera->setAspectRatio( 1.0f );
            m_camera->setFixedYawAxis( false );
            m_camera->setNearClipDistance( m_nearClip );
            m_camera->setFarClipDistance( m_farClip );

            if( auto logMgr = Ogre::LogManager::getSingletonPtr() )
            {
                auto msg = "CCubemapOgreNext: Created cubemap camera '" + camName +
                           "' with FOV 90 degrees, aspect ratio 1.0, near clip " +
                           StringUtil::toString( m_nearClip ) + ", far clip " +
                           StringUtil::toString( m_farClip );
                logMgr->logMessage( msg.c_str() );
            }
        }
    }

    void CCubemapOgreNext::createCubemapWorkspaces()
    {
        if( !m_renderToTexture || !m_cubemapTexture || !m_camera || !m_sceneMgr )
            return;

        auto root = Ogre::Root::getSingletonPtr();
        if( !root )
            return;

        auto compositorManager = root->getCompositorManager2();
        if( !compositorManager )
            return;

        destroyCubemapWorkspaces();

        const auto nodeDefName = "CubemapNodeDef_" + m_textureName;
        auto workspaceDefName = "CubemapWorkspaceDef_" + m_textureName;

        if( compositorManager->hasWorkspaceDefinition( workspaceDefName.c_str() ) )
        {
            compositorManager->removeWorkspaceDefinition( workspaceDefName.c_str() );
        }

        if( compositorManager->hasNodeDefinition( nodeDefName.c_str() ) )
        {
            compositorManager->removeNodeDefinition( nodeDefName.c_str() );
        }

        auto nodeDef = compositorManager->addNodeDefinition( nodeDefName.c_str() );
        nodeDef->addTextureSourceName( "CubemapRT", 0,
                                       Ogre::TextureDefinitionBase::TEXTURE_INPUT );

        nodeDef->setNumTargetPass( 6u );
        for( u32 i = 0; i < 6u; ++i )
        {
            auto targetDef = nodeDef->addTargetPass( "CubemapRT", i );
            targetDef->setNumPasses( 1u );

            auto passScene = static_cast<Ogre::CompositorPassSceneDef *>(
                targetDef->addPass( Ogre::PASS_SCENE ) );
            passScene->setAllLoadActions( Ogre::LoadAction::Clear );
            passScene->setAllClearColours( m_backgroundColor );
            passScene->mClearDepth = 1.0f;
            passScene->mIncludeOverlays = false;
            passScene->mCameraCubemapReorient = true;
            passScene->mVisibilityMask = m_visibilityMask;
            passScene->mExecutionMask = getFaceExecutionMask( i );
        }

        auto workspaceDef = compositorManager->addWorkspaceDefinition( workspaceDefName.c_str() );
        workspaceDef->connectExternal( 0u, nodeDefName.c_str(), 0u );

        Ogre::CompositorChannelVec externalChannels;
        externalChannels.push_back( m_cubemapTexture );

        for( u32 i = 0u; i < 6u; ++i )
        {
            auto workspace = compositorManager->addWorkspace(
                m_sceneMgr, externalChannels, m_camera, workspaceDefName.c_str(), false, -1,
                nullptr, nullptr, Ogre::Vector4::ZERO, 0x00, getFaceExecutionMask( i ) );

            workspace->setEnabled( false );
            m_workspaces.push_back( workspace );
        }

        if( auto logMgr = Ogre::LogManager::getSingletonPtr() )
        {
            auto str = "CCubemapOgreNext: Created " + StringUtil::toString( (u32)m_workspaces.size() ) +
                       " cubemap workspaces";
            logMgr->logMessage( str.c_str() );
        }
    }

    void CCubemapOgreNext::destroyCubemapWorkspaces()
    {
        auto root = Ogre::Root::getSingletonPtr();
        if( !root )
            return;

        auto compositorManager = root->getCompositorManager2();
        if( !compositorManager )
            return;

        for( auto workspace : m_workspaces )
        {
            if( workspace )
            {
                compositorManager->removeWorkspace( workspace );
            }
        }
        m_workspaces.clear();
    }

    void CCubemapOgreNext::destroyCubemapTexture()
    {
        if( auto ogreTexture = workphone::dynamic_pointer_cast<CTextureOgreNext>( m_texture ) )
        {
            ogreTexture->setTexture( nullptr );
        }
        m_texture = nullptr;
        m_renderToTexture = false;

        if( m_cubemapTexture )
        {
            auto root = Ogre::Root::getSingletonPtr();
            if( root )
            {
                auto renderSystem = root->getRenderSystem();
                if( renderSystem )
                {
                    auto textureManager = renderSystem->getTextureGpuManager();
                    if( textureManager )
                    {
                        textureManager->destroyTexture( m_cubemapTexture );
                    }
                }
            }
            m_cubemapTexture = nullptr;
            notifyAutomaticMaterialStateChanged();
        }
    }

    void CCubemapOgreNext::updateTextureWrapper()
    {
        if( !m_cubemapTexture )
        {
            return;
        }

        auto ogreTexture = workphone::dynamic_pointer_cast<CTextureOgreNext>( m_texture );
        if( !ogreTexture )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            ogreTexture = factoryManager->make_ptr<CTextureOgreNext>();
            m_texture = ogreTexture;
        }

        if( ogreTexture )
        {
            ogreTexture->setName( m_textureName );
            ogreTexture->setFilePath( m_textureName );
            ogreTexture->setUsageFlags(
                m_renderToTexture ? static_cast<u32>( TextureUsage::TU_RENDERTARGET ) : 0u );
            ogreTexture->setSize( Vector2I( m_resolution, m_resolution ) );
            ogreTexture->setTexture( m_cubemapTexture );
        }
    }

    void CCubemapOgreNext::render()
    {
        renderFaces( m_faceMask );
    }

    void CCubemapOgreNext::renderFaces( u32 faceMask )
    {
        try
        {
            if( !m_camera || !m_enable )
                return;

            // Set camera position
            m_camera->setPosition( Ogre::Vector3( m_position[0], m_position[1], m_position[2] ) );

            faceMask &= FaceAll;
            for( u32 i = 0; i < 6; ++i )
            {
                if( faceMask & ( 1u << i ) )
                {
                    renderFace( i );
                }
            }
        }
        catch( const std::exception &e )
        {
            if( Ogre::LogManager::getSingletonPtr() )
                Ogre::LogManager::getSingletonPtr()->logMessage(
                    Ogre::String( "Exception in CCubemapOgreNext::renderFaces: " ) + e.what() );
        }
    }

    u32 CCubemapOgreNext::findNextFace( u32 faceMask ) const
    {
        faceMask &= FaceAll;
        for( u32 offset = 0; offset < 6; ++offset )
        {
            const auto faceIndex = ( m_currentIndex + offset ) % 6u;
            if( faceMask & ( 1u << faceIndex ) )
            {
                return faceIndex;
            }
        }

        return 6u;
    }

    void CCubemapOgreNext::completeRequestedUpdate()
    {
        if( m_updateQueued && m_faceMask != 0u )
        {
            m_pendingFaceMask = m_faceMask;
            m_updateQueued = false;
        }
        else
        {
            m_pendingFaceMask = 0u;
            m_updateQueued = false;
        }
    }

    void CCubemapOgreNext::renderFace( u32 faceIndex )
    {
        if( faceIndex >= 6 || faceIndex >= m_workspaces.size() )
            return;

        // Hide excluded objects
        for( auto &obj : m_objects )
        {
            if( obj )
            {
                obj->setVisible( false );
            }
        }

        // The compositor pass reorients the cubemap camera for the selected slice.
        m_camera->setPosition( Ogre::Vector3( m_position[0], m_position[1], m_position[2] ) );

        // Set visibility mask
        if( m_workspaces[faceIndex] )
        {
            m_workspaces[faceIndex]->setEnabled( true );
            m_workspaces[faceIndex]->_validateFinalTarget();
            m_workspaces[faceIndex]->_beginUpdate( false );
            m_workspaces[faceIndex]->_update();
            m_workspaces[faceIndex]->_endUpdate( false );
            m_workspaces[faceIndex]->setEnabled( false );
        }

        // Restore visibility of excluded objects
        for( auto &obj : m_objects )
        {
            if( obj )
            {
                obj->setVisible( true );
            }
        }
    }

    void CCubemapOgreNext::setVisibilityMask( u32 visibilityMask )
    {
        if( m_visibilityMask != visibilityMask )
        {
            m_visibilityMask = visibilityMask;

            if( m_renderToTexture && m_cubemapTexture && m_camera && m_sceneMgr )
            {
                createCubemapWorkspaces();
            }

            notifyAutomaticMaterialStateChanged();
        }
    }

    auto CCubemapOgreNext::getExclusionMask() const -> u32
    {
        return m_exclusionMask;
    }

    void CCubemapOgreNext::setExclusionMask( u32 exclusionMask )
    {
        m_exclusionMask = exclusionMask;
    }

    auto CCubemapOgreNext::getSceneManager() const -> SmartPtr<IGraphicsScene>
    {
        return m_graphicsScene;
    }

    void CCubemapOgreNext::setSceneManager( SmartPtr<IGraphicsScene> smgr )
    {
        try
        {
            unregisterFromScene();

            m_graphicsScene = smgr;
            m_sceneMgr = nullptr;

            if( smgr )
            {
                smgr->_getObject( (void **)&m_sceneMgr );
            }

            if( !m_sceneMgr )
            {
                if( auto logMgr = Ogre::LogManager::getSingletonPtr() )
                {
                    logMgr->logMessage( "[CCubemapOgreNext::setSceneManager] SceneManager is null." );
                }
            }

            registerWithScene();
        }
        catch( const std::exception &e )
        {
            if( Ogre::LogManager::getSingletonPtr() )
                Ogre::LogManager::getSingletonPtr()->logMessage(
                    Ogre::String( "Exception in CCubemapOgreNext::setSceneManager: " ) + e.what() );
        }
    }

    void CCubemapOgreNext::setUpdateInterval( u32 milliseconds )
    {
        m_updateInterval = milliseconds;
    }

    IGraphicsCubemap::UpdateMode CCubemapOgreNext::getUpdateMode() const
    {
        return m_updateMode;
    }

    void CCubemapOgreNext::setUpdateMode( UpdateMode updateMode )
    {
        if( m_updateMode != updateMode )
        {
            m_updateMode = updateMode;
            m_pendingFaceMask = 0u;
            m_updateQueued = false;
        }
    }

    IGraphicsCubemap::TimeSlicingMode CCubemapOgreNext::getTimeSlicingMode() const
    {
        return m_timeSlicingMode;
    }

    void CCubemapOgreNext::setTimeSlicingMode( TimeSlicingMode timeSlicingMode )
    {
        m_timeSlicingMode = timeSlicingMode;
    }

    u32 CCubemapOgreNext::getFaceMask() const
    {
        return m_faceMask;
    }

    void CCubemapOgreNext::setFaceMask( u32 faceMask )
    {
        faceMask &= FaceAll;
        if( m_faceMask == faceMask )
        {
            return;
        }

        m_faceMask = faceMask;
        m_pendingFaceMask &= m_faceMask;
        if( m_pendingFaceMask == 0u )
        {
            completeRequestedUpdate();
        }
    }

    void CCubemapOgreNext::requestUpdate()
    {
        if( m_updateMode != UpdateMode::OnDemand || m_faceMask == 0u )
        {
            return;
        }

        if( m_pendingFaceMask == 0u )
        {
            m_pendingFaceMask = m_faceMask;
            m_updateQueued = false;
        }
        else
        {
            // Coalesce repeated requests into one follow-up refresh. This prevents an
            // EveryFrame probe from continually re-adding faces to the active cycle.
            m_updateQueued = true;
        }
    }

    bool CCubemapOgreNext::isUpdatePending() const
    {
        return m_updateMode == UpdateMode::OnDemand &&
               ( m_pendingFaceMask != 0u || m_updateQueued );
    }

    void CCubemapOgreNext::addExcludedObject( SmartPtr<IGraphicsObject> object )
    {
        m_objects.push_back( object );
    }

    auto CCubemapOgreNext::getExcludedObjects() const -> Array<SmartPtr<IGraphicsObject>>
    {
        return m_objects;
    }

    void CCubemapOgreNext::generateMaterial( const String &materialName )
    {
        try
        {
            if( materialName.empty() )
            {
                return;
            }

            if( !m_cubemapTexture )
            {
                createCubemapTexture( m_enable || StringUtil::isNullOrEmpty(
                                                       Path::getFileExtension( m_textureName ) ) );
            }

            if( !m_cubemapTexture )
            {
                return;
            }

            auto root = Ogre::Root::getSingletonPtr();
            if( !root )
            {
                return;
            }

            auto hlmsManager = root->getHlmsManager();
            if( !hlmsManager )
            {
                return;
            }

            auto hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );
            if( !hlmsPbs )
            {
                return;
            }

            auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock *>(
                hlmsManager->getDatablockNoDefault( materialName.c_str() ) );
            if( !datablock )
            {
                auto existingDatablock = hlmsManager->getDatablockNoDefault( materialName.c_str() );
                if( existingDatablock )
                {
                    if( auto logMgr = Ogre::LogManager::getSingletonPtr() )
                    {
                        auto msg = "CCubemapOgreNext::generateMaterial - Datablock '" +
                                   materialName + "' is not an HLMS PBS datablock.";
                        logMgr->logMessage( msg.c_str() );
                    }
                    return;
                }

                datablock = static_cast<Ogre::HlmsPbsDatablock *>( hlmsPbs->createDatablock(
                    materialName.c_str(), materialName.c_str(), Ogre::HlmsMacroblock(),
                    Ogre::HlmsBlendblock(), Ogre::HlmsParamVec() ) );
            }

            auto samplerblock = createCubemapSamplerblock();
            datablock->setTexture( Ogre::PBSM_REFLECTION, m_cubemapTexture, &samplerblock );

            if( auto logMgr = Ogre::LogManager::getSingletonPtr() )
            {
                auto msg = "CCubemapOgreNext::generateMaterial - Assigned cubemap texture '" +
                           m_textureName + "' to PBS reflection slot for material '" + materialName +
                           "'.";
                logMgr->logMessage( msg.c_str() );
            }
        }
        catch( const std::exception &e )
        {
            if( Ogre::LogManager::getSingletonPtr() )
                Ogre::LogManager::getSingletonPtr()->logMessage(
                    Ogre::String( "Exception in CCubemapOgreNext::generateMaterial: " ) + e.what() );
        }
    }

    auto CCubemapOgreNext::getVisibilityMask() const -> u32
    {
        return m_visibilityMask;
    }

    auto CCubemapOgreNext::getPosition() const -> Vector3F
    {
        return m_position;
    }

    void CCubemapOgreNext::setPosition( const Vector3F &position )
    {
        if( m_position != position )
        {
            m_position = position;
            notifyAutomaticMaterialStateChanged();
        }
    }

    auto CCubemapOgreNext::getEnable() const -> bool
    {
        return m_enable;
    }

    void CCubemapOgreNext::setEnable( bool enable )
    {
        if( m_enable != enable )
        {
            m_enable = enable;

            auto root = Ogre::Root::getSingletonPtr();
            if( !root )
                return;

            if( m_enable )
            {
                if( m_frameListener )
                {
                    root->addFrameListener( m_frameListener );
                }
            }
            else
            {
                if( m_frameListener )
                {
                    root->removeFrameListener( m_frameListener );
                }

                if( m_updateMode == UpdateMode::OnDemand )
                {
                    m_pendingFaceMask = 0u;
                    m_updateQueued = false;
                }
            }

            notifyAutomaticMaterialStateChanged();
        }
    }

    auto CCubemapOgreNext::getUpdateInterval() const -> u32
    {
        return m_updateInterval;
    }

    auto CCubemapOgreNext::getTextureName() const -> String
    {
        return m_textureName;
    }

    void CCubemapOgreNext::setTextureName( const String &textureName )
    {
        if( m_textureName == textureName )
        {
            return;
        }

        m_textureName = textureName;

        if( m_cubemapTexture )
        {
            const auto wasEnabled = m_enable;
            setEnable( false );
            destroyCubemapWorkspaces();
            destroyCubemapTexture();
            createCubemapTexture( wasEnabled || StringUtil::isNullOrEmpty(
                                                    Path::getFileExtension( m_textureName ) ) );
            createCubemapWorkspaces();
            setEnable( wasEnabled );
            notifyAutomaticMaterialStateChanged();
        }
    }

    auto CCubemapOgreNext::getTexture() const -> SmartPtr<ITexture>
    {
        return m_texture;
    }

    bool CCubemapOgreNext::getAutoApplyToMaterials() const
    {
        return m_autoApplyToMaterials;
    }

    void CCubemapOgreNext::setAutoApplyToMaterials( bool autoApply )
    {
        if( m_autoApplyToMaterials != autoApply )
        {
            m_autoApplyToMaterials = autoApply;
            notifyAutomaticMaterialStateChanged();
        }
    }

    IGraphicsCubemap::ProjectionMode CCubemapOgreNext::getProjectionMode() const
    {
        return m_projectionMode;
    }

    void CCubemapOgreNext::setProjectionMode( ProjectionMode projectionMode )
    {
        if( m_projectionMode != projectionMode )
        {
            m_projectionMode = projectionMode;
            notifyAutomaticMaterialStateChanged();
        }
    }

    IGraphicsCubemap::InfluenceShape CCubemapOgreNext::getInfluenceShape() const
    {
        return m_influenceShape;
    }

    void CCubemapOgreNext::setInfluenceShape( InfluenceShape influenceShape )
    {
        if( m_influenceShape != influenceShape )
        {
            m_influenceShape = influenceShape;
            notifyAutomaticMaterialStateChanged();
        }
    }

    f32 CCubemapOgreNext::getSphereRadius() const
    {
        return m_sphereRadius;
    }

    void CCubemapOgreNext::setSphereRadius( f32 sphereRadius )
    {
        const auto value = sanitizeFinite( sphereRadius, m_sphereRadius, 0.0f, 1000000.0f );
        if( m_sphereRadius != value )
        {
            m_sphereRadius = value;
            notifyAutomaticMaterialStateChanged();
        }
    }

    Vector3F CCubemapOgreNext::getBoxExtents() const
    {
        return m_boxExtents;
    }

    void CCubemapOgreNext::setBoxExtents( const Vector3F &boxExtents )
    {
        Vector3F value( sanitizeFinite( boxExtents[0], m_boxExtents[0], 0.0f, 1000000.0f ),
                        sanitizeFinite( boxExtents[1], m_boxExtents[1], 0.0f, 1000000.0f ),
                        sanitizeFinite( boxExtents[2], m_boxExtents[2], 0.0f, 1000000.0f ) );

        if( m_boxExtents != value )
        {
            m_boxExtents = value;
            notifyAutomaticMaterialStateChanged();
        }
    }

    f32 CCubemapOgreNext::getBlendDistance() const
    {
        return m_blendDistance;
    }

    void CCubemapOgreNext::setBlendDistance( f32 blendDistance )
    {
        const auto value = sanitizeFinite( blendDistance, m_blendDistance, 0.0f, 1000000.0f );
        if( m_blendDistance != value )
        {
            m_blendDistance = value;
            notifyAutomaticMaterialStateChanged();
        }
    }

    f32 CCubemapOgreNext::getImportance() const
    {
        return m_importance;
    }

    void CCubemapOgreNext::setImportance( f32 importance )
    {
        const auto value = sanitizeFinite( importance, m_importance, 0.0f, 1000000.0f );
        if( m_importance != value )
        {
            m_importance = value;
            notifyAutomaticMaterialStateChanged();
        }
    }

    f32 CCubemapOgreNext::getIntensity() const
    {
        return m_intensity;
    }

    void CCubemapOgreNext::setIntensity( f32 intensity )
    {
        const auto value = sanitizeFinite( intensity, m_intensity, 0.0f, 64.0f );
        if( m_intensity != value )
        {
            m_intensity = value;
            notifyAutomaticMaterialStateChanged();
        }
    }

    bool CCubemapOgreNext::getAutoEnableByDistance() const
    {
        return m_autoEnableByDistance;
    }

    void CCubemapOgreNext::setAutoEnableByDistance( bool autoEnableByDistance )
    {
        if( m_autoEnableByDistance != autoEnableByDistance )
        {
            m_autoEnableByDistance = autoEnableByDistance;
            notifyAutomaticMaterialStateChanged();
        }
    }

    f32 CCubemapOgreNext::getEnableDistanceThreshold() const
    {
        return m_enableDistanceThreshold;
    }

    void CCubemapOgreNext::setEnableDistanceThreshold( f32 distanceThreshold )
    {
        const auto value = sanitizeFinite( distanceThreshold, m_enableDistanceThreshold, 0.0f,
                                           100000000.0f );
        if( m_enableDistanceThreshold != value )
        {
            m_enableDistanceThreshold = value;
            notifyAutomaticMaterialStateChanged();
        }
    }

    bool CCubemapOgreNext::isAutomaticMaterialCandidate() const
    {
        return m_autoApplyToMaterials && m_texture && m_cubemapTexture;
    }

    bool CCubemapOgreNext::canAffectPosition( const Vector3F &position ) const
    {
        if( !isAutomaticMaterialCandidate() )
        {
            return false;
        }

        const auto centerDistance = distanceBetween( position, m_position );
        if( m_autoEnableByDistance && m_enableDistanceThreshold > 0.0f &&
            centerDistance > m_enableDistanceThreshold )
        {
            return false;
        }

        if( m_influenceShape == InfluenceShape::Box )
        {
            if( m_boxExtents[0] <= 0.0f || m_boxExtents[1] <= 0.0f || m_boxExtents[2] <= 0.0f )
            {
                return false;
            }

            return boxOutsideDistance( position, m_position, m_boxExtents ) <= m_blendDistance;
        }

        if( m_sphereRadius <= 0.0f )
        {
            return false;
        }

        return centerDistance <= ( m_sphereRadius + m_blendDistance );
    }

    f32 CCubemapOgreNext::getInfluenceScore( const Vector3F &position ) const
    {
        if( !canAffectPosition( position ) )
        {
            return -std::numeric_limits<f32>::max();
        }

        const auto centerDistance = distanceBetween( position, m_position );
        auto falloff = 0.0f;

        if( m_influenceShape == InfluenceShape::Box )
        {
            const auto outsideDistance = boxOutsideDistance( position, m_position, m_boxExtents );
            falloff = boundaryFalloff( outsideDistance, m_blendDistance );
        }
        else
        {
            falloff = boundaryFalloff( centerDistance - m_sphereRadius, m_blendDistance );
        }

        return m_importance * 1000000.0f + falloff * 1000.0f - centerDistance;
    }

    Ogre::TextureGpu *CCubemapOgreNext::getCubemapTexture() const
    {
        return m_cubemapTexture;
    }

    void CCubemapOgreNext::setResolution( u32 resolution )
    {
        m_resolution = resolution;
    }

    u32 CCubemapOgreNext::getResolution() const
    {
        return m_resolution;
    }

    void CCubemapOgreNext::setNearClipDistance( f32 nearClip )
    {
        m_nearClip = nearClip;
        if( m_camera )
        {
            m_camera->setNearClipDistance( nearClip );
        }
    }

    f32 CCubemapOgreNext::getNearClipDistance() const
    {
        return m_nearClip;
    }

    void CCubemapOgreNext::setFarClipDistance( f32 farClip )
    {
        m_farClip = farClip;
        if( m_camera )
        {
            m_camera->setFarClipDistance( farClip );
        }
    }

    f32 CCubemapOgreNext::getFarClipDistance() const
    {
        return m_farClip;
    }

    bool CCubemapOgreNext::getUpdateAllFaces() const
    {
        return m_timeSlicingMode == TimeSlicingMode::AllFacesAtOnce;
    }

    void CCubemapOgreNext::setUpdateAllFaces( bool updateAllFaces )
    {
        setTimeSlicingMode( updateAllFaces ? TimeSlicingMode::AllFacesAtOnce
                                           : TimeSlicingMode::OneFacePerFrame );
    }

    Ogre::PixelFormatGpu CCubemapOgreNext::getPixelFormat() const
    {
        return m_pixelFormat;
    }

    void CCubemapOgreNext::setPixelFormat( Ogre::PixelFormatGpu format )
    {
        m_pixelFormat = format;
    }

    Ogre::ColourValue CCubemapOgreNext::getBackgroundColour() const
    {
        return m_backgroundColor;
    }

    void CCubemapOgreNext::setBackgroundColour( const Ogre::ColourValue &colour )
    {
        if( m_backgroundColor != colour )
        {
            m_backgroundColor = colour;

            if( m_renderToTexture && m_cubemapTexture && m_camera && m_sceneMgr )
            {
                createCubemapWorkspaces();
            }

            notifyAutomaticMaterialStateChanged();
        }
    }

    void CCubemapOgreNext::registerWithScene()
    {
        if( m_registeredWithScene )
        {
            return;
        }

        if( auto scene = getOgreNextScene() )
        {
            scene->registerCubemap( this );
            m_registeredWithScene = true;
        }
    }

    void CCubemapOgreNext::unregisterFromScene()
    {
        if( !m_registeredWithScene )
        {
            return;
        }

        if( auto scene = getOgreNextScene() )
        {
            scene->unregisterCubemap( this );
        }

        m_registeredWithScene = false;
    }

    void CCubemapOgreNext::notifyAutomaticMaterialStateChanged()
    {
        if( m_registeredWithScene )
        {
            if( auto scene = getOgreNextScene() )
            {
                scene->refreshAutomaticCubemaps();
            }
        }
    }

    CGraphicsSceneOgreNext *CCubemapOgreNext::getOgreNextScene() const
    {
        auto scene = workphone::dynamic_pointer_cast<CGraphicsSceneOgreNext>( m_graphicsScene );
        return scene.get();
    }

    // CubemapFrameListener implementation
    CCubemapOgreNext::CubemapFrameListener::CubemapFrameListener( CCubemapOgreNext *cubemap ) :
        m_cubemap( cubemap )
    {
    }

    CCubemapOgreNext::CubemapFrameListener::~CubemapFrameListener() = default;

    auto CCubemapOgreNext::CubemapFrameListener::frameEnded( const Ogre::FrameEvent &evt ) -> bool
    {
        return true;
    }

    auto CCubemapOgreNext::CubemapFrameListener::frameStarted( const Ogre::FrameEvent &evt ) -> bool
    {
        if( !m_cubemap || !m_cubemap->m_enable )
            return true;

        if( m_cubemap->m_updateMode == UpdateMode::OnDemand )
        {
            if( m_cubemap->m_pendingFaceMask == 0u )
            {
                return true;
            }

            if( m_cubemap->m_timeSlicingMode == TimeSlicingMode::AllFacesAtOnce )
            {
                const auto pendingFaceMask = m_cubemap->m_pendingFaceMask;
                m_cubemap->m_pendingFaceMask = 0u;
                m_cubemap->renderFaces( pendingFaceMask );
                m_cubemap->completeRequestedUpdate();
            }
            else
            {
                const auto faceIndex = m_cubemap->findNextFace( m_cubemap->m_pendingFaceMask );
                if( faceIndex < 6u )
                {
                    m_cubemap->renderFace( faceIndex );
                    m_cubemap->m_pendingFaceMask &= ~( 1u << faceIndex );
                    m_cubemap->m_currentIndex = ( faceIndex + 1u ) % 6u;
                }

                if( m_cubemap->m_pendingFaceMask == 0u )
                {
                    m_cubemap->completeRequestedUpdate();
                }
            }

            return true;
        }

        m_accumulatedTime += evt.timeSinceLastFrame * 1000.0f;

        // Check if enough time has passed for an update
        if( m_cubemap->m_updateInterval > 0 &&
            m_accumulatedTime < static_cast<f32>( m_cubemap->m_updateInterval ) )
        {
            return true;
        }

        m_accumulatedTime = 0.0f;

        // Optionally, update one face per frame for performance
        // This spreads the cubemap update over 6 frames
        if( m_cubemap->m_timeSlicingMode == TimeSlicingMode::AllFacesAtOnce )
        {
            m_cubemap->render();
        }
        else
        {
            const auto faceIndex = m_cubemap->findNextFace( m_cubemap->m_faceMask );
            if( faceIndex < 6u )
            {
                m_cubemap->renderFace( faceIndex );
                m_cubemap->m_currentIndex = ( faceIndex + 1u ) % 6u;
            }
        }

        return true;
    }

    auto CCubemapOgreNext::CubemapFrameListener::frameRenderingQueued( const Ogre::FrameEvent &evt )
        -> bool
    {
        return true;
    }

    // CubemapRTListener implementation
    CCubemapOgreNext::CubemapRTListener::CubemapRTListener( CCubemapOgreNext *cubemap ) :
        m_cubemap( cubemap )
    {
    }

    CCubemapOgreNext::CubemapRTListener::~CubemapRTListener() = default;

}  // namespace workphone::render
