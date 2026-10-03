#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Camera/ThirdPersonCameraController.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/Input/IInputDeviceManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, ThirdPersonCameraController, CameraController );

    const real_Num ThirdPersonCameraController::DEFAULT_DISTANCE = real_Num( 10.0 );
    const real_Num ThirdPersonCameraController::DEFAULT_HEIGHT = real_Num( 5.0 );
    const real_Num ThirdPersonCameraController::DEFAULT_HEIGHT_OFFSET = real_Num( 0.0 );
    const real_Num ThirdPersonCameraController::DEFAULT_ROTATION_SPEED = real_Num( 2.0 );
    const real_Num ThirdPersonCameraController::DEFAULT_ZOOM_SPEED = real_Num( 5.0 );
    const real_Num ThirdPersonCameraController::DEFAULT_MIN_DISTANCE = real_Num( 2.0 );
    const real_Num ThirdPersonCameraController::DEFAULT_MAX_DISTANCE = real_Num( 20.0 );
    const real_Num ThirdPersonCameraController::DEFAULT_MIN_HEIGHT = real_Num( 1.0 );
    const real_Num ThirdPersonCameraController::DEFAULT_MAX_HEIGHT = real_Num( 15.0 );
    const real_Num ThirdPersonCameraController::DEFAULT_DAMPING = real_Num( 2.0 );
    const int ThirdPersonCameraController::DEFAULT_ORBIT_MOUSE_BUTTON = 1;

    // Static const string definitions for properties
    const String ThirdPersonCameraController::targetStr = "target";
    const String ThirdPersonCameraController::distanceStr = "distance";
    const String ThirdPersonCameraController::heightStr = "height";
    const String ThirdPersonCameraController::heightOffsetStr = "heightOffset";
    const String ThirdPersonCameraController::rotationSpeedStr = "rotationSpeed";
    const String ThirdPersonCameraController::zoomSpeedStr = "zoomSpeed";
    const String ThirdPersonCameraController::minDistanceStr = "minDistance";
    const String ThirdPersonCameraController::maxDistanceStr = "maxDistance";
    const String ThirdPersonCameraController::minHeightStr = "minHeight";
    const String ThirdPersonCameraController::maxHeightStr = "maxHeight";
    const String ThirdPersonCameraController::dampingStr = "damping";
    const String ThirdPersonCameraController::lookAtOffsetStr = "lookAtOffset";
    const String ThirdPersonCameraController::orbitMouseButtonStr = "orbitMouseButton";
    const String ThirdPersonCameraController::resourceStr = "resource";
    const String ThirdPersonCameraController::emptyStr = "";

    ThirdPersonCameraController::ThirdPersonCameraController() :
        m_distance( DEFAULT_DISTANCE ),
        m_height( DEFAULT_HEIGHT ),
        m_heightOffset( DEFAULT_HEIGHT_OFFSET ),
        m_rotationSpeed( DEFAULT_ROTATION_SPEED ),
        m_zoomSpeed( DEFAULT_ZOOM_SPEED ),
        m_minDistance( DEFAULT_MIN_DISTANCE ),
        m_maxDistance( DEFAULT_MAX_DISTANCE ),
        m_minHeight( DEFAULT_MIN_HEIGHT ),
        m_maxHeight( DEFAULT_MAX_HEIGHT ),
        m_damping( DEFAULT_DAMPING ),
        m_lookAtOffset( Vector3<real_Num>::zero() ),
        m_orbitMouseButton( DEFAULT_ORBIT_MOUSE_BUTTON ),
        m_currentRotation( real_Num( 0.0 ) ),
        m_currentHeight( DEFAULT_HEIGHT ),
        m_currentDistance( DEFAULT_DISTANCE )
    {
    }

    ThirdPersonCameraController::~ThirdPersonCameraController()
    {
    }

    void ThirdPersonCameraController::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            CameraController::load( data );
            setLoadingState( LoadingState::Loaded );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ThirdPersonCameraController::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Unloaded )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );
            target = nullptr;
            CameraController::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ThirdPersonCameraController::update()
    {
        auto task = Thread::getCurrentTask();
        switch( task )
        {
        case TaskId::Application:
        {
            auto actor = getActor();
            if( !actor )
            {
                return;
            }

            auto enabled = isEnabled() && actor->isEnabledInScene();
            if( enabled && target )
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto timer = applicationManager->getTimer();
                auto inputDeviceManager = applicationManager->getInputDeviceManager();

                auto dt = static_cast<real_Num>( timer->getSmoothDeltaTime() );

                // Handle input
                if( inputDeviceManager )
                {
                    handleInput( inputDeviceManager, dt );
                }

                // Update camera position and rotation
                updateCameraTransform( dt );
            }
        }
        break;
        default:
        {
        }
        };
    }

    void ThirdPersonCameraController::postUpdate()
    {
        // Empty implementation - override if needed for specific post-update logic
    }

    void ThirdPersonCameraController::handleInput( SmartPtr<IInputDeviceManager> inputManager,
                                                   real_Num dt )
    {
        if( !inputManager )
        {
            return;
        }

        /*
        // Handle mouse scroll for zooming
        auto mouseScrollDelta = inputManager->getMouseScroll();
        if( mouseScrollDelta.y != 0.0f )
        {
            m_distance -= mouseScrollDelta.y * m_zoomSpeed * dt;
            m_distance = Math<real_Num>::clamp( m_distance, m_minDistance, m_maxDistance );
        }

        // Handle mouse movement for rotation (if right mouse button is held)
        if( inputManager->isMouseButtonPressed( m_orbitMouseButton ) )  // Orbit mouse button
        {
            auto mouseDelta = inputManager->getMouseMovement();
            m_currentRotation += mouseDelta.x * m_rotationSpeed * dt;

            // Handle vertical rotation (pitch) if needed
            auto pitchDelta = -mouseDelta.y * m_rotationSpeed * dt;
            m_height += pitchDelta;
            m_height = Math<real_Num>::clamp( m_height, m_minHeight, m_maxHeight );
        }
        */
    }

    void ThirdPersonCameraController::updateCameraTransform( real_Num dt )
    {
        if( !target )
        {
            return;
        }

        auto actor = getActor();
        if( !actor )
        {
            return;
        }

        auto targetTransform = target->getTransform();
        if( !targetTransform )
        {
            return;
        }

        auto targetPosition = targetTransform->getPosition();

        // Smooth damping for distance and height
        m_currentDistance = Math<real_Num>::lerp( m_currentDistance, m_distance, dt * m_damping );
        m_currentHeight = Math<real_Num>::lerp( m_currentHeight, m_height, dt * m_damping );

        // Calculate camera position
        auto offset = Vector3<real_Num>( Math<real_Num>::Sin( m_currentRotation ) * m_currentDistance,
                                         m_currentHeight + m_heightOffset,
                                         Math<real_Num>::Cos( m_currentRotation ) * m_currentDistance );

        auto cameraPosition = targetPosition + offset;
        auto lookAtPosition = targetPosition + m_lookAtOffset;

        // Set camera position and make it look at the target
        actor->setPosition( cameraPosition );
        actor->lookAt( lookAtPosition, Vector3<real_Num>::unitY() );
    }

    auto ThirdPersonCameraController::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto objects = CameraController::getChildObjects();
        if( target )
        {
            objects.emplace_back( target );
        }
        return objects;
    }

    auto ThirdPersonCameraController::getProperties() const -> SmartPtr<Properties>
    {
        try
        {
            auto properties = CameraController::getProperties();

            if( !properties )
            {
                WP_LOG_WARNING(
                    "ThirdPersonCameraController::getProperties: base class returned null properties." );
                return {};
            }

            if( target )
            {
                auto handle = target->getHandle();
                auto uuid = handle->getUUIDAsString();
                properties->setProperty( targetStr, uuid, resourceStr, false );
            }
            else
            {
                properties->setProperty( targetStr, emptyStr, resourceStr, false );
            }

            properties->setProperty( distanceStr, m_distance );
            properties->setProperty( heightStr, m_height );
            properties->setProperty( heightOffsetStr, m_heightOffset );
            properties->setProperty( rotationSpeedStr, m_rotationSpeed );
            properties->setProperty( zoomSpeedStr, m_zoomSpeed );
            properties->setProperty( minDistanceStr, m_minDistance );
            properties->setProperty( maxDistanceStr, m_maxDistance );
            properties->setProperty( minHeightStr, m_minHeight );
            properties->setProperty( maxHeightStr, m_maxHeight );
            properties->setProperty( dampingStr, m_damping );
            properties->setProperty( lookAtOffsetStr, m_lookAtOffset );
            properties->setProperty( orbitMouseButtonStr, m_orbitMouseButton );

            return properties;
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    void ThirdPersonCameraController::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            CameraController::setProperties( properties );

            if( !properties )
            {
                WP_LOG_WARNING(
                    "ThirdPersonCameraController::setProperties: null properties supplied." );
                return;
            }

            real_Num distance = m_distance;
            if( properties->getPropertyValue( distanceStr, distance ) )
            {
                setDistance( distance );
            }

            real_Num height = m_height;
            if( properties->getPropertyValue( heightStr, height ) )
            {
                setHeight( height );
            }

            real_Num heightOffset = m_heightOffset;
            if( properties->getPropertyValue( heightOffsetStr, heightOffset ) )
            {
                setHeightOffset( heightOffset );
            }

            real_Num rotationSpeed = m_rotationSpeed;
            if( properties->getPropertyValue( rotationSpeedStr, rotationSpeed ) )
            {
                setRotationSpeed( rotationSpeed );
            }

            real_Num zoomSpeed = m_zoomSpeed;
            if( properties->getPropertyValue( zoomSpeedStr, zoomSpeed ) )
            {
                setZoomSpeed( zoomSpeed );
            }

            real_Num minDistance = m_minDistance;
            if( properties->getPropertyValue( minDistanceStr, minDistance ) )
            {
                setMinDistance( minDistance );
            }

            real_Num maxDistance = m_maxDistance;
            if( properties->getPropertyValue( maxDistanceStr, maxDistance ) )
            {
                setMaxDistance( maxDistance );
            }

            real_Num minHeight = m_minHeight;
            if( properties->getPropertyValue( minHeightStr, minHeight ) )
            {
                setMinHeight( minHeight );
            }

            real_Num maxHeight = m_maxHeight;
            if( properties->getPropertyValue( maxHeightStr, maxHeight ) )
            {
                setMaxHeight( maxHeight );
            }

            real_Num damping = m_damping;
            if( properties->getPropertyValue( dampingStr, damping ) )
            {
                setDamping( damping );
            }

            Vector3<real_Num> lookAtOffset = m_lookAtOffset;
            if( properties->getPropertyValue( lookAtOffsetStr, lookAtOffset ) )
            {
                setLookAtOffset( lookAtOffset );
            }

            int orbitMouseButton = m_orbitMouseButton;
            if( properties->getPropertyValue( orbitMouseButtonStr, orbitMouseButton ) )
            {
                setOrbitMouseButton( orbitMouseButton );
            }

            // Synchronise interpolated values to newly set target values
            m_currentDistance = m_distance;
            m_currentHeight = m_height;
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto ThirdPersonCameraController::getTarget() const -> SmartPtr<IGameActor>
    {
        return target;
    }

    void ThirdPersonCameraController::setTarget( SmartPtr<IGameActor> newTarget )
    {
        target = newTarget;
    }

    auto ThirdPersonCameraController::getDistance() const -> real_Num
    {
        return m_distance;
    }

    void ThirdPersonCameraController::setDistance( real_Num distance )
    {
        m_distance = Math<real_Num>::clamp( distance, m_minDistance, m_maxDistance );
    }

    auto ThirdPersonCameraController::getHeight() const -> real_Num
    {
        return m_height;
    }

    void ThirdPersonCameraController::setHeight( real_Num height )
    {
        m_height = Math<real_Num>::clamp( height, m_minHeight, m_maxHeight );
    }

    auto ThirdPersonCameraController::getRotationSpeed() const -> real_Num
    {
        return m_rotationSpeed;
    }

    void ThirdPersonCameraController::setRotationSpeed( real_Num speed )
    {
        if( speed < real_Num( 0.0 ) )
        {
            WP_LOG_WARNING(
                "ThirdPersonCameraController::setRotationSpeed: value must be >= 0; ignoring." );
            return;
        }
        m_rotationSpeed = speed;
    }

    auto ThirdPersonCameraController::getZoomSpeed() const -> real_Num
    {
        return m_zoomSpeed;
    }

    void ThirdPersonCameraController::setZoomSpeed( real_Num speed )
    {
        if( speed < real_Num( 0.0 ) )
        {
            WP_LOG_WARNING( "ThirdPersonCameraController::setZoomSpeed: value must be >= 0; ignoring." );
            return;
        }
        m_zoomSpeed = speed;
    }

    auto ThirdPersonCameraController::getDamping() const -> real_Num
    {
        return m_damping;
    }

    void ThirdPersonCameraController::setDamping( real_Num damping )
    {
        if( damping < real_Num( 0.0 ) )
        {
            WP_LOG_WARNING( "ThirdPersonCameraController::setDamping: value must be >= 0; ignoring." );
            return;
        }
        m_damping = damping;
    }

    auto ThirdPersonCameraController::getLookAtOffset() const -> Vector3<real_Num>
    {
        return m_lookAtOffset;
    }

    void ThirdPersonCameraController::setLookAtOffset( const Vector3<real_Num> &offset )
    {
        m_lookAtOffset = offset;
    }

    auto ThirdPersonCameraController::getHeightOffset() const -> real_Num
    {
        return m_heightOffset;
    }

    void ThirdPersonCameraController::setHeightOffset( real_Num offset )
    {
        m_heightOffset = offset;
    }

    auto ThirdPersonCameraController::getMinDistance() const -> real_Num
    {
        return m_minDistance;
    }

    void ThirdPersonCameraController::setMinDistance( real_Num minDistance )
    {
        if( minDistance <= real_Num( 0.0 ) )
        {
            WP_LOG_WARNING(
                "ThirdPersonCameraController::setMinDistance: value must be > 0; ignoring." );
            return;
        }
        if( minDistance > m_maxDistance )
        {
            WP_LOG_WARNING(
                "ThirdPersonCameraController::setMinDistance: value exceeds maxDistance; clamping." );
            minDistance = m_maxDistance;
        }
        m_minDistance = minDistance;
        m_distance = Math<real_Num>::clamp( m_distance, m_minDistance, m_maxDistance );
    }

    auto ThirdPersonCameraController::getMaxDistance() const -> real_Num
    {
        return m_maxDistance;
    }

    void ThirdPersonCameraController::setMaxDistance( real_Num maxDistance )
    {
        if( maxDistance < m_minDistance )
        {
            WP_LOG_WARNING(
                "ThirdPersonCameraController::setMaxDistance: value is less than minDistance; "
                "clamping." );
            maxDistance = m_minDistance;
        }
        m_maxDistance = maxDistance;
        m_distance = Math<real_Num>::clamp( m_distance, m_minDistance, m_maxDistance );
    }

    auto ThirdPersonCameraController::getMinHeight() const -> real_Num
    {
        return m_minHeight;
    }

    void ThirdPersonCameraController::setMinHeight( real_Num minHeight )
    {
        if( minHeight > m_maxHeight )
        {
            WP_LOG_WARNING(
                "ThirdPersonCameraController::setMinHeight: value exceeds maxHeight; clamping." );
            minHeight = m_maxHeight;
        }
        m_minHeight = minHeight;
        m_height = Math<real_Num>::clamp( m_height, m_minHeight, m_maxHeight );
    }

    auto ThirdPersonCameraController::getMaxHeight() const -> real_Num
    {
        return m_maxHeight;
    }

    void ThirdPersonCameraController::setMaxHeight( real_Num maxHeight )
    {
        if( maxHeight < m_minHeight )
        {
            WP_LOG_WARNING(
                "ThirdPersonCameraController::setMaxHeight: value is less than minHeight; clamping." );
            maxHeight = m_minHeight;
        }
        m_maxHeight = maxHeight;
        m_height = Math<real_Num>::clamp( m_height, m_minHeight, m_maxHeight );
    }

    auto ThirdPersonCameraController::getOrbitMouseButton() const -> int
    {
        return m_orbitMouseButton;
    }

    void ThirdPersonCameraController::setOrbitMouseButton( int button )
    {
        if( button < 0 || button > 2 )
        {
            WP_LOG_WARNING(
                "ThirdPersonCameraController::setOrbitMouseButton: invalid button index; must be 0, 1, "
                "or 2; ignoring." );
            return;
        }
        m_orbitMouseButton = button;
    }

}  // namespace workphone::scene
