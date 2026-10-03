#ifndef UIDropdown_h__
#define UIDropdown_h__

#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Interface/UI/IUIDropdown.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class UIDropdown
         * @brief UI element that provides a dropdown list for selecting a single option.
         *
         * This class implements the IUIDropdown interface and manages a list of options
         * that the user can choose from, providing methods to manipulate the options list
         * and track the currently selected item.
         */
        class WPCore_API UIDropdown : public UIElement<IUIDropdown>
        {
        public:
            /**
             * @brief Constructs a new UIDropdown instance.
             */
            UIDropdown();

            /**
             * @brief Destroys the UIDropdown instance.
             */
            ~UIDropdown() override;

            /**
             * @brief Loads data into the dropdown element.
             * @param data Smart pointer to the shared object containing the data to load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads data from the dropdown element.
             * @param data Smart pointer to the shared object containing the data to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Retrieves the current list of options available in the dropdown.
             * @return An array of strings containing the options.
             */
            Array<String> getOptions() const override;

            /**
             * @brief Sets the list of options available in the dropdown.
             * @param options An array of strings to set as the new options list.
             */
            void setOptions( const Array<String> &options ) override;

            /**
             * @brief Adds a new option to the existing dropdown list.
             * @param option The string representing the new option.
             */
            void addOption( const String &option ) override;

            /**
             * @brief Retrieves the index of the currently selected option.
             * @return The index of the selected option (0-based).
             */
            u32 getSelectedOption() const override;

            /**
             * @brief Sets the currently selected option by its index.
             * @param selectedOption The index of the option to select.
             */
            void setSelectedOption( u32 selectedOption ) override;

            /**
             * @brief Marks the dropdown as invalid and triggers a redraw or update.
             */
            void invalidate() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UIDropdown_h__
