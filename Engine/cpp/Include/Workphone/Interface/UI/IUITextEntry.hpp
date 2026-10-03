#ifndef __IUITextEntry_h__
#define __IUITextEntry_h__

#include <Workphone/Interface/UI/IUIText.hpp>

namespace workphone
{
    namespace ui
    {

        /** Interface for a text entry widget. */
        class WPCore_API IUITextEntry : public IUIText
        {
        public:
            /**
             * @brief Enum for input type, used for soft keyboard hints.
             */
            enum class InputType
            {
                Text,
                Multiline,
                Password,
                Email
            };

            IUITextEntry() : IUIText( IUITextEntry::typeInfo() )
            {
            }

            IUITextEntry( u32 poolTypeId ) : IUIText( poolTypeId )
            {
            }

            /** Destructor. */
            ~IUITextEntry() override;

            /**
             * @brief Sets the placeholder text that is shown when the entry is empty.
             * @param placeholder The placeholder text to display.
             */
            virtual void setPlaceholder( const String &placeholder ) = 0;

            /**
             * @brief Gets the placeholder text.
             * @return The current placeholder text.
             */
            virtual String getPlaceholder() const = 0;

            /**
             * @brief Sets whether the text entry is read-only.
             * @param readOnly True to make the entry read-only, false to allow editing.
             */
            virtual void setReadOnly( bool readOnly ) = 0;

            /**
             * @brief Gets whether the text entry is read-only.
             * @return True if the entry is read-only, false otherwise.
             */
            virtual bool isReadOnly() const = 0;

            /**
             * @brief Sets whether the text entry is in secure (password) mode.
             * @param secureEntry True to enable password/secure entry mode.
             */
            virtual void setSecureEntry( bool secureEntry ) = 0;

            /**
             * @brief Gets whether the text entry is in secure (password) mode.
             * @return True if secure entry is enabled, false otherwise.
             */
            virtual bool isSecureEntry() const = 0;

            /**
             * @brief Sets whether the text entry supports multiple lines.
             * @param multiline True to enable multiline input, false for single line.
             */
            virtual void setMultiline( bool multiline ) = 0;

            /**
             * @brief Gets whether the text entry supports multiple lines.
             * @return True if multiline is enabled, false otherwise.
             */
            virtual bool isMultiline() const = 0;

            /**
             * @brief Sets the input type and optional text hint for soft keyboards.
             * @param inputType The input type (text, password, email, etc.).
             * @param textHint The hint to display on soft keyboards (optional).
             */
            virtual void setInputType( InputType inputType, const String &textHint = "" ) = 0;

            /**
             * @brief Gets the current input type.
             * @return The input type.
             */
            virtual InputType getInputType() const = 0;

            /**
             * @brief Gets the text hint for soft keyboards.
             * @return The text hint string.
             */
            virtual String getTextHint() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IGUITextEntry_h__
