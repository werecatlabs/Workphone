#ifndef ISoundPlayer_h__
#define ISoundPlayer_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    /**
     * @class ISoundPlayer
     * @brief Interface for a sound file player, providing playback control and configuration.
     *
     * This interface defines the contract for a sound player, allowing clients to control playback,
     * adjust playback parameters, and query playback state. Implementations should provide mechanisms
     * for playing, pausing, stopping, seeking, and configuring sound playback, as well as querying
     * playback status and properties such as volume, repeat mode, and delay between beeps.
     */
    class WPCore_API ISoundPlayer : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor for ISoundPlayer.
         */
        ~ISoundPlayer() override;

        /**
         * @brief Start or resume playback of the sound.
         *
         * If the sound is already playing, this has no effect.
         */
        virtual void play() = 0;

        /**
         * @brief Stop playback of the sound.
         *
         * Playback will be halted and the playback position reset to the beginning.
         */
        virtual void stop() = 0;

        /**
         * @brief Pause playback of the sound.
         *
         * Playback can be resumed from the current position by calling play().
         */
        virtual void pause() = 0;

        /**
         * @brief Skip forward in the sound track.
         *
         * The amount to skip forward is implementation-defined.
         */
        virtual void forward() = 0;

        /**
         * @brief Skip backward in the sound track.
         *
         * The amount to skip backward is implementation-defined.
         */
        virtual void rewind() = 0;

        /**
         * @brief Restart playback from the beginning of the sound track.
         */
        virtual void restart() = 0;

        /**
         * @brief Skip to the next section or track in the sound, if supported.
         *
         * The meaning of "section" is implementation-defined (e.g., next track, next chapter).
         */
        virtual void skip() = 0;

        /**
         * @brief Enable or disable repeat mode for playback.
         * @param repeat True to enable repeat mode, false to disable.
         */
        virtual void setRepeat( bool repeat ) = 0;

        /**
         * @brief Query whether repeat mode is enabled.
         * @return True if repeat mode is enabled, false otherwise.
         */
        virtual bool getRepeat() const = 0;

        /**
         * @brief Enable or disable a delay between beeps in the sound.
         * @param beepDelay True to enable a delay between beeps, false to disable.
         */
        virtual void setBeepDelay( bool beepDelay ) = 0;

        /**
         * @brief Query whether a delay between beeps is enabled.
         * @return True if a delay between beeps is enabled, false otherwise.
         */
        virtual bool getBeepDelay() const = 0;

        /**
         * @brief Set the playback volume.
         * @param volume The new volume level, typically in the range [0.0, 1.0].
         */
        virtual void setVolume( f32 volume ) = 0;

        /**
         * @brief Get the current playback volume.
         * @return The current volume level, typically in the range [0.0, 1.0].
         */
        virtual f32 getVolume() const = 0;

        /**
         * @brief Get the total length of the sound track.
         * @return The length of the sound track in milliseconds.
         */
        virtual s32 getTrackLength() = 0;

        /**
         * @brief Get the current playback position.
         * @return The current playback time in milliseconds.
         */
        virtual s32 getPlaybackTime() = 0;

        /**
         * @brief Set the current playback position.
         * @param playbackTime The new playback time in milliseconds.
         */
        virtual void setPlaybackTime( s32 playbackTime ) = 0;

        /**
         * @brief Set the delay time between beeps in the sound.
         * @param delayTime The delay time in milliseconds.
         */
        virtual void setDelayTime( s32 delayTime ) = 0;

        /**
         * @brief Get the delay time between beeps in the sound.
         * @return The delay time in milliseconds.
         */
        virtual s32 getDelayTime() const = 0;

        /**
         * @brief Get the identifier tag for the sound.
         * @return The ID tag as a String.
         */
        virtual String getIdTag() = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ISoundPlayer_h__
