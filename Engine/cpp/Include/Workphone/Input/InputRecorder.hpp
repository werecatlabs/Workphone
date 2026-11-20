#ifndef InputRecorder_h__
#define InputRecorder_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Input/InputManagerExt.hpp>

/**
 * @file InputRecorder.hpp
 * @brief State machine that records input events in memory and plays them back with timing.
 */

namespace workphone
{
    /**
     * @class InputRecorder
     * @brief Records input events in memory and plays them back with timing control.
     *
     * @details
     * The recorder is a small state machine (see InputRecorder::State) that captures input events
     * together with a timestamp, replays them against an elapsed-time clock, and exposes its
     * configuration to the editor through the property system.
     *
     * Features:
     * - Record input events with timestamps
     * - Play back recorded events based on their timestamps
     * - Save/load recordings to/from a binary file
     * - Pause and resume during playback
     * - Loop playback mode
     * - Maximum event limit to prevent unbounded memory growth
     * - Error handling and logging (public operations catch exceptions and log them)
     * - Editor-exposed properties for configuration
     *
     * @note All operations are defensive and will log errors rather than crash on invalid input.
     * @note The current implementation is incomplete: recorded timestamps are placeholders (0.0),
     *       played-back events are not re-injected into the input system, and file persistence stores
     *       only a header and the timestamps, not the event payloads.
     * @note The recorder does not own the input manager passed to the constructor, and it is not
     *       thread-safe.
     */
    class WPCore_API InputRecorder : public ISharedObject
    {
    public:
        /**
         * @brief Enumeration of recorder states.
         *
         * Allowed transitions: Idle may always be entered; Recording cannot be entered from
         * Playing/Paused; Playing cannot be entered from Recording; Paused can only be entered
         * from Playing.
         */
        enum class State
        {
            Idle,       ///< Not recording or playing.
            Recording,  ///< Currently recording input events.
            Playing,    ///< Currently playing back recorded events.
            Paused      ///< Playback is paused.
        };

        /**
         * @brief Constructs an InputRecorder in the Idle state.
         * @param inputManager Non-owning pointer to the input manager. May be nullptr, in which case
         *        an error is logged, record() fails and functionality is limited.
         */
        explicit InputRecorder( InputManagerExt *inputManager );

        /**
         * @brief Destructor. Stops any recording/playback, clears recorded events and releases the
         *        input manager pointer. Exceptions are caught and logged.
         */
        ~InputRecorder() override;

        /**
         * @brief Per-frame update; advances playback while in the Playing state.
         * @param task The task identifier for the update context. Only TaskId::Application is
         *        processed; other tasks are ignored.
         * @param t Current time in seconds.
         * @param dt Delta time since last update in seconds (unused).
         * @note If an exception occurs during playback, it is logged and the recorder is stopped.
         */
        void update( TaskId task, const double &t, const double &dt );

        /**
         * @brief Starts playing back recorded events from the beginning.
         * @return true if playback started successfully, false otherwise.
         * @note Fails (and logs) if already playing or if there are no recorded events.
         *       An active recording is stopped first. Calling this while paused continues from the
         *       current playback position instead of restarting.
         */
        bool play();

        /**
         * @brief Starts recording input events.
         * @return true if recording started successfully, false otherwise.
         * @note Fails if the input manager is null or if already recording. Any current playback is
         *       stopped and all previously recorded events are discarded.
         */
        bool record();

        /**
         * @brief Stops recording or playback and returns to the Idle state.
         * @note Recorded events are kept; only the playback position and timing state are reset.
         *       Has no effect if already idle.
         */
        void stop();

        /**
         * @brief Pauses playback.
         * @return true if successfully paused; false (and logs) if not currently playing, i.e.
         *         when recording, idle or already paused.
         */
        bool pause();

        /**
         * @brief Resumes playback from the paused state.
         * @return true if successfully resumed; false (and logs) if not currently paused.
         */
        bool resume();

        /**
         * @brief Clears all recorded events and timestamps and resets the playback position and
         *        timing state.
         * @note Does not change the current State.
         */
        void clear();

        /**
         * @brief Saves the recording to a binary file.
         * @param filepath Path to the file where the recording will be saved.
         * @return true if save was successful, false otherwise (empty path, no recorded events, or
         *         the file could not be opened/written; the failure is logged).
         * @note The file contains a version header (version 1), the event count and one double
         *       timestamp per event. Event payloads are not serialized yet.
         *       On success the path is remembered as the last file path.
         */
        bool saveToFile( const String &filepath );

