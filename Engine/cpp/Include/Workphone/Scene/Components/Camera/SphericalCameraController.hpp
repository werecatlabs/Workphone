#ifndef SphericalCamera_h__
#define SphericalCamera_h__

#include <Workphone/Scene/Components/Camera/CameraController.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Ray3.hpp>

namespace workphone
{
    namespace scene
    {

        /** A spherical camera controller class.
         */
        class WPCore_API SphericalCameraController : public CameraController
        {
        public:
            // Default value constants
            static const f32 DEFAULT_FAR_DISTANCE;
            static const f32 DEFAULT_NEAR_DISTANCE;
            static const f32 DEFAULT_ZOOM_SPEED;
            static const f32 DEFAULT_MOVE_SPEED;
            static const f32 DEFAULT_MAX_DELTA_TIME;
            static const f32 DEFAULT_PAN_FACTOR;
            static const f32 DEFAULT_KEY_ZOOM_FORCE;

            static const Vector3<real_Num> DEFAULT_SPHERICAL_COORDS;

            // Property key strings
            static const String positionStr;
            static const String targetStr;
            static const String sphericalCoordsStr;
            static const String rotationSpeedStr;
            static const String zoomSpeedStr;
            static const String moveSpeedStr;
            static const String translationSpeedStr;
            static const String maxDistanceStr;
            static const String nearDistanceStr;
            static const String maxDeltaTimeStr;
            static const String panFactorStr;
            static const String keyZoomForceStr;

            /** Constructor. */
            SphericalCameraController();

            /** Destructor. */
            ~SphericalCameraController() override;

            /** @copydoc CameraController::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc CameraController::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc CameraController::update */
            void update() override;

            bool handleEvent( const SmartPtr<IInputEvent> &event );

            Vector3<real_Num> getSphericalCoords() const;
            void setSphericalCoords( const Vector3<real_Num> &spherical );

            void setPosition( const Vector3<real_Num> &position );
            Vector3<real_Num> getPosition() const;

            void setTargetPosition( const Vector3<real_Num> &position );
            Vector3<real_Num> getTargetPosition() const;

            void setOrientation( const Quaternion<real_Num> &orientation );
            Quaternion<real_Num> getOrientation() const;

            void setDirection( const Vector3<real_Num> &direction );
            Vector3<real_Num> getDirection() const;

            Vector3<real_Num> getUp() const;
            Vector3<real_Num> getRight() const;

            Ray3<real_Num> getCameraToViewportRay( const Vector2<real_Num> &screenPosition ) const;

            void setProperties( SmartPtr<Properties> properties ) override;
            SmartPtr<Properties> getProperties() const override;

            f32 getRotationSpeed() const;
            void setRotationSpeed( f32 rotationSpeed );

            f32 getMaxDistance() const;
            void setMaxDistance( f32 maxDistance );

            f32 getNearDistance() const;
            void setNearDistance( f32 nearDistance );

            f32 getZoomSpeed() const;
            void setZoomSpeed( f32 zoomSpeed );

            f32 getMoveSpeed() const;
            void setMoveSpeed( f32 moveSpeed );

            f32 getTranslationSpeed() const;
            void setTranslationSpeed( f32 translationSpeed );

            /** Get the maximum capped delta-time per frame (seconds). */
            real_Num getMaxDeltaTime() const;

            /** Set the maximum capped delta-time per frame (seconds). */
            void setMaxDeltaTime( real_Num maxDt );

            /** Get the panning speed factor applied to middle-mouse translation. */
            f32 getPanFactor() const;

            /** Set the panning speed factor applied to middle-mouse translation. */
            void setPanFactor( f32 factor );

            /** Get the force magnitude applied by W/S keyboard zoom keys. */
            f32 getKeyZoomForce() const;

            /** Set the force magnitude applied by W/S keyboard zoom keys. */
            void setKeyZoomForce( f32 force );

            void focusSelection() override;

            bool isViewDirty() const;

            void setViewDirty( bool dirty );

            WP_CLASS_REGISTER_DECL;

        private:
            bool isMouseBtnDown( s32 key );
            void allKeysUp();

            Vector3<real_Num> getCartCoords( Vector3<real_Num> &spherical );
            Vector3<real_Num> getSphericalCoords( Vector3<real_Num> &cartCoords );
            void wrapSphericalCoords( Vector3<real_Num> &spherical );

            Vector3<real_Num> m_targetVector;
            Vector2<real_Num> m_prevCursor;
            Vector2<real_Num> m_cursorPos;
            Vector2<real_Num> m_mousePos;
            Vector2<real_Num> m_relativeMouse;

            Vector3<real_Num> m_target;
            Vector3<real_Num> m_spherical;
            Vector3<real_Num> m_sphericalForce;
            Vector3<real_Num> m_position;
            mutable Vector3<real_Num> m_targetPosition;
            Vector3<real_Num> m_rotation;

            real_Num m_rotationSpeed = real_Num( 0.0 );

            /// The camera far distance.
            real_Num m_maxDistance = real_Num( 0.0 );

            /// The camera near distance.
            real_Num m_nearDistance = real_Num( 0.0 );

            /// The zoom speed.
            real_Num m_zoomSpeed = real_Num( 0.0 );

            /// The move speed.
            real_Num m_moveSpeed = real_Num( 0.0 );

            real_Num m_translationSpeed = real_Num( 0.0 );

            /// Maximum frame delta-time cap (seconds). Prevents large jumps on slow frames.
            real_Num m_maxDeltaTime = real_Num( 1.0 / 30.0 );

            /// Speed factor applied to middle-mouse panning translations.
            f32 m_panFactor = 30.0f;

            /// Force magnitude applied when W/S keyboard keys zoom the camera.
            f32 m_keyZoomForce = 5.0f;

            Array<bool> m_mouseKeys;
            Array<bool> m_cursorKeys;

            bool m_isViewDirty = false;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // SphericalCamera_h__
