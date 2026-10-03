#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Camera/FPSCameraController.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/MathUtil.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IKeyboardState.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Interface/System/ISelectionManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Scene/Components/Camera.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, FpsCameraController, CameraController );

    // Property key string initialisers
    const String FpsCameraController::positionStr = "Position";
    const String FpsCameraController::targetPositionStr = "TargetPosition";
    const String FpsCameraController::targetDirectionStr = "Direction";
    const String FpsCameraController::moveSpeedStr = "MoveSpeed";
    const String FpsCameraController::translationSpeedStr = "TranslationSpeed";
    const String FpsCameraController::rotationSpeedStr = "RotationSpeed";
    const String FpsCameraController::mouseSensitivityStr = "MouseSensitivity";
    const String FpsCameraController::maxPitchAngleStr = "MaxPitchAngle";
    const String FpsCameraController::wheelSpeedFactorStr = "WheelSpeedFactor";
    const String FpsCameraController::minMoveSpeedStr = "MinMoveSpeed";
    const String FpsCameraController::maxMoveSpeedStr = "MaxMoveSpeed";
    const String FpsCameraController::yawStr = "Yaw";
    const String FpsCameraController::pitchStr = "Pitch";

    FpsCameraController::FpsCameraController()
    {
        m_mouseKeys.resize( 3, false );
        m_cursorKeys.resize( 6, false );

        // create default key map
        m_keyMap.emplace_back( 0, static_cast<u32>( KeyCodes::KEY_UP ),
                               static_cast<u32>( KeyCodes::KEY_KEY_W ) );
        m_keyMap.emplace_back( 1, static_cast<u32>( KeyCodes::KEY_DOWN ),
                               static_cast<u32>( KeyCodes::KEY_KEY_S ) );
        m_keyMap.emplace_back( 2, static_cast<u32>( KeyCodes::KEY_LEFT ),
                               static_cast<u32>( KeyCodes::KEY_KEY_A ) );
        m_keyMap.emplace_back( 3, static_cast<u32>( KeyCodes::KEY_RIGHT ),
                               static_cast<u32>( KeyCodes::KEY_KEY_D ) );

        allKeysUp();

        m_targetDirection = Vector3<real_Num>::UNIT_Z;
    }

    FpsCameraController::~FpsCameraController() = default;

    void FpsCameraController::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        CameraController::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void FpsCameraController::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        CameraController::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void FpsCameraController::update()
    {
        if( !m_selectedCamera )
            return;

        // Get delta time from application timer
        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
            return;

        auto timer = applicationManager->getTimer();
        if( !timer )
            return;

        real_Num dt = static_cast<real_Num>( timer->getDeltaTime() );

        // Calculate movement
        Vector3<real_Num> position = m_position;
        Vector3<real_Num> moveDirection = Vector3<real_Num>::zero();

        // Forward/Backward movement
        if( m_cursorKeys[0] )  // Forward
            moveDirection += m_targetDirection;

        if( m_cursorKeys[1] )  // Backward
            moveDirection -= m_targetDirection;

        // Strafing (left/right)
        Vector3<real_Num> upVector = Vector3<real_Num>::UNIT_Y;
        Vector3<real_Num> strafeVector = m_targetDirection.crossProduct( upVector );
        strafeVector.normalise();

        if( m_cursorKeys[2] )  // Left
            position -= strafeVector * dt * m_moveSpeed * m_translationSpeed;

        if( m_cursorKeys[3] )  // Right
            position += strafeVector * dt * m_moveSpeed * m_translationSpeed;

        // Apply movement
        if( moveDirection.lengthSquared() > 0.0f )
        {
            moveDirection.normalise();
            position += moveDirection * dt * m_moveSpeed * m_translationSpeed;
        }

        m_position = position;

        // Update camera position
        //m_selectedCamera->setPosition( m_position );

        // Update target direction based on yaw and pitch
        updateTargetDirection();

        // Update camera orientation
        Vector3<real_Num> lookAtTarget = m_position + m_targetDirection;
        //m_selectedCamera->lookAt( lookAtTarget, upVector );
    }

    auto FpsCameraController::handleInputEvent( const SmartPtr<IInputEvent> &event ) -> bool
    {
        if( !event )
            return false;

        bool eventHandled = false;

        auto eventType = event->getEventType();

        switch( eventType )
        {
        case IInputEvent::EventType::Key:
        {
            //auto keyboardState = event->getKeyboardState();
            //auto keyCode = keyboardState->getKeyCode();
            //bool isPressed = keyboardState->isKeyPressed();

            //// Check key mappings
            //for( u32 i = 0; i < m_keyMap.size(); ++i )
            //{
            //    if( m_keyMap[i].keycode == static_cast<u32>( keyCode ) ||
            //        m_keyMap[i].keycode1 == static_cast<u32>( keyCode ) )
            //    {
            //        m_cursorKeys[m_keyMap[i].action] = isPressed;
            //        eventHandled = true;
            //    }
            //}
        }
        break;
        case IInputEvent::EventType::Mouse:
        {
            //auto mouseEventType = event->getMouseEventType();

            //switch( mouseEventType )
            //{
            //case IInputEvent::MouseEventType::ButtonPressed:
            //{
            //    auto button = event->getMouseButton();
            //    if( button == IInputEvent::MouseButton::Left )
            //    {
            //        m_mouseKeys[0] = true;
            //    }
            //    else if( button == IInputEvent::MouseButton::Right )
            //    {
            //        m_mouseKeys[2] = true;
            //        m_isMouseCaptured = true;
            //        m_prevCursor = Vector2<real_Num>( event->getMouseX(), event->getMouseY() );
            //        eventHandled = true;
            //    }
            //    else if( button == IInputEvent::MouseButton::Middle )
            //    {
            //        m_mouseKeys[1] = true;
            //    }
            //}
            //break;

            //case IInputEvent::MouseEventType::ButtonReleased:
            //{
            //    auto button = event->getMouseButton();
            //    if( button == IInputEvent::MouseButton::Left )
            //    {
            //        m_mouseKeys[0] = false;
            //    }
            //    else if( button == IInputEvent::MouseButton::Right )
            //    {
            //        m_mouseKeys[2] = false;
            //        m_isMouseCaptured = false;
            //    }
            //    else if( button == IInputEvent::MouseButton::Middle )
            //    {
            //        m_mouseKeys[1] = false;
            //    }
            //}
            //break;

            //case IInputEvent::MouseEventType::Moved:
            //{
            //    m_mousePos.X() = static_cast<real_Num>( event->getMouseX() );
            //    m_mousePos.Y() = static_cast<real_Num>( event->getMouseY() );

            //    if( m_isMouseCaptured && isMouseKeyDown( 2 ) )
            //    {
            //        Vector2<real_Num> currentCursor( m_mousePos.X(), m_mousePos.Y() );
            //        Vector2<real_Num> delta = currentCursor - m_prevCursor;

            //        // Update rotation
            //        m_yaw -= delta.X() * m_rotationSpeed * m_mouseSensitivity;
            //        m_pitch -= delta.Y() * m_rotationSpeed * m_mouseSensitivity;

            //        // Clamp pitch to avoid gimbal lock
            //        const real_Num maxPitch = m_maxPitchAngle * MathUtil<real_Num>::DEG_TO_RAD();
            //        m_pitch = MathUtil<real_Num>::clamp( m_pitch, -maxPitch, maxPitch );

            //        m_prevCursor = currentCursor;
            //        eventHandled = true;
            //    }
            //}
            //break;

            //case IInputEvent::MouseEventType::Wheel:
            //{
            //    // Optional: Implement zoom or speed adjustment
            //    real_Num wheelDelta = event->getMouseWheelDelta();
            //    m_moveSpeed += wheelDelta * m_wheelSpeedFactor;
            //    m_moveSpeed = MathUtil<real_Num>::clamp( m_moveSpeed, m_minMoveSpeed, m_maxMoveSpeed );
            //}
            //break;

            //default:
            //    break;
            //}
        }
        break;

        default:
            break;
        }

        return eventHandled;
    }

    void FpsCameraController::updateTargetDirection()
    {
        // Calculate direction vector from yaw and pitch
        auto cosYaw = Math<real_Num>::Cos( m_yaw );
        auto sinYaw = Math<real_Num>::Sin( m_yaw );
        auto cosPitch = Math<real_Num>::Cos( m_pitch );
        auto sinPitch = Math<real_Num>::Sin( m_pitch );

        m_targetDirection.X() = cosYaw * cosPitch;
        m_targetDirection.Y() = sinPitch;
        m_targetDirection.Z() = sinYaw * cosPitch;
        m_targetDirection.normalise();
    }

    void FpsCameraController::setPosition( const Vector3<real_Num> &position )
    {
        m_position = position;
    }

    auto FpsCameraController::getPosition() const -> Vector3<real_Num>
    {
        return m_position;
    }

    void FpsCameraController::setTargetPosition( const Vector3<real_Num> &position )
    {
        m_targetPosition = position;

        auto direction = position - m_position;
        if( direction.lengthSquared() > 0.0f )
        {
            direction.normalise();
            m_targetDirection = direction;

            // Calculate yaw and pitch from direction
            m_yaw = Math<real_Num>::ATan2( direction.Z(), direction.X() );
            m_pitch = Math<real_Num>::ASin( direction.Y() );
        }
    }

    auto FpsCameraController::getTargetPosition() const -> Vector3<real_Num>
    {
        return m_position + m_targetDirection;
    }

    void FpsCameraController::setOrientation( const Quaternion<real_Num> &orientation )
    {
        m_orientation = orientation;

        // Convert quaternion to yaw and pitch
        Vector3<real_Num> direction = orientation * Vector3<real_Num>::UNIT_Z;
        m_targetDirection = direction;

        m_yaw = Math<real_Num>::ATan2( direction.Z(), direction.X() );
        m_pitch = Math<real_Num>::ASin( direction.Y() );
    }

    auto FpsCameraController::getOrientation() const -> Quaternion<real_Num>
    {
        return m_orientation;
    }

    auto FpsCameraController::getDirection() const -> Vector3<real_Num>
    {
        return m_targetDirection;
    }

    void FpsCameraController::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            if( !properties )
            {
                WP_LOG_WARNING( "setProperties called with null properties" );
                return;
            }

            CameraController::setProperties( properties );

            // Transform state
            Vector3<real_Num> position = m_position;
            if( properties->getPropertyValue( positionStr, position ) )
            {
                setPosition( position );
            }

            // Speed settings
            properties->getPropertyValue( moveSpeedStr, m_moveSpeed );
            properties->getPropertyValue( translationSpeedStr, m_translationSpeed );
            properties->getPropertyValue( rotationSpeedStr, m_rotationSpeed );

            // Mouse tuning
            properties->getPropertyValue( mouseSensitivityStr, m_mouseSensitivity );

            real_Num maxPitch = m_maxPitchAngle;
            if( properties->getPropertyValue( maxPitchAngleStr, maxPitch ) )
            {
                setMaxPitchAngle( maxPitch );  // applies clamp
            }

            properties->getPropertyValue( wheelSpeedFactorStr, m_wheelSpeedFactor );

            real_Num minSpeed = m_minMoveSpeed;
            if( properties->getPropertyValue( minMoveSpeedStr, minSpeed ) )
            {
                setMinMoveSpeed( minSpeed );  // keeps min <= max
            }

            real_Num maxSpeed = m_maxMoveSpeed;
            if( properties->getPropertyValue( maxMoveSpeedStr, maxSpeed ) )
            {
                setMaxMoveSpeed( maxSpeed );
            }

            // Euler angles – recompute direction after both are read
            real_Num yaw = m_yaw, pitch = m_pitch;
            bool anglesChanged = false;
            anglesChanged |= properties->getPropertyValue( yawStr, yaw );
            anglesChanged |= properties->getPropertyValue( pitchStr, pitch );
            if( anglesChanged )
            {
                m_yaw = yaw;
                auto maxRad = Math<real_Num>::deg_to_rad() * m_maxPitchAngle;
                m_pitch = Math<real_Num>::clamp( pitch, -maxRad, maxRad );
                updateTargetDirection();
            }
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<Properties> FpsCameraController::getProperties() const
    {
        auto properties = CameraController::getProperties();

        // Transform state
        properties->setProperty( positionStr, m_position );
        properties->setProperty( targetPositionStr, m_targetPosition );
        properties->setProperty( targetDirectionStr, m_targetDirection );

        // Euler angles
        properties->setProperty( yawStr, m_yaw );
        properties->setProperty( pitchStr, m_pitch );

        // Speed settings
        properties->setProperty( moveSpeedStr, m_moveSpeed );
        properties->setProperty( translationSpeedStr, m_translationSpeed );
        properties->setProperty( rotationSpeedStr, m_rotationSpeed );

        // Mouse tuning
        properties->setProperty( mouseSensitivityStr, m_mouseSensitivity );
        properties->setProperty( maxPitchAngleStr, m_maxPitchAngle );
        properties->setProperty( wheelSpeedFactorStr, m_wheelSpeedFactor );
        properties->setProperty( minMoveSpeedStr, m_minMoveSpeed );
        properties->setProperty( maxMoveSpeedStr, m_maxMoveSpeed );

        return properties;
    }

    auto FpsCameraController::isMouseKeyDown( s32 key ) -> bool
    {
        if( key >= 0 && key < static_cast<s32>( m_mouseKeys.size() ) )
        {
            return m_mouseKeys[key];
        }
        return false;
    }

    void FpsCameraController::allKeysUp()
    {
        for( u32 i = 0; i < m_cursorKeys.size(); ++i )
        {
            m_cursorKeys[i] = false;
        }
    }

    auto FpsCameraController::getCameraToViewportRay( const Vector2<real_Num> &screenPosition ) const
        -> Ray3F
    {
        Ray3F ray;

        if( m_selectedCamera )
        {
            //auto viewport = m_selectedCamera->getViewport();
            //if( viewport )
            //{
            //    f32 width = static_cast<f32>( viewport->getWidth() );
            //    f32 height = static_cast<f32>( viewport->getHeight() );

            //    if( width > 0.0f && height > 0.0f )
            //    {
            //        Vector2<real_Num> normalisedScreenCoords;
            //        normalisedScreenCoords.X() = screenPosition.X() / width;
            //        normalisedScreenCoords.Y() = screenPosition.Y() / height;

            //        ray = m_selectedCamera->getCameraToViewportRay( normalisedScreenCoords.X(),
            //                                                        normalisedScreenCoords.Y() );
            //    }
            //}
        }

        return ray;
    }

    auto FpsCameraController::isInFrustum( const AABB3F &box ) const -> bool
    {
        if( m_selectedCamera )
        {
            //return m_selectedCamera->isVisible( box );
        }

        return false;
    }

    void FpsCameraController::addCamera( SmartPtr<render::IGraphicsCamera> camera )
    {
        if( camera )
        {
            m_cameras.push_back( camera );
            if( !m_selectedCamera )
            {
                m_selectedCamera = camera;
            }
        }
    }

    auto FpsCameraController::removeCamera( SmartPtr<render::IGraphicsCamera> camera ) -> bool
    {
        auto it = std::find( m_cameras.begin(), m_cameras.end(), camera );
        if( it != m_cameras.end() )
        {
            m_cameras.erase( it );
            if( m_selectedCamera == camera )
            {
                m_selectedCamera = m_cameras.empty() ? nullptr : m_cameras.front();
            }
            return true;
        }
        return false;
    }

    void FpsCameraController::setMoveSpeed( real_Num speed )
    {
        m_moveSpeed = speed;
    }

    auto FpsCameraController::getMoveSpeed() const -> real_Num
    {
        return m_moveSpeed;
    }

    void FpsCameraController::setRotationSpeed( real_Num speed )
    {
        m_rotationSpeed = speed;
    }

    auto FpsCameraController::getRotationSpeed() const -> real_Num
    {
        return m_rotationSpeed;
    }

    real_Num FpsCameraController::getTranslationSpeed() const
    {
        return m_translationSpeed;
    }

    void FpsCameraController::setTranslationSpeed( real_Num speed )
    {
        m_translationSpeed = speed;
    }

    real_Num FpsCameraController::getMouseSensitivity() const
    {
        return m_mouseSensitivity;
    }

    void FpsCameraController::setMouseSensitivity( real_Num sensitivity )
    {
        m_mouseSensitivity = sensitivity;
    }

    real_Num FpsCameraController::getMaxPitchAngle() const
    {
        return m_maxPitchAngle;
    }

    void FpsCameraController::setMaxPitchAngle( real_Num degrees )
    {
        m_maxPitchAngle = Math<real_Num>::clamp( degrees, static_cast<real_Num>( 1.0 ),
                                                 static_cast<real_Num>( 89.9 ) );
        // Re-clamp current pitch immediately
        auto maxRad = Math<real_Num>::deg_to_rad() * m_maxPitchAngle;
        m_pitch = Math<real_Num>::clamp( m_pitch, -maxRad, maxRad );
        updateTargetDirection();
    }

    real_Num FpsCameraController::getWheelSpeedFactor() const
    {
        return m_wheelSpeedFactor;
    }

    void FpsCameraController::setWheelSpeedFactor( real_Num factor )
    {
        m_wheelSpeedFactor = factor;
    }

    real_Num FpsCameraController::getMinMoveSpeed() const
    {
        return m_minMoveSpeed;
    }

    void FpsCameraController::setMinMoveSpeed( real_Num speed )
    {
        m_minMoveSpeed = Math<real_Num>::max( speed, Math<real_Num>::epsilon() );
        m_maxMoveSpeed = Math<real_Num>::max( m_maxMoveSpeed, m_minMoveSpeed );
    }

    real_Num FpsCameraController::getMaxMoveSpeed() const
    {
        return m_maxMoveSpeed;
    }

    void FpsCameraController::setMaxMoveSpeed( real_Num speed )
    {
        m_maxMoveSpeed = Math<real_Num>::max( speed, m_minMoveSpeed );
    }

    real_Num FpsCameraController::getYaw() const
    {
        return m_yaw;
    }

    void FpsCameraController::setYaw( real_Num yaw )
    {
        m_yaw = yaw;
        updateTargetDirection();
    }

    real_Num FpsCameraController::getPitch() const
    {
        return m_pitch;
    }

    void FpsCameraController::setPitch( real_Num pitch )
    {
        auto maxRad = Math<real_Num>::deg_to_rad() * m_maxPitchAngle;
        m_pitch = Math<real_Num>::clamp( pitch, -maxRad, maxRad );
        updateTargetDirection();
    }

    void FpsCameraController::setPropertyValue( const String &name, const String &value )
    {
        if( name == moveSpeedStr || name == "MoveSpeed" )
        {
            m_translationSpeed = StringUtil::parseFloat( value );
        }
        else if( name == rotationSpeedStr || name == "RotationSpeed" )
        {
            m_rotationSpeed = StringUtil::parseFloat( value );
        }
        else if( name == "FarDistance" )
        {
            if( m_selectedCamera )
            {
                f32 farDistance = StringUtil::parseFloat( value );
                m_selectedCamera->setFarClipDistance( farDistance );
            }
        }
        else
        {
            WP_LOG_WARNING( "setPropertyValue: unknown property '" + name + "'" );
        }
    }

    FpsCameraController::SCamKeyMap::SCamKeyMap( s32 a, s32 k, s32 k1 ) :
        action( a ),
        keycode( k ),
        keycode1( k1 )
    {
    }

    FpsCameraController::SCamKeyMap::SCamKeyMap() = default;

}  // namespace workphone::scene