        /**
         * @brief Loads a recording from a binary file written by saveToFile().
         * @param filepath Path to the file to load.
         * @return true if load was successful, false otherwise (empty path, file could not be
         *         opened, unsupported version, or event count above the maximum; logged).
         * @note Existing events are cleared once the file has been opened. Only the timestamps are
         *       restored; event payloads are not deserialized yet, so getEventCount() stays 0 and
         *       play() will fail until that is implemented. On success the path is remembered as the
         *       last file path.
         */
        bool loadFromFile( const String &filepath );

        /**
         * @brief Gets the current state of the recorder.
         * @return Current State enumeration value.
         */
        State getState() const;

        /**
         * @brief Gets the number of currently recorded input events.
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
         * @param loop true to restart playback from the first event at the end, false to stop.
         */
        void setLooping( bool loop );

        /**
         * @brief Gets the current looping state.
         * @return true if looping is enabled, false otherwise.
         */
        bool isLooping() const;

        /**
         * @brief Sets the maximum number of events that can be recorded.
         * @param maxEvents Maximum event count (0 = unlimited, which logs a warning and is not
         *        recommended).
         * @note When the limit is reached during recording, recording stops (no events are
         *       dropped). loadFromFile() also rejects files with more events than this limit.
         */
        void setMaxEvents( size_t maxEvents );

        /**
         * @brief Gets the maximum event limit.
         * @return Maximum number of events allowed (0 = unlimited).
         */
        size_t getMaxEvents() const;

        /**
         * @brief Retrieves the recorder's properties for editor integration.
         * @return Properties object extending the base object's properties with the read-only
         *         "State", "Event Count" and "Playback Index" and the editable "Looping",
         *         "Max Events" and "Last File Path".
         */
        SmartPtr<Properties> getProperties() const override;

        /**
         * @brief Applies properties from the editor or from serialization.
         * @param properties Properties object containing the values to set; a null pointer is
         *        logged and ignored.
         * @note Only "Looping", "Max Events" and "Last File Path" are applied. A negative
         *       "Max Events" is logged as an error and treated as 0 (unlimited).
         */
        void setProperties( SmartPtr<Properties> properties ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Handles an input event during recording.
         * @param event The input event to record.
         * @return Always false, so the event keeps propagating to other listeners.
         * @note Ignored unless the recorder is in the Recording state. A null event is logged and
         *       ignored. If the maximum event count has been reached, recording is stopped instead
         *       of storing the event. The stored timestamp is currently a placeholder (0.0).
         */
        bool inputEvent( const SmartPtr<IInputEvent> &event );

        /**
         * @brief Validates that the input manager pointer is set.
         * @return true if valid, false otherwise (logs an error).
         */
        bool validateInputManager() const;

        /**
         * @brief Transitions to a new state after validating the transition.
         * @param newState The state to transition to.
         * @return true if the transition was valid and applied, false otherwise (logs an error).
         * @see State for the allowed transitions.
         */
        bool transitionToState( State newState );

        /**
         * @brief Advances playback and handles end-of-recording.
         * @param currentTime Current time in seconds. The first call after playback starts only
         *        latches this value as the playback start time.
         * @note Steps past every event whose timestamp has elapsed (events are not yet re-injected
         *       into the input system). When the last event is reached, playback restarts if looping
         *       is enabled, otherwise the recorder is stopped. Exceptions are logged and stop the
         *       recorder.
         */
        void processPlayback( double currentTime );

        /**
         * @brief Logs a message prefixed with "InputRecorder: ". Exceptions from logging are
         *        swallowed.
         * @param message The message to log.
         * @param isError true to log as an error, false to log as info.
         */
        void logMessage( const String &message, bool isError = false ) const;

        InputManagerExt *m_inputManager;             ///< Non-owning input manager pointer; may be null.
        State m_state;                               ///< Current recorder state.
        Array<SmartPtr<IInputEvent>> m_inputEvents;  ///< Recorded input events, in order.
        Array<double> m_eventTimestamps;             ///< Seconds since recording start, one per event.
        size_t m_playbackIndex;                      ///< Index of the next event to play back.
        double m_recordingStartTime;                 ///< Reference time set on the first recorded event.
        double m_playbackStartTime;                  ///< Reference time set on the first playback frame.
        double m_pauseTime;                          ///< Reserved for resume timing; reset on pause.
        bool m_looping;                              ///< Whether playback restarts at the end.
        size_t m_maxEvents;                          ///< Maximum events to record (0 = unlimited).
        String m_lastSavedFilePath;                  ///< Last file path used for save/load.

    };
}  // namespace workphone

#endif  // InputRecorder_h__
