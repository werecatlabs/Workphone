#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawImguiManager.hpp>
#include <WPImGui/WPImGui.hpp>
#include <Workphone/Workphone.hpp>
#include "workphone_graphics_renderer.h"
#include <workphone_graphics_renderer_dx11.h>

#include "imgui.h"

#if defined WP_PLATFORM_WIN32
#    include "backends/imgui_impl_win32.h"
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <vector>

namespace workphone
{
    constexpr unsigned int kMaxConvertVertices = 16u * 1024u * 1024u;

    namespace render
    {
        namespace
        {
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

            int clampInt( int value, int minValue, int maxValue )
            {
                return std::max( minValue, std::min( value, maxValue ) );
            }

            auto clampClip = []( float value, int maximum ) -> int {
                if( !std::isfinite( value ) )
                    return 0;

                value = std::max( 0.0f, std::min( value, static_cast<float>( maximum ) ) );

                return static_cast<int>( value );
            };

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

        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawImguiManager, ISharedObject );

        ClawImguiManager *ClawImguiManager::ms_singleton = nullptr;

        ClawImguiManager *ClawImguiManager::getSingletonPtr()
        {
            if( !ms_singleton )
                ms_singleton = new ClawImguiManager();
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
                ms_singleton = this;
        }

        ClawImguiManager::~ClawImguiManager()
        {
            shutdown();

            if( ms_singleton == this )
                ms_singleton = nullptr;
        }

