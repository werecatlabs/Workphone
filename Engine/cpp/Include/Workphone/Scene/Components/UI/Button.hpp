#ifndef ButtonComponent_h__
#define ButtonComponent_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class Button
         * @brief UI Button component for creating interactive buttons in the UI.
         *
         * This component provides functionality for displaying a button with customizable text, image,
         * and colors for different states. It supports event handling and allows setting a callback
         * function to be invoked on button actions.
         *
         * @note Inherits from UIComponent.
         */
        class WPCore_API Button : public UIComponent
        {
        public:
            /** @brief String identifier for the image property. */
            static const String imageStr;
            /** @brief String identifier for the text property. */
            static const String textStr;
            /** @brief String identifier for the normal color property. */
            static const String normalColourStr;
            /** @brief String identifier for the highlighted color property. */
            static const String highlightedColourStr;
            /** @brief String identifier for the pressed color property. */
            static const String pressedColourStr;
            /** @brief String identifier for the disabled color property. */
            static const String disabledColourStr;
            /** @brief String identifier for the text size property. */
            static const String textSizeStr;

            /**
             * @brief Default constructor.
             */
            Button();

            /**
             * @brief Destructor.
             */
            ~Button() override;

            /**
             * @brief Loads the button component with the given data.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the button component and releases resources.
             * @param data Shared object containing unload data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the child objects of this component.
             * @return Array of shared pointers to child objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Gets the properties of the button component.
             * @return Shared pointer to the properties object.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets the properties of the button component.
             * @param properties Shared pointer to the properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Gets the image associated with the button.
             * @return Shared pointer to the Image object.
             */
            SmartPtr<Image> getImage() const;

            /**
             * @brief Sets the image for the button.
             * @param image Shared pointer to the Image object.
             */
            void setImage( SmartPtr<Image> image );

            /**
             * @brief Gets the text associated with the button.
             * @return Shared pointer to the Text object.
             */
            SmartPtr<Text> getText() const;

            /**
             * @brief Sets the text for the button.
             * @param text Shared pointer to the Text object.
             */
            void setText( SmartPtr<Text> text );

            /**
             * @brief Gets the button's text as a string.
             * @return The text string.
             */
            String getTextStr() const;

            /**
             * @brief Sets the button's text as a string.
             * @param textStr The text string to set.
             */
            void setTextStr( const String &textStr );

            /**
             * @brief Gets the size of the button's text.
             * @return The text size.
             */
            u32 getTextSize() const;

            /**
             * @brief Sets the size of the button's text.
             * @param textSize The text size to set.
             */
            void setTextSize( u32 textSize );

            /**
             * @brief Gets the normal color of the button.
             * @return The normal color.
             */
            ColourF getNormalColour() const;

            /**
             * @brief Sets the normal color of the button.
             * @param normalColour The color to set for the normal state.
             */
            void setNormalColour( const ColourF &normalColour );

            /**
             * @brief Gets the highlighted color of the button.
             * @return The highlighted color.
             */
            ColourF getHighlightedColour() const;

            /**
             * @brief Sets the highlighted color of the button.
             * @param highlightedColour The color to set for the highlighted state.
             */
            void setHighlightedColour( const ColourF &highlightedColour );

            /**
             * @brief Gets the pressed color of the button.
             * @return The pressed color.
             */
            ColourF getPressedColour() const;

            /**
             * @brief Sets the pressed color of the button.
             * @param pressedColour The color to set for the pressed state.
             */
            void setPressedColour( const ColourF &pressedColour );

            /**
             * @brief Gets the disabled color of the button.
             * @return The disabled color.
             */
            ColourF getDisabledColour() const;

            /**
             * @brief Sets the disabled color of the button.
             * @param disabledColour The color to set for the disabled state.
             */
            void setDisabledColour( const ColourF &disabledColour );

            /**
             * @brief Handles an input event for the button.
             * @param event Shared pointer to the input event.
             * @return True if the event was handled, false otherwise.
             */
            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            /**
             * @brief Sets a callback function to be called on button events.
             * @param callbackFunction The callback function taking a hash_type argument.
             */
            void setCallbackFunction( std::function<void( hash_type )> callbackFunction );

            /**
             * @brief Handles a generic event for the button.
             * @param eventType The type of the event.
             * @param eventValue The value associated with the event.
             * @param arguments Additional event arguments.
             * @param sender The sender object.
             * @param object The target object.
             * @param event Shared pointer to the event object.
             * @return Parameter object with event handling result.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Creates the UI elements for the button.
             */
            void createUI() override;

            /**
             * @brief Updates the visual state of the button element.
             */
            void updateElementState() override;

            /** @brief The color of the button in its normal state. */
            AtomicObject<ColourF> m_normalColour = ColourF::White;

            /** @brief The color of the button when highlighted. */
            AtomicObject<ColourF> m_highlightedColour = ColourF::White;

            /** @brief The color of the button when pressed. */
            AtomicObject<ColourF> m_pressedColour = ColourF::White;

            /** @brief The color of the button when disabled. */
            AtomicObject<ColourF> m_disabledColour = ColourF::White;

            /** @brief The image displayed on the button. */
            AtomicSmartPtr<Image> m_image;

            /** @brief The text displayed on the button. */
            AtomicSmartPtr<Text> m_text;

            /** @brief The size of the button's text. */
            atomic_u32 m_textSize = 12;

            /** @brief The callback function to be called on button events. */
            std::function<void( hash_type )> m_callbackFunction;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // ButtonComponent_h__
