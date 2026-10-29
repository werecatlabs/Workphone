#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawWindow.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawViewport.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/System/WindowMessageData.hpp>
#include "workphone_graphics_window.h"
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_dx11.h>

#if defined WP_PLATFORM_WIN32
#    include <workphone_platform_window_win32.h>

#    if WP_BUILD_IMGUI
#        include "imgui.h"
#        include "backends/imgui_impl_win32.h"
#        include <WPGraphics/ClawWin32Ime.hpp>
#        include <WPGraphics/ClawWin32ImGuiMessages.hpp>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hwnd, UINT message, WPARAM wParam,
                                                              LPARAM lParam );
#    endif
#endif

#include <cstring>

namespace workphone
{
    namespace render
    {
        namespace
        {
            const wp_c8 *toNativeString( const char *value )
            {
                return reinterpret_cast<const wp_c8 *>( value );
            }

            const char *fromNativeString( const wp_c8 *value )
            {
                return reinterpret_cast<const char *>( value );
            }

#if defined WP_PLATFORM_WIN32
            wp_s32 forwardWindowEvent( void *userData, HWND hwnd, UINT message, WPARAM wParam,
                                       LPARAM lParam )
            {
#    if WP_BUILD_IMGUI
                // Consume value-only IME updates on the native window owner;
                // they are neither gameplay input nor ImGui backend events.
                if( detail::handlePlatformImeMessage( hwnd, message, wParam, lParam ) ) return 1;
#    endif
                auto window = static_cast<ClawWindow *>( userData );
                auto event = workphone::make_ptr<WindowMessageData>();
                event->setWindowHandle( hwnd );
                event->setMessage( message );
                event->setWParam( wParam );
                event->setLParam( lParam );

                for( auto &listener : window->getListeners() )
                {
                    if( listener )
                    {
                        listener->handleEvent( event );
                    }
                }

#    if WP_BUILD_IMGUI
                if( ImGui::GetCurrentContext() && ImGui::GetIO().BackendPlatformUserData )
                {
                    const auto handled = ImGui_ImplWin32_WndProcHandler( hwnd, message, wParam, lParam );
                    return handled || detail::consumeImGuiSystemMenuMessage( message, wParam, true ) ? 1 : 0;
                }
#    endif

                return 0;
            }
#endif
        }  // namespace

        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawWindow, GraphicsWindow );

        ClawWindow::ClawWindow() : m_window( wp_graphics_window_create( 1280, 720 ) )
        {
            static const auto name = String( "ClawWindow" );
            setName( name );

            if( m_window )
            {
                wp_graphics_window_set_title( m_window, toNativeString( "Workphone" ) );
            }

            try
            {
                static const String name = "CWindowOgreNext";
                setName( name );

                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto stateManager = applicationManager->getStateManagerPtr();
                WP_ASSERT( stateManager );

                auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
                WP_ASSERT( graphicsSystem );

                auto factoryManager = applicationManager->getFactoryManagerPtr();
                WP_ASSERT( factoryManager );

                auto stateContext = stateManager->addStateContext();
                stateContext->setOwner( this );
                stateContext->setTaskId( TaskId::Render );
                setStateContext( stateContext );

                auto stateListener = factoryManager->make_ptr<RenderTargetListener>();
                stateListener->setOwner( this );
                setStateListener( stateListener );
                stateContext->addStateListener( stateListener );

                auto state = factoryManager->make_ptr<State>();
                state->setId( getId() );
                state->setOwner( this );
                stateContext->addState( state );

                auto stateData = factoryManager->make_ptr<WindowStateData>();
                state->setData( stateData );

                auto renderTargetState = factoryManager->make_ptr<State>();
                renderTargetState->setId( getId() );
                renderTargetState->setOwner( this );
                stateContext->addState( renderTargetState );

                auto renderTargetStateData = factoryManager->make_ptr<RenderTargetStateData>();
                renderTargetState->setData( renderTargetStateData );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        ClawWindow::~ClawWindow()
        {
            unload( nullptr );
        }

        void ClawWindow::load( SmartPtr<ISharedObject> data )
        {
            if( isLoaded() )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );
            bool startupFullscreen=false;
            if(getName()=="DefaultWindow")
                if(auto app=core::IApplicationManager::instancePtr())
                    if(auto settings=app->getPlayerSettings())
                    {
                        const auto resolution=settings->getPropertyAsInt("graphics.resolution");
                        constexpr s32 sizes[][2]={{1920,1080},{1280,720},{2560,1440},{3840,2160}};
                        if(resolution>=0 && resolution<4) resize(sizes[resolution][0],sizes[resolution][1]);
                        else if(resolution==5) resize(std::max(1,settings->getPropertyAsInt("graphics.customWidth")),
                                                      std::max(1,settings->getPropertyAsInt("graphics.customHeight")));
                        settings->getPropertyValue("graphics.fullscreen",startupFullscreen);
                    }
            const auto requestedSize = getSize();
            WP_LOG_INFO( "WPGraphics/Window: loading '" + getName() + "' at " +
                         std::to_string( requestedSize.x ) + "x" + std::to_string( requestedSize.y ) );

            if( !m_window )
            {
                m_window = wp_graphics_window_create( 1280, 720 );
            }

#if defined WP_PLATFORM_WIN32
            if( m_window && !m_platformWindow )
            {
                const auto title = getTitle();
                const auto size = getSize();
                m_platformWindow = wp_platform_window_win32_create( toNativeString( title.c_str() ),
                                                                    static_cast<wp_u32>( size.X() ),
                                                                    static_cast<wp_u32>( size.Y() ) );

                if( m_platformWindow )
                {
                    wp_graphics_window_set_window_handle(
                        m_window, wp_platform_window_win32_get_hwnd( m_platformWindow ) );

                    wp_platform_window_win32_set_event_callback( m_platformWindow, forwardWindowEvent,
                                                                 this );
                }
            }
#endif

            if( !m_window
#if defined WP_PLATFORM_WIN32
                || !m_platformWindow
#endif
            )
            {
                WP_LOG_ERROR( "ClawWindow::load: failed to create the native window." );
                setLoadingState( LoadingState::Unloaded );
                return;
            }

            setLoadingState( LoadingState::Loaded );
            if(startupFullscreen) setFullscreen(true);
            WP_LOG_INFO( "WPGraphics/Window: '" + getName() + "' initialized." );
        }

        void ClawWindow::unload( SmartPtr<ISharedObject> data )
        {
            if( getLoadingState() == LoadingState::Unloaded && !m_window )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );
            removeAllViewports();

#if defined WP_PLATFORM_WIN32
            if( m_platformWindow )
            {
                wp_platform_window_win32_destroy( m_platformWindow );
                m_platformWindow = nullptr;
            }
#endif

            if( m_window )
            {
                wp_graphics_window_destroy( m_window );
                m_window = nullptr;
            }

            GraphicsWindow::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }

        String ClawWindow::getTitle() const
        {
            return m_window ? String( fromNativeString( wp_graphics_window_get_title( m_window ) ) )
                            : String();
        }

        void ClawWindow::setTitle( const String &title )
        {
            if( m_window )
            {
                wp_graphics_window_set_title( m_window, toNativeString( title.c_str() ) );
            }

#if defined WP_PLATFORM_WIN32
            if( m_platformWindow )
            {
                wp_platform_window_win32_set_title( m_platformWindow, toNativeString( title.c_str() ) );
            }
#endif
        }

        void ClawWindow::destroy()
        {
            unload( nullptr );
        }

        void ClawWindow::setFullscreen( bool fullscreen, u32 width, u32 height )
        {
            setFullscreen( fullscreen );
            if( !fullscreen && width && height ) resize( width, height );
        }

        bool ClawWindow::isFullScreen() const
        {
#if defined WP_PLATFORM_WIN32
            return m_nativeFullscreen;
#else
            return GraphicsWindow::isFullScreen();
#endif
        }

        void ClawWindow::setFullscreen( bool fullscreen )
        {
#if defined WP_PLATFORM_WIN32
            void *native = nullptr;
            getWindowHandle( &native );
            auto handle = static_cast<HWND>( native );
            if( !handle || fullscreen == m_nativeFullscreen ) return;
            if( fullscreen )
            {
                RECT rectangle{};
                GetWindowRect( handle, &rectangle );
                m_windowedPosition = Vector2I( rectangle.left, rectangle.top );
                m_windowedSize = Vector2I( rectangle.right - rectangle.left, rectangle.bottom - rectangle.top );
                m_windowedStyle = GetWindowLongPtr( handle, GWL_STYLE );
                MONITORINFO monitor{};
                monitor.cbSize = sizeof( monitor );
                if( !GetMonitorInfo( MonitorFromWindow( handle, MONITOR_DEFAULTTONEAREST ), &monitor ) ) return;
                SetWindowLongPtr( handle, GWL_STYLE, m_windowedStyle & ~WS_OVERLAPPEDWINDOW );
                SetWindowPos( handle, HWND_TOP, monitor.rcMonitor.left, monitor.rcMonitor.top,
                    monitor.rcMonitor.right - monitor.rcMonitor.left,
                    monitor.rcMonitor.bottom - monitor.rcMonitor.top, SWP_FRAMECHANGED | SWP_NOOWNERZORDER );
            }
            else
            {
                SetWindowLongPtr( handle, GWL_STYLE, m_windowedStyle );
                SetWindowPos( handle, nullptr, m_windowedPosition.x, m_windowedPosition.y,
                              m_windowedSize.x, m_windowedSize.y, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOOWNERZORDER );
            }
            m_nativeFullscreen = fullscreen;
            windowMovedOrResized();
#endif
            GraphicsWindow::setFullscreen( fullscreen );
        }

