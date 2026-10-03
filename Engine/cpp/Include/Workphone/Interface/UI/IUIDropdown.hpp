#ifndef IUIDropdown_h__
#define IUIDropdown_h__

#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class IUIDropdown
         * @brief Interface for a dropdown UI element that displays a list of options and allows the user
         * to select one
         */
        class WPCore_API IUIDropdown : public IUIElement
        {
        public:
            IUIDropdown();

            IUIDropdown( u32 poolTypeId );

            /**
             * @brief Destructor
             */
            ~IUIDropdown() override;

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

            /** Adds an option. */
            virtual void addOption( const String &option ) = 0;

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

#endif  // IUIDropdown_h__
