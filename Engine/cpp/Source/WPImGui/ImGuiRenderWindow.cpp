#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiRenderWindow.hpp>
#include <WPImGui/ImGuiApplication.hpp>
#include <WPImGui/ImGuiManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiRenderWindow, ImGuiWindowT<IUIRenderWindow> );

    ImGuiRenderWindow::ImGuiRenderWindow() = default;

    ImGuiRenderWindow::~ImGuiRenderWindow()
    {
        unload( nullptr );
    }

    void *ImGuiRenderWindow::getHWND() const
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto uiManager = workphone::static_pointer_cast<ImGuiManager>( applicationManager->getUI() );
        WP_ASSERT( uiManager );

        if( uiManager )
        {
            auto application = uiManager->getApplication();
            if( application )
            {
                auto pApplication = workphone::static_pointer_cast<ImGuiApplication>( application );
                return pApplication->getHWND();
            }
        }

        return nullptr;
    }

    SmartPtr<render::IGraphicsWindow> ImGuiRenderWindow::getWindow() const
    {
        auto p = m_window.load();
        return p.lock();
    }

    void ImGuiRenderWindow::setWindow( SmartPtr<render::IGraphicsWindow> window )
    {
        m_window = window;
    }

    SmartPtr<render::ITexture> ImGuiRenderWindow::getRenderTexture() const
    {
        auto p = m_renderTexture.load();
        return p.lock();
    }

    void ImGuiRenderWindow::setRenderTexture( SmartPtr<render::ITexture> renderTexture )
    {
        m_renderTexture = renderTexture;
    }

    void ImGuiRenderWindow::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        ImGuiWindowT<IUIRenderWindow>::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void ImGuiRenderWindow::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        m_window = nullptr;
        m_renderTexture = nullptr;
        ImGuiWindowT<IUIRenderWindow>::unload( data );

        setLoadingState( LoadingState::Unloaded );
    }
}  // namespace workphone::ui
