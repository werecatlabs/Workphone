#ifndef _WP_ISound_H
#define _WP_ISound_H

#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{

    /**
     * @brief Interface representing a sound resource.
     *
     * This interface provides methods for controlling sound playback, volume, panning, 3D positioning,
     * looping, and querying or setting various sound properties and flags. Implementations of this
     * interface are responsible for managing the underlying sound data and playback state.
     */
    class WPCore_API ISound : public IResource
    {
    public:
        /** @name Sound Flags */
        ///@{
        static const u32 SOUND_FLAG_3D;        //!< Indicates the sound is 3D positional.
        static const u32 SOUND_FLAG_STREAM;    //!< Indicates the sound is streamed from disk.
        static const u32 SOUND_FLAG_LOOP;      //!< Indicates the sound should loop.
        static const u32 SOUND_FLAG_HARDWARE;  //!< Indicates the sound should use hardware acceleration.
        static const u32 SOUND_FLAG_MUTE;      //!< Indicates the sound is muted.
        static const u32
            SOUND_FLAG_DELETE_WHEN_FINISHED;  //!< Indicates the sound should be deleted when finished.
        static const u32 SOUND_FLAG_PAUSED;   //!< Indicates the sound is paused.
        static const u32 SOUND_FLAG_2D;       //!< Indicates the sound is 2D (not positional).
        static const u32 SOUND_FLAG_PLAYING;  //!< Indicates the sound is unloaded.
        ///@}

        ISound();

        ISound( u32 poolTypeId );

        /**
         * @brief Virtual destructor.
         */
        ~ISound() override;

        /**
         * @brief Starts playback of the sound.
         */
        virtual void play() = 0;

        /** @brief Pauses playback of the sound. */
        virtual void pause() = 0;

        /**
         * @brief Stops playback of the sound.
         */
        virtual void stop() = 0;

        /**
         * @brief Checks if the sound is currently playing.
         * @return True if the sound is playing, false otherwise.
         */
        virtual bool isPlaying() const = 0;

        /**
         * @brief Sets the volume of the sound.
         * @param volume Volume level in the range [0.0, 1.0].
         */
        virtual void setVolume( f32 volume ) = 0;

        /**
         * @brief Gets the current volume of the sound.
         * @return Volume level in the range [0.0, 1.0].
         */
        virtual f32 getVolume() const = 0;

        /**
         * @brief Sets whether the sound should loop when it reaches the end.
         * @param loop True to enable looping, false to disable.
         */
        virtual void setLoop( bool loop ) = 0;

        /**
         * @brief Checks if the sound is set to loop.
         * @return True if looping is enabled, false otherwise.
         */
        virtual bool getLoop() const = 0;

        /**
         * @brief Retrieves the frequency spectrum data from the sound.
         * @param spectrum Array to store the spectrum data.
         * @param numValues Number of values to retrieve in the spectrum.
         * @remarks The actual data and behavior may depend on the underlying sound library
         * implementation.
         */
        virtual void getSpectrum( Array<f32> &spectrum, u32 numValues ) const = 0;

        /**
         * @brief Sets the stereo pan of the sound.
         * @param pan Pan value, typically in the range [-1.0 (left), 1.0 (right)].
         */
        virtual void setPan( f32 pan ) = 0;

        /**
         * @brief Sets the 3D position of the sound in world space.
         * @param position The new position vector.
         */
        virtual void setPosition( const Vector3<real_Num> &position ) = 0;

        /**
         * @brief Gets the current 3D position of the sound in world space.
         * @return The position vector.
         */
        virtual Vector3<real_Num> getPosition() const = 0;

        /**
         * @brief Sets the minimum and maximum distances for 3D sound attenuation.
         * @param minDistance Minimum distance for full volume.
         * @param maxDistance Maximum distance for attenuation.
         */
        virtual void setMinMaxDistance( f32 minDistance, f32 maxDistance ) = 0;

        /**
         * @brief Gets the minimum and maximum distances for 3D sound attenuation.
         * @param minDistance Output parameter for minimum distance.
         * @param maxDistance Output parameter for maximum distance.
         */
        virtual void getMinMaxDistance( f32 &minDistance, f32 &maxDistance ) = 0;

        /**
         * @brief Gets the owner sound manager of this sound.
         * @return Smart pointer to the ISoundManager that owns this sound.
         */
        virtual SmartPtr<ISoundManager> getOwner() const = 0;

        /**
         * @brief Sets the owner sound manager of this sound.
         * @param owner Smart pointer to the ISoundManager to set as owner.
         */
        virtual void setOwner( SmartPtr<ISoundManager> owner ) = 0;

        /**
         * @brief Gets the current sound flags.
         * @return Bitmask of sound flags.
         */
        virtual u32 getFlags() const = 0;

        /**
         * @brief Sets the sound flags.
         * @param flags Bitmask of flags to set.
         */
        virtual void setFlags( u32 flags ) = 0;

        /**
         * @brief Sets or clears a specific sound flag.
         * @param flags The flag to modify.
         * @param value True to set the flag, false to clear it.
         */
        virtual void setFlag( u32 flags, bool value ) = 0;

        /**
         * @brief Checks if a specific sound flag is set.
         * @param flags The flag to check.
         * @return True if the flag is set, false otherwise.
         */
        virtual bool getFlag( u32 flags ) const = 0;

        /**
         * @brief Registers the class for reflection or serialization.
         */
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif
