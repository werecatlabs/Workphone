#ifndef UITextOgreNext_h__
#define UITextOgreNext_h__

#include <WPGraphicsOgreNext/UI/Colibri/UIElementColibri.hpp>
#include <Workphone/UI/UIText.hpp>
#include <Workphone/Memory/AtomicRawPtr.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @file UITextOgreNext.hpp
         * @brief OgreNext implementation of the UI text element.
         *
         * This header declares `UITextOgreNext`, a concrete UI text element that integrates
         * the engine's `UIText` interface with OgreNext / Colibri rendering (via `Colibri::Label`).
         * The class wraps a `Colibri::Label` instance and exposes lifecycle, state handling
         * and property access necessary for the engine's UI framework.
         *
         * @see UIText
         */

        /**
         * @class UITextOgreNext
         * @brief UI text element implementation for the OgreNext (Colibri) renderer.
         *
         * `UITextOgreNext` implements the engine `UIText` UI element on top of Colibri's
         * `Label` widget, providing text display and behaviour for the UI system.
         *
         * Responsibilities:
         * - Manage Colibri label lifetime and integration with the UI element hierarchy.
         * - Forward text, alignment and rendering-related requests to the Colibri `Label`.
         * - Handle UI element loading/unloading and state transitions.
         *
         * Threading / ownership:
         * - The underlying Colibri label pointer is stored in an `AtomicRawPtr` to allow
         *   safe concurrent reads from other threads where the pointer may be observed.
         *
         * Notes:
         * - Inherits from `UIElementOgreNext<UIText>` and therefore implements the `UIText`
         *   interface for the OgreNext backend.
         */
        class UITextColibri : public UIElementColibri<UIText>
        {
        public:
            /**
             * @brief Construct a new UITextOgreNext.
             *
             * The constructor does not create the Colibri label; label creation is performed
             * during `load` or other initialization phases so that it happens on the correct
             * UI/rendering thread if required by the Colibri manager.
             */
            UITextColibri();

            /**
             * @brief Destroy the UITextOgreNext.
             *
             * The destructor ensures any resources owned by this object are released.
             * It does not assume a specific thread for destruction of Colibri objects -
             * callers should ensure proper teardown order if Colibri requires main-thread cleanup.
             */
            ~UITextColibri() override;

            /**
             * @brief Load and initialize the UI text element.
             *
             * Called when the UI element is being created/initialized. Implementations should
             * create or attach the underlying Colibri `Label`, apply saved `Properties` and
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
             * detach and release references to Colibri objects and clear runtime state.
             *
             * @param data Smart pointer to an `ISharedObject` that contains unload context.
             *             May be null depending on the calling code.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Handle a state change for the UI element.
             *
             * When the UI element's state changes (e.g. enabled/disabled/hovered/pressed),
             * this method should update the Colibri label state and any visual properties
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
             * values to the underlying Colibri label (for example: text, font size,
             * alignment, color).
             *
             * @param properties Smart pointer to the `Properties` to apply. May be null.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get the underlying Colibri `Label` used to render the text.
             *
             * This returns a raw pointer to the Colibri label instance. The pointer is
             * managed using an atomic wrapper stored in this object; callers should not
             * assume ownership. The returned pointer may be null if the label has not been
             * created or has been released.
             *
             * @return Pointer to a `Colibri::Label` or nullptr.
             */
            Colibri::Label *getLabelText() const;

            /**
             * @brief Set / replace the underlying Colibri `Label`.
             *
             * This allows external code to inject or replace the Colibri label used for
             * rendering. The `UITextOgreNext` instance will store the pointer atomically.
             *
             * @param labelText Raw pointer to a `Colibri::Label`. Ownership semantics are
             *                  implementation-specific; typically the Colibri manager owns it.
             */
            void setLabelText( Colibri::Label *labelText );

            /**
             * @brief Create and configure any state context necessary for this element.
             *
             * This method is intended to prepare state-specific structures (for example,
             * Colibri/Label state entries) so that subsequent state changes or property
             * applications function correctly.
             */
            void createStateContext();

            WP_CLASS_REGISTER_DECL;  ///< Macro for class registration (implementation-specific).

        protected:
            /**
             * @brief Atomic pointer to the Colibri label used for rendering text.
             *
             * Stored in an `AtomicRawPtr` to allow safe concurrent reads of the pointer value.
             * The atomic wrapper does not manage the lifetime of the underlying `Label` beyond
             * the pointer value itself; lifetime is controlled by the Colibri manager or by
             * code that creates/releases the `Label`.
             */
            AtomicRawPtr<Colibri::Label>
                m_labelText;  ///< Pointer to the Colibri label used for rendering text.
        };
    }  // namespace ui
}  // namespace workphone

#endif  // UITextOgreNext_h__
