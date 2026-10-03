#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/ImguiOgre/ImGuiManagerOgre.hpp>
#include <WPGraphicsOgre/ImguiOgre/ImGuiOverlayOgre.hpp>
#include <WPGraphicsOgre/ImguiOgre/ImGuiRenderTargetListener.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreOverlayManager.h>
#include <OgreRenderTargetListener.h>
#include <OgreRenderWindow.h>

#if defined WP_PLATFORM_WIN32
#    if WP_BUILD_RENDERER_DX11
#        include <imgui_impl_win32.h>
#        include <imgui_impl_dx11.h>
#        include <d3d11.h>
#    elif WP_BUILD_RENDERER_DX9
#        include "imgui_impl_dx9.h"
#        include "imgui_impl_win32.h"
#        include <d3d9.h>
#    else
#        include "imgui_internal.h"
#        include <imgui_impl_win32.h>
#    endif
#endif

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ImGuiManagerOgre, ISharedObject );
    WP_CLASS_REGISTER_DERIVED( workphone, ImGuiManagerOgre::WindowListener,
                               render::IGraphicsWindowListener );
    WP_CLASS_REGISTER_DERIVED( workphone, ImGuiManagerOgre::UIOverlay, ISharedObject );

    ImGuiOverlayOgre *ImGuiManagerOgre::m_overlay = nullptr;

    ImGuiManagerOgre::ImGuiManagerOgre()
    {
    }

    ImGuiManagerOgre::~ImGuiManagerOgre()
    {
        unload( nullptr );
    }

    void ImGuiManagerOgre::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto ui = applicationManager->getUI();

        setupImgui();

        auto overlay = workphone::make_ptr<UIOverlay>();
        overlay->setOwner( this );
        m_uiOverlay = overlay;

        if( ui )
        {
            ui->setOverlay( overlay );
        }

        setLoadingState( LoadingState::Loaded );
    }

    void ImGuiManagerOgre::unload( SmartPtr<ISharedObject> data )
    {
        if( !isLoaded() )
            return;

        setLoadingState( LoadingState::Unloading );

        shutdown();

        if( m_uiOverlay )
        {
            m_uiOverlay->unload( nullptr );
            m_uiOverlay = nullptr;
        }

        if( m_windowListener )
        {
            if( m_window )
                m_window->removeListener( m_windowListener );

            m_windowListener->unload( nullptr );
            m_windowListener = nullptr;
        }

        m_window = nullptr;

        setLoadingState( LoadingState::Unloaded );
    }

    bool ImGuiManagerOgre::isInitialised() const
    {
        return m_initialised;
    }

    void ImGuiManagerOgre::shutdown()
    {
        if( !m_initialised )
            return;

        // Remove render-target listener and delete it
        if( m_renderTargetListener )
        {
            if( m_window )
            {
                Ogre::RenderWindow *renderWindow = nullptr;
                m_window->_getObject( (void **)&renderWindow );
                if( renderWindow )
                    renderWindow->removeListener( m_renderTargetListener );
            }

            delete m_renderTargetListener;
            m_renderTargetListener = nullptr;
        }

        // Tear down ImGui platform backend
#if defined WP_PLATFORM_WIN32
#    if WP_BUILD_RENDERER_DX11
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
#    elif WP_BUILD_RENDERER_DX9
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
#    else
        ImGui_ImplWin32_Shutdown();
#    endif
#endif

        // Unload and remove the Ogre overlay
        if( m_overlay )
        {
            m_overlay->unload();
            Ogre::OverlayManager::getSingleton().destroy( m_overlay );
            m_overlay = nullptr;
        }

        // Destroy the ImGui context
        if( ImGui::GetCurrentContext() )
            ImGui::DestroyContext();

        m_initialised = false;
    }

    SmartPtr<render::IGraphicsWindow> ImGuiManagerOgre::getWindow() const
    {
        return m_window;
    }

    void ImGuiManagerOgre::setWindow( SmartPtr<render::IGraphicsWindow> window )
    {
        m_window = window;
    }

    void ImGuiManagerOgre::setupImgui()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto application = applicationManager->getApplication();
        if( !application )
        {
            WP_LOG_ERROR( "Application instance not found during ImGuiManagerOgre initialization." );
            return;
        }

        auto applicationName = application->getName();

        static const String iniFileExt = ".ini";
        static const String logFileExt = "_ui.log";

#if defined WP_PLATFORM_WIN32
        auto iniPath = applicationName + iniFileExt;
        auto logPath = applicationName + logFileExt;
