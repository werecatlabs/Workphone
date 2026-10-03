#ifndef Animation_h__
#define Animation_h__

#include <Workphone/Scene/Components/SubComponent.hpp>

namespace workphone
{
    namespace scene
    {
        /** @brief Animation subcomponent.
         */
        class WPCore_API Animation : public SubComponent
        {
        public:
            static const String nameStr;
            static const String lengthStr;
            static const String loopingStr;
            static const String speedStr;

            /** @brief Constructor. */
            Animation();

            /** @brief Destructor. */
            ~Animation() override;

            /** @copydoc SubComponent::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc SubComponent::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc SubComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc SubComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** Gets the animation clip name used to look up the animation resource. */
            String getName() const override;

            /** Sets the animation clip name. */
            void setName( const String &name ) override;

            /** Gets the length of the animation clip in seconds. */
            f32 getLength() const;

            /** Sets the length of the animation clip in seconds. */
            void setLength( f32 length );

            /** Returns true if the animation loops when it reaches the end. */
            bool isLooping() const;

            /** Sets whether the animation loops when it reaches the end. */
            void setLooping( bool looping );

            /** Gets the playback speed multiplier (1.0 = normal speed). */
            f32 getSpeed() const;

            /** Sets the playback speed multiplier. */
            void setSpeed( f32 speed );

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_name;
            f32 m_length = 0.0f;
            bool m_looping = false;
            f32 m_speed = 1.0f;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // Animation_h__
