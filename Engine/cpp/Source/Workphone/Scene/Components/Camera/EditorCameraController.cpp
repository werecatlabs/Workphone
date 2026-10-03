#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Camera/EditorCameraController.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
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
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/Input/IInputDeviceManager.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Interface/Input/IKeyboardState.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Math/Math.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, EditorCameraController, CameraController );

    // Property key string initialisers
    const String EditorCameraController::positionStr = "Position";
    const String EditorCameraController::targetPositionStr = "TargetPosition";
    const String EditorCameraController::moveSpeedStr = "MoveSpeed";
    const String EditorCameraController::rotationSpeedStr = "RotationSpeed";
    const String EditorCameraController::translationSpeedStr = "TranslationSpeed";
    const String EditorCameraController::panSpeedStr = "PanSpeed";
    const String EditorCameraController::dollySpeedStr = "DollySpeed";
    const String EditorCameraController::orbitDistanceStr = "OrbitDistance";
    const String EditorCameraController::minOrbitDistanceStr = "MinOrbitDistance";
    const String EditorCameraController::maxOrbitDistanceStr = "MaxOrbitDistance";
    const String EditorCameraController::invertStr = "Invert";
    const String EditorCameraController::shiftSpeedMultiplierStr = "ShiftSpeedMultiplier";
    const String EditorCameraController::orbitSensitivityStr = "OrbitSensitivity";
    const String EditorCameraController::panSensitivityStr = "PanSensitivity";
    const String EditorCameraController::dollySensitivityStr = "DollySensitivity";
    const String EditorCameraController::freeLookSensitivityStr = "FreeLookSensitivity";
    const String EditorCameraController::wheelSensitivityStr = "WheelSensitivity";
    const String EditorCameraController::maxPitchAngleStr = "MaxPitchAngle";

    /** Key mapping actions for camera controls. */
    enum class CameraAction
    {
        None = 0,
        MoveForward,
        MoveBackward,
        MoveLeft,
        MoveRight,
        MoveUp,
        MoveDown,
        Rotate,
        Pan,
        Zoom,
        Focus
    };

    /** Virtual key codes for modifier keys. */
    namespace KeyCodes
    {
        constexpr s32 Alt = 0x12;       // VK_MENU
        constexpr s32 LeftAlt = 0xA4;   // VK_LMENU
        constexpr s32 RightAlt = 0xA5;  // VK_RMENU
        constexpr s32 Shift = 0x10;     // VK_SHIFT
        constexpr s32 Control = 0x11;   // VK_CONTROL
        constexpr s32 KeyF = 'F';
    }  // namespace KeyCodes

    EditorCameraController::SCamKeyMap::SCamKeyMap() : action( 0 ), keycode( 0 ), keycode1( 0 )
    {
    }

    EditorCameraController::SCamKeyMap::SCamKeyMap( s32 actionType, s32 primaryKey, s32 secondaryKey ) :
        action( actionType ),
        keycode( primaryKey ),
        keycode1( secondaryKey )
    {
    }

    EditorCameraController::EditorCameraController() :
        m_targetVector( Vector3<real_Num>::zero() ),
        m_prevCursorPos( Vector2<real_Num>::zero() ),
        m_cursorPos( Vector2<real_Num>::zero() ),
        m_mousePos( Vector2<real_Num>::zero() ),
        m_relativeMouse( Vector2<real_Num>::zero() ),
        m_position( Vector3<real_Num>( 0, 5, 10 ) ),
        m_targetPosition( Vector3<real_Num>::zero() ),
        m_rotation( Vector3<real_Num>::zero() ),
        m_moveSpeed( 100.0f ),
        m_rotationSpeed( 100.0f ),
        m_translationSpeed( 100.0f ),
        m_orbitDistance( 100.0f ),
        m_panSpeed( 100.0f ),
        m_dollySpeed( 100.0f ),
        m_minOrbitDistance( 0.1f ),
        m_maxOrbitDistance( 1000.0f ),
        m_bInvert( false ),
        m_controlMode( CameraControlMode::None )
    {
        // Initialize mouse key states (8 buttons should cover most mice)
        m_mouseKeys.resize( 8, false );

        // Initialize keyboard key states (256 virtual key codes)
        m_cursorKeys.resize( 256, false );

        // Set up default key mappings
        setupDefaultKeyMappings();

        // Calculate initial orbit distance from position and target
        updateOrbitDistance();
    }

    EditorCameraController::~EditorCameraController()
    {
    }

    void EditorCameraController::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            CameraController::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            if( applicationManager )
            {
                auto inputManager = applicationManager->getInputDeviceManager();
                if( inputManager )
                {
                    auto inputListener = workphone::make_ptr<EditorCameraInputListener>();
                    inputListener->setOwner( this );
                    inputManager->addListener( inputListener );
                    m_inputListener = inputListener;
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EditorCameraController::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            m_cameras.clear();
            allKeysUp();

            auto applicationManager = core::IApplicationManager::instance();
            auto inputManager = applicationManager->getInputDeviceManager();
            if( inputManager )
            {
                if( m_inputListener )
                {
                    m_inputListener->unload( nullptr );
                    inputManager->removeListener( m_inputListener );
                    m_inputListener = nullptr;
                }
            }

            CameraController::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EditorCameraController::update()
    {
        auto task = Thread::getCurrentTask();
        switch( task )
        {
        case TaskId::Application:
        {
            if( auto actor = getActorPtr() )
            {
                auto enabled = isEnabled() && actor->isEnabledInScene();
                if( enabled )
                {
                    auto applicationManager = core::IApplicationManager::instancePtr();
                    auto timer = applicationManager->getTimerPtr();
                    auto dt = static_cast<real_Num>( timer->getSmoothDeltaTime() );

                    updateMovement( dt );
                    updateRotation( dt );
                    updateCameraTransform();
                }
            }
        }
        break;
        default:
            break;
        }
    }

    bool EditorCameraController::handleInputEvent( const SmartPtr<IInputEvent> &event )
    {
        if( !event )
        {
            return false;
        }

        auto mouseState = event->getMouseState();
        auto keyboardState = event->getKeyboardState();

        // Handle keyboard first to update modifier key states
        if( keyboardState )
        {
            handleKeyboardInput( keyboardState );
        }

        if( mouseState )
        {
            handleMouseInput( mouseState );
        }

        return true;
    }

    void EditorCameraController::setPosition( const Vector3<real_Num> &position )
    {
        m_position = position;
        updateOrbitDistance();

        if( auto actor = getActor() )
        {
            if( auto transform = actor->getTransform() )
            {
                transform->setPosition( position );
            }
        }
    }

    Vector3<real_Num> EditorCameraController::getPosition() const
    {
        return m_position;
    }

    void EditorCameraController::setTargetPosition( const Vector3<real_Num> &position )
    {
        m_targetPosition = position;
        updateOrbitDistance();
    }

    Vector3<real_Num> EditorCameraController::getTargetPosition() const
    {
        return m_targetPosition;
    }

    void EditorCameraController::setOrientation( const Quaternion<real_Num> &orientation )
    {
        if( auto actor = getActor() )
        {
            if( auto transform = actor->getTransform() )
            {
                transform->setOrientation( orientation );
            }
        }
    }

    Quaternion<real_Num> EditorCameraController::getOrientation() const
    {
        if( auto actor = getActor() )
        {
            if( auto transform = actor->getTransform() )
            {
                return transform->getOrientation();
            }
        }
        return Quaternion<real_Num>::identity();
    }

    void EditorCameraController::setDirection( const Vector3<real_Num> &direction )
    {
        if( auto actor = getActor() )
        {
            if( auto transform = actor->getTransform() )
            {
                auto position = transform->getPosition();
                auto target = position + direction.normaliseCopy();
                transform->lookAt( target, Vector3<real_Num>::unitY() );
            }
        }
    }

    Vector3<real_Num> EditorCameraController::getDirection() const
    {
        if( auto actor = getActorPtr() )
        {
            if( auto transform = actor->getTransformPtr() )
            {
                return transform->getForward();
            }
        }
        return Vector3<real_Num>::unitZ();
    }

    Vector3<real_Num> EditorCameraController::getUp() const
    {
        if( auto actor = getActorPtr() )
        {
            if( auto transform = actor->getTransformPtr() )
            {
                return transform->getUp();
            }
        }
        return Vector3<real_Num>::unitY();
    }

    Vector3<real_Num> EditorCameraController::getRight() const
    {
        if( auto actor = getActorPtr() )
        {
            if( auto transform = actor->getTransformPtr() )
            {
                return transform->getRight();
            }
        }
        return Vector3<real_Num>::unitX();
    }

    void EditorCameraController::addCamera( SmartPtr<render::IGraphicsCamera> camera )
    {
        if( camera )
        {
            m_cameras.push_back( camera );
        }
    }

    bool EditorCameraController::removeCamera( SmartPtr<render::IGraphicsCamera> camera )
    {
        if( camera )
        {
            auto it = std::find( m_cameras.begin(), m_cameras.end(), camera );
            if( it != m_cameras.end() )
            {
                m_cameras.erase( it );
                return true;
            }
        }
        return false;
    }

    f32 EditorCameraController::getRotationSpeed() const
    {
        return m_rotationSpeed;
    }

    void EditorCameraController::setRotationSpeed( f32 rotationSpeed )
    {
        m_rotationSpeed = rotationSpeed;
    }

    bool EditorCameraController::getInvert() const
    {
        return m_bInvert;
    }

    void EditorCameraController::setInvert( bool inverted )
    {
        m_bInvert = inverted;
    }

    f32 EditorCameraController::getMoveSpeed() const
    {
        return m_moveSpeed;
    }

    void EditorCameraController::setMoveSpeed( f32 speed )
    {
        m_moveSpeed = speed;
    }

    f32 EditorCameraController::getTranslationSpeed() const
    {
        return m_translationSpeed;
    }

    void EditorCameraController::setTranslationSpeed( f32 speed )
    {
        m_translationSpeed = speed;
    }

    f32 EditorCameraController::getOrbitDistance() const
    {
        return m_orbitDistance;
    }

    void EditorCameraController::setOrbitDistance( f32 distance )
    {
        m_orbitDistance = Math<real_Num>::clamp( distance, m_minOrbitDistance, m_maxOrbitDistance );
        updatePositionFromOrbit();
    }

    f32 EditorCameraController::getPanSpeed() const
    {
        return m_panSpeed;
    }

    void EditorCameraController::setPanSpeed( f32 panSpeed )
    {
        m_panSpeed = panSpeed;
    }

    f32 EditorCameraController::getDollySpeed() const
    {
        return m_dollySpeed;
    }

    void EditorCameraController::setDollySpeed( f32 dollySpeed )
    {
        m_dollySpeed = dollySpeed;
    }

    f32 EditorCameraController::getMinOrbitDistance() const
    {
        return m_minOrbitDistance;
    }

    void EditorCameraController::setMinOrbitDistance( f32 distance )
    {
        m_minOrbitDistance = Math<real_Num>::max( distance, Math<real_Num>::epsilon() );
        // Clamp current orbit distance to the updated limits
        m_orbitDistance =
            Math<real_Num>::clamp( m_orbitDistance, m_minOrbitDistance, m_maxOrbitDistance );
    }

    f32 EditorCameraController::getMaxOrbitDistance() const
    {
        return m_maxOrbitDistance;
    }

    void EditorCameraController::setMaxOrbitDistance( f32 distance )
    {
        m_maxOrbitDistance = Math<real_Num>::max( distance, m_minOrbitDistance );
        m_orbitDistance =
            Math<real_Num>::clamp( m_orbitDistance, m_minOrbitDistance, m_maxOrbitDistance );
    }

    f32 EditorCameraController::getShiftSpeedMultiplier() const
    {
        return m_shiftSpeedMultiplier;
    }

    void EditorCameraController::setShiftSpeedMultiplier( f32 multiplier )
    {
        m_shiftSpeedMultiplier = multiplier;
    }

    f32 EditorCameraController::getOrbitSensitivity() const
    {
        return m_orbitSensitivity;
    }

    void EditorCameraController::setOrbitSensitivity( f32 sensitivity )
    {
        m_orbitSensitivity = sensitivity;
    }

    f32 EditorCameraController::getPanSensitivity() const
    {
        return m_panSensitivity;
    }

    void EditorCameraController::setPanSensitivity( f32 sensitivity )
    {
        m_panSensitivity = sensitivity;
    }

    f32 EditorCameraController::getDollySensitivity() const
    {
        return m_dollySensitivity;
    }

    void EditorCameraController::setDollySensitivity( f32 sensitivity )
    {
        m_dollySensitivity = sensitivity;
    }

    f32 EditorCameraController::getFreeLookSensitivity() const
    {
        return m_freeLookSensitivity;
    }

    void EditorCameraController::setFreeLookSensitivity( f32 sensitivity )
    {
        m_freeLookSensitivity = sensitivity;
    }

    f32 EditorCameraController::getWheelSensitivity() const
    {
        return m_wheelSensitivity;
    }

    void EditorCameraController::setWheelSensitivity( f32 sensitivity )
    {
        m_wheelSensitivity = sensitivity;
    }

    f32 EditorCameraController::getMaxPitchAngle() const
    {
        return m_maxPitchAngle;
    }

    void EditorCameraController::setMaxPitchAngle( f32 degrees )
    {
        m_maxPitchAngle = Math<real_Num>::clamp( degrees, 1.0f, 89.9f );
    }

    void EditorCameraController::focusOnTarget()
    {
        focusOnPosition( m_targetPosition, 0.0f );
    }

    void EditorCameraController::focusOnPosition( const Vector3<real_Num> &targetPosition, f32 distance )
    {
        m_targetPosition = targetPosition;

        // If distance is 0, calculate a reasonable viewing distance
        if( distance <= Math<real_Num>::epsilon() )
        {
            distance = m_orbitDistance;
        }

        m_orbitDistance = Math<real_Num>::clamp( distance, m_minOrbitDistance, m_maxOrbitDistance );
        updatePositionFromOrbit();
    }

    CameraControlMode EditorCameraController::getControlMode() const
    {
        return m_controlMode;
    }

    auto EditorCameraController::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = CameraController::getProperties();

        // Camera transform state
        properties->setProperty( positionStr, m_position );
        properties->setProperty( targetPositionStr, m_targetPosition );

        // Speed settings
        properties->setProperty( moveSpeedStr, m_moveSpeed );
        properties->setProperty( rotationSpeedStr, m_rotationSpeed );
        properties->setProperty( translationSpeedStr, m_translationSpeed );
        properties->setProperty( panSpeedStr, m_panSpeed );
        properties->setProperty( dollySpeedStr, m_dollySpeed );

        // Orbit distance
        properties->setProperty( orbitDistanceStr, m_orbitDistance );
        properties->setProperty( minOrbitDistanceStr, m_minOrbitDistance );
        properties->setProperty( maxOrbitDistanceStr, m_maxOrbitDistance );

        // Mouse behaviour
        properties->setProperty( invertStr, m_bInvert );
        properties->setProperty( shiftSpeedMultiplierStr, m_shiftSpeedMultiplier );
        properties->setProperty( orbitSensitivityStr, m_orbitSensitivity );
        properties->setProperty( panSensitivityStr, m_panSensitivity );
        properties->setProperty( dollySensitivityStr, m_dollySensitivity );
        properties->setProperty( freeLookSensitivityStr, m_freeLookSensitivity );
        properties->setProperty( wheelSensitivityStr, m_wheelSensitivity );
        properties->setProperty( maxPitchAngleStr, m_maxPitchAngle );

        return properties;
    }

    void EditorCameraController::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            if( !properties )
            {
                WP_LOG_WARNING( "setProperties called with null properties" );
                return;
            }

            CameraController::setProperties( properties );

            // Camera transform state
            properties->getPropertyValue( positionStr, m_position );
            properties->getPropertyValue( targetPositionStr, m_targetPosition );
            updateOrbitDistance();

            // Speed settings
            properties->getPropertyValue( moveSpeedStr, m_moveSpeed );
            properties->getPropertyValue( rotationSpeedStr, m_rotationSpeed );
            properties->getPropertyValue( translationSpeedStr, m_translationSpeed );
            properties->getPropertyValue( panSpeedStr, m_panSpeed );
            properties->getPropertyValue( dollySpeedStr, m_dollySpeed );

            // Orbit distance limits (apply limits before restoring current distance)
            properties->getPropertyValue( minOrbitDistanceStr, m_minOrbitDistance );
            m_minOrbitDistance = Math<real_Num>::max( m_minOrbitDistance, Math<real_Num>::epsilon() );

            properties->getPropertyValue( maxOrbitDistanceStr, m_maxOrbitDistance );
            m_maxOrbitDistance = Math<real_Num>::max( m_maxOrbitDistance, m_minOrbitDistance );

            f32 orbitDistance = m_orbitDistance;
            if( properties->getPropertyValue( orbitDistanceStr, orbitDistance ) )
            {
                m_orbitDistance =
                    Math<real_Num>::clamp( orbitDistance, m_minOrbitDistance, m_maxOrbitDistance );
            }

            // Mouse behaviour
            properties->getPropertyValue( invertStr, m_bInvert );
            properties->getPropertyValue( shiftSpeedMultiplierStr, m_shiftSpeedMultiplier );
            properties->getPropertyValue( orbitSensitivityStr, m_orbitSensitivity );
            properties->getPropertyValue( panSensitivityStr, m_panSensitivity );
            properties->getPropertyValue( dollySensitivityStr, m_dollySensitivity );
            properties->getPropertyValue( freeLookSensitivityStr, m_freeLookSensitivity );
            properties->getPropertyValue( wheelSensitivityStr, m_wheelSensitivity );

            f32 maxPitch = m_maxPitchAngle;
            if( properties->getPropertyValue( maxPitchAngleStr, maxPitch ) )
            {
                m_maxPitchAngle = Math<real_Num>::clamp( maxPitch, 1.0f, 89.9f );
            }
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void EditorCameraController::setupDefaultKeyMappings()
    {
        m_keyMap.clear();

        // WASD movement keys (FPS-style when no modifier)
        m_keyMap.emplace_back( static_cast<s32>( CameraAction::MoveForward ), 'W', 0 );
        m_keyMap.emplace_back( static_cast<s32>( CameraAction::MoveBackward ), 'S', 0 );
        m_keyMap.emplace_back( static_cast<s32>( CameraAction::MoveLeft ), 'A', 0 );
        m_keyMap.emplace_back( static_cast<s32>( CameraAction::MoveRight ), 'D', 0 );
        m_keyMap.emplace_back( static_cast<s32>( CameraAction::MoveUp ), 'Q', 0 );
        m_keyMap.emplace_back( static_cast<s32>( CameraAction::MoveDown ), 'E', 0 );

        // Alternative movement with arrow keys
        m_keyMap.emplace_back( static_cast<s32>( CameraAction::MoveForward ), 0x26, 0 );   // Up arrow
        m_keyMap.emplace_back( static_cast<s32>( CameraAction::MoveBackward ), 0x28, 0 );  // Down arrow
        m_keyMap.emplace_back( static_cast<s32>( CameraAction::MoveLeft ), 0x25, 0 );      // Left arrow
        m_keyMap.emplace_back( static_cast<s32>( CameraAction::MoveRight ), 0x27, 0 );     // Right arrow

        // Focus key (Maya-style frame selection)
        m_keyMap.emplace_back( static_cast<s32>( CameraAction::Focus ), KeyCodes::KeyF, 0 );
    }

    void EditorCameraController::handleMouseInput( SmartPtr<IMouseState> mouseState )
    {
        if( !mouseState )
        {
            return;
        }

        // Update mouse position tracking
        m_prevCursorPos = m_cursorPos;
        auto mousePos = mouseState->getRelativePosition();
        m_cursorPos = Vector2<real_Num>( mousePos.X(), mousePos.Y() );
        m_relativeMouse = m_cursorPos - m_prevCursorPos;

        // Update mouse button states
        for( size_t i = 0; i < m_mouseKeys.size() && i < 8; ++i )
        {
            m_mouseKeys[i] = mouseState->isButtonPressed( static_cast<u32>( i ) );
        }

        // Determine control mode based on modifier keys and mouse buttons
        bool altPressed = isAltKeyDown();
        bool leftMouseDown = isMouseKeyDown( 0 );
        bool middleMouseDown = isMouseKeyDown( 2 );
        bool rightMouseDown = isMouseKeyDown( 1 );

        // Maya-style controls: Alt + mouse buttons
        if( altPressed )
        {
            if( leftMouseDown )
            {
                // Alt + LMB: Orbit around target
                m_controlMode = CameraControlMode::Orbit;
                auto sensitivity = m_rotationSpeed * m_orbitSensitivity;
                auto deltaYaw = m_relativeMouse.X() * sensitivity;
                auto deltaPitch = m_relativeMouse.Y() * sensitivity * ( m_bInvert ? -1.0f : 1.0f );
                performOrbit( deltaYaw, deltaPitch );
            }
            else if( middleMouseDown )
            {
                // Alt + MMB: Pan
                m_controlMode = CameraControlMode::Pan;
                auto panSensitivity = m_panSpeed * m_orbitDistance * m_panSensitivity;
                performPan( -m_relativeMouse.X() * panSensitivity,
                            m_relativeMouse.Y() * panSensitivity );
            }
            else if( rightMouseDown )
            {
                // Alt + RMB: Dolly
                m_controlMode = CameraControlMode::Dolly;
                auto dollySensitivity = m_dollySpeed * m_orbitDistance * m_dollySensitivity;
                auto dollyAmount = ( m_relativeMouse.X() + m_relativeMouse.Y() ) * dollySensitivity;
                performDolly( dollyAmount );
            }
            else
            {
                m_controlMode = CameraControlMode::None;
            }
        }
        else
        {
            // Non-Maya controls (fallback for quick navigation)
            if( rightMouseDown )
            {
                // RMB without Alt: Free look rotation (FPS-style)
                m_controlMode = CameraControlMode::FreeLook;
                auto sensitivity = m_rotationSpeed * m_freeLookSensitivity;
                m_rotation.X() += m_relativeMouse.Y() * sensitivity * ( m_bInvert ? -1.0f : 1.0f );
                m_rotation.Y() += m_relativeMouse.X() * sensitivity;

                // Clamp pitch to prevent camera flipping
                m_rotation.X() =
                    Math<real_Num>::clamp( m_rotation.X(), Math<real_Num>::DegToRad( -m_maxPitchAngle ),
                                           Math<real_Num>::DegToRad( m_maxPitchAngle ) );

                // Update target position to maintain orbit distance in front of camera
                auto forward = getDirection();
                m_targetPosition = m_position + forward * m_orbitDistance;
            }
            else if( middleMouseDown )
            {
                // MMB without Alt: Pan (convenience shortcut)
                m_controlMode = CameraControlMode::Pan;
                auto panSensitivity = m_panSpeed * m_orbitDistance * m_panSensitivity;
                performPan( -m_relativeMouse.X() * panSensitivity,
                            m_relativeMouse.Y() * panSensitivity );
            }
            else
            {
                m_controlMode = CameraControlMode::None;
            }
        }

        // Handle zoom with mouse wheel (always active)
        auto scroll = mouseState->getWheelDelta();
        if( Math<real_Num>::Abs( scroll.Y() ) > Math<real_Num>::epsilon() )
        {
            auto zoomSensitivity = m_dollySpeed * m_orbitDistance * m_wheelSensitivity;
            performDolly( scroll.Y() * zoomSensitivity );
        }
    }

    void EditorCameraController::handleKeyboardInput( SmartPtr<IKeyboardState> keyboardState )
    {
        if( !keyboardState )
        {
            return;
        }

        // Update cursor key states
        for( size_t i = 0; i < m_cursorKeys.size(); ++i )
        {
            m_cursorKeys[i] = keyboardState->isPressedDown( static_cast<u32>( i ) );
        }

        // Check for focus key press (F key - Maya-style frame)
        if( m_cursorKeys[KeyCodes::KeyF] )
        {
            focusOnTarget();
        }
    }

    void EditorCameraController::updateMovement( f32 deltaTime )
    {
        // Only process WASD movement when not in a mouse control mode
        if( m_controlMode != CameraControlMode::None && m_controlMode != CameraControlMode::FreeLook )
        {
            return;
        }

        Vector3<real_Num> movement = Vector3<real_Num>::zero();

        // Check for movement input based on key mappings
        for( const auto &keyMap : m_keyMap )
        {
            bool isPressed = false;

            // Check primary keycode
            if( keyMap.keycode >= 0 && keyMap.keycode < static_cast<s32>( m_cursorKeys.size() ) )
            {
                isPressed = m_cursorKeys[keyMap.keycode];
            }

            // Check secondary keycode if available
            if( !isPressed && keyMap.keycode1 > 0 &&
                keyMap.keycode1 < static_cast<s32>( m_cursorKeys.size() ) )
            {
                isPressed = m_cursorKeys[keyMap.keycode1];
            }

            if( isPressed )
            {
                auto action = static_cast<CameraAction>( keyMap.action );
                switch( action )
                {
                case CameraAction::MoveForward:
                    movement += getDirection();
                    break;
                case CameraAction::MoveBackward:
                    movement -= getDirection();
                    break;
                case CameraAction::MoveLeft:
                    movement -= getRight();
                    break;
                case CameraAction::MoveRight:
                    movement += getRight();
                    break;
                case CameraAction::MoveUp:
                    movement += Vector3<real_Num>::unitY();  // World up for consistency
                    break;
                case CameraAction::MoveDown:
                    movement -= Vector3<real_Num>::unitY();  // World down for consistency
                    break;
                default:
                    break;
                }
            }
        }

        // Apply movement with Shift modifier for speed boost
        if( movement.length() > Math<real_Num>::epsilon() )
        {
            movement.normalise();
            auto speed = m_moveSpeed;
            if( isShiftKeyDown() )
            {
                speed *= m_shiftSpeedMultiplier;
            }

            m_position += movement * speed * deltaTime;

            // Update target position to maintain relative offset
            m_targetPosition += movement * speed * deltaTime;
        }
    }

    void EditorCameraController::updateRotation( f32 deltaTime )
    {
        // Rotation is handled directly in mouse input handlers
        // This method can be used for smooth rotation interpolation if needed
    }

    void EditorCameraController::updateCameraTransform()
    {
        auto actor = getActorPtr();
        if( !actor )
        {
            return;
        }

        auto transform = actor->getTransformPtr();
        if( !transform )
        {
            return;
        }

        // Set position
        transform->setPosition( m_position );

        // Calculate orientation from rotation angles
        auto pitch = Quaternion<real_Num>::angleAxis( m_rotation.X(), Vector3<real_Num>::unitX() );
        auto yaw = Quaternion<real_Num>::angleAxis( m_rotation.Y(), Vector3<real_Num>::unitY() );
        auto roll = Quaternion<real_Num>::angleAxis( m_rotation.Z(), Vector3<real_Num>::unitZ() );

        // Apply rotations in yaw-pitch-roll order
        auto orientation = yaw * pitch * roll;
        transform->setOrientation( orientation );

        transform->setLocalDirty( true );
    }

    void EditorCameraController::performOrbit( f32 deltaYaw, f32 deltaPitch )
    {
        // Update rotation angles
        m_rotation.Y() += deltaYaw;
        m_rotation.X() += deltaPitch;

        // Clamp pitch to prevent flipping over the poles
        m_rotation.X() =
            Math<real_Num>::clamp( m_rotation.X(), Math<real_Num>::DegToRad( -m_maxPitchAngle ),
                                   Math<real_Num>::DegToRad( m_maxPitchAngle ) );

        // Recalculate camera position based on orbit parameters
        updatePositionFromOrbit();
    }

    void EditorCameraController::performPan( f32 deltaX, f32 deltaY )
    {
        auto right = getRight();
        auto up = getUp();

        // Calculate pan offset in world space
        auto panOffset = right * deltaX + up * deltaY;

        // Move both camera and target by the same amount
        m_position += panOffset;
        m_targetPosition += panOffset;
    }

    void EditorCameraController::performDolly( f32 deltaDistance )
    {
        // Calculate new orbit distance
        auto newDistance = m_orbitDistance - deltaDistance;
        m_orbitDistance = Math<real_Num>::clamp( newDistance, m_minOrbitDistance, m_maxOrbitDistance );

        // Update camera position to maintain orbit
        updatePositionFromOrbit();
    }

    void EditorCameraController::updateOrbitDistance()
    {
        auto offset = m_position - m_targetPosition;
        m_orbitDistance = offset.length();

        if( m_orbitDistance < m_minOrbitDistance )
        {
            m_orbitDistance = m_minOrbitDistance;
        }
    }

    void EditorCameraController::updatePositionFromOrbit()
    {
        // Calculate camera position from spherical coordinates around target
        auto cosPitch = Math<real_Num>::Cos( m_rotation.X() );
        auto sinPitch = Math<real_Num>::Sin( m_rotation.X() );
        auto cosYaw = Math<real_Num>::Cos( m_rotation.Y() );
        auto sinYaw = Math<real_Num>::Sin( m_rotation.Y() );

        // Camera offset from target (spherical to Cartesian)
        Vector3<real_Num> offset;
        offset.X() = m_orbitDistance * cosPitch * sinYaw;
        offset.Y() = m_orbitDistance * sinPitch;
        offset.Z() = m_orbitDistance * cosPitch * cosYaw;

        m_position = m_targetPosition + offset;
    }

    bool EditorCameraController::isMouseKeyDown( s32 buttonIndex )
    {
        if( buttonIndex >= 0 && buttonIndex < static_cast<s32>( m_mouseKeys.size() ) )
        {
            return m_mouseKeys[buttonIndex];
        }
        return false;
    }

    bool EditorCameraController::isAltKeyDown() const
    {
        // Check all Alt key variants
        return m_cursorKeys[KeyCodes::Alt] || m_cursorKeys[KeyCodes::LeftAlt] ||
               m_cursorKeys[KeyCodes::RightAlt];
    }

    bool EditorCameraController::isShiftKeyDown() const
    {
        return m_cursorKeys[KeyCodes::Shift];
    }

    bool EditorCameraController::isCtrlKeyDown() const
    {
        return m_cursorKeys[KeyCodes::Control];
    }

    void EditorCameraController::allKeysUp()
    {
        std::fill( m_mouseKeys.begin(), m_mouseKeys.end(), false );
        std::fill( m_cursorKeys.begin(), m_cursorKeys.end(), false );
        m_controlMode = CameraControlMode::None;
    }

    EditorCameraController::EditorCameraInputListener::EditorCameraInputListener() = default;
    EditorCameraController::EditorCameraInputListener::~EditorCameraInputListener() = default;

    Parameter EditorCameraController::EditorCameraInputListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( auto owner = getOwner() )
        {
            auto result = owner->handleInputEvent( event );
            return Parameter( result );
        }

        return {};
    }

    void EditorCameraController::EditorCameraInputListener::setOwner(
        SmartPtr<EditorCameraController> owner )
    {
        m_owner = owner;
    }

    SmartPtr<EditorCameraController> EditorCameraController::EditorCameraInputListener::getOwner() const
    {
        return m_owner;
    }

}  // namespace workphone::scene
