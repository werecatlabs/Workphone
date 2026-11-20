#ifndef InputDeviceManager_h__
#define InputDeviceManager_h__

#include <Workphone/Interface/Input/IInputDeviceManager.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/Input/IInputPlatformBackend.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindowListener.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/HashMap.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

/**
 * @file InputDeviceManager.hpp
 * @brief Manages input devices, their states, and dispatching input events.
 */

namespace workphone
{
    class Win32InputBackend;

    /**
     * @class InputDeviceManager
     * @brief Default IInputDeviceManager implementation: window-driven mouse/keyboard/touch input,
     *        joystick polling, game-input mapping and event dispatch.
     *
     * The manager owns the current input event and mouse/keyboard state snapshots, the registered
     * game inputs, the joystick list and the event listeners. OS input arrives through an internal
     * window listener (see WindowListener) attached to the graphics window set with setWindow().
     * Each received message is converted into an IInputEvent and published with postEvent().
     *
     * Events are published to one queue per TaskId; update() drains only the queue of the task it
     * is called from, so update() must be called on every task that should receive events.
     * Joystick polling (XInput, falling back to the Windows multimedia API) runs from update()
     * on the task configured with setTask().
     *
     * @note Window message handling, joystick polling, cursor handling and the asynchronous key and
     *       mouse button queries are implemented only for Win32; on other platforms they are no-ops
     *       (the queries return false).
     */
    class WPCore_API InputDeviceManager : public IInputDeviceManager
    {
        friend class Win32InputBackend;

    public:
        /**
         * @brief Construct a new InputDeviceManager instance.
         *
         * Device creation flags default to enabled; no window is attached until setWindow().
         */
        InputDeviceManager();

        /**
         * @brief Virtual destructor.
         *
         * Detaches the internal window listener from the window (if still alive) and clears the
         * listener's back-pointer to this manager.
         */
        ~InputDeviceManager() override;

        /**
         * @brief Get the input settings as a property set.
         *
         * Keys: @c input.system, @c input.gamepad, @c input.touch, @c input.mouseCapture,
         * @c input.rebinds, @c input.gestures and @c input.haptics.
         *
         * @return New Properties object holding the current values.
         */
        SmartPtr<Properties> getProperties() const override;

        /**
         * @brief Apply input settings and propagate them to the platform.
         *
         * Reads the keys listed in getProperties(); missing keys keep their current value. Updates
         * the rebinding flag of every registered game input and, on Win32, the attached window's
         * touch registration, gesture configuration and mouse capture. When haptics are disabled, or
         * @c input.vibration.left / @c input.vibration.right are present, XInput vibration is applied
         * (device from @c input.vibration.device, all devices are stopped when haptics are disabled).
         *
         * @param properties Properties to apply; a null pointer is ignored.
         */
        void setProperties( SmartPtr<Properties> properties ) override;

        /**
         * @brief Initialize the manager and bind it to the graphics window.
         *
         * Applies the player settings of the application manager (if available), creates the
         * current input event and mouse/keyboard snapshots, resets cached mouse state, sizes the
         * per-task event queues and, on Win32, (re)attaches a fresh window listener to the current
         * window and resets the joystick list so it is rediscovered by update(). Safe to call
         * repeatedly. Exceptions are logged and not propagated.
         *
         * @param data Unused.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Reinitialize platform bindings without discarding client registrations.
         *
         * Equivalent to load(). Registered listeners and game-input mappings are kept; joystick
         * slots are cleared and recreated by update().
         *
         * @param data Forwarded to load().
         */
        void reload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Release resources acquired during load() and registrations of clients.
         *
         * Clears listeners, game inputs and joysticks, detaches and releases the window listener and
         * drops the current event and state snapshots. The window pointer is kept so a later load()
         * can reattach without another setWindow() call. Exceptions are logged and not propagated.
         *
         * @param data Unused.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Poll devices and dispatch queued events of the calling task.
         *
         * Does nothing while getDebuggerSuspended() is true. On Win32, when called from the task
         * returned by getTask() and joysticks are enabled, discovers (at most once per second) and
         * polls joysticks and posts joystick button, axis and POV events. Then pops every event
         * from the queue of the current task and delivers it to a snapshot of the listeners.
         */
        void update() override;

        /**
         * @brief Hold or release gameplay input while the debugger pumps native windows and Editor UI.
         *
         * Every transition discards all task queues, the current event and the transient snapshots,
         * scroll, drag and modifier state, and releases mouse capture when suspending. Events
         * received while held are rejected rather than replayed on resume. Call on the owner task
         * at a debugger pause boundary, preserving and restoring the prior flag. Direct window
         * messages (including ImGui and close) remain independent.
         *
         * @param suspended True to hold gameplay input, false to resume it.
         */
        void setDebuggerSuspended( bool suspended );

