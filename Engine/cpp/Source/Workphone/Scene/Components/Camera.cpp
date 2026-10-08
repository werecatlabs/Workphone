#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Scene/Components/RenderTexture.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Jobs/CameraManagerReset.hpp>

namespace workphone::scene
{
    const String Camera::s_zOrderStr = "zOrder";
    const String Camera::s_isActiveStr = "isActive";
    const String Camera::s_enableShadowsStr = "enableShadows";
    const String Camera::s_visibilityMaskStr = "visibilityMask";
    const String Camera::s_resetStr = "reset";
    const String Camera::s_updateActiveStateStr = "updateActiveState";
    const String Camera::s_viewportBackgroundColourStr = "viewportBackgroundColour";
    const String Camera::s_enableSceneRenderStr = "enableSceneRender";
    const String Camera::s_enableUIStr = "enableUI";
    const String Camera::s_clearEveryFrameStr = "clearEveryFrame";
    const String Camera::s_overlaysEnabledStr = "overlaysEnabled";
    const String Camera::s_autoUpdatedStr = "autoUpdated";
    const String Camera::s_fovStr = "fov";
    const String Camera::s_nearClipStr = "nearClip";
    const String Camera::s_farClipStr = "farClip";
    const String Camera::s_orthoWidthStr = "orthoWidth";
    const String Camera::s_orthoHeightStr = "orthoHeight";
    const String Camera::s_outputRenderTextureStr = "outputRenderTexture";
    const String Camera::s_postProcessEnabledStr = "postProcessing.enabled";
    const String Camera::s_postProcessWorkspaceStr = "postProcessing.workspace";
    const String Camera::s_fxaaStr = "postProcessing.fxaa";
    const String Camera::s_bloomStr = "postProcessing.bloom";
    const String Camera::s_exposureStr = "postProcessing.exposure";
    const String Camera::s_gammaStr = "postProcessing.gamma";
    const String Camera::s_contrastStr = "postProcessing.contrast";
    const String Camera::s_saturationStr = "postProcessing.saturation";
    const String Camera::s_bloomIntensityStr = "postProcessing.bloomIntensity";
    const String Camera::s_bloomThresholdStr = "postProcessing.bloomThreshold";
    const String Camera::s_vignetteStr = "postProcessing.vignette";
    const String Camera::s_compositeLayerPrefix = "compositing.layer";

    WP_CLASS_REGISTER_DERIVED( workphone::scene, Camera, Component );
    u32 Camera::m_nameExt = 0;
    u32 Camera::m_zorderExt = 0;
    u32 Camera::m_vpExt = 0;

    Camera::Camera() : Component( Camera::typeInfo() )
    {
        constexpr u32 maxEditorLayers = 4;
        m_compositeRenderTextures.resize( maxEditorLayers );
        m_compositeBlendModes.resize( maxEditorLayers,
                                      render::IGraphicsCamera::CompositeBlendMode::Alpha );
        m_compositeOpacities.resize( maxEditorLayers, 1.0f );
        m_compositeEnabled.resize( maxEditorLayers, false );
    }

    Camera::~Camera()
    {
        WP_ASSERT( isLoaded() == false );

        m_targetTexture = nullptr;
        m_viewport = nullptr;
        m_camera = nullptr;
        m_node = nullptr;

        m_outputRenderTexture = nullptr;

        WP_ASSERT( m_compositeRenderTextures.empty() );
        WP_ASSERT( m_compositeBlendModes.empty() );
        WP_ASSERT( m_compositeOpacities.empty() );
        WP_ASSERT( m_compositeEnabled.empty() );

        m_compositeRenderTextures.clear();
        m_compositeBlendModes.clear();
        m_compositeOpacities.clear();
        m_compositeEnabled.clear();
    }

    void Camera::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto sceneManager = applicationManager->getGameManagerPtr();

            Component::load( data );

            createRenderCamera();

            sceneManager->registerComponentUpdate( TaskId::Render, Thread::UpdateState::Transform,
                                                   this );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Camera::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            auto sceneManager = applicationManager->getGameManager();

            m_compositeRenderTextures.clear();
            m_compositeBlendModes.clear();
            m_compositeOpacities.clear();
            m_compositeEnabled.clear();

            destroyRenderCamera();

            sceneManager->unregisterAllComponent( this );

            m_targetTexture = nullptr;
            m_viewport = nullptr;
            m_camera = nullptr;
            m_node = nullptr;

            m_outputRenderTexture = nullptr;

            Component::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto Camera::getTargetTexture() const -> SmartPtr<render::ITexture>
    {
        return m_targetTexture;
    }

