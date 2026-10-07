#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawDebug.hpp>
#include <WPGraphics/ClawOverlayManager.hpp>
#include <WPGraphics/ClawScene.hpp>
#include <WPGraphics/ClawWindow.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawShader.hpp>
#include <WPGraphics/ClawResourceGroupManager.hpp>
#include <WPGraphics/ClawFontManager.hpp>
#include <WPGraphics/ClawGraphicsPipeline.hpp>
#include <WPGraphics/ClawMaterialManager.hpp>
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawMaterialPass.hpp>
#include <WPGraphics/ClawMaterialTechnique.hpp>
#include <WPGraphics/ClawMaterialTexture.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/ClawTextureManager.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRendererDX12.hpp>
#include <WPGraphics/ClawRendererSoftware.hpp>
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <WPGraphics/ClawImguiManager.hpp>
#include <Workphone/Workphone.hpp>
#include <WPImGui/WPImGui.hpp>
#include "workphone_graphics_system.h"
#include "workphone_graphics_renderer_dx11.h"
#include <chrono>
#include <cstdio>

namespace workphone
{
    namespace render
    {
        namespace
        {
            // Keep phase timings separate from the render task so CPU scene
            // preparation, editor UI work and presentation waits can be compared.
            class RenderPhaseProfile
            {
            public:
                RenderPhaseProfile( WeakPtr<IProfile> &cached, const char *label )
                {
                    m_profile = cached.lock();
                    if( !m_profile )
                        if( auto app = core::IApplicationManager::instancePtr() )
                            if( auto profiler = app->getProfiler() )
                            {
                                m_profile = profiler->addProfile();
                                m_profile->setLabel( label );
                                cached = m_profile;
                            }
                    if( m_profile )
                        m_profile->start();
                }
                ~RenderPhaseProfile() { end(); }
                void end()
                {
                    if( m_profile )
                    {
                        m_profile->end();
                        m_profile = nullptr;
                    }
                }
            private:
                SmartPtr<IProfile> m_profile;
            };

            const char *renderApiName( IGraphicsSystem::RenderApi api )
            {
                using Api = IGraphicsSystem::RenderApi;
                switch( api )
                {
                case Api::None:
                    return "Automatic";
                case Api::Software:
                    return "Software";
                case Api::DX9:
                    return "DirectX 9";
                case Api::DX11:
                    return "DirectX 11";
                case Api::DX12:
                    return "DirectX 12";
                case Api::GL:
                    return "OpenGL";
                case Api::GL3Plus:
                    return "OpenGL 3+";
                case Api::Vulkan:
                    return "Vulkan";
                case Api::Metal:
                    return "Metal";
                default:
                    return "Unknown";
                }
            }

            String initializationTime( std::chrono::steady_clock::time_point start )
            {
                return std::to_string( std::chrono::duration_cast<std::chrono::milliseconds>(
                                           std::chrono::steady_clock::now() - start )
                                           .count() ) +
                       " ms";
            }
        }  // namespace
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawHammerSystem, GraphicsSystem );

        ClawHammerSystem::ClawHammerSystem() : m_sys( nullptr )
        {
        }

        ClawHammerSystem::~ClawHammerSystem()
        {
            if( m_sys )
            {
                wp_graphics_system_destroy( m_sys );
                m_sys = nullptr;
            }
        }

        SmartPtr<Properties> ClawHammerSystem::getProperties() const
        {
            SmartPtr<Properties> props = GraphicsSystem::getProperties();
            if( props )
            {
                props->setProperty( "native_system_active", m_sys != nullptr );
                props->setProperty( "vsync", getVSync() );
                std::lock_guard<std::mutex> lock( m_statisticsMutex );
                const auto split = m_renderStatistics.find( '\n' );
                props->setProperty( "render_statistics", m_renderStatistics.substr( 0, split ) );
                if( split != String::npos )
                    props->setProperty( "render_counters", m_renderStatistics.substr( split + 1 ) );
            }
            return props;
        }

        void ClawHammerSystem::setProperties( SmartPtr<Properties> properties )
        {
            GraphicsSystem::setProperties( properties );
            if( properties )
            {
                bool requested = false;
                if( properties->getPropertyValue( "reset_render_statistics", requested ) && requested )
                {
                    m_statisticsResetRequested = true;
                    std::lock_guard<std::mutex> lock( m_statisticsMutex );
                    m_renderStatistics.clear();
                }
                requested = false;
                if( properties->getPropertyValue( "request_render_statistics", requested ) && requested )
                    m_statisticsSnapshotRequested = true;
                bool enabled;
                if( properties->getPropertyValue( "vsync", enabled ) )
                    m_vsync.store( enabled, std::memory_order_relaxed );
            }
        }

