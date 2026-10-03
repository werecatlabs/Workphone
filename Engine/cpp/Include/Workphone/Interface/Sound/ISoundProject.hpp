#ifndef ISoundProject_h__
#define ISoundProject_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /**
     * @brief An interface for a sound project.
     */
    class WPCore_API ISoundProject : public ISharedObject
    {
    public:
        /**
         * @brief Destructor for ISoundProject.
         */
        ~ISoundProject() override;

        /**
         * @brief Adds a sound event to the project.
         * @param soundEventName The name of the sound event.
         * @return A shared pointer to the added sound event.
         */
        virtual SmartPtr<ISoundEvent> addSoundEvent( const String &soundEventName ) = 0;

        /**
         * @brief Removes a sound event from the project.
         * @param soundEvent A shared pointer to the sound event to remove.
         */
        virtual void removeSoundEvent( SmartPtr<ISoundEvent> soundEvent ) = 0;

        /**
         * @brief Gets the master volume of the sound project.
         * @return The master volume.
         */
        virtual f32 getMasterVolume() const = 0;

        /**
         * @brief Sets the master volume of the sound project.
         * @param masterVolume The new master volume.
         */
        virtual void setMasterVolume( f32 masterVolume ) = 0;

        /**
         * @brief Sets up the reverb for the sound project.
         */
        virtual void setupReverb() = 0;

        /**
         * @brief Gets whether reverb is enabled for the sound project.
         * @return True if reverb is enabled, false otherwise.
         */
        virtual bool getEnableReverb() const = 0;

        /**
         * @brief Sets whether reverb is enabled for the sound project.
         * @param enableReverb True to enable reverb, false to disable.
         */
        virtual void setEnableReverb( bool enableReverb ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ISoundProject_h__
