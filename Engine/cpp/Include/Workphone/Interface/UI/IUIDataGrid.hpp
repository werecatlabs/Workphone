#ifndef IUIDataGrid_h__
#define IUIDataGrid_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** A class representing a data grid in the UI.
         * This class is used to display and manage tabular data in a grid format.
         * It inherits from IUIElement, which provides basic UI element functionality.
         */
        class WPCore_API IUIDataGrid : public IUIElement
        {
        public:
            IUIDataGrid();

            IUIDataGrid( u32 poolTypeId );

            ~IUIDataGrid() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // IUIDataGrid_h__
