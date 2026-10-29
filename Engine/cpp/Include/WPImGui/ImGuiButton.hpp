#ifndef ImGuiButton_h__
#define ImGuiButton_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>

namespace workphone
{
    namespace core
    {
        extern template class WPCore_API Prototype<ui::IUIButton>;
    }

    namespace ui
    {
        class ImGuiButton : public ImGuiElement<IUIButton>
        {
        public:
            struct ShortcutObservation
            {
                int frame = -1;
                bool enabled = false, itemVisible = false, applicationFocused = false;
                bool ctrl = false, alt = false, shift = false, super = false;
                bool keyPressed = false;
            };

            struct KeyboardShortcutSnapshot
            {
                String keyboardShortcut;
                ShortcutObservation lastRendered, lastKeyPress;
                u64 keyPressCount = 0, activationCount = 0;
            };

            ImGuiButton();
            ~ImGuiButton() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void update() override;

            String getLabel() const override;
            void setLabel( const String &label ) override;

            void setTextSize( f32 textSize ) override;
            f32 getTextSize() const override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            // Value-only diagnostics; reading never touches the ImGui context.
            KeyboardShortcutSnapshot getKeyboardShortcutSnapshot() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            AtomicObject<FixedString<128>> m_label;
            atomic_f32 m_textSize = 14.0f;
            atomic_u32 m_keyboardShortcut = 0;
            int m_shortcutFrame = -1;
            atomic_u64 m_shortcutObservation = 0, m_shortcutKeyPressObservation = 0;
            atomic_u64 m_shortcutKeyPressCount = 0, m_activationCount = 0;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiButton_h__
