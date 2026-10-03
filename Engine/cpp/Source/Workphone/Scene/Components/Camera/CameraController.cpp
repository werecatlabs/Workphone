#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Camera/CameraController.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/System/ISelectionManager.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>

namespace workphone::scene
{
    const String CameraController::viewportIdStr = String( "viewportId" );
    const String CameraController::cameraFlagsStr = String( "cameraFlags" );
    const String CameraController::isActiveStr = String( "isActive" );
    const String CameraController::isMainCameraStr = String( "isMainCamera" );
    const String CameraController::isOrthographicStr = String( "isOrthographic" );

    WP_CLASS_REGISTER_DERIVED( workphone::scene, CameraController, Component );
    WP_CLASS_REGISTER_DERIVED( workphone::scene, CameraController::EventListener, IEventListener );

    // Camera controller flags
    const u32 CameraController::CameraFlags_None = 0;
    const u32 CameraController::CameraFlags_Active = 1 << 1;
    const u32 CameraController::CameraFlags_MainCamera = 1 << 2;
    const u32 CameraController::CameraFlags_Orthographic = 1 << 3;
    const u32 CameraController::CameraFlags_RenderToTexture = 1 << 4;
    const u32 CameraController::CameraFlags_PostProcessing = 1 << 5;

    // Property key strings
    const String CameraController::positionStr = "Position";
    const String CameraController::targetStr = "Target";
    const String CameraController::sphericalCoordsStr = "SphericalCoords";
    const String CameraController::rotationSpeedStr = "RotationSpeed";
    const String CameraController::zoomSpeedStr = "ZoomSpeed";
    const String CameraController::moveSpeedStr = "MoveSpeed";
    const String CameraController::maxDistanceStr = "MaxDistance";
    const String CameraController::nearDistanceStr = "NearDistance";

    CameraController::CameraController()
    {
        setComponentFlag( IComponent::ComponentEnabledFlag, true );
    }

    CameraController::~CameraController()
    {
    }

    void CameraController::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            Component::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            // Create and set up event listener
            auto eventListener = workphone::make_ptr<EventListener>();
            eventListener->setOwner( this );
            m_eventListener = eventListener;

            // Register with application manager for global events
            applicationManager->addObjectListener( eventListener );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CameraController::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( applicationManager && m_eventListener )
            {
                applicationManager->removeObjectListener( m_eventListener );
            }

            if( m_eventListener )
            {
                m_eventListener->unload( nullptr );
                m_eventListener = nullptr;
            }

            Component::unload( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CameraController::reload( SmartPtr<ISharedObject> data )
    {
        unload( data );
        load( data );
    }

    void CameraController::setCameraFlag( u32 flag, bool value )
    {
        u32 oldFlags = m_cameraFlags.load();
        u32 newFlags = BitUtil::setFlagValue( oldFlags, flag, value );
        m_cameraFlags = newFlags;

        // Handle specific flag changes
        if( oldFlags != newFlags )
        {
            onCameraFlagsChanged( oldFlags, newFlags );
        }
    }

    auto CameraController::getCameraFlag( u32 flag ) const -> bool
    {
        return BitUtil::getFlagValue( static_cast<u32>( m_cameraFlags.load() ), flag );
    }

    void CameraController::setViewportId( u32 viewportId )
    {
        if( m_viewportId != viewportId )
        {
            m_viewportId = viewportId;
            updateViewport();
        }
    }

    auto CameraController::getViewportId() const -> u32
    {
        return m_viewportId;
    }

    void CameraController::handleSetActive( bool active )
    {
        WP_ASSERT( isValid() );

        if( auto actor = getActor() )
        {
            auto cameraComponent = actor->getComponent<Camera>();
            if( cameraComponent )
            {
                cameraComponent->setActive( active );
            }
        }
    }

    auto CameraController::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto objects = Component::getChildObjects();

        if( m_eventListener )
        {
            objects.push_back( m_eventListener );
        }

        return objects;
    }

    auto CameraController::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Component::getProperties();

        properties->setProperty( CameraController::viewportIdStr, static_cast<s32>( m_viewportId ) );
        properties->setProperty( CameraController::cameraFlagsStr,
                                 static_cast<s32>( m_cameraFlags.load() ) );
        properties->setProperty( CameraController::isActiveStr, getCameraFlag( CameraFlags_Active ) );
        properties->setProperty( CameraController::isMainCameraStr,
                                 getCameraFlag( CameraFlags_MainCamera ) );
        properties->setProperty( CameraController::isOrthographicStr,
                                 getCameraFlag( CameraFlags_Orthographic ) );