        /**
         * @brief Query whether gameplay input is held by the debugger.
         *
         * @return true while input is suspended, see setDebuggerSuspended().
         */
        bool getDebuggerSuspended() const;

        /**
         * @brief Immediately dispatch an input event to all listeners on the calling thread.
         *
         * Bypasses the queues. The event is dropped when it is null, input is suspended, or it is
         * a joystick event while joysticks are disabled.
         *
         * @param inputEvent Event to dispatch.
         */
        void triggerEvent( SmartPtr<IInputEvent> inputEvent ) override;

        /**
         * @brief Queue an input event for later processing by update().
         *
         * Forwards to postEvent(); the result is discarded.
         *
         * @param event Event to enqueue.
         */
        void queueEvent( SmartPtr<IInputEvent> event ) override;

        /**
         * @brief Post an event to the per-task event queues.
         *
         * The event is rejected (false) when it is null, input is suspended, it is a joystick
         * event while joysticks are disabled, it is a touch or gesture user event whose feature is
         * disabled, or the queues have not been created yet (before load()). Unless the project
         * input system is 2, the first game input whose map binds the event's key or joystick
         * button gets the event's game-input state and id assigned. On success the event is
         * appended to the queue of every task.
         *
         * @param event Event to post.
         * @return true if the event was queued, false if it was rejected.
         */
        bool postEvent( SmartPtr<IInputEvent> event ) override;

        /**
         * @brief Create a new, empty input event instance.
         *
         * @return New InputEvent.
         */
        SmartPtr<IInputEvent> createInputEvent() override;

        /**
         * @brief Create a new, empty mouse state snapshot object.
         *
         * @return New MouseState.
         */
        SmartPtr<IMouseState> createMouseState() override;

        /**
         * @brief Create a new, empty keyboard state snapshot object.
         *
         * @return New KeyboardState.
         */
        SmartPtr<IKeyboardState> createKeyboardState() override;

        /**
         * @brief Get the most recent mouse or keyboard input event produced from window messages.
         *
         * @return Last event, or null before load(), after unload() or after a debugger suspension.
         */
        SmartPtr<IInputEvent> getCurrentInputEvent() const override;

        /**
         * @brief Get the most recent mouse state snapshot.
         *
         * @return Last mouse state, or null before load() or after unload().
         */
        SmartPtr<IMouseState> getCurrentMouseState() const override;

        /**
         * @brief Get the most recent keyboard state snapshot.
         *
         * Safe to call from any thread; the read is serialized with snapshot replacement.
         *
         * @return Last keyboard state, or null before load() or after unload().
         */
        SmartPtr<IKeyboardState> getCurrentKeyboardState() const override;

        /**
         * @brief Create a GameInput identified by the provided hash id.
         *
         * An existing game input with the same id is replaced by the new one.
         *
         * @param id Unique identifier for the game input.
         * @return The newly created game input, or null if creation failed.
         */
        SmartPtr<IGameInput> addGameInput( hash_type id ) override;

        /**
         * @brief Find a previously added GameInput by id.
         *
         * @param id Identifier to search for.
         * @return The game input, or null if none is registered with that id.
         */
        SmartPtr<IGameInput> findGameInput( hash_type id ) const override;

        /**
         * @brief Get a snapshot array of all registered game inputs.
         *
         * @return Copy of the registered game inputs, in unspecified order.
         */
        Array<SmartPtr<IGameInput>> getGameInputs() const override;

        /**
         * @brief Query whether the system cursor is visible.
         *
         * @return true if visible (the manager's own flag, initially true).
         */
        bool isCursorVisible() const override;

        /**
         * @brief Show or hide the system cursor.
         *
         * On Win32 the OS cursor display counter is changed (ShowCursor) only when the visibility
         * actually changes.
         *
         * @param visible True to show, false to hide.
         */
        void setCursorVisible( bool visible ) override;

        /**
         * @brief Register an event listener to receive input events.
         *
         * Listeners are invoked from update() (queued events) and triggerEvent() (immediate events)
         * with EventType::Input and IEvent::inputEvent.
         *
         * @param listener Listener to add.
         */
        void addListener( SmartPtr<IEventListener> listener ) override;

        /**
         * @brief Remove a previously registered event listener.
         *
         * @param listener Listener to remove; ignored if not registered.
         */
        void removeListener( SmartPtr<IEventListener> listener ) override;

        /**
         * @brief Remove all registered event listeners.
         */
        void removeListeners() override;

