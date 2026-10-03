#ifndef __UITextCore_h__
#define __UITextCore_h__

#include <WPGraphicsOgreNext/UI/Core/UIElementCore.hpp>
#include <Workphone/UI/UIText.hpp>
#include <Workphone/Memory/AtomicRawPtr.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @file UITextOgreNext.hpp
         * @brief OgreNext implementation of the UI text element.
         *
         * This header declares `UITextOgreNext`, a concrete UI text element that integrates
         * the engine's `UIText` interface with OgreNext / Core rendering (via `wp_text`).
         * The class wraps a `wp_text` instance and exposes lifecycle, state handling
         * and property access necessary for the engine's UI framework.
         *
         * @see UIText
         */

        /**
         * @class UITextOgreNext
         * @brief UI text element implementation for the OgreNext (Core) renderer.
         *
         * `UITextOgreNext` implements the engine `UIText` UI element on top of Core's
         * `Label` widget, providing text display and behaviour for the UI system.
         *
         * Responsibilities:
         * - Manage Core label lifetime and integration with the UI element hierarchy.
         * - Forward text, alignment and rendering-related requests to the Core `Label`.
         * - Handle UI element loading/unloading and state transitions.
         *
         * Threading / ownership:
         * - The underlying Core label pointer is stored in an `AtomicRawPtr` to allow
         *   safe concurrent reads from other threads where the pointer may be observed.
         *
         * Notes:
         * - Inherits from `UIElementOgreNext<UIText>` and therefore implements the `UIText`
         *   interface for the OgreNext backend.
         */
        class UITextCore : public UIElementCore<UIText>
        {
        public:
            /**
             * @brief Construct a new UITextOgreNext.
             *
             * The constructor does not create the Core label; label creation is performed
             * during `load` or other initialization phases so that it happens on the correct
             * UI/rendering thread if required by the Core manager.
             */
            UITextCore();

            /**
             * @brief Destroy the UITextOgreNext.
             *
             * The destructor ensures any resources owned by this object are released.
             * It does not assume a specific thread for destruction of Core objects -
             * callers should ensure proper teardown order if Core requires main-thread cleanup.
             */
            ~UITextCore() override;

            /**
             * @brief Load and initialize the UI text element.
             *
             * Called when the UI element is being created/initialized. Implementations should
             * create or attach the underlying Core `Label`, apply saved `Properties` and
             * prepare the element for rendering.
             *
             * @param data Smart pointer to an `ISharedObject` that contains initialization data.
             *             May be null depending on the calling code.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload and release resources held by the UI text element.
             *
             * Called when the UI element is being destroyed or removed. Implementations should
             * detach and release references to Core objects and clear runtime state.
             *
             * @param data Smart pointer to an `ISharedObject` that contains unload context.
             *             May be null depending on the calling code.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Per-frame update � submits the label draw command to the Workphone context.
             *
             * Opens a frameless, non-interactive Workphone window scoped to this element's
             * bounds and emits a `wp_label_colored` call so the OgreNext renderer can pick
             * it up during the same frame.
             */
            void update() override;

            /**
             * @brief Handle a state change for the UI element.
             *
             * When the UI element's state changes (e.g. enabled/disabled/hovered/pressed),
             * this method should update the Core label state and any visual properties
             * that depend on the UI state.
             *
             * @param state Reference to the new `IState` object to apply.
             * @return True if the state change was processed by this element, false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @brief Retrieve current properties for this UI element.
             *
             * Returns a `Properties` object representing configurable values (text, font,
             * alignment, etc.). The returned pointer may be shared with other systems but
             * callers should treat it as an immutable snapshot unless explicitly documented.
             *
             * @return Smart pointer to a `Properties` instance describing the element.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply a set of properties to the UI element.
             *
             * Properties applied here should update runtime state and propagate relevant
             * values to the underlying Core label (for example: text, font size,
             * alignment, color).
             *
             * @param properties Smart pointer to the `Properties` to apply. May be null.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            // -----------------------------------------------------------------------
            // Text colour
            // -----------------------------------------------------------------------

            /** @brief Gets the foreground (text) colour. */
            ColourF getTextColour() const;

            /** @brief Sets the foreground (text) colour and propagates it to the style struct. */
            void setTextColour( const ColourF &colour );

            // -----------------------------------------------------------------------
            // Background colour
            // -----------------------------------------------------------------------

            /** @brief Gets the background colour drawn behind the label. */
            ColourF getBackgroundColour() const;

            /** @brief Sets the background colour and propagates it to the style struct. */
            void setBackgroundColour( const ColourF &colour );

            // -----------------------------------------------------------------------
            // Text padding
            // -----------------------------------------------------------------------

            /** @brief Gets the inner padding (in normalised 0-1 space) applied around the text. */
            Vector2F getTextPadding() const;

            /** @brief Sets the inner padding and propagates it to the style struct. */
            void setTextPadding( const Vector2F &padding );

            // -----------------------------------------------------------------------
            // Word-wrap
            // -----------------------------------------------------------------------

            /** @brief Returns true if word-wrap is enabled for this label. */
            bool getTextWrap() const;

            /**
             * @brief Enables or disables word-wrap.
             *
             * When enabled, `update()` calls `wp_label_colored_wrap` instead of
             * `wp_label_colored`, letting Workphone break lines automatically.
             */
            void setTextWrap( bool wrap );
            bool getDrawBackground() const;
            void setDrawBackground( bool drawBackground );

            f32 getBackgroundRounding() const;
            void setBackgroundRounding( f32 rounding );

            u8 getLeftHorizontalAlignment() const;
            void setLeftHorizontalAlignment( u8 alignment );

            u8 getRightHorizontalAlignment() const;
            void setRightHorizontalAlignment( u8 alignment );

            u8 getTopVerticalAlignment() const;
            void setTopVerticalAlignment( u8 alignment );

            u8 getBottomVerticalAlignment() const;
            void setBottomVerticalAlignment( u8 alignment );

            /**
             * @brief Create and configure any state context necessary for this element.
             *
             * This method is intended to prepare state-specific structures (for example,
             * Core/Label state entries) so that subsequent state changes or property
             * applications function correctly.
             */
            void createStateContext();

            WP_CLASS_REGISTER_DECL;  ///< Macro for class registration (implementation-specific).

        protected:
            /** @brief Pushes m_textColour, m_backgroundColour, and m_textPadding into m_labelText. */
            void applyStyle();

            /** @brief Foreground (text) colour. Default: opaque white. */
            ColourF m_textColour{ 1.0f, 1.0f, 1.0f, 1.0f };

            /** @brief Background colour drawn behind the label. Default: fully transparent. */
            ColourF m_backgroundColour{ 0.0f, 0.0f, 0.0f, 0.0f };

            /** @brief Inner padding around the text, in normalised [0,1] space. Default: zero. */
            Vector2F m_textPadding{ 0.0f, 0.0f };

            /** @brief When true, update() uses wp_label_colored_wrap for automatic line breaks. */
            bool m_textWrap = false;
            bool m_drawBackground = false;

            f32 m_backgroundRounding = 0.0f;

            u8 m_leftHorizontalAlignment = (u8)HorizontalAlignment::LEFT;
            u8 m_rightHorizontalAlignment = (u8)HorizontalAlignment::RIGHT;
            u8 m_topVerticalAlignment = (u8)VerticalAlignment::TOP;
            u8 m_bottomVerticalAlignment = (u8)VerticalAlignment::BOTTOM;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // UITextOgreNext_h__
