#ifndef LayoutContainer_h__
#define LayoutContainer_h__

#include <Workphone/Scene/Components/Component.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @class LayoutContainer
         * @brief A base class for UI layout containers that manage the arrangement of child components.
         *
         * This class provides functionality for spacing, alignment, and offsetting of child UI elements.
         * It is intended to be subclassed by specific layout types (e.g., horizontal, vertical, grid).
         *
         * @note Used as a base class for layout components.
         */
        class WPCore_API LayoutContainer : public Component
        {
        public:
            static const String offsetStr;
            static const String spacingStr;
            static const String childVerticalAlignmentStr;
            static const String childHorizontalAlignmentStr;
            static const String useChildStartOffsetStr;
            static const String useChildVerticalAlignmentStr;
            static const String useChildHorizontalAlignmentStr;

            /**
             * @brief Default constructor.
             */
            LayoutContainer();

            /**
             * @brief Destructor.
             */
            ~LayoutContainer() override;

            /**
             * @brief Loads the layout container with the given data.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the layout container and releases resources.
             * @param data Shared object containing unload data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the component's flags.
             * @param flags The new flags to set.
             * @param oldFlags The previous flags.
             */
            void updateFlags( u32 flags, u32 oldFlags ) override;

            /**
             * @brief Updates the transform of the layout container and its children.
             */
            void updateTransform() override;

            /**
             * @brief Gets the properties of the layout container.
             * @return A smart pointer to the properties object.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets the properties of the layout container.
             * @param properties A smart pointer to the properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Gets the spacing between child elements.
             * @return The spacing value in layout units.
             */
            f32 getSpacing() const;

            /**
             * @brief Sets the spacing between child elements.
             * @param spacing The spacing value in layout units.
             */
            void setSpacing( f32 spacing );

            /*
             * @brief Gets the padding applied to the layout container.
             */
            f32 getPadding() const;

            /**
             * @brief Sets the padding applied to the layout container.
             * @param padding The padding value in layout units.
             */
            void setPadding( f32 padding );

            /**
             * @brief Gets the horizontal alignment for child elements.
             * @return The horizontal alignment setting.
             */
            HorizontalAlignment getChildHorizontalAlignment() const;

            /**
             * @brief Sets the horizontal alignment for child elements.
             * @param childHorizontalAlignment The horizontal alignment to use.
             */
            void setChildHorizontalAlignment( HorizontalAlignment childHorizontalAlignment );

            /**
             * @brief Gets the vertical alignment for child elements.
             * @return The vertical alignment setting.
             */
            VerticalAlignment getChildVerticalAlignment() const;

            /**
             * @brief Sets the vertical alignment for child elements.
             * @param childVerticalAlignment The vertical alignment to use.
             */
            void setChildVerticalAlignment( VerticalAlignment childVerticalAlignment );

            /**
             * @brief Gets the offset applied to child elements.
             * @return The offset value in layout units.
             */
            f32 getOffset() const;

            /**
             * @brief Sets the offset applied to child elements.
             * @param offset The offset value in layout units.
             */
            void setOffset( f32 offset );

            /**
             * @brief Checks if child horizontal alignment is used.
             * @return True if child horizontal alignment is enabled, false otherwise.
             */
            bool getUseChildHorizontalAlignment() const;

            /**
             * @brief Enables or disables child horizontal alignment.
             * @param useChildHorizontalAlignment True to enable, false to disable.
             */
            void setUseChildHorizontalAlignment( bool useChildHorizontalAlignment );

            /**
             * @brief Checks if child vertical alignment is used.
             * @return True if child vertical alignment is enabled, false otherwise.
             */
            bool getUseChildVerticalAlignment() const;

            /**
             * @brief Enables or disables child vertical alignment.
             * @param useChildVerticalAlignment True to enable, false to disable.
             */
            void setUseChildVerticalAlignment( bool useChildVerticalAlignment );

            /**
             * @brief Checks if the start offset for child elements is used.
             * @return True if the start offset is enabled, false otherwise.
             */
            bool getUseChildStartOffset() const;

            /**
             * @brief Enables or disables the use of start offset for child elements.
             * @param useChildStartOffset True to enable, false to disable.
             */
            void setUseChildStartOffset( bool useChildStartOffset );

            WP_CLASS_REGISTER_DECL;

        protected:
            /*
             * @brief The padding applied to the layout container.
             */
            f32 m_padding = 0.0f;

            /**
             * @brief The offset applied to the position of child elements.
             */
            f32 m_offset = 0.0f;

            /**
             * @brief The spacing between child elements.
             */
            f32 m_spacing = 5.0f;

            /**
             * @brief The horizontal alignment setting for child elements.
             */
            HorizontalAlignment m_childHorizontalAlignment = HorizontalAlignment::CENTER;

            /**
             * @brief The vertical alignment setting for child elements.
             */
            VerticalAlignment m_childVerticalAlignment = VerticalAlignment::CENTER;

            /**
             * @brief Whether to use the child horizontal alignment setting.
             */
            bool m_useChildHorizontalAlignment = true;

            /**
             * @brief Whether to use the child vertical alignment setting.
             */
            bool m_useChildVerticalAlignment = true;

            /**
             * @brief Whether to use the start offset for child elements.
             */
            bool m_useChildStartOffset = true;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // LayoutContainer_h__
