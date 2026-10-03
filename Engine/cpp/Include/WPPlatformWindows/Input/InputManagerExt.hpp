#ifndef __InputManagerExt_h__
#define __InputManagerExt_h__

#include <Workphone/Interface/Input/IInputManager.hpp>
#include <Workphone/Input/InputEvent.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include "Workphone/Atomics/AtomicFloat.hpp"
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Input/AxisData.hpp>
#include <Workphone/Input/InputFunction.hpp>
#include <Workphone/Input/InputConfiguration.hpp>
#include "Workphone/System/Job.hpp"
#include <unordered_map>

namespace workphone
{
    /**
     * @class InputManagerExt
     * @brief Extended input manager providing comprehensive device and event management.
     *
     * @details
     * InputManagerExt is a production-ready input management system that extends IInputManager
     * to provide robust handling of various input devices including keyboards, mice, joysticks,
     * and custom hardware adapters. The class is designed with defensive programming practices,
     * comprehensive error logging, and data-driven configuration through the Properties interface.
     *
     * Key features:
     * - Multi-device input support (keyboard, mouse, joystick, hardware adapters)
     * - Thread-safe event queuing and listener management
     * - Configurable channel mapping with up to 128 input channels
     * - Data-driven configuration via Properties for editor integration
     * - Defensive error handling with graceful degradation
     * - Optional database integration for legacy compatibility
     * - Hardware adapter support with multiple operating modes
     *
     * Thread Safety:
     * - Uses atomic types for frequently accessed state variables
     * - ConcurrentArray and ConcurrentQueue for thread-safe collections
     * - Listeners and events can be safely accessed from multiple threads
     *
     * Configuration:
     * - Use getProperties()/setProperties() for data-driven configuration
     * - All timing, flags, and device settings are exposed through Properties
     * - Database integration is optional and falls back to defaults if unavailable
     *
     * Error Handling:
     * - All public methods validate inputs and log errors via WP_LOG_ERROR/WARNING
     * - Methods continue execution and use safe defaults on errors rather than throwing
     * - Null pointer checks on all listener and event operations
     *
     * @note Originally designed for flight simulator input but refactored for general use.
     * @see IInputManager, Properties, AxisData, IInputEvent
     */
    class WPCore_API InputManagerExt : public IInputManager
    {
    public:
        /**
         * @class StartDongleJob
         * @brief Background job for initializing hardware adapter ("dongle") connections.
         *
         * @details
         * This job handles asynchronous initialization of USB hardware adapters,
         * allowing the main thread to continue while device enumeration and
         * connection establishment occurs in the background.
         */
        class StartDongleJob : public Job
        {
        public:
            /**
             * @brief Constructs a StartDongleJob for the given input manager.
             * @param inputManager Pointer to the InputManagerExt instance to initialize.
             */
            StartDongleJob( InputManagerExt *inputManager );

            /** @brief Destructor. */
            ~StartDongleJob() override;

            /** @brief Executes the dongle initialization task. */
            void execute() override;

            RawPtr<InputManagerExt> m_inputManager;  ///< Pointer to the owning input manager.
        };

        /**
         * @class CreateJobstickJob
         * @brief Background job for creating and initializing joystick devices.
         *
         * @details
         * This coroutine-based job attempts to create a joystick device with
         * retry logic. It yields between attempts to avoid blocking the main thread.
         */
        class CreateJobstickJob : public Job
        {
        public:
            /**
             * @brief Constructs a CreateJobstickJob for the given input manager.
             * @param inputManager Pointer to the InputManagerExt instance.
             */
            CreateJobstickJob( RawPtr<InputManagerExt> inputManager );

            /** @brief Destructor. */
            ~CreateJobstickJob() override;

            /** @brief Executes the joystick creation task. */
            void execute() override;

            /**
             * @brief Coroutine step function with retry logic.
             * @param rYield Coroutine yield data for suspension/resumption.
             */
            void coroutine_execute_step( SmartPtr<ICoroutineData> &rYield ) override;

            RawPtr<InputManagerExt> m_inputManager;  ///< Pointer to the owning input manager.
            s32 m_retries;                           ///< Number of retry attempts made.
        };

        // std::map<int, SmartPtr<UnityKeyMapping>> gUnityKeyMap;

