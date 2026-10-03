#ifndef AudioEmitter_h__
#define AudioEmitter_h__

#include <Workphone/Scene/Components/Component.hpp>

namespace workphone
{
    namespace scene
    {
        /** @brief Audio emitter component.
         *  This component is used to play sounds.
         */
        class WPCore_API AudioEmitter : public Component
        {
        public:
            static const String soundStr;
            static const String playStr;
            static const String stopStr;
            static const String pauseStr;
            static const String unpauseStr;

            /** @brief Constructor. */
            AudioEmitter();

            /** @brief Destructor. */
            ~AudioEmitter() override;

            /** @copydoc Component::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Component::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc Component::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @brief Gets the sound.
             *  @return The sound.
             */
            SmartPtr<ISound> getSound() const;

            /** @brief Sets the sound.
             *  @param sound The sound.
             */
            void setSound( SmartPtr<ISound> sound );

            /** @brief Plays the sound. */
            void play();

            /** @brief Stops the sound. */
            void stop();

            /** @brief Pauses the sound. */
            void pause();

            /** @brief Unpauses the sound. */
            void unpause();

            WP_CLASS_REGISTER_DECL;

        protected:
            /** The sound object. */
            SmartPtr<ISound> m_sound;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // AudioEmitter_h__