    void Camera::setTargetTexture( SmartPtr<render::ITexture> targetTexture )
    {
        if( m_targetTexture == targetTexture )
        {
            return;
        }

        if( m_viewport )
        {
            auto oldViewport = m_viewport;
            auto oldRenderTarget = oldViewport->getRenderTarget();
            oldViewport->setRenderTarget( nullptr );
            if( oldRenderTarget )
            {
                oldRenderTarget->removeViewport( oldViewport );
            }
            m_viewport = nullptr;
        }

        m_targetTexture = targetTexture;

        if( m_camera )
        {
            m_camera->setTargetTexture( targetTexture );
            createViewport();
        }

        auto active = isActive();
        updateActiveState( active );
    }

    auto Camera::getProperties() const -> SmartPtr<Properties>
    {
        if( auto properties = Component::getProperties() )
        {
            properties->setProperty( s_zOrderStr, m_zOrder );
            properties->setProperty( s_isActiveStr, m_isActive );
            properties->setProperty( s_enableShadowsStr, m_enableShadows );
            properties->setProperty( s_visibilityMaskStr, m_visibilityMask );
            properties->setProperty( s_viewportBackgroundColourStr, m_viewportBackgroundColour );
            properties->setProperty( s_enableSceneRenderStr, m_enableSceneRender );
            properties->setProperty( s_enableUIStr, m_enableUI );
            properties->setProperty( s_clearEveryFrameStr, m_clearEveryFrame );
            properties->setProperty( s_overlaysEnabledStr, m_overlaysEnabled );
            properties->setProperty( s_autoUpdatedStr, m_autoUpdated );
            properties->setProperty( s_fovStr, m_fov );
            properties->setProperty( s_nearClipStr, m_nearClip );
            properties->setProperty( s_farClipStr, m_farClip );
            properties->setProperty( s_orthoWidthStr, m_orthoWidth );
            properties->setProperty( s_orthoHeightStr, m_orthoHeight );
            properties->setProperty(
                s_outputRenderTextureStr,
                workphone::static_pointer_cast<IComponent>( getOutputRenderTexture() ) );
            properties->getPropertyObject( s_outputRenderTextureStr )
                .setAttribute( "resourceType", "RenderTexture" );
            properties->setProperty( s_postProcessEnabledStr, m_postProcessSettings.enabled );
            properties->setProperty( s_postProcessWorkspaceStr, m_postProcessSettings.workspace );
            properties->setProperty( s_fxaaStr, m_postProcessSettings.fxaa );
            properties->setProperty( s_bloomStr, m_postProcessSettings.bloom );
            properties->setProperty( s_exposureStr, m_postProcessSettings.exposure );
            properties->setProperty( s_gammaStr, m_postProcessSettings.gamma );
            properties->setProperty( s_contrastStr, m_postProcessSettings.contrast );
            properties->setProperty( s_saturationStr, m_postProcessSettings.saturation );
            properties->setProperty( s_bloomIntensityStr, m_postProcessSettings.bloomIntensity );
            properties->setProperty( s_bloomThresholdStr, m_postProcessSettings.bloomThreshold );
            properties->setProperty( s_vignetteStr, m_postProcessSettings.vignette );

            for( u32 i = 0; i < m_compositeRenderTextures.size(); ++i )
            {
                const auto prefix = s_compositeLayerPrefix + StringUtil::toString( i );
                auto component = m_compositeRenderTextures[i];
                properties->setProperty( prefix + ".renderTexture",
                                         workphone::static_pointer_cast<IComponent>( component ) );
                properties->getPropertyObject( prefix + ".renderTexture" )
                    .setAttribute( "resourceType", "RenderTexture" );
                properties->setProperty( prefix + ".enabled", m_compositeEnabled[i] );
                properties->setPropertyAsEnum( prefix + ".blendMode",
                                               static_cast<s32>( m_compositeBlendModes[i] ),
                                               { "Replace", "Alpha", "Add", "Multiply", "Screen" } );
                properties->setProperty( prefix + ".opacity", m_compositeOpacities[i] );
            }
            properties->setButtonPressed( s_resetStr );
            properties->setButtonPressed( s_updateActiveStateStr );
            return properties;
        }

        return nullptr;
    }

    void Camera::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto cameraManager = applicationManager->getCameraManager();

        const auto wasEnabled = isEnabled();

        properties->getPropertyValue( s_zOrderStr, m_zOrder );
        setZOrder( m_zOrder );