        void ClawHammerSystem::publishRenderStatistics()
        {
            if( !m_statisticsSnapshotRequested.exchange( false ) ) return;
            String result = "Render statistics unavailable";
            if( auto renderer = dynamic_pointer_cast<ClawRendererDX11>( getRenderer() ) )
                if( auto dx11 = wp_renderer_get_dx11( renderer->getNativeRenderer() ) )
                {
                    wp_render_statistics_dx11 stats{};
                    wp_renderer_dx11_get_statistics( dx11, &stats );
                    char summary[1024];
                    std::snprintf( summary, sizeof( summary ),
                        "Render frames=%llu interval mean=%.3f ms p95=%.3f ms render-pass CPU=%.3f ms Present=%.3f ms GPU=%.3f ms GPU samples=%llu\nDraws=%llu triangles=%llu material uploads=%llu transform uploads=%llu geometry creations=%llu state bindings=%llu presentation=%s",
                        (unsigned long long)stats.frames, stats.interval_ms, stats.interval_p95_ms,
                        stats.cpu_frame_ms, stats.present_ms, stats.gpu_frame_ms,
                        (unsigned long long)stats.gpu_samples, (unsigned long long)stats.draws,
                        (unsigned long long)stats.triangles, (unsigned long long)stats.material_uploads,
                        (unsigned long long)stats.transform_uploads, (unsigned long long)stats.geometry_creations,
                        (unsigned long long)stats.state_bindings, stats.flip_model ? "flip" : "legacy" );
                    result = summary;
                }
            std::lock_guard<std::mutex> lock( m_statisticsMutex );
            m_renderStatistics = std::move( result );
        }

        bool ClawHammerSystem::getVSync() const
        {
            return m_vsync.load( std::memory_order_relaxed );
        }

        void ClawHammerSystem::load( SmartPtr<ISharedObject> data )
        {
            if( isLoaded() && m_sys )
            {
                return;
            }

            const auto start = std::chrono::steady_clock::now();
            WP_LOG_INFO( "WPGraphics: initialization started; requested API=" +
                         String( renderApiName( m_configuredRendererType ) ) );
            try
            {
                ScopedLock lock( this );
                setLoadingState( LoadingState::Loading );
                GraphicsSystem::load( data );

                if( !m_sys )
                {
                    m_sys = wp_graphics_system_create();
                }
                if( !m_sys )
                {
                    WP_LOG_ERROR( "ClawHammerSystem failed to create native graphics system." );
                    setLoadingState( LoadingState::Unloaded );
                    return;
                }
                wp_graphics_system_set_frame_func( m_sys, &ClawHammerSystem::renderFrameCallback, this );
                WP_LOG_INFO( "WPGraphics: native graphics system and frame callback ready." );

                auto factoryManager = workphone::make_ptr<FactoryManager>();
                factoryManager->load( nullptr );
                setFactoryManager( factoryManager );

                FactoryUtil::addFactory<ClawScene>( factoryManager );
                FactoryUtil::addFactory<ClawGraphicsPipeline>( factoryManager );
                FactoryUtil::addFactory<ClawWindow>( factoryManager );
                FactoryUtil::addFactory<ClawRenderTarget>( factoryManager );
                FactoryUtil::addFactory<ClawShader>( factoryManager );
                FactoryUtil::addFactory<ClawMaterial>( factoryManager );
                FactoryUtil::addFactory<ClawTexture>( factoryManager );
                FactoryUtil::addFactory<ClawTextureManager>( factoryManager );
                FactoryUtil::addFactory<ClawMaterialTechnique>( factoryManager );
                FactoryUtil::addFactory<ClawMaterialPass>( factoryManager );
                FactoryUtil::addFactory<ClawMaterialTexture>( factoryManager );
                FactoryUtil::addFactory<ui::ClawUIManager>( factoryManager );
                WP_LOG_INFO(
                    "WPGraphics: scene, pipeline, window, material, texture, shader and UI factories "
                    "registered." );

                auto textureManager = workphone::make_ptr<ClawTextureManager>();
                textureManager->load( nullptr );
                m_textureManager = textureManager;
                WP_LOG_INFO( "WPGraphics: texture manager loaded." );

                auto fontManager = workphone::make_ptr<ClawFontManager>();
                loadObject( fontManager, true );
                m_fontManager = fontManager;

                auto materialManager = workphone::make_ptr<ClawMaterialManager>();
                loadObject( materialManager, true );
                m_materialManager = materialManager;

                auto overlayMgr = workphone::make_ptr<ClawOverlayManager>();
                loadObject( overlayMgr, true );
                m_overlayMgr = overlayMgr;
                WP_LOG_INFO( "WPGraphics: font, material and overlay manager loading requested." );

                m_plugin = workphone::make_ptr<ui::WPImGui>();
                m_plugin->load( nullptr );

                auto imguiMgr = workphone::make_ptr<ClawImguiManager>();
                loadObject( imguiMgr, true );
                m_imguiManager = imguiMgr;
                WP_LOG_INFO( "WPGraphics: UI plugin loaded; ImGui manager loading requested." );

                if( auto applicationManager = core::IApplicationManager::instancePtr() )
                {
                    auto stateManager = applicationManager->getStateManager();
                    if( stateManager )
                    {
                        auto stateContext = stateManager->addStateContext();
                        stateContext->setOwner( this );
                        stateContext->setTaskId( getRenderTask() );
                        setStateContext( stateContext );

                        auto listener = workphone::make_ptr<StateListener>();
                        listener->setOwner( this );
                        stateContext->addStateListener( listener );
                        setStateListener( listener );
                    }
                    else
                    {
                        WP_LOG_WARNING(
                            "WPGraphics: no state manager; graphics state updates are unavailable." );
                    }
                }
                else
                {
                    WP_LOG_WARNING(
                        "WPGraphics: no application manager; graphics state context was not attached." );
                }

                setLoadingState( LoadingState::Loaded );
                WP_LOG_INFO( "WPGraphics: initialization completed in " + initializationTime( start ) +
                             "; renderer configuration follows." );
            }
            catch( std::exception &e )
            {
                WP_LOG_ERROR( "WPGraphics: initialization failed after " + initializationTime( start ) +
                              "; releasing partial resources." );
                WP_LOG_EXCEPTION( e );
                unload( data );
            }
        }

