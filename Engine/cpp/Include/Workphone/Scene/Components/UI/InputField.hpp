#ifndef InputFieldComponent_h__
#define InputFieldComponent_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>
#include <Workphone/Interface/UI/IUITextEntry.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief InputField component for UI text input elements.
         *
         * This component represents a text input field in the UI, allowing users to enter and edit text.
         * It supports features like placeholder text, read-only mode, secure (password) entry,
         * multiline input, and different input types for mobile keyboards.
         */
        class WPCore_API InputField : public UIComponent
        {
        public:
            // Property string constants
            static const String textStr;
            static const String placeholderStr;
            static const String readOnlyStr;
            static const String secureEntryStr;
            static const String multilineStr;
            static const String inputTypeStr;
            static const String textHintStr;
            static const String textSizeStr;

            /** Constructor. */
            InputField();

            /** Destructor. */
            ~InputField() override;

            /** @copydoc UIComponent::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc UIComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc UIComponent::updateElementState */
            void updateElementState() override;

            /**
             * @brief Gets the underlying UI text entry object.
             * @return Smart pointer to the UI text entry object.
             */
            SmartPtr<ui::IUITextEntry> getTextEntry() const;

            /**
             * @brief Sets the underlying UI text entry object.
             * @param textEntry Smart pointer to the UI text entry object.
             */
            void setTextEntry( SmartPtr<ui::IUITextEntry> textEntry );

            /**
             * @brief Gets the current text in the input field.
             * @return The text string.
             */
            String getText() const;

            /**
             * @brief Sets the text in the input field.
             * @param text The new text string.
             */
            void setText( const String &text );

            /**
             * @brief Gets the placeholder text.
             * @return The placeholder text string.
             */
            String getPlaceholder() const;

            /**
             * @brief Sets the placeholder text that is shown when the field is empty.
             * @param placeholder The placeholder text to display.
             */
            void setPlaceholder( const String &placeholder );

            /**
             * @brief Gets whether the input field is read-only.
             * @return True if the field is read-only, false otherwise.
             */
            bool isReadOnly() const;

            /**
             * @brief Sets whether the input field is read-only.
             * @param readOnly True to make the field read-only, false to allow editing.
             */
            void setReadOnly( bool readOnly );

            /**
             * @brief Gets whether the input field is in secure (password) mode.
             * @return True if secure entry is enabled, false otherwise.
             */
            bool isSecureEntry() const;

            /**
             * @brief Sets whether the input field is in secure (password) mode.
             * @param secureEntry True to enable password/secure entry mode.
             */
            void setSecureEntry( bool secureEntry );

            /**
             * @brief Gets whether the input field supports multiple lines.
             * @return True if multiline is enabled, false otherwise.
             */
            bool isMultiline() const;

            /**
             * @brief Sets whether the input field supports multiple lines.
             * @param multiline True to enable multiline input, false for single line.
             */
            void setMultiline( bool multiline );

            /**
             * @brief Gets the current input type.
             * @return The input type.
             */
            ui::IUITextEntry::InputType getInputType() const;

            /**
             * @brief Sets the input type and optional text hint for soft keyboards.
             * @param inputType The input type (text, password, email, etc.).
             * @param textHint The hint to display on soft keyboards (optional).
             */
            void setInputType( ui::IUITextEntry::InputType inputType, const String &textHint = "" );

            /**
             * @brief Gets the text hint for soft keyboards.
             * @return The text hint string.
             */
            String getTextHint() const;

            /**
             * @brief Gets the text size.
             * @return The text size value.
             */
            u32 getTextSize() const;

            /**
             * @brief Sets the text size.
             * @param textSize The new text size value.
             */
            void setTextSize( u32 textSize );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @copydoc UIComponent::createUI */
            void createUI() override;

            /** The underlying UI text entry object */
            SmartPtr<ui::IUITextEntry> m_textEntry;

            /** The current text in the input field */
            String m_text;

            /** The placeholder text shown when the field is empty */
            String m_placeholder;

            /** Whether the field is read-only */
            bool m_readOnly;

            /** Whether the field is in secure (password) mode */
            bool m_secureEntry;

            /** Whether the field supports multiple lines */
            bool m_multiline;

            /** The input type for soft keyboard hints */
            ui::IUITextEntry::InputType m_inputType;

            /** The text hint for soft keyboards */
            String m_textHint;

            /** The text size */
            u32 m_textSize;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // InputFieldComponent_h__