        auto active = isActive();
        properties->getPropertyValue( s_isActiveStr, active );
        properties->getPropertyValue( s_enableShadowsStr, m_enableShadows );
        properties->getPropertyValue( s_visibilityMaskStr, m_visibilityMask );
        properties->getPropertyValue( s_viewportBackgroundColourStr, m_viewportBackgroundColour );
        properties->getPropertyValue( s_enableSceneRenderStr, m_enableSceneRender );
        properties->getPropertyValue( s_enableUIStr, m_enableUI );
        properties->getPropertyValue( s_clearEveryFrameStr, m_clearEveryFrame );
        properties->getPropertyValue( s_overlaysEnabledStr, m_overlaysEnabled );
        properties->getPropertyValue( s_autoUpdatedStr, m_autoUpdated );

        SmartPtr<IComponent> outputComponent;
        if( properties->hasProperty( s_outputRenderTextureStr ) )
        {
            properties->getPropertyValue( s_outputRenderTextureStr, outputComponent );
            setOutputRenderTexture( workphone::dynamic_pointer_cast<RenderTexture>( outputComponent ) );
        }

        properties->getPropertyValue( s_postProcessEnabledStr, m_postProcessSettings.enabled );
        properties->getPropertyValue( s_postProcessWorkspaceStr, m_postProcessSettings.workspace );
        properties->getPropertyValue( s_fxaaStr, m_postProcessSettings.fxaa );
        properties->getPropertyValue( s_bloomStr, m_postProcessSettings.bloom );
        properties->getPropertyValue( s_exposureStr, m_postProcessSettings.exposure );
        properties->getPropertyValue( s_gammaStr, m_postProcessSettings.gamma );
        properties->getPropertyValue( s_contrastStr, m_postProcessSettings.contrast );
        properties->getPropertyValue( s_saturationStr, m_postProcessSettings.saturation );
        properties->getPropertyValue( s_bloomIntensityStr, m_postProcessSettings.bloomIntensity );
        properties->getPropertyValue( s_bloomThresholdStr, m_postProcessSettings.bloomThreshold );
        properties->getPropertyValue( s_vignetteStr, m_postProcessSettings.vignette );

        for( u32 i = 0; i < m_compositeRenderTextures.size(); ++i )
        {
            const auto prefix = s_compositeLayerPrefix + StringUtil::toString( i );
            SmartPtr<IComponent> component;
            if( properties->hasProperty( prefix + ".renderTexture" ) )
            {
                properties->getPropertyValue( prefix + ".renderTexture", component );
            }

            s32 blendMode = static_cast<s32>( m_compositeBlendModes[i] );
            bool layerEnabled = m_compositeEnabled[i];
            properties->getPropertyValue( prefix + ".blendMode", blendMode );
            properties->getPropertyValue( prefix + ".opacity", m_compositeOpacities[i] );
            properties->getPropertyValue( prefix + ".enabled", layerEnabled );
            setCompositeRenderTexture(
                i, workphone::dynamic_pointer_cast<RenderTexture>( component ),
                static_cast<render::IGraphicsCamera::CompositeBlendMode>( blendMode ),
                m_compositeOpacities[i], layerEnabled );
        }

        setPostProcessSettings( m_postProcessSettings );

        f32 fov = m_fov;
        if( properties->getPropertyValue( s_fovStr, fov ) )
        {
            setFOV( fov );
        }

        f32 nearClip = m_nearClip;
        if( properties->getPropertyValue( s_nearClipStr, nearClip ) )
        {
            setNearClipDistance( nearClip );
        }

        f32 farClip = m_farClip;
        if( properties->getPropertyValue( s_farClipStr, farClip ) )
        {
            setFarClipDistance( farClip );
        }

        f32 orthoWidth = m_orthoWidth;
        if( properties->getPropertyValue( s_orthoWidthStr, orthoWidth ) )
        {
            setOrthoWindowWidth( orthoWidth );
        }

        f32 orthoHeight = m_orthoHeight;
        if( properties->getPropertyValue( s_orthoHeightStr, orthoHeight ) )
        {
            setOrthoWindowHeight( orthoHeight );
        }

        if( properties->isButtonPressed( s_resetStr ) )
        {
            cameraManager->reset();
        }

        if( properties->isButtonPressed( s_updateActiveStateStr ) )
        {
            m_isActive = true;
            updateActiveState( m_isActive );
        }

        Component::setProperties( properties );

        const auto enabled = isEnabled();

        if( wasEnabled != enabled )
        {
            cameraManager->reset();
        }

        if( m_isActive != active )
        {
            m_isActive = active;
            cameraManager->reset();
        }

        if( m_camera )
        {
            m_camera->setVisible( m_isActive );
            m_camera->setAutoAspectRatio( true );
        }

