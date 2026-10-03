#ifndef _IGUICONTAINER_H
#define _IGUICONTAINER_H

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        class WPCore_API IUILayoutContainer : public IUIElement
        {
        public:
            IUILayoutContainer();

            IUILayoutContainer( u32 poolTypeId );

            /** Virtual destructor. */
            ~IUILayoutContainer() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif
