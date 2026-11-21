#ifndef IGUIScrollingText_h__
#define IGUIScrollingText_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for a scrolling text element. */
        class WPCore_API IUIScrollingText : public IUIElement
        {
        public:
            IUIScrollingText();

            IUIScrollingText( u32 poolTypeId );

            /** Destructor. */
            ~IUIScrollingText() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IGUIScrollingText_h__
