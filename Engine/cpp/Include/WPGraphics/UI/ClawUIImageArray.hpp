#ifndef ClawUIImageArray_h__
#define ClawUIImageArray_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIImageArray.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUIImageArray : public ClawUIElement<IUIImageArray>
        {
        public:
            ClawUIImageArray();
            ~ClawUIImageArray() override;

            SmartPtr<IUIImage> getImage( u32 index ) override;

            void draw( struct wp_context *ctx ) override;

        private:
            Array<SmartPtr<IUIImage>> m_images;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ClawUIImageArray_h__