        void ClawWindow::resize( u32 width, u32 height )
        {
            if( m_window )
            {
                wp_graphics_window_resize( m_window, width, height );
            }

#if defined WP_PLATFORM_WIN32
            if( m_platformWindow )
            {
                wp_platform_window_win32_resize( m_platformWindow, width, height );
            }
#endif
        }

        void ClawWindow::maximize()
        {
            if( m_window )
            {
                wp_graphics_window_maximize( m_window );
            }

#if defined WP_PLATFORM_WIN32
            if( m_platformWindow )
            {
                wp_platform_window_win32_maximize( m_platformWindow );
            }
#endif
        }

        bool ClawWindow::isVisible() const
        {
            return m_window && wp_graphics_window_is_visible( m_window ) != 0;
        }

        void ClawWindow::setVisible( bool visible )
        {
            if( m_window )
            {
                wp_graphics_window_set_visible( m_window, visible ? 1 : 0 );
            }

#if defined WP_PLATFORM_WIN32
            if( m_platformWindow )
            {
                if( visible )
                {
                    wp_platform_window_win32_show( m_platformWindow );
                }
                else
                {
                    wp_platform_window_win32_hide( m_platformWindow );
                }
            }
#endif
        }

        bool ClawWindow::isClosed() const
        {
#if defined WP_PLATFORM_WIN32
            if( m_platformWindow )
            {
                return wp_platform_window_win32_is_closed( m_platformWindow ) != 0;
            }
#endif

            return !m_window || wp_graphics_window_is_closed( m_window ) != 0;
        }

        Vector2I ClawWindow::getSize() const
        {
            if( m_window )
            {
                return Vector2I( static_cast<s32>( wp_graphics_window_get_width( m_window ) ),
                                 static_cast<s32>( wp_graphics_window_get_height( m_window ) ) );
            }

            return Vector2I( 1280, 720 );
        }

        void ClawWindow::setSize( const Vector2I &size )
        {
            resize( static_cast<u32>( std::max( size.X(), 1 ) ),
                    static_cast<u32>( std::max( size.Y(), 1 ) ) );
        }

        void ClawWindow::getWindowHandle( void *pData )
        {
            if( pData )
            {
                auto handle = m_window ? wp_graphics_window_get_window_handle( m_window ) : nullptr;
                std::memcpy( pData, &handle, sizeof( handle ) );
            }
        }

        String ClawWindow::getWindowHandleAsString() const
        {
            const auto handle = m_window ? wp_graphics_window_get_window_handle( m_window ) : nullptr;
            return handle ? StringUtil::toString( reinterpret_cast<size_t>( handle ) ) : String();
        }

        void ClawWindow::copyContentsToMemory( void *buffer, u32 size, FrameBuffer bufferId )
        {
            if( !tryCopyContentsToMemory( buffer, size, bufferId ) )
            {
                WP_LOG_WARNING( "ClawWindow::copyContentsToMemory: back buffer unavailable or RGBA8 buffer too small." );
            }
        }

        bool ClawWindow::tryCopyContentsToMemory( void *buffer, u32 size,
                                                  FrameBuffer bufferId ) const
        {
            if( !buffer || !size || bufferId == FrameBuffer::Front || bufferId == FrameBuffer::Count )
            {
                return false;
            }
            auto app = core::IApplicationManager::instancePtr();
            auto graphics = app ? app->getGraphicsSystemPtr() : nullptr;
            auto renderer = graphics
                ? dynamic_pointer_cast<ClawRendererDX11>( graphics->getRenderer() ) : nullptr;
            auto *dx11 = renderer ? wp_renderer_get_dx11( renderer->getNativeRenderer() ) : nullptr;
            void *windowHandle = nullptr;
            const_cast<ClawWindow *>( this )->getWindowHandle( &windowHandle );
            return wp_renderer_dx11_copy_back_buffer_rgba8( dx11, windowHandle, buffer, size ) != 0;
        }