        /**
         * @brief Get the delta of the most recent mouse wheel message.
         *
         * The value is replaced (not accumulated) by each wheel message and is not reset by this
         * call; it is zeroed by load() and debugger suspension.
         *
         * @return Vector with the vertical wheel delta in Y, or the horizontal delta in X.
         */
        Vector3<real_Num> getMouseScroll() const override;

        /**
         * @brief Query whether Shift is currently pressed.
         *
         * Tracks the state sampled on the last key message unless overridden by setShiftPressed().
         *
         * @return true if Shift is down.
         */
        bool isShiftPressed() const override;

        /**
         * @brief Set the Shift pressed state (used for synthetic or injected input).
         *
         * @param shiftPressed True if Shift should be considered pressed.
         */
        void setShiftPressed( bool shiftPressed ) override;

        /**
         * @brief Check whether a mouse button is currently down.
         *
         * Queries the live OS state (Win32 GetAsyncKeyState), independent of queued events.
         *
         * @param button Button index: 0 left, 1 right, 2 middle, 3 X1, 4 X2.
         * @return true if the button is down; always false for other indices and on non-Win32.
         */
        bool isMouseButtonDown( u32 button ) const override;

        /**
         * @brief Get the stored timestamp of the last mouse click.
         *
         * The manager only stores this value; clients update it with setLastClickTime().
         *
         * @return Last click time in seconds (initially 0).
         */
        f64 getLastClickTime() const override;

        /**
         * @brief Set the timestamp of the last mouse click.
         *
         * @param lastClickTime Time in seconds.
         */
        void setLastClickTime( f64 lastClickTime ) override;

        /**
         * @brief Get the double-click interval in seconds.
         *
         * @return Maximum time between two clicks to count as a double click (default 0.25).
         */
        f64 getDoubleClickInterval() const override;

        /**
         * @brief Set the double-click interval used by clients to detect double clicks.
         *
         * @param doubleClickInterval Interval in seconds.
         */
        void setDoubleClickInterval( f64 doubleClickInterval ) override;

        /**
         * @brief Get the stored timestamp of the last input activity.
         *
         * The manager only stores this value; clients update it with setLastInputTime().
         *
         * @return Last input time in seconds (initially 0).
         */
        f64 getLastInputTime() const override;

        /**
         * @brief Set the timestamp of the last input activity.
         *
         * @param lastInputTime Time in seconds.
         */
        void setLastInputTime( f64 lastInputTime ) override;

        /**
         * @brief Get the accumulated input time (used for time-based input queries).
         *
         * @return Input time in seconds (initially 0).
         */
        f64 getInputTime() const override;

        /**
         * @brief Set the accumulated input time.
         *
         * @param inputTime Time in seconds.
         */
        void setInputTime( f64 inputTime ) override;

        /**
         * @brief Add to the accumulated input time.
         *
         * @param inputTime Delta time in seconds to add.
         */
        void addInputTime( f64 inputTime ) override;

        /**
         * @brief Check whether the given key is pressed at the moment.
         *
         * Queries the live OS state (Win32 GetAsyncKeyState); KeyCodes::KEY_LALT is mapped to the
         * generic Alt key.
         *
         * @param keyCode KeyCodes value (a Win32 virtual-key code).
         * @return true if pressed; always false on non-Win32.
         */
        bool isKeyPressed( KeyCodes keyCode ) const override;

        /**
         * @brief Query whether mouse input is processed.
         *
         * When false, mouse window messages are ignored and produce no events.
         *
         * @return true if mouse input is enabled (default).
         */
        bool getCreateMouse() const override;

        /**
         * @brief Enable or disable processing of mouse window messages.
         *
         * @param createMouse True to process mouse input.
         */
        void setCreateMouse( bool createMouse ) override;

        /**
         * @brief Query whether keyboard input is processed.
         *
         * When false, key window messages are ignored and produce no events.
         *
         * @return true if keyboard input is enabled (default).
         */
        bool getCreateKeyboard() const override;

        /**
         * @brief Enable or disable processing of keyboard window messages.
         *
         * @param createKeyboard True to process keyboard input.
         */
        void setCreateKeyboard( bool createKeyboard ) override;

        /**
         * @brief Query whether joysticks are polled.
         *
         * When false, no joystick polling occurs and joystick events are rejected by triggerEvent()
         * and postEvent(). Also exposed as the @c input.gamepad property.
         *
         * @return true if joystick support is enabled (default).
         */
        bool getCreateJoysticks() const override;

