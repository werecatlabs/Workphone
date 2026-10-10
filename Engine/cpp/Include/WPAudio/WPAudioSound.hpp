#ifndef __WPAudioSound_H
#define __WPAudioSound_H

#include <WPAudio/WPAudioPrerequisites.hpp>
#include <Workphone/Sound/Sound.hpp>

// Keep Windows SDK declarations out of this public, cross-platform header.
#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
struct IXAudio2SourceVoice;
#endif

namespace workphone
{
    /**
     * Sound implementation for the built-in platform audio backend.
     *
     * Native objects are stored in an opaque implementation so including this
     * header never requires XAudio2, AudioToolbox, AudioUnit, or OpenSL ES
     * headers. The implementation selects XAudio2 on Windows, Audio Queue on
     * macOS/iOS, and OpenSL ES on Android.
     */
    class WPAudio_API WPAudioSound : public Sound
    {
    public:
        WPAudioSound();
        ~WPAudioSound() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void reload( SmartPtr<ISharedObject> data ) override;
        bool hasReloadError() const;

        void play() override;
        void pause() override;
        void stop() override;
        bool isPlaying() const override;
        void setVolume( f32 volume ) override;
        void setLoop( bool loop ) override;

        Parameter handleEvent( EventType eventType, hash_type eventValue,
                               const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                               SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;
        bool handleStateChanged( SmartPtr<IState> &state ) override;

#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
        /** Non-owning access to the XAudio2 voice, for legacy Windows callers. */
        IXAudio2SourceVoice *getSourceVoice() const;

        /**
         * Replaces the native voice used by this sound. The sound owns the
         * supplied voice and destroys it during unload or replacement.
         */
        void setSourceVoice( IXAudio2SourceVoice *sourceVoice );
#endif

        WP_CLASS_REGISTER_DECL;

    private:
        struct PlatformSoundState;

        bool loadPlatformSound( const String &filename );
        void unloadPlatformSound();

        // Platform SDK types and owned audio data live entirely in the .cpp.
        PlatformSoundState *m_platformSoundState = nullptr;
        bool m_reloadFailed = false;
    };
}  // namespace workphone

#endif  // __WPAudioSound_H
