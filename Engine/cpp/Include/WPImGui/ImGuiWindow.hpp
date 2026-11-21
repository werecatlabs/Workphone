#ifndef ImGuiWindow_h__
#define ImGuiWindow_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiWindowT.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <functional>

struct ImGuiContext;

namespace workphone
{
    namespace core
    {
        extern template class WPCore_API Prototype<ui::IUIWindow>;
    }

    namespace ui
    {
        class ImGuiWindow : public ImGuiWindowT<IUIWindow>
        {
        public:
            struct ContentContext
            {
                ImGuiContext *context;
                void *( *allocate )( size_t, void * );
                void ( *free )( void *, void * );
                void *allocatorData;
            };

            using ContentRenderer = std::function<void( const ContentContext & )>;

            ImGuiWindow();
            ~ImGuiWindow() override;

            void load( SmartPtr<ISharedObject> data ) override;

            void update() override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            /** Apply first-use floating placement before Begin; saved layouts take precedence.
             * Returns true when this window opts out of the ordinary default dock.
             */
            bool applyInitialPlacement() const;

            /** Draw custom contents inside the active native window. Set during window creation. */
            void setContentRenderer( ContentRenderer renderer );
            void drawContent() const;

            WP_CLASS_REGISTER_DECL;

        private:
            atomic_bool m_initiallyFloating = false;
            ContentRenderer m_contentRenderer;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // ImGuiWindow_h__
