#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include "WPGraphicsOgreNext/ImguiManagerOgreNext.hpp"
#include <WPImGui/WPImGui.hpp>
#include <Workphone/Workphone.hpp>
#include <chrono>
#include <OgrePass.h>
#include <OgreSceneManager.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreUnifiedHighLevelGpuProgram.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgreHlmsDatablock.h>
#include <OgreViewport.h>
#include <OgreRenderSystem.h>
#include <OgreRoot.h>
#include <OgreWindow.h>
#include <OgreTextureGpuManager.h>
#include <OgreStagingTexture.h>
#include <OgreTextureBox.h>
#include <CommandBuffer/OgreCbDrawCall.h>
#include <OgrePsoCacheHelper.h>
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstring>

#if WP_BUILD_RENDERER_DX11
#    include <OgreD3D11RenderSystem.h>
#endif

#if defined WP_PLATFORM_WIN32
#    include <imgui_impl_win32.h>
#endif

namespace workphone::render
{
    namespace
    {
        float getSafeFramebufferScale( float scale )
        {
            return std::isfinite( scale ) && scale > 0.0f ? scale : 1.0f;
        }

        int clampInt( int value, int minValue, int maxValue )
        {
            return std::max( minValue, std::min( value, maxValue ) );
        }

        void uploadViewportScissors( Ogre::RenderSystem *renderSystem, const Ogre::Viewport &viewport )
        {
#if WP_BUILD_RENDERER_DX11
            if( renderSystem && renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
            {
                auto *d3dRenderSystem = static_cast<Ogre::D3D11RenderSystem *>( renderSystem );
                auto *context = d3dRenderSystem ? d3dRenderSystem->_getDevice().GetImmediateContext() : nullptr;
                if( !context )
                    return;

                D3D11_RECT scissorRect;
                scissorRect.left = static_cast<LONG>( viewport.getScissorActualLeft() );
                scissorRect.top = static_cast<LONG>( viewport.getScissorActualTop() );
                scissorRect.right = static_cast<LONG>( viewport.getScissorActualLeft() +
                                                       viewport.getScissorActualWidth() );
                scissorRect.bottom = static_cast<LONG>( viewport.getScissorActualTop() +
                                                        viewport.getScissorActualHeight() );

                if( scissorRect.right > scissorRect.left && scissorRect.bottom > scissorRect.top )
                    context->RSSetScissorRects( 1u, &scissorRect );
            }
#else
            (void)renderSystem;
            (void)viewport;
#endif
        }

        void setViewportScissors( Ogre::RenderSystem *renderSystem, Ogre::Viewport &viewport, float left,
                                  float top, float width, float height )
        {
            viewport.setScissors( left, top, width, height );
            uploadViewportScissors( renderSystem, viewport );
        }

        void resetViewportScissors( Ogre::RenderSystem *renderSystem, Ogre::Viewport &viewport )
        {
            setViewportScissors( renderSystem, viewport, viewport.getLeft(), viewport.getTop(),
                                 viewport.getWidth(), viewport.getHeight() );
        }

        bool applyImguiClipRect( Ogre::RenderSystem *renderSystem, Ogre::Viewport &viewport,
                                 const ImDrawCmd &drawCmd, const ImVec2 &clipOffset,
                                 const ImVec2 &clipScale )
        {
            const int vpWidth = viewport.getActualWidth();
            const int vpHeight = viewport.getActualHeight();

            if( vpWidth <= 0 || vpHeight <= 0 )
                return false;

            const float scaleX = getSafeFramebufferScale( clipScale.x );
            const float scaleY = getSafeFramebufferScale( clipScale.y );

            const float clipMinX = ( drawCmd.ClipRect.x - clipOffset.x ) * scaleX;
            const float clipMinY = ( drawCmd.ClipRect.y - clipOffset.y ) * scaleY;
            const float clipMaxX = ( drawCmd.ClipRect.z - clipOffset.x ) * scaleX;
            const float clipMaxY = ( drawCmd.ClipRect.w - clipOffset.y ) * scaleY;

            if( !std::isfinite( clipMinX ) || !std::isfinite( clipMinY ) ||
                !std::isfinite( clipMaxX ) || !std::isfinite( clipMaxY ) )
            {
                return false;
            }

            const int clipL = clampInt( static_cast<int>( std::floor( clipMinX ) ), 0, vpWidth );
            const int clipT = clampInt( static_cast<int>( std::floor( clipMinY ) ), 0, vpHeight );
            const int clipR = clampInt( static_cast<int>( std::ceil( clipMaxX ) ), 0, vpWidth );
            const int clipB = clampInt( static_cast<int>( std::ceil( clipMaxY ) ), 0, vpHeight );

            if( clipR <= clipL || clipB <= clipT )
                return false;

            const float vpWidthF = static_cast<float>( vpWidth );
            const float vpHeightF = static_cast<float>( vpHeight );
            const float left = viewport.getLeft() + viewport.getWidth() *
                                                    ( static_cast<float>( clipL ) / vpWidthF );
            const float top = viewport.getTop() + viewport.getHeight() *
                                                 ( static_cast<float>( clipT ) / vpHeightF );
            const float width = viewport.getWidth() *
                                ( static_cast<float>( clipR - clipL ) / vpWidthF );
            const float height = viewport.getHeight() *
                                 ( static_cast<float>( clipB - clipT ) / vpHeightF );

            setViewportScissors( renderSystem, viewport, left, top, width, height );
            return true;
        }
    }  // namespace

