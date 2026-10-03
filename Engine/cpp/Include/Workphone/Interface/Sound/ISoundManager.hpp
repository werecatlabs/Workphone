#ifndef _ISOUNDMANAGER_H
#define _ISOUNDMANAGER_H

#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{

    /**
     * @class ISoundManager
     * @brief Interface for managing sound resources, playback, listeners, and recording in the audio
     * subsystem.
     *
     * This interface provides methods for controlling the master volume, muting, sound listeners,
     * recording, buffer management, and flag-based configuration. It is intended to be implemented by
     * concrete sound manager classes.
     */
    class WPCore_API ISoundManager : public IResourceManager
    {
    public:
        /**
         * @brief Flag to mute all sounds managed by the sound manager.
         */
        static const u32 SOUND_FLAG_MUTE;

        /**
         * @brief Flag to reset the sound manager state.
         */
        static const u32 SOUND_FLAG_RESET;

        /**
         * @brief Flag to reset the sound manager state.
         */
        static const u32 SOUND_FLAG_REALTIME;

        /**
         * @brief Virtual destructor for ISoundManager.
         */
        ~ISoundManager() override;

        /**
         * @brief Adds a 3D sound listener to the sound manager.
         *
         * @param name The unique name of the listener.
         * @param position The initial position of the listener in 3D space. Defaults to the origin.
         * @return A smart pointer to the created ISoundListener3 instance.
         */
        virtual SmartPtr<ISoundListener3> addListener3(
            const String &name, const Vector3<real_Num> &position = Vector3<real_Num>::zero() ) = 0;

        /**
         * @brief Finds a 3D sound listener by its name.
         *
         * @param name The name of the listener to find.
         * @return A smart pointer to the found ISoundListener3 instance, or nullptr if not found.
         */
        virtual SmartPtr<ISoundListener3> findListener3( const String &name ) = 0;

        /**
         * @brief Sets the master volume for all sounds managed by the sound manager.
         *
         * @param volume The volume level, ranging from 0.0 (silent) to 1.0 (full volume).
         */
        virtual void setVolume( f32 volume ) = 0;

        /**
         * @brief Gets the current master volume of the sound manager.
         *
         * @return The volume level, ranging from 0.0 (silent) to 1.0 (full volume).
         */
        virtual f32 getVolume() const = 0;

        /**
         * @brief Starts recording audio input from the default or configured device.
         */
        virtual void startRecording() = 0;

        /**
         * @brief Stops the ongoing audio recording.
         */
        virtual void stopRecording() = 0;

        /**
         * @brief Gets the size of the internal sound buffer used for recording or playback.
         *
         * @return The size of the sound buffer in bytes.
         */
        virtual u32 getBufferSize() const = 0;

        /**
         * @brief Copies the contents of the internal sound buffer to a user-provided memory buffer.
         *
         * @param buffer Pointer to the destination buffer.
         * @param size The size of the destination buffer in bytes.
         */
        virtual void copyContentsToMemory( void *buffer, u32 size ) = 0;

        /**
         * @brief Checks if the sound manager is running in real-time mode.
         *
         * @return True if running in real-time, false otherwise (e.g., offline processing).
         */
        virtual bool isRealtime() const = 0;

        /**
         * @brief Checks if the sound manager is currently muted.
         *
         * @return True if muted, false otherwise.
         */
        virtual bool isMute() const = 0;

        /**
         * @brief Sets the mute state of the sound manager.
         *
         * @param mute True to mute all sounds, false to unmute.
         */
        virtual void setMute( bool mute ) = 0;

        /**
         * @brief Loads a resource object via the sound manager, optionally queuing for deferred loading.
         *
         * @param object The resource object to load.
         * @param forceQueue If true, forces the object to be queued for deferred loading.
         */
        virtual void loadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) = 0;

        /**
         * @brief Unloads a resource object via the sound manager, optionally queuing for deferred
         * unloading.
         *
         * @param object The resource object to unload.
         * @param forceQueue If true, forces the object to be queued for deferred unloading.
         */
        virtual void unloadObject( SmartPtr<ISharedObject> object, bool forceQueue = false ) = 0;

        /**
         * @brief Gets the current flags set on the sound manager.
         *
         * @return The bitmask of flags.
         */
        virtual u32 getFlags() const = 0;

        /**
         * @brief Sets the flags for the sound manager.
         *
         * @param flags The bitmask of flags to set.
         */
        virtual void setFlags( u32 flags ) = 0;

        /**
         * @brief Sets or clears a specific flag on the sound manager.
         *
         * @param flags The flag(s) to modify.
         * @param value True to set the flag(s), false to clear.
         */
        virtual void setFlag( u32 flags, bool value ) = 0;

        /**
         * @brief Checks if a specific flag is set on the sound manager.
         *
         * @param flags The flag(s) to check.
         * @return True if the flag(s) are set, false otherwise.
         */
        virtual bool getFlag( u32 flags ) const = 0;

        /**
         * @brief Gets the factory manager used for creating sound-related objects.
         *
         * @return A smart pointer to the IFactoryManager instance.
         */
        virtual SmartPtr<IFactoryManager> getFactoryManager() const = 0;

        /**
         * @brief Sets the factory manager used for creating sound-related objects.
         *
         * @param factoryManager A smart pointer to the IFactoryManager instance to set.
         */
        virtual void setFactoryManager( SmartPtr<IFactoryManager> factoryManager ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif
