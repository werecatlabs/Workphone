#ifndef TextComponent_h__
#define TextComponent_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Text component for UI elements.
         *
         * This component represents a text label or field in the UI, allowing customization of text
         * content, alignment, color, and size. It manages the underlying UI text object and provides
         * property accessors.
         */
        class WPCore_API Text : public UIComponent
        {
        public:
            // Static const property key strings
            static const String textPropertyStr;
            static const String sizePropertyStr;
            static const String colourPropertyStr;
            static const String verticalAlignmentPropertyStr;
            static const String horizontalAlignmentPropertyStr;

            /**
             * @brief Default constructor.
             */
            Text();

            /**
             * @brief Destructor.
             */
            ~Text() override;

            /**
             * @brief Loads the component with the given data.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the component and releases resources.
             * @param data Shared object containing unload data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the component's flags.
             * @param flags New flags to set.
             * @param oldFlags Previous flags.
             */
            void updateFlags( u32 flags, u32 oldFlags ) override;

            /**
             * @brief Gets the child shared objects of this component.
             * @return Array of child shared objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Gets the properties of this component.
             * @return Smart pointer to the properties object.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets the properties of this component.
             * @param properties Smart pointer to the properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Updates the state of the UI element, such as text or alignment.
             */
            void updateElementState() override;

            /**
             * @brief Gets the underlying UI text object.
             * @return Smart pointer to the UI text object.
             */
            SmartPtr<ui::IUIText> getTextObject() const;

            /**
             * @brief Sets the underlying UI text object.
             * @param textObject Smart pointer to the UI text object.
             */
            void setTextObject( SmartPtr<ui::IUIText> textObject );

            /**
             * @brief Gets the current text string.
             * @return The text string.
             */
            String getText() const;

            /**
             * @brief Sets the text string.
             * @param text The new text string.
             */
            void setText( const String &text );

            /**
             * @brief Gets the vertical alignment of the text.
             * @return The vertical alignment value.
             */
            u8 getVerticalAlignment() const;

            /**
             * @brief Sets the vertical alignment of the text.
             * @param verticalAlignment The vertical alignment value.
             */
            void setVerticalAlignment( u8 verticalAlignment );

            /**
             * @brief Gets the horizontal alignment of the text.
             * @return The horizontal alignment value.
             */
            u8 getHorizontalAlignment() const;

            /**
             * @brief Sets the horizontal alignment of the text.
             * @param horizontalAlignment The horizontal alignment value.
             */
            void setHorizontalAlignment( u8 horizontalAlignment );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Creates the UI text object and initializes it.
             */
            void createUI() override;

            /**
             * @brief The underlying UI text object.
             */
            SmartPtr<ui::IUIText> m_textObject;

            /**
             * @brief The font size of the text (default: 12).
             */
            u32 m_size = 12;

            /**
             * @brief The vertical alignment of the text
             */
            u8 m_verticalAlignment = (u8)VerticalAlignment::CENTER;

            /**
             * @brief The horizontal alignment of the text
             */
            u8 m_horizontalAlignment = (u8)HorizontalAlignment::CENTER;

            /**
             * @brief The text string displayed by this component.
             */
            String m_text;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // TextComponent_h__