        /**
         * @brief Enable or disable joystick polling and joystick events.
         *
         * @param createJoysticks True to enable joystick support.
         */
        void setCreateJoysticks( bool createJoysticks ) override;

        /**
         * @brief Get the graphics window input is received from.
         *
         * @return The window, or null if none was set or it has been destroyed.
         */
        SmartPtr<render::IGraphicsWindow> getWindow() const override;

        /**
         * @brief Set the graphics window to receive OS/window events from.
         *
         * Detaches the internal listener from the previous window, attaches it to the new one
         * (creating it if needed) and reapplies the current properties (touch, gesture and capture
         * settings) to the new window. The window is held weakly.
         *
         * @param window Graphics window; null only detaches from the previous window.
         */
        void setWindow( SmartPtr<render::IGraphicsWindow> window ) override;

        /**
         * @brief Get the connected joysticks.
         *
         * @return Copy of the joystick slots that report at least one button or axis; slots of
         *         disconnected devices are omitted.
         */
        Array<SmartPtr<IJoystick>> getJoysticks() const override;

        /**
         * @brief Replace the joystick slot list.
         *
         * @note update() reuses slots that are Joystick instances and replaces other entries when
         *       it polls a device.
         *
         * @param joysticks New joystick list.
         */
        void setJoysticks( const Array<SmartPtr<IJoystick>> &joysticks ) override;

        /**
         * @brief Get the task on which update() polls joysticks.
         *
         * @return Input task (default TaskId::Input).
         */
        TaskId getTask() const override;

        /**
         * @brief Set the task on which update() polls joysticks.
         *
         * @param task Task id.
         */
        void setTask( TaskId task ) override;

        /**
         * @brief Get the stored event task id.
         *
         * @note Stored for clients; this class does not use it for dispatching.
         *
         * @return Event task (default TaskId::Application).
         */
        TaskId getEventTask() const override;

        /**
         * @brief Set the stored event task id.
         *
         * @param eventTask Task id.
         */
        void setEventTask( TaskId eventTask ) override;

        /**
         * @brief Retrieve the underlying object pointer.
         *
         * @param ppObject Receives a pointer to this InputDeviceManager; must not be null.
         */
        void _getObject( void **ppObject ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Internal window listener that translates graphics window messages into input events.
         *
         * On Win32 it converts mouse, wheel, key, touch and gesture messages into events that are
         * published through the owner's postEvent(), updates the owner's cached mouse, keyboard and
         * modifier state, and manages mouse capture while dragging. It is a no-op on other platforms.
         */
        class WindowListener : public render::IGraphicsWindowListener
        {
        public:
            /**
             * @brief Construct a listener without an owner.
             */
            WindowListener();

            /**
             * @brief Virtual destructor.
             */
            ~WindowListener() override;

            /**
             * @brief Generic event callback of the listener.
             *
             * @param eventType Type of the event.
             * @param eventValue Event identifier hash.
             * @param arguments Event arguments (unused).
             * @param sender Event sender (unused).
             * @param object Event object (unused).
             * @param event Event data (unused).
             * @return Parameter holding true for the window-closing event so closing is not
             *         blocked; an empty Parameter for every other event.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @brief Handle a native window message and turn it into input events.
             *
             * Ignored when there is no owner or the event is not a WindowMessageData. While the
             * owner is debugger-suspended no events are produced, but touch and gesture handles
             * are still released. Mouse, key and touch handling honors the owner's enable flags;
             * losing capture or focus ends a drag and resets mouse delta tracking.
             *
             * @param event Window event carrying the native message.
             */
            void handleEvent( SmartPtr<render::IGraphicsWindowEvent> event ) override;

            /**
             * @brief Get pointer to the owner InputDeviceManager.
             *
             * @return Owner, or null if detached.
             */
            InputDeviceManager *getOwner() const;

            /**
             * @brief Set the owner InputDeviceManager pointer.
             *
             * Also resets mouse delta tracking.
             *
             * @param owner Non-owning pointer; null detaches the listener.
             */
            void setOwner( InputDeviceManager *owner );

            WP_CLASS_REGISTER_DECL;

        protected:
            /// Non-owning pointer to the manager that created this listener; null once detached.
            InputDeviceManager *m_owner = nullptr;
            /// Last known mouse position in client coordinates, used to compute move deltas.
            Vector2<real_Num> m_lastMousePosition;
            /// Whether m_lastMousePosition contains a position from the current focus session.
            bool m_hasLastMousePosition = false;
        };

        /// Weak pointer to the associated graphics window (may be null).
        AtomicWeakPtr<render::IGraphicsWindow> m_window;
        /// Strong reference to the window listener object.
        SmartPtr<WindowListener> m_windowListener;

        /// Most recent mouse or keyboard input event produced from window messages.
        SmartPtr<IInputEvent> m_currentInputEvent;
        /// Most recent mouse state snapshot.
        SmartPtr<IMouseState> m_currentMouseState;
        /// Most recent keyboard state snapshot; replaced under m_mutex.
        SmartPtr<IKeyboardState> m_currentKeyboardState;

        /// Delta of the most recent mouse wheel message (replaced, not accumulated).
        Vector3<real_Num> m_mouseScroll;
        /// Last known mouse position in client coordinates.
        Vector2<real_Num> m_mousePosition;
        /// Mouse position where the current left-button drag started.
        Vector2<real_Num> m_dragStartPosition;

        /// Time of the last click, stored on behalf of clients (seconds).
        f64 m_lastClickTime = 0.0;
        /// Maximum time between two clicks to count as a double click (seconds).
        f64 m_doubleClickInterval = 0.25;
        /// Time of the last input activity, stored on behalf of clients (seconds).
        f64 m_lastInputTime = 0.0;
        /// Accumulated input time (seconds).
        f64 m_inputTime = 0.0;

        /// Task on which update() polls joysticks.
        TaskId m_task = TaskId::Input;
        /// Stored event task id; not used for dispatching by this class.
        TaskId m_eventTask = TaskId::Application;

        /// Whether Shift is considered pressed; sampled on key messages or set explicitly.
        atomic_bool m_shiftPressed = false;
        /// Whether Control is considered pressed; sampled on key messages.
        atomic_bool m_controlPressed = false;
        /// Cursor visibility flag.
        atomic_bool m_cursorVisible = true;
        /// Whether gameplay input is held by the debugger, see setDebuggerSuspended().
        atomic_bool m_debuggerSuspended = false;

        /// Debug flag set by the constructor; not read elsewhere in this implementation.
        bool m_debugEnabled = false;

        /// Whether mouse window messages are processed.
        bool m_createMouse = true;
        /// Whether keyboard window messages are processed.
        bool m_createKeyboard = true;
        /// Whether joysticks are polled and joystick events accepted.
        bool m_createJoysticks = true;

        /// Whether a left-button drag is currently in progress.
        atomic_bool m_isDragging = false;

        /// Thread-safe listener collection.
        ConcurrentArray<SmartPtr<IEventListener>> m_listeners;

        /// Event queues indexed by TaskId; every posted event is pushed to all of them and each
        /// task drains its own queue in update().
        Array<ConcurrentQueue<SmartPtr<IInputEvent>>> m_inputEventQueue;

        /// Game input objects keyed by hash id.
        using GameInputs = HashMap<hash_type, SmartPtr<IGameInput>>;
        /// Registered game inputs.
        GameInputs m_gameInputs;

        /// Joystick slots in discovery order.
        Array<SmartPtr<IJoystick>> m_joysticks;
        /// Touch input enabled (property @c input.touch); touch events are dropped when false.
        /// Mouse capture on button press enabled (property @c input.mouseCapture).
        bool m_touchEnabled = true, m_mouseCaptureEnabled = true;
        /// Rebinding enabled in game input maps (@c input.rebinds), gesture events enabled
        /// (@c input.gestures) and haptic feedback enabled (@c input.haptics).
        bool m_rebindingEnabled = true, m_gesturesEnabled = true, m_hapticsEnabled = true;
        /// Project input system id (property @c input.system); the value 2 disables the built-in
        /// mapping of key and joystick events to game input actions in postEvent().
        s32 m_projectInputSystem = 0;

        /// Active joystick backend. A value of zero means
        /// no backend has detected a connected device, one is XInput, and two is
        /// the Windows multimedia joystick API used for generic HID devices.
        u32 m_joystickBackend = 0;
        /// Native device ids (XInput user indices or multimedia joystick ids) of the connected
        /// devices of the active backend, in slot order.
        Array<u32> m_joystickDeviceIds;
        /// Last reported POV hat value for each joystick slot.
        Array<u32> m_joystickPovs;
        /// Tick count (milliseconds) at which joystick discovery runs next; discovery repeats
        /// every second.
        u64 m_nextJoystickDiscoveryTime = 0;

        /// Recursive mutex serializing event publication, debugger suspension transitions and
        /// replacement of the keyboard snapshot.
        mutable RecursiveSpinMutex m_mutex;

        /// Optional platform input backend (set up by Win32InputBackend, which is a friend). Not
        /// owned and not assigned or used by this implementation.
        IInputPlatformBackend *m_backend = nullptr;
    };
}  // namespace workphone

#endif  // InputDeviceManager_h__
