#ifndef IUIHorizontalLayout_h__
#define IUIHorizontalLayout_h__

#include <Workphone/Interface/UI/IUILayoutContainer.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for a horizontal layout. */
        class WPCore_API IUIHorizontalLayout : public IUILayoutContainer
        {
        public:
            IUIHorizontalLayout();

            IUIHorizontalLayout( u32 poolTypeId );

            /** Virtual destructor. */
            ~IUIHorizontalLayout() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // IUIHorizontalLayout_h__