        void ClawHammerSystem::unload( SmartPtr<ISharedObject> data )
        {
            if( getLoadingState() == LoadingState::Unloaded && !m_sys )
            {
                return;
            }

            ScopedLock lock( this );
            setLoadingState( LoadingState::Unloading );

            // Queued objects can be owned by the graphics managers below. Release
            // their references before those managers destroy the objects.
            clearObjectQueues();
            m_reloadQueue.clear();

            if( m_sys )
            {
                wp_graphics_system_set_frame_func( m_sys, nullptr, nullptr );
            }

            if( auto debug = getDebug() )
            {
                debug->unload( nullptr );
                setDebug( nullptr );
            }

            if( auto overlayManager = getOverlayManager() )
            {
                overlayManager->unload( nullptr );
                m_overlayMgr = nullptr;
            }

            if( auto fontManager = getFontManager() )
            {
                fontManager->unload( nullptr );
                m_fontManager = nullptr;
            }

            if( auto materialManager = getMaterialManager() )
            {
                materialManager->unload( nullptr );
                m_materialManager = nullptr;
            }

            if( auto textureManager = getTextureManager() )
            {
                textureManager->unload( nullptr );
                m_textureManager = nullptr;
            }

            if( m_imguiManager )
            {
                m_imguiManager->unload( nullptr );
                m_imguiManager = nullptr;
            }

            if( m_graphicsPipeline )
            {
                m_graphicsPipeline->shutdown();
                m_graphicsPipeline = nullptr;
            }

            if( auto renderer = getRenderer() )
            {
                if( m_sys )
                {
                    wp_graphics_system_set_renderer( m_sys, nullptr );
                }

                renderer->unload( nullptr );
                m_renderer = nullptr;
            }

            removeAllGraphicsScenes();

            if( auto resourceGroupManager = getResourceGroupManager() )
            {
                resourceGroupManager->unload( nullptr );
                m_resourceGroupManager = nullptr;
            }

            if( m_sys )
            {
                wp_graphics_system_destroy( m_sys );
                m_sys = nullptr;
            }

            GraphicsSystem::unload( data );

            if( auto applicationManager = core::IApplicationManager::instancePtr() )
            {
                if( auto stateManager = applicationManager->getStateManager() )
                {
                    if( auto stateContext = getStateContext() )
                    {
                        if( auto listener = getStateListener() )
                        {
                            stateContext->removeStateListener( listener );
                            setStateListener( nullptr );
                        }

                        stateManager->removeStateContext( stateContext );
                        setStateContext( nullptr );
                    }
                }
            }

            if( m_plugin )
            {
                m_plugin->unload( nullptr );
                m_plugin = nullptr;
            }

            setLoadingState( LoadingState::Unloaded );
        }

