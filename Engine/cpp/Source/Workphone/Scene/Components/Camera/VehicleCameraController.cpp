#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Camera/VehicleCameraController.hpp>
#include <Workphone/Scene/Components/VehicleController.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/MathUtil.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/Input/IInputDeviceManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, VehicleCameraController, CameraController );

    const real_Num VehicleCameraController::DEFAULT_DISTANCE = real_Num( 5.0 );
    const real_Num VehicleCameraController::DEFAULT_MIN_DISTANCE = real_Num( 0.0 );
    const real_Num VehicleCameraController::DEFAULT_MAX_DISTANCE = real_Num( 10.0 );
    const real_Num VehicleCameraController::DEFAULT_TARGET_OFFSET = real_Num( 1.0 );
    const real_Num VehicleCameraController::DEFAULT_HEIGHT = real_Num( 1.5 );
    const real_Num VehicleCameraController::DEFAULT_MIN_HEIGHT = real_Num( 0.0 );
    const real_Num VehicleCameraController::DEFAULT_MAX_HEIGHT = real_Num( 5.0 );
    const real_Num VehicleCameraController::DEFAULT_HEIGHT_DAMPING = real_Num( 2.0 );
    const real_Num VehicleCameraController::DEFAULT_LOOK_AT_HEIGHT = real_Num( 0.0 );
    const real_Num VehicleCameraController::DEFAULT_ROTATION_SNAP_TIME = real_Num( 0.35 );
    const real_Num VehicleCameraController::DEFAULT_MIN_ROTATION_SNAP_TIME = real_Num( 0.0 );
    const real_Num VehicleCameraController::DEFAULT_MAX_ROTATION_SNAP_TIME = real_Num( 3.0 );
    const real_Num VehicleCameraController::DEFAULT_DISTANCE_SNAP_TIME = real_Num( 1.5 );
    const real_Num VehicleCameraController::DEFAULT_MIN_DISTANCE_SNAP_TIME = real_Num( 0.0 );
    const real_Num VehicleCameraController::DEFAULT_MAX_DISTANCE_SNAP_TIME = real_Num( 3.0 );
    const real_Num VehicleCameraController::DEFAULT_DISTANCE_MULTIPLIER = real_Num( 0.025 );
    const real_Num VehicleCameraController::DEFAULT_ZOOM_SPEED = real_Num( 10.0 );
    const real_Num VehicleCameraController::DEFAULT_POSITION_LERP_FACTOR = real_Num( 5.0 );
    const real_Num VehicleCameraController::DEFAULT_DISTANCE_LERP_FACTOR = real_Num( 2.0 );
    const real_Num VehicleCameraController::DEFAULT_REVERSE_THRESHOLD = real_Num( 1.0 );

    // Static const string definitions
    const String VehicleCameraController::targetStr = "target";
    const String VehicleCameraController::heightStr = "height";
    const String VehicleCameraController::targetOffsetStr = "targetOffset";
    const String VehicleCameraController::distanceStr = "distance";
    const String VehicleCameraController::minDistanceStr = "minDistance";
    const String VehicleCameraController::maxDistanceStr = "maxDistance";
    const String VehicleCameraController::heightDampingStr = "heightDamping";
    const String VehicleCameraController::rotationSnapTimeStr = "rotationSnapTime";
    const String VehicleCameraController::maxRotationSnapTimeStr = "maxRotationSnapTime";
    const String VehicleCameraController::minRotationSnapTimeStr = "minRotationSnapTime";
    const String VehicleCameraController::zoomSpeedStr = "zoomSpeed";
    const String VehicleCameraController::distanceSnapTimeStr = "distanceSnapTime";
    const String VehicleCameraController::minDistanceSnapTimeStr = "minDistanceSnapTime";
    const String VehicleCameraController::maxDistanceSnapTimeStr = "maxDistanceSnapTime";
    const String VehicleCameraController::distanceMultiplierStr = "distanceMultiplier";
    const String VehicleCameraController::positionLerpFactorStr = "positionLerpFactor";
    const String VehicleCameraController::distanceLerpFactorStr = "distanceLerpFactor";
    const String VehicleCameraController::minHeightStr = "minHeight";
    const String VehicleCameraController::maxHeightStr = "maxHeight";
    const String VehicleCameraController::lookAtHeightStr = "lookAtHeight";
    const String VehicleCameraController::reverseThresholdStr = "reverseThreshold";
    const String VehicleCameraController::resourceStr = "resource";
    const String VehicleCameraController::emptyStr = "";

    VehicleCameraController::VehicleCameraController() = default;

    VehicleCameraController::~VehicleCameraController()
    {
    }

    void VehicleCameraController::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            CameraController::load( data );
            if( auto actor = getActor() )
                m_cameraTransform = actor->getWorldTransform();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void VehicleCameraController::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                m_target = nullptr;
                CameraController::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void VehicleCameraController::update()
    {
        auto task = Thread::getCurrentTask();
        switch( task )
        {
        case TaskId::Application:
        {
            auto actor = getActorPtr();
            if( !actor )
            {
                return;
            }

            if( actor->isSmoothMotion() )
            {
                return;
            }

            auto enabled = isEnabled() && actor->isEnabledInScene();
            if( enabled )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                auto gameManager = applicationManager->getGameManagerPtr();
                auto timer = applicationManager->getTimerPtr();

                auto t = timer->getTime();
                auto dt = timer->getDeltaTime();
                if( dt <= 0.0 )
                    return;

                auto inputDeviceManager = applicationManager->getInputDeviceManager();

                if( applicationManager->isPlaying() )
                {
                    if( !m_target )
                    {
                        auto vehicle = gameManager->getObjectByType<VehicleController>();
                        if( vehicle )
                        {
                            m_target = vehicle->getActor();
                        }
                    }

                    if( inputDeviceManager )
                    {
                        auto mouseScrollDelta = inputDeviceManager->getMouseScroll();
                        if( mouseScrollDelta.y > 0.0f )
                        {
                            m_distance -= m_zoomSpeed * static_cast<real_dNum>( dt );
                        }
                        else if( mouseScrollDelta.y < 0.0f )
                        {
                            m_distance += m_zoomSpeed * static_cast<real_dNum>( dt );
                        }
                    }

                    m_distance = Math<real_dNum>::clamp( m_distance, m_minDistance, m_maxDistance );

                    if( m_target )
                    {
                        auto worldTransform = m_target->getTransform();
                        if( worldTransform )
                        {
                            auto fPosition = worldTransform->getPosition();
                            auto fOrientation = worldTransform->getOrientation();

                            auto position = Vector3<real_dNum>( fPosition.x, fPosition.y, fPosition.z );
                            auto orientation = Quaternion<real_dNum>( fOrientation.w, fOrientation.x,
                                                                      fOrientation.y, fOrientation.z );

                            m_targetHeight = position.y + m_height;

                            // Fix rotation snap time calculation
                            auto fRotationSnapTime = Math<real_Num>::clamp(
                                m_maxRotationSnapTime - ( m_rotationSnapTime + m_minRotationSnapTime ),
                                m_minRotationSnapTime, m_maxRotationSnapTime );

                            // Use proper smoothDampAngle with reference parameter
                            m_currentRotationAngle = Math<real_dNum>::smoothDampAngle(
                                m_currentRotationAngle, m_targetRotationAngle, m_cameraVelocity.Y(),
                                fRotationSnapTime );

                            // Smooth height interpolation with proper damping
                            m_currentHeight = Math<real_Num>::lerp(
                                m_currentHeight, m_targetHeight,
                                Math<real_Num>::clamp01( m_heightDamping *
                                                         static_cast<real_Num>( dt ) ) );

                            // Calculate velocity using target position consistently
                            m_velocity =
                                ( position - m_lastTargetPosition ) / static_cast<real_dNum>( dt );
                            m_lastTargetPosition = position;

                            // Fix distance snap time calculation
                            auto fDistanceSnapTime = Math<real_dNum>::clamp(
                                m_maxDistanceSnapTime - ( m_distanceSnapTime + m_minDistanceSnapTime ),
                                m_minDistanceSnapTime, m_maxDistanceSnapTime );

                            // Calculate target distance with velocity-based adjustment
                            auto velocityMagnitude = m_velocity.length();
                            auto desiredDistance =
                                m_distance + ( velocityMagnitude * m_distanceMultiplier );

                            // Use smoothDampAngle for distance with proper velocity reference
                            auto smoothedDistance = Math<real_dNum>::smoothDampAngle(
                                m_targetDistance, desiredDistance, m_cameraVelocity.Z(),
                                fDistanceSnapTime );

                            // Apply smooth interpolation to target distance
                            m_targetDistance = Math<real_dNum>::lerp(
                                m_targetDistance, (real_dNum)smoothedDistance,
                                Math<real_dNum>::clamp01( static_cast<real_dNum>( dt ) *
                                                          m_distanceLerpFactor ) );

                            m_targetDistance =
                                Math<real_dNum>::clamp( m_targetDistance, m_minDistance, m_maxDistance );

                            // Calculate final camera position
                            // Ogre3D/OpenGL coordinate system: Y-up, -Z forward (right-handed)
                            // Positive distance places camera behind vehicle (+Z direction)
                            auto cameraPosition = position;
                            cameraPosition.y = m_currentHeight;
                            cameraPosition +=
                                Quaternion<real_dNum>::euler( 0, m_currentRotationAngle, 0 ) *
                                Vector3<real_dNum>( 0, 0, m_targetDistance );

                            // Handle rotation logic
                            auto targetTransform = worldTransform->getWorldTransform();

                            auto fVelocity = Vector3<real_Num>(
                                (real_Num)m_velocity.x, (real_Num)m_velocity.y, (real_Num)m_velocity.z );
                            auto velocityDir = targetTransform.inverseTransformVector( fVelocity );
                            auto targetOrientation = targetTransform.getOrientation();
                            auto targetRotation = Euler<real_Num>( targetOrientation );

                            // Vehicle moving backwards
                            // In Ogre3D/OpenGL: negative Z is forward, positive Z is backward
                            if( velocityDir.z <= m_reverseThreshold )
                            {
                                // Moving backward (positive Z in local space)
                                m_targetRotationAngle = targetRotation.yaw();
                            }
                            else
                            {
                                // Moving forward (negative Z in local space)
                                m_targetRotationAngle = -targetRotation.yaw();
                            }

                            // Apply smooth position interpolation to reduce jitter
                            auto fCurrentPosition = actor->getPosition();
                            auto currentPosition = Vector3<real_dNum>(
                                fCurrentPosition.x, fCurrentPosition.y, fCurrentPosition.z );
                            auto lerpFactor = Math<real_dNum>::clamp01(
                                static_cast<real_dNum>( dt ) * (real_dNum)m_positionLerpFactor );

                            auto finalPosition =
                                Math<real_dNum>::lerp( currentPosition, cameraPosition, lerpFactor );

                            actor->setPosition( Vector3<real_Num>( (real_Num)finalPosition.x,
                                                                   (real_Num)finalPosition.y,
                                                                   (real_Num)finalPosition.z ) );
                            actor->lookAt( Vector3<real_Num>( (real_Num)position.x, (real_Num)position.y,
                                                              (real_Num)position.z ),
                                           Vector3F::unitY() );
                        }
                    }
                }
            }
        }
        break;
        case TaskId::Render:
        {
            // Render task logic (if any) can be added here
            auto actor = getActorPtr();
            if( !actor )
            {
                return;
            }

            if( actor->isSmoothMotion() )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                auto gameManager = applicationManager->getGameManagerPtr();
                auto timer = applicationManager->getTimerPtr();

                if( !m_target )
                {
                    auto vehicle = gameManager->getObjectByType<VehicleController>();
                    if( vehicle )
                    {
                        m_target = vehicle->getActor();
                    }
                }

                if( !m_target )
                {
                    return;
                }

                auto enabled = isEnabled() && actor->isEnabledInScene();
                if( enabled )
                {
                    auto applicationManager = core::IApplicationManager::instancePtr();
                    auto gameManager = applicationManager->getGameManagerPtr();
                    auto timer = applicationManager->getTimerPtr();

                    auto t = timer->getTime();
                    auto dt = timer->getDeltaTime();
                    if( dt <= 0.0 )
                        return;

                    if( auto input = applicationManager->getInputDeviceManager() )
                    {
                        const auto scroll = input->getMouseScroll().y;
                        if( scroll > 0 )
                            m_distance -= m_zoomSpeed * static_cast<real_dNum>( dt );
                        else if( scroll < 0 )
                            m_distance += m_zoomSpeed * static_cast<real_dNum>( dt );
                        m_distance = Math<real_dNum>::clamp( m_distance, m_minDistance, m_maxDistance );
                    }

                    auto smoothDeltaTime = timer->getDeltaTime( task );
                    auto transformTime = timer->getTime( task ) - IGameManager::smoothMotionDelay;

                    if( auto transform = m_target->getTransform() )
                    {
                        auto handle = m_target->getHandle();
                        auto id = handle->getInstanceId();
                        auto transformTask = transform->getTask();

                        auto smoothTransform = transform->getWorldTransform();
                        if( gameManager->getTransformState( id, transformTime, smoothDeltaTime,
                                                            smoothTransform, transformTask ) )
                        {
                            auto fPosition = smoothTransform.getPosition();
                            auto fOrientation = smoothTransform.getOrientation();

                            auto position = Vector3<real_dNum>( fPosition.x, fPosition.y, fPosition.z );
                            auto orientation = Quaternion<real_dNum>( fOrientation.w, fOrientation.x,
                                                                      fOrientation.y, fOrientation.z );

                            m_targetHeight = position.y + m_height;

                            // Fix rotation snap time calculation
                            auto fRotationSnapTime = Math<real_Num>::clamp(
                                m_maxRotationSnapTime - ( m_rotationSnapTime + m_minRotationSnapTime ),
                                m_minRotationSnapTime, m_maxRotationSnapTime );

                            // Use proper smoothDampAngle with reference parameter
                            m_currentRotationAngle = Math<real_dNum>::smoothDampAngle(
                                m_currentRotationAngle, m_targetRotationAngle, m_cameraVelocity.Y(),
                                fRotationSnapTime );

                            // Smooth height interpolation with proper damping
                            m_currentHeight = Math<real_Num>::lerp(
                                m_currentHeight, m_targetHeight,
                                Math<real_Num>::clamp01( m_heightDamping *
                                                         static_cast<real_Num>( dt ) ) );

                            // Calculate velocity using target position consistently
                            m_velocity =
                                ( position - m_lastTargetPosition ) / static_cast<real_dNum>( dt );
                            m_lastTargetPosition = position;

                            // Fix distance snap time calculation
                            auto fDistanceSnapTime = Math<real_dNum>::clamp(
                                m_maxDistanceSnapTime - ( m_distanceSnapTime + m_minDistanceSnapTime ),
                                m_minDistanceSnapTime, m_maxDistanceSnapTime );

                            // Calculate target distance with velocity-based adjustment
                            auto velocityMagnitude = m_velocity.length();
                            auto desiredDistance =
                                m_distance + ( velocityMagnitude * m_distanceMultiplier );

                            // Use smoothDampAngle for distance with proper velocity reference
                            auto smoothedDistance = Math<real_dNum>::smoothDampAngle(
                                m_targetDistance, desiredDistance, m_cameraVelocity.Z(),
                                fDistanceSnapTime );

                            // Apply smooth interpolation to target distance
                            m_targetDistance = Math<real_dNum>::lerp(
                                m_targetDistance, (real_dNum)smoothedDistance,
                                Math<real_dNum>::clamp01( static_cast<real_dNum>( dt ) *
                                                          m_distanceLerpFactor ) );

                            m_targetDistance =
                                Math<real_dNum>::clamp( m_targetDistance, m_minDistance, m_maxDistance );

                            // Calculate final camera position
                            // Ogre3D/OpenGL coordinate system: Y-up, -Z forward (right-handed)
                            // Positive distance places camera behind vehicle (+Z direction)
                            auto cameraPosition = position;
                            cameraPosition.y = m_currentHeight;
                            cameraPosition +=
                                Quaternion<real_dNum>::euler( 0, m_currentRotationAngle, 0 ) *
                                Vector3<real_dNum>( 0, 0, m_targetDistance );

                            // Handle rotation logic
                            auto &targetTransform = smoothTransform;

                            auto fVelocity = Vector3<real_Num>(
                                (real_Num)m_velocity.x, (real_Num)m_velocity.y, (real_Num)m_velocity.z );
                            auto velocityDir = targetTransform.inverseTransformVector( fVelocity );
                            auto targetOrientation = targetTransform.getOrientation();
                            auto targetRotation = Euler<real_Num>( targetOrientation );

                            // Vehicle moving backwards
                            // In Ogre3D/OpenGL: negative Z is forward, positive Z is backward
                            if( velocityDir.z <= m_reverseThreshold )
                            {
                                // Moving backward (positive Z in local space)
                                m_targetRotationAngle = targetRotation.yaw();
                            }
                            else
                            {
                                // Moving forward (negative Z in local space)
                                m_targetRotationAngle = -targetRotation.yaw();
                            }

                            // Apply smooth position interpolation to reduce jitter
                            auto fCurrentPosition = m_cameraTransform.getPosition();
                            auto currentPosition = Vector3<real_dNum>(
                                fCurrentPosition.x, fCurrentPosition.y, fCurrentPosition.z );
                            auto lerpFactor = Math<real_dNum>::clamp01(
                                static_cast<real_dNum>( dt ) * (real_dNum)m_positionLerpFactor );

                            auto finalPosition =
                                Math<real_dNum>::lerp( currentPosition, cameraPosition, lerpFactor );

                            auto cameraRenderPosition =
                                Vector3<real_Num>( (real_Num)finalPosition.x, (real_Num)finalPosition.y,
                                                   (real_Num)finalPosition.z );

                            auto lookAtPosition = Vector3<real_Num>(
                                (real_Num)position.x, (real_Num)position.y + m_lookAtHeight,
                                (real_Num)position.z );
                            auto vec = lookAtPosition - cameraRenderPosition;
                            auto cameraRenderOrientation =
                                MathUtil<real_Num>::getOrientationFromDirection( vec );
                            auto camera = actor->getComponentPtr<Camera>();

                            m_cameraTransform =
                                Transform3F( cameraRenderPosition, cameraRenderOrientation );
                            // Keep LOD queries and the standalone sample camera on this render pose.
                            auto actorTransform = actor->getTransform();
                            actorTransform->setWorldTransform( m_cameraTransform );
                            actorTransform->setLocalDirty( true, false );
                            actorTransform->update();
                            if( camera )
                                camera->updateTransform( m_cameraTransform );
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

    auto VehicleCameraController::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto objects = CameraController::getChildObjects();
        objects.emplace_back( m_target );
        return objects;
    }

    auto VehicleCameraController::getProperties() const -> SmartPtr<Properties>
    {
        try
        {
            auto properties = CameraController::getProperties();

            if( !properties )
            {
                WP_LOG_WARNING(
                    "VehicleCameraController::getProperties: base class returned null properties." );
                return {};
            }

            if( m_target )
            {
                auto handle = m_target->getHandle();
                auto uuid = handle->getUUIDAsString();
                properties->setProperty( targetStr, uuid, resourceStr, false );
            }
            else
            {
                properties->setProperty( targetStr, emptyStr, resourceStr, false );
            }

            properties->setProperty( heightStr, m_height );
            properties->setProperty( targetOffsetStr, m_targetOffset );
            properties->setProperty( distanceStr, m_distance );
            properties->setProperty( minDistanceStr, m_minDistance );
            properties->setProperty( maxDistanceStr, m_maxDistance );
            properties->setProperty( minHeightStr, m_minHeight );
            properties->setProperty( maxHeightStr, m_maxHeight );
            properties->setProperty( lookAtHeightStr, m_lookAtHeight );
            properties->setProperty( heightDampingStr, m_heightDamping );
            properties->setProperty( rotationSnapTimeStr, m_rotationSnapTime );
            properties->setProperty( maxRotationSnapTimeStr, m_maxRotationSnapTime );
            properties->setProperty( minRotationSnapTimeStr, m_minRotationSnapTime );
            properties->setProperty( zoomSpeedStr, m_zoomSpeed );
            properties->setProperty( distanceSnapTimeStr, m_distanceSnapTime );
            properties->setProperty( minDistanceSnapTimeStr, m_minDistanceSnapTime );
            properties->setProperty( maxDistanceSnapTimeStr, m_maxDistanceSnapTime );
            properties->setProperty( distanceMultiplierStr, m_distanceMultiplier );
            properties->setProperty( positionLerpFactorStr, m_positionLerpFactor );
            properties->setProperty( distanceLerpFactorStr, m_distanceLerpFactor );
            properties->setProperty( reverseThresholdStr, m_reverseThreshold );

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    void VehicleCameraController::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            CameraController::setProperties( properties );

            if( !properties )
            {
                WP_LOG_WARNING( "VehicleCameraController::setProperties: null properties supplied." );
                return;
            }

            real_Num height = getHeight();
            if( properties->getPropertyValue( heightStr, height ) )
            {
                setHeight( height );
            }

            real_Num targetOffset = getTargetOffset();
            if( properties->getPropertyValue( targetOffsetStr, targetOffset ) )
            {
                setTargetOffset( targetOffset );
            }

            real_Num distance = getDistance();
            if( properties->getPropertyValue( distanceStr, distance ) )
            {
                setDistance( distance );
            }

            real_Num minDistance = getMinDistance();
            if( properties->getPropertyValue( minDistanceStr, minDistance ) )
            {
                setMinDistance( minDistance );
            }

            real_Num maxDistance = getMaxDistance();
            if( properties->getPropertyValue( maxDistanceStr, maxDistance ) )
            {
                setMaxDistance( maxDistance );
            }

            real_Num minHeight = getMinHeight();
            if( properties->getPropertyValue( minHeightStr, minHeight ) )
            {
                setMinHeight( minHeight );
            }

            real_Num maxHeight = getMaxHeight();
            if( properties->getPropertyValue( maxHeightStr, maxHeight ) )
            {
                setMaxHeight( maxHeight );
            }

            real_Num lookAtHeight = getLookAtHeight();
            if( properties->getPropertyValue( lookAtHeightStr, lookAtHeight ) )
            {
                setLookAtHeight( lookAtHeight );
            }

            real_Num heightDamping = getHeightDamping();
            if( properties->getPropertyValue( heightDampingStr, heightDamping ) )
            {
                setHeightDamping( heightDamping );
            }

            real_Num rotationSnapTime = getRotationSnapTime();
            if( properties->getPropertyValue( rotationSnapTimeStr, rotationSnapTime ) )
            {
                setRotationSnapTime( rotationSnapTime );
            }

            real_Num minRotationSnapTime = getMinRotationSnapTime();
            if( properties->getPropertyValue( minRotationSnapTimeStr, minRotationSnapTime ) )
            {
                setMinRotationSnapTime( minRotationSnapTime );
            }

            real_Num maxRotationSnapTime = getMaxRotationSnapTime();
            if( properties->getPropertyValue( maxRotationSnapTimeStr, maxRotationSnapTime ) )
            {
                setMaxRotationSnapTime( maxRotationSnapTime );
            }

            real_Num zoomSpeed = getZoomSpeed();
            if( properties->getPropertyValue( zoomSpeedStr, zoomSpeed ) )
            {
                setZoomSpeed( zoomSpeed );
            }

            real_Num distanceSnapTime = getDistanceSnapTime();
            if( properties->getPropertyValue( distanceSnapTimeStr, distanceSnapTime ) )
            {
                setDistanceSnapTime( distanceSnapTime );
            }

            real_Num minDistanceSnapTime = getMinDistanceSnapTime();
            if( properties->getPropertyValue( minDistanceSnapTimeStr, minDistanceSnapTime ) )
            {
                setMinDistanceSnapTime( minDistanceSnapTime );
            }

            real_Num maxDistanceSnapTime = getMaxDistanceSnapTime();
            if( properties->getPropertyValue( maxDistanceSnapTimeStr, maxDistanceSnapTime ) )
            {
                setMaxDistanceSnapTime( maxDistanceSnapTime );
            }

            real_Num distanceMultiplier = getDistanceMultiplier();
            if( properties->getPropertyValue( distanceMultiplierStr, distanceMultiplier ) )
            {
                setDistanceMultiplier( distanceMultiplier );
            }

            real_Num positionLerpFactor = getPositionLerpFactor();
            if( properties->getPropertyValue( positionLerpFactorStr, positionLerpFactor ) )
            {
                setPositionLerpFactor( positionLerpFactor );
            }

            real_Num distanceLerpFactor = getDistanceLerpFactor();
            if( properties->getPropertyValue( distanceLerpFactorStr, distanceLerpFactor ) )
            {
                setDistanceLerpFactor( distanceLerpFactor );
            }

            real_Num reverseThreshold = getReverseThreshold();
            if( properties->getPropertyValue( reverseThresholdStr, reverseThreshold ) )
            {
                setReverseThreshold( reverseThreshold );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto VehicleCameraController::getTarget() const -> SmartPtr<IGameActor>
    {
        return m_target;
    }

    void VehicleCameraController::setTarget( SmartPtr<IGameActor> target )
    {
        m_target = target;
    }

    auto VehicleCameraController::getDistance() const -> real_Num
    {
        return (real_Num)m_distance;
    }

    void VehicleCameraController::setDistance( real_Num distance )
    {
        m_distance = (real_dNum)Math<real_Num>::clamp( distance, m_minDistance, m_maxDistance );
    }

    auto VehicleCameraController::getMinDistance() const -> real_Num
    {
        return m_minDistance;
    }

    void VehicleCameraController::setMinDistance( real_Num minDistance )
    {
        if( minDistance < real_Num( 0.0 ) )
        {
            WP_LOG_WARNING( "VehicleCameraController::setMinDistance: value must be >= 0; ignoring." );
            return;
        }
        if( minDistance > m_maxDistance )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setMinDistance: value exceeds maxDistance; clamping." );
            minDistance = m_maxDistance;
        }
        m_minDistance = minDistance;
        m_distance = Math<real_Num>::clamp( (real_Num)m_distance, m_minDistance, m_maxDistance );
    }

    auto VehicleCameraController::getMaxDistance() const -> real_Num
    {
        return m_maxDistance;
    }

    void VehicleCameraController::setMaxDistance( real_Num maxDistance )
    {
        if( maxDistance < m_minDistance )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setMaxDistance: value is less than minDistance; clamping." );
            maxDistance = m_minDistance;
        }
        m_maxDistance = maxDistance;
        m_distance = Math<real_Num>::clamp( (real_Num)m_distance, m_minDistance, m_maxDistance );
    }

    auto VehicleCameraController::getDistanceSnapTime() const -> real_Num
    {
        return m_distanceSnapTime;
    }

    void VehicleCameraController::setDistanceSnapTime( real_Num distanceSnapTime )
    {
        if( distanceSnapTime < real_Num( 0.0 ) )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setDistanceSnapTime: value must be >= 0; ignoring." );
            return;
        }
        m_distanceSnapTime = distanceSnapTime;
    }

    auto VehicleCameraController::getMinDistanceSnapTime() const -> real_Num
    {
        return m_minDistanceSnapTime;
    }

    void VehicleCameraController::setMinDistanceSnapTime( real_Num minDistanceSnapTime )
    {
        if( minDistanceSnapTime < real_Num( 0.0 ) )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setMinDistanceSnapTime: value must be >= 0; ignoring." );
            return;
        }
        if( minDistanceSnapTime > m_maxDistanceSnapTime )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setMinDistanceSnapTime: value exceeds maxDistanceSnapTime; "
                "clamping." );
            minDistanceSnapTime = m_maxDistanceSnapTime;
        }
        m_minDistanceSnapTime = minDistanceSnapTime;
    }

    auto VehicleCameraController::getMaxDistanceSnapTime() const -> real_Num
    {
        return m_maxDistanceSnapTime;
    }

    void VehicleCameraController::setMaxDistanceSnapTime( real_Num maxDistanceSnapTime )
    {
        if( maxDistanceSnapTime < m_minDistanceSnapTime )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setMaxDistanceSnapTime: value is less than "
                "minDistanceSnapTime; clamping." );
            maxDistanceSnapTime = m_minDistanceSnapTime;
        }
        m_maxDistanceSnapTime = maxDistanceSnapTime;
    }

    auto VehicleCameraController::getDistanceMultiplier() const -> real_Num
    {
        return m_distanceMultiplier;
    }

    void VehicleCameraController::setDistanceMultiplier( real_Num distanceMultiplier )
    {
        if( distanceMultiplier < real_Num( 0.0 ) )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setDistanceMultiplier: value must be >= 0; ignoring." );
            return;
        }
        m_distanceMultiplier = distanceMultiplier;
    }

    auto VehicleCameraController::getPositionLerpFactor() const -> real_Num
    {
        return m_positionLerpFactor;
    }

    void VehicleCameraController::setPositionLerpFactor( real_Num positionLerpFactor )
    {
        if( positionLerpFactor < real_Num( 0.0 ) )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setPositionLerpFactor: value must be >= 0; ignoring." );
            return;
        }
        m_positionLerpFactor = positionLerpFactor;
    }

    auto VehicleCameraController::getDistanceLerpFactor() const -> real_Num
    {
        return m_distanceLerpFactor;
    }

    void VehicleCameraController::setDistanceLerpFactor( real_Num distanceLerpFactor )
    {
        if( distanceLerpFactor < real_Num( 0.0 ) )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setDistanceLerpFactor: value must be >= 0; ignoring." );
            return;
        }
        m_distanceLerpFactor = distanceLerpFactor;
    }

    auto VehicleCameraController::getHeight() const -> real_Num
    {
        return (real_Num)m_height;
    }

    void VehicleCameraController::setHeight( real_Num height )
    {
        m_height = (real_dNum)Math<real_Num>::clamp( height, m_minHeight, m_maxHeight );
    }

    auto VehicleCameraController::getTargetOffset() const -> real_Num
    {
        return (real_Num)m_targetOffset;
    }

    void VehicleCameraController::setTargetOffset( real_Num offset )
    {
        m_targetOffset = (real_dNum)offset;
    }

    auto VehicleCameraController::getHeightDamping() const -> real_Num
    {
        return m_heightDamping;
    }

    void VehicleCameraController::setHeightDamping( real_Num damping )
    {
        if( damping < real_Num( 0.0 ) )
        {
            WP_LOG_WARNING( "VehicleCameraController::setHeightDamping: value must be >= 0; ignoring." );
            return;
        }
        m_heightDamping = damping;
    }

    auto VehicleCameraController::getMinHeight() const -> real_Num
    {
        return m_minHeight;
    }

    void VehicleCameraController::setMinHeight( real_Num minHeight )
    {
        if( minHeight > m_maxHeight )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setMinHeight: value exceeds maxHeight; clamping." );
            minHeight = m_maxHeight;
        }
        m_minHeight = minHeight;
        m_height = Math<real_Num>::clamp( (real_Num)m_height, m_minHeight, m_maxHeight );
    }

    auto VehicleCameraController::getMaxHeight() const -> real_Num
    {
        return m_maxHeight;
    }

    void VehicleCameraController::setMaxHeight( real_Num maxHeight )
    {
        if( maxHeight < m_minHeight )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setMaxHeight: value is less than minHeight; clamping." );
            maxHeight = m_minHeight;
        }

        m_maxHeight = maxHeight;
        m_height = Math<real_Num>::clamp( (real_Num)m_height, m_minHeight, m_maxHeight );
    }

    auto VehicleCameraController::getLookAtHeight() const -> real_Num
    {
        return m_lookAtHeight;
    }

    void VehicleCameraController::setLookAtHeight( real_Num height )
    {
        m_lookAtHeight = height;
    }

    auto VehicleCameraController::getRotationSnapTime() const -> real_Num
    {
        return m_rotationSnapTime;
    }

    void VehicleCameraController::setRotationSnapTime( real_Num snapTime )
    {
        if( snapTime < real_Num( 0.0 ) )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setRotationSnapTime: value must be >= 0; ignoring." );
            return;
        }
        m_rotationSnapTime = snapTime;
    }

    auto VehicleCameraController::getMinRotationSnapTime() const -> real_Num
    {
        return m_minRotationSnapTime;
    }

    void VehicleCameraController::setMinRotationSnapTime( real_Num minSnapTime )
    {
        if( minSnapTime < real_Num( 0.0 ) )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setMinRotationSnapTime: value must be >= 0; ignoring." );
            return;
        }
        if( minSnapTime > m_maxRotationSnapTime )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setMinRotationSnapTime: value exceeds maxRotationSnapTime; "
                "clamping." );
            minSnapTime = m_maxRotationSnapTime;
        }
        m_minRotationSnapTime = minSnapTime;
    }

    auto VehicleCameraController::getMaxRotationSnapTime() const -> real_Num
    {
        return m_maxRotationSnapTime;
    }

    void VehicleCameraController::setMaxRotationSnapTime( real_Num maxSnapTime )
    {
        if( maxSnapTime < m_minRotationSnapTime )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setMaxRotationSnapTime: value is less than "
                "minRotationSnapTime; clamping." );
            maxSnapTime = m_minRotationSnapTime;
        }
        m_maxRotationSnapTime = maxSnapTime;
    }

    auto VehicleCameraController::getZoomSpeed() const -> real_Num
    {
        return m_zoomSpeed;
    }

    void VehicleCameraController::setZoomSpeed( real_Num speed )
    {
        if( speed < real_Num( 0.0 ) )
        {
            WP_LOG_WARNING( "VehicleCameraController::setZoomSpeed: value must be >= 0; ignoring." );
            return;
        }
        m_zoomSpeed = speed;
    }

    auto VehicleCameraController::getReverseThreshold() const -> real_Num
    {
        return m_reverseThreshold;
    }

    void VehicleCameraController::setReverseThreshold( real_Num threshold )
    {
        if( threshold <= real_Num( 0.0 ) )
        {
            WP_LOG_WARNING(
                "VehicleCameraController::setReverseThreshold: value must be > 0; ignoring." );
            return;
        }
        m_reverseThreshold = threshold;
    }

}  // namespace workphone::scene
