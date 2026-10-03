#ifndef UiElementDirector_h__
#define UiElementDirector_h__

#include <Workphone/System/Director.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class UiElementDirector
         * @brief Director that configures common UI element properties.
         *
         * UiElementDirector provides a centralized set of properties used by UI elements
         * such as panels, buttons and text labels. It exposes position, size, anchors,
         * colour, z-order, alignment and visibility flags and serializes them via the
         * Properties API.
         */
        class WPCore_API UiElementDirector : public Director
        {
        public:
            /** Property key for element position in Properties objects. */
            static const String positionStr;

            /** Property key for element size in Properties objects. */
            static const String sizeStr;

            /** Property key for the element anchor point (normalized). */
            static const String anchorStr;

            /** Property key for minimum anchor (normalized). */
            static const String anchorMinStr;

            /** Property key for maximum anchor (normalized). */
            static const String anchorMaxStr;

            /** Property key for element colour. */
            static const String colourStr;

            /** Property key for element z-order (render order). */
            static const String zOrderStr;

            /** Property key for metrics mode (pixels vs normalized). */
            static const String metricsModeStr;

            /** Property key for horizontal alignment. */
            static const String ghaStr;

            /** Property key for vertical alignment. */
            static const String gvaStr;

            /** Property key for visibility flag. */
            static const String visibleStr;

            /** Property key for whether element handles input events. */
            static const String handleInputEventsStr;

            /** Property key for element-level visibility (separate from layout visibility). */
            static const String elementVisibleStr;

            /** Property key for element caption/text. */
            static const String captionStr;

            /**
             * @brief Construct a UiElementDirector with default layout values.
             */
            UiElementDirector();

            /**
             * @brief Destructor.
             */
            ~UiElementDirector() override;

            /**
             * @brief Return a Properties object representing the UI element configuration.
             * @return SmartPtr<Properties> containing serializable layout and style settings.
             * @copydoc Director::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply configuration from a Properties object to this director.
             * @param properties Properties containing layout/style values.
             * @copydoc Director::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the normalized anchor point for the element (0..1 range).
             * @return Vector2F anchor point where (0,0) is bottom-left and (1,1) top-right.
             */
            Vector2F getAnchor() const;

            /**
             * @brief Set the normalized anchor point for the element.
             * @param anchor Anchor point in normalized coordinates (0..1 range).
             */
            void setAnchor( const Vector2F &anchor );

            /**
             * @brief Get the element's position (in current metrics).
             * @return 2D position vector.
             */
            Vector2<real_Num> getPosition() const;

            /**
             * @brief Set the element's position.
             * @param position 2D position vector in current metrics (pixels or normalized).
             */
            void setPosition( const Vector2<real_Num> &position );

            /**
             * @brief Get the element's size (width, height) in current metrics.
             * @return 2D size vector.
             */
            Vector2<real_Num> getSize() const;

            /**
             * @brief Set the element's size.
             * @param size 2D size vector (width, height) in current metrics.
             */
            void setSize( const Vector2<real_Num> &size );

            /**
             * @brief Get the minimum anchor value for stretchable UI elements.
             * @return Vector2F minimum anchor.
             */
            Vector2F getAnchorMin() const;

            /**
             * @brief Set the minimum anchor for stretchable UI elements.
             * @param anchorMin Minimum anchor value.
             */
            void setAnchorMin( const Vector2F &anchorMin );

            /**
             * @brief Get the maximum anchor value for stretchable UI elements.
             * @return Vector2F maximum anchor.
             */
            Vector2F getAnchorMax() const;

            /**
             * @brief Set the maximum anchor for stretchable UI elements.
             * @param anchorMax Maximum anchor value.
             */
            void setAnchorMax( const Vector2F &anchorMax );

            /**
             * @brief Get the element's colour multiplier.
             * @return ColourF colour applied to the element's material/texture.
             */
            ColourF getColour() const;

            /**
             * @brief Set the element's colour multiplier used during rendering.
             * @param colour Colour to apply.
             */
            void setColour( const ColourF &colour );

            /**
             * @brief Get the element's z-order (render order). Higher values render later.
             * @return Z-order integer.
             */
            u32 getZOrder() const;

            /**
             * @brief Set the element's z-order for rendering.
             * @param zOrder Integer z-order value.
             */
            void setZOrder( u32 zOrder );

            /**
             * @brief Get the metrics mode (for example pixels vs normalized coordinates).
             * @return Metrics mode code (implementation-defined).
             */
            u8 getMetricsMode() const;

            /**
             * @brief Set the metrics mode for interpreting position/size values.
             * @param metricsMode Mode code (implementation-defined semantics).
             */
            void setMetricsMode( u8 metricsMode );

            /**
             * @brief Get the horizontal alignment code used for content layout.
             * @return Alignment code (implementation-defined).
             */
            u8 getHorizontalAlignment() const;

            /**
             * @brief Set the horizontal alignment for content within the element.
             * @param horizontalAlignment Alignment code.
             */
            void setHorizontalAlignment( u8 horizontalAlignment );

            /**
             * @brief Get the vertical alignment code used for content layout.
             * @return Alignment code (implementation-defined).
             */
            u8 getVerticalAlignment() const;

            /**
             * @brief Set the vertical alignment for content within the element.
             * @param verticalAlignment Alignment code.
             */
            void setVerticalAlignment( u8 verticalAlignment );

            /**
             * @brief Get a bitmask of flags affecting element behaviour.
             * @return Flags bitmask.
             */
            u32 getFlags() const;

            /**
             * @brief Set a bitmask of flags affecting element behaviour.
             * @param flags Bitmask value.
             */
            void setFlags( u32 flags );

            /**
             * @brief Query whether the element is enabled for layout/visibility.
             * @return True if visible in layout pass.
             */
            bool isVisible() const;

            /**
             * @brief Enable or disable element visibility in layout and rendering.
             * @param visible True to make visible.
             */
            void setVisible( bool visible );

            /**
             * @brief Query whether the element handles input events (mouse, touch).
             * @return True if input events are handled by the element.
             */
            bool getHandleInputEvents() const;

            /**
             * @brief Enable or disable input event handling for the element.
             * @param handleInputEvents True to accept input events.
             */
            void setHandleInputEvents( bool handleInputEvents );

            /**
             * @brief Query whether the element is visible at the element level (separate from
             * parent/layout visibility).
             * @return True if the element itself is visible.
             */
            bool isElementVisible() const;

            /**
             * @brief Set element-level visibility (independent of layout visibility).
             * @param elementVisible True to make element visible.
             */
            void setElementVisible( bool elementVisible );

            /**
             * @brief Get the element caption or label text.
             * @return String containing the caption.
             */
            String getCaption() const;

            /**
             * @brief Set the element caption or label text.
             * @param caption New caption string.
             */
            void setCaption( const String &caption );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Material applied to the element when rendering (optional). */
            SmartPtr<render::IMaterial> m_material;

            /** Texture used by the element (optional). */
            SmartPtr<render::ITexture> m_texture;

            /** Position of the element in the chosen metrics (default: zero). */
            Vector2<real_Num> m_position = Vector2<real_Num>::zero();

            /** Size of the element (default: unit vector). */
            Vector2<real_Num> m_size = Vector2<real_Num>::unit();

            /** Normalized anchor point (0.5,0.5 centers the element). */
            Vector2F m_anchor = Vector2F( 0.5f, 0.5f );

            /** Minimum normalized anchor used for stretch behaviour. */
            Vector2F m_anchorMin = Vector2F( 0.5f, 0.5f );

            /** Maximum normalized anchor used for stretch behaviour. */
            Vector2F m_anchorMax = Vector2F( 0.5f, 0.5f );

            /** Colour multiplier for rendering the element. Default: White. */
            ColourF m_colour = ColourF::White;

            /** Render order priority. Higher values render later. Default: 0. */
            u32 m_zorder = 0;

            /** Metrics interpretation mode (implementation-defined). Default: 0. */
            u8 m_metricsMode = 0;

            /** Horizontal alignment code (implementation-defined). */
            u8 m_gha = 0;

            /** Vertical alignment code (implementation-defined). */
            u8 m_gva = 0;

            /** Bitmask of element flags (implementation-defined). */
            u32 m_flags = 0;

            /** Whether the element participates in layout/rendering. Default: true. */
            bool m_visible = true;

            /** Whether the element should handle input events. Default: true. */
            bool m_handleInputEvents = true;

            /** Element-level visibility separate from layout visibility. Default: true. */
            bool m_elementVisible = true;

            /** Optional caption or label text for the element. */
            String m_caption;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // UiElementDirector_h__
