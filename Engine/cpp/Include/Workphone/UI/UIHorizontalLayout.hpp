#ifndef UIHorizontalLayout_h__
#define UIHorizontalLayout_h__

#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Interface/UI/IUIHorizontalLayout.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         */
        class WPCore_API UIHorizontalLayout : public UIElement<IUIHorizontalLayout>
        {
        public:
            UIHorizontalLayout();
            ~UIHorizontalLayout() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UIHorizontalLayout_h__