        void ClawImguiManager::load( SmartPtr<ISharedObject> data )
        {
            if( isLoaded() )
                return;

            setLoadingState( LoadingState::Loading );

            try
            {
                auto renderer = getRenderer();
                if( renderer )
                    init( renderer );
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
                return;

            setLoadingState( LoadingState::Unloading );

            shutdown();

            setLoadingState( LoadingState::Unloaded );
        }

        void ClawImguiManager::shutdown()
        {
            if( m_shuttingDown )
                return;

            m_shuttingDown = true;

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

            m_renderer = nullptr;
            m_vpWidth = 0;
            m_vpHeight = 0;
            m_frameEnded = true;
            m_lastRenderedFrame = -1;
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
                return;
            }

            if( m_renderer || ImGui::GetCurrentContext() )
                shutdown();

            WP_LOG_INFO( "ClawImguiManager::init: creating UI context and font atlas." );

            auto applicationManager = core::IApplicationManager::instancePtr();

            String applicationName = "workphone";
            if( applicationManager )
            {
                if( auto application = applicationManager->getApplication() )
                    applicationName = application->getName();
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
            io.IniFilename = m_iniPath.c_str();
            io.LogFilename = m_logPath.c_str();

            m_renderer = renderer;

#if defined WP_PLATFORM_WIN32
            if( applicationManager )
            {
                if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
                {
                    if( auto window = graphicsSystem->getDefaultWindow() )
                    {
                        void *windowHandle = nullptr;
                        window->getWindowHandle( &windowHandle );
                        if( windowHandle && !ImGui_ImplWin32_Init( windowHandle ) )
                            WP_LOG_ERROR( "ClawImguiManager: failed to initialise the Win32 backend." );
                    }
                }
            }
#endif

            io.Fonts->AddFontDefault();

            if( applicationManager )
            {
                if( auto ui = applicationManager->getUI() )
                {
                    const String fontPath = "fa-solid-900.ttf";
                    if( !ui->loadFont( fontPath, "awesome" ) )
                        WP_LOG_WARNING( "ClawImguiManager::init: failed to load fa-solid-900.ttf; "
                                        "the default font remains available." );
                }
            }

            createFontTexture();

            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            ImGui::StyleColorsDark();

            const s32 width = wp_renderer_get_width( m_renderer );
            const s32 height = wp_renderer_get_height( m_renderer );
            updateProjectionMatrix( width > 0 ? static_cast<float>( width ) : 1280.0f,
                                    height > 0 ? static_cast<float>( height ) : 720.0f );
            WP_LOG_INFO( "ClawImguiManager::init: context configured; viewport=" +
                         std::to_string( width ) + "x" + std::to_string( height ) +
                         ", docking enabled, settings=" + m_iniPath );
        }

        bool ClawImguiManager::isInitialised() const
        {
            return m_renderer != nullptr && ImGui::GetCurrentContext() != nullptr;
        }

        void ClawImguiManager::setupViewport( s32 width, s32 height )
        {
            if( width > 0 && height > 0 )
                updateProjectionMatrix( static_cast<float>( width ), static_cast<float>( height ) );
        }

        wp_renderer *ClawImguiManager::getRenderer() const
        {
            return m_renderer;
        }

        void ClawImguiManager::setRenderer( wp_renderer *renderer )
        {
            m_renderer = renderer;

            // Reset frame accounting to prevent skipping the next ImGui frame due
            // to stale m_lastRenderedFrame / m_frameEnded from the old context.
            m_frameEnded = true;
            m_lastRenderedFrame = -1;
        }

        void ClawImguiManager::newFrame()
        {
            ScopedLock lock( this, true );

            if( !m_renderer || !ImGui::GetCurrentContext() )
                return;

            m_frameEnded = false;
            ImGuiIO &io = ImGui::GetIO();

#if defined WP_PLATFORM_WIN32
            const bool hasPlatformBackend = io.BackendPlatformUserData != nullptr;
            if( hasPlatformBackend )
                ImGui_ImplWin32_NewFrame();
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
            m_vpWidth = static_cast<s32>( safeW );
            m_vpHeight = static_cast<s32>( safeH );

            if( ImGui::GetCurrentContext() )
            {
                ImGuiIO &io = ImGui::GetIO();
                io.DisplaySize = ImVec2( safeW, safeH );
            }
        }

        void ClawImguiManager::render()
        {
            ScopedLock lock( this, true );

            if( !m_renderer || m_frameEnded || !ImGui::GetCurrentContext() )
                return;

            m_frameEnded = true;

            const int currentFrame = ImGui::GetFrameCount();
            if( currentFrame == m_lastRenderedFrame )
                return;
            m_lastRenderedFrame = currentFrame;

            ImGui::Render();
            ImDrawData *drawData = ImGui::GetDrawData();
            if( !drawData || drawData->CmdListsCount <= 0 || drawData->CmdListsCount > 0x10000 )
                return;

            if( drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f ||
                !std::isfinite( drawData->DisplaySize.x ) || !std::isfinite( drawData->DisplaySize.y ) ||
                !std::isfinite( drawData->DisplayPos.x ) || !std::isfinite( drawData->DisplayPos.y ) )
                return;

            const wp_viewport_i vp = wp_renderer_get_viewport( m_renderer );
            if( vp.width <= 0 || vp.height <= 0 )
                return;

            if( m_vpWidth != vp.width || m_vpHeight != vp.height )
                updateProjectionMatrix( static_cast<float>( vp.width ),
                                        static_cast<float>( vp.height ) );

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

            const wp_blend_mode savedBlend = wp_renderer_get_blend_mode( m_renderer );
            const wp_cull_mode savedCull = wp_renderer_get_cull_mode( m_renderer );
            const wp_fill_mode savedFill = wp_renderer_get_fill_mode( m_renderer );
            const wp_s32 savedDepthTest = wp_renderer_get_depth_test_enabled( m_renderer );
            const wp_s32 savedDepthWrite = wp_renderer_get_depth_write_enabled( m_renderer );
            const wp_depth_func savedDepthFunc = wp_renderer_get_depth_func( m_renderer );

            wp_renderer_set_blend_mode( m_renderer, WORKPHONE_BLEND_MODE_ALPHA );
            wp_renderer_set_cull_mode( m_renderer, WORKPHONE_CULL_MODE_NONE );
            wp_renderer_set_fill_mode( m_renderer, WORKPHONE_FILL_MODE_SOLID );
            wp_renderer_set_depth_test_enabled( m_renderer, 0 );
            wp_renderer_set_depth_write_enabled( m_renderer, 0 );
            wp_renderer_set_depth_func( m_renderer, WORKPHONE_DEPTH_FUNC_ALWAYS );
            wp_renderer_set_scissor_enabled( m_renderer, 1 );

            // do NOT call wp_renderer_set_texture_native(nullptr) here.
            // That clears ptc_external_texture_view and forces fallback to
            // ptc_texture_view (the font), but it does NOT prevent a scene
            // texture SRV from shadowing it if set_texture_native(scene_srv)
            // was called earlier. The per-cmd branch below binds explicitly on
            // every draw call, so no header-level unbind is needed.

            wp_renderer_set_world_matrix( m_renderer, &identity );
            wp_renderer_set_view_matrix( m_renderer, &identity );
            wp_renderer_set_projection_matrix( m_renderer, &projMatrix );

            const ImVec2 clipOff = drawData->DisplayPos;
            const ImVec2 clipScale = drawData->FramebufferScale;
            const ImTextureID fontTexId = ImGui::GetIO().Fonts ? ImGui::GetIO().Fonts->TexID : nullptr;

            std::vector<wp_vertex_ptc> converted;
            converted.reserve( 4096 );

            for( int n = 0; n < drawData->CmdListsCount; ++n )
            {
                const ImDrawList *drawList = drawData->CmdLists[n];
                if( !drawList )
                    continue;

                if( drawList->VtxBuffer.empty() || drawList->IdxBuffer.empty() )
                    continue;

                const ImDrawVert *vtxSrc = drawList->VtxBuffer.Data;
                const unsigned int vertexCount = static_cast<unsigned int>( drawList->VtxBuffer.Size );
                const unsigned int indexCount = static_cast<unsigned int>( drawList->IdxBuffer.Size );

                if( vertexCount == 0u || indexCount == 0u )
                    continue;

                if( vtxSrc == nullptr || drawList->IdxBuffer.Data == nullptr )
                    continue;

                if( vertexCount > kMaxConvertVertices )
                {
                    WP_LOG_ERROR(
                        "ClawImguiManager::render: draw list vertex count exceeds limit; skipping." );
                    continue;
                }

                if( converted.size() < vertexCount )
                    converted.resize( vertexCount );

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
                const auto dx11 = wp_renderer_get_dx11( m_renderer );
                bool prepared = false;

                if( dx11 &&
                    vertexCount <= static_cast<unsigned int>( std::numeric_limits<wp_s32>::max() ) &&
                    indexCount <= static_cast<unsigned int>( std::numeric_limits<wp_s32>::max() ) )
                {
                    if constexpr( sizeof( ImDrawIdx ) == sizeof( uint16_t ) )
                    {
                        prepared = wp_renderer_dx11_prepare_indexed_triangles_ptc(
                                       dx11, converted.data(), static_cast<wp_s32>( vertexCount ),
                                       idxBuf, static_cast<wp_s32>( indexCount ) ) != 0;
                    }
                    else
                    {
                        prepared = wp_renderer_dx11_prepare_indexed_triangles_ptc_u32(
                                       dx11, converted.data(), static_cast<wp_s32>( vertexCount ),
                                       reinterpret_cast<const uint32_t *>( idxBuf ),
                                       static_cast<wp_s32>( indexCount ) ) != 0;
                    }
                }

                for( int i = 0; i < drawList->CmdBuffer.Size; ++i )
                {
                    const ImDrawCmd *cmd = &drawList->CmdBuffer[i];
                    if( !cmd || cmd->ElemCount == 0u )
                        continue;

                    if( cmd->UserCallback )
                    {
                        if( cmd->UserCallback == ImDrawCallback_ResetRenderState )
                        {
                            wp_renderer_set_blend_mode( m_renderer, WORKPHONE_BLEND_MODE_ALPHA );
                            wp_renderer_set_cull_mode( m_renderer, WORKPHONE_CULL_MODE_NONE );
                            wp_renderer_set_fill_mode( m_renderer, WORKPHONE_FILL_MODE_SOLID );
                            wp_renderer_set_depth_test_enabled( m_renderer, 0 );
                            wp_renderer_set_depth_write_enabled( m_renderer, 0 );
                            wp_renderer_set_depth_func( m_renderer, WORKPHONE_DEPTH_FUNC_ALWAYS );
                            wp_renderer_set_scissor_enabled( m_renderer, 1 );
                            wp_renderer_set_texture_native( m_renderer, nullptr );
                            wp_renderer_set_world_matrix( m_renderer, &identity );
                            wp_renderer_set_view_matrix( m_renderer, &identity );
                            wp_renderer_set_projection_matrix( m_renderer, &projMatrix );

                            wp_viewport_i fullScissor;
                            fullScissor.x = static_cast<wp_s32>( offX );
                            fullScissor.y = static_cast<wp_s32>( offY );
                            fullScissor.width = static_cast<wp_s32>( vpWidth );
                            fullScissor.height = static_cast<wp_s32>( vpHeight );
                            wp_renderer_set_scissor_rect( m_renderer, fullScissor );
                        }
                        else
                        {
                            cmd->UserCallback( drawList, cmd );
                        }
                        continue;
                    }

                    const unsigned int indexOffset = cmd->IdxOffset;

                    if( indexOffset >= indexCount || cmd->ElemCount > ( indexCount - indexOffset ) )
                        continue;

                    if( !prepared && cmd->VtxOffset != 0u )
                        continue;

                    if( cmd->VtxOffset > vertexCount )
                        continue;

                    const ImTextureID textureId = cmd->GetTexID();
                    // Bind whichever texture this draw cmd expects:
                    //   scene/other texture -> bind it as native SRV
                    //   font atlas          -> bind the DX11 SRV from createFontTexture()
                    // The old 'else -> nullptr' branch failed when a prior scene draw
                    // planted a valid SRV in ptc_external_texture_view (common after
                    // project load), because nullptr does not clear that field.
                    if( textureId && textureId != fontTexId )
                        wp_renderer_set_texture_native( m_renderer, textureId );
                    else
                        wp_renderer_set_texture_native( m_renderer, fontTexId );

                    const float scaleX = getSafeFramebufferScale( clipScale.x );
                    const float scaleY = getSafeFramebufferScale( clipScale.y );
                    const float clipMinX = ( cmd->ClipRect.x - clipOff.x ) * scaleX;
                    const float clipMinY = ( cmd->ClipRect.y - clipOff.y ) * scaleY;
                    const float clipMaxX = ( cmd->ClipRect.z - clipOff.x ) * scaleX;
                    const float clipMaxY = ( cmd->ClipRect.w - clipOff.y ) * scaleY;

                    if( !std::isfinite( clipMinX ) || !std::isfinite( clipMinY ) ||
                        !std::isfinite( clipMaxX ) || !std::isfinite( clipMaxY ) )
                        continue;

                    const int clipL = clampInt( static_cast<int>( std::floor( clipMinX ) ), 0, vpWidth );
                    const int clipT =
                        clampInt( static_cast<int>( std::floor( clipMinY ) ), 0, vpHeight );
                    const int clipR = clampInt( static_cast<int>( std::ceil( clipMaxX ) ), 0, vpWidth );
                    const int clipB = clampInt( static_cast<int>( std::ceil( clipMaxY ) ), 0, vpHeight );

                    if( clipR <= clipL || clipB <= clipT )
                        continue;

                    wp_viewport_i scissor;
                    scissor.x = offX + clipL;
                    scissor.y = offY + clipT;
                    scissor.width = clipR - clipL;
                    scissor.height = clipB - clipT;
                    wp_renderer_set_scissor_enabled( m_renderer, 1 );
                    wp_renderer_set_scissor_rect( m_renderer, scissor );

                    if( prepared )
                    {
                        wp_renderer_dx11_draw_prepared_indexed_triangles_ptc(
                            dx11, static_cast<wp_s32>( indexOffset ),
                            static_cast<wp_s32>( cmd->ElemCount ),
                            static_cast<wp_s32>( cmd->VtxOffset ) );
                    }
                    else
                    {
                        const wp_vertex_ptc *vertices = converted.data();
                        if constexpr( sizeof( ImDrawIdx ) == sizeof( uint16_t ) )
                        {
                            const auto indices = idxBuf + indexOffset;
                            wp_renderer_draw_indexed_triangles_ptc(
                                m_renderer, vertices, static_cast<wp_s32>( vertexCount ), indices,
                                static_cast<wp_s32>( cmd->ElemCount ) );
                        }
                        else
                        {
                            if( !dx11 )
                            {
                                WP_LOG_ERROR( "32-bit ImDrawIdx requires DX11 u32 index support." );
                                continue;
                            }

                            wp_renderer_dx11_draw_indexed_triangles_ptc_u32(
                                dx11, vertices, static_cast<wp_s32>( vertexCount ),
                                reinterpret_cast<const uint32_t *>( idxBuf + indexOffset ),
                                static_cast<wp_s32>( cmd->ElemCount ) );
                        }
                    }
                }
            }

            wp_renderer_set_scissor_enabled( m_renderer, 0 );
            wp_renderer_set_blend_mode( m_renderer, savedBlend );
            wp_renderer_set_cull_mode( m_renderer, savedCull );
            wp_renderer_set_fill_mode( m_renderer, savedFill );
            wp_renderer_set_depth_test_enabled( m_renderer, savedDepthTest );
            wp_renderer_set_depth_write_enabled( m_renderer, savedDepthWrite );
            wp_renderer_set_depth_func( m_renderer, savedDepthFunc );
            wp_renderer_set_viewport( m_renderer, vp );
        }

        void ClawImguiManager::createFontTexture()
        {
            ScopedLock lock( this, true );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();

            if( !m_renderer )
                return;

            if( !ImGui::GetCurrentContext() )
            {
                WP_LOG_ERROR( "ClawImguiManager::createFontTexture: no ImGui context." );
                return;
            }

            ImGuiIO &io = ImGui::GetIO();
            if( !io.Fonts )
            {
                WP_LOG_ERROR( "ClawImguiManager::createFontTexture: font atlas is null." );
                return;
            }

            if( io.Fonts->Fonts.empty() )
                io.Fonts->AddFontDefault();

            // Load Font Awesome font (must happen before createFontTexture bakes the atlas)
            if( ui )
            {
                String fontPath = "fa-solid-900.ttf";
                if( !ui->loadFont( fontPath, "awesome" ) )
                {
                    WP_LOG_ERROR( "Failed to load Font Awesome font" );
                }
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
            io.Fonts->SetTexID( nativeHandle );
        }
    }  // namespace render
}  // namespace workphone
