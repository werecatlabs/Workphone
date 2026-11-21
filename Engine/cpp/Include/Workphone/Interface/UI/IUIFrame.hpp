#ifndef WP_IUIFRAME_H
#define WP_IUIFRAME_H

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for a ui frame. */
        class WPCore_API IUIFrame : public IUIElement
        {
        public:
            IUIFrame();

            IUIFrame( u32 poolTypeId );

            /** Destructor. */
            ~IUIFrame() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // WP_IUIFRAME_H
