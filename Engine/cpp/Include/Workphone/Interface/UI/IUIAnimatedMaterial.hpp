#ifndef IAnimatedMaterial_h__
#define IAnimatedMaterial_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        class WPCore_API IUIAnimatedMaterial : public IUIElement
        {
        public:
            IUIAnimatedMaterial();

            IUIAnimatedMaterial( u32 poolTypeId );

            ~IUIAnimatedMaterial() override;

            /** Sets the material used. */
            virtual void setMaterialName( const String &materialName ) = 0;

            /** Gets the name of the material used. */
            virtual String getMaterialName() const = 0;

            /** Plays the animation. */
            virtual void play() = 0;

            /** Pauses the animation. */
            virtual void pause() = 0;

            /** Stops the animation. */
            virtual void stop() = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // IAnimatedMaterial_h__
