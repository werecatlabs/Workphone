#ifndef _WPFMODSoundManager_H
#define _WPFMODSoundManager_H

#include <WPFMODStudio/WPFMODStudioPrerequisites.hpp>
#include <Workphone/Sound/SoundManager.hpp>
#include <Workphone/Core/Array.hpp>
#include <map>
#include <fmod_studio.hpp>

namespace workphone
{

    /**
     * @class WPFMODStudioManager
     * @brief The WPFMODStudioManager class is a class that manages the sound system.
     */
    class WPFMODStudioManager : public SoundManager
    {
    public:
        /** Constructor. */
        WPFMODStudioManager();

        /** Destructor. */
        ~WPFMODStudioManager() override;

        /** @copydoc ISoundManager::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISoundManager::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISoundManager::update */
        void update() override;

        void removeAll();

        void setVolume( f32 fVolume ) override;
        f32 getVolume() const override;

        void startRecording() override;
        void stopRecording() override;
        u32 getBufferSize() const override;
        void copyContentsToMemory( void *buffer, u32 size ) override;

        void _getObject( void **ppObject ) const override;

        void *GetEventSystem() const;

        WP_CLASS_REGISTER_DECL;

    private:
        void handleStateChanged( const SmartPtr<IStateMessage> &message ) override;

        void handleStateChanged( SmartPtr<IState> &state ) override;

        FMOD::Studio::System m_pSystem;

        using SoundMap = std::map<String, String>;
        SoundMap m_soundMap;
    };
}  // namespace workphone

#endif
