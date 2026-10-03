#ifndef ImGuiRenderWindow_h__
#define ImGuiRenderWindow_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiWindowT.hpp>
#include <Workphone/Interface/UI/IUIRenderWindow.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiRenderWindow : public ImGuiWindowT<IUIRenderWindow>
        {
        public:
            ImGuiRenderWindow();
            ~ImGuiRenderWindow() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            void *getHWND() const;

            SmartPtr<render::IGraphicsWindow> getWindow() const override;
            void setWindow( SmartPtr<render::IGraphicsWindow> window ) override;

            SmartPtr<render::ITexture> getRenderTexture() const override;
            void setRenderTexture( SmartPtr<render::ITexture> renderTexture ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            AtomicWeakPtr<render::IGraphicsWindow> m_window;
            AtomicWeakPtr<render::ITexture> m_renderTexture;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiWindow_h__
