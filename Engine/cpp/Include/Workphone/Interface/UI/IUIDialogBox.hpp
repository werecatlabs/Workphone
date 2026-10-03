#ifndef IGUIDialogBox_h__
#define IGUIDialogBox_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class IUIDialogBox
         * @brief Interface for a dialog box, responsible for managing dialog box display in the user
         * interface
         */
        class WPCore_API IUIDialogBox : public IUIElement
        {
        public:
            IUIDialogBox();

            IUIDialogBox( u32 poolTypeId );

            /**
             * @brief Virtual destructor
             */
            ~IUIDialogBox() override;

            /**
             * @brief Shows the dialog box, does not block; use the listener to handle events
             * @return A boolean indicating whether the operation was successful
             */
            virtual bool show() = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IGUIDialogBox_h__