        /**
         * @enum modes
         * @brief Hardware adapter operating modes.
         *
         * @details
         * Defines the various operating modes supported by hardware adapters.
         * These modes determine the communication protocol and power settings
         * used when interfacing with external RC transmitters or receivers.
         */
        enum modes
        {
            no_mode = 0x00,        ///< Idle state, no active mode
            get_version = 0x80,    ///< Request version ID information from adapter
            ppm_mode = 0x81,       ///< Basic PPM via buddy box (no RX power required)
            ppm_rx_mode = 0x82,    ///< PPM via RX with power supply (~4.8v for legacy receivers)
            s_bus_mode = 0x83,     ///< S.BUS protocol mode
            dsm2_mode = 0x84,      ///< DSM2 (Spektrum) protocol mode
            dsmx_mode = 0x85,      ///< DSMX (Spektrum) protocol mode
            dsm2_bind = 0x71,      ///< DSM2 binding mode
            dsmx_bind = 0x72,      ///< DSMX binding mode
            get_chipid = 0x91,     ///< Request chip ID from hardware
            get_signature = 0x92,  ///< Request signature from hardware
            interlink = 0x95,      ///< Interlink controller mode
            vbar = 0x96,           ///< VBar system mode
            gamepad = 0x97,        ///< Generic gamepad mode
            jeti_usb = 0x98,       ///< Jeti USB adapter mode
            spare_usb1 = 0x93,     ///< Reserved USB mode 1
            taranis_usb = 0x94,    ///< Taranis USB mode
            bootload = 0x99        ///< Bootloader mode for firmware updates
        };

        /**
         * @enum updateStates
         * @brief Firmware update state machine states.
         *
         * @details
         * Tracks the current state during firmware update operations.
         * The update process is asynchronous and progresses through these states.
         */
        enum updateStates
        {
            UpdateIdle,          ///< No update in progress
            StartUpdate,         ///< Update process initiated
            SendBootLoad,        ///< Sending bootloader command
            WaitingForBootload,  ///< Waiting for bootloader to respond
            SendUpdate,          ///< Transmitting firmware data
            WaitingForUpdate,    ///< Waiting for update confirmation
            UpdateFinished       ///< Update completed successfully
        };

        static const s32 NUM_CHANNELS = 128;  ///< Maximum number of input channels supported

        /**
         * @brief Default constructor. Initializes input manager with default settings.
         *
         * @details
         * Creates an InputManagerExt instance with:
         * - Buffered keyboard and mouse input enabled
         * - 128 input channels initialized to zero
         * - All flags set to safe defaults
         * - Double-click interval of 0.25 seconds
         *
         * @note Call load() after construction to initialize input devices.
         */
        InputManagerExt();

        /**
         * @brief Constructs input manager with specified window and buffering settings.
         *
         * @param window Graphics window for input capture (can be null for headless mode).
         * @param bufferedKeys If true, keyboard input is buffered; otherwise immediate.
         * @param bufferedMouse If true, mouse input is buffered; otherwise immediate.
         *
         * @details
         * Allows configuration of input buffering at construction time. Buffered input
         * queues events for processing on the next frame, while immediate input triggers
         * listeners synchronously.
         *
         * @note Call load() after construction to initialize input devices.
         */
        InputManagerExt( render::IGraphicsWindow *window, bool bufferedKeys, bool bufferedMouse );

        /**
         * @brief Destructor. Cleans up all input devices and releases resources.
         *
         * @details
         * Automatically calls unload() to ensure proper cleanup of:
         * - Input device handles
         * - Event listeners
         * - Channel data
         * - Hardware adapter connections
         */
        ~InputManagerExt() override;

        /**
         * @brief Initializes input devices and prepares the manager for use.
         *
         * @details
         * Loads and initializes:
         * - Keyboard input (if enabled)
         * - Mouse input (if enabled)
         * - Joystick devices (if available)
         * - Hardware adapters (if configured)
         * - Channel data structures
         *
         * @note Must be called after construction and before using the input manager.
         * @note Safe to call multiple times; checks loading state internally.
         */
        void load();

        /**
         * @brief Unloads input devices and releases resources.
         *
         * @param flags Bitfield flags controlling unload behavior (reserved for future use).
         *
         * @details
         * Releases all input device handles, clears listeners, and frees allocated memory.
         * Safe to call multiple times.
         *
         * @note Automatically called by destructor.
         */
        void unload( u32 flags );

        /**
         * @brief Pre-update phase for input processing.
         *
         * @param task Task identifier for the current update.
         * @param t Current simulation time in seconds.
         * @param dt Delta time since last update in seconds.
         *
         * @details
         * Called before the main update loop. Handles:
         * - Input device polling
         * - Event queue processing
         * - Listener priority updates
         *
         * @note Call this before updateState() in your update loop.
         */
        void preUpdate( TaskId task, const double &t, const double &dt );

