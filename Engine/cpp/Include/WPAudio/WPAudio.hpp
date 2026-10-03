#ifndef FBAudio_h__
#define FBAudio_h__

#include <WPAudio/WPAudioPrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    /**
     * @class WPAudio
     * @brief Main entry point for the Workphone Audio library.
     * 
     * This class manages the lifecycle of the audio plugin and provides access 
     * to the factory manager for audio object creation.
     */
    class WPAudio_API WPAudio : public ISharedObject
    {
    public:
        /** 
         * @brief Default constructor. 
         */
        WPAudio();

        /** 
         * @brief Destructor. 
         */
        ~WPAudio() override;

        /** @copydoc ISharedObject::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISharedObject::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** 
         * @brief Gets the singleton instance of the audio plugin. 
         * @return A SmartPtr to the WPAudio instance.
         */
        static SmartPtr<WPAudio> instance();

        /** 
         * @brief Sets the singleton instance of the audio plugin. 
         * @param plugin The SmartPtr to the WPAudio instance to set.
         */
        static void setInstance( SmartPtr<WPAudio> plugin );

        /** 
         * @brief Gets the factory manager used for creating audio-related objects. 
         * @return A SmartPtr to the IFactoryManager.
         */
        static SmartPtr<IFactoryManager> getFactoryManager();

        /** 
         * @brief Sets the factory manager for the audio library. 
         * @param factoryManager The SmartPtr to the IFactoryManager to set.
         */
        static void setFactoryManager( SmartPtr<IFactoryManager> factoryManager );

    protected:
        static SmartPtr<WPAudio> m_sPlugin;
        static SmartPtr<IFactoryManager> m_factoryManager;
    };
}  // namespace workphone

#endif  // FBAudio_h__
