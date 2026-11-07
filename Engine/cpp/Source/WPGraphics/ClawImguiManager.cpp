#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawImguiManager.hpp>
#include "ClawImGuiFrameRecovery.hpp"
#include <WPGraphics/ClawUiSettings.hpp>
#include <WPImGui/ImGuiRuntimeFont.hpp>
#include <WPImGui/WPImGui.hpp>
#include <Workphone/Workphone.hpp>
#include "workphone_graphics_renderer.h"
#include <workphone_graphics_renderer_dx11.h>

#include "imgui.h"
#include "imgui_internal.h"

#if defined WP_PLATFORM_WIN32
#    include "backends/imgui_impl_win32.h"
#    include <WPGraphics/ClawWin32Ime.hpp>
#    include <workphone_graphics_ui_dx11.h>
#    include <d3d11.h>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace workphone
{
    constexpr unsigned int kMaxConvertVertices = 16u * 1024u * 1024u;

    namespace render
    {
        void detail::discardInterruptedImGuiFrame( ClawImguiManager &backend )
        {
            ScopedLock lock( &backend, true );
            auto *context = ImGui::GetCurrentContext();
            if( !context || context->IO.BackendRendererUserData != &backend )
            {
                return;
            }
            if( context->WithinFrameScope )
            {
                // Application callbacks have unwound before entry. Recover
                // their unmatched Begin/Push calls using this ImGui version's
                // internal recovery path, while old font/atlas pins still live.
                // Its window recovery closes a global tab bar using the current
                // window, which may be an inner child rather than the tab's
                // owner. This frame is discarded, so retire that layout stack
                // directly; per-window recovery will unwind its ID scopes.
                context->CurrentTabBar = nullptr;
                context->CurrentTabBarStack.resize( 0 );
                while( context->CurrentWindowStack.Size > 0 )
                {
                    const auto fontDepth =
                        context->CurrentWindowStack.back().StackSizesOnBegin.SizeOfFontStack;
                    while( context->FontStack.Size > fontDepth )
                    {
                        ImGui::PopFont();
                    }
                    ImGui::ErrorCheckEndWindowRecover( nullptr, nullptr );
                    // EndTable may have closed its own child window. Unwind
                    // fonts against the now-current owner's saved depth too.
                    const auto recoveredFontDepth =
                        context->CurrentWindowStack.back().StackSizesOnBegin.SizeOfFontStack;
                    while( context->FontStack.Size > recoveredFontDepth )
                    {
                        ImGui::PopFont();
                    }
                    if( context->CurrentWindowStack.Size == 1 )
                    {
                        break;
                    }
                    if( context->CurrentWindow->Flags & ImGuiWindowFlags_ChildWindow )
                    {
                        ImGui::EndChild();
                    }
                    else
                    {
                        ImGui::End();
                    }
                }
                ImGui::EndFrame();
            }
            // Discard every borrowed SRV/font command before a subsequent
            // NewFramePre callback may retire its immutable resource owners.
            for( auto *window : context->Windows )
            {
                window->DrawList->_ClearFreeMemory();
            }
            for( auto *viewport : context->Viewports )
            {
                for( auto *draw : viewport->DrawLists )
                {
                    if( draw )
                    {
                        draw->_ClearFreeMemory();
                    }
                }
                viewport->DrawDataP.Clear();
                viewport->DrawDataBuilder.Clear();
            }
            // Reset frame accounting and retire any retained native texture views.
            backend.setRenderer( backend.getRenderer() );
        }

        namespace
        {
            std::atomic<bool> uiSettingsPersistence{ true };

#if defined WP_PLATFORM_WIN32
            void postPlatformImeData( ImGuiViewport *viewport, ImGuiPlatformImeData *data )
            {
                auto hwnd = static_cast<HWND>( viewport->PlatformHandleRaw );
#    ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
                if( !hwnd )
                {
                    hwnd = static_cast<HWND>( ImGui::GetIO().ImeWindowHandle );
                }
#    endif
                detail::postPlatformImeUpdate(
                    hwnd, data->WantVisible,
                    detail::platformImeCoordinate( data->InputPos.x - viewport->Pos.x ),
                    detail::platformImeCoordinate( data->InputPos.y - viewport->Pos.y ) );
            }

            void bindWin32BackendIfAvailable()
            {
                auto &io = ImGui::GetIO();
                if( io.BackendPlatformUserData )
                {
                    return;
                }

                auto applicationManager = core::IApplicationManager::instancePtr();
                if( !applicationManager )
                {
                    return;
                }

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                auto window = graphicsSystem ? graphicsSystem->getDefaultWindow() : nullptr;
                if( !window )
                {
                    return;
                }

                void *windowHandle = nullptr;
                window->getWindowHandle( &windowHandle );
                const auto hwnd = static_cast<HWND>( windowHandle );
                if( !hwnd || !IsWindow( hwnd ) )
                {
                    return;
                }

                if( ImGui_ImplWin32_Init( hwnd ) )
                {
                    // Keep IME updates asynchronous after the platform backend binds.
                    io.SetPlatformImeDataFn = postPlatformImeData;
                    WP_LOG_INFO( "ClawImguiManager: Win32 input backend bound to HWND " +
                                 std::to_string( reinterpret_cast<std::uintptr_t>( hwnd ) ) + "." );
                }
                else
                {
                    WP_LOG_ERROR( "ClawImguiManager: failed to initialise the Win32 backend." );
                }
            }
#endif

            // Dear ImGui packs vertex colours as ImU32 == 0xAABBGGRR
            // (A in bits 24-31, B in 16-23, G in 8-15, R in 0-7).
            //
            // The C89 renderer"s PTC shaders unpack the per-vertex colour as
            // 0xRRGGBBAA (R in bits 24-31, A in bits 0-7), as documented in
            // workphone_graphics_vertex.h and confirmed by the DX11 vs_main:
            //
            //   o.color = float4(
            //       float((i.color >> 24u) & 0xFFu) / 255.0,   // R
            //       float((i.color >> 16u) & 0xFFu) / 255.0,   // G
            //       float((i.color >>  8u) & 0xFFu) / 255.0,   // B
            //       float( i.color         & 0xFFu) / 255.0);  // A
            //
            // Each vertex colour therefore has to be byte-swapped when
            // ImDrawVert is converted to wp_vertex_ptc. The explicit extract-
            // and-repack form below is branch-free and avoids any ambiguity
            // about signed shifts on ImU32 (which is uint32_t).
            wp_u32 swizzleImguiColour( ImU32 col ) noexcept
            {
                const wp_u32 value = static_cast<wp_u32>( col );

                const wp_u32 r = ( value >> IM_COL32_R_SHIFT ) & 0xFFu;
                const wp_u32 g = ( value >> IM_COL32_G_SHIFT ) & 0xFFu;
                const wp_u32 b = ( value >> IM_COL32_B_SHIFT ) & 0xFFu;
                const wp_u32 a = ( value >> IM_COL32_A_SHIFT ) & 0xFFu;

                return ( r << 24u ) | ( g << 16u ) | ( b << 8u ) | a;
            }

            // Defensive framebuffer scale: ImGui clamps this to >0 in practice,
            // but a NaN/Inf/non-positive value (e.g. from a zero-sized surface
            // during minimisation) would poison the clip-rect math. Fall back
            // to 1.0 so the UI keeps rendering at logical resolution.
            float getSafeFramebufferScale( float scale )
            {
                return std::isfinite( scale ) && scale > 0.0f ? scale : 1.0f;
            }

            int clampClip( double value, int maximum )
            {
                // Clamp before conversion: even finite draw coordinates may exceed INT_MAX.
                return static_cast<int>( std::max( 0.0, std::min( value, double( maximum ) ) ) );
            }

            // Clamp a float to a safe positive range for viewport math. This
            // guards against negative/NaN/Inf dimensions reported by a renderer
            // that is mid-resize or has lost its swap chain.
            float safeDimension( float value, float fallback )
            {
                return ( std::isfinite( value ) && value > 0.0f ) ? value : fallback;
            }

            void makeIdentity( wp_mat4f &m )
            {
                std::memset( &m, 0, sizeof( m ) );
                m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
            }

            // Build ImGui's orthographic projection for the column-vector
            // transform used by both C89 renderer backends. The matrix storage
            // is row-major, so translation belongs in the final column.
            void makeImguiProjection( wp_mat4f &m, float left, float right, float top, float bottom )
            {
                std::memset( &m, 0, sizeof( m ) );
                m.m[3][3] = 1.0f;
                if( right > left && bottom > top )
                {
                    m.m[0][0] = 2.0f / ( right - left );
                    m.m[1][1] = 2.0f / ( top - bottom );
                    m.m[2][2] = -1.0f;
                    m.m[0][3] = ( right + left ) / ( left - right );
                    m.m[1][3] = ( top + bottom ) / ( bottom - top );
                }
            }
        }  // namespace

        void setClawUiSettingsPersistence( bool enabled )
        {
            uiSettingsPersistence.store( enabled, std::memory_order_relaxed );
        }

        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawImguiManager, ISharedObject );

        ClawImguiManager *ClawImguiManager::ms_singleton = nullptr;

        ClawImguiManager *ClawImguiManager::getSingletonPtr()
        {
            if( !ms_singleton )
            {
                ms_singleton = new ClawImguiManager();
            }
            return ms_singleton;
        }

        ClawImguiManager &ClawImguiManager::getSingleton()
        {
            return *getSingletonPtr();
        }

        ClawImguiManager::ClawImguiManager() :
            m_lastHeadlessTime( std::chrono::high_resolution_clock::now() )
        {
            if( !ms_singleton )
            {
                ms_singleton = this;
            }
        }

        ClawImguiManager::~ClawImguiManager()
        {
            shutdown();

            if( ms_singleton == this )
            {
                ms_singleton = nullptr;
            }
        }

        void ClawImguiManager::load( SmartPtr<ISharedObject> data )
        {
            if( isLoaded() )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            try
            {
                auto renderer = getRenderer();
                if( renderer )
                {
                    init( renderer );
                }
            }
            catch( const std::bad_alloc & )
            {
                setLoadingState( LoadingState::Error );
                WP_LOG_ERROR( "ClawImguiManager::load: out of memory allocating WPImGui plugin." );
                return;
            }

            setLoadingState( LoadingState::Loaded );
        }

        void ClawImguiManager::unload( SmartPtr<ISharedObject> data )
        {
            if( getLoadingState() == LoadingState::Unloaded )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            shutdown();

            setLoadingState( LoadingState::Unloaded );
        }

        void ClawImguiManager::shutdown()
        {
            if( m_shuttingDown )
            {
                return;
            }

            m_shuttingDown = true;
            discardPreparedFrame();

            if( ImGui::GetCurrentContext() )
            {
                ImGuiIO &io = ImGui::GetIO();
                if( io.BackendRendererUserData == this )
                {
                    io.Fonts->SetTexID( nullptr );  // invalidate font SRV handle before teardown
#if defined WP_PLATFORM_WIN32
                    if( io.BackendPlatformUserData && io.BackendPlatformName &&
                        std::strcmp( io.BackendPlatformName, "imgui_impl_win32" ) == 0 )
                    {
                        ImGui_ImplWin32_Shutdown();
                    }
#endif

                    // Font SRV already cleared at the top of this block
                    io.BackendRendererUserData = nullptr;
                    io.BackendRendererName = nullptr;
                    ImGui::DestroyContext();
                }
            }

            if( m_fontTexture )
            {
                wp_renderer_dx11_destroy_texture_native( m_fontTexture );
                m_fontTexture = nullptr;
            }

            m_renderer = nullptr;
            m_vpWidth = 0;
            m_vpHeight = 0;
            m_frameEnded = true;
            m_contextOwner = false;
            m_shuttingDown = false;
        }

        void ClawImguiManager::init( wp_renderer *renderer )
        {
            ScopedLock lock( this, true );

            if( !renderer )
            {
                WP_LOG_ERROR( "ClawImguiManager::init: null renderer supplied." );
                return;
            }

            if( m_renderer == renderer && ImGui::GetCurrentContext() &&
                ImGui::GetIO().BackendRendererUserData == this )
            {
                // Same renderer and live context - refresh viewport and re-upload the
                // font atlas.  The atlas SRV is only valid for the current DX11
                // device; if the device was reset (resize, GPU crash, device-lost)
                // the old handle is stale and must be rebuilt.
                setupViewport( wp_renderer_get_width( renderer ), wp_renderer_get_height( renderer ) );
                createFontTexture();
#if defined WP_PLATFORM_WIN32
                bindWin32BackendIfAvailable();
#endif
                return;
            }

            if( m_renderer || ImGui::GetCurrentContext() )
            {
                shutdown();
            }

            WP_LOG_INFO( "ClawImguiManager::init: creating UI context and font atlas." );

            auto applicationManager = core::IApplicationManager::instancePtr();

            String applicationName = "workphone";
            if( applicationManager )
            {
                if( auto application = applicationManager->getApplication() )
                {
                    applicationName = application->getName();
                }
            }

            static const String iniFileExt = ".ini";
            static const String logFileExt = "_ui.log";
            m_iniPath = applicationName + iniFileExt;
            m_logPath = applicationName + logFileExt;

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            m_contextOwner = true;

            ImGuiIO &io = ImGui::GetIO();
            io.BackendRendererName = "imgui_impl_claw_dx11";
            io.BackendRendererUserData = this;
            io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
            const auto persistSettings = uiSettingsPersistence.load( std::memory_order_relaxed );
            io.IniFilename = persistSettings ? m_iniPath.c_str() : nullptr;
            io.LogFilename = persistSettings ? m_logPath.c_str() : nullptr;

            m_renderer = renderer;

#if defined WP_PLATFORM_WIN32
            // ImGui ends its frame under the GraphicsSystem render lock. Its
            // default IME hook sends synchronously to the main HWND thread,
            // which may itself be waiting for that lock during scene teardown.
            io.SetPlatformImeDataFn = postPlatformImeData;
            bindWin32BackendIfAvailable();
#endif

            io.Fonts->AddFontDefault();

            if( applicationManager )
            {
                if( auto ui = applicationManager->getUI() )
                {
                    const String fontPath = "fa-solid-900.ttf";
                    if( !ui->loadFont( fontPath, "awesome" ) )
                    {
                        WP_LOG_WARNING(
                            "ClawImguiManager::init: failed to load fa-solid-900.ttf; "
                            "the default font remains available." );
                    }
                }
            }

            createFontTexture();

            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            ImGui::StyleColorsDark();

            const s32 width = wp_renderer_get_width( m_renderer );
            const s32 height = wp_renderer_get_height( m_renderer );
            updateProjectionMatrix( width > 0 ? static_cast<float>( width ) : 1280.0f,
                                    height > 0 ? static_cast<float>( height ) : 720.0f );
            WP_LOG_INFO(
                "ClawImguiManager::init: context configured; viewport=" + std::to_string( width ) + "x" +
                std::to_string( height ) + ", docking enabled, settings=" +
                ( persistSettings ? m_iniPath : String( "disabled" ) ) );
        }

        bool ClawImguiManager::isInitialised() const
        {
            return m_renderer != nullptr && ImGui::GetCurrentContext() != nullptr &&
                   ImGui::GetIO().BackendRendererUserData == this;
        }

        void ClawImguiManager::setupViewport( s32 width, s32 height )
        {
            if( width > 0 && height > 0 )
            {
                updateProjectionMatrix( static_cast<float>( width ), static_cast<float>( height ) );
            }
        }

        wp_renderer *ClawImguiManager::getRenderer() const
        {
            return m_renderer;
        }

        void ClawImguiManager::setRenderer( wp_renderer *renderer )
        {
            if( !hasPreparedFrame() && isInitialised() )
            {
                ui::releaseImGuiDx11FrameTextures();
            }
            discardPreparedFrame();

            m_renderer = renderer;
            m_frameEnded = true;
        }

        void ClawImguiManager::newFrame()
        {
            ScopedLock lock( this, true );

            if( !isInitialised() || !m_frameEnded || hasPreparedFrame() )
            {
                return;
            }

            m_frameEnded = false;
            ImGuiIO &io = ImGui::GetIO();

#if defined WP_PLATFORM_WIN32
            // The Editor creates its main window after configuring the renderer.
            // Bind once that HWND is published without resetting the live UI context.
            bindWin32BackendIfAvailable();
            const bool hasPlatformBackend = io.BackendPlatformUserData != nullptr;
            if( hasPlatformBackend )
            {
                ImGui_ImplWin32_NewFrame();
            }
#else
            const bool hasPlatformBackend = false;
#endif

            if( !hasPlatformBackend )
            {
                const auto now = std::chrono::high_resolution_clock::now();
                const float deltaTime = std::chrono::duration<float>( now - m_lastHeadlessTime ).count();
                m_lastHeadlessTime = now;
                io.DeltaTime = ( deltaTime > 0.0f && deltaTime < 1.0f ) ? deltaTime : ( 1.0f / 60.0f );
            }

            const float width = m_vpWidth > 0 ? static_cast<float>( m_vpWidth ) : 400.0f;
            const float height = m_vpHeight > 0 ? static_cast<float>( m_vpHeight ) : 400.0f;
            io.DisplaySize = ImVec2( width, height );
            io.DisplayFramebufferScale = ImVec2( 1.0f, 1.0f );

            ImGui::NewFrame();
        }

        void ClawImguiManager::updateProjectionMatrix( float width, float height )
        {
            const float safeW = safeDimension( width, 1.0f );
            const float safeH = safeDimension( height, 1.0f );
            m_vpWidth = static_cast<s32>( std::min(
                static_cast<double>( safeW ), static_cast<double>( std::numeric_limits<s32>::max() ) ) );
            m_vpHeight = static_cast<s32>( std::min(
                static_cast<double>( safeH ), static_cast<double>( std::numeric_limits<s32>::max() ) ) );

            if( ImGui::GetCurrentContext() )
            {
                ImGuiIO &io = ImGui::GetIO();
                io.DisplaySize = ImVec2( safeW, safeH );
            }
        }

        void ClawImguiManager::finishFrame()
        {
            ScopedLock lock( this, true );

            if( !isInitialised() || m_frameEnded )
            {
                return;
            }

            m_frameEnded = true;
            ImGui::Render();
            m_preparedDrawData = ImGui::GetDrawData();
            if( !m_preparedDrawData )
            {
                ui::releaseImGuiDx11FrameTextures();
                return;
            }

            const auto *fonts = ImGui::GetIO().Fonts;
            m_preparedFontTexId = fonts ? fonts->TexID : nullptr;
#if defined WP_PLATFORM_WIN32
            if( m_fontTexture )
            {
                static_cast<ID3D11ShaderResourceView *>( m_fontTexture )->AddRef();
                m_preparedFontTexture = m_fontTexture;
            }
#endif
        }

        bool ClawImguiManager::hasPreparedFrame() const
        {
            return m_preparedDrawData != nullptr;
        }

        void ClawImguiManager::discardPreparedFrame()
        {
            if( hasPreparedFrame() && isInitialised() )
            {
                ui::releaseImGuiDx11FrameTextures();
            }
            m_preparedDrawData = nullptr;
            m_preparedFontTexId = nullptr;
            wp_renderer_dx11_destroy_texture_native( std::exchange( m_preparedFontTexture, nullptr ) );
        }

        void ClawImguiManager::render()
        {
            ScopedLock lock( this, true );

            if( !isInitialised() )
            {
                discardPreparedFrame();
                return;
            }

            if( !m_frameEnded )
            {
                finishFrame();
            }
            auto *drawData = std::exchange( m_preparedDrawData, nullptr );
            if( !drawData )
            {
                return;
            }

            const ImTextureID fontTexId = std::exchange( m_preparedFontTexId, nullptr );
            const std::unique_ptr<void, decltype( &wp_renderer_dx11_destroy_texture_native )> fontPin(
                std::exchange( m_preparedFontTexture, nullptr ), wp_renderer_dx11_destroy_texture_native );
            struct TexturePins
            {
                ~TexturePins() { ui::releaseImGuiDx11FrameTextures(); }
            } texturePins;

            if( drawData->CmdListsCount <= 0 || drawData->CmdListsCount > 0x10000 )
            {
                return;
            }

            if( drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f ||
                !std::isfinite( drawData->DisplaySize.x ) || !std::isfinite( drawData->DisplaySize.y ) ||
                !std::isfinite( drawData->DisplayPos.x ) || !std::isfinite( drawData->DisplayPos.y ) )
            {
                return;
            }

            const wp_viewport_i vp = wp_renderer_get_viewport( m_renderer );
            if( vp.width <= 0 || vp.height <= 0 )
            {
                return;
            }

            if( m_vpWidth != vp.width || m_vpHeight != vp.height )
            {
                updateProjectionMatrix( static_cast<float>( vp.width ),
                                        static_cast<float>( vp.height ) );
            }

            const int vpWidth = vp.width;
            const int vpHeight = vp.height;
            const int offX = vp.x;
            const int offY = vp.y;

            wp_mat4f projMatrix;
            const float left = drawData->DisplayPos.x;
            const float right = drawData->DisplayPos.x + drawData->DisplaySize.x;
            const float top = drawData->DisplayPos.y;
            const float bottom = drawData->DisplayPos.y + drawData->DisplaySize.y;
            makeImguiProjection( projMatrix, left, right, top, bottom );

            wp_mat4f identity;
            makeIdentity( identity );

            struct RenderState
            {
                wp_renderer *renderer;
                wp_viewport_i viewport;
                wp_blend_mode blend = wp_renderer_get_blend_mode( renderer );
                wp_cull_mode cull = wp_renderer_get_cull_mode( renderer );
                wp_fill_mode fill = wp_renderer_get_fill_mode( renderer );
                wp_s32 depthTest = wp_renderer_get_depth_test_enabled( renderer );
                wp_s32 depthWrite = wp_renderer_get_depth_write_enabled( renderer );
                wp_depth_func depthFunc = wp_renderer_get_depth_func( renderer );

                ~RenderState()
                {
                    wp_renderer_set_texture_native( renderer, nullptr );
                    wp_renderer_set_scissor_enabled( renderer, 0 );
                    wp_renderer_set_blend_mode( renderer, blend );
                    wp_renderer_set_cull_mode( renderer, cull );
                    wp_renderer_set_fill_mode( renderer, fill );
                    wp_renderer_set_depth_test_enabled( renderer, depthTest );
                    wp_renderer_set_depth_write_enabled( renderer, depthWrite );
                    wp_renderer_set_depth_func( renderer, depthFunc );
                    wp_renderer_set_viewport( renderer, viewport );
                }
            } savedState{ m_renderer, vp };

            const auto setupRenderState = [&]() {
                wp_renderer_set_blend_mode( m_renderer, WORKPHONE_BLEND_MODE_ALPHA );
                wp_renderer_set_cull_mode( m_renderer, WORKPHONE_CULL_MODE_NONE );
                wp_renderer_set_fill_mode( m_renderer, WORKPHONE_FILL_MODE_SOLID );
                wp_renderer_set_depth_test_enabled( m_renderer, 0 );
                wp_renderer_set_depth_write_enabled( m_renderer, 0 );
                wp_renderer_set_depth_func( m_renderer, WORKPHONE_DEPTH_FUNC_ALWAYS );
                wp_renderer_set_scissor_enabled( m_renderer, 1 );
                wp_renderer_set_viewport( m_renderer, vp );
                wp_renderer_set_scissor_rect( m_renderer, vp );
                wp_renderer_set_world_matrix( m_renderer, &identity );
                wp_renderer_set_view_matrix( m_renderer, &identity );
                wp_renderer_set_projection_matrix( m_renderer, &projMatrix );
            };
            setupRenderState();

            const ImVec2 clipOff = drawData->DisplayPos;
            const ImVec2 clipScale = drawData->FramebufferScale;
            const float scaleX = getSafeFramebufferScale( clipScale.x );
            const float scaleY = getSafeFramebufferScale( clipScale.y );
#if defined WP_PLATFORM_WIN32
            auto *dx11 = wp_renderer_get_dx11( m_renderer );
#endif

            std::vector<wp_vertex_ptc> converted;
            converted.reserve( 4096 );

            for( int n = 0; n < drawData->CmdListsCount; ++n )
            {
                const ImDrawList *drawList = drawData->CmdLists[n];
                if( !drawList )
                {
                    continue;
                }

                const ImDrawVert *vtxSrc = drawList->VtxBuffer.Data;
                const unsigned int vertexCount = static_cast<unsigned int>( drawList->VtxBuffer.Size );
                const unsigned int indexCount = static_cast<unsigned int>( drawList->IdxBuffer.Size );

                if( ( vertexCount && !vtxSrc ) || ( indexCount && !drawList->IdxBuffer.Data ) )
                {
                    continue;
                }

                if( vertexCount > kMaxConvertVertices )
                {
                    WP_LOG_ERROR(
                        "ClawImguiManager::render: draw list vertex count exceeds limit; skipping." );
                    continue;
                }

                if( converted.size() < vertexCount )
                {
                    converted.resize( vertexCount );
                }

                for( unsigned int v = 0u; v < vertexCount; ++v )
                {
                    const ImDrawVert &src = vtxSrc[v];
                    wp_vertex_ptc &dst = converted[v];
                    dst.position.x = src.pos.x;
                    dst.position.y = src.pos.y;
                    dst.position.z = 0.0f;
                    dst.uv.x = src.uv.x;
                    dst.uv.y = src.uv.y;
                    dst.color = swizzleImguiColour( src.col );
                }

                const ImDrawIdx *idxBuf = drawList->IdxBuffer.Data;
#if defined WP_PLATFORM_WIN32
                wp_ui_draw_list_dx11 nativeList{ converted.data(), idxBuf, vertexCount,
                                                 indexCount, sizeof( ImDrawIdx ), 0 };
#endif

                for( int i = 0; i < drawList->CmdBuffer.Size; ++i )
                {
                    const ImDrawCmd *cmd = &drawList->CmdBuffer[i];
                    if( cmd->UserCallback )
                    {
                        if( cmd->UserCallback == ImDrawCallback_ResetRenderState )
                        {
                            setupRenderState();
                        }
                        else
                        {
                            cmd->UserCallback( drawList, cmd );
                            // Callbacks may overwrite the backend's shared vertex/index buffers.
#if defined WP_PLATFORM_WIN32
                            nativeList.upload_state = 0;
#endif
                        }
                        continue;
                    }

                    if( cmd->ElemCount == 0u )
                    {
                        continue;
                    }

                    const ImTextureID textureId = cmd->GetTexID();
                    const ImTextureID texture = textureId ? textureId : fontTexId;

                    const float clipMinX = ( cmd->ClipRect.x - clipOff.x ) * scaleX;
                    const float clipMinY = ( cmd->ClipRect.y - clipOff.y ) * scaleY;
                    const float clipMaxX = ( cmd->ClipRect.z - clipOff.x ) * scaleX;
                    const float clipMaxY = ( cmd->ClipRect.w - clipOff.y ) * scaleY;

                    if( !std::isfinite( clipMinX ) || !std::isfinite( clipMinY ) ||
                        !std::isfinite( clipMaxX ) || !std::isfinite( clipMaxY ) )
                    {
                        continue;
                    }

                    const int clipL = clampClip( std::floor( double( clipMinX ) ), vpWidth );
                    const int clipT = clampClip( std::floor( double( clipMinY ) ), vpHeight );
                    const int clipR = clampClip( std::ceil( double( clipMaxX ) ), vpWidth );
                    const int clipB = clampClip( std::ceil( double( clipMaxY ) ), vpHeight );

                    if( clipR <= clipL || clipB <= clipT )
                    {
                        continue;
                    }

                    wp_viewport_i scissor;
                    scissor.x = offX + clipL;
                    scissor.y = offY + clipT;
                    scissor.width = clipR - clipL;
                    scissor.height = clipB - clipT;

#if defined WP_PLATFORM_WIN32
                    if( dx11 )
                    {
                        wp_renderer_dx11_draw_ui_range( dx11, &nativeList, cmd->IdxOffset,
                                                        cmd->ElemCount, cmd->VtxOffset, scissor,
                                                        texture );
                        continue;
                    }
#endif
                    if constexpr( sizeof( ImDrawIdx ) == sizeof( uint16_t ) )
                    {
                        const auto indexOffset = cmd->IdxOffset;
                        if( indexOffset >= indexCount || cmd->ElemCount > indexCount - indexOffset ||
                            cmd->VtxOffset >= vertexCount ||
                            cmd->ElemCount > static_cast<unsigned int>( std::numeric_limits<wp_s32>::max() ) )
                        {
                            continue;
                        }

                        const auto availableVertices = vertexCount - cmd->VtxOffset;
                        const auto *indices = idxBuf + indexOffset;
                        if( std::any_of( indices, indices + cmd->ElemCount,
                                         [availableVertices]( ImDrawIdx index ) {
                                             return index >= availableVertices;
                                         } ) )
                        {
                            continue;
                        }
                        wp_renderer_set_texture_native( m_renderer, texture );
                        wp_renderer_set_scissor_enabled( m_renderer, 1 );
                        wp_renderer_set_scissor_rect( m_renderer, scissor );
                        wp_renderer_draw_indexed_triangles_ptc(
                            m_renderer, converted.data() + cmd->VtxOffset,
                            static_cast<wp_s32>( availableVertices ),
                            reinterpret_cast<const uint16_t *>( indices ),
                            static_cast<wp_s32>( cmd->ElemCount ) );
                    }
                    else
                    {
                        WP_LOG_ERROR( "32-bit ImDrawIdx requires DX11 u32 index support." );
                    }
                }
            }
        }

        void ClawImguiManager::createFontTexture()
        {
            ScopedLock lock( this, true );

            if( !isInitialised() )
            {
                return;
            }

            ImGuiIO &io = ImGui::GetIO();
            if( !io.Fonts || io.Fonts->Locked )
            {
                WP_LOG_ERROR(
                    "ClawImguiManager::createFontTexture: font atlas is null or locked by an active "
                    "frame." );
                return;
            }

            if( io.Fonts->Fonts.empty() )
            {
                io.Fonts->AddFontDefault();
            }

            unsigned char *pixels = nullptr;
            int width = 0;
            int height = 0;
            io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );

            if( !pixels || width <= 0 || height <= 0 )
            {
                WP_LOG_ERROR( "ClawImguiManager::createFontTexture: font atlas bake failed." );
                return;
            }

            constexpr int kMaxAtlasDim = 8192;
            if( width > kMaxAtlasDim || height > kMaxAtlasDim )
            {
                WP_LOG_ERROR( "ClawImguiManager::createFontTexture: font atlas exceeds limit." );
                return;
            }

            const size_t atlasPixels = static_cast<size_t>( width ) * static_cast<size_t>( height );
            if( atlasPixels > static_cast<size_t>( kMaxAtlasDim ) * static_cast<size_t>( kMaxAtlasDim ) )
            {
                WP_LOG_ERROR(
                    "ClawImguiManager::createFontTexture: font atlas pixel count exceeds limit." );
                return;
            }

            // Upload the font atlas via the DX11 native path so the SRV is
            // usable as a ShaderResourceView. This keeps the font consistent with
            // all other texture bindings in render(), which all go through
            // wp_renderer_set_texture_native().
            void *nativeHandle = nullptr;
            if( auto *dx11 = wp_renderer_get_dx11( m_renderer ) )
            {
                nativeHandle = wp_renderer_dx11_create_texture_native( dx11, pixels, width, height,
                                                                       WORKPHONE_PIXEL_FORMAT_RGBA8 );
            }
            if( !nativeHandle )
            {
                WP_LOG_ERROR( "ClawImguiManager::createFontTexture: font texture upload failed." );
                return;
            }

            io.Fonts->SetTexID( nativeHandle );
            if( m_fontTexture )
            {
                wp_renderer_dx11_destroy_texture_native( m_fontTexture );
            }
            m_fontTexture = nativeHandle;
        }
    }  // namespace render
}  // namespace workphone
