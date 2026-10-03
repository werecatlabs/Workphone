#ifndef _FPSCAMERACONTROLLER_H
#define _FPSCAMERACONTROLLER_H

#include <Workphone/Scene/Components/Camera/CameraController.hpp>
#include <Workphone/Math/Ray3.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief First-person-style camera controller.
         *
         * This controller implements typical FPS camera controls: free translation
         * and rotation based on mouse and keyboard input. It manages one or more
         * cameras and exposes methods to query and modify the camera transform
         * and controller properties such as movement and rotation speeds.
         */
        class WPCore_API FpsCameraController : public CameraController
        {
        public:
            // ---------------------------------------------------------------
            // Property key strings
            // ---------------------------------------------------------------
            static const String positionStr;          ///< Camera world position
            static const String targetPositionStr;    ///< Camera look-at position
            static const String targetDirectionStr;   ///< Camera forward direction
            static const String moveSpeedStr;         ///< Linear movement speed
            static const String translationSpeedStr;  ///< Translation speed multiplier
            static const String rotationSpeedStr;     ///< Mouse-look rotation speed
            static const String mouseSensitivityStr;  ///< Per-pixel mouse sensitivity
            static const String maxPitchAngleStr;     ///< Pitch clamp limit (degrees)
            static const String wheelSpeedFactorStr;  ///< Scroll-wheel move-speed adjustment factor
            static const String minMoveSpeedStr;      ///< Minimum move speed (wheel clamp)
            static const String maxMoveSpeedStr;      ///< Maximum move speed (wheel clamp)
            static const String yawStr;               ///< Current yaw angle (radians)
            static const String pitchStr;             ///< Current pitch angle (radians)

            /**
             * @brief Maps an action to one or two keycodes.
             *
             * Used to bind controller actions (move forward, jump, etc.) to
             * keyboard keys. Two keycodes are supported to allow alternative
             * bindings for the same action.
             */
            struct SCamKeyMap
            {
                /** Default constructor; initializes members to zero. */
                SCamKeyMap();

                /**
                 * @brief Construct a key mapping.
                 * @param a Action id
                 * @param k Primary keycode
                 * @param k1 Secondary keycode (optional)
                 */
                SCamKeyMap( s32 a, s32 k, s32 k1 );

                s32 action;    //!< Action identifier
                s32 keycode;   //!< Primary keycode bound to the action
                s32 keycode1;  //!< Secondary keycode bound to the action
            };

            /** Default constructor. */
            FpsCameraController();

            /** Virtual destructor. */
            ~FpsCameraController() override;

            /**
             * @copydoc Component::load
             *
             * Loads optional initialization data for the controller (e.g. key
             * mappings, initial position/orientation) from a shared object.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::unload
             *
             * Releases resources and removes input bindings when the
             * controller is unloaded.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Update controller state.
             *
             * Called each frame to update camera transforms based on current
             * input state, velocities and internal interpolation.
             */
            void update() override;

            /**
             * @brief Handle an input event.
             * @param event Input event to process
             * @return True if the event was consumed by the controller
             */
            bool handleInputEvent( const SmartPtr<IInputEvent> &event );

            /**
             * @brief Set the world-space camera position.
             * @param position New position for the camera
             */
            void setPosition( const Vector3<real_Num> &position );

            /**
             * @brief Get the current world-space camera position.
             * @return Camera position vector
             */
            Vector3<real_Num> getPosition() const;

            /**
             * @brief Set the world-space position the camera should look at.
             * @param position Target position the camera will orient toward
             */
            void setTargetPosition( const Vector3<real_Num> &position );

            /**
             * @brief Get the target position the camera is looking at.
             * @return Target position vector
             */
            Vector3<real_Num> getTargetPosition() const;

            /**
             * @brief Set the camera orientation.
             * @param orientation New orientation quaternion
             */
            void setOrientation( const Quaternion<real_Num> &orientation );

            /**
             * @brief Get the camera orientation.
             * @return Orientation quaternion
             */
            Quaternion<real_Num> getOrientation() const;

            /**
             * @brief Get the forward direction vector for the camera.
             * @return Normalized forward direction
             */
            Vector3<real_Num> getDirection() const;

            /**
             * @brief Create a world-space ray from the camera through a
             * viewport/screen position.
             * @param screenPosition Screen coordinates (usually in pixels or
             * normalized device coordinates depending on convention)
             * @return Ray in world space starting at the camera and passing
             * through the provided screen position
             */
            Ray3F getCameraToViewportRay( const Vector2<real_Num> &screenPosition ) const;

            /**
             * @brief Test whether an axis-aligned bounding box intersects the
             * camera frustum.
             * @param box Axis-aligned bounding box to test
             * @return True if the box is (partially or fully) inside the
             * camera frustum
             */
            bool isInFrustum( const AABB3F &box ) const;

            /**
             * @brief Set a named property value on the controller.
             * @param name Property name
             * @param value Property value (string representation)
             */
            void setPropertyValue( const String &name, const String &value );

            /**
             * @brief Apply multiple properties from a Properties object.
             * @param properties Properties container with values to apply
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Retrieve the current properties describing this controller.
             * @return Properties object describing controller state and bindings
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Add a camera instance to be managed by this controller.
             * @param camera Camera to add
             */
            void addCamera( SmartPtr<render::IGraphicsCamera> camera );

            /**
             * @brief Remove a managed camera from the controller.
             * @param camera Camera to remove
             * @return True if the camera was successfully removed
             */
            bool removeCamera( SmartPtr<render::IGraphicsCamera> camera );

            /**
             * @brief Set the linear movement speed used for translations.
             * @param speed Movement speed in world units per second
             */
            void setMoveSpeed( real_Num speed );

            /**
             * @brief Get the configured movement speed.
             * @return Movement speed in world units per second
             */
            real_Num getMoveSpeed() const;

            /**
             * @brief Set the angular rotation speed used for mouse look.
             * @param speed Rotation speed (units depend on implementation)
             */
            void setRotationSpeed( real_Num speed );

            /**
             * @brief Get the configured rotation speed.
             * @return Rotation speed
             */
            real_Num getRotationSpeed() const;

            /**
             * @brief Get the translation speed multiplier.
             * @return Translation speed multiplier
             */
            real_Num getTranslationSpeed() const;

            /**
             * @brief Set the translation speed multiplier.
             * @param speed Translation speed multiplier
             */
            void setTranslationSpeed( real_Num speed );

            /**
             * @brief Get the per-pixel mouse sensitivity used for rotation.
             * @return Mouse sensitivity scalar
             */
            real_Num getMouseSensitivity() const;

            /**
             * @brief Set the per-pixel mouse sensitivity.
             * @param sensitivity Mouse sensitivity scalar
             */
            void setMouseSensitivity( real_Num sensitivity );

            /**
             * @brief Get the maximum pitch angle (in degrees).
             * @return Max pitch angle
             */
            real_Num getMaxPitchAngle() const;

            /**
             * @brief Set the maximum pitch clamp angle in degrees.
             * @param degrees Angle in degrees; clamped to [1, 89.9]
             */
            void setMaxPitchAngle( real_Num degrees );

            /**
             * @brief Get the scroll-wheel move-speed adjustment factor.
             * @return Wheel speed factor
             */
            real_Num getWheelSpeedFactor() const;

            /**
             * @brief Set the scroll-wheel move-speed adjustment factor.
             * @param factor Wheel speed factor
             */
            void setWheelSpeedFactor( real_Num factor );

            /**
             * @brief Get the minimum allowed move speed (wheel clamp lower bound).
             * @return Minimum move speed
             */
            real_Num getMinMoveSpeed() const;

            /**
             * @brief Set the minimum allowed move speed.
             * @param speed Minimum move speed
             */
            void setMinMoveSpeed( real_Num speed );

            /**
             * @brief Get the maximum allowed move speed (wheel clamp upper bound).
             * @return Maximum move speed
             */
            real_Num getMaxMoveSpeed() const;

            /**
             * @brief Set the maximum allowed move speed.
             * @param speed Maximum move speed
             */
            void setMaxMoveSpeed( real_Num speed );

            /**
             * @brief Get the current yaw angle in radians.
             * @return Yaw angle
             */
            real_Num getYaw() const;

            /**
             * @brief Set the yaw angle in radians and recompute target direction.
             * @param yaw Yaw angle in radians
             */
            void setYaw( real_Num yaw );

            /**
             * @brief Get the current pitch angle in radians.
             * @return Pitch angle
             */
            real_Num getPitch() const;

            /**
             * @brief Set the pitch angle in radians and recompute target direction.
             * @param pitch Pitch angle in radians; clamped to [-maxPitch, maxPitch]
             */
            void setPitch( real_Num pitch );

            WP_CLASS_REGISTER_DECL;

        private:
            /**
             * @brief Query whether a mouse button is currently pressed.
             * @param key Mouse button code to check
             * @return True if the button is pressed
             */
            bool isMouseKeyDown( s32 key );

            /**
             * @brief Reset all internal key state arrays to the unpressed state.
             */
            void allKeysUp();

            /**
             * @brief Recompute the controller's target direction vector from
             * the current yaw and pitch angles.
             */
            void updateTargetDirection();

            // Managed cameras; controller may control multiple cameras but
            // only one is selected at a time for active updates.
            Array<SmartPtr<render::IGraphicsCamera>> m_cameras;
            SmartPtr<render::IGraphicsCamera> m_selectedCamera;  //!< Currently active camera

            Vector3<real_Num> m_targetVector;  //!< Computed movement/target vector
            Vector2<real_Num> m_prevCursor;    //!< Previous cursor position (for delta calculations)
            Vector2<real_Num> m_cursorPos;     //!< Current logical cursor position
            Vector2<real_Num> m_mousePos;      //!< Raw mouse position

            Vector3<real_Num> m_position;  //!< Camera world position
            mutable Vector3<real_Num>
                m_targetPosition;  //!< Target/look-at position (mutable for const accessors)
            Vector3<real_Num> m_targetDirection;  //!< Forward direction the camera should face

            mutable Quaternion<real_Num>
                m_orientation;  //!< Camera orientation quaternion (mutable for const accessors)

            f32 m_moveSpeed = 1.0f;           //!< Linear movement speed
            f32 m_rotationSpeed = 10.01f;     //!< Rotation speed for mouse look
            f32 m_translationSpeed = 100.0f;  //!< Additional translation speed multiplier

            // Mouse-look tuning
            real_Num m_mouseSensitivity = 0.01f;  //!< Per-pixel sensitivity applied to mouse delta
            real_Num m_maxPitchAngle = 89.0f;     //!< Maximum pitch in degrees (prevents gimbal flip)
            real_Num m_wheelSpeedFactor = 0.1f;   //!< Scroll-wheel speed-change factor
            real_Num m_minMoveSpeed = 0.1f;       //!< Lower bound for wheel-adjusted move speed
            real_Num m_maxMoveSpeed = 10.0f;      //!< Upper bound for wheel-adjusted move speed

            // Euler angles used to compute orientation
            real_Num m_yaw = 0.0f;    //!< Yaw angle (rotation around vertical axis)
            real_Num m_pitch = 0.0f;  //!< Pitch angle (rotation around horizontal axis)

            // Mouse capture state indicates whether the controller has
            // exclusive mouse control (cursor hidden/locked)
            bool m_isMouseCaptured = false;

            Array<bool> m_mouseKeys;   //!< Current pressed state for mouse buttons
            Array<bool> m_cursorKeys;  //!< Current pressed state for cursor keys (if used)

            Array<SCamKeyMap> m_keyMap;  //!< Key mapping table for actions
        };

    }  // namespace scene
}  // namespace workphone

#endif