    // ---------------------------------------------------------------------------
    // Per-viewport data stored in ImGuiViewport::RendererUserData
    // ---------------------------------------------------------------------------
    struct ImGui_OgreNext_ViewportData
    {
        Ogre::Window *Window = nullptr;
        bool WindowOwned = false;
    };

    // ---------------------------------------------------------------------------
    // ImGuiPlatformIO renderer callbacks
    // ---------------------------------------------------------------------------
    static void ImGui_OgreNext_CreateWindow( ImGuiViewport *viewport )
    {
        auto *manager = ImguiManagerOgreNext::getSingletonPtr();
        if( !manager || !manager->isInitialised() )
            return;

        auto *vd = new ImGui_OgreNext_ViewportData();
        viewport->RendererUserData = vd;

        Ogre::Root *root = Ogre::Root::getSingletonPtr();
        if( !root )
            return;

        Ogre::NameValuePairList params;
#if defined WP_PLATFORM_WIN32
        if( viewport->PlatformHandle )
            params["externalWindowHandle"] =
                Ogre::StringConverter::toString( reinterpret_cast<size_t>( viewport->PlatformHandle ) );
#endif

        std::string windowName = "ImGui_Viewport_" + std::to_string( viewport->ID );
        vd->Window =
            root->createRenderWindow( windowName.c_str(), static_cast<Ogre::uint32>( viewport->Size.x ),
                                      static_cast<Ogre::uint32>( viewport->Size.y ), false, &params );
        vd->WindowOwned = true;
    }

    static void ImGui_OgreNext_DestroyWindow( ImGuiViewport *viewport )
    {
        if( viewport->RendererUserData )
        {
            auto *vd = static_cast<ImGui_OgreNext_ViewportData *>( viewport->RendererUserData );
            if( vd->WindowOwned && vd->Window )
            {
                vd->Window->destroy();
                vd->Window = nullptr;
            }
            delete vd;
            viewport->RendererUserData = nullptr;
        }
    }

    static void ImGui_OgreNext_SetWindowSize( ImGuiViewport *viewport, ImVec2 size )
    {
        auto *vd = static_cast<ImGui_OgreNext_ViewportData *>( viewport->RendererUserData );
        if( vd && vd->Window )
            vd->Window->requestResolution( static_cast<Ogre::uint32>( size.x ),
                                           static_cast<Ogre::uint32>( size.y ) );
    }

    static void ImGui_OgreNext_RenderWindow( ImGuiViewport *viewport, void * /*render_arg*/ )
    {
        // Secondary viewport rendering — draw data is available in viewport->DrawData.
        // Full multi-window rendering would submit viewport->DrawData to the per-viewport
        // Ogre::Window stored in RendererUserData. This is a stub for secondary viewports;
        // the primary viewport is rendered by ImguiManagerOgreNext::render().
        (void)viewport;
    }

    static void ImGui_OgreNext_SwapBuffers( ImGuiViewport *viewport, void * /*render_arg*/ )
    {
        // OgreNext performs buffer swapping internally during Root::renderOneFrame().
        // Nothing additional is needed here.
        (void)viewport;
    }

    WP_CLASS_REGISTER_DERIVED( workphone::render, ImguiManagerOgreNext, ISharedObject );

    ImguiManagerOgreNext *ImguiManagerOgreNext::ms_singleton = nullptr;

    auto ImguiManagerOgreNext::getSingletonPtr() -> ImguiManagerOgreNext *
    {
        if( !ms_singleton )
        {
            ms_singleton = new ImguiManagerOgreNext();
        }

        return ms_singleton;
    }
    auto ImguiManagerOgreNext::getSingleton() -> ImguiManagerOgreNext &
    {
        if( !ms_singleton )
        {
            ms_singleton = new ImguiManagerOgreNext();
        }

        return ( *ms_singleton );
    }