        /**
         * @brief Main update phase for input state management.
         *
         * @param task Task identifier for the current update.
         * @param t Current simulation time in seconds.
         * @param dt Delta time since last update in seconds.
         *
         * @details
         * Updates:
         * - Hardware adapter state
         * - Joystick connection status
         * - Input recording/playback (if enabled)
         *
         * @note Call this after preUpdate() in your update loop.
         */
        void updateState( TaskId task, const double &t, const double &dt );

        /** @brief Updates internal input state variables. Called automatically by updateState(). */
        void updateInputState();

        /**
         * @brief Updates hardware adapter ("dongle") state.
         *
         * @param t Current simulation time in seconds.
         * @param dt Delta time since last update in seconds.
         *
         * @details
         * Polls hardware adapter for:
         * - Connection status
         * - Firmware version
         * - Channel data
         * - Operating mode changes
         */
        void updateDongle( const double &t, const double &dt );

        /**
         * @brief Registers an event listener to receive input events.
         *
         * @param listener SmartPtr to the listener to add. Must not be null.
         *
         * @details
         * The listener will receive all input events (keyboard, mouse, joystick, etc.)
         * in priority order. Duplicate listeners are ignored.
         *
         * @note Thread-safe. Validates null pointers and logs errors.
         * @see removeListener(), removeListeners()
         */
        void addListener( SmartPtr<IEventListener> listener );

        /**
         * @brief Unregisters an event listener.
         *
         * @param listener SmartPtr to the listener to remove. Must not be null.
         *
         * @details
         * The listener will no longer receive input events. Logs a warning if
         * the listener was not found.
         *
         * @note Thread-safe. Validates null pointers and logs errors.
         * @see addListener(), removeListeners()
         */
        void removeListener( SmartPtr<IEventListener> listener );

        /**
         * @brief Removes all registered event listeners.
         *
         * @details
         * Clears the entire listener list. Useful during shutdown or scene transitions.
         *
         * @see addListener(), removeListener()
         */
        void removeListeners();

        /**
         * @brief Manually sets an input channel value.
         *
         * @param channel Channel index [0, NUM_CHANNELS). Must be valid.
         * @param value Normalized channel value, typically [-1.0, 1.0].
         *
         * @details
         * Allows external sources (e.g., network, AI, replay) to inject input values.
         * Validates channel bounds and value for NaN/Inf. Logs errors for invalid inputs.
         *
         * @note Requires useOverride flag to be enabled for values to take effect.
         * @see getAxisValue(), getAxisValueRaw(), setUseOverride()
         */
        void setAxisValue( s32 channel, f32 value ) override;

        /** Gets input from the Tx. */
        f32 getAxisValue( s32 channel ) override;

        /** Gets input from the Tx. */
        f32 getAxisValueRaw( s32 channel ) override;

        /** */
        void setWindowExtents( s32 width, s32 height );

        void reset();

        void resetDongle();

        bool getKeyboardInputEnabled() const;
        void setKeyboardInputEnabled( bool keyboardInputEnabled );

        // get the button id of last mouse event
        s32 getBtnID();

        void initDongle( const String &param );

        void getSignature( SmartPtr<IDatabaseManager> database );

        void getChipId( char *hex, SmartPtr<IDatabaseManager> database );

        void resetDongleAuthenticate();

        void getDongleVersion();

        void setupBind( const String &param );

        void setupAdapterModeValue();

        void setupChannelMap( const String &param );

        String getModelTypeFromParam( const String &param );

        void auth();

        void triggerEvent( SmartPtr<IEventListener> inputEvent );

        bool isShiftPressed() const;
        void setShiftPressed( bool shiftPressed );
        bool checkAdaptorMode( void );
        String adaptorTxStatus();
        s32 adaptorTxStatusInt();

        double getLastInputTime() const;
        void setLastInputTime( double lastInputTime );

        bool getUseOverride() const;
        void setUseOverride( bool useOverride );

        void setChannelData( s32 channel, f32 offset, f32 multiplier );
        void updateFirmware( void );

        bool getEnableInputLog() const;
        void setEnableInputLog( bool enableInputLog );

        double getInputTime() const;
        void setInputTime( double inputTime );
        void addInputTime( double inputTime );

        bool getRunInputLog() const;
        void setRunInputLog( bool runInputLog );

        void updateListenersPriority();

        void sendKeyEvent( IInputEvent::EventType evnt, s32 keycode );

        bool isLeftPressed() const;
        void setLeftPressed( bool leftPressed );