        return properties;
    }

    void CameraController::setProperties( SmartPtr<Properties> properties )
    {
        Component::setProperties( properties );

        if( properties )
        {
            s32 viewportId = static_cast<s32>( m_viewportId );
            if( properties->getPropertyValue( CameraController::viewportIdStr, viewportId ) )
            {
                setViewportId( static_cast<u32>( viewportId ) );
            }

            s32 cameraFlags = static_cast<s32>( m_cameraFlags.load() );
            if( properties->getPropertyValue( CameraController::cameraFlagsStr, cameraFlags ) )
            {
                m_cameraFlags = static_cast<u32>( cameraFlags );
            }

            bool isActive = getCameraFlag( CameraFlags_Active );
            if( properties->getPropertyValue( CameraController::isActiveStr, isActive ) )
            {
                setCameraFlag( CameraFlags_Active, isActive );
            }

            bool isMainCamera = getCameraFlag( CameraFlags_MainCamera );
            if( properties->getPropertyValue( CameraController::isMainCameraStr, isMainCamera ) )
            {
                setCameraFlag( CameraFlags_MainCamera, isMainCamera );
            }

            bool isOrthographic = getCameraFlag( CameraFlags_Orthographic );
            if( properties->getPropertyValue( CameraController::isOrthographicStr, isOrthographic ) )
            {
                setCameraFlag( CameraFlags_Orthographic, isOrthographic );
            }
        }
    }

    void CameraController::focusSelection()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager )
                return;

            auto selectionManager = applicationManager->getSelectionManager();
            if( !selectionManager )
                return;

            auto selection = selectionManager->getSelection();
            if( selection.empty() )
                return;

            // Calculate bounding box of selected objects
            AABB3<real_Num> combinedBounds;
            bool hasValidBounds = false;

            for( auto selectedObject : selection )
            {
                if( auto actor = workphone::static_pointer_cast<IGameActor>( selectedObject ) )
                {
                    auto bounds = GameActorUtil::getActorLocalAABB( actor );
                    if( bounds.isValid() )
                    {
                        if( !hasValidBounds )
                        {
                            combinedBounds = bounds;
                            hasValidBounds = true;
                        }
                        else
                        {
                            combinedBounds.merge( bounds );
                        }
                    }
                }
            }

            if( hasValidBounds )
            {
                focusOnBounds( combinedBounds );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CameraController::focusOnBounds( const AABB3<real_Num> &bounds )
    {
        try
        {
            if( auto actor = getActor() )
            {
                auto transform = actor->getTransform();
                if( !transform )
                    return;

                // Calculate center and size of bounds
                Vector3<real_Num> center = bounds.getCenter();
                Vector3<real_Num> size = bounds.getExtent();
                real_Num maxSize =
                    Math<real_Num>::max( size.X(), Math<real_Num>::max( size.Y(), size.Z() ) );

                // Calculate distance based on camera type and field of view
                real_Num distance = maxSize * 2.0f;  // Default multiplier

                if( auto cameraComponent = actor->getComponent<Camera>() )
                {
                    if( auto camera = cameraComponent->getCamera() )
                    {
                        if( getCameraFlag( CameraFlags_Orthographic ) )
                        {
                            // For orthographic cameras, set orthographic size
                            //camera->setOrthoWindowHeight( maxSize * 2.0f );
                        }
                        else
                        {
                            // For perspective cameras, calculate distance based on FOV
                            real_Num fov = camera->getFOVy();
                            real_Num fovRad = Math<real_Num>::DegToRad( fov );
                            distance = maxSize / Math<real_Num>::Tan( fovRad * 0.5f );
                        }
                    }
                }

                // Position camera behind the bounds center
                Vector3<real_Num> offset( 0, 0, distance );
                Vector3<real_Num> newPosition = center + offset;

                // Set camera position and look at center
                transform->setPosition( newPosition );
                //transform->lookAt( center, Vector3<real_Num>::UNIT_Y );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CameraController::updateViewport()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager )
                return;

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            if( !graphicsSystem )
                return;

            if( auto actor = getActor() )
            {
                auto cameraComponent = actor->getComponent<Camera>();
                if( cameraComponent )
                {
                    if( auto camera = cameraComponent->getCamera() )
                    {
                        // Get viewport by ID
                        //auto viewport = graphicsSystem->getViewport( m_viewportId );
                        //if( viewport )
                        //{
                        //    viewport->setCamera( camera );
                        //}
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CameraController::onCameraFlagsChanged( u32 oldFlags, u32 newFlags )
    {
        try
        {
            // Handle main camera flag change
            if( BitUtil::getFlagValue( oldFlags, CameraFlags_MainCamera ) !=
                BitUtil::getFlagValue( newFlags, CameraFlags_MainCamera ) )
            {
                if( getCameraFlag( CameraFlags_MainCamera ) )
                {
                    setAsMainCamera();
                }
            }

            // Handle orthographic flag change
            if( BitUtil::getFlagValue( oldFlags, CameraFlags_Orthographic ) !=
                BitUtil::getFlagValue( newFlags, CameraFlags_Orthographic ) )
            {
                updateProjectionType();
            }

            // Handle active flag change
            if( BitUtil::getFlagValue( oldFlags, CameraFlags_Active ) !=
                BitUtil::getFlagValue( newFlags, CameraFlags_Active ) )
            {
                handleSetActive( getCameraFlag( CameraFlags_Active ) );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CameraController::setAsMainCamera()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager )
                return;

            auto sceneManager = applicationManager->getGameManager();
            if( !sceneManager )
                return;

            auto currentScene = sceneManager->getCurrentScene();
            if( !currentScene )
                return;

            // Find all other camera controllers and remove main camera flag
            auto actors = currentScene->getActors();
            for( auto actor : actors )
            {
                auto cameraControllers = actor->getComponentsByType<CameraController>();
                for( auto controller : cameraControllers )
                {
                    if( controller.get() != this )
                    {
                        controller->setCameraFlag( CameraFlags_MainCamera, false );
                    }
                }
            }

            // Set this camera as main camera in graphics system
            if( auto actor = getActor() )
            {
                auto cameraComponent = actor->getComponent<Camera>();
                if( cameraComponent )
                {
                    if( auto camera = cameraComponent->getCamera() )
                    {
                        auto graphicsSystem = applicationManager->getGraphicsSystem();
                        if( graphicsSystem )
                        {
                            // Set as primary camera
                            camera->setVisible( true );
                            updateViewport();
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CameraController::updateProjectionType()
    {
        try
        {
            if( auto actor = getActor() )
            {
                auto cameraComponent = actor->getComponent<Camera>();
                if( cameraComponent )
                {
                    if( auto camera = cameraComponent->getCamera() )
                    {
                        auto isOrthographic = getCameraFlag( CameraFlags_Orthographic );
                        //camera->setProjectionType( isOrthographic
                        //                               ? render::IGraphicsCamera::ProjectionType::Orthographic
                        //                               : render::IGraphicsCamera::ProjectionType::Perspective );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    //
    // EventListener Implementation
    //

    CameraController::EventListener::EventListener() = default;

    CameraController::EventListener::~EventListener() = default;

    auto CameraController::EventListener::handleEvent( EventType eventType, hash_type eventValue,
                                                       const Array<Parameter> &arguments,
                                                       SmartPtr<ISharedObject> sender,
                                                       SmartPtr<ISharedObject> object,
                                                       SmartPtr<IEvent> event ) -> Parameter
    {
        static const auto focusSelectionHash = StringUtil::getHash( "focus_selection" );

        auto owner = getOwner();
        if( !owner )
            return {};

        try
        {
            if( eventValue == focusSelectionHash )
            {
                owner->focusSelection();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    void CameraController::EventListener::setOwner( SmartPtr<CameraController> owner )
    {
        m_cameraController = owner;
    }

    auto CameraController::EventListener::getOwner() const -> SmartPtr<CameraController>
    {
        auto p = m_cameraController.load();
        return p.lock();
    }

    void CameraController::resetCamera()
    {
        try
        {
            if( auto actor = getActor() )
            {
                auto transform = actor->getTransform();
                if( transform )
                {
                    // Reset to default position and rotation
                    transform->setPosition( Vector3<real_Num>( 0, 0, 10 ) );
                    transform->setRotation( Vector3<real_Num>::zero() );
                }

                auto cameraComponent = actor->getComponent<Camera>();
                if( cameraComponent )
                {
                    if( auto camera = cameraComponent->getCamera() )
                    {
                        // Reset camera properties to defaults
                        camera->setFOVy( 45.0f );
                        camera->setNearClipDistance( 0.1f );
                        camera->setFarClipDistance( 1000.0f );

                        if( getCameraFlag( CameraFlags_Orthographic ) )
                        {
                            //camera->setOrthoWindowHeight( 10.0f );
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    bool CameraController::isMainCamera() const
    {
        return getCameraFlag( CameraFlags_MainCamera );
    }

    void CameraController::setMainCamera( bool mainCamera )
    {
        setCameraFlag( CameraFlags_MainCamera, mainCamera );
    }

    bool CameraController::isOrthographic() const
    {
        return getCameraFlag( CameraFlags_Orthographic );
    }

    void CameraController::setOrthographic( bool orthographic )
    {
        setCameraFlag( CameraFlags_Orthographic, orthographic );
    }

    auto CameraController::getUiWindow() const -> SmartPtr<ui::IUIWindow>
    {
        return m_uiWindow;
    }

    void CameraController::setUiWindow( SmartPtr<ui::IUIWindow> uiWindow )
    {
        m_uiWindow = uiWindow;
    }

    FSMReturnType CameraController::handleComponentEvent( u32 state, FSMEvent eventType )
    {
        switch( eventType )
        {
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Destroyed:
            {
            }
            break;
            case State::Edit:
            case State::Play:
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto gameManager = applicationManager->getGameManagerPtr();
                if( gameManager )
                {
                    gameManager->registerComponentUpdate( TaskId::Render, Thread::UpdateState::Update,
                                                          this );
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
            case State::Destroyed:
            {
            }
            break;
            case State::Edit:
            case State::Play:
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto gameManager = applicationManager->getGameManagerPtr();
                if( gameManager )
                {
                    gameManager->unregisterComponentUpdate( TaskId::Render, Thread::UpdateState::Update,
                                                            this );
                }
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
        }

        return FSMReturnType::Ok;
    }

}  // namespace workphone::scene
