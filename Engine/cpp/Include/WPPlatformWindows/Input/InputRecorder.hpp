#ifndef InputRecorder_h__
#define InputRecorder_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Input/InputManagerExt.hpp>

namespace workphone
{
    /**
     * @class InputRecorder
     * @brief A comprehensive production tool for recording and playing back input events.
     *
     * @details
     * This class provides a robust system for capturing, storing, and replaying input events
     * in a game or application. It supports recording to memory and file, playback with timing
     * control, and is fully integrated with the game editor through property exposure.
     *
     * Features:
     * - Record input events with timestamps
     * - Play back recorded events with accurate timing
     * - Save/load recordings to/from files
     * - Pause and resume during playback
     * - Loop playback mode
     * - Maximum event limiting to prevent memory overflow
     * - Full error handling and logging
     * - Editor-exposed properties for configuration
     *
     * @note All operations are defensive and will log errors rather than crash on invalid input.
     */
    class WPCore_API InputRecorder : public ISharedObject
    {
    public:
        /**
         * @brief Enumeration of recorder states.
         */
        enum class State
        {
            Idle,       ///< Not recording or playing.
            Recording,  ///< Currently recording input events.
            Playing,    ///< Currently playing back recorded events.
            Paused      ///< Playback is paused.
        };

        /**
         * @brief Constructs an InputRecorder with a reference to the input manager.
         * @param inputManager Pointer to the input manager (can be nullptr, will be validated).
         */
        explicit InputRecorder( InputManagerExt *inputManager );

        /**
         * @brief Destructor. Ensures all resources are cleaned up safely.
         */
        ~InputRecorder() override;

        /**
         * @brief Updates the recorder state, handling playback timing and state transitions.
         * @param task The task identifier for the update context.
         * @param t Current time in seconds.
         * @param dt Delta time since last update in seconds.
         */
        void update( TaskId task, const double &t, const double &dt );

        /**
         * @brief Starts playing back recorded events.
         * @return true if playback started successfully, false otherwise.
         * @note Will log an error if no events are recorded or already playing.
         */
        bool play();

        /**
         * @brief Starts recording input events.
         * @return true if recording started successfully, false otherwise.
         * @note Will stop any current playback and clear existing events.
         */
        bool record();

        /**
         * @brief Stops recording or playback and returns to idle state.
         */
        void stop();

        /**
         * @brief Pauses playback. Has no effect during recording or when idle.
         * @return true if successfully paused, false otherwise.
         */
        bool pause();

        /**
         * @brief Resumes playback from paused state.
         * @return true if successfully resumed, false otherwise.
         */
        bool resume();

        /**
         * @brief Clears all recorded events and resets playback state.
         */
        void clear();

        /**
         * @brief Saves recorded events to a file.
         * @param filepath Path to the file where events will be saved.
         * @return true if save was successful, false otherwise.
         * @note Will log errors if file cannot be created or written.
         */
        bool saveToFile( const String &filepath );

        /**
         * @brief Loads recorded events from a file.
         * @param filepath Path to the file to load.
         * @return true if load was successful, false otherwise.
         * @note Will clear existing events before loading. Logs errors on failure.
         */
        bool loadFromFile( const String &filepath );

        /**
         * @brief Gets the current state of the recorder.
         * @return Current State enumeration value.
         */
        State getState() const;

        /**
         * @brief Gets the number of currently recorded events.
         * @return Count of recorded events.
         */
        size_t getEventCount() const;

        /**
         * @brief Checks if the recorder is currently recording.
         * @return true if recording, false otherwise.
         */
        bool isRecording() const;

        /**
         * @brief Checks if the recorder is currently playing back.
         * @return true if playing, false otherwise.
         */
        bool isPlaying() const;

        /**
         * @brief Checks if playback is paused.
         * @return true if paused, false otherwise.
         */
        bool isPaused() const;

        /**
         * @brief Sets whether playback should loop when reaching the end.
         * @param loop true to enable looping, false to stop at end.
         */
        void setLooping( bool loop );

        /**
         * @brief Gets the current looping state.
         * @return true if looping is enabled, false otherwise.
         */
        bool isLooping() const;

        /**
         * @brief Sets the maximum number of events that can be recorded.
         * @param maxEvents Maximum event count (0 = unlimited, but not recommended).
         * @note If exceeded during recording, oldest events may be dropped or recording may stop.
         */
        void setMaxEvents( size_t maxEvents );

        /**
         * @brief Gets the maximum event limit.
         * @return Maximum number of events allowed.
         */
        size_t getMaxEvents() const;

        /**
         * @brief Retrieves properties for editor integration.
         * @return Smart pointer to a Properties object containing all exposed properties.
         */
        SmartPtr<Properties> getProperties() const override;

        /**
         * @brief Sets properties from editor or serialization.
         * @param properties Smart pointer to Properties object containing values to set.
         * @note Will validate all properties and log errors for invalid values.
         */
        void setProperties( SmartPtr<Properties> properties ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Handles an input event during recording.
         * @param event The input event to record.
         * @return false to allow event propagation to other listeners.
         */
        bool inputEvent( const SmartPtr<IInputEvent> &event );

        /**
         * @brief Validates that the input manager is valid.
         * @return true if valid, false otherwise (logs error).
         */
        bool validateInputManager() const;

        /**
         * @brief Transitions to a new state with validation.
         * @param newState The state to transition to.
         * @return true if transition was valid, false otherwise.
         */
        bool transitionToState( State newState );

        /**
         * @brief Processes playback of events based on current time.
         * @param currentTime Current playback time in seconds.
         */
        void processPlayback( double currentTime );

        /**
         * @brief Logs a message with InputRecorder prefix.
         * @param message The message to log.
         * @param isError true for error, false for info.
         */
        void logMessage( const String &message, bool isError = false ) const;

        InputManagerExt *m_inputManager;             ///< Pointer to the input manager.
        State m_state;                               ///< Current recorder state.
        Array<SmartPtr<IInputEvent>> m_inputEvents;  ///< Recorded input events.
        Array<double> m_eventTimestamps;             ///< Timestamps for each event.
        size_t m_playbackIndex;                      ///< Current playback position.
        double m_recordingStartTime;                 ///< Time when recording started.
        double m_playbackStartTime;                  ///< Time when playback started.
        double m_pauseTime;                          ///< Time when paused (for resume calculation).
        bool m_looping;                              ///< Whether to loop playback.
        size_t m_maxEvents;                          ///< Maximum events to record (0 = unlimited).
        String m_lastSavedFilePath;                  ///< Last file path used for save/load.
    };
}  // namespace workphone

#endif  // InputRecorder_h__
