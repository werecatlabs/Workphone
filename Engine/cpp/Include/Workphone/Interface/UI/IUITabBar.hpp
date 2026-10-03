#ifndef IUITabBar_h__
#define IUITabBar_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for a tabbar. */
        class WPCore_API IUITabBar : public IUIElement
        {
        public:
            IUITabBar() : IUIElement( IUITabBar::typeInfo() )
            {
            }

            IUITabBar( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /** Destructor. */
            ~IUITabBar() override;

            /** Adds a new tab item to the tab bar.
             * @return The new tab item.
             */
            virtual SmartPtr<IUITabItem> addTabItem() = 0;

            /** Removes a tab item from the tab bar.
             * @param tabItem The tab item to remove.
             */
            virtual void removeTabItem( SmartPtr<IUITabItem> tabItem ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUITabBar_h__
