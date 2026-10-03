#ifndef InputDeviceManager_h__
#define InputDeviceManager_h__

#include <Workphone/Interface/Input/IInputDeviceManager.hpp>
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
     * @brief Platform-agnostic manager for input devices and events.
     *
     * InputDeviceManager coordinates creation and querying of input device
     * objects (mouse, keyboard, joysticks), maintains current input state
     * snapshots, queues and dispatches input events, and integrates with the
     * graphics window to receive OS-level input callbacks.
     */
    class WPCore_API InputDeviceManager : public IInputDeviceManager
    {
        friend class Win32InputBackend;

    public:
        /**
         * @brief Construct a new InputDeviceManager instance.
         */
        InputDeviceManager();

        /**
         * @brief Virtual destructor.
         */
        ~InputDeviceManager() override;

        /**
         * @brief Load configuration or shared data required by the manager.
         *
         * @param data Optional shared object with initialization data
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Reinitialize platform bindings without discarding client registrations.
         *
         * Existing joystick objects, listeners, and game-input mappings remain valid.
         */
        void reload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unload resources acquired during load().
         *
         * @param data Optional shared object provided at load time
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Update internal state, process queued events and dispatch
         *        them to listeners.
         */
        void update() override;

        /**
         * @brief Immediately trigger (dispatch) an input event to listeners.
         *
         * @param inputEvent Event to dispatch
         */
        void triggerEvent( SmartPtr<IInputEvent> inputEvent ) override;

        /**
         * @brief Queue an input event for later processing by update().
         *
         * @param event Event to enqueue
         */
        void queueEvent( SmartPtr<IInputEvent> event ) override;

        /**
         * @brief Post an event to the manager's event queue, may fail if the
         *        queue is full or the manager is shutting down.
         *
         * @param event Event to post
         * @return true on success, false on failure
         */
        bool postEvent( SmartPtr<IInputEvent> event ) override;

        /**
         * @brief Create a new input event instance.
         *
         * @return SmartPtr<IInputEvent>
         */
        SmartPtr<IInputEvent> createInputEvent() override;

        /**
         * @brief Create a new mouse state snapshot object.
         *
         * @return SmartPtr<IMouseState>
         */
        SmartPtr<IMouseState> createMouseState() override;

        /**
         * @brief Create a new keyboard state snapshot object.
         *
         * @return SmartPtr<IKeyboardState>
         */
        SmartPtr<IKeyboardState> createKeyboardState() override;

        /**
         * @brief Get the input event currently being processed.
         *
         * @return SmartPtr<IInputEvent>
         */
        SmartPtr<IInputEvent> getCurrentInputEvent() const override;

        /**
         * @brief Get the current mouse state snapshot.
         *
         * @return SmartPtr<IMouseState>
         */
        SmartPtr<IMouseState> getCurrentMouseState() const override;

        /**
         * @brief Get the current keyboard state snapshot.
         *
         * @return SmartPtr<IKeyboardState>
         */
        SmartPtr<IKeyboardState> getCurrentKeyboardState() const override;

        /**
         * @brief Add a GameInput instance identified by the provided hash id.
         *
         * @param id Unique identifier for the game input
         * @return SmartPtr<IGameInput> The created or existing game input
         */
        SmartPtr<IGameInput> addGameInput( hash_type id ) override;

        /**
         * @brief Find a previously added GameInput by id.
         *
         * @param id Identifier to search for
         * @return SmartPtr<IGameInput> Found game input or null
         */
        SmartPtr<IGameInput> findGameInput( hash_type id ) const override;

        /**
         * @brief Get a snapshot array of all registered game inputs.
         *
         * @return Array<SmartPtr<IGameInput>>
         */
        Array<SmartPtr<IGameInput>> getGameInputs() const override;

        /**
         * @brief Query whether the system cursor is visible.
         *
         * @return true if visible
         */
        bool isCursorVisible() const override;

        /**
         * @brief Show or hide the system cursor.
         *
         * @param visible True to show, false to hide
         */
        void setCursorVisible( bool visible ) override;

        /**
         * @brief Register an event listener to receive input events.
         *
         * @param listener Listener to add
         */
        void addListener( SmartPtr<IEventListener> listener ) override;

        /**
         * @brief Remove a previously registered event listener.
         *
         * @param listener Listener to remove
         */
        void removeListener( SmartPtr<IEventListener> listener ) override;

        /**
         * @brief Remove all registered event listeners.
         */
        void removeListeners() override;

        /**
         * @brief Get the accumulated mouse scroll (wheel) delta since last
         *        query.
         *
         * @return Vector3<real_Num> Scroll delta vector
         */
        Vector3<real_Num> getMouseScroll() const override;

        /**
         * @brief Query whether Shift is currently pressed.
         *
         * @return true if Shift is down
         */
        bool isShiftPressed() const override;

        /**
         * @brief Set the Shift pressed state (used for synthetic or injected input).
         *
         * @param shiftPressed True if Shift should be considered pressed
         */
        void setShiftPressed( bool shiftPressed ) override;

        /**
         * @brief Check whether a mouse button is currently down.
         *
         * @param button Button index
         * @return true if the button is down
         */
        bool isMouseButtonDown( u32 button ) const override;

        /**
         * @brief Get timestamp of the last mouse click.
         *
         * @return f64 Last click time in seconds
         */
        f64 getLastClickTime() const override;

        /**
         * @brief Set timestamp of the last mouse click.
         *
         * @param lastClickTime Time in seconds
         */
        void setLastClickTime( f64 lastClickTime ) override;

        /**
         * @brief Get the configured double-click interval in seconds.
         *
         * @return f64 Double click interval
         */
        f64 getDoubleClickInterval() const override;

        /**
         * @brief Set the double-click interval used to detect double clicks.
         *
         * @param doubleClickInterval Interval in seconds
         */
        void setDoubleClickInterval( f64 doubleClickInterval ) override;

        /**
         * @brief Get the timestamp of the last input activity.
         *
         * @return f64 Last input time in seconds
         */
        f64 getLastInputTime() const override;

        /**
         * @brief Set the timestamp of the last input activity.
         *
         * @param lastInputTime Time in seconds
         */
        void setLastInputTime( f64 lastInputTime ) override;

        /**
         * @brief Get the accumulated input time (used for time-based input queries).
         *
         * @return f64 Input time in seconds
         */
        f64 getInputTime() const override;

        /**
         * @brief Set the accumulated input time.
         *
         * @param inputTime Time in seconds
         */
        void setInputTime( f64 inputTime ) override;

        /**
         * @brief Add to the accumulated input time.
         *
         * @param inputTime Delta time in seconds to add
         */
        void addInputTime( f64 inputTime ) override;

        /**
         * @brief Check whether the given keycode is pressed at the moment.
         *
         * @param keyCode KeyCodes value
         * @return true if pressed
         */
        bool isKeyPressed( KeyCodes keyCode ) const override;

        /**
         * @brief Query whether the manager should create a mouse device object.
         *
         * @return true if mouse creation is enabled
         */
        bool getCreateMouse() const override;

        /**
         * @brief Enable or disable automatic creation of a mouse device object.
         *
         * @param createMouse True to create a mouse object
         */
        void setCreateMouse( bool createMouse ) override;

        /**
         * @brief Query whether the manager should create a keyboard device object.
         *
         * @return true if keyboard creation is enabled
         */
        bool getCreateKeyboard() const override;

        /**
         * @brief Enable or disable automatic creation of a keyboard device object.
         *
         * @param createKeyboard True to create keyboard object
         */
        void setCreateKeyboard( bool createKeyboard ) override;

        /**
         * @brief Query whether the manager should create joystick device objects.
         *
         * @return true if joystick creation is enabled
         */
        bool getCreateJoysticks() const override;

        /**
         * @brief Enable or disable automatic creation of joystick device objects.
         *
         * @param createJoysticks True to create joystick objects
         */
        void setCreateJoysticks( bool createJoysticks ) override;

        /**
         * @brief Get associated graphics window used for receiving input.
         *
         * @return SmartPtr<render::IGraphicsWindow> Graphics window
         */
        SmartPtr<render::IGraphicsWindow> getWindow() const override;

        /**
         * @brief Set the graphics window to receive OS/window events from.
         *
         * @param window Graphics window pointer
         */
        void setWindow( SmartPtr<render::IGraphicsWindow> window ) override;

        /**
         * @brief Get the array of joystick devices managed by the manager.
         *
         * @return Array<SmartPtr<IJoystick>>
         */
        Array<SmartPtr<IJoystick>> getJoysticks() const override;

        /**
         * @brief Replace the current joystick device list.
         *
         * @param joysticks Array of joystick pointers
         */
        void setJoysticks( const Array<SmartPtr<IJoystick>> &joysticks ) override;

        /**
         * @brief Get the task id associated with input processing.
         *
         * @return TaskId
         */
        TaskId getTask() const override;

        /**
         * @brief Set the task id used for input processing.
         *
         * @param task TaskId value
         */
        void setTask( TaskId task ) override;

        /**
         * @brief Get the task id used for event dispatching.
         *
         * @return TaskId
         */
        TaskId getEventTask() const override;

        /**
         * @brief Set the task id used for event dispatching.
         *
         * @param eventTask TaskId for event dispatch
         */
        void setEventTask( TaskId eventTask ) override;

        /**
         * @brief Internal helper to obtain underlying object pointer.
         */
        void _getObject( void **ppObject ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Internal window listener that forwards graphics window events
         *        into the input manager.
         */
        class WindowListener : public render::IGraphicsWindowListener
        {
        public:
            WindowListener();
            ~WindowListener() override;

            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            void handleEvent( SmartPtr<render::IGraphicsWindowEvent> event ) override;

            /**
             * @brief Get pointer to owner InputDeviceManager.
             *
             * @return InputDeviceManager*
             */
            InputDeviceManager *getOwner() const;

            /**
             * @brief Set the owner InputDeviceManager pointer.
             */
            void setOwner( InputDeviceManager *owner );

            WP_CLASS_REGISTER_DECL;

        protected:
            /// Non-owning pointer to the manager that created this listener.
            InputDeviceManager *m_owner = nullptr;
            /// Last known mouse position used to compute deltas and dragging.
            Vector2<real_Num> m_lastMousePosition;
            /// Whether m_lastMousePosition contains a position from the current focus session.
            bool m_hasLastMousePosition = false;
        };

        /// Weak pointer to the associated graphics window (may be null).
        AtomicWeakPtr<render::IGraphicsWindow> m_window;
        /// Strong reference to the window listener object.
        SmartPtr<WindowListener> m_windowListener;

        /// Currently processed input event and state snapshots.
        SmartPtr<IInputEvent> m_currentInputEvent;
        SmartPtr<IMouseState> m_currentMouseState;
        SmartPtr<IKeyboardState> m_currentKeyboardState;

        /// Accumulated mouse scroll delta (wheel).
        Vector3<real_Num> m_mouseScroll;
        /// Current mouse position and drag start position.
        Vector2<real_Num> m_mousePosition;
        Vector2<real_Num> m_dragStartPosition;

        /// Timing values for click/double-click detection and input timestamps.
        f64 m_lastClickTime = 0.0;
        f64 m_doubleClickInterval = 0.25;
        f64 m_lastInputTime = 0.0;
        f64 m_inputTime = 0.0;

        /// Task identifiers used for scheduling input and event processing.
        TaskId m_task = TaskId::Input;
        TaskId m_eventTask = TaskId::Application;

        /// Modifier key atomic flags.
        atomic_bool m_shiftPressed = false;
        atomic_bool m_controlPressed = false;
        /// Cursor visibility atomic flag.
        atomic_bool m_cursorVisible = true;

        bool m_debugEnabled = false;

        /// Creation flags used to determine which device objects are created.
        bool m_createMouse = true;
        bool m_createKeyboard = true;
        bool m_createJoysticks = true;

        /// Whether a mouse drag is currently in progress.
        atomic_bool m_isDragging = false;

        /// Thread-safe listener collection.
        ConcurrentArray<SmartPtr<IEventListener>> m_listeners;

        /// Per-producer queues of input events.
        Array<ConcurrentQueue<SmartPtr<IInputEvent>>> m_inputEventQueue;

        using GameInputs = HashMap<hash_type, SmartPtr<IGameInput>>;
        /// Map of game input objects keyed by hash id.
        GameInputs m_gameInputs;

        /// Managed joystick device list.
        Array<SmartPtr<IJoystick>> m_joysticks;

        /// Platform joystick backend and native device ids. A value of zero means
        /// no backend has detected a connected device, one is XInput, and two is
        /// the Windows multimedia joystick API used for generic HID devices.
        u32 m_joystickBackend = 0;
        Array<u32> m_joystickDeviceIds;
        Array<u32> m_joystickPovs;
        u64 m_nextJoystickDiscoveryTime = 0;

        /// Recursive mutex protecting internal mutable state for thread-safety.
        mutable RecursiveSpinMutex m_mutex;

        /// Platform-specific input backend that processes window events and device state (owned by this
        /// manager).
        IInputPlatformBackend *m_backend = nullptr;
    };
}  // namespace workphone

#endif  // InputDeviceManager_h__
