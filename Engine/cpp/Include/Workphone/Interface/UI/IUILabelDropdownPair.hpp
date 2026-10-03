#ifndef IUILabelDropdownPair_h__
#define IUILabelDropdownPair_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for a label and dropdown pair. */
        class WPCore_API IUILabelDropdownPair : public IUIElement
        {
        public:
            IUILabelDropdownPair();

            IUILabelDropdownPair( u32 poolTypeId );

            /** Destructor. */
            ~IUILabelDropdownPair() override;

            String getLabel() const override = 0;

            void setLabel( const String &label ) override = 0;

            /**
             * @brief Gets the list of available options
             * @return The list of available options
             */
            virtual Array<String> getOptions() const = 0;

            /**
             * @brief Sets the list of available options
             * @param options The list of available options to set
             */
            virtual void setOptions( const Array<String> &options ) = 0;

            /**
             * @brief Adds an option to the list of available options
             * @param option The option to add
             */
            virtual void addOption( const String &option ) = 0;

            /**
             * @brief Removes an option from the list of available options
             * @param option The option to remove
             */
            virtual void removeOption( const String &option ) = 0;

            /**
             * @brief Gets the index of the currently selected option
             * @return The index of the currently selected option
             */
            virtual u32 getSelectedOption() const = 0;

            /**
             * @brief Sets the index of the currently selected option
             * @param selectedOption The index of the option to select
             */
            virtual void setSelectedOption( u32 selectedOption ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // IUILabelDropdownPair_h__