        void ClawHammerSystem::update()
        {
            if( !isLoaded() || !m_sys )
            {
                return;
            }

            try
            {
                ScopedLock lock( this );
                static thread_local WeakPtr<IProfile> preparationProfile;
                RenderPhaseProfile preparation( preparationProfile, "Graphics preparation" );
                if( m_statisticsResetRequested.exchange( false ) )
                    if( auto renderer = dynamic_pointer_cast<ClawRendererDX11>( getRenderer() ) )
                        if( auto dx11 = wp_renderer_get_dx11( renderer->getNativeRenderer() ) )
                            wp_renderer_dx11_reset_statistics( dx11 );
                GraphicsSystem::update();

                SmartPtr<ISharedObject> object;
                while( m_reloadQueue.try_pop( object ) )
                {
                    if( object )
                    {
                        object->reload( nullptr );
                    }
                }

                updateUnloadQueue();

                for( auto &window : m_windows )
                {
                    if( window )
                    {
                        window->update();
                    }
                }

                auto applicationManager = core::IApplicationManager::instancePtr();
                auto renderUI = applicationManager->getRenderUI();

                if( renderUI )
                {
                    renderUI->update();
                }

                if( auto debug = getDebug() )
                {
                    debug->preUpdate();
                    debug->update();
                    debug->postUpdate();
                }

                wp_f32 deltaTime = 1.0f / 60.0f;

                if( auto timer = applicationManager->getTimer() )
                {
                    deltaTime = static_cast<wp_f32>( timer->getDeltaTime() );
                }

                preparation.end();
                wp_graphics_system_render_frame( m_sys, deltaTime );
                publishRenderStatistics();

                for( auto &scene : m_scenes )
                {
                    if( scene )
                    {
                        scene->postUpdate();
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void ClawHammerSystem::renderFrameCallback( wp_graphics_system *system, wp_f32 deltaTime,
                                                    void *userData )
        {
            auto owner = static_cast<ClawHammerSystem *>( userData );
            if( owner && owner->m_sys == system )
            {
                try
                {
                    owner->renderFrame( deltaTime );
                }
                catch( std::exception &e )
                {
                    // Never unwind a C++ exception through the C89 callback boundary.
                    WP_LOG_EXCEPTION( e );
                }
            }
        }

        void ClawHammerSystem::renderFrame( wp_f32 deltaTime )
        {
            (void)deltaTime;

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto renderUI = applicationManager ? applicationManager->getRenderUI() : nullptr;

            if( auto renderer = getRenderer() )
            {
                const auto renderScenePass = [this, &renderer]() {
                    static thread_local WeakPtr<IProfile> sceneProfile;
                    RenderPhaseProfile timing( sceneProfile, "Scene draw" );
                    const auto pipelineFrameStarted = beginGraphicsPipelineFrame( renderer );

                    render();

                    if( pipelineFrameStarted )
                    {
                        endGraphicsPipelineFrame( renderer );
                    }
                };

                // The Editor camera targets a render texture. Treat each active render
                // target as a compositor pass, then sample its SRV in the ImGui window pass.
                if( auto textureManager = getTextureManager() )
                {
                    for( auto &texture : textureManager->getTextures() )
                    {
                        if( !texture || !( texture->getUsageFlags() &
                                           static_cast<u32>( TextureUsage::TU_RENDERTARGET ) ) )
                        {
                            continue;
                        }

                        auto target = texture->getRenderTarget();
                        if( !target || !target->isActive() || target->getSize().x <= 0 ||
                            target->getSize().y <= 0 )
                        {
                            continue;
                        }

                        for( auto &targetViewport : target->getViewports() )
                        {
                            if( !targetViewport || !targetViewport->isActive() ||
                                !targetViewport->getEnableSceneRender() )
                            {
                                continue;
                            }

                            renderer->setRenderTarget( target );
                            renderer->setViewport( targetViewport );

                            if( auto renderer3 = dynamic_pointer_cast<IRenderer3>( renderer ) )
                            {
                                renderer3->setCamera( targetViewport->getCamera() );
                            }

                            renderer->beginRender();

                            if( targetViewport->getClearEveryFrame() )
                            {
                                renderer->clear( targetViewport->getBackgroundColour() );
                            }

                            renderScenePass();
                            if( targetViewport->getEnableUI() && renderUI && renderUI->isLoaded() )
                            {
                                renderUI->render();
                            }
                            renderer->endRender();
                        }
                    }
                }

                auto window = getDefaultWindow();
                auto viewport = window ? window->getViewport( 0 ) : nullptr;

                renderer->setRenderTarget( window );
                renderer->setViewport( viewport );

                if( auto renderer3 = dynamic_pointer_cast<IRenderer3>( renderer ) )
                {
                    renderer3->setCamera( viewport ? viewport->getCamera() : nullptr );
                }

                renderer->beginRender();
                renderer->clear( viewport ? viewport->getBackgroundColour() : ColourF::Black );
                if( !viewport || viewport->getEnableSceneRender() )
                {
                    renderScenePass();
                }

                // Submit UI after the scene pipeline so temporal and lens effects never alter it.
                if( ( !viewport || ( viewport->getEnableSceneRender() && viewport->getEnableUI() ) ) &&
                    renderUI && renderUI->isLoaded() )
                {
                    renderUI->render();
                }

                if( m_imguiManager && m_imguiManager->isInitialised() )
                {
                    m_imguiManager->newFrame();

                    if( applicationManager )
                    {
                        if( auto ui = applicationManager->getUI() )
                        {
                            if( auto application = ui->getApplication() )
                            {
                                static thread_local WeakPtr<IProfile> uiBuildProfile;
                                RenderPhaseProfile timing( uiBuildProfile, "Editor UI build" );
                                application->update();
                            }
                        }
                    }

                    static thread_local WeakPtr<IProfile> uiSubmitProfile;
                    RenderPhaseProfile timing( uiSubmitProfile, "UI submission" );
                    m_imguiManager->render();
                }

                static thread_local WeakPtr<IProfile> presentationProfile;
                RenderPhaseProfile timing( presentationProfile, "Presentation" );
                renderer->endRender();
            }
        }

        void ClawHammerSystem::render()
        {
            GraphicsSystem::render();

            if( auto renderer = getRenderer() )
            {
                auto camera = renderer->getCamera();
                auto cameraScene = camera ? camera->getCreator() : nullptr;

                for( auto &scene : m_scenes.snapshot() )
                {
                    if( cameraScene && scene != cameraScene )
                    {
                        continue;
                    }

                    if( auto clawScene = dynamic_pointer_cast<ClawScene>( scene ) )
                    {
                        clawScene->render( (void *)renderer.get() );
                    }
                }
            }
        }

        bool ClawHammerSystem::beginGraphicsPipelineFrame( SmartPtr<IRenderer> renderer )
        {
            if( !renderer || !m_graphicsPipeline )
            {
                return false;
            }

            // The C89 pipeline currently consumes and presents the software framebuffer.
            // Do not resize or advance its temporal history for unrelated render textures.
            if( renderer->getRenderTarget() != getDefaultWindow() ||
                !dynamic_pointer_cast<ClawRendererSoftware>( renderer ) )
            {
                return false;
            }

            auto camera = renderer->getCamera();
            if( !camera )
            {
                return false;
            }

            auto target = renderer->getRenderTarget();
            auto viewport = renderer->getViewport();
            auto size = viewport ? viewport->getActualSize()
                                 : Vector2<real_Num>(
                                       static_cast<real_Num>( target ? target->getSize().x : 0 ),
                                       static_cast<real_Num>( target ? target->getSize().y : 0 ) );
            const auto width = static_cast<s32>( size.x );
            const auto height = static_cast<s32>( size.y );
            if( width <= 0 || height <= 0 )
            {
                return false;
            }

            if( !m_graphicsPipeline->isInitialized() || m_graphicsPipeline->getWidth() != width ||
                m_graphicsPipeline->getHeight() != height )
            {
                if( !m_graphicsPipeline->initialize( width, height ) )
                {
                    WP_LOG_ERROR( "ClawHammerSystem failed to resize its graphics pipeline." );
                    return false;
                }
            }

            auto cameraScene = camera->getCreator();
            m_graphicsPipeline->setCamera( camera.get() );
            m_graphicsPipeline->setScene( cameraScene.get() );
            m_graphicsPipeline->beginFrame();
            return true;
        }

        void ClawHammerSystem::endGraphicsPipelineFrame( SmartPtr<IRenderer> renderer )
        {
            if( !renderer || !m_graphicsPipeline || !m_graphicsPipeline->isInitialized() )
            {
                return;
            }

            auto clawPipeline = dynamic_pointer_cast<ClawGraphicsPipeline>( m_graphicsPipeline );
            wp_renderer *softwareNativeRenderer = nullptr;
            bool capturedSoftwareFrame = false;

            // The CPU pipeline can currently exchange framebuffer data only with the software
            // backend. Render textures and GPU backends keep their native presentation path.
            if( clawPipeline && renderer->getRenderTarget() == getDefaultWindow() )
            {
                if( auto softwareRenderer = dynamic_pointer_cast<ClawRendererSoftware>( renderer ) )
                {
                    softwareNativeRenderer = softwareRenderer->getNativeRenderer();
                    capturedSoftwareFrame = clawPipeline->captureSoftwareFrame( softwareNativeRenderer );
                }
            }

            m_graphicsPipeline->render();

            if( capturedSoftwareFrame )
            {
                clawPipeline->presentSoftwareFrame( softwareNativeRenderer );
            }

            m_graphicsPipeline->endFrame();
        }

        void ClawHammerSystem::messagePump()
        {
            for( auto &graphicsWindow : m_windows )
            {
                if( auto window = dynamic_pointer_cast<ClawWindow>( graphicsWindow ) )
                {
                    if( !window->messagePump() )
                    {
                        if( auto applicationManager = core::IApplicationManager::instancePtr() )
                        {
                            applicationManager->setQuit( true );
                        }
                    }
                }
            }
        }

        bool ClawHammerSystem::configure( SmartPtr<IBuildDirector> data )
        {
            const auto start = std::chrono::steady_clock::now();
            try
            {
                // The native ClawHammer system is created during load(). If that failed
                // there is nothing to configure.
                if( !m_sys )
                {
                    WP_LOG_ERROR( "ClawHammerSystem::configure: native graphics system not created." );
                    return false;
                }

                auto config = workphone::dynamic_pointer_cast<GraphicsSettings>( data );

                bool createWindow = true;
                if( config )
                {
                    createWindow = config->getCreateWindow();
                    m_vsync.store( config->getVSync(), std::memory_order_relaxed );
                }
                WP_LOG_INFO( "WPGraphics: configuration started; requested API=" +
                             String( renderApiName( m_configuredRendererType ) ) +
                             ", create window=" + String( createWindow ? "yes" : "no" ) );

                if( createWindow && !getDefaultWindow() )
                {
                    auto window = createRenderWindow( "DefaultWindow", 1280, 720, false, nullptr );
                    if( !window )
                    {
                        WP_LOG_ERROR(
                            "WPGraphics: configuration failed creating the default 1280x720 window." );
                        return false;
                    }
                }

                if( !getRenderer() )
                {
                    // Use configured renderer type, or auto-detect if not set
                    SmartPtr<IRenderer> renderer;
                    RenderApi selectedApi = m_configuredRendererType;

                    // Auto-detect if no specific renderer was configured
                    if( selectedApi == RenderApi::None )
                    {
#if defined WP_PLATFORM_WIN32 && WP_BUILD_RENDERER_DX11
                        if( createWindow && getDefaultWindow() )
                        {
                            selectedApi = RenderApi::DX11;
                        }
                        else
                        {
                            selectedApi = RenderApi::None;
                        }
#else
                        selectedApi = RenderApi::None;
#endif
                    }

                    // Create the selected renderer
                    switch( selectedApi )
                    {
#if defined WP_PLATFORM_WIN32 && WP_BUILD_RENDERER_DX12
                    case RenderApi::DX12:
                    {
                        renderer = workphone::make_ptr<ClawRendererDX12>();
                        break;
                    }
#endif
#if defined WP_PLATFORM_WIN32 && WP_BUILD_RENDERER_DX11
                    case RenderApi::DX11:
                    {
                        renderer = workphone::make_ptr<ClawRendererDX11>();
                        break;
                    }
#endif
                    case RenderApi::None:
                    case RenderApi::Software:
                    {
                        selectedApi = RenderApi::Software;
                        renderer = workphone::make_ptr<ClawRendererSoftware>();
                        break;
                    }
                    default:
                    {
                        WP_LOG_WARNING( "WPGraphics: requested API " +
                                        String( renderApiName( selectedApi ) ) +
                                        " is unavailable in this build; using Software." );
                        selectedApi = RenderApi::Software;
                        renderer = workphone::make_ptr<ClawRendererSoftware>();
                        break;
                    }
                    }

                    m_renderApi = selectedApi;
                    WP_LOG_INFO( "WPGraphics: creating " + String( renderApiName( selectedApi ) ) +
                                 " renderer." );
                    renderer->load( getDefaultWindow() );
                    if( !renderer->isLoaded() )
                    {
                        WP_LOG_ERROR( "WPGraphics: configuration failed loading " +
                                      String( renderApiName( selectedApi ) ) + " renderer." );
                        return false;
                    }

                    m_renderer = renderer;
                    wp_renderer *nativeRenderer = nullptr;
                    if( auto dx12Renderer = dynamic_pointer_cast<ClawRendererDX12>( renderer ) )
                    {
                        // ClawRendererDX12 has no wp_renderer bridge; the UI manager will
                        // receive nullptr here and bail in submit() / newFrame().
                        nativeRenderer = dx12Renderer->getNativeRenderer();
                    }
                    else if( auto dx11Renderer = dynamic_pointer_cast<ClawRendererDX11>( renderer ) )
                    {
                        nativeRenderer = dx11Renderer->getNativeRenderer();
                    }
                    else if( auto softwareRenderer =
                                 dynamic_pointer_cast<ClawRendererSoftware>( renderer ) )
                    {
                        nativeRenderer = softwareRenderer->getNativeRenderer();
                    }
                    wp_graphics_system_set_renderer( m_sys, nativeRenderer );
                    if( !nativeRenderer )
                        WP_LOG_WARNING(
                            "WPGraphics: selected renderer has no native bridge; native scene "
                            "submission and UI are unavailable." );
                    WP_LOG_INFO( "WPGraphics: renderer loaded; active API=" +
                                 String( renderApiName( m_renderApi ) ) );
                }

                if( m_imguiManager && !m_imguiManager->isInitialised() )
                {
                    wp_renderer *nativeRenderer = nullptr;
                    if( auto dx12Renderer = dynamic_pointer_cast<ClawRendererDX12>( getRenderer() ) )
                    {
                        // ClawRendererDX12 has no wp_renderer bridge; the UI manager will
                        // receive nullptr here and bail in submit() / newFrame().
                        nativeRenderer = dx12Renderer->getNativeRenderer();
                    }
                    else if( auto dx11Renderer =
                                 dynamic_pointer_cast<ClawRendererDX11>( getRenderer() ) )
                    {
                        nativeRenderer = dx11Renderer->getNativeRenderer();
                    }
                    else if( auto softwareRenderer =
                                 dynamic_pointer_cast<ClawRendererSoftware>( getRenderer() ) )
                    {
                        nativeRenderer = softwareRenderer->getNativeRenderer();
                    }

                    if( nativeRenderer )
                    {
                        m_imguiManager->setRenderer( nativeRenderer );
                    }
                }

                if( !getResourceGroupManager() )
                {
                    m_resourceGroupManager = workphone::make_ptr<ClawResourceGroupManager>();
                    m_resourceGroupManager->load( nullptr );
                    WP_LOG_INFO( "WPGraphics: resource group manager loaded." );
                }

                if( !m_graphicsPipeline )
                {
                    auto pipeline = workphone::make_ptr<ClawGraphicsPipeline>();
                    auto target = getDefaultWindow();
                    const auto size = target ? target->getSize() : Vector2I( 1280, 720 );
                    if( !pipeline->initialize( std::max<s32>( size.x, 1 ), std::max<s32>( size.y, 1 ) ) )
                    {
                        WP_LOG_ERROR(
                            "ClawHammerSystem::configure: failed to create graphics pipeline." );
                        return false;
                    }
                    m_graphicsPipeline = pipeline;
                }

                // Provide a debug helper so application createScene() implementations can
                // queue debug primitives/text without dereferencing a null interface.
                if( !getDebug() )
                {
                    auto debug = workphone::make_ptr<ClawDebug>();
                    debug->load( nullptr );
                    setDebug( debug );
                }

                WP_LOG_INFO( "WPGraphics: configuration completed in " + initializationTime( start ) +
                             "; active API=" + String( renderApiName( m_renderApi ) ) +
                             "; CPU pipeline applies to the software main window." );
                return true;
            }
            catch( std::exception &e )
            {
                WP_LOG_ERROR( "WPGraphics: configuration failed after " + initializationTime( start ) +
                              "; requested API=" + String( renderApiName( m_configuredRendererType ) ) );
                WP_LOG_EXCEPTION( e );
            }

            return false;
        }

        SmartPtr<IGraphicsWindow> ClawHammerSystem::createRenderWindow(
            const String &name, u32 width, u32 height, bool fullScreen,
            const SmartPtr<Properties> &properties )
        {
            if( auto existing = getRenderWindow( name ) )
            {
                return existing;
            }
            WP_LOG_INFO( "WPGraphics: creating window '" + name + "', " + std::to_string( width ) + "x" +
                         std::to_string( height ) +
                         ", fullscreen=" + String( fullScreen ? "yes" : "no" ) );
            if( width == 0 || height == 0 )
                WP_LOG_WARNING( "WPGraphics: window '" + name +
                                "' has zero dimensions; clamping each dimension to at least 1 pixel." );

            auto window = workphone::make_ptr<ClawWindow>();
            window->setName( name );
            window->setTitle( name == "DefaultWindow" ? String( "Workphone" ) : name );
            window->setSize( Vector2I( static_cast<s32>( std::max<u32>( width, 1u ) ),
                                       static_cast<s32>( std::max<u32>( height, 1u ) ) ) );
            window->setFullscreen( fullScreen );
            if( properties )
            {
                window->setProperties( properties );
            }
            window->load( nullptr );

            if( !window->isLoaded() )
            {
                WP_LOG_ERROR( "WPGraphics: window '" + name + "' failed to load." );
                return nullptr;
            }

            m_windows.push_back( window );
            if( !getDefaultWindow() )
            {
                setDefaultWindow( window );
            }

#if defined WP_PLATFORM_WIN32 && ( WP_BUILD_RENDERER_DX11 || WP_BUILD_RENDERER_DX12 )
            // Some hosts (notably Editor) configure the graphics system before
            // creating their render window. That initial headless configure uses
            // the software backend, which has no way to present into the later
            // Win32 window. Promote it to the configured hardware renderer as soon
            // as a native window exists.
            if( auto softwareRenderer = dynamic_pointer_cast<ClawRendererSoftware>( getRenderer() );
                softwareRenderer && m_configuredRendererType != RenderApi::Software )
            {
                SmartPtr<IRenderer> newRenderer;
                RenderApi targetApi = m_configuredRendererType != RenderApi::None
                                          ? m_configuredRendererType
                                          : RenderApi::DX11;

                // Prefer DX12 if configured and available, otherwise DX11
#    if defined WP_BUILD_RENDERER_DX12
                if( targetApi == RenderApi::DX12 )
                {
                    newRenderer = workphone::make_ptr<ClawRendererDX12>();
                }
                else
#    endif
#    if defined WP_BUILD_RENDERER_DX11
                {
                    newRenderer = workphone::make_ptr<ClawRendererDX11>();
                }
#    endif

                if( newRenderer )
                {
                    newRenderer->load( window );

                    if( newRenderer->isLoaded() )
                    {
                        wp_renderer *nativeRenderer = nullptr;
#    if defined WP_BUILD_RENDERER_DX12
                        if( auto dx12Renderer = dynamic_pointer_cast<ClawRendererDX12>( newRenderer ) )
                        {
                            nativeRenderer = dx12Renderer->getNativeRenderer();
                            m_renderApi = RenderApi::DX12;
                        }
                        else
#    endif
#    if defined WP_BUILD_RENDERER_DX11
                            if( auto dx11Renderer =
                                    dynamic_pointer_cast<ClawRendererDX11>( newRenderer ) )
                        {
                            nativeRenderer = dx11Renderer->getNativeRenderer();
                            m_renderApi = RenderApi::DX11;
                        }
#    endif

                        if( nativeRenderer )
                        {
                            wp_graphics_system_set_renderer( m_sys, nativeRenderer );
                            m_renderer = newRenderer;

                            if( m_imguiManager )
                            {
                                m_imguiManager->setRenderer( nativeRenderer );
                            }

                            softwareRenderer->unload( nullptr );
                            WP_LOG_INFO( "WPGraphics: window '" + name +
                                         "' promoted from headless Software to " +
                                         String( renderApiName( m_renderApi ) ) + "." );
                        }
                    }
                    else
                    {
                        WP_LOG_ERROR(
                            "ClawHammerSystem::createRenderWindow: failed to promote "
                            "renderer to hardware." );
                    }
                }
            }
#endif

            WP_LOG_INFO( "WPGraphics: window '" + name +
                         "' ready; active API=" + String( renderApiName( m_renderApi ) ) );
            return window;
        }

        SmartPtr<IGraphicsScene> ClawHammerSystem::addGraphicsScene( const String &type,
                                                                     const String &name )
        {
            if( StringUtil::isNullOrEmpty( type ) || StringUtil::isNullOrEmpty( name ) || !m_sys )
            {
                WP_LOG_ERROR( "ClawHammerSystem::addGraphicsScene: invalid scene request." );
                return nullptr;
            }

            if( auto existing = getGraphicsScene( name ) )
            {
                return existing;
            }

            auto nativeScene = wp_graphics_system_create_scene( m_sys );
            if( !nativeScene )
            {
                WP_LOG_ERROR( "ClawHammerSystem::addGraphicsScene: native scene creation failed." );
                return nullptr;
            }

            auto scene = workphone::make_ptr<ClawScene>( nativeScene, false );
            scene->setType( type );
            scene->setName( name );
            m_scenes.push_back( scene );

            if( !getGraphicsScene() )
            {
                setGraphicsScene( scene );
            }

            scene->load( nullptr );
            if( !scene->isLoaded() )
            {
                m_scenes.erase( std::remove( m_scenes.begin(), m_scenes.end(), scene ), m_scenes.end() );
                scene->releaseNativeScene();
                wp_graphics_system_destroy_scene( m_sys, nativeScene );
                return nullptr;
            }

            return scene;
        }

        void ClawHammerSystem::removeGraphicsScene( SmartPtr<IGraphicsScene> scene )
        {
            if( !scene )
            {
                return;
            }

            wp_graphics_scene *nativeScene = nullptr;
            if( auto clawScene = dynamic_pointer_cast<ClawScene>( scene ) )
            {
                nativeScene = clawScene->releaseNativeScene();
            }

            scene->unload( nullptr );
            m_scenes.erase( std::remove( m_scenes.begin(), m_scenes.end(), scene ), m_scenes.end() );
            if( getGraphicsScene() == scene )
            {
                auto scenes = m_scenes.snapshot();
                setGraphicsScene( scenes.empty() ? nullptr : scenes.front() );
            }

            if( m_sys && nativeScene )
            {
                wp_graphics_system_destroy_scene( m_sys, nativeScene );
            }
        }

        void ClawHammerSystem::removeAllGraphicsScenes()
        {
            auto scenes = m_scenes.snapshot();
            m_scenes.clear();
            setGraphicsScene( nullptr );

            for( auto &scene : scenes )
            {
                wp_graphics_scene *nativeScene = nullptr;
                if( auto clawScene = dynamic_pointer_cast<ClawScene>( scene ) )
                {
                    nativeScene = clawScene->releaseNativeScene();
                }

                if( scene )
                {
                    scene->unload( nullptr );
                }
                if( m_sys && nativeScene )
                {
                    wp_graphics_system_destroy_scene( m_sys, nativeScene );
                }
            }
        }

        void ClawHammerSystem::setupRenderer( SmartPtr<IGraphicsScene> sceneManager,
                                              SmartPtr<IGraphicsWindow> window,
                                              SmartPtr<IGraphicsCamera> camera, String workspaceName,
                                              bool enabled )
        {
            (void)workspaceName;
            if( sceneManager )
            {
                setGraphicsScene( sceneManager );
            }
            if( window )
            {
                setDefaultWindow( window );
            }

            auto viewport = window && camera ? window->getViewport( 0 ) : nullptr;
            if( window && camera && !viewport )
            {
                viewport = window->addViewport( 0, camera );
            }
            if( viewport )
            {
                viewport->setActive( enabled );
            }

            if( auto renderer = getRenderer() )
            {
                renderer->setRenderTarget( window );
                renderer->setViewport( viewport );
                if( auto renderer3 = dynamic_pointer_cast<IRenderer3>( renderer ) )
                {
                    renderer3->setCamera( camera );
                }
            }
        }

        wp_graphics_system *ClawHammerSystem::getNativeSystem() const
        {
            return m_sys;
        }

        SmartPtr<IGraphicsPipeline> ClawHammerSystem::getGraphicsPipeline() const
        {
            return m_graphicsPipeline;
        }

        bool ClawHammerSystem::handleStateChanged( SmartPtr<IState> &state )
        {
            for( auto &scene : m_scenes )
            {
                if( scene && scene->handleStateChanged( state ) )
                {
                    return true;
                }
            }

            for( auto &window : m_windows )
            {
                if( window && window->handleStateChanged( state ) )
                {
                    return true;
                }
            }

            return getTextureManagerPtr() && getTextureManagerPtr()->handleStateChanged( state );
        }

        bool ClawHammerSystem::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            for( auto &scene : m_scenes )
            {
                if( scene && scene->handleStateMessage( message ) )
                {
                    return true;
                }
            }
            return false;
        }

        ClawHammerSystem::StateListener::StateListener() : m_owner( nullptr )
        {
        }

        ClawHammerSystem::StateListener::~StateListener() = default;

        bool ClawHammerSystem::StateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            SmartPtr<ClawHammerSystem> owner = m_owner.load().lock();
            if( owner )
            {
                return owner->handleStateChanged( state );
            }

            return false;
        }

        bool ClawHammerSystem::StateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            SmartPtr<ClawHammerSystem> owner = m_owner.load().lock();
            if( owner )
            {
                return owner->handleStateMessage( message );
            }

            return false;
        }

        SmartPtr<ClawHammerSystem> ClawHammerSystem::StateListener::getOwner() const
        {
            return m_owner.load().lock();
        }

        void ClawHammerSystem::StateListener::setOwner( SmartPtr<ClawHammerSystem> owner )
        {
            m_owner = owner;
        }

        void ClawHammerSystem::setRendererType( RenderApi api )
        {
            m_configuredRendererType = api;
        }

        IGraphicsSystem::RenderApi ClawHammerSystem::getRendererType() const
        {
            return m_configuredRendererType;
        }

        ClawCapabilities ClawHammerSystem::getCapabilities() const
        {
            return getClawCapabilities( m_renderApi );
        }

        bool ClawHammerSystem::switchRenderer( RenderApi api )
        {
            if( !m_sys )
            {
                WP_LOG_ERROR(
                    "ClawHammerSystem::switchRenderer: native graphics system not available." );
                return false;
            }

            if( m_renderApi == api )
            {
                // Already using the requested renderer
                return true;
            }

            WP_LOG_WARNING(
                "ClawHammerSystem::switchRenderer: Runtime renderer switching is experimental. "
                "Some resources may need to be recreated." );

            // Store current state
            auto currentWindow = getDefaultWindow();
            auto currentScene = getGraphicsScene();

            // Note: Full runtime switching would require:
            // 1. Unloading current renderer
            // 2. Creating new renderer
            // 3. Recreating all renderer-specific resources
            // 4. Rebinding to native system
            //
            // For now, we set the configured type for next initialization
            m_configuredRendererType = api;

            WP_LOG_INFO( "ClawHammerSystem::switchRenderer: Renderer type set to " +
                         std::to_string( static_cast<int>( api ) ) +
                         ". Restart required for full switch." );

            return true;
        }

    }  // namespace render
}  // namespace workphone
