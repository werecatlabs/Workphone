#ifndef IUIToolbar_h__
#define IUIToolbar_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** @brief Interface for a toolbar.
         */
        class WPCore_API IUIToolbar : public IUIElement
        {
        public:
            IUIToolbar();

            IUIToolbar( u32 poolTypeId );

            /* @brief Virtual destructor.
             */
            ~IUIToolbar() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // IUIToolbar_h__