        if( auto vp = getViewport() )
        {
            vp->setBackgroundColour( m_viewportBackgroundColour );
            vp->setEnableSceneRender( m_enableSceneRender );
            vp->setEnableUI( m_enableUI );
            vp->setClearEveryFrame( m_clearEveryFrame );
            vp->setOverlaysEnabled( m_overlaysEnabled );
            vp->setAutoUpdated( m_autoUpdated );
            vp->setShadowsEnabled( m_enableShadows );
            vp->setVisibilityMask( m_visibilityMask );
            vp->setActive( m_isActive );
        }

        updateSmoothTransformState();
    }

    auto Camera::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto objects = Component::getChildObjects();

        if( m_targetTexture )
        {
            objects.emplace_back( m_targetTexture );
        }
        if( m_camera )
        {
            objects.emplace_back( m_camera );
        }
        if( m_viewport )
        {
            objects.emplace_back( m_viewport );
        }
        if( m_node )
        {
            objects.emplace_back( m_node );
        }

        return objects;
    }

    auto Camera::getCamera() const -> SmartPtr<render::IGraphicsCamera>
    {
        return m_camera;
    }

    void Camera::setCamera( SmartPtr<render::IGraphicsCamera> camera )
    {
        m_camera = camera;
    }

    auto Camera::getNode() const -> SmartPtr<render::IGraphicsSceneNode>
    {
        return m_node;
    }

    void Camera::setNode( SmartPtr<render::IGraphicsSceneNode> node )
    {
        m_node = node;
    }

    auto Camera::isActive() const -> bool
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto actor = getActorPtr();
        if( actor )
        {
            if( applicationManager->isEditorCamera() )
            {
                if( actor->getFlag( IGameActor::ActorFlagIsEditor ) )
                {
                    return true;
                }
            }

            auto enabled = isEnabled() && actor->isEnabledInScene();
            return enabled && m_isActive;
        }

        return false;
    }

    void Camera::setActive( bool active )
    {
        m_isActive = active;

        updateActiveState( m_isActive );

        if( m_isActive )
        {
            if( auto actor = getActor() )
            {
                actor->updateTransform();
            }
        }
    }

    void Camera::updateTransform()
    {
        if( m_camera )
        {
            if( auto output = getOutputRenderTexture() )
            {
                auto texture = output->getTexture();
                if( texture && texture != getTargetTexture() )
                {
                    setTargetTexture( texture );
                }
            }

            const auto layers = getCompositeLayers();
            const auto currentLayers = m_camera->getCompositeLayers();
            auto layersChanged = layers.size() != currentLayers.size();
            if( !layersChanged )
            {
                for( u32 i = 0; i < layers.size(); ++i )
                {
                    if( layers[i].texture != currentLayers[i].texture ||
                        layers[i].blendMode != currentLayers[i].blendMode ||
                        layers[i].opacity != currentLayers[i].opacity ||
                        layers[i].enabled != currentLayers[i].enabled )
                    {
                        layersChanged = true;
                        break;
                    }
                }
            }

            if( layersChanged )
            {
                m_camera->setCompositeLayers( layers );
            }
        }

        auto task = Thread::getCurrentTask();
        switch( task )
        {
        case TaskId::Application:
        {
            auto state = getState();
            switch( state )
            {
            case State::Edit:
            case State::Play:
            {
                if( isActive() )
                {
                    if( Thread::getTaskFlag( Thread::Application_Flag ) )
                    {
                        if( auto actor = getActorPtr() )
                        {
                            if( !actor->isSmoothMotion() )
                            {
                                if( auto actorTransform = actor->getTransformPtr() )
                                {
                                    auto t = actorTransform->getWorldTransform();

                                    if( auto node = getNode() )
                                    {
                                        node->setTransform( t );
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
            default:
            {
            }
            break;
            }
        }
        break;
        case TaskId::Render:
        {
        }
        break;
        default:
        {
        }
        break;
        };
    }

    void Camera::updateTransform( const Transform3<real_Num> &t )
    {
        auto state = getState();
        switch( state )
        {
        case State::Edit:
        case State::Play:
        {
            if( isActive() )
            {
                if( auto actor = getActorPtr() )
                {
                    if( actor->isSmoothMotion() )
                    {
                        if( auto node = getNode() )
                        {
                            node->setTransform( t );
                        }
                    }
                }
            }
        }
        break;
        default:
        {
        }
        };
    }

    auto Camera::getCameraToViewportRay( const Vector2<real_Num> &screenPosition ) -> Ray3<real_Num>
    {
        if( auto camera = getCamera() )
        {
            return camera->getRay( screenPosition.x, screenPosition.y );
        }

        return {};
    }

    auto Camera::isInFrustum( const AABB3<real_Num> &box ) const -> bool
    {
        if( m_camera )
        {
            return m_camera->isObjectVisible( box );
        }

        return true;
    }

    void Camera::updateOrder()
    {
        if( auto actor = getActorPtr() )
        {
            if( actor->getFlag( IGameActor::ActorFlagIsEditor ) )
            {
                setZOrder( 100 );
                return;
            }
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManagerPtr();
        if( !sceneManager )
            return;

        auto scene = sceneManager->getCurrentScenePtr();
        if( !scene )
            return;

        auto zorder = 1;

        auto cameras = Array<SmartPtr<Camera>>();
        cameras.reserve( 12 );

        auto actors = scene->getActors();
        for( auto &sceneActor : actors )
        {
            auto actorCameras = sceneActor->getComponentsAndInChildren<Camera>();
            for( auto camera : actorCameras )
            {
                cameras.push_back( camera );
            }
        }

        for( auto &camera : cameras )
        {
            if( camera == this )
            {
                break;
            }

            ++zorder;
        }

        setZOrder( zorder );
    }

    auto Camera::getZOrder( SmartPtr<IGameActor> other ) -> s32
    {
        if( auto actor = getActor() )
        {
            auto children = actor->getAllComponentsInChildren<Camera>();

            auto count = 100;
            for( auto &child : children )
            {
                if( child->getActor() == other )
                {
                    return count;
                }

                ++count;
            }
        }

        return 0;
    }

    auto Camera::getZOrder() const -> u32
    {
        return m_zOrder;
    }

    void Camera::setZOrder( u32 zOrder )
    {
        m_zOrder = zOrder;

        if( m_viewport )
        {
            m_viewport->setZOrder( zOrder );
        }
    }

    void Camera::createViewport()
    {
        if( auto camera = getCamera() )
        {
            if( !m_viewport )
            {
                if( auto renderTarget = getRenderTarget() )
                {
                    if( m_zOrder == 0u )
                    {
                        updateOrder();
                    }

                    auto zOrder = getZOrder();
                    auto vp = renderTarget->addViewport( m_vpExt++, camera, zOrder );
                    WP_ASSERT( vp );

                    vp->setBackgroundColour( m_viewportBackgroundColour );
                    vp->setEnableSceneRender( m_enableSceneRender );
                    vp->setEnableUI( m_enableUI );
                    vp->setClearEveryFrame( m_clearEveryFrame );
                    vp->setOverlaysEnabled( m_overlaysEnabled );
                    vp->setAutoUpdated( m_autoUpdated );
                    vp->setShadowsEnabled( m_enableShadows );
                    vp->setVisibilityMask( m_visibilityMask );
                    vp->setActive( m_isActive );

                    m_viewport = vp;
                }
            }
        }
    }

    void Camera::createRenderCamera()
    {
        if( m_camera )
        {
            return;
        }

        if( auto actor = getActorPtr() )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto sceneManager = applicationManager->getGameManagerPtr();

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( !graphicsSystem )
            {
                WP_LOG_ERROR( "Graphics system is not available" );
                setLoadingState( LoadingState::Loaded );
                return;
            }

            auto window = graphicsSystem->getDefaultWindow();
            WP_ASSERT( window );

            auto smgr = graphicsSystem->getGraphicsScenePtr();
            WP_ASSERT( smgr );

            auto camera = smgr->addGraphicsObjectByType<render::IGraphicsCamera>();
            WP_ASSERT( camera );

            auto cameraName = String( "Camera" ) + StringUtil::toString( m_nameExt++ );

            if( !actor->getFlag( IGameActor::ActorFlagIsEditor ) )
            {
                cameraName = String( "SceneCamera: " ) + cameraName;
            }
            else
            {
                cameraName = String( "EditorCamera: " ) + cameraName;
            }

            camera->setName( cameraName );

            m_camera = camera;

            m_node = m_camera->getOwner();
            if( !m_node )
            {
                auto rootNode = smgr->getRootSceneNode();
                WP_ASSERT( rootNode );

                if( rootNode )
                {
                    const auto cameraComponentNodeName = String( "CameraComponentNode" );
                    const auto cameraNodeName =
                        cameraComponentNodeName + StringUtil::toString( m_nameExt++ );
                    m_node = rootNode->addChildSceneNode( cameraNodeName );
                    if( m_node )
                    {
                        m_node->attachObject( m_camera );
                    }
                }
            }

            auto cameraManager = applicationManager->getCameraManager();
            if( cameraManager )
            {
                auto cameraActor = getActor();
                cameraManager->addCamera( cameraActor );
            }

            m_camera->setVisible( false );
            m_camera->setAutoAspectRatio( true );
            m_camera->setFOVy( m_fov );
            m_camera->setNearClipDistance( m_nearClip );
            m_camera->setFarClipDistance( m_farClip );
            m_camera->setPostProcessSettings( m_postProcessSettings );
            m_camera->setCompositeLayers( getCompositeLayers() );

            if( auto outputRenderTexture = getOutputRenderTexture() )
            {
                setTargetTexture( outputRenderTexture->getTexture() );
            }
        }
    }

    void Camera::destroyRenderCamera()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto sceneManager = applicationManager->getGameManagerPtr();

        if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
        {
            auto window = graphicsSystem->getDefaultWindow();
            WP_ASSERT( window );

            auto smgr = graphicsSystem->getGraphicsScene();
            WP_ASSERT( smgr );

            if( m_viewport )
            {
                m_viewport->setRenderTarget( nullptr );

                if( auto renderTarget = getRenderTarget() )
                {
                    renderTarget->removeViewport( m_viewport );
                }
                else
                {
                    window->removeViewport( m_viewport );
                }

                m_viewport = nullptr;
            }

            m_targetTexture = nullptr;

            auto cameraManager = applicationManager->getCameraManager();
            if( cameraManager )
            {
                auto cameraActor = getActor();
                cameraManager->removeCamera( cameraActor );
            }

            if( m_camera )
            {
                m_camera->setVisible( false );
            }

            if( m_viewport )
            {
                m_viewport->setCamera( nullptr );
            }

            if( m_camera )
            {
                m_camera->setTargetTexture( nullptr );

                if( m_camera->isAttached() )
                {
                    if( m_node )
                    {
                        m_camera->detachFromParent( m_node );
                    }
                }

                smgr->removeGraphicsObject( m_camera );
                m_camera = nullptr;
            }

            if( m_node )
            {
                smgr->removeSceneNode( m_node );
                m_node = nullptr;
            }
        }
    }

    auto Camera::getRenderTarget() const -> SmartPtr<render::IRenderTarget>
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        if( auto t = getTargetTexture() )
        {
            return t->getRenderTarget();
        }

        return applicationManager->getWindow();
    }

    void Camera::updateFlags( u32 flags, u32 oldFlags )
    {
        if( auto actor = getActorPtr() )
        {
            auto state = getState();
            switch( state )
            {
            case State::Edit:
            case State::Play:
            {
                Component::updateFlags( flags, oldFlags );

                auto applicationManager = core::IApplicationManager::instancePtr();
                auto taskManager = applicationManager->getTaskManagerPtr();
                auto factoryManager = applicationManager->getFactoryManagerPtr();
                auto cameraManager = applicationManager->getCameraManager();

                if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagInScene ) !=
                    BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagInScene ) )
                {
                    if( taskManager )
                    {
                        if( auto applicationTask = taskManager->getTask( TaskId::Application ) )
                        {
                            auto cameraManagerResetJob = factoryManager->make_ptr<CameraManagerReset>();
                            applicationTask->addJob( cameraManagerResetJob );
                        }
                    }
                }
                else if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagEnabled ) !=
                         BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagEnabled ) )
                {
                    if( taskManager )
                    {
                        if( auto applicationTask = taskManager->getTask( TaskId::Application ) )
                        {
                            auto cameraManagerResetJob = factoryManager->make_ptr<CameraManagerReset>();
                            applicationTask->addJob( cameraManagerResetJob );
                        }
                    }
                }

                auto active = isActive();
                updateActiveState( active );

                updateSmoothTransformState();
            }
            break;
            default:
            {
            }
            break;
            };
        }
    }

    auto Camera::handleComponentEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto taskManager = applicationManager->getTaskManagerPtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        if( !graphicsSystem )
        {
            return FSMReturnType::Failed;
        }

        auto smgr = graphicsSystem->getGraphicsScenePtr();
        WP_ASSERT( smgr );

        auto cameraManager = applicationManager->getCameraManager();

        switch( eventType )
        {
        case FSMEvent::Change:
        {
        }
        break;
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            {
                updateOrder();
                createRenderCamera();
                createViewport();

                auto active = isActive();
                updateActiveState( active );

                auto applicationTask = taskManager->getTaskPtr( TaskId::Application );
                auto cameraManagerResetJob = factoryManager->make_ptr<CameraManagerReset>();
                applicationTask->addJob( cameraManagerResetJob );
            }
            break;
            case State::Play:
            {
                updateOrder();
                createRenderCamera();
                createViewport();

                auto active = isActive();
                updateActiveState( active );

                auto applicationTask = taskManager->getTaskPtr( TaskId::Application );
                auto cameraManagerResetJob = factoryManager->make_ptr<CameraManagerReset>();
                applicationTask->addJob( cameraManagerResetJob );
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
            case State::Edit:
            case State::Play:
            {
                // Edit and Play share the camera and viewport. CameraManager owns
                // selection; a deferred state transition must not deactivate its
                // selected viewport after reset has already activated it.
            }
            break;
            default:
            {
            }
            }
        }
        break;
        default:
        {
        }
        break;
        };

        return FSMReturnType::Ok;
    }

    Parameter Camera::handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        Component::handleEvent( eventType, eventValue, arguments, sender, object, event );

        if( eventValue == IEvent::enabled )
        {
        }

        return {};
    }

    void Camera::updateActiveState( bool active )
    {
#if 0
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        if( applicationManager->isEditorCamera() )
        {
            if( auto actor = getActor() )
            {
                if( !actor->getFlag( scene::IGameActor::ActorFlagIsEditor ) )
                {
                    if( m_camera )
                    {
                        m_camera->setRenderUI( false );
                    }

                    if( m_viewport )
                    {
                        m_viewport->setEnableUI( false );
                        m_viewport->setActive( false );
                    }

                    if( m_camera )
                    {
                        m_camera->setVisible( false );
                    }
                }
                else
                {
                    if( m_camera )
                    {
                        m_camera->setRenderUI( false );
                    }

                    if( m_viewport )
                    {
                        m_viewport->setEnableUI( false );
                        m_viewport->setActive( active );
                    }

                    if( m_camera )
                    {
                        m_camera->setVisible( active );
                    }
                }
            }
        }
        else
        {
            if( m_camera )
            {
                m_camera->setRenderUI( false );
            }

            if( m_viewport )
            {
                m_viewport->setEnableUI( false );
                m_viewport->setActive( active );
            }

            if( m_camera )
            {
                m_camera->setVisible( active );
            }
        }
#else
        auto state = getState();
        switch( state )
        {
        case State::Edit:
        case State::Play:
        {
            if( m_camera )
            {
                m_camera->setTargetTexture( m_targetTexture );
                m_camera->setVisible( active );
            }

            if( m_viewport )
            {
                m_viewport->setActive( active );
            }
        }
        break;
        default:
        {
        }
        };
#endif
    }

    auto Camera::getViewport() const -> SmartPtr<render::IViewport>
    {
        return m_viewport;
    }

    void Camera::setViewport( SmartPtr<render::IViewport> viewport )
    {
        m_viewport = viewport;
    }

    bool Camera::getEnableShadows() const
    {
        return m_enableShadows;
    }

    void Camera::setEnableShadows( bool enableShadows )
    {
        m_enableShadows = enableShadows;

        if( auto vp = getViewport() )
        {
            vp->setShadowsEnabled( m_enableShadows );
        }
    }

    void Camera::updateSmoothTransformState()
    {
        if( auto actor = getActorPtr() )
        {
            auto transform = actor->getTransform();
            if( transform )
            {
                auto enabled = isEnabled() && actor->isEnabledInScene();
                if( enabled )
                {
                    if( actor->isSmoothMotion() )
                    {
                        transform->setTask( TaskId::Application );
                    }
                    else
                    {
                        transform->setTask( TaskId::None );
                    }
                }
                else
                {
                    transform->setTask( TaskId::None );
                }
            }
        }
    }

    void Camera::setOrthoWindowHeight( f32 height )
    {
        if( m_orthoHeight == height )
        {
            return;
        }

        m_orthoHeight = height;
    }

    f32 Camera::getOrthoWindowHeight() const
    {
        return m_orthoHeight;
    }

    void Camera::setOrthoWindowWidth( f32 width )
    {
        if( m_orthoWidth == width )
        {
            return;
        }

        m_orthoWidth = width;
    }

    f32 Camera::getOrthoWindowWidth() const
    {
        return m_orthoWidth;
    }

    f32 Camera::getFOV() const
    {
        return m_fov;
    }

    void Camera::setFOV( f32 fov )
    {
        if( m_fov == fov )
        {
            return;
        }

        m_fov = fov;

        if( m_camera )
        {
            m_camera->setFOVy( m_fov );
        }
    }

    f32 Camera::getNearClipDistance() const
    {
        return m_nearClip;
    }

    void Camera::setNearClipDistance( f32 nearDist )
    {
        if( m_nearClip == nearDist )
        {
            return;
        }

        m_nearClip = nearDist;

        if( m_camera )
        {
            m_camera->setNearClipDistance( m_nearClip );
        }
    }

    f32 Camera::getFarClipDistance() const
    {
        return m_farClip;
    }

    void Camera::setFarClipDistance( f32 farDist )
    {
        if( m_farClip == farDist )
        {
            return;
        }

        m_farClip = farDist;

        if( m_camera )
        {
            m_camera->setFarClipDistance( m_farClip );
        }
    }

    ColourF Camera::getViewportBackgroundColour() const
    {
        return m_viewportBackgroundColour;
    }

    void Camera::setViewportBackgroundColour( const ColourF &colour )
    {
        m_viewportBackgroundColour = colour;

        if( m_viewport )
        {
            m_viewport->setBackgroundColour( m_viewportBackgroundColour );
        }
    }

    bool Camera::getEnableSceneRender() const
    {
        return m_enableSceneRender;
    }

    void Camera::setEnableSceneRender( bool enable )
    {
        m_enableSceneRender = enable;

        if( m_viewport )
        {
            m_viewport->setEnableSceneRender( m_enableSceneRender );
        }
    }

    bool Camera::getEnableUI() const
    {
        return m_enableUI;
    }

    void Camera::setEnableUI( bool enable )
    {
        m_enableUI = enable;

        if( m_viewport )
        {
            m_viewport->setEnableUI( m_enableUI );
        }
    }

    bool Camera::getClearEveryFrame() const
    {
        return m_clearEveryFrame;
    }

    void Camera::setClearEveryFrame( bool clear )
    {
        m_clearEveryFrame = clear;

        if( m_viewport )
        {
            m_viewport->setClearEveryFrame( m_clearEveryFrame );
        }
    }

    bool Camera::getOverlaysEnabled() const
    {
        return m_overlaysEnabled;
    }

    void Camera::setOverlaysEnabled( bool enabled )
    {
        m_overlaysEnabled = enabled;

        if( m_viewport )
        {
            m_viewport->setOverlaysEnabled( m_overlaysEnabled );
        }
    }

    bool Camera::getAutoUpdated() const
    {
        return m_autoUpdated;
    }

    void Camera::setAutoUpdated( bool autoUpdated )
    {
        m_autoUpdated = autoUpdated;

        if( m_viewport )
        {
            m_viewport->setAutoUpdated( m_autoUpdated );
        }
    }

    auto Camera::getOutputRenderTexture() const -> SmartPtr<RenderTexture>
    {
        return m_outputRenderTexture;
    }

    void Camera::setOutputRenderTexture( SmartPtr<RenderTexture> renderTexture )
    {
        m_outputRenderTexture = renderTexture;
        setTargetTexture( renderTexture ? renderTexture->getTexture() : nullptr );
    }

    auto Camera::getPostProcessSettings() const -> render::IGraphicsCamera::PostProcessSettings
    {
        return m_postProcessSettings;
    }

    void Camera::setPostProcessSettings( const render::IGraphicsCamera::PostProcessSettings &settings )
    {
        m_postProcessSettings = settings;
        if( m_camera )
        {
            m_camera->setPostProcessSettings( settings );
        }
    }

    auto Camera::getCompositeLayers() const -> Array<render::IGraphicsCamera::CompositeLayer>
    {
        Array<render::IGraphicsCamera::CompositeLayer> layers;
        for( u32 i = 0; i < m_compositeRenderTextures.size(); ++i )
        {
            if( auto renderTexture = m_compositeRenderTextures[i] )
            {
                render::IGraphicsCamera::CompositeLayer layer;
                layer.texture = renderTexture->getTexture();
                layer.blendMode = m_compositeBlendModes[i];
                layer.opacity = m_compositeOpacities[i];
                layer.enabled = m_compositeEnabled[i];
                layers.push_back( layer );
            }
        }

        return layers;
    }

    void Camera::setCompositeRenderTexture( u32 index, SmartPtr<RenderTexture> renderTexture,
                                            render::IGraphicsCamera::CompositeBlendMode blendMode,
                                            f32 opacity, bool enabled )
    {
        if( index >= m_compositeRenderTextures.size() )
        {
            return;
        }

        m_compositeRenderTextures[index] = renderTexture;
        m_compositeBlendModes[index] = blendMode;
        m_compositeOpacities[index] = MathF::clamp( opacity, 0.0f, 1.0f );
        m_compositeEnabled[index] = enabled;

        if( m_camera )
        {
            m_camera->setCompositeLayers( getCompositeLayers() );
        }
    }

    void Camera::updateStatic()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto sceneManager = applicationManager->getGameManagerPtr();

        if( auto actor = getActorPtr() )
        {
            auto isstatic = actor->isStatic();
            if( auto graphicsNode = getNode() )
            {
                graphicsNode->setStatic( isstatic );
            }

            if( !isstatic )
            {
                sceneManager->registerComponentUpdate( TaskId::Render, Thread::UpdateState::Transform,
                                                       this );
            }
            else
            {
                sceneManager->unregisterComponentUpdate( TaskId::Render, Thread::UpdateState::Transform,
                                                         this );
            }
        }
    }

}  // namespace workphone::scene