    ImguiManagerOgreNext::ImguiManagerOgreNext() :
        m_sceneMgr( nullptr ),
        m_psoCache( nullptr ),
        m_pass( nullptr ),
        m_lastRenderedFrame( 4 ),
        m_frameEnded( true ),
        m_vp( nullptr )
    {
    }

    ImguiManagerOgreNext::~ImguiManagerOgreNext()
    {
        shutdown();
    }

    void ImguiManagerOgreNext::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        m_plugin = workphone::make_ptr<ui::WPImGui>();
        m_plugin->load( nullptr );

        setLoadingState( LoadingState::Loaded );
    }

    void ImguiManagerOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        shutdown();

        if( m_plugin )
        {
            m_plugin->unload( nullptr );
            m_plugin = nullptr;
        }

        setLoadingState( LoadingState::Unloaded );
    }

    void ImguiManagerOgreNext::shutdown()
    {
        // Clean up main viewport renderer user data
        if( ImGui::GetCurrentContext() )
        {
            ImGuiIO &io = ImGui::GetIO();
            io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
            if( io.BackendRendererUserData == this )
                io.BackendRendererUserData = nullptr;
            if( io.BackendRendererName && strcmp( io.BackendRendererName, "imgui_impl_ogrenext" ) == 0 )
            {
                io.BackendRendererName = nullptr;
            }

            ImGuiViewport *mainViewport = ImGui::GetMainViewport();
            if( mainViewport && mainViewport->RendererUserData )
            {
                delete static_cast<ImGui_OgreNext_ViewportData *>( mainViewport->RendererUserData );
                mainViewport->RendererUserData = nullptr;
            }
        }

        while( m_renderables.size() > 0 )
        {
            delete m_renderables.back();
            m_renderables.pop_back();
        }

        if( m_psoCache )
        {
            delete m_psoCache;
            m_psoCache = nullptr;
        }
    }

    void ImguiManagerOgreNext::init( Ogre::SceneManager *mgr )
    {
        ScopedLock lock( this, true );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto application = applicationManager->getApplication();
        if( !application )
        {
            WP_LOG_ERROR( "Application instance not found during ImguiManagerOgreNext initialization." );
            return;
        }

        auto ui = applicationManager->getUI();

        auto applicationName = application->getName();

        static const String iniFileExt = ".ini";
        static const String logFileExt = "_ui.log";

#if defined WP_PLATFORM_WIN32
        m_iniPath = applicationName + iniFileExt;
        m_logPath = applicationName + logFileExt;
#elif defined WP_PLATFORM_APPLE
        m_iniPath = macGetConfigFilePath() + Path::separatorStr + applicationName + iniFileExt;
        m_logPath = macGetLogFilePath() + Path::separatorStr + applicationName + logFileExt;
#elif defined WP_PLATFORM_LINUX
        m_iniPath = applicationName + iniFileExt;
        m_logPath = applicationName + logFileExt;
#else
        m_iniPath = applicationName + iniFileExt;
        m_logPath = applicationName + logFileExt;
#endif

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.BackendRendererName = "imgui_impl_ogrenext";
        io.BackendRendererUserData = this;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io.IniFilename = m_iniPath.c_str();
        io.LogFilename = m_logPath.c_str();

        m_sceneMgr = mgr;

        auto root = Ogre::Root::getSingletonPtr();
        auto renderSystem = root->getRenderSystem();

        m_psoCache = new Ogre::PsoCacheHelper( renderSystem );

        // Add default font first, then merge any additional fonts before baking the atlas.
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

        createFontTexture();
        createMaterial();

        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
        // io.ConfigFlags |= ImGuiDockNodeFlags_PassthruCentralNode;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        if( auto window = graphicsSystem->getDefaultWindow() )
        {
            // Seed the projection matrix from the window size before the
            // first render viewport is available
            auto size = window->getSize();
            updateProjectionMatrix( static_cast<float>( size.X() ), static_cast<float>( size.Y() ) );

            size_t windowHandle = 0;
            window->getCustomAttribute( "WINDOW", &windowHandle );

            if( windowHandle != 0 )
            {
#if defined WP_PLATFORM_WIN32
                ImGui_ImplWin32_Init( (HWND)windowHandle );
#endif
            }
        }

        // Set up ImGui platform IO renderer callbacks for multi-viewport support
        ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
        platform_io.Renderer_CreateWindow = ImGui_OgreNext_CreateWindow;
        platform_io.Renderer_DestroyWindow = ImGui_OgreNext_DestroyWindow;
        platform_io.Renderer_SetWindowSize = ImGui_OgreNext_SetWindowSize;
        platform_io.Renderer_RenderWindow = ImGui_OgreNext_RenderWindow;
        platform_io.Renderer_SwapBuffers = ImGui_OgreNext_SwapBuffers;

        // Register the main viewport's renderer user data
        ImGuiViewport *mainViewport = ImGui::GetMainViewport();
        auto *vd = new ImGui_OgreNext_ViewportData();
        mainViewport->RendererUserData = vd;
    }

    bool ImguiManagerOgreNext::isInitialised() const
    {
        return m_sceneMgr != nullptr;
    }

    void ImguiManagerOgreNext::setupViewport( Ogre::Viewport *viewport )
    {
        m_vp = viewport;

        if( m_vp )
        {
            float width = static_cast<float>( m_vp->getActualWidth() );
            float height = static_cast<float>( m_vp->getActualHeight() );
            updateProjectionMatrix( width, height );
        }
    }

    void ImguiManagerOgreNext::newFrame()
    {
        ScopedLock lock( this, true );

        m_frameEnded = false;
        ImGuiIO &io = ImGui::GetIO();

        // Delta time
        static auto lastTime = std::chrono::high_resolution_clock::now();
        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>( now - lastTime ).count();
        lastTime = now;
        io.DeltaTime = deltaTime > 0.0f ? deltaTime : ( 1.0f / 60.0f );

        // Display size — use actual viewport dimensions when available
        float width = 400.0f;
        float height = 400.0f;

        if( m_vp )
        {
            width = static_cast<float>( m_vp->getActualWidth() );
            height = static_cast<float>( m_vp->getActualHeight() );
        }

        io.DisplaySize = ImVec2( width, height );

#if defined WP_PLATFORM_WIN32
        ImGui_ImplWin32_NewFrame();
#endif

        // Start the frame
        ImGui::NewFrame();
    }

    void ImguiManagerOgreNext::updateProjectionMatrix( float width, float height )
    {
        if( !m_pass || width <= 0.0f || height <= 0.0f )
            return;

        Ogre::Matrix4 projMatrix( 2.0f / width, 0.0f, 0.0f, -1.0f, 0.0f, -2.0f / height, 0.0f, 1.0f,
                                  0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f );

        m_pass->getVertexProgramParameters()->setNamedConstant( "ProjectionMatrix", projMatrix );

        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize = ImVec2( width, height );
    }

    void ImguiManagerOgreNext::render()
    {
        ScopedLock lock( this, true );

        if( !m_sceneMgr || !m_pass || !m_psoCache )
            return;

        // Cancel rendering if newFrame() was not called this frame
        if( m_frameEnded )
            return;

        m_frameEnded = true;

        int currentFrame = ImGui::GetFrameCount();
        if( currentFrame == m_lastRenderedFrame )
            return;

        m_lastRenderedFrame = currentFrame;

        // Finalise ImGui draw data
        ImGui::Render();

        ImDrawData *drawData = ImGui::GetDrawData();
        if( !drawData || drawData->CmdListsCount == 0 )
            return;

        Ogre::RenderSystem *renderSystem = m_sceneMgr->getDestinationRenderSystem();
        if( !renderSystem )
            return;

        Ogre::RenderPassDescriptor *passDescriptor = renderSystem->getCurrentPassDescriptor();
        if( !passDescriptor )
            return;

        // Use Ogre's live viewport object. A copied viewport would not update the active
        // scissor state and lets scroll-window contents bleed outside their clip rects.
        Ogre::Viewport &liveVp = renderSystem->_getCurrentRenderViewport();
        setupViewport( &liveVp );
        const float viewportWidth = static_cast<float>( liveVp.getActualWidth() );
        const float viewportHeight = static_cast<float>( liveVp.getActualHeight() );

        if( viewportWidth <= 0.0f || viewportHeight <= 0.0f )
            return;

        updateProjectionMatrix( viewportWidth, viewportHeight );

        // Bind the font texture
        if( m_fontTexture )
            renderSystem->_setTexture( 0, m_fontTexture, false );

        // Rebuild PSO cache state every frame
        m_psoCache->clearState();
        m_psoCache->setRenderTarget( passDescriptor );
        m_psoCache->setMacroblock( m_pass->getMacroblock() );
        m_psoCache->setBlendblock( m_pass->getBlendblock() );
        m_psoCache->setVertexShader( const_cast<Ogre::GpuProgramPtr &>( m_pass->getVertexProgram() ) );
        m_psoCache->setPixelShader( const_cast<Ogre::GpuProgramPtr &>( m_pass->getFragmentProgram() ) );

        int numberRenderables = 0;

        // Pre-compute display offset and framebuffer scale once per frame.
        // DisplayPos is non-zero in multi-viewport scenarios.
        // FramebufferScale handles HiDPI / Retina displays.
        const ImVec2 clipOff = drawData->DisplayPos;
        const ImVec2 clipScale = drawData->FramebufferScale;

        // Iterate through all ImGui draw lists
        for( int n = 0; n < drawData->CmdListsCount; n++ )
        {
            const ImDrawList *drawList = drawData->CmdLists[n];
            if( !drawList || drawList->VtxBuffer.empty() || drawList->IdxBuffer.empty() )
                continue;

            const ImDrawVert *vtxBuf = drawList->VtxBuffer.Data;
            const ImDrawIdx *idxBuf = drawList->IdxBuffer.Data;
            const unsigned int vertexCount = static_cast<unsigned int>( drawList->VtxBuffer.Size );
            const unsigned int indexCount = static_cast<unsigned int>( drawList->IdxBuffer.Size );
            ImguiRenderableOgreNext *renderable = nullptr;

            for( int i = 0; i < drawList->CmdBuffer.Size; i++ )
            {
                const ImDrawCmd *drawCmd = &drawList->CmdBuffer[i];
                if( !drawCmd || drawCmd->ElemCount == 0 )
                    continue;

                if( drawCmd->UserCallback )
                {
                    if( drawCmd->UserCallback == ImDrawCallback_ResetRenderState )
                    {
                        resetViewportScissors( renderSystem, liveVp );
                    }
                    else
                    {
                        drawCmd->UserCallback( drawList, drawCmd );
                    }

                    continue;
                }

                const unsigned int vertexOffset = drawCmd->VtxOffset;
                const unsigned int indexOffset = drawCmd->IdxOffset;

                if( vertexOffset >= vertexCount || indexOffset >= indexCount ||
                    drawCmd->ElemCount > indexCount - indexOffset )
                {
                    continue;
                }

                if( !applyImguiClipRect( renderSystem, liveVp, *drawCmd, clipOff, clipScale ) )
                    continue;

                // Upload each draw list once, then reuse it for every command range in that list.
                if( !renderable )
                {
                    if( numberRenderables >= static_cast<int>( m_renderables.size() ) )
                        m_renderables.push_back( new ImguiRenderableOgreNext() );

                    renderable = m_renderables[numberRenderables++];
                    renderable->updateVertexData( vtxBuf, idxBuf, vertexCount, indexCount );
                }
                renderable->setDrawRange( vertexOffset, indexOffset, drawCmd->ElemCount );

                // Resolve which texture to use for this draw command.
                Ogre::TextureGpu *texToBind = nullptr;
                if( drawCmd->GetTexID() )
                    texToBind = static_cast<Ogre::TextureGpu *>( drawCmd->GetTexID() );

                // Build and apply PSO for this draw call
                Ogre::v1::RenderOperation renderOp;
                renderable->getRenderOperation( renderOp, false );

                Ogre::VertexElement2VecVec vertexElements =
                    renderOp.vertexData->vertexDeclaration->convertToV2();
                m_psoCache->setVertexFormat( vertexElements, renderOp.operationType, false );

                Ogre::HlmsPso *pso = m_psoCache->getPso();
                renderSystem->_setPipelineStateObject( pso );

                renderSystem->bindGpuProgramParameters(
                    Ogre::GPT_VERTEX_PROGRAM, m_pass->getVertexProgramParameters(), Ogre::GPV_ALL );

                // Bind texture after PSO is set — D3D11 resets SRV slots when applying a new PSO.
                // Only bind if the texture is fully resident; non-resident textures have no SRV yet.
                if( texToBind && texToBind->getResidencyStatus() == Ogre::GpuResidency::Resident &&
                    texToBind->isDataReady() )
                {
                    renderSystem->_setTexture( 0, texToBind, false );
                }
                else
                {
                    // Bind the font texture
                    if( m_fontTexture )
                        renderSystem->_setTexture( 0, m_fontTexture, false );
                }

                Ogre::v1::CbRenderOp op( renderOp );
                renderSystem->_setRenderOperation( &op );
                renderSystem->_render( renderOp );
            }
        }

        resetViewportScissors( renderSystem, liveVp );
    }

    void ImguiManagerOgreNext::createMaterial()
    {
        ScopedLock lock( this, true );

        static const char *vertexShaderSrcD3D11 = {
            "uniform float4x4 ProjectionMatrix;\n"
            "struct VS_INPUT\n"
            "{\n"
            "float2 pos : POSITION;\n"
            "float4 col : COLOR0;\n"
            "float2 uv  : TEXCOORD0;\n"
            "};\n"
            "struct PS_INPUT\n"
            "{\n"
            "float4 pos : SV_POSITION;\n"
            "float4 col : COLOR0;\n"
            "float2 uv  : TEXCOORD0;\n"
            "};\n"
            "PS_INPUT main(VS_INPUT input)\n"
            "{\n"
            "PS_INPUT output;\n"
            "output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\n"
            "output.col = input.col;\n"
            "output.uv  = input.uv;\n"
            "return output;\n"
            "}"
        };

        static const char *pixelShaderSrcD3D11 = {
            "struct PS_INPUT\n"
            "{\n"
            "float4 pos : SV_POSITION;\n"
            "float4 col : COLOR0;\n"
            "float2 uv  : TEXCOORD0;\n"
            "};\n"
            "sampler sampler0: register(s0);\n"
            "Texture2D texture0: register(t0);\n"
            "\n"
            "float4 main(PS_INPUT input) : SV_Target\n"
            "{\n"
            "float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \n"
            "return out_col; \n"
            "}"
        };

        static const char *vertexShaderSrcGLSL = {
            "#version 150\n"
            "uniform mat4 ProjectionMatrix; \n"
            "in vec2 vertex;\n"
            "in vec2 uv0;\n"
            "in vec4 colour;\n"
            "out vec2 Texcoord;\n"
            "out vec4 col;\n"
            "void main()\n"
            "{\n"
            "gl_Position = ProjectionMatrix* vec4(vertex.xy, 0.f, 1.f);\n"
            "Texcoord  = uv0;\n"
            "col = colour;\n"
            "}"
        };

        static const char *pixelShaderSrcGLSL = {
            "#version 150\n"
            "in vec2 Texcoord;\n"
            "in vec4 col;\n"
            "uniform sampler2D sampler0;\n"
            "out vec4 out_col;\n"
            "void main()\n"
            "{\n"
            "out_col = col * texture(sampler0, Texcoord);\n"
            "}"
        };

        static const char *fragmentShaderSrcMetal = {
            "#include <metal_stdlib>\n"
            "using namespace metal;\n"
            "\n"
            "struct VertexOut {\n"
            "    float4 position [[position]];\n"
            "    float2 texCoords;\n"
            "    float4 colour;\n"
            "};\n"
            "\n"
            "fragment float4 main_metal(VertexOut in [[stage_in]],\n"
            "                             texture2d<float> texture [[texture(0)]]) {\n"
            "    constexpr sampler linearSampler(coord::normalized, min_filter::linear, "
            "mag_filter::linear, "
            "mip_filter::linear);\n"
            "    float4 texColour = texture.sample(linearSampler, in.texCoords);\n"
            "    return in.colour * texColour;\n"
            "}\n"
        };

        static const char *vertexShaderSrcMetal = {
            "#include <metal_stdlib>\n"
            "using namespace metal;\n"
            "\n"
            "struct Constant {\n"
            "    float4x4 ProjectionMatrix;\n"
            "};\n"
            "\n"
            "struct VertexIn {\n"
            "    float2 position  [[attribute(VES_POSITION)]];\n"
            "    float2 texCoords [[attribute(VES_TEXTURE_COORDINATES0)]];\n"
            "    float4 colour     [[attribute(VES_DIFFUSE)]];\n"
            "};\n"
            "\n"
            "struct VertexOut {\n"
            "    float4 position [[position]];\n"
            "    float2 texCoords;\n"
            "    float4 colour;\n"
            "};\n"
            "\n"
            "vertex VertexOut vertex_main(VertexIn in                 [[stage_in]],\n"
            "                             constant Constant &uniforms [[buffer(PARAMETER_SLOT)]]) {\n"
            "    VertexOut out;\n"
            "    out.position = uniforms.ProjectionMatrix * float4(in.position, 0, 1);\n"

            "    out.texCoords = in.texCoords;\n"
            "    out.colour = in.colour;\n"

            "    return out;\n"
            "}\n"
        };

        //create the default shadows material
        Ogre::HighLevelGpuProgramManager &mgr = Ogre::HighLevelGpuProgramManager::getSingleton();

        Ogre::HighLevelGpuProgramPtr vertexShaderUnified = mgr.getByName( "imgui/VP" );
        Ogre::HighLevelGpuProgramPtr pixelShaderUnified = mgr.getByName( "imgui/FP" );

        Ogre::HighLevelGpuProgramPtr vertexShaderD3D11 = mgr.getByName( "imgui/VP/D3D11" );
        Ogre::HighLevelGpuProgramPtr pixelShaderD3D11 = mgr.getByName( "imgui/FP/D3D11" );

        Ogre::HighLevelGpuProgramPtr vertexShaderGL = mgr.getByName( "imgui/VP/GL150" );
        Ogre::HighLevelGpuProgramPtr pixelShaderGL = mgr.getByName( "imgui/FP/GL150" );

        Ogre::HighLevelGpuProgramPtr vertexShaderMetal = mgr.getByName( "imgui/VP/Metal" );
        Ogre::HighLevelGpuProgramPtr pixelShaderMetal = mgr.getByName( "imgui/FP/Metal" );

        if( vertexShaderUnified.isNull() )
        {
            vertexShaderUnified =
                mgr.createProgram( "imgui/VP", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                   "unified", Ogre::GPT_VERTEX_PROGRAM );
        }

        if( pixelShaderUnified.isNull() )
        {
            pixelShaderUnified =
                mgr.createProgram( "imgui/FP", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                   "unified", Ogre::GPT_FRAGMENT_PROGRAM );
        }

        auto *vertexShaderPtr =
            static_cast<Ogre::UnifiedHighLevelGpuProgram *>( vertexShaderUnified.get() );
        auto *pixelShaderPtr =
            static_cast<Ogre::UnifiedHighLevelGpuProgram *>( pixelShaderUnified.get() );

        if( vertexShaderD3D11.isNull() )
        {
            vertexShaderD3D11 = mgr.createProgram(
                "imgui/VP/D3D11", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "hlsl",
                Ogre::GPT_VERTEX_PROGRAM );
            vertexShaderD3D11->setParameter( "target", "vs_4_0" );
            vertexShaderD3D11->setParameter( "entry_point", "main" );
            vertexShaderD3D11->setSource( vertexShaderSrcD3D11 );
            vertexShaderD3D11->load();

            vertexShaderPtr->addDelegateProgram( vertexShaderD3D11->getName() );
        }

        if( pixelShaderD3D11.isNull() )
        {
            pixelShaderD3D11 = mgr.createProgram(
                "imgui/FP/D3D11", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "hlsl",
                Ogre::GPT_FRAGMENT_PROGRAM );
            pixelShaderD3D11->setParameter( "target", "ps_4_0" );
            pixelShaderD3D11->setParameter( "entry_point", "main" );
            pixelShaderD3D11->setSource( pixelShaderSrcD3D11 );
            pixelShaderD3D11->load();

            pixelShaderPtr->addDelegateProgram( pixelShaderD3D11->getName() );
        }

        if( vertexShaderMetal.isNull() )
        {
            vertexShaderMetal = mgr.createProgram(
                "imgui/VP/Metal", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "metal",
                Ogre::GPT_VERTEX_PROGRAM );
            vertexShaderMetal->setParameter( "entry_point", "vertex_main" );
            vertexShaderMetal->setSource( vertexShaderSrcMetal );
            vertexShaderMetal->load();
            vertexShaderPtr->addDelegateProgram( vertexShaderMetal->getName() );
        }

        if( pixelShaderMetal.isNull() )
        {
            pixelShaderMetal = mgr.createProgram(
                "imgui/FP/Metal", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "metal",
                Ogre::GPT_FRAGMENT_PROGRAM );
            vertexShaderMetal->setParameter( "entry_point", "fragment_main" );
            pixelShaderMetal->setSource( fragmentShaderSrcMetal );
            pixelShaderMetal->load();
            pixelShaderPtr->addDelegateProgram( pixelShaderMetal->getName() );
        }

        if( vertexShaderGL.isNull() )
        {
            vertexShaderGL = mgr.createProgram( "imgui/VP/GL150",
                                                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                "glsl", Ogre::GPT_VERTEX_PROGRAM );
            vertexShaderGL->setSource( vertexShaderSrcGLSL );
            vertexShaderGL->load();
            vertexShaderPtr->addDelegateProgram( vertexShaderGL->getName() );
        }

        if( pixelShaderGL.isNull() )
        {
            pixelShaderGL = mgr.createProgram( "imgui/FP/GL150",
                                               Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                               "glsl", Ogre::GPT_FRAGMENT_PROGRAM );
            pixelShaderGL->setSource( pixelShaderSrcGLSL );
            pixelShaderGL->load();
            pixelShaderGL->setParameter( "sampler0", "int 0" );

            pixelShaderPtr->addDelegateProgram( pixelShaderGL->getName() );
        }

        Ogre::MaterialPtr imguiMaterial = Ogre::MaterialManager::getSingleton().create(
            "imgui/material", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
        m_pass = imguiMaterial->getTechnique( 0 )->getPass( 0 );
        m_pass->setFragmentProgram( "imgui/FP" );
        m_pass->setVertexProgram( "imgui/VP" );

        Ogre::HlmsBlendblock blendblock( *m_pass->getBlendblock() );
        blendblock.mSourceBlendFactor = Ogre::SBF_SOURCE_ALPHA;
        blendblock.mDestBlendFactor = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;
        blendblock.mSourceBlendFactorAlpha = Ogre::SBF_ONE;
        blendblock.mDestBlendFactorAlpha = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;
        blendblock.mBlendOperation = Ogre::SBO_ADD;
        blendblock.mBlendOperationAlpha = Ogre::SBO_ADD;
        blendblock.mSeparateBlend = true;
        blendblock.mIsTransparent = true;

        Ogre::HlmsMacroblock macroblock( *m_pass->getMacroblock() );
        macroblock.mCullMode = Ogre::CULL_NONE;
        macroblock.mDepthFunc = Ogre::CMPF_ALWAYS_PASS;
        macroblock.mDepthCheck = false;
        macroblock.mDepthWrite = false;
        macroblock.mScissorTestEnabled = true;

        m_pass->setBlendblock( blendblock );
        m_pass->setMacroblock( macroblock );

        Ogre::String renderSystemName = m_sceneMgr->getDestinationRenderSystem()->getName();

        //Apparently opengl was the only one that needed this.
        if( renderSystemName == "OpenGL 3+ Rendering Subsystem" )
        {
            m_pass->getFragmentProgramParameters()->setNamedConstant( "sampler0", 0 );
        }

        m_pass->createTextureUnitState()->setTextureName( "ImguiFontTex" );
    }

    void ImguiManagerOgreNext::createFontTexture()
    {
        ScopedLock lock( this, true );

        // Build texture atlas
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        unsigned char *pixels = nullptr;
        int width = 0, height = 0;

        // If no fonts were registered before this call, add the default font as a fallback.
        if( io.Fonts->Fonts.empty() )
            io.Fonts->AddFontDefault();
        io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );

        if( !pixels || width <= 0 || height <= 0 )
            return;

        Ogre::Root *root = Ogre::Root::getSingletonPtr();
        if( !root )
            return;

        Ogre::RenderSystem *renderSystem = root->getRenderSystem();
        if( !renderSystem )
            return;

        Ogre::TextureGpuManager *textureManager = renderSystem->getTextureGpuManager();
        if( !textureManager )
            return;

        Ogre::TextureGpu *fontTexture = textureManager->createTexture(
            "ImguiFontTex", Ogre::GpuPageOutStrategy::Discard, Ogre::TextureFlags::ManualTexture,
            Ogre::TextureTypes::Type2D );
        fontTexture->setResolution( static_cast<Ogre::uint32>( width ),
                                    static_cast<Ogre::uint32>( height ) );
        fontTexture->setPixelFormat( Ogre::PFG_RGBA8_UNORM );
        fontTexture->scheduleTransitionTo( Ogre::GpuResidency::Resident );

        Ogre::StagingTexture *stagingTexture = textureManager->getStagingTexture(
            static_cast<Ogre::uint32>( width ), static_cast<Ogre::uint32>( height ), 1u, 1u,
            Ogre::PFG_RGBA8_UNORM );
        stagingTexture->startMapRegion();
        Ogre::TextureBox texBox = stagingTexture->mapRegion( static_cast<Ogre::uint32>( width ),
                                                             static_cast<Ogre::uint32>( height ), 1u, 1u,
                                                             Ogre::PFG_RGBA8_UNORM );

        const size_t srcBytesPerRow = static_cast<size_t>( width ) * 4u;
        const Ogre::uint8 *srcData = static_cast<const Ogre::uint8 *>( pixels );
        Ogre::uint8 *dstData = static_cast<Ogre::uint8 *>( texBox.data );
        for( int y = 0; y < height; ++y )
        {
            memcpy( dstData, srcData, srcBytesPerRow );
            srcData += srcBytesPerRow;
            dstData += texBox.bytesPerRow;
        }

        stagingTexture->stopMapRegion();
        stagingTexture->upload( texBox, fontTexture, 0, 0, 0 );
        textureManager->removeStagingTexture( stagingTexture );

        // Force the texture manager to process the residency transition immediately so the
        // D3D11 SRV exists before any _setTexture() call this frame.
        fontTexture->notifyDataIsReady();

        m_fontTexture = fontTexture;
        io.Fonts->SetTexID( static_cast<ImTextureID>( fontTexture ) );
    }

}  // namespace workphone::render
