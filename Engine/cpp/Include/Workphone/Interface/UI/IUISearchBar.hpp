#ifndef __IUISearchBar_h__
#define __IUISearchBar_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        /** Interface for a search bar. */
        class WPCore_API IUISearchBar : public IUIElement
        {
        public:
            IUISearchBar() : IUIElement( IUISearchBar::typeInfo() )
            {
            }

            IUISearchBar( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /** Virtual destructor. */
            ~IUISearchBar() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // __IUISearchBar_h__