        bool isRightPressed() const;
        void setRightPressed( bool rightPressed );

        bool isMiddlePressed() const;
        void setMiddlePressed( bool middlePressed );

        double getLastClickTime() const;
        void setLastClickTime( double lastClickTime );

        double getDoubleClickInterval() const;
        void setDoubleClickInterval( double doubleClickInterval );

        bool getBufferedKeys() const;
        void setBufferedKeys( bool bufferedKeys );

        bool getBufferedMouse() const;
        void setBufferedMouse( bool bufferedMouse );

        bool getEnableInternalInputCapture() const;
        void setEnableInternalInputCapture( bool enableInternalInputCapture );

        Array<SmartPtr<IEventListener>> getInputListeners() const;
        void setInputListeners( const Array<SmartPtr<IEventListener>> &p );

        String getAsString( s32 kc );

        bool isKeyDown( s32 kc );

        void TxChannels2db();
        void StartChannelMonitor();
        void EndChannelMonitor();

        void queueEvent( SmartPtr<IInputEvent> inputEvent );

        s32 getStatus() const;
        void setStatus( s32 status );

        void joystickConnected();
        void joystickPreDisconnect();
        void joystickDisconnected();

        ConcurrentArray<SmartPtr<AxisData>> getChannelData() const;
        void setChannelData( const ConcurrentArray<SmartPtr<AxisData>> &channelData );

        ConcurrentArray<SmartPtr<AxisData>> getChannelDataMap() const;
        void setChannelDataMap( const ConcurrentArray<SmartPtr<AxisData>> &channelDataMap );

        SmartPtr<InputConfiguration> getInputConfiguration() const;
        void setInputConfiguration( SmartPtr<InputConfiguration> inputConfiguration );

        SmartPtr<AxisData> &getChannelDataByIndex( s32 index );
        const SmartPtr<AxisData> &getChannelDataByIndex( s32 index ) const;

        bool isAssigning() const;
        void setAssigning( bool assigning );

        String getCurrentTxModel() const;
        void setCurrentTxModel( const String &currentTxModel );

        u32 getCurrentButtons() const;
        void setCurrentButtons( u32 currentButtons );

        u32 getPreviousButtons() const;
        void setPreviousButtons( u32 previousButtons );

        s32 getNumButtons() const;
        void setNumButtons( s32 numButtons );

        bool isDongleOk() const;
        void setDongleOk( bool dongleOk );

        f32 getGyroOverride() const;
        void setGyroOverride( f32 gyroOverride );

        std::array<unsigned char, 65> getDataArray();

        bool isDeviceUSB() const;

        SmartPtr<IGameInput> addGameInput( hash32 id );

        SmartPtr<IGameInput> findGameInput( hash32 id ) const;

        Array<SmartPtr<IGameInput>> getGameInputs() const;

        bool isCursorVisible() const;

        void setCursorVisible( bool visible );

        bool postEvent( SmartPtr<IInputEvent> event );

        void triggerEvent( SmartPtr<IInputEvent> inputEvent );

        /**
         * @brief Retrieves all configurable properties of the input manager.
         * @return SmartPtr to a Properties object containing all exposed member variables.
         *
         * @details
         * Populates a Properties object with all configurable settings including:
         * - Timing parameters (doubleClickInterval, lastClickTime, etc.)
         * - Input flags (keyboardInputEnabled, bufferedKeys, bufferedMouse, etc.)
         * - Device configuration (adapterMode, currentTxModel, numButtons, etc.)
         * - Channel configuration (jsThrottleChannel, gyroOverride)
         * - State flags (dongleOk, isAssigning, isShiftPressed, etc.)
         *
         * This method enables data-driven configuration via the game editor by exposing
         * internal state through the Properties interface.
         *
         * @note Calls parent IInputManager::getProperties() before adding derived class properties.
         * @see setProperties()
         */
        SmartPtr<Properties> getProperties() const override;

