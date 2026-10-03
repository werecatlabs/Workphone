#ifndef SoundResourceDirector_h__
#define SoundResourceDirector_h__

#include <Workphone/Scene/Directors/ResourceDirector.hpp>

namespace workphone
{
    namespace scene
    {

        /** Sound resource director implementation. */
        class WPCore_API SoundResourceDirector : public ResourceDirector
        {
        public:
            /** Property key for play button. */
            static const String playStr;

            /** Property key for stop button. */
            static const String stopStr;

            /** Property key for save button. */
            static const String saveStr;

            /** Property key for import button. */
            static const String importStr;

            /** Constructor. */
            SoundResourceDirector();

            /** Destructor. */
            ~SoundResourceDirector() override;

            /** @copydoc IBuildDirector::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IBuildDirector::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IBuildDirector::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IBuildDirector::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** Plays the sound. */
            void play();

            /** Stops the sound. */
            void stop();

            /** Returns the sound object created when previewing. */
            SmartPtr<ISound> getSound() const;

            /** Sets the sound object created when previewing. */
            void setSound( SmartPtr<ISound> sound );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** The sound object created when previewing. */
            SmartPtr<ISound> m_sound;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // SoundResourceDirector_h__
