#ifndef TextDirector_h__
#define TextDirector_h__

#include <Workphone/Scene/Directors/UiElementDirector.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class TextDirector
         * @brief Director responsible for configuring text UI elements.
         *
         * TextDirector exposes properties used to control text layout and appearance
         * for UI elements such as alignment and font size. Properties are serializable
         * via the Properties API and used when instancing text widgets in the UI.
         */
        class WPCore_API TextDirector : public UiElementDirector
        {
        public:
            /** Property key for vertical alignment in Properties objects. */
            static const String VerticalAlignmentStr;

            /** Property key for horizontal alignment in Properties objects. */
            static const String HorizontalAlignmentStr;

            /** Property key for text size in Properties objects. */
            static const String TextSizeStr;

            /** Property key for a save button identifier used by certain text widgets. */
            static const String SaveButtonStr;

            /** Property key for an import button identifier used by certain text widgets. */
            static const String ImportButtonStr;

            /** Property key for save action text. */
            static const String SaveStr;

            /** Property key for import action text. */
            static const String ImportStr;

            /**
             * @brief Construct a TextDirector using default alignment and text size.
             */
            TextDirector();

            /**
             * @brief Destructor.
             */
            ~TextDirector() override;

            /**
             * @brief Retrieve serializable properties for this director.
             * @return SmartPtr<Properties> containing vertical/horizontal alignment and text size.
             * @copydoc UiElementDirector::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply properties to configure this director.
             * @param properties Properties object containing alignment and text-size values.
             * @copydoc UiElementDirector::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the vertical alignment value used by text elements.
             * @return Numeric code representing vertical alignment (implementation-defined).
             */
            u32 getVerticalAlignment() const;

            /**
             * @brief Set the vertical alignment for text elements.
             * @param verticalAlignment Numeric code representing the desired alignment.
             */
            void setVerticalAlignment( u32 verticalAlignment );

            /**
             * @brief Get the horizontal alignment value used by text elements.
             * @return Numeric code representing horizontal alignment (implementation-defined).
             */
            u32 getHorizontalAlignment() const;

            /**
             * @brief Set the horizontal alignment for text elements.
             * @param horizontalAlignment Numeric code representing the desired alignment.
             */
            void setHorizontalAlignment( u32 horizontalAlignment );

            /**
             * @brief Get the configured text size for UI text elements.
             * @return Text size in pixels (or logical units depending on UI system).
             */
            u32 getTextSize() const;

            /**
             * @brief Set the text size used by UI text elements.
             * @param textSize Desired text size in pixels (or logical units).
             */
            void setTextSize( u32 textSize );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Vertical alignment code (implementation-defined). Default: 0. */
            u32 m_verticalAlignment = 0;

            /** Horizontal alignment code (implementation-defined). Default: 0. */
            u32 m_horizontalAlignment = 0;

            /** Text size used for labels. Default: 12. */
            u32 m_textSize = 12;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // TextDirector_h__
