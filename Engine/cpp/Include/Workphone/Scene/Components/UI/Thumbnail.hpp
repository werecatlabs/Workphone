#ifndef Thumbnail_h__
#define Thumbnail_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief UI component representing a thumbnail with optional label and highlight features.
         *
         * The Thumbnail component displays an image (thumbnail) with an optional label and supports
         * highlight visuals, such as a highlight image or color. It is typically used in UI lists,
         * galleries, or selection grids.
         */
        class WPCore_API Thumbnail : public UIComponent
        {
        public:
            static const String thumbStr;
            static const String labelTextStr;
            static const String highlightObjectStr;
            static const String highlightImageStr;
            static const String highlightColorStr;
            static const String normalColorStr;

            /**
             * @brief Constructs a new Thumbnail component.
             */
            Thumbnail();

            /**
             * @brief Destroys the Thumbnail component.
             */
            ~Thumbnail() override;

            /**
             * @brief Loads the component with the given data.
             * @param data Shared object containing initialization data.
             *
             * @copydoc UIComponent::load
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the component and releases resources.
             * @param data Shared object containing unload data.
             *
             * @copydoc UIComponent::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the child objects of this component.
             * @return Array of shared pointers to child objects.
             *
             * @copydoc UIComponent::getChildObjects
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Gets the properties of this component.
             * @return Shared pointer to the properties object.
             *
             * @copydoc UIComponent::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets the properties of this component.
             * @param properties Shared pointer to the properties object.
             *
             * @copydoc UIComponent::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Gets the thumbnail image.
             * @return Shared pointer to the Image object representing the thumbnail.
             */
            SmartPtr<Image> getThumb() const;

            /**
             * @brief Sets the thumbnail image.
             * @param thumb Shared pointer to the Image object to use as the thumbnail.
             */
            void setThumb( SmartPtr<Image> thumb );

            /**
             * @brief Gets the label text associated with the thumbnail.
             * @return Shared pointer to the Text object for the label.
             */
            SmartPtr<Text> getLabelText() const;

            /**
             * @brief Sets the label text for the thumbnail.
             * @param labelText Shared pointer to the Text object to use as the label.
             */
            void setLabelText( SmartPtr<Text> labelText );

            /**
             * @brief Gets the highlight object associated with the thumbnail.
             * @return Shared pointer to the IActor object used for highlighting.
             */
            SmartPtr<IGameActor> getHighlightObject() const;

            /**
             * @brief Sets the highlight object for the thumbnail.
             * @param highlightObject Shared pointer to the IActor object to use for highlighting.
             */
            void setHighlightObject( SmartPtr<IGameActor> highlightObject );

            /**
             * @brief Gets the highlight image.
             * @return Shared pointer to the Image object used for highlight visuals.
             */
            SmartPtr<Image> getHighlightImage() const;

            /**
             * @brief Sets the highlight image.
             * @param highlightImage Shared pointer to the Image object to use for highlighting.
             */
            void setHighlightImage( SmartPtr<Image> highlightImage );

            /**
             * @brief Gets the highlight color.
             * @return The color used for highlighting the thumbnail.
             */
            ColourF getHighlightColor() const;

            /**
             * @brief Sets the highlight color.
             * @param highlightColor The color to use for highlighting the thumbnail.
             */
            void setHighlightColor( const ColourF &highlightColor );

            /**
             * @brief Gets the normal (non-highlighted) color.
             * @return The color used when the thumbnail is not highlighted.
             */
            ColourF getNormalColor() const;

            /**
             * @brief Sets the normal (non-highlighted) color.
             * @param normalColor The color to use when the thumbnail is not highlighted.
             */
            void setNormalColor( const ColourF &normalColor );

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<Image> m_thumb;                    ///< Thumbnail image.
            SmartPtr<Text> m_labelText;                 ///< Label text displayed with the thumbnail.
            SmartPtr<IGameActor> m_highlightObject;     ///< Object used for highlight effects.
            SmartPtr<Image> m_highlightImage;           ///< Image used for highlight visuals.
            ColourF m_highlightColor = ColourF::White;  ///< Color used for highlighting.
            ColourF m_normalColor = ColourF::White;     ///< Color used when not highlighted.
        };
    }  // namespace scene
}  // namespace workphone

#endif  // Thumbnail_h__
