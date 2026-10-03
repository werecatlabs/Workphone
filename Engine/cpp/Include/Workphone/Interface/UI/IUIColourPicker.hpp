#ifndef IUIColourPicker_h__
#define IUIColourPicker_h__

#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/Pair.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class IUIColourPicker
         * @brief Interface for a colour picker user interface element, which allows users to choose
         * colours from a gradient.
         */
        class WPCore_API IUIColourPicker : public IUIElement
        {
        public:
            /**
             * @brief Enumeration for the colour format options.
             */
            enum class ColourFormat
            {
                RGB, /**< RGB format */
                HSL, /**< HSL format */
                HEX  /**< HEX format */
            };

            IUIColourPicker();

            IUIColourPicker( u32 poolTypeId );

            /** Virtual destructor. */
            ~IUIColourPicker() override;

            /**
             * @brief Gets the label text of the button
             * @return The label text as a string
             */
            String getLabel() const override = 0;

            /**
             * @brief Sets the label text of the button
             * @param label The new label text for the button
             */
            void setLabel( const String &label ) override = 0;

            /**
             * @brief Sets the currently selected colour.
             * @param colour The new colour to set
             */
            void setColour( const ColourF &colour ) override = 0;

            /**
             * @brief Gets the currently selected colour.
             * @return The currently selected colour
             */
            ColourF getColour() const override = 0;

            /**
             * @brief Sets the gradient for the colour picker.
             * @param startColour The starting colour of the gradient
             * @param endColour The ending colour of the gradient
             */
            virtual void setGradient( const ColourF &startColour, const ColourF &endColour ) = 0;

            /**
             * @brief Gets the gradient for the colour picker.
             * @return A pair of the starting and ending colours of the gradient
             */
            virtual Pair<ColourF, ColourF> getGradient() const = 0;

            /**
             * @brief Sets the colour format for the colour picker.
             * @param format The colour format to set
             */
            virtual void setColourFormat( ColourFormat format ) = 0;

            /**
             * @brief Gets the colour format for the colour picker.
             * @return The colour format for the colour picker
             */
            virtual ColourFormat getColourFormat() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUIColourPicker_h__