        /**
         * @brief Applies property values to configure the input manager.
         * @param properties SmartPtr to a Properties object containing configuration data.
         *
         * @details
         * Extracts property values from the provided Properties object and applies them
         * to the input manager's configuration. Validates all inputs for range/null checks
         * and logs errors for invalid values while continuing to process remaining properties.
         *
         * Properties that can be configured:
         * - Timing: doubleClickInterval (default 0.25), inputTime, lastClickTime
         * - Flags: keyboardInputEnabled, bufferedKeys, bufferedMouse, etc.
         * - Device: adapterMode, currentTxModel, numButtons (0-128)
         * - Channels: jsThrottleChannel (0-127), gyroOverride (-1.0 to 1.0)
         *
         * @note Calls parent IInputManager::setProperties() before processing derived properties.
         * @note Uses defensive programming: validates inputs, logs errors, continues on failure.
         * @see getProperties()
         */
        void setProperties( SmartPtr<Properties> properties ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        void createRecorderListener();

        void createChannelData();

        void createKeyboard();
        void createMouse();

        void startDongle();
        void stopDongle();

        void updateFirmware( const double &t, const double &dt );

        /** Gets an event object that populated with the necessary data. */
        SmartPtr<InputEvent> getMouseEvent( u32 eventType );

        SmartPtr<InputEvent> getResetEvent( u32 eventType );

        s32 getKeyCode( char c );
        void setupButtons( String type );

        void createJoystick();
        void destroyJoystick();

        void updateJoystick();

        bool isFunction( SmartPtr<InputFunction> &func );

        void UpdateMiscFunctions();

        void setupTxFunctions();

        bool isDongleConnected();
        Array<String> getConnectedDevices() const;

        hash_type getAdapterModeHash() const;

        ConcurrentQueue<SmartPtr<IInputEvent>> m_inputEvents;

        SmartPtr<ITimer> m_timer;

        SmartPtr<InputConfiguration> m_inputConfiguration;

        // std::array<AtomicD, 32> m_rawChannels;
        atomic_f64 m_dongleCheckTime;
        atomic_f64 m_lastInputTime;
        atomic_f64 m_inputTime;
        atomic_f64 m_lastClickTime;
        atomic_f64 m_doubleClickInterval;
        atomic_f64 m_nextDongleCheckTime;

        u32 m_inputIndex;

        s32 m_btnID;

        /// To know is the Tx init was successful.
        atomic_bool m_dongleOk;
        atomic_bool m_txOk;

        /// Used to know if keyboard input should be enabled.
        atomic_bool m_keyboardInputEnabled;

        atomic_s32 m_status;

        bool m_isAssigning;
        bool m_isShiftPressed;
        bool m_bUseOverride;
        bool m_bEnableInputLog;
        bool m_bRunInputLog;

        bool m_bufferedKeys;
        bool m_bufferedMouse;

        bool m_enableInternalInputCapture;

        /// Used to know if the left mouse button is pressed.
        bool m_isLeftPressed;

        /// Used to know if the right mouse button is pressed.
        bool m_isRightPressed;

        /// Used to know if the middle mouse button is pressed.
        bool m_isMiddlePressed;

        s32 m_numButtons;

        s32 m_updateState;
        String m_msg;
        // atomic_bool m_bUseJoyStick;
        atomic_u32 m_previousButtons;
        atomic_u32 m_currentButtons;
        s32 m_jsThrottleChannel;

        SmartPtr<InputFunction> m_idleUpFunction;
        SmartPtr<InputFunction> m_normalModeFunction;
        SmartPtr<InputFunction> m_throttleHoldFunction;

        Array<s32> m_buttonsON;
        Array<s32> m_buttonsOFF;

        /// Queue for thread safe events.
        ConcurrentQueue<SmartPtr<IInputEvent>> m_inputEventQueue;

        /// A typedef for the a map.
        using ChannelDataMap = std::unordered_map<s32, SmartPtr<AxisData>>;

        /// Pool of channel data.
        ConcurrentArray<SmartPtr<AxisData>> m_channelData;

        /// Data for channel mapping.
        ConcurrentArray<SmartPtr<AxisData>> m_channelDataMap;

        /// A vector storing a list of listeners.
        ConcurrentArray<SmartPtr<IEventListener>> m_inputListeners;

        /// Used for debugging.
        std::unordered_map<s32, f64> m_externalChannelInput;

        /// Used to know the current tx mode. This normally base on model type.
        String m_currentTxMode;

        /// Use to know the dongle or the controller mode. E.g. can be buddy for the dongle.
        String m_adapterMode;

        /// A hash value for the quick compare.
        hash_type m_adapterModeHash;

        /// The next time to check the joystick.
        f64 m_nextJoystickCheck;

        /// A channel value for controlling the gyro.
        f32 m_gyroOverride;
    };

    inline u32 InputManagerExt::getCurrentButtons() const
    {
        return m_currentButtons;
    }

    inline u32 InputManagerExt::getPreviousButtons() const
    {
        return m_previousButtons;
    }

    inline bool InputManagerExt::isDongleOk() const
    {
        return m_dongleOk;
    }
}  // namespace workphone

#endif  // __InputManagerExt_h__