#elif defined WP_PLATFORM_APPLE
        auto iniPath = applicationName + iniFileExt;
        auto logPath = applicationName + logFileExt;
#else
        auto iniPath = applicationName + iniFileExt;
        auto logPath = applicationName + logFileExt;
#endif

        if( auto window = getWindow() )
        {
            auto listener = factoryManager->make_ptr<WindowListener>();
            listener->setOwner( this );
            window->addListener( listener );
            m_windowListener = listener;

#if defined WP_PLATFORM_WIN32
            HWND windowHandle = nullptr;
            window->getCustomAttribute( "WINDOW", &windowHandle );

            m_hwnd = windowHandle;

#    if WP_BUILD_RENDERER_DX11
            ID3D11Device *device = nullptr;
            window->getCustomAttribute( "D3DDEVICE", &device );
#    elif WP_BUILD_RENDERER_DX9
            IDirect3DDevice9 *device = nullptr;
            window->getCustomAttribute( "D3DDEVICE", &device );
#    endif
#elif defined WP_PLATFORM_APPLE
#elif defined WP_PLATFORM_LINUX
#endif

            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();

            ImGuiIO &io = ImGui::GetIO();
            io.IniFilename = iniPath.c_str();
            io.LogFilename = logPath.c_str();
            // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();

            // Handle DPI scaling
#if defined WP_PLATFORM_WIN32
            auto vpScale = 1.0f;
#else
            auto vpScale = 2.0f;
#endif

            io.FontGlobalScale = std::round( vpScale );
            ImGui::GetStyle().ScaleAllSizes( vpScale );

            auto imguiOverlay = new ImGuiOverlayOgre();
            imguiOverlay->setZOrder( 300 );
            Ogre::OverlayManager::getSingleton().addOverlay( imguiOverlay );
            m_overlay = imguiOverlay;

            m_renderTargetListener = new ImGuiRenderTargetListener;

            Ogre::RenderWindow *renderWindow = nullptr;
            window->_getObject( (void **)&renderWindow );

            if( renderWindow )
                renderWindow->addListener( m_renderTargetListener );

            // Initialise platform and renderer backends
#if defined WP_PLATFORM_WIN32
            ImGui_ImplWin32_Init( (HWND)windowHandle );
#    if WP_BUILD_RENDERER_DX11
            if( device )
            {
                ID3D11DeviceContext *deviceContext = nullptr;
                device->GetImmediateContext( &deviceContext );
                ImGui_ImplDX11_Init( device, deviceContext );
            }
#    elif WP_BUILD_RENDERER_DX9
            if( device )
                ImGui_ImplDX9_Init( device );
#    endif
#endif

            m_initialised = true;
        }
    }

    // ---------------------------------------------------------------------------
    // WindowListener
    // ---------------------------------------------------------------------------

    ImGuiManagerOgre::WindowListener::WindowListener() = default;

    ImGuiManagerOgre::WindowListener::~WindowListener() = default;

    void ImGuiManagerOgre::WindowListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    void ImGuiManagerOgre::WindowListener::handleEvent( SmartPtr<render::IGraphicsWindowEvent> event )
    {
    }

    Parameter ImGuiManagerOgre::WindowListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == windowClosingHash )
        {
            return Parameter( true );
        }

        return {};
    }

    void ImGuiManagerOgre::WindowListener::setOwner( SmartPtr<ImGuiManagerOgre> owner )
    {
        m_owner = owner;
    }

    SmartPtr<ImGuiManagerOgre> ImGuiManagerOgre::WindowListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    // ---------------------------------------------------------------------------
    // UIOverlay
    // ---------------------------------------------------------------------------

    ImGuiManagerOgre::UIOverlay::UIOverlay() = default;

    ImGuiManagerOgre::UIOverlay::~UIOverlay() = default;

    void ImGuiManagerOgre::UIOverlay::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        auto owner = getOwner();
        if( owner && owner->m_overlay )
        {
            owner->m_overlay->load();
        }

        setLoadingState( LoadingState::Loaded );
    }

    void ImGuiManagerOgre::UIOverlay::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        auto owner = getOwner();
        if( owner && owner->m_overlay )
        {
            owner->m_overlay->unload();
        }

        setLoadingState( LoadingState::Unloaded );
    }

    void ImGuiManagerOgre::UIOverlay::setOwner( SmartPtr<ImGuiManagerOgre> owner )
    {
        m_owner = owner;
    }

    SmartPtr<ImGuiManagerOgre> ImGuiManagerOgre::UIOverlay::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }
}  // namespace workphone