        bool ClawWindow::messagePump()
        {
#if defined WP_PLATFORM_WIN32
            if( m_platformWindow )
            {
                wp_platform_window_win32_pump_messages( m_platformWindow );

                const auto width = wp_platform_window_win32_get_width( m_platformWindow );
                const auto height = wp_platform_window_win32_get_height( m_platformWindow );
                if( m_window && width > 0 && height > 0 &&
                    ( width != wp_graphics_window_get_width( m_window ) ||
                      height != wp_graphics_window_get_height( m_window ) ) )
                {
                    wp_graphics_window_resize( m_window, width, height );
                }
            }
#endif

            return !isClosed();
        }

        SmartPtr<IViewport> ClawWindow::addViewport( hash_type id, SmartPtr<IGraphicsCamera> camera,
                                                     s32 ZOrder, f32 left, f32 top, f32 width,
                                                     f32 height )
        {
            try
            {
                if( !camera )
                {
                    WP_LOG_WARNING( "ClawWindow::addViewport: null camera supplied." );
                    return {};
                }

                auto applicationManager = core::IApplicationManager::instance();
                auto factoryManager = applicationManager->getFactoryManager();

                auto stateContext = getStateContext();
                if( !stateContext )
                {
                    WP_LOG_ERROR( "ClawWindow::addViewport: no state context." );
                    return {};
                }
                auto data = stateContext->getStateDataById<RenderTargetStateData>( getId() );
                if( !data )
                {
                    WP_LOG_ERROR( "ClawWindow::addViewport: RenderTargetStateData not found." );
                    return {};
                }

                auto viewport = factoryManager->make_ptr<ClawViewport>();
                if( !viewport )
                {
                    WP_LOG_ERROR( "ClawWindow::addViewport: failed to create viewport." );
                    return {};
                }

                viewport->setViewportId( id );
                viewport->setCamera( camera );
                viewport->setZOrder( ZOrder < 0 ? static_cast<s32>( data->viewports.size() ) : ZOrder );
                viewport->setPosition( Vector2<real_Num>( left, top ) );
                viewport->setSize( Vector2<real_Num>( width, height ) );
                viewport->setRenderTarget( this );

                data->viewports.push_back( viewport );
                return viewport;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return {};
        }

        bool ClawWindow::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        bool ClawWindow::handleStateChanged( SmartPtr<IState> &state )
        {
            auto viewports = getViewports();
            for( auto &viewport : viewports )
            {
                if( viewport )
                {
                    if( viewport->handleStateChanged( state ) )
                    {
                        return true;
                    }
                }
            }

            if( state && state->getOwnerPtr() == this )
            {
                auto stateData = state->getData();
                if( stateData->isDerived<WindowStateData>() )
                {
                    auto renderWindowState =
                        workphone::static_pointer_cast<WindowStateData>( stateData );

                    const auto &flags = renderWindowState->flags;
                    auto visible = BitUtil::getFlagValue( flags, IGraphicsWindow::WINDOW_FLAG_VISIBLE );

                    if( m_window )
                    {
                        wp_graphics_window_set_visible( m_window, visible ? 1 : 0 );
                    }

#if defined WP_PLATFORM_WIN32
                    if( m_platformWindow )
                    {
                        if( visible )
                        {
                            wp_platform_window_win32_show( m_platformWindow );
                        }
                        else
                        {
                            wp_platform_window_win32_hide( m_platformWindow );
                        }
                    }
#endif

                    return true;
                }
                else if( stateData->isDerived<RenderTargetStateData>() )
                {
                    auto renderTargetState =
                        workphone::static_pointer_cast<RenderTargetStateData>( stateData );

                    if( m_window )
                    {
                        // State changes also cover priority, flags and viewports. Record the
                        // actual size instead of resizing the window from stale state data.
                        renderTargetState->size = getSize();
                        return true;
                    }
                }
            }

            return false;
        }

        SmartPtr<IViewport> ClawWindow::getViewport( u32 index )
        {
            auto viewports = getViewports();
            if( index < static_cast<u32>( viewports.size() ) )
            {
                return viewports[index];
            }
            return nullptr;
        }

        Array<SmartPtr<IViewport>> ClawWindow::getViewports() const
        {
            auto stateContext = getStateContext();
            if( !stateContext )
            {
                return {};
            }
            auto data = stateContext->getStateDataById<RenderTargetStateData>( getId() );
            if( !data )
            {
                return {};
            }
            return data->viewports;
        }

        u32 ClawWindow::getNumViewports() const
        {
            auto viewports = getViewports();
            return static_cast<u32>( viewports.size() );
        }

    }  // namespace render
}  // namespace workphone
