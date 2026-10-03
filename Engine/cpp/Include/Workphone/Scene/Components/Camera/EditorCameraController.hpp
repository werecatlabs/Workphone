#ifndef __EditorCameraController_H
#define __EditorCameraController_H

#include <Workphone/Scene/Components/Camera/CameraController.hpp>
#include <Workphone/Math/Ray3.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Camera interaction modes used by the editor camera controller.
         *
         * Mirrors common "Maya-style" editor interactions:
         * - Orbit: Alt + LMB (rotate around a pivot)
         * - Pan:   Alt + MMB (translate parallel to view plane)
         * - Dolly: Alt + RMB (move toward/away from pivot)
         * - FreeLook: RMB without Alt (first-person style look)
         */
        enum class CameraControlMode
        {
            None,     ///< No active control
            Orbit,    ///< Alt + LMB: Rotate around target point
            Pan,      ///< Alt + MMB: Pan camera parallel to view plane
            Dolly,    ///< Alt + RMB: Move camera toward/away from target
            FreeLook  ///< RMB without Alt: Free look rotation
        };

        /**
         * @brief Editor camera controller component.
         *
         * This component implements a full-featured editor camera that supports
         * both orbit-style and first-person interactions. Typical features:
         * - Orbit/Tumble around a pivot point
         * - Pan/Track parallel to the view plane
         * - Dolly (zoom) toward/away from pivot
         * - Mouse-wheel zoom
         * - WASD movement when no modifier keys are pressed
         * - Frame/Focus on a target using the F key
         *
         * The controller manages one or more cameras (render::IGraphicsCamera) and updates
         * their transforms according to user input and configured speed settings.
         */
        class WPCore_API EditorCameraController : public CameraController
        {
        public:
            // ---------------------------------------------------------------
            // Property key strings (used in getProperties / setProperties)
            // ---------------------------------------------------------------
            static const String positionStr;              ///< Camera world position
            static const String targetPositionStr;        ///< Orbit pivot world position
            static const String moveSpeedStr;             ///< Keyboard move speed
            static const String rotationSpeedStr;         ///< Mouse rotation sensitivity
            static const String translationSpeedStr;      ///< General translation multiplier
            static const String panSpeedStr;              ///< Pan speed multiplier
            static const String dollySpeedStr;            ///< Dolly/zoom speed multiplier
            static const String orbitDistanceStr;         ///< Current orbit distance
            static const String minOrbitDistanceStr;      ///< Minimum orbit distance
            static const String maxOrbitDistanceStr;      ///< Maximum orbit distance
            static const String invertStr;                ///< Invert vertical mouse axis
            static const String shiftSpeedMultiplierStr;  ///< Shift key speed boost factor
            static const String orbitSensitivityStr;      ///< Alt+LMB orbit mouse sensitivity
            static const String panSensitivityStr;        ///< Alt+MMB pan mouse sensitivity
            static const String dollySensitivityStr;      ///< Alt+RMB dolly mouse sensitivity
            static const String freeLookSensitivityStr;   ///< RMB free-look mouse sensitivity
            static const String wheelSensitivityStr;      ///< Scroll-wheel zoom sensitivity
            static const String maxPitchAngleStr;         ///< Maximum pitch angle (degrees)

            /**
             * @brief Key mapping entry used to map actions to key codes.
             *
             * Each entry maps an action identifier to a primary and optional
             * secondary key binding.
             */
            struct SCamKeyMap
            {
                SCamKeyMap();
                SCamKeyMap( s32 actionType, s32 primaryKey, s32 secondaryKey );

                s32 action;    ///< Action identifier (e.g. move forward/back)
                s32 keycode;   ///< Primary key code for the action
                s32 keycode1;  ///< Secondary/alternative key code
            };

            /**
             * @brief Event listener adapter that forwards input events to the controller.
             *
             * This nested listener implements the IEventListener interface and holds
             * a weak/strong reference to the owning EditorCameraController instance.
             * It extracts relevant parameters from events and calls controller methods.
             */
            class EditorCameraInputListener : public IEventListener
            {
            public:
                EditorCameraInputListener();
                ~EditorCameraInputListener() override;

                /**
                 * @brief Handle an incoming event and forward it to the owner controller.
                 *
                 * @param eventType Type of the incoming event.
                 * @param eventValue Event-specific hashed value.
                 * @param arguments Event parameters.
                 * @param sender Event sender (may be null).
                 * @param object Event object (may be null).
                 * @param event Original event object (may be null).
                 * @return Parameter result according to IEventListener contract.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /**
                 * @brief Get the owner controller instance.
                 * @return SmartPtr to the owning EditorCameraController.
                 */
                SmartPtr<EditorCameraController> getOwner() const;

                /**
                 * @brief Set the owner controller instance for this listener.
                 * @param owner SmartPtr to the owning EditorCameraController.
                 */
                void setOwner( SmartPtr<EditorCameraController> owner );

            protected:
                SmartPtr<EditorCameraController> m_owner;  ///< Controller that receives forwarded events
            };

            EditorCameraController();
            ~EditorCameraController() override;

            /** @copydoc Component::load
             *
             * @note Expects optional serialized properties in @p data to initialize
             *       camera speeds, inversion, and key mappings.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::update
             *
             * Processes pending input, updates movement/rotation and applies the
             * resulting transform to managed cameras.
             */
            void update() override;

            /**
             * @brief Handle a raw input event for camera control.
             * @param event Input event to process.
             * @return True if the event was consumed by the camera controller.
             */
            bool handleInputEvent( const SmartPtr<IInputEvent> &event );

            /**
             * @brief Set the camera world-space position.
             * @param position New camera position in world space.
             */
            void setPosition( const Vector3<real_Num> &position );

            /**
             * @brief Get the current camera world-space position.
             * @return Camera position in world space.
             */
            Vector3<real_Num> getPosition() const;

            /**
             * @brief Set the orbit target/pivot position.
             * @param position World-space pivot used for orbit and focus operations.
             */
            void setTargetPosition( const Vector3<real_Num> &position );

            /**
             * @brief Get the current orbit target/pivot position.
             * @return World-space pivot position.
             */
            Vector3<real_Num> getTargetPosition() const;

            /**
             * @brief Set the camera orientation using a quaternion.
             * @param orientation New orientation quaternion.
             */
            void setOrientation( const Quaternion<real_Num> &orientation );

            /**
             * @brief Get the camera orientation quaternion.
             * @return Current orientation quaternion.
             */
            Quaternion<real_Num> getOrientation() const;

            /**
             * @brief Set the forward direction of the camera.
             * @param direction Unit or non-unit vector indicating forward direction.
             *
             * @note The controller will typically normalize and use this direction
             *       to derive orientation/up/right vectors when updating transforms.
             */
            void setDirection( const Vector3<real_Num> &direction );

            /**
             * @brief Get the current forward direction vector.
             * @return Camera forward (direction) vector in world space.
             */
            Vector3<real_Num> getDirection() const;

            /**
             * @brief Get the camera up vector.
             * @return Up vector derived from current orientation.
             */
            Vector3<real_Num> getUp() const;

            /**
             * @brief Get the camera right vector.
             * @return Right vector derived from current orientation.
             */
            Vector3<real_Num> getRight() const;

            /**
             * @brief Add a render camera to be controlled by this component.
             * @param camera Camera instance to register and update.
             *
             * Multiple cameras may be managed; transforms are applied to all added cameras.
             */
            void addCamera( SmartPtr<render::IGraphicsCamera> camera );

            /**
             * @brief Remove a previously added camera.
             * @param camera Camera instance to remove.
             * @return True if the camera was found and removed; false otherwise.
             */
            bool removeCamera( SmartPtr<render::IGraphicsCamera> camera );

            /**
             * @brief Get the mouse rotation sensitivity multiplier.
             * @return Rotation speed scalar.
             */
            f32 getRotationSpeed() const;

            /**
             * @brief Set the mouse rotation sensitivity multiplier.
             * @param rotationSpeed Scalar multiplier applied to mouse delta when rotating.
             */
            void setRotationSpeed( f32 rotationSpeed );

            /**
             * @brief Get whether the vertical mouse axis is inverted.
             * @return True if Y axis is inverted; false otherwise.
             */
            bool getInvert() const;

            /**
             * @brief Set whether to invert the vertical mouse axis for rotation.
             * @param inverted True to invert Y, false for normal behavior.
             */
            void setInvert( bool inverted );

            /**
             * @brief Get translation speed used for keyboard movement (WASD).
             * @return Movement speed in world units per second.
             */
            f32 getMoveSpeed() const;

            /**
             * @brief Set translation speed for keyboard movement.
             * @param speed Movement speed in world units per second.
             */
            void setMoveSpeed( f32 speed );

            /**
             * @brief Get general translation speed used for non-orbit translations.
             * @return Translation speed scalar.
             */
            f32 getTranslationSpeed() const;

            /**
             * @brief Set general translation speed multiplier.
             * @param speed Translation speed scalar.
             */
            void setTranslationSpeed( f32 speed );

            /**
             * @brief Get the current orbit distance (distance from camera to pivot).
             * @return Orbit distance in world units.
             */
            f32 getOrbitDistance() const;

            /**
             * @brief Set the desired orbit distance from the pivot.
             * @param distance Orbit distance in world units.
             *
             * The value will typically be clamped to [m_minOrbitDistance, m_maxOrbitDistance].
             */
            void setOrbitDistance( f32 distance );

            /**
             * @brief Get the pan speed multiplier.
             * @return Pan speed scalar.
             */
            f32 getPanSpeed() const;

            /**
             * @brief Set the pan speed multiplier.
             * @param panSpeed Scalar multiplier applied to panning.
             */
            void setPanSpeed( f32 panSpeed );

            /**
             * @brief Get the dolly (zoom) speed multiplier.
             * @return Dolly speed scalar.
             */
            f32 getDollySpeed() const;

            /**
             * @brief Set the dolly (zoom) speed multiplier.
             * @param dollySpeed Scalar multiplier applied to dolly/zoom operations.
             */
            void setDollySpeed( f32 dollySpeed );

            /**
             * @brief Move camera to frame the current target pivot and adjust orientation.
             *
             * Computes an appropriate camera position and orientation so that the target
             * pivot is framed in view, using current orbit distance / camera parameters.
             */
            void focusOnTarget();

            /**
             * @brief Move camera to frame a specific world position and optionally set distance.
             * @param targetPosition World-space position to focus on.
             * @param distance Optional distance from the camera to the target; if 0, distance is
             * auto-computed.
             */
            void focusOnPosition( const Vector3<real_Num> &targetPosition, f32 distance = 0.0f );

            /**
             * @brief Get the active camera control mode (orbit/pan/dolly/freelook/none).
             * @return Current CameraControlMode.
             */
            CameraControlMode getControlMode() const;

            /** @copydoc Component::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Component::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            // ---------------------------------------------------------------
            // Orbit distance limits
            // ---------------------------------------------------------------
            /** Get the minimum allowed orbit distance. */
            f32 getMinOrbitDistance() const;

            /** Set the minimum allowed orbit distance. */
            void setMinOrbitDistance( f32 distance );

            /** Get the maximum allowed orbit distance. */
            f32 getMaxOrbitDistance() const;

            /** Set the maximum allowed orbit distance. */
            void setMaxOrbitDistance( f32 distance );

            // ---------------------------------------------------------------
            // Mouse sensitivity accessors
            // ---------------------------------------------------------------
            /** Get the Shift-key speed boost multiplier. */
            f32 getShiftSpeedMultiplier() const;

            /** Set the Shift-key speed boost multiplier. */
            void setShiftSpeedMultiplier( f32 multiplier );

            /** Get the Alt+LMB orbit mouse sensitivity. */
            f32 getOrbitSensitivity() const;

            /** Set the Alt+LMB orbit mouse sensitivity. */
            void setOrbitSensitivity( f32 sensitivity );

            /** Get the Alt+MMB pan mouse sensitivity. */
            f32 getPanSensitivity() const;

            /** Set the Alt+MMB pan mouse sensitivity. */
            void setPanSensitivity( f32 sensitivity );

            /** Get the Alt+RMB dolly mouse sensitivity. */
            f32 getDollySensitivity() const;

            /** Set the Alt+RMB dolly mouse sensitivity. */
            void setDollySensitivity( f32 sensitivity );

            /** Get the RMB free-look mouse sensitivity. */
            f32 getFreeLookSensitivity() const;

            /** Set the RMB free-look mouse sensitivity. */
            void setFreeLookSensitivity( f32 sensitivity );

            /** Get the scroll-wheel zoom sensitivity. */
            f32 getWheelSensitivity() const;

            /** Set the scroll-wheel zoom sensitivity. */
            void setWheelSensitivity( f32 sensitivity );

            /** Get the maximum pitch angle in degrees. */
            f32 getMaxPitchAngle() const;

            /** Set the maximum pitch angle in degrees. */
            void setMaxPitchAngle( f32 degrees );

            WP_CLASS_REGISTER_DECL;

        private:
            /** Initialize default key mappings (WASD, arrows, focus, etc.). */
            void setupDefaultKeyMappings();

            /**
             * @brief Process mouse state for rotation, pan and orbit interactions.
             * @param mouseState Current mouse state snapshot (buttons, position, wheel).
             */
            void handleMouseInput( SmartPtr<IMouseState> mouseState );

            /**
             * @brief Process keyboard state for camera translation and shortcuts.
             * @param keyboardState Current keyboard state snapshot.
             */
            void handleKeyboardInput( SmartPtr<IKeyboardState> keyboardState );

            /**
             * @brief Update camera translational movement based on accumulated input.
             * @param deltaTime Elapsed time since last update (seconds).
             */
            void updateMovement( f32 deltaTime );

            /**
             * @brief Update camera rotational movement based on accumulated input.
             * @param deltaTime Elapsed time since last update (seconds).
             */
            void updateRotation( f32 deltaTime );

            /** Recompute and apply camera transform(s) from internal position/rotation state. */
            void updateCameraTransform();

            /**
             * @brief Rotate the camera around the pivot (orbit).
             * @param deltaYaw Horizontal rotation delta (radians).
             * @param deltaPitch Vertical rotation delta (radians).
             */
            void performOrbit( f32 deltaYaw, f32 deltaPitch );

            /**
             * @brief Translate the camera/pivot parallel to the view plane (panning).
             * @param deltaX Horizontal pan amount (screen units or normalized).
             * @param deltaY Vertical pan amount (screen units or normalized).
             */
            void performPan( f32 deltaX, f32 deltaY );

            /**
             * @brief Move the camera closer to or further from the pivot (dolly/zoom).
             * @param deltaDistance Positive values move the camera toward the target.
             */
            void performDolly( f32 deltaDistance );

            /** Recalculate the orbit distance from the current camera and target positions. */
            void updateOrbitDistance();

            /** Recalculate camera world position from orbit parameters (angles + distance). */
            void updatePositionFromOrbit();

            /**
             * @brief Query whether a mouse button is currently pressed according to internal state.
             * @param buttonIndex Index of the mouse button to query.
             * @return True if the requested mouse button is down.
             */
            bool isMouseKeyDown( s32 buttonIndex );

            /**
             * @brief Query whether the Alt modifier is currently pressed.
             * @return True if Alt is pressed.
             */
            bool isAltKeyDown() const;

            /**
             * @brief Query whether the Shift modifier is currently pressed.
             * @return True if Shift is pressed.
             */
            bool isShiftKeyDown() const;

            /**
             * @brief Query whether the Control modifier is currently pressed.
             * @return True if Control is pressed.
             */
            bool isCtrlKeyDown() const;

            /** Reset all tracked key/button states to the "up" state. */
            void allKeysUp();

            ///< Input event listener adapter
            SmartPtr<IEventListener> m_inputListener;

            ///< Registered cameras controlled by this component
            Array<SmartPtr<render::IGraphicsCamera>> m_cameras;

            ///< Cached vector pointing from camera toward target pivot
            Vector3<real_Num> m_targetVector;

            Vector2<real_Num> m_prevCursorPos;  ///< Cursor position from previous frame (screen-space)
            Vector2<real_Num> m_cursorPos;      ///< Current cursor position (screen-space)
            Vector2<real_Num> m_mousePos;       ///< Mouse absolute position
            Vector2<real_Num> m_relativeMouse;  ///< Mouse movement delta since last sample

            Vector3<real_Num> m_position;        ///< Camera world-space position
            Vector3<real_Num> m_targetPosition;  ///< Orbit pivot / focus target world position
            Vector3<real_Num> m_rotation;        ///< Camera rotation (pitch, yaw, roll) in radians

            f32 m_moveSpeed;         ///< Movement speed for keyboard controls (units/sec)
            f32 m_rotationSpeed;     ///< Mouse look sensitivity (multiplier applied to mouse delta)
            f32 m_translationSpeed;  ///< Generic translation speed multiplier
            f32 m_orbitDistance;     ///< Current distance from camera to orbit target (world units)
            f32 m_panSpeed;          ///< Pan speed multiplier (screen-to-world conversion factor)
            f32 m_dollySpeed;        ///< Dolly/zoom speed multiplier
            f32 m_minOrbitDistance = 0.1f;     ///< Minimum allowed orbit distance (prevents clipping)
            f32 m_maxOrbitDistance = 1000.0f;  ///< Maximum allowed orbit distance

            // Mouse sensitivity and speed multipliers (all data-driven)
            f32 m_shiftSpeedMultiplier = 3.0f;  ///< Speed boost applied when Shift is held
            f32 m_orbitSensitivity = 0.005f;    ///< Alt+LMB orbit per-pixel sensitivity
            f32 m_panSensitivity = 0.002f;      ///< Alt+MMB pan per-pixel sensitivity
            f32 m_dollySensitivity = 0.005f;    ///< Alt+RMB dolly per-pixel sensitivity
            f32 m_freeLookSensitivity = 0.01f;  ///< RMB free-look per-pixel sensitivity
            f32 m_wheelSensitivity = 0.1f;      ///< Scroll-wheel zoom sensitivity
            f32 m_maxPitchAngle = 89.0f;        ///< Pitch clamp limit in degrees

            bool m_bInvert = false;  ///< Invert vertical mouse axis when true

            CameraControlMode m_controlMode;  ///< Active input/control mode

            Array<bool> m_mouseKeys;   ///< Per-button mouse pressed state
            Array<bool> m_cursorKeys;  ///< Keyboard key pressed state cache

            Array<SCamKeyMap> m_keyMap;  ///< Action-to-key mappings used for input handling

            Properties m_propertyGroup;  ///< Serializable property group for editor persistence
        };

    }  // namespace scene
}  // namespace workphone

#endif
