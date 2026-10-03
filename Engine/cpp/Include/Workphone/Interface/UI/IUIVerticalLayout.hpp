#ifndef IUIVerticalLayout_h__
#define IUIVerticalLayout_h__

#include <Workphone/Interface/UI/IUILayoutContainer.hpp>

namespace workphone
{
    namespace ui
    {

        class WPCore_API IUIVerticalLayout : public IUILayoutContainer
        {
        public:
            IUIVerticalLayout();

            IUIVerticalLayout( u32 poolTypeId );

            /** Virtual destructor. */
            ~IUIVerticalLayout() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // IUIVerticalLayout_h__
