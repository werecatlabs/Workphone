#ifndef IUITabItem_h__
#define IUITabItem_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for a tab item. */
        class WPCore_API IUITabItem : public IUIElement
        {
        public:
            IUITabItem() : IUIElement( IUITabItem::typeInfo() )
            {
            }

            IUITabItem( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /** Destructor. */
            ~IUITabItem() override;

            /** Returns the label of the tab item.
             * @return The label of the tab item.
             */
            String getLabel() const override = 0;

            /** Sets the label of the tab item.
             * @param label The label of the tab item.
             */
            void setLabel( const String &label ) override = 0;

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_label;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUITabItem_h__
