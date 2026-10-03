#ifndef UIVerticalLayout_h__
#define UIVerticalLayout_h__

#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Interface/UI/IUIVerticalLayout.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         */
        class WPCore_API UIVerticalLayout : public UIElement<IUIVerticalLayout>
        {
        public:
            UIVerticalLayout();
            ~UIVerticalLayout() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UIVerticalLayout_h__
