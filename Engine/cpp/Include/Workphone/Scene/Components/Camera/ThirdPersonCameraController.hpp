#ifndef ThirdPersonCameraControllerl_h__
#define ThirdPersonCameraControllerl_h__

#include <Workphone/Scene/Components/Camera/CameraController.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace scene
    {
        /** Class for a third person camera controller.
         */
        class WPCore_API ThirdPersonCameraController : public CameraController
        {
        public:
            // Default value constants
            static const real_Num DEFAULT_DISTANCE;
            static const real_Num DEFAULT_HEIGHT;
            static const real_Num DEFAULT_HEIGHT_OFFSET;
            static const real_Num DEFAULT_ROTATION_SPEED;
            static const real_Num DEFAULT_ZOOM_SPEED;
            static const real_Num DEFAULT_MIN_DISTANCE;
            static const real_Num DEFAULT_MAX_DISTANCE;
            static const real_Num DEFAULT_MIN_HEIGHT;
            static const real_Num DEFAULT_MAX_HEIGHT;
            static const real_Num DEFAULT_DAMPING;
            static const int DEFAULT_ORBIT_MOUSE_BUTTON;

            // Static string constants for properties
            static const String targetStr;
            static const String distanceStr;
            static const String heightStr;
            static const String heightOffsetStr;
            static const String rotationSpeedStr;
            static const String zoomSpeedStr;
            static const String minDistanceStr;
            static const String maxDistanceStr;
            static const String minHeightStr;
            static const String maxHeightStr;
            static const String dampingStr;
            static const String lookAtOffsetStr;
            static const String orbitMouseButtonStr;
            static const String resourceStr;
            static const String emptyStr;

            /** Constructor. */
            ThirdPersonCameraController();

            /** Destructor. */
            ~ThirdPersonCameraController() override;

            /** @copydoc CameraController::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc CameraController::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::update */
            void update() override;

            /** @copydoc CameraController::postUpdate */
            void postUpdate() override;

            /** @copydoc IComponent::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc IComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** Get the camera target.
             * @return The target actor.
             */
            SmartPtr<IGameActor> getTarget() const;

            /** Set the camera target.
             * @param target The target actor.
             */
            void setTarget( SmartPtr<IGameActor> target );

            /** Get the distance from target.
             * @return The distance.
             */
            real_Num getDistance() const;

            /** Set the distance from target.
             * @param distance The distance.
             */
            void setDistance( real_Num distance );

            /** Get the height above target.
             * @return The height.
             */
            real_Num getHeight() const;

            /** Set the height above target.
             * @param height The height.
             */
            void setHeight( real_Num height );

            /** Get the rotation speed.
             * @return The rotation speed.
             */
            real_Num getRotationSpeed() const;

            /** Set the rotation speed.
             * @param speed The rotation speed.
             */
            void setRotationSpeed( real_Num speed );

            /** Get the zoom speed.
             * @return The zoom speed.
             */
            real_Num getZoomSpeed() const;

            /** Set the zoom speed.
             * @param speed The zoom speed.
             */
            void setZoomSpeed( real_Num speed );

            /** Get the damping factor.
             * @return The damping factor.
             */
            real_Num getDamping() const;

            /** Set the damping factor.
             * @param damping The damping factor.
             */
            void setDamping( real_Num damping );

            /** Get the look-at offset.
             * @return The look-at offset.
             */
            Vector3<real_Num> getLookAtOffset() const;

            /** Set the look-at offset.
             * @param offset The look-at offset.
             */
            void setLookAtOffset( const Vector3<real_Num> &offset );

            /** Get the additional height offset.
             * @return The height offset.
             */
            real_Num getHeightOffset() const;

            /** Set the additional height offset.
             * @param offset The height offset.
             */
            void setHeightOffset( real_Num offset );

            /** Get the minimum camera distance.
             * @return The minimum distance.
             */
            real_Num getMinDistance() const;

            /** Set the minimum camera distance.
             * @param minDistance The minimum distance (must be > 0).
             */
            void setMinDistance( real_Num minDistance );

            /** Get the maximum camera distance.
             * @return The maximum distance.
             */
            real_Num getMaxDistance() const;

            /** Set the maximum camera distance.
             * @param maxDistance The maximum distance (must be >= minDistance).
             */
            void setMaxDistance( real_Num maxDistance );

            /** Get the minimum camera height.
             * @return The minimum height.
             */
            real_Num getMinHeight() const;

            /** Set the minimum camera height.
             * @param minHeight The minimum height.
             */
            void setMinHeight( real_Num minHeight );

            /** Get the maximum camera height.
             * @return The maximum height.
             */
            real_Num getMaxHeight() const;

            /** Set the maximum camera height.
             * @param maxHeight The maximum height (must be >= minHeight).
             */
            void setMaxHeight( real_Num maxHeight );

            /** Get the mouse button index used for orbit rotation.
             * @return The mouse button index (0=left, 1=right, 2=middle).
             */
            int getOrbitMouseButton() const;

            /** Set the mouse button index used for orbit rotation.
             * @param button The mouse button index (0=left, 1=right, 2=middle).
             */
            void setOrbitMouseButton( int button );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Handle input for camera control.
             * @param inputManager The input device manager.
             * @param dt Delta time.
             */
            void handleInput( SmartPtr<IInputDeviceManager> inputManager, real_Num dt );

            /** Update camera transform based on target.
             * @param dt Delta time.
             */
            void updateCameraTransform( real_Num dt );

            // The camera target
            SmartPtr<IGameActor> target;

            // Camera parameters
            real_Num m_distance;       // Distance from target
            real_Num m_height;         // Height above target
            real_Num m_heightOffset;   // Additional height offset
            real_Num m_rotationSpeed;  // Mouse rotation sensitivity
            real_Num m_zoomSpeed;      // Mouse scroll zoom speed
            real_Num m_minDistance;    // Minimum zoom distance
            real_Num m_maxDistance;    // Maximum zoom distance
            real_Num m_minHeight;      // Minimum height
            real_Num m_maxHeight;      // Maximum height
            real_Num m_damping;        // Smoothing factor for camera movement

            Vector3<real_Num> m_lookAtOffset;  // Offset for look-at target

            int m_orbitMouseButton = DEFAULT_ORBIT_MOUSE_BUTTON;  // Mouse button for orbit rotation

            // Current interpolated values
            real_Num m_currentRotation;  // Current horizontal rotation
            real_Num m_currentHeight;    // Current interpolated height
            real_Num m_currentDistance;  // Current interpolated distance
        };
    }  // namespace scene
}  // namespace workphone

#endif  // ThirdPersonCameraController_h__
