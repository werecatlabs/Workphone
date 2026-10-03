#ifndef VehicleCameraController_h__
#define VehicleCameraController_h__

#include <Workphone/Scene/Components/Camera/CameraController.hpp>

namespace workphone
{
    namespace scene
    {

        /** Class for a vehicle camera controller.
         */
        class WPCore_API VehicleCameraController : public CameraController
        {
        public:
            // Default value constants
            static const real_Num DEFAULT_DISTANCE;
            static const real_Num DEFAULT_MIN_DISTANCE;
            static const real_Num DEFAULT_MAX_DISTANCE;
            static const real_Num DEFAULT_TARGET_OFFSET;
            static const real_Num DEFAULT_HEIGHT;
            static const real_Num DEFAULT_MIN_HEIGHT;
            static const real_Num DEFAULT_MAX_HEIGHT;
            static const real_Num DEFAULT_HEIGHT_DAMPING;
            static const real_Num DEFAULT_LOOK_AT_HEIGHT;
            static const real_Num DEFAULT_ROTATION_SNAP_TIME;
            static const real_Num DEFAULT_MIN_ROTATION_SNAP_TIME;
            static const real_Num DEFAULT_MAX_ROTATION_SNAP_TIME;
            static const real_Num DEFAULT_DISTANCE_SNAP_TIME;
            static const real_Num DEFAULT_MIN_DISTANCE_SNAP_TIME;
            static const real_Num DEFAULT_MAX_DISTANCE_SNAP_TIME;
            static const real_Num DEFAULT_DISTANCE_MULTIPLIER;
            static const real_Num DEFAULT_ZOOM_SPEED;
            static const real_Num DEFAULT_POSITION_LERP_FACTOR;
            static const real_Num DEFAULT_DISTANCE_LERP_FACTOR;
            static const real_Num DEFAULT_REVERSE_THRESHOLD;

            // Static const string constants for property names
            static const String targetStr;
            static const String heightStr;
            static const String targetOffsetStr;
            static const String distanceStr;
            static const String minDistanceStr;
            static const String maxDistanceStr;
            static const String heightDampingStr;
            static const String rotationSnapTimeStr;
            static const String maxRotationSnapTimeStr;
            static const String minRotationSnapTimeStr;
            static const String zoomSpeedStr;
            static const String distanceSnapTimeStr;
            static const String minDistanceSnapTimeStr;
            static const String maxDistanceSnapTimeStr;
            static const String distanceMultiplierStr;
            static const String positionLerpFactorStr;
            static const String distanceLerpFactorStr;
            static const String minHeightStr;
            static const String maxHeightStr;
            static const String lookAtHeightStr;
            static const String reverseThresholdStr;
            static const String resourceStr;
            static const String emptyStr;

            /** Constructor. */
            VehicleCameraController();

            /** Destructor. */
            ~VehicleCameraController() override;

            /** @copydoc CameraController::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc CameraController::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc CameraController::update */
            void update() override;

            /** @copydoc IComponent::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc IComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** Gets the camera target.
             * @return The camera target.
             */
            SmartPtr<IGameActor> getTarget() const;

            /** Sets the camera target.
             * @param target The camera target.
             */
            void setTarget( SmartPtr<IGameActor> target );

            /** Gets the distance.
             * @return The distance.
             */
            real_Num getDistance() const;

            /** Sets the distance.
             * @param distance The distance.
             */
            void setDistance( real_Num distance );

            /** Gets the minimum distance.
             * @return The minimum distance.
             */
            real_Num getMinDistance() const;

            /** Sets the minimum distance.
             * @param minDistance The minimum distance.
             */
            void setMinDistance( real_Num minDistance );

            /** Gets the maximum distance.
             * @return The maximum distance.
             */
            real_Num getMaxDistance() const;

            /** Sets the maximum distance.
             * @param maxDistance The maximum distance.
             */
            void setMaxDistance( real_Num maxDistance );

            /** Gets the distance snap time.
             * @return The distance snap time.
             */
            real_Num getDistanceSnapTime() const;

            /** Sets the distance snap time.
             * @param distanceSnapTime The distance snap time.
             */
            void setDistanceSnapTime( real_Num distanceSnapTime );

            /** Gets the minimum distance snap time.
             * @return The minimum distance snap time.
             */
            real_Num getMinDistanceSnapTime() const;

            /** Sets the minimum distance snap time.
             * @param minDistanceSnapTime The minimum distance snap time.
             */
            void setMinDistanceSnapTime( real_Num minDistanceSnapTime );

            /** Gets the maximum distance snap time.
             * @return The maximum distance snap time.
             */
            real_Num getMaxDistanceSnapTime() const;

            /** Sets the maximum distance snap time.
             * @param maxDistanceSnapTime The maximum distance snap time.
             */
            void setMaxDistanceSnapTime( real_Num maxDistanceSnapTime );

            /** Gets the distance multiplier.
             * @return The distance multiplier.
             */
            real_Num getDistanceMultiplier() const;

            /** Sets the distance multiplier.
             * @param distanceMultiplier The distance multiplier.
             */
            void setDistanceMultiplier( real_Num distanceMultiplier );

            /** Gets the position lerp factor.
             * @return The position lerp factor.
             */
            real_Num getPositionLerpFactor() const;

            /** Sets the position lerp factor.
             * @param positionLerpFactor The position lerp factor.
             */
            void setPositionLerpFactor( real_Num positionLerpFactor );

            /** Gets the distance lerp factor.
             * @return The distance lerp factor.
             */
            real_Num getDistanceLerpFactor() const;

            /** Sets the distance lerp factor.
             * @param distanceLerpFactor The distance lerp factor.
             */
            void setDistanceLerpFactor( real_Num distanceLerpFactor );

            /** Gets the camera height above the target.
             * @return The height.
             */
            real_Num getHeight() const;

            /** Sets the camera height above the target.
             * @param height The height.
             */
            void setHeight( real_Num height );

            /** Gets the target offset.
             * @return The target offset.
             */
            real_Num getTargetOffset() const;

            /** Sets the target offset.
             * @param offset The target offset.
             */
            void setTargetOffset( real_Num offset );

            /** Gets the height damping factor.
             * @return The height damping.
             */
            real_Num getHeightDamping() const;

            /** Sets the height damping factor.
             * @param damping The height damping (must be >= 0).
             */
            void setHeightDamping( real_Num damping );

            /** Gets the minimum camera height.
             * @return The minimum height.
             */
            real_Num getMinHeight() const;

            /** Sets the minimum camera height.
             * @param minHeight The minimum height.
             */
            void setMinHeight( real_Num minHeight );

            /** Gets the maximum camera height.
             * @return The maximum height.
             */
            real_Num getMaxHeight() const;

            /** Sets the maximum camera height.
             * @param maxHeight The maximum height (must be >= minHeight).
             */
            void setMaxHeight( real_Num maxHeight );

            /** Gets the look-at height offset.
             * @return The look-at height.
             */
            real_Num getLookAtHeight() const;

            /** Sets the look-at height offset.
             * @param height The look-at height.
             */
            void setLookAtHeight( real_Num height );

            /** Gets the rotation snap time.
             * @return The rotation snap time.
             */
            real_Num getRotationSnapTime() const;

            /** Sets the rotation snap time.
             * @param snapTime The rotation snap time (must be >= 0).
             */
            void setRotationSnapTime( real_Num snapTime );

            /** Gets the minimum rotation snap time.
             * @return The minimum rotation snap time.
             */
            real_Num getMinRotationSnapTime() const;

            /** Sets the minimum rotation snap time.
             * @param minSnapTime The minimum rotation snap time.
             */
            void setMinRotationSnapTime( real_Num minSnapTime );

            /** Gets the maximum rotation snap time.
             * @return The maximum rotation snap time.
             */
            real_Num getMaxRotationSnapTime() const;

            /** Sets the maximum rotation snap time.
             * @param maxSnapTime The maximum rotation snap time (must be >= minRotationSnapTime).
             */
            void setMaxRotationSnapTime( real_Num maxSnapTime );

            /** Gets the zoom speed.
             * @return The zoom speed.
             */
            real_Num getZoomSpeed() const;

            /** Sets the zoom speed.
             * @param speed The zoom speed (must be >= 0).
             */
            void setZoomSpeed( real_Num speed );

            /** Gets the velocity magnitude threshold for reverse detection.
             * @return The reverse threshold.
             */
            real_Num getReverseThreshold() const;

            /** Sets the velocity magnitude threshold for reverse detection.
             * @param threshold The reverse threshold (must be > 0).
             */
            void setReverseThreshold( real_Num threshold );

            WP_CLASS_REGISTER_DECL;

        protected:
            // The camera target.
            SmartPtr<IGameActor> m_target;

            // Zoom distance.
            real_dNum m_distance = DEFAULT_DISTANCE;

            // Camera distance.
            real_dNum m_targetDistance = real_dNum( 0.0 );

            real_Num m_minDistance = DEFAULT_MIN_DISTANCE;
            real_Num m_maxDistance = DEFAULT_MAX_DISTANCE;

            real_dNum m_targetOffset = DEFAULT_TARGET_OFFSET;
            real_dNum m_height = DEFAULT_HEIGHT;

            real_Num m_minHeight = DEFAULT_MIN_HEIGHT;
            real_Num m_maxHeight = DEFAULT_MAX_HEIGHT;

            real_Num m_heightDamping = DEFAULT_HEIGHT_DAMPING;

            real_Num m_lookAtHeight = DEFAULT_LOOK_AT_HEIGHT;

            /// Time taken to snap back to original rotation
            real_Num m_rotationSnapTime = DEFAULT_ROTATION_SNAP_TIME;

            real_Num m_minRotationSnapTime = DEFAULT_MIN_ROTATION_SNAP_TIME;
            real_Num m_maxRotationSnapTime = DEFAULT_MAX_ROTATION_SNAP_TIME;

            /// Time taken to snap back to the original distance
            real_Num m_distanceSnapTime = DEFAULT_DISTANCE_SNAP_TIME;

            real_Num m_minDistanceSnapTime = DEFAULT_MIN_DISTANCE_SNAP_TIME;
            real_Num m_maxDistanceSnapTime = DEFAULT_MAX_DISTANCE_SNAP_TIME;

            /// Rate at which speed zoom occurs.
            real_Num m_distanceMultiplier = DEFAULT_DISTANCE_MULTIPLIER;

            real_dNum m_targetRotationAngle = real_Num( 0.0 );
            real_dNum m_targetHeight = real_Num( 1.0 );

            real_dNum m_currentRotationAngle = real_Num( 45.0 );
            real_dNum m_currentHeight = real_Num( 1.0 );

            real_Num m_zoomSpeed = DEFAULT_ZOOM_SPEED;

            /// Lerp factor for smooth position interpolation
            real_Num m_positionLerpFactor = DEFAULT_POSITION_LERP_FACTOR;

            /// Lerp factor for distance interpolation
            real_Num m_distanceLerpFactor = DEFAULT_DISTANCE_LERP_FACTOR;

            /// Velocity z-component threshold for reverse detection
            real_Num m_reverseThreshold = DEFAULT_REVERSE_THRESHOLD;

            Vector3<real_dNum> m_velocity;
            Vector3<real_dNum> m_cameraVelocity;
            Vector3<real_dNum> m_lastTargetPosition;
            Transform3F m_cameraTransform;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // VehicleCameraController_h__
