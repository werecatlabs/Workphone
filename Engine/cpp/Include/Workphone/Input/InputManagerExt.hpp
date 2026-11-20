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
     * @brief Input manager that tracks input state, channel (axis) data and event listeners.
     *
     * @details
     * InputManagerExt implements IInputManager. It keeps a pool of NUM_CHANNELS AxisData
     * entries, a channel-to-function map, keyboard/mouse/button state flags, adapter ("dongle")
     * state and a list of IEventListener objects that receive IInputEvent notifications.
     *
     * Responsibilities:
     * - Axis values: getAxisValue() / getAxisValueRaw() return either values injected through
     *   setAxisValue() (when the override flag is set) or values derived from the channel map.
     * - Listeners: addListener(), removeListener() and triggerEvent( SmartPtr<IInputEvent> )
     *   dispatch input events to registered listeners.
     * - Configuration: getProperties() / setProperties() expose the tunable state (timings,
     *   flags, adapter mode, button counts, channel settings) for data-driven configuration.
     *
     * Thread safety:
     * - Frequently accessed scalar state (times, status, button masks, dongle flags) is stored in
     *   atomic types.
     * - The listener list, the channel arrays and the event queues are concurrent containers.
     *   Compound operations (for example add/remove through getInputListeners() and
     *   setInputListeners()) are not atomic as a whole.
     * - Plain members (mouse button flags, assigning/override flags, the external channel map)
     *   are not synchronized and are expected to be used from the primary task.
     *
     * Error handling:
     * - Public methods validate their arguments, log through WP_LOG_ERROR / WP_LOG_WARNING and
     *   return a safe default instead of throwing.
     *
     * @note Device-specific parts (keyboard, mouse and joystick creation, adapter/dongle
     *       communication, firmware update, key mapping) are placeholders in this implementation
     *       and do nothing; each member documents its actual behaviour. Platform backends are
     *       expected to feed normalized values through setAxisValue() and events through
     *       triggerEvent().
     * @note play(), record(), stop(), getFlags() and setFlags() of IInputManager are not
     *       overridden by this class.
     * @see IInputManager, Properties, AxisData, IInputEvent
     */
    class WPCore_API InputManagerExt : public IInputManager
    {
    public:
        /**
         * @class StartDongleJob
         * @brief Job that starts the hardware adapter ("dongle") connection.
         *
         * @details
         * When executed it calls InputManagerExt::startDongle() on the owning manager, so the
         * start-up can be run from a job queue instead of the calling thread.
         *
         * @note The owning manager does not queue this job at the moment; updateDongle() calls
         *       startDongle() directly.
         */
        class StartDongleJob : public Job
        {
        public:
            /**
             * @brief Constructs a StartDongleJob for the given input manager.
             * @param inputManager Input manager whose dongle should be started. Must stay valid
             *        until the job has been executed.
             */
            StartDongleJob( InputManagerExt *inputManager );

            /** @brief Destructor. Releases the reference to the input manager. */
            ~StartDongleJob() override;

            /** @brief Calls InputManagerExt::startDongle() on the owning input manager. */
            void execute() override;

            RawPtr<InputManagerExt> m_inputManager;  ///< Input manager the job operates on.
        };

        /**
         * @class CreateJobstickJob
         * @brief Coroutine job that asks the input manager to create the joystick device.
         *
         * @details
         * Each coroutine step calls InputManagerExt::createJoystick() once. The plain
         * execute() entry point does nothing.
         *
         * @note m_retries is initialized to 0 but no retry logic uses it yet.
         */
        class CreateJobstickJob : public Job
        {
        public:
            /**
             * @brief Constructs a CreateJobstickJob for the given input manager.
             * @param inputManager Input manager that will create the joystick. Must stay valid
             *        while the job runs.
             */
            CreateJobstickJob( RawPtr<InputManagerExt> inputManager );

            /** @brief Destructor. */
            ~CreateJobstickJob() override;

            /** @brief Intentionally empty; the work is done in coroutine_execute_step(). */
            void execute() override;

            /**
             * @brief Calls InputManagerExt::createJoystick() on the owning input manager.
             * @param rYield Coroutine data supplied by the scheduler (not used).
             */
            void coroutine_execute_step( SmartPtr<ICoroutineData> &rYield ) override;

            RawPtr<InputManagerExt> m_inputManager;  ///< Input manager the job operates on.
            s32 m_retries;                           ///< Retry counter, initialized to 0 (currently unused).
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
         * @brief Default constructor. Equivalent to InputManagerExt( nullptr, true, true ).
         *
         * @details
         * See InputManagerExt( render::IGraphicsWindow *, bool, bool ) for the initial state.
         *
         * @note Call load() after construction to finish initialization.
         */
        InputManagerExt();

        /**
         * @brief Constructs the input manager with the given window and buffering settings.
         *
         * @param window Graphics window for input capture. Currently unused.
         * @param bufferedKeys Initial value of the buffered keyboard flag, see getBufferedKeys().
         * @param bufferedMouse Initial value of the buffered mouse flag, see getBufferedMouse().
         *
         * @details
         * Initial state:
         * - Keyboard input enabled, mouse capture (internal input capture) enabled.
         * - Double-click interval of 0.25 seconds; all other times, flags, buttons and status
         *   cleared to zero/false.
         * - Current Tx model set to "heli".
         * - Channel data pool created through createChannelData() (NUM_CHANNELS AxisData
         *   entries with offset 0 and multipliers 1).
         *
         * @note Call load() after construction to finish initialization.
         */
        InputManagerExt( render::IGraphicsWindow *window, bool bufferedKeys, bool bufferedMouse );

        /**
         * @brief Destructor. Calls unload( 0 ).
         *
         * @details
         * Stops the dongle, clears the listeners and the channel data/map and marks the
         * manager as unloaded.
         */
        ~InputManagerExt() override;

        /**
         * @brief Prepares the manager for use.
         *
         * @details
         * Does nothing if the manager is already loaded. Otherwise it sets the loading state to
         * Loading, clears the dongle/Tx OK flags, zeroes the external channel input of channels
         * 0..11, disables input logging and log playback, creates the recorder listener and
         * initializes the dongle ("load"). On success the loading state becomes Loaded.
         *
         * @note Must be called after construction; preUpdate() ignores the manager until it is
         *       Loaded.
         * @note Exceptions are logged and not rethrown; the loading state is then left at
         *       Loading.
         */
        void load();

        /**
         * @brief Unloads the manager and releases its resources.
         *
         * @param flags Reserved; currently unused.
         *
         * @details
         * Stops the dongle, removes all listeners, clears the channel data and channel map and
         * sets the loading state to Unloaded. Safe to call multiple times.
         *
         * @note Called by the destructor.
         */
        void unload( u32 flags );

        /**
         * @brief Per-frame entry point of the input manager.
         *
         * @param task Task the update is running on.
         * @param t Current time in seconds.
         * @param dt Time since the previous update in seconds.
         *
         * @details
         * Does nothing unless the manager is Loaded. For TaskId::Primary it calls updateState();
         * other tasks (Physics, Controls, ...) are ignored. Exceptions are logged and not
         * rethrown.
         */
        void preUpdate( TaskId task, const double &t, const double &dt );

        /**
         * @brief Updates the manager state for the current frame.
         *
         * @param task Task the update is running on.
         * @param t Current time in seconds.
         * @param dt Time since the previous update in seconds.
         *
         * @details
         * Re-arms the next dongle check time (t + 5 seconds) when it has expired and forwards
         * the update to the input configuration (InputConfiguration::preUpdate()) if one is set.
         *
         * @note Called by preUpdate(); the event queue processing, input log playback and
         *       dongle/joystick polling are currently disabled.
         */
        void updateState( TaskId task, const double &t, const double &dt );

        /**
         * @brief Copies the axis/button state into the player input state.
         *
         * @note Currently a no-op (the implementation is disabled) and not called by
         *       updateState().
         */
        void updateInputState();

        /**
         * @brief Periodically checks the hardware adapter ("dongle") and drives firmware updates.
         *
         * @param t Current time in seconds.
         * @param dt Time since the previous update in seconds.
         *
         * @details
         * When the next check time has passed and no firmware update is running, it calls
         * startDongle() if the dongle is not OK and schedules the next check one second later.
         * While a firmware update is in progress (update state past UpdateIdle) it also calls
         * the protected updateFirmware( t, dt ).
         *
         * @note startDongle() and updateFirmware( t, dt ) are currently no-ops.
         */
        void updateDongle( const double &t, const double &dt );

        /**
         * @brief Registers a listener that receives input events.
         *
         * @param listener Listener to add. Must not be null.
         *
         * @details
         * The listener is appended to the listener list and is notified by
         * triggerEvent( SmartPtr<IInputEvent> ). A listener that is already registered is not
         * added again (a warning is logged). A null listener is rejected with an error log.
         *
         * @note The list is copied, modified and written back, so concurrent add/remove calls
         *       can race with each other.
         * @see removeListener(), removeListeners()
         */
        void addListener( SmartPtr<IEventListener> listener );

        /**
         * @brief Unregisters a listener.
         *
         * @param listener Listener to remove. Must not be null.
         *
         * @details
         * Removes every occurrence of the listener. A warning is logged if it was not
         * registered; a null listener is rejected with an error log.
         *
         * @see addListener(), removeListeners()
         */
        void removeListener( SmartPtr<IEventListener> listener );

        /**
         * @brief Removes all registered listeners.
         *
         * @details
         * Useful during shutdown or scene transitions. Also done by unload().
         *
         * @see addListener(), removeListener()
         */
        void removeListeners();

        /**
         * @brief Stores an externally supplied value for an input channel.
         *
         * @param channel Channel index in [0, NUM_CHANNELS).
         * @param value Normalized channel value, typically in [-1.0, 1.0]. Must be finite.
         *
         * @details
         * Out-of-range channels and NaN/infinite values are rejected with an error log and
         * leave the stored value unchanged. The value is always stored, but getAxisValue() and
         * getAxisValueRaw() only return it while the override flag is set.
         *
         * @note The external channel map is not synchronized; call from the primary task.
         * @see IInputManager::setAxisValue(), getAxisValue(), getAxisValueRaw(), setUseOverride()
         */
        void setAxisValue( s32 channel, f32 value ) override;

        /**
         * @brief Gets the processed value of an input channel.
         *
         * @param channel Channel index.
         * @return With the override flag set, the value stored by setAxisValue() (0 if none was
         *         stored). Otherwise, when the dongle is OK and the adapter is not a USB device,
         *         the value of channels 0..7 computed from the channel map entry:
         *         ( offset ) * multiplier, then scaled by the low multiplier when negative or the
         *         high multiplier otherwise. A "Gyro" function whose device map is -1 returns
         *         getGyroOverride() instead. In all other cases 0.0.
         * @see IInputManager::getAxisValue(), setAxisValue()
         */
        f32 getAxisValue( s32 channel ) override;

        /**
         * @brief Gets the unprocessed value of an input channel.
         *
         * @param channel Channel index.
         * @return With the override flag set, the value stored by setAxisValue() (0 if none was
         *         stored). Otherwise 0.0, as no device supplies raw values in this
         *         implementation.
         * @see IInputManager::getAxisValueRaw(), setAxisValue()
         */
        f32 getAxisValueRaw( s32 channel ) override;

        /**
         * @brief Sets the size of the window used for mouse coordinates.
         * @param width Window width in pixels.
         * @param height Window height in pixels.
         * @note Currently a no-op.
         */
        void setWindowExtents( s32 width, s32 height );

        /**
         * @brief Sends a reset event to the listeners.
         *
         * @note Must be called from the primary task. The event creation and the dispatch to
         *       the listeners are currently disabled, so this has no observable effect.
         */
        void reset();

        /** @brief Re-initializes the dongle when the vehicle type changed. Currently a no-op. */
        void resetDongle();

        /**
         * @brief Checks whether keyboard input is enabled.
         * @return True if keyboard input is enabled.
         */
        bool getKeyboardInputEnabled() const;

        /**
         * @brief Enables or disables keyboard input.
         * @param keyboardInputEnabled True to enable keyboard input.
         */
        void setKeyboardInputEnabled( bool keyboardInputEnabled );

        /**
         * @brief Gets the button id of the last mouse event.
         * @return Button id; 0 until it is set (it is not updated by this class).
         */
        s32 getBtnID();

        /**
         * @brief Initializes the dongle for the given purpose.
         * @param param Purpose of the call, e.g. "load" or "setTxData".
         * @note Currently a no-op.
         */
        void initDongle( const String &param );

        /**
         * @brief Reads the adapter signature.
         * @param database Database manager (unused).
         * @deprecated Only logs a warning; configure through Properties instead.
         */
        void getSignature( SmartPtr<IDatabaseManager> database );

        /**
         * @brief Reads the adapter chip id.
         * @param hex Output buffer for the id. Must not be null (an error is logged otherwise);
         *        it is not written to.
         * @param database Database manager (unused).
         * @deprecated Only logs a warning; the buffer is left untouched.
         */
        void getChipId( char *hex, SmartPtr<IDatabaseManager> database );

        /** @brief Resets the dongle authentication state. Currently a no-op. */
        void resetDongleAuthenticate();

        /** @brief Requests the dongle firmware version. Currently a no-op. */
        void getDongleVersion();

        /**
         * @brief Sets up receiver binding on the adapter.
         * @param param Binding parameter, e.g. the bind mode.
         * @note Currently a no-op.
         */
        void setupBind( const String &param );

        /**
         * @brief Loads the adapter mode from the "adapter_mode" row of the settings database.
         *
         * @details
         * Updates the adapter mode string and its hash when the database returns a non-empty
         * value. Without a database the mode defaults to "gamepad" if none is set yet. In every
         * other failure case the current mode is kept; warnings are logged.
         */
        void setupAdapterModeValue();

        /**
         * @brief Sets up the channel map for the given model.
         * @param param Model or map identifier.
         * @note Currently a no-op.
         */
        void setupChannelMap( const String &param );

        /**
         * @brief Derives the vehicle type from a model parameter string.
         *
         * @param param Non-empty string such as a model name.
         * @return "heli", "plane", "drone", "car" or "truck" when the string contains the
         *         capitalized or lower-case type name (checked in that order); "heli" otherwise.
         */
        String getModelTypeFromParam( const String &param );

        /** @brief Authenticates the adapter. Currently a no-op. */
        void auth();

        /**
         * @brief Passes an event object to the registered listeners.
         *
         * @param inputEvent Event to dispatch.
         *
         * @note Dispatch is currently disabled; the call only walks the listener list (and
         *       reads the input log flag), so it has no observable effect. Use the
         *       triggerEvent( SmartPtr<IInputEvent> ) overload to deliver events.
         */
        void triggerEvent( SmartPtr<IEventListener> inputEvent );

        /** @brief Checks whether the shift modifier is pressed. @return True if shift is pressed. */
        bool isShiftPressed() const;

        /** @brief Sets the shift modifier state. @param shiftPressed True if shift is pressed. */
        void setShiftPressed( bool shiftPressed );

        /** @brief Checks the adapter mode. @return Always false in this implementation. */
        bool checkAdaptorMode( void );

        /**
         * @brief Gets the adapter/Tx status as XML switch markup.
         * @return Markup describing the adapter and Tx switches, or an empty string unless the
         *         ACCURX_NEW_VERSION build option is enabled (where status codes 0xf0..0xf3
         *         are translated; 0xf0 and 0xf1 also clear the dongle OK flag).
         */
        String adaptorTxStatus();

        /** @brief Gets the raw adapter/Tx status code. @return Same value as getStatus(). */
        s32 adaptorTxStatusInt();

        /** @brief Gets the time of the last input. @return Time in seconds. */
        double getLastInputTime() const;

        /** @brief Sets the time of the last input. @param lastInputTime Time in seconds. */
        void setLastInputTime( double lastInputTime );

        /**
         * @brief Checks whether externally injected axis values are used.
         * @return True if getAxisValue() / getAxisValueRaw() return values stored with
         *         setAxisValue().
         */
        bool getUseOverride() const;

        /**
         * @brief Enables or disables the use of externally injected axis values.
         * @param useOverride True to return the values stored with setAxisValue().
         */
        void setUseOverride( bool useOverride );

        /**
         * @brief Sets calibration for a single channel.
         * @param channel Channel index.
         * @param offset Offset added to the channel value.
         * @param multiplier Scale applied to the channel value.
         * @note Currently a no-op; use the channel data accessors to change AxisData directly.
         */
        void setChannelData( s32 channel, f32 offset, f32 multiplier );

        /**
         * @brief Starts a firmware update of the adapter.
         * @note Sets the update state to StartUpdate (except in unity builds); the update state
         *       machine itself is currently disabled, see updateDongle().
         */
        void updateFirmware( void );

        /**
         * @brief Checks whether input events are being logged.
         * @return True if input logging is enabled; always false in unity builds
         *         (WP_UNITY_BUILD).
         */
        bool getEnableInputLog() const;

        /** @brief Enables or disables input event logging. @param enableInputLog True to log. */
        void setEnableInputLog( bool enableInputLog );

        /** @brief Gets the input log clock. @return Accumulated input time in seconds. */
        double getInputTime() const;

        /** @brief Sets the input log clock. @param inputTime New input time in seconds. */
        void setInputTime( double inputTime );

        /** @brief Advances the input log clock. @param inputTime Seconds to add. */
        void addInputTime( double inputTime );

        /** @brief Checks whether a recorded input log is being played back. @return True if so. */
        bool getRunInputLog() const;

        /** @brief Starts or stops input log playback. @param runInputLog True to play back. */
        void setRunInputLog( bool runInputLog );

        /**
         * @brief Sorts the listeners by priority.
         *
         * @note The sort is applied to a copy of the listener list, so the stored order is not
         *       changed.
         */
        void updateListenersPriority();

        /**
         * @brief Sends a keyboard event.
         * @param evnt Event type.
         * @param keycode Key code.
         * @note Currently a no-op.
         */
        void sendKeyEvent( IInputEvent::EventType evnt, s32 keycode );

        /** @brief Checks the left mouse button. @return True if it is pressed. */
        bool isLeftPressed() const;

        /** @brief Sets the left mouse button state. @param leftPressed True if pressed. */
        void setLeftPressed( bool leftPressed );

        /** @brief Checks the right mouse button. @return True if it is pressed. */
        bool isRightPressed() const;

        /** @brief Sets the right mouse button state. @param rightPressed True if pressed. */
        void setRightPressed( bool rightPressed );

        /** @brief Checks the middle mouse button. @return True if it is pressed. */
        bool isMiddlePressed() const;

        /** @brief Sets the middle mouse button state. @param middlePressed True if pressed. */
        void setMiddlePressed( bool middlePressed );

        /** @brief Gets the time of the last mouse click. @return Time in seconds. */
        double getLastClickTime() const;

        /** @brief Sets the time of the last mouse click. @param lastClickTime Time in seconds. */
        void setLastClickTime( double lastClickTime );

        /** @brief Gets the maximum time between two clicks of a double click. @return Seconds. */
        double getDoubleClickInterval() const;

        /**
         * @brief Sets the maximum time between two clicks of a double click.
         * @param doubleClickInterval Interval in seconds.
         */
        void setDoubleClickInterval( double doubleClickInterval );

        /** @brief Checks whether keyboard input is buffered. @return True if buffered. */
        bool getBufferedKeys() const;

        /** @brief Sets whether keyboard input is buffered. @param bufferedKeys True to buffer. */
        void setBufferedKeys( bool bufferedKeys );

        /** @brief Checks whether mouse input is buffered. @return True if buffered. */
        bool getBufferedMouse() const;

        /** @brief Sets whether mouse input is buffered. @param bufferedMouse True to buffer. */
        void setBufferedMouse( bool bufferedMouse );

        /** @brief Checks whether the manager captures input itself. @return True if enabled. */
        bool getEnableInternalInputCapture() const;

        /**
         * @brief Enables or disables internal input capture.
         * @param enableInternalInputCapture True to let the manager capture input itself.
         */
        void setEnableInternalInputCapture( bool enableInternalInputCapture );

        /**
         * @brief Gets a snapshot of the registered listeners.
         * @return Copy of the listener list; later changes are not reflected in it.
         */
        Array<SmartPtr<IEventListener>> getInputListeners() const;

        /**
         * @brief Replaces the registered listeners.
         * @param p New listener list.
         */
        void setInputListeners( const Array<SmartPtr<IEventListener>> &p );

        /**
         * @brief Gets the display name of a key.
         * @param kc Key code.
         * @return Empty string; keyboard lookup is currently not implemented.
         */
        String getAsString( s32 kc );

        /**
         * @brief Checks whether a key is held down.
         * @param kc Key code.
         * @return Always false; keyboard polling is currently not implemented.
         */
        bool isKeyDown( s32 kc );

        /**
         * @brief Persists the Tx channel data to the database.
         * @deprecated Only logs a warning; use getChannelData() with Properties or custom
         *             serialization instead.
         */
        void TxChannels2db();

        /** @brief Starts monitoring the Tx channels. Currently a no-op. */
        void StartChannelMonitor();

        /** @brief Stops monitoring the Tx channels. Currently a no-op. */
        void EndChannelMonitor();

        /**
         * @brief Queues an input event for later processing.
         * @param inputEvent Event to queue.
         * @note The event is pushed onto the internal thread-safe event queue; nothing in the
         *       manager consumes that queue at the moment.
         */
        void queueEvent( SmartPtr<IInputEvent> inputEvent );

        /** @brief Gets the adapter/Tx status code (0xf0..0xf3 when reported by the adapter). */
        s32 getStatus() const;

        /** @brief Sets the adapter/Tx status code. @param status New status code. */
        void setStatus( s32 status );

        /** @brief Called when a joystick connected; destroys the current joystick object. */
        void joystickConnected();

        /** @brief Called before a joystick disconnects. Currently a no-op. */
        void joystickPreDisconnect();

        /** @brief Called after a joystick disconnected. Currently a no-op. */
        void joystickDisconnected();

        /**
         * @brief Gets the pool of channel data.
         * @return Copy of the NUM_CHANNELS AxisData entries created at construction.
         */
        ConcurrentArray<SmartPtr<AxisData>> getChannelData() const;

        /** @brief Replaces the pool of channel data. @param channelData New channel data. */
        void setChannelData( const ConcurrentArray<SmartPtr<AxisData>> &channelData );

        /**
         * @brief Gets the channel mapping data used by getAxisValue().
         * @return Copy of the channel map; empty until set with setChannelDataMap().
         */
        ConcurrentArray<SmartPtr<AxisData>> getChannelDataMap() const;

        /**
         * @brief Replaces the channel mapping data.
         * @param channelDataMap New channel map. Its size is logged.
         */
        void setChannelDataMap( const ConcurrentArray<SmartPtr<AxisData>> &channelDataMap );

        /** @brief Gets the input configuration. @return Configuration, or null if none is set. */
        SmartPtr<InputConfiguration> getInputConfiguration() const;

        /**
         * @brief Sets the input configuration that is updated by updateState().
         * @param inputConfiguration New configuration; may be null.
         */
        void setInputConfiguration( SmartPtr<InputConfiguration> inputConfiguration );

        /**
         * @brief Gets an entry of the channel map by index.
         * @param index Index into the channel map (see getChannelDataMap()).
         * @return The entry, or a reference to a null pointer when the index is negative or
         *         out of range (an error or warning is logged).
         */
        SmartPtr<AxisData> &getChannelDataByIndex( s32 index );

        /** @brief Const overload of getChannelDataByIndex( s32 ); same behaviour. */
        const SmartPtr<AxisData> &getChannelDataByIndex( s32 index ) const;

        /** @brief Checks whether a binding is being assigned. @return True while assigning. */
        bool isAssigning() const;

        /** @brief Sets the binding assignment state. @param assigning True while assigning. */
        void setAssigning( bool assigning );

        /** @brief Gets the current Tx model type, e.g. "heli". @return Model type string. */
        String getCurrentTxModel() const;

        /** @brief Sets the current Tx model type. @param currentTxModel Model type string. */
        void setCurrentTxModel( const String &currentTxModel );

        /** @brief Gets the current button state. @return Bitmask, one bit per button. */
        u32 getCurrentButtons() const;

        /** @brief Sets the current button state. @param currentButtons Bitmask of pressed buttons. */
        void setCurrentButtons( u32 currentButtons );

        /** @brief Gets the button state of the previous frame. @return Bitmask. */
        u32 getPreviousButtons() const;

        /** @brief Sets the previous button state. @param previousButtons Bitmask of buttons. */
        void setPreviousButtons( u32 previousButtons );

        /** @brief Gets the number of buttons of the device. @return Button count. */
        s32 getNumButtons() const;

        /** @brief Sets the number of buttons of the device. @param numButtons Button count. */
        void setNumButtons( s32 numButtons );

        /** @brief Checks whether the dongle was initialized successfully. @return True if OK. */
        bool isDongleOk() const;

        /** @brief Sets the dongle OK flag. @param dongleOk True if the dongle is OK. */
        void setDongleOk( bool dongleOk );

        /** @brief Gets the value returned for the gyro channel. @return Gyro value in [-1, 1]. */
        f32 getGyroOverride() const;

        /** @brief Sets the value returned for the gyro channel. @param gyroOverride New value. */
        void setGyroOverride( f32 gyroOverride );

        /**
         * @brief Gets the raw data buffer of the adapter.
         * @return Zero-filled 65-byte array; adapter communication is not implemented.
         */
        std::array<unsigned char, 65> getDataArray();

        /**
         * @brief Checks whether the adapter mode is a USB device mode.
         * @return True for the "interlink", "vbar", "gamepad" and "jeti_usb" adapter modes.
         */
        bool isDeviceUSB() const;

        /**
         * @brief Adds a game input device.
         * @param id Identifier of the game input.
         * @return Always null; game inputs are not supported by this class.
         */
        SmartPtr<IGameInput> addGameInput( hash32 id );

        /**
         * @brief Finds a game input device.
         * @param id Identifier of the game input.
         * @return Always null; game inputs are not supported by this class.
         */
        SmartPtr<IGameInput> findGameInput( hash32 id ) const;

        /**
         * @brief Gets all game input devices.
         * @return Always an empty array.
         */
        Array<SmartPtr<IGameInput>> getGameInputs() const;

        /**
         * @copydoc IInputManager::isCursorVisible
         * @note This implementation always returns false.
         */
        bool isCursorVisible() const;

        /**
         * @copydoc IInputManager::setCursorVisible
         * @note This implementation ignores the request.
         */
        void setCursorVisible( bool visible );

        /**
         * @brief Posts an input event.
         * @param event Event to post.
         * @return Always false; the event is not processed.
         */
        bool postEvent( SmartPtr<IInputEvent> event );

        /**
         * @brief Delivers an input event immediately to all listeners.
         *
         * @param inputEvent Event to deliver. Null events are rejected with an error log.
         *
         * @details
         * Calls IEventListener::handleEvent() with EventType::Input on every registered
         * listener, in list order, passing the event. Null listeners are skipped with a warning.
         * An exception thrown by one listener is logged and does not stop delivery to the
         * others. Returns silently when no listener is registered.
         */
        void triggerEvent( SmartPtr<IInputEvent> inputEvent );

        /**
         * @brief Gets the configurable state of the input manager as properties.
         * @return Properties object containing the base class properties plus this class's own.
         *         An empty Properties object is returned if an exception occurs.
         *
         * @details
         * Properties added by this class:
         * - Timing: doubleClickInterval, inputTime, lastClickTime, lastInputTime,
         *   nextJoystickCheck
         * - Flags: keyboardInputEnabled, dongleOk, txOk, isAssigning, isShiftPressed,
         *   useOverride, enableInputLog, runInputLog, bufferedKeys, bufferedMouse,
         *   enableInternalInputCapture
         * - Buttons: numButtons, currentButtons, previousButtons
         * - Device: adapterMode, currentTxMode, status
         * - Channels: jsThrottleChannel, gyroOverride
         * - Read-only (diagnostic): isLeftPressed, isRightPressed, isMiddlePressed, updateState,
         *   inputIndex
         *
         * @note Starts from IInputManager::getProperties() and adds the properties above.
         * @see setProperties()
         */
        SmartPtr<Properties> getProperties() const override;

        /**
         * @brief Applies property values to the input manager.
         * @param properties Properties to read. Null is rejected with an error log.
         *
         * @details
         * Calls IInputManager::setProperties() first, then reads the properties listed for
         * getProperties(). Properties that are missing keep their current value. Values with
         * a restricted range are validated; invalid values are logged and ignored while the
         * remaining properties are still processed:
         * - doubleClickInterval in [0.0, 2.0] seconds
         * - inputTime >= 0.0
         * - numButtons in [0, 128]
         * - adapterMode must not be empty (the adapter mode hash is updated with it)
         * - jsThrottleChannel in [0, NUM_CHANNELS)
         * - gyroOverride in [-1.0, 1.0]
         *
         * @note The read-only properties (mouse buttons, updateState, inputIndex) are not
         *       applied.
         * @see getProperties()
         */
        void setProperties( SmartPtr<Properties> properties ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /** @brief Creates the listener that records input for the input log. Currently a no-op. */
        void createRecorderListener();

        /**
         * @brief Creates the pool of channel data.
         *
         * @details
         * Fills the pool with NUM_CHANNELS AxisData objects (offset 0, all multipliers 1) made
         * by the factory manager and clears the channel map. Logs an error and returns if the
         * application or factory manager is unavailable; channels that cannot be created are
         * skipped.
         */
        void createChannelData();

        /** @brief Creates the keyboard device. Currently a no-op. */
        void createKeyboard();

        /** @brief Creates the mouse device. Currently a no-op. */
        void createMouse();

        /** @brief Starts the dongle connection. Currently a no-op. */
        void startDongle();

        /** @brief Stops the dongle connection. Currently a no-op. */
        void stopDongle();

        /**
         * @brief Advances the firmware update state machine.
         * @param t Current time in seconds.
         * @param dt Time since the previous update in seconds.
         * @note Currently a no-op.
         */
        void updateFirmware( const double &t, const double &dt );

        /**
         * @brief Creates an event populated with the current mouse state.
         * @param eventType Event type to set.
         * @return Currently always null, as mouse polling is not implemented.
         */
        SmartPtr<InputEvent> getMouseEvent( u32 eventType );

        /**
         * @brief Creates a reset event populated with the current mouse state.
         * @param eventType Event type to set.
         * @return Currently always null, as mouse polling is not implemented.
         */
        SmartPtr<InputEvent> getResetEvent( u32 eventType );

        /**
         * @brief Converts a character to a keyboard key code.
         * @param c Character such as 'D'.
         * @return Key code, currently always 0 as the mapping is disabled.
         */
        s32 getKeyCode( char c );

        /**
         * @brief Loads the button-to-key mapping from the settings database.
         *
         * @param type Prefix of the settings parameters to read. Must not be empty.
         *
         * @details
         * Reads rows whose param starts with "type" and fills m_buttonsON / m_buttonsOFF (7
         * entries each) from params ending in B1, B2, B3, B4, B5, B27 and B29 followed by "on"
         * or "off". The first character of the value is converted with getKeyCode(). Without a
         * database, or on error, both arrays are set to 7 zeros.
         */
        void setupButtons( String type );

        /** @brief Creates the joystick device. Currently a no-op. */
        void createJoystick();

        /** @brief Destroys the joystick device. Currently a no-op. */
        void destroyJoystick();

        /** @brief Polls the joystick. Currently a no-op; backends supply values via setAxisValue(). */
        void updateJoystick();

        /**
         * @brief Evaluates an input function against the current axis/button state.
         * @param func Function to evaluate. May be null.
         * @return True if the function was triggered by the current input.
         */
        bool isFunction( SmartPtr<InputFunction> &func );

        /** @brief Updates the idle-up, normal-mode and throttle-hold functions. Currently a no-op. */
        void UpdateMiscFunctions();

        /** @brief Sets up the Tx function bindings. Currently a no-op. */
        void setupTxFunctions();

        /** @brief Checks the dongle connection. @return True if the dongle is OK (isDongleOk()). */
        bool isDongleConnected();

        /** @brief Gets the connected devices. @return Always an empty array. */
        Array<String> getConnectedDevices() const;

        /** @brief Gets the hash of the adapter mode. @return Hash of m_adapterMode. */
        hash_type getAdapterModeHash() const;

        ConcurrentQueue<SmartPtr<IInputEvent>> m_inputEvents;  ///< Queue of recorded input events (not used by this class).

        SmartPtr<ITimer> m_timer;  ///< Timer used to measure listener time.

        SmartPtr<InputConfiguration> m_inputConfiguration;  ///< Input configuration updated by updateState().

        // std::array<AtomicD, 32> m_rawChannels;
        atomic_f64 m_dongleCheckTime;      ///< Next time updateDongle() checks the dongle, in seconds.
        atomic_f64 m_lastInputTime;        ///< Time of the last input, in seconds.
        atomic_f64 m_inputTime;            ///< Input log clock, in seconds.
        atomic_f64 m_lastClickTime;        ///< Time of the last mouse click, in seconds.
        atomic_f64 m_doubleClickInterval;  ///< Maximum time between double-click clicks, in seconds (default 0.25).
        atomic_f64 m_nextDongleCheckTime;  ///< Time at which updateState() re-arms the dongle check, in seconds.

        u32 m_inputIndex;  ///< Index of the next recorded event to play back.

        s32 m_btnID;  ///< Button id of the last mouse event.

        /// True once the dongle/Tx initialization succeeded.
        atomic_bool m_dongleOk;
        atomic_bool m_txOk;  ///< True if the Tx (transmitter) is connected and OK.

        /// True if keyboard input is enabled.
        atomic_bool m_keyboardInputEnabled;

        atomic_s32 m_status;  ///< Adapter/Tx status code.

        bool m_isAssigning;  ///< True while a binding is being assigned.
        bool m_isShiftPressed;  ///< True while the shift modifier is pressed.
        bool m_bUseOverride;  ///< True if axis queries return the values set with setAxisValue().
        bool m_bEnableInputLog;  ///< True if input events are logged.
        bool m_bRunInputLog;  ///< True if a recorded input log is played back.

        bool m_bufferedKeys;  ///< True if keyboard input is buffered.
        bool m_bufferedMouse;  ///< True if mouse input is buffered.

        bool m_enableInternalInputCapture;  ///< True if the manager captures input itself.

        /// True while the left mouse button is pressed.
        bool m_isLeftPressed;

        /// True while the right mouse button is pressed.
        bool m_isRightPressed;

        /// True while the middle mouse button is pressed.
        bool m_isMiddlePressed;

        s32 m_numButtons;  ///< Number of buttons of the device.

        s32 m_updateState;  ///< Firmware update state, a value of updateStates.
        String m_msg;  ///< Status message of the firmware update.
        // atomic_bool m_bUseJoyStick;
        atomic_u32 m_previousButtons;  ///< Button bitmask of the previous frame.
        atomic_u32 m_currentButtons;  ///< Current button bitmask.
        s32 m_jsThrottleChannel;  ///< Channel index used for the joystick throttle.

        SmartPtr<InputFunction> m_idleUpFunction;  ///< Function bound to the idle-up switch.
        SmartPtr<InputFunction> m_normalModeFunction;  ///< Function bound to the normal-mode switch.
        SmartPtr<InputFunction> m_throttleHoldFunction;  ///< Function bound to the throttle-hold switch.

        Array<s32> m_buttonsON;  ///< Key codes mapped to the button "on" states, see setupButtons().
        Array<s32> m_buttonsOFF;  ///< Key codes mapped to the button "off" states, see setupButtons().

        /// Thread-safe queue filled by queueEvent().
        ConcurrentQueue<SmartPtr<IInputEvent>> m_inputEventQueue;

        /// Alias for a map from channel index to channel data (currently unused).
        using ChannelDataMap = std::unordered_map<s32, SmartPtr<AxisData>>;

        /// Pool of channel data, created by createChannelData().
        ConcurrentArray<SmartPtr<AxisData>> m_channelData;

        /// Channel mapping data used by getAxisValue(), see setChannelDataMap().
        ConcurrentArray<SmartPtr<AxisData>> m_channelDataMap;

        /// Registered input listeners.
        ConcurrentArray<SmartPtr<IEventListener>> m_inputListeners;

        /// Values set with setAxisValue(), by channel; returned while the override flag is set.
        std::unordered_map<s32, f64> m_externalChannelInput;

        /// Current Tx model type (e.g. "heli"), see getCurrentTxModel().
        String m_currentTxMode;

        /// Adapter or controller mode, e.g. "gamepad" or "interlink".
        String m_adapterMode;

        /// Hash of m_adapterMode for quick comparison.
        hash_type m_adapterModeHash;

        /// Next time to check for a joystick, in seconds.
        f64 m_nextJoystickCheck;

        /// Value returned for the gyro channel when its device map is -1.
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
