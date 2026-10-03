#ifndef _WPFMODStudioSound3_H
#define _WPFMODStudioSound3_H

#include <WPFMODStudio/WPFMODStudioPrerequisites.hpp>
#include <Workphone/Sound/Sound.hpp>
#include <fmod.h>
#include <fmod_errors.h>
#include <fmod_studio.hpp>

namespace workphone
{

    /**
     * @class WPFMODStudioSound
     * @brief This class is used to play sounds using the FMOD Studio API.
     */
    class WPFMODStudioSound : public Sound
    {
    public:
        /** Constructor. */
        WPFMODStudioSound();

        /** Constructor. */
        WPFMODStudioSound( ISoundManager *pSoundManager, FMOD::Studio::System *system );

        /** Destructor. */
        ~WPFMODStudioSound() override;

        /** @copydoc ISharedObject::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISharedObject::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        void play() override;
        void stop() override;
        bool isPlaying() const override;
        void setPosition( const Vector3F &position ) override;
        Vector3F getPosition() const override;

        void setVolume( f32 fVolume ) override;
        f32 getVolume() const override;

        void setMinMaxDistance( f32 minDistance, f32 maxDistance ) override;
        void getMinMaxDistance( f32 &minDistance, f32 &maxDistance ) override;

        String getSoundName() const;

        void getSpectrum( Array<f32> &spectrumData, u32 numvalues ) const override;

        WP_CLASS_REGISTER_DECL;

    private:
        ISoundManager *m_pSoundManager = nullptr;
        FMOD::Studio::System *m_pSystem = nullptr;
        FMOD::Sound *m_pSound = nullptr;
        FMOD::Channel *m_pChannel = nullptr;

        Vector3F m_position;
    };
}  // namespace workphone

#endif
