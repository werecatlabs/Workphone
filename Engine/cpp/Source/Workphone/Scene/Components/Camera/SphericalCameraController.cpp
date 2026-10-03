#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Camera/SphericalCameraController.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/AABB2.hpp>
#include <Workphone/Math/MathUtil.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Input/IInputDeviceManager.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IKeyboardState.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Interface/System/ISelectionManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, SphericalCameraController, CameraController );

    const f32 SphericalCameraController::DEFAULT_FAR_DISTANCE = 2500.0f;
    const f32 SphericalCameraController::DEFAULT_NEAR_DISTANCE = 0.01f;
    const f32 SphericalCameraController::DEFAULT_ZOOM_SPEED = 1.0f;
    const f32 SphericalCameraController::DEFAULT_MOVE_SPEED = 1.0f;
    const f32 SphericalCameraController::DEFAULT_MAX_DELTA_TIME = 1.0f / 30.0f;
    const f32 SphericalCameraController::DEFAULT_PAN_FACTOR = 30.0f;
    const f32 SphericalCameraController::DEFAULT_KEY_ZOOM_FORCE = 5.0f;

    const String SphericalCameraController::positionStr = "position";
    const String SphericalCameraController::targetStr = "target";
    const String SphericalCameraController::sphericalCoordsStr = "sphericalCoords";
    const String SphericalCameraController::rotationSpeedStr = "rotationSpeed";
    const String SphericalCameraController::zoomSpeedStr = "zoomSpeed";
    const String SphericalCameraController::moveSpeedStr = "moveSpeed";
    const String SphericalCameraController::translationSpeedStr = "translationSpeed";
    const String SphericalCameraController::maxDistanceStr = "maxDistance";
    const String SphericalCameraController::nearDistanceStr = "nearDistance";
    const String SphericalCameraController::maxDeltaTimeStr = "maxDeltaTime";
    const String SphericalCameraController::panFactorStr = "panFactor";
    const String SphericalCameraController::keyZoomForceStr = "keyZoomForce";

    const Vector3<real_Num> SphericalCameraController::DEFAULT_SPHERICAL_COORDS =
        Vector3<real_Num>( 10.0f, 0.0f, Math<real_Num>::pi() * 0.5f );

    SphericalCameraController::SphericalCameraController() :
        m_rotationSpeed( 1.0f ),
        m_maxDistance( DEFAULT_FAR_DISTANCE ),
        m_nearDistance( DEFAULT_NEAR_DISTANCE ),
        m_zoomSpeed( DEFAULT_ZOOM_SPEED ),
        m_moveSpeed( DEFAULT_MOVE_SPEED ),
        m_translationSpeed( 100.0f ),
        m_maxDeltaTime( DEFAULT_MAX_DELTA_TIME ),
        m_panFactor( DEFAULT_PAN_FACTOR ),
        m_keyZoomForce( DEFAULT_KEY_ZOOM_FORCE ),
        m_spherical( DEFAULT_SPHERICAL_COORDS ),
        m_target( Vector3<real_Num>::zero() ),
        m_position( Vector3<real_Num>::zero() ),
        m_targetPosition( Vector3<real_Num>::zero() ),
        m_rotation( Vector3<real_Num>::zero() ),
        m_sphericalForce( Vector3<real_Num>::zero() ),
        m_isViewDirty( false )
    {
    }

    SphericalCameraController::~SphericalCameraController()
    {
    }

    void SphericalCameraController::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            CameraController::load( data );

            m_mouseKeys.resize( 3 );
            m_cursorKeys.resize( 6 );

            for( auto &&mouseKey : m_mouseKeys )
            {
                mouseKey = false;
            }

            allKeysUp();

            // Initialize position based on spherical coordinates
            m_position = m_target + getCartCoords( m_spherical );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void SphericalCameraController::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                CameraController::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void SphericalCameraController::update()
    {
        switch( auto task = Thread::getCurrentTask() )
        {
        case TaskId::Application:
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto inputManager = applicationManager->getInputDeviceManager();
            auto sceneManager = applicationManager->getGameManager();
            auto cameraManager = applicationManager->getCameraManager();
            auto timer = applicationManager->getTimerPtr();

            auto t = timer->getTime();
            auto dt = timer->getDeltaTime();

            if( !inputManager )
            {
                return;
            }

            if( dt > m_maxDeltaTime )
            {
                dt = m_maxDeltaTime;
            }

            auto isAltPressed = inputManager->isKeyPressed( KeyCodes::KEY_LALT ) ||
                                inputManager->isKeyPressed( KeyCodes::KEY_RALT );

            // Handle spherical rotation (Alt + Left Mouse)
            if( isAltPressed && isMouseBtnDown( 0 ) )
            {
                m_spherical.Y() += m_sphericalForce.Y() * -m_moveSpeed * static_cast<f32>( dt );
                m_spherical.Z() += m_sphericalForce.Z() * -m_moveSpeed * static_cast<f32>( dt );
                WP_ASSERT( m_spherical.isValid() );
            }
            // Handle right-click rotation (Maya style)
            else if( isMouseBtnDown( 2 ) )
            {
                if( auto actor = getActorPtr() )
                {
                    auto transform = actor->getTransform();
                    if( transform )
                    {
                        auto position = transform->getPosition();
                        auto orientation = transform->getOrientation();
                        auto euler = Euler<real_Num>( orientation );
                        auto eulerAngles = euler.toRadians();

                        eulerAngles.Y() -= m_sphericalForce.Y() * -m_moveSpeed * static_cast<f32>( dt );
                        eulerAngles.X() -= m_sphericalForce.Z() * -m_moveSpeed * static_cast<f32>( dt );

                        auto newOrientation = Euler<real_Num>( eulerAngles ).toQuaternion();

                        auto targetVector = position - m_target;
                        auto length = targetVector.length();

                        m_target = position + newOrientation * Vector3<real_Num>::forward() * length;

                        m_spherical.Y() -= m_sphericalForce.Y() * -m_moveSpeed * static_cast<f32>( dt );
                        m_spherical.Z() += m_sphericalForce.Z() * -m_moveSpeed * static_cast<f32>( dt );
                        WP_ASSERT( m_spherical.isValid() );
                    }
                }
            }
            // Handle panning (Middle Mouse)
            else if( isMouseBtnDown( 1 ) )
            {
                if( auto actor = getActorPtr() )
                {
                    auto transform = actor->getTransform();
                    if( transform )
                    {
                        auto orientation = transform->getOrientation();
                        auto translateX = orientation * Vector3<real_Num>::right() *
                                          m_sphericalForce.Y() * m_moveSpeed * static_cast<f32>( dt ) *
                                          m_panFactor;
                        auto translateY = orientation * Vector3<real_Num>::up() * m_sphericalForce.Z() *
                                          -m_moveSpeed * static_cast<f32>( dt ) * m_panFactor;

                        m_target -= translateX + translateY;
                        WP_ASSERT( m_target.isValid() );
                    }
                }
            }

            // Handle zoom
            if( m_sphericalForce.X() != real_Num( 0.0 ) )
            {
                m_spherical.X() += m_sphericalForce.X() * -m_zoomSpeed * static_cast<f32>( dt );
            }

            m_sphericalForce = Vector3<real_Num>::zero();

            // Clamp distance
            if( m_spherical.X() < m_nearDistance )
            {
                m_spherical.X() = m_nearDistance;
            }
            else if( m_spherical.X() > m_maxDistance )
            {
                m_spherical.X() = m_maxDistance;
            }

            // m_sphericalForce *= static_cast<f32>( dt );
            WP_ASSERT( m_sphericalForce.isValid() );

            wrapSphericalCoords( m_spherical );
            m_position = m_target + getCartCoords( m_spherical );
            WP_ASSERT( m_position.isValid() );

            auto yawAxis = Vector3<real_Num>::unitY();
            if( m_spherical.Z() >= 0.0f )
            {
                if( m_spherical.Z() <= Math<real_Num>::pi() )
                {
                    yawAxis = Vector3<real_Num>::UNIT_Y;
                }
                else
                {
                    yawAxis = -Vector3<real_Num>::UNIT_Y;
                }
            }
            else
            {
                if( m_spherical.Z() >= -MathF::pi() )
                {
                    yawAxis = -Vector3<real_Num>::UNIT_Y;
                }
                else
                {
                    yawAxis = Vector3<real_Num>::UNIT_Y;
                }
            }

            // Update actor transform if changed
            if( auto actor = getActorPtr() )
            {
                auto position = actor->getPosition();
                auto orientation = actor->getOrientation();
                auto dirty = !MathUtilF::equals( m_position, position );
                if( dirty )
                {
                    actor->setPosition( m_position );

                    auto direction = ( m_target - m_position ).normaliseCopy();
                    WP_ASSERT( direction.isValid() );

                    auto actorOrientation = MathUtil<real_Num>::getOrientationFromDirection(
                        direction, -Vector3<real_Num>::unitZ(), true, yawAxis );
                    WP_ASSERT( actorOrientation.isValid() );

                    actor->setOrientation( actorOrientation );

                    setViewDirty( true );
                }
            }
        }
        break;
        default:
        {
        }
        }
    }

    auto SphericalCameraController::handleEvent( const SmartPtr<IInputEvent> &event ) -> bool
    {
        auto applicationManager = core::IApplicationManager::instance();

        bool eventHandled = false;

        auto inputEventType = event->getEventType();
        switch( inputEventType )
        {
        case IInputEvent::EventType::Key:
        {
            auto keyboardState = event->getKeyboardState();
            auto keycode = (KeyCodes)keyboardState->getKeyCode();
            switch( keycode )
            {
            case KeyCodes::KEY_KEY_W:
            {
                m_sphericalForce.X() += m_keyZoomForce;
                eventHandled = true;
            }
            break;
            case KeyCodes::KEY_KEY_S:
            {
                m_sphericalForce.X() -= m_keyZoomForce;
                eventHandled = true;
            }
            break;
            default:
            {
            }
            };
        }
        break;
        case IInputEvent::EventType::Mouse:
        {
            if( auto mouseState = event->getMouseState() )
            {
                auto relativePosition = mouseState->getRelativePosition();

                // Check if mouse is within UI window bounds
                if( auto uiWindow = getUiWindow() )
                {
                    if( auto mainWindow = applicationManager->getWindow() )
                    {
                        auto mainWindowSize = mainWindow->getSize();
                        auto mainWindowSizeF = Vector2<real_Num>( static_cast<f32>( mainWindowSize.x ),
                                                                  static_cast<f32>( mainWindowSize.y ) );

                        auto pos = uiWindow->getPosition() / mainWindowSizeF;
                        auto size = uiWindow->getSize() / mainWindowSizeF;

                        auto aabb = AABB2<real_Num>( pos, size, true );
                        if( !aabb.isInside( relativePosition ) )
                        {
                            for( auto &&m_mouseKey : m_mouseKeys )
                            {
                                m_mouseKey = false;
                            }

                            m_sphericalForce = Vector3<real_Num>::zero();

                            return false;
                        }
                    }
                }
                else
                {
                    // Check viewport bounds
                    auto actor = getActor();
                    auto cameraComponent = actor->getComponent<Camera>();
                    auto camera = cameraComponent->getCamera();

                    if( camera )
                    {
                        auto vp = camera->getViewport();
                        if( !vp )
                        {
                            return false;
                        }

                        auto pos = vp->getPosition();
                        auto size = vp->getSize();
                        if( !( relativePosition > pos && relativePosition < ( pos + size ) ) )
                        {
                            return false;
                        }
                    }
                }

                auto mouseEventType = mouseState->getEventType();
                switch( mouseEventType )
                {
                case IMouseState::Event::LeftPressed:
                {
                    m_mouseKeys[0] = true;
                    eventHandled = true;
                }
                break;
                case IMouseState::Event::RightPressed:
                {
                    m_mouseKeys[2] = true;
                    eventHandled = true;
                }
                break;
                case IMouseState::Event::MiddlePressed:
                {
                    m_mouseKeys[1] = true;
                    eventHandled = true;
                }
                break;
                case IMouseState::Event::LeftReleased:
                {
                    m_mouseKeys[0] = false;
                    eventHandled = true;
                }
                break;
                case IMouseState::Event::RightReleased:
                {
                    m_mouseKeys[2] = false;
                    eventHandled = true;
                }
                break;
                case IMouseState::Event::MiddleReleased:
                {
                    m_mouseKeys[1] = false;
                    eventHandled = true;
                }
                break;
                case IMouseState::Event::Moved:
                {
                    if( !isMouseBtnDown( 0 ) && !isMouseBtnDown( 1 ) && !isMouseBtnDown( 2 ) )
                    {
                        break;
                    }

                    // Native window input can deliver several moves before the next update.
                    auto sphericalForce = mouseState->getDelta();
                    m_sphericalForce.Y() += sphericalForce.X();
                    m_sphericalForce.Z() += sphericalForce.Y();

                    eventHandled = true;
                }
                break;
                case IMouseState::Event::Wheel:
                {
                    auto wheelDelta = mouseState->getWheelDelta();
                    m_sphericalForce.X() += wheelDelta.Y();

                    eventHandled = true;
                }
                break;
                case IMouseState::Event::Count:
                {
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

        return eventHandled;
    }

    void SphericalCameraController::setPosition( const Vector3<real_Num> &position )
    {
        m_position = position;

        // Update spherical coordinates based on new position
        auto offset = m_position - m_target;
        m_spherical = getSphericalCoords( offset );
    }

    auto SphericalCameraController::getPosition() const -> Vector3<real_Num>
    {
        return m_position;
    }

    void SphericalCameraController::setTargetPosition( const Vector3<real_Num> &position )
    {
        m_target = position;
        m_targetPosition = position;
        // Update position to maintain spherical offset
        m_position = m_target + getCartCoords( m_spherical );
    }

    auto SphericalCameraController::getTargetPosition() const -> Vector3<real_Num>
    {
        return m_target;
    }

    void SphericalCameraController::setOrientation( const Quaternion<real_Num> &orientation )
    {
        // Convert orientation to spherical coordinates if needed
        auto direction = orientation * -Vector3<real_Num>::unitZ();
        auto euler = Euler<real_Num>( orientation );
        m_rotation = euler.toDegrees();
    }

    auto SphericalCameraController::getOrientation() const -> Quaternion<real_Num>
    {
        auto direction = ( m_target - m_position ).normaliseCopy();
        return MathUtil<real_Num>::getOrientationFromDirection( direction );
    }

    void SphericalCameraController::setDirection( const Vector3<real_Num> &direction )
    {
        // Convert direction to spherical coordinates
        auto normalizedDir = direction.normaliseCopy();
        m_spherical.Y() = Math<real_Num>::ATan2( normalizedDir.X(), normalizedDir.Z() );
        m_spherical.Z() = Math<real_Num>::ACos( normalizedDir.Y() );
    }

    auto SphericalCameraController::getDirection() const -> Vector3<real_Num>
    {
        return ( m_target - m_position ).normaliseCopy();
    }

    void SphericalCameraController::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            CameraController::setProperties( properties );

            if( !properties )
            {
                WP_LOG_WARNING( "SphericalCameraController::setProperties: null properties supplied." );
                return;
            }

            properties->getPropertyValue( positionStr, m_position );
            properties->getPropertyValue( targetStr, m_target );
            properties->getPropertyValue( sphericalCoordsStr, m_spherical );

            f32 rotationSpeed = static_cast<f32>( m_rotationSpeed );
            if( properties->getPropertyValue( rotationSpeedStr, rotationSpeed ) )
            {
                setRotationSpeed( rotationSpeed );
            }

            f32 zoomSpeed = static_cast<f32>( m_zoomSpeed );
            if( properties->getPropertyValue( zoomSpeedStr, zoomSpeed ) )
            {
                setZoomSpeed( zoomSpeed );
            }

            f32 moveSpeed = static_cast<f32>( m_moveSpeed );
            if( properties->getPropertyValue( moveSpeedStr, moveSpeed ) )
            {
                setMoveSpeed( moveSpeed );
            }

            f32 translationSpeed = static_cast<f32>( m_translationSpeed );
            if( properties->getPropertyValue( translationSpeedStr, translationSpeed ) )
            {
                setTranslationSpeed( translationSpeed );
            }

            f32 maxDistance = static_cast<f32>( m_maxDistance );
            if( properties->getPropertyValue( maxDistanceStr, maxDistance ) )
            {
                setMaxDistance( maxDistance );
            }

            f32 nearDistance = static_cast<f32>( m_nearDistance );
            if( properties->getPropertyValue( nearDistanceStr, nearDistance ) )
            {
                setNearDistance( nearDistance );
            }

            real_Num maxDeltaTime = m_maxDeltaTime;
            if( properties->getPropertyValue( maxDeltaTimeStr, maxDeltaTime ) )
            {
                setMaxDeltaTime( maxDeltaTime );
            }

            f32 panFactor = m_panFactor;
            if( properties->getPropertyValue( panFactorStr, panFactor ) )
            {
                setPanFactor( panFactor );
            }

            f32 keyZoomForce = m_keyZoomForce;
            if( properties->getPropertyValue( keyZoomForceStr, keyZoomForce ) )
            {
                setKeyZoomForce( keyZoomForce );
            }

            // Recalculate position from updated spherical + target
            m_position = m_target + getCartCoords( m_spherical );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<Properties> SphericalCameraController::getProperties() const
    {
        try
        {
            auto properties = CameraController::getProperties();

            if( !properties )
            {
                WP_LOG_WARNING(
                    "SphericalCameraController::getProperties: base class returned null properties." );
                return {};
            }

            properties->setProperty( positionStr, m_position );
            properties->setProperty( targetStr, m_target );
            properties->setProperty( sphericalCoordsStr, m_spherical );
            properties->setProperty( rotationSpeedStr, static_cast<f32>( m_rotationSpeed ) );
            properties->setProperty( zoomSpeedStr, static_cast<f32>( m_zoomSpeed ) );
            properties->setProperty( moveSpeedStr, static_cast<f32>( m_moveSpeed ) );
            properties->setProperty( translationSpeedStr, static_cast<f32>( m_translationSpeed ) );
            properties->setProperty( maxDistanceStr, static_cast<f32>( m_maxDistance ) );
            properties->setProperty( nearDistanceStr, static_cast<f32>( m_nearDistance ) );
            properties->setProperty( maxDeltaTimeStr, static_cast<f32>( m_maxDeltaTime ) );
            properties->setProperty( panFactorStr, m_panFactor );
            properties->setProperty( keyZoomForceStr, m_keyZoomForce );

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    auto SphericalCameraController::isMouseBtnDown( s32 key ) -> bool
    {
        if( key >= 0 && key < static_cast<s32>( m_mouseKeys.size() ) )
        {
            return m_mouseKeys[key];
        }
        return false;
    }

    void SphericalCameraController::allKeysUp()
    {
        for( u32 i = 0; i < m_cursorKeys.size(); ++i )
        {
            m_cursorKeys[i] = false;
        }
    }

    auto SphericalCameraController::getCameraToViewportRay(
        const Vector2<real_Num> &screenPosition ) const -> Ray3<real_Num>
    {
        if( auto actor = getActorPtr() )
        {
            if( auto camera = actor->getComponentPtr<Camera>() )
            {
                if( auto renderCamera = camera->getCamera() )
                {
                    auto viewport = camera->getViewport();
                    auto size = viewport->getSize();
                    auto normalizedScreenCoords = screenPosition / size;

                    return renderCamera->getRay( normalizedScreenCoords.X(),
                                                 normalizedScreenCoords.Y() );
                }
            }
        }

        return {};
    }

    auto SphericalCameraController::getSphericalCoords() const -> Vector3<real_Num>
    {
        return m_spherical;
    }

    void SphericalCameraController::setSphericalCoords( const Vector3<real_Num> &spherical )
    {
        m_spherical = spherical;

        // Update position based on new spherical coordinates
        m_position = m_target + getCartCoords( m_spherical );
    }

    auto SphericalCameraController::getCartCoords( Vector3<real_Num> &spherical ) -> Vector3<real_Num>
    {
        Vector3<real_Num> cart;
        cart.Z() = spherical.X() * sin( spherical.Z() ) * cos( spherical.Y() );
        cart.X() = spherical.X() * sin( spherical.Z() ) * sin( spherical.Y() );
        cart.Y() = spherical.X() * -cos( spherical.Z() );
        return cart;
    }

    auto SphericalCameraController::getSphericalCoords( Vector3<real_Num> &vec ) -> Vector3<real_Num>
    {
        auto coords = Vector3<real_Num>::zero();
        coords.X() = vec.length();
        coords.Y() = Math<real_Num>::ATan2( vec.X(), vec.Z() );
        coords.Z() = Math<real_Num>::ACos( vec.Y() / coords.X() );
        return coords;
    }

    void SphericalCameraController::wrapSphericalCoords( Vector3<real_Num> &spherical )
    {
        // Wrap azimuth angle (Y) to [0, 2π]
        while( spherical.Y() < 0.0f )
        {
            spherical.Y() += Math<real_Num>::pi() * 2.0f;
        }
        while( spherical.Y() >= Math<real_Num>::pi() * 2.0f )
        {
            spherical.Y() -= Math<real_Num>::pi() * 2.0f;
        }

        // Clamp polar angle (Z) to [0, π]
        if( spherical.Z() < 0.0f )
        {
            spherical.Z() = 0.0f;
        }
        else if( spherical.Z() > Math<real_Num>::pi() )
        {
            spherical.Z() = Math<real_Num>::pi();
        }
    }

    auto SphericalCameraController::getMaxDistance() const -> f32
    {
        return m_maxDistance;
    }

    void SphericalCameraController::setMaxDistance( f32 maxDistance )
    {
        m_maxDistance = maxDistance;

        // Clamp current distance if needed
        if( m_spherical.X() > m_maxDistance )
        {
            m_spherical.X() = m_maxDistance;
            m_position = m_target + getCartCoords( m_spherical );
        }
    }

    auto SphericalCameraController::getNearDistance() const -> f32
    {
        return static_cast<f32>( m_nearDistance );
    }

    void SphericalCameraController::setNearDistance( f32 nearDistance )
    {
        if( nearDistance <= 0.0f )
        {
            WP_LOG_WARNING(
                "SphericalCameraController::setNearDistance: nearDistance must be > 0; ignoring." );
            return;
        }
        m_nearDistance = nearDistance;
    }

    auto SphericalCameraController::getZoomSpeed() const -> f32
    {
        return static_cast<f32>( m_zoomSpeed );
    }

    void SphericalCameraController::setZoomSpeed( f32 zoomSpeed )
    {
        if( zoomSpeed < 0.0f )
        {
            WP_LOG_WARNING( "SphericalCameraController::setZoomSpeed: value must be >= 0; ignoring." );
            return;
        }
        m_zoomSpeed = zoomSpeed;
    }

    auto SphericalCameraController::getMoveSpeed() const -> f32
    {
        return static_cast<f32>( m_moveSpeed );
    }

    void SphericalCameraController::setMoveSpeed( f32 moveSpeed )
    {
        if( moveSpeed < 0.0f )
        {
            WP_LOG_WARNING( "SphericalCameraController::setMoveSpeed: value must be >= 0; ignoring." );
            return;
        }
        m_moveSpeed = moveSpeed;
    }

    auto SphericalCameraController::getTranslationSpeed() const -> f32
    {
        return static_cast<f32>( m_translationSpeed );
    }

    void SphericalCameraController::setTranslationSpeed( f32 translationSpeed )
    {
        if( translationSpeed < 0.0f )
        {
            WP_LOG_WARNING(
                "SphericalCameraController::setTranslationSpeed: value must be >= 0; ignoring." );
            return;
        }
        m_translationSpeed = translationSpeed;
    }

    auto SphericalCameraController::getMaxDeltaTime() const -> real_Num
    {
        return m_maxDeltaTime;
    }

    void SphericalCameraController::setMaxDeltaTime( real_Num maxDt )
    {
        if( maxDt <= real_Num( 0.0 ) )
        {
            WP_LOG_WARNING( "SphericalCameraController::setMaxDeltaTime: value must be > 0; ignoring." );
            return;
        }
        m_maxDeltaTime = maxDt;
    }

    auto SphericalCameraController::getPanFactor() const -> f32
    {
        return m_panFactor;
    }

    void SphericalCameraController::setPanFactor( f32 factor )
    {
        if( factor < 0.0f )
        {
            WP_LOG_WARNING( "SphericalCameraController::setPanFactor: value must be >= 0; ignoring." );
            return;
        }
        m_panFactor = factor;
    }

    auto SphericalCameraController::getKeyZoomForce() const -> f32
    {
        return m_keyZoomForce;
    }

    void SphericalCameraController::setKeyZoomForce( f32 force )
    {
        if( force < 0.0f )
        {
            WP_LOG_WARNING(
                "SphericalCameraController::setKeyZoomForce: value must be >= 0; ignoring." );
            return;
        }
        m_keyZoomForce = force;
    }

    void SphericalCameraController::setRotationSpeed( f32 rotationSpeed )
    {
        m_rotationSpeed = rotationSpeed;
    }

    auto SphericalCameraController::getRotationSpeed() const -> f32
    {
        return m_rotationSpeed;
    }

    auto SphericalCameraController::getRight() const -> Vector3<real_Num>
    {
        return getOrientation() * Vector3<real_Num>::right();
    }

    auto SphericalCameraController::getUp() const -> Vector3<real_Num>
    {
        return getOrientation() * Vector3<real_Num>::up();
    }

    void SphericalCameraController::focusSelection()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto selectionManager = applicationManager->getSelectionManager();
        WP_ASSERT( selectionManager );

        auto position = Vector3<real_Num>::zero();
        auto count = 0;

        auto selection = selectionManager->getSelection();
        for( auto selected : selection )
        {
            if( selected->isDerived<IGameActor>() )
            {
                auto actor = workphone::static_pointer_cast<IGameActor>( selected );
                if( actor )
                {
                    if( auto t = actor->getTransform() )
                    {
                        position += t->getPosition();
                        ++count;
                    }
                }
            }
        }

        WP_ASSERT( position.isValid() );

        if( count > 0 )
        {
            setTargetPosition( position / static_cast<real_Num>( count ) );
        }

        WP_ASSERT( m_target.isValid() );
    }

    void SphericalCameraController::setViewDirty( bool dirty )
    {
        m_isViewDirty = dirty;
    }

    bool SphericalCameraController::isViewDirty() const
    {
        return m_isViewDirty;
    }

}  // namespace workphone::scene
