#ifndef AnimatedMaterial_h__
#define AnimatedMaterial_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/Animation/IAnimator.hpp>

namespace workphone
{
    namespace scene
    {
        /** Animated material component. */
        class WPCore_API AnimatedMaterial : public Component
        {
        public:
            static const String materialNameStr;
            static const String animatorStr;

            /** Constructor. */
            AnimatedMaterial();

            /** Destructor. */
            ~AnimatedMaterial() override;

            /** @copydoc Component::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** Advance the animation by one tick. */
            void update() override;

            /** Starts playback of the material animation. */
            void play();

            /** Pauses playback of the material animation. */
            void pause();

            /** Stops playback and resets the material animation. */
            void stop();

            /** Returns true if the animation is currently playing. */
            bool isPlaying() const;

            /** Gets the name of the material to animate. */
            String getMaterialName() const;

            /** Sets the name of the material to animate. */
            void setMaterialName( const String &materialName );

            /** @copydoc Component::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Component::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** Gets the animator. */
            SmartPtr<IAnimator> getAnimator() const;

            /** Sets the animator. */
            void setAnimator( SmartPtr<IAnimator> animator );

            /** @copydoc Component::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_materialName;
            SmartPtr<IAnimator> m_animator;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // AnimatedMaterial_h__
