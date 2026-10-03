#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Input/InputRecorder.hpp"
#include "Workphone/Input/InputManagerExt.hpp"
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <fstream>
#include <sstream>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, InputRecorder, ISharedObject );

    // Default maximum events to prevent memory overflow (10 minutes at 60fps ~= 36000 events)
    static const size_t DEFAULT_MAX_EVENTS = 50000;

    InputRecorder::InputRecorder( InputManagerExt *inputManager ) :
        m_inputManager( inputManager ),
        m_state( State::Idle ),
        m_playbackIndex( 0 ),
        m_recordingStartTime( 0.0 ),
        m_playbackStartTime( 0.0 ),
        m_pauseTime( 0.0 ),
        m_looping( false ),
        m_maxEvents( DEFAULT_MAX_EVENTS )
    {
        if( !validateInputManager() )
        {
            logMessage( "InputRecorder created with null InputManager - functionality will be limited",
                        true );
        }

        m_inputEvents.reserve( 1000 );  // Pre-allocate for better performance
        m_eventTimestamps.reserve( 1000 );
    }

    InputRecorder::~InputRecorder()
    {
        try
        {
            stop();
            clear();
            m_inputManager = nullptr;
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Exception in destructor: " ) + e.what(), true );
        }
        catch( ... )
        {
            logMessage( "Unknown exception in destructor", true );
        }
    }

    void InputRecorder::update( TaskId task, const double &t, const double &dt )
    {
        if( task != TaskId::Application )
        {
            return;  // Only update during application task
        }

        try
        {
            if( m_state == State::Playing )
            {
                double playbackTime = t - m_playbackStartTime;
                processPlayback( playbackTime );
            }
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Exception during update: " ) + e.what(), true );
            stop();  // Stop on error to prevent further issues
        }
        catch( ... )
        {
            logMessage( "Unknown exception during update", true );
            stop();
        }
    }

    bool InputRecorder::record()
    {
        if( !validateInputManager() )
        {
            return false;
        }

        if( m_state == State::Recording )
        {
            logMessage( "Already recording", false );
            return false;
        }

        try
        {
            // Stop any current playback
            if( m_state == State::Playing || m_state == State::Paused )
            {
                stop();
            }

            // Clear previous recording
            clear();

            // Transition to recording state
            if( !transitionToState( State::Recording ) )
            {
                return false;
            }

            m_recordingStartTime = 0.0;  // Will be set on first event
            logMessage( "Recording started", false );
            return true;
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Failed to start recording: " ) + e.what(), true );
            return false;
        }
        catch( ... )
        {
            logMessage( "Failed to start recording: unknown exception", true );
            return false;
        }
    }

    bool InputRecorder::play()
    {
        if( m_state == State::Playing )
        {
            logMessage( "Already playing", false );
            return false;
        }

        if( m_inputEvents.empty() )
        {
            logMessage( "No recorded events to play back", true );
            return false;
        }

        try
        {
            // Stop recording if active
            if( m_state == State::Recording )
            {
                stop();
            }

            // If resuming from pause, don't reset playback index
            if( m_state != State::Paused )
            {
                m_playbackIndex = 0;
                m_playbackStartTime = 0.0;  // Will be set on first frame
            }

            if( !transitionToState( State::Playing ) )
            {
                return false;
            }

            logMessage( String( "Playback started with " ) +
                            StringUtil::toString( m_inputEvents.size() ) + " events",
                        false );
            return true;
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Failed to start playback: " ) + e.what(), true );
            return false;
        }
        catch( ... )
        {
            logMessage( "Failed to start playback: unknown exception", true );
            return false;
        }
    }

    void InputRecorder::stop()
    {
        try
        {
            State previousState = m_state;

            if( !transitionToState( State::Idle ) )
            {
                return;
            }

            m_playbackIndex = 0;
            m_recordingStartTime = 0.0;
            m_playbackStartTime = 0.0;
            m_pauseTime = 0.0;

            if( previousState != State::Idle )
            {
                logMessage( "Stopped", false );
            }
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Exception during stop: " ) + e.what(), true );
            m_state = State::Idle;  // Force to idle state on error
        }
        catch( ... )
        {
            logMessage( "Unknown exception during stop", true );
            m_state = State::Idle;
        }
    }

    bool InputRecorder::pause()
    {
        if( m_state != State::Playing )
        {
            logMessage( "Cannot pause - not currently playing", false );
            return false;
        }

        try
        {
            if( !transitionToState( State::Paused ) )
            {
                return false;
            }

            m_pauseTime = 0.0;  // Store time for resume calculation if needed
            logMessage( "Playback paused", false );
            return true;
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Failed to pause: " ) + e.what(), true );
            return false;
        }
        catch( ... )
        {
            logMessage( "Failed to pause: unknown exception", true );
            return false;
        }
    }

    bool InputRecorder::resume()
    {
        if( m_state != State::Paused )
        {
            logMessage( "Cannot resume - not currently paused", false );
            return false;
        }

        try
        {
            if( !transitionToState( State::Playing ) )
            {
                return false;
            }

            logMessage( "Playback resumed", false );
            return true;
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Failed to resume: " ) + e.what(), true );
            return false;
        }
        catch( ... )
        {
            logMessage( "Failed to resume: unknown exception", true );
            return false;
        }
    }

    void InputRecorder::clear()
    {
        try
        {
            m_inputEvents.clear();
            m_eventTimestamps.clear();
            m_playbackIndex = 0;
            m_recordingStartTime = 0.0;
            m_playbackStartTime = 0.0;
            m_pauseTime = 0.0;
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Exception during clear: " ) + e.what(), true );
        }
        catch( ... )
        {
            logMessage( "Unknown exception during clear", true );
        }
    }

    bool InputRecorder::saveToFile( const String &filepath )
    {
        if( filepath.empty() )
        {
            logMessage( "Cannot save to empty filepath", true );
            return false;
        }

        if( m_inputEvents.empty() )
        {
            logMessage( "No events to save", true );
            return false;
        }

        try
        {
            std::ofstream file( filepath.c_str(), std::ios::binary );
            if( !file.is_open() )
            {
                logMessage( String( "Failed to open file for writing: " ) + filepath, true );
                return false;
            }

            // Write header
            u32 version = 1;
            u32 eventCount = static_cast<u32>( m_inputEvents.size() );
            file.write( reinterpret_cast<const char *>( &version ), sizeof( version ) );
            file.write( reinterpret_cast<const char *>( &eventCount ), sizeof( eventCount ) );

            // Write timestamps
            for( size_t i = 0; i < m_eventTimestamps.size(); ++i )
            {
                file.write( reinterpret_cast<const char *>( &m_eventTimestamps[i] ), sizeof( double ) );
            }

            // Note: Actual event serialization would require implementing serialization
            // for IInputEvent. This is a placeholder for the structure.
            // In production, you'd serialize event type, data, etc.

            file.close();
            m_lastSavedFilePath = filepath;
            logMessage(
                String( "Saved " ) + StringUtil::toString( eventCount ) + " events to " + filepath,
                false );
            return true;
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Failed to save file: " ) + e.what(), true );
            return false;
        }
        catch( ... )
        {
            logMessage( "Failed to save file: unknown exception", true );
            return false;
        }
    }

    bool InputRecorder::loadFromFile( const String &filepath )
    {
        if( filepath.empty() )
        {
            logMessage( "Cannot load from empty filepath", true );
            return false;
        }

        try
        {
            std::ifstream file( filepath.c_str(), std::ios::binary );
            if( !file.is_open() )
            {
                logMessage( String( "Failed to open file for reading: " ) + filepath, true );
                return false;
            }

            // Clear existing data
            clear();

            // Read header
            u32 version = 0;
            u32 eventCount = 0;
            file.read( reinterpret_cast<char *>( &version ), sizeof( version ) );
            file.read( reinterpret_cast<char *>( &eventCount ), sizeof( eventCount ) );

            if( version != 1 )
            {
                logMessage( String( "Unsupported file version: " ) + StringUtil::toString( version ),
                            true );
                return false;
            }

            if( eventCount > m_maxEvents && m_maxEvents > 0 )
            {
                logMessage( String( "File contains too many events: " ) +
                                StringUtil::toString( eventCount ) +
                                " (max: " + StringUtil::toString( m_maxEvents ) + ")",
                            true );
                return false;
            }

            // Read timestamps
            m_eventTimestamps.resize( eventCount );
            for( u32 i = 0; i < eventCount; ++i )
            {
                file.read( reinterpret_cast<char *>( &m_eventTimestamps[i] ), sizeof( double ) );
            }

            // Note: Event deserialization would be implemented here

            file.close();
            m_lastSavedFilePath = filepath;
            logMessage(
                String( "Loaded " ) + StringUtil::toString( eventCount ) + " events from " + filepath,
                false );
            return true;
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Failed to load file: " ) + e.what(), true );
            clear();  // Clear partial data on error
            return false;
        }
        catch( ... )
        {
            logMessage( "Failed to load file: unknown exception", true );
            clear();
            return false;
        }
    }

    InputRecorder::State InputRecorder::getState() const
    {
        return m_state;
    }

    size_t InputRecorder::getEventCount() const
    {
        return m_inputEvents.size();
    }

    bool InputRecorder::isRecording() const
    {
        return m_state == State::Recording;
    }

    bool InputRecorder::isPlaying() const
    {
        return m_state == State::Playing;
    }

    bool InputRecorder::isPaused() const
    {
        return m_state == State::Paused;
    }

    void InputRecorder::setLooping( bool loop )
    {
        m_looping = loop;
    }

    bool InputRecorder::isLooping() const
    {
        return m_looping;
    }

    void InputRecorder::setMaxEvents( size_t maxEvents )
    {
        if( maxEvents == 0 )
        {
            logMessage( "Warning: Setting max events to 0 (unlimited) may cause memory issues", false );
        }
        m_maxEvents = maxEvents;
    }

    size_t InputRecorder::getMaxEvents() const
    {
        return m_maxEvents;
    }

    SmartPtr<Properties> InputRecorder::getProperties() const
    {
        auto properties = ISharedObject::getProperties();
        if( !properties )
        {
            properties = workphone::make_ptr<Properties>();
        }

        try
        {
            // State information (read-only)
            String stateStr;
            switch( m_state )
            {
            case State::Idle:
                stateStr = "Idle";
                break;
            case State::Recording:
                stateStr = "Recording";
                break;
            case State::Playing:
                stateStr = "Playing";
                break;
            case State::Paused:
                stateStr = "Paused";
                break;
            default:
                stateStr = "Unknown";
                break;
            }
            properties->setProperty( "State", stateStr, true );

            // Event count (read-only)
            properties->setProperty( "Event Count", static_cast<s32>( m_inputEvents.size() ), true );

            // Configurable properties
            properties->setProperty( "Looping", m_looping, false );
            properties->setProperty( "Max Events", static_cast<s32>( m_maxEvents ), false );
            properties->setProperty( "Last File Path", m_lastSavedFilePath, false );

            // Playback position (read-only)
            properties->setProperty( "Playback Index", static_cast<s32>( m_playbackIndex ), true );
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Exception in getProperties: " ) + e.what(), true );
        }
        catch( ... )
        {
            logMessage( "Unknown exception in getProperties", true );
        }

        return properties;
    }

    void InputRecorder::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            logMessage( "setProperties received null properties", true );
            return;
        }

        try
        {
            ISharedObject::setProperties( properties );

            // Get current values as defaults
            bool looping = m_looping;
            s32 maxEvents = static_cast<s32>( m_maxEvents );
            String filePath = m_lastSavedFilePath;

            // Read properties with validation
            properties->getPropertyValue( "Looping", looping );
            properties->getPropertyValue( "Max Events", maxEvents );
            properties->getPropertyValue( "Last File Path", filePath );

            // Validate and apply
            if( maxEvents < 0 )
            {
                logMessage( "Invalid Max Events value (negative), using 0 (unlimited)", true );
                maxEvents = 0;
            }

            m_looping = looping;
            setMaxEvents( static_cast<size_t>( maxEvents ) );
            m_lastSavedFilePath = filePath;
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Exception in setProperties: " ) + e.what(), true );
        }
        catch( ... )
        {
            logMessage( "Unknown exception in setProperties", true );
        }
    }

    bool InputRecorder::inputEvent( const SmartPtr<IInputEvent> &event )
    {
        if( m_state != State::Recording )
        {
            return false;  // Not recording, ignore event
        }

        if( !event )
        {
            logMessage( "Received null input event during recording", true );
            return false;
        }

        try
        {
            // Check if we've hit the event limit
            if( m_maxEvents > 0 && m_inputEvents.size() >= m_maxEvents )
            {
                logMessage( String( "Max events reached (" ) + StringUtil::toString( m_maxEvents ) +
                                "), stopping recording",
                            true );
                stop();
                return false;
            }

            // Store the event
            m_inputEvents.push_back( event );

            // Store timestamp (implement proper timing based on your engine's time system)
            double timestamp = 0.0;  // Would use actual game time here
            if( m_recordingStartTime == 0.0 )
            {
                m_recordingStartTime = timestamp;
            }
            m_eventTimestamps.push_back( timestamp - m_recordingStartTime );
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Exception recording event: " ) + e.what(), true );
            return false;
        }
        catch( ... )
        {
            logMessage( "Unknown exception recording event", true );
            return false;
        }

        return false;  // Allow event to propagate to other listeners
    }

    bool InputRecorder::validateInputManager() const
    {
        if( !m_inputManager )
        {
            logMessage( "InputManager is null", true );
            return false;
        }
        return true;
    }

    bool InputRecorder::transitionToState( State newState )
    {
        try
        {
            // Validate state transitions
            switch( newState )
            {
            case State::Recording:
                if( m_state == State::Playing || m_state == State::Paused )
                {
                    logMessage( "Cannot record while playing/paused", true );
                    return false;
                }
                break;

            case State::Playing:
                if( m_state == State::Recording )
                {
                    logMessage( "Cannot play while recording", true );
                    return false;
                }
                break;

            case State::Paused:
                if( m_state != State::Playing )
                {
                    logMessage( "Can only pause during playback", true );
                    return false;
                }
                break;

            case State::Idle:
                // Can always transition to idle
                break;

            default:
                logMessage( "Unknown state transition requested", true );
                return false;
            }

            m_state = newState;
            return true;
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Exception during state transition: " ) + e.what(), true );
            return false;
        }
        catch( ... )
        {
            logMessage( "Unknown exception during state transition", true );
            return false;
        }
    }

    void InputRecorder::processPlayback( double currentTime )
    {
        try
        {
            if( m_playbackStartTime == 0.0 )
            {
                m_playbackStartTime = currentTime;
                return;
            }

            double playbackTime = currentTime - m_playbackStartTime;

            // Play all events that should have occurred by now
            while( m_playbackIndex < m_eventTimestamps.size() )
            {
                if( m_eventTimestamps[m_playbackIndex] <= playbackTime )
                {
                    // Dispatch event (would need to re-inject into input system)
                    // This is framework-specific implementation
                    m_playbackIndex++;
                }
                else
                {
                    break;  // Wait for next frame
                }
            }

            // Check if playback is complete
            if( m_playbackIndex >= m_inputEvents.size() )
            {
                if( m_looping && !m_inputEvents.empty() )
                {
                    logMessage( "Looping playback", false );
                    m_playbackIndex = 0;
                    m_playbackStartTime = currentTime;
                }
                else
                {
                    logMessage( "Playback complete", false );
                    stop();
                }
            }
        }
        catch( const std::exception &e )
        {
            logMessage( String( "Exception during playback: " ) + e.what(), true );
            stop();
        }
        catch( ... )
        {
            logMessage( "Unknown exception during playback", true );
            stop();
        }
    }

    void InputRecorder::logMessage( const String &message, bool isError ) const
    {
        String fullMessage = "InputRecorder: " + message;

        try
        {
            if( isError )
            {
                WP_LOG_ERROR( fullMessage );
            }
            else
            {
                WP_LOG_INFO( fullMessage );
            }
        }
        catch( ... )
        {
            // If logging fails, we can't do much about it
            // In debug builds, this could assert or break
        }
    }

}  // namespace workphone
