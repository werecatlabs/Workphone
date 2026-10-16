#include "WPGraphics/WPClawHammerPCH.hpp"
#include <stdexcept>
#include "workphone_graphics_cull_job.h"
#include <WPGraphics/ClawDebug.hpp>
#include <WPGraphics/ClawFontManager.hpp>
#include <WPGraphics/ClawGraphicsPipeline.hpp>
#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawImguiManager.hpp>
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawMaterialManager.hpp>
#include <WPGraphics/ClawMaterialPass.hpp>
#include <WPGraphics/ClawMaterialTechnique.hpp>
#include <WPGraphics/ClawMaterialTexture.hpp>
#include <WPGraphics/ClawOverlayManager.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <WPGraphics/ClawRendererDX12.hpp>
#include <WPGraphics/ClawRendererSoftware.hpp>
#include <WPGraphics/ClawResourceGroupManager.hpp>
#include <WPGraphics/ClawScene.hpp>
#include <WPGraphics/ClawShader.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/ClawTextureManager.hpp>
#include <WPGraphics/ClawWindow.hpp>
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <WPGraphics/Vehicle/ClawVehicleVisualEffects.hpp>
#include <WPImGui/WPImGui.hpp>
#include <Workphone/Workphone.hpp>
#include <chrono>
#include <cstdio>

#include "workphone_graphics_renderer_dx11.h"
#include "workphone_graphics_renderer.h"
#include "workphone_graphics_system.h"

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
                RenderPhaseProfile( SmartPtr<IProfile> &cached, const char *label )
                {
                    m_profile = cached;
                    if( !m_profile )
                    {
                        if( auto app = core::IApplicationManager::instancePtr() )
                        {
                            if( auto profiler = app->getProfiler() )
                            {
                                m_profile = profiler->addProfile();
                                m_profile->setLabel( label );
                                cached = m_profile;
                            }
                        }
                    }
                    if( m_profile )
                    {
                        m_profile->start();
                    }
                }

                ~RenderPhaseProfile()
                {
                    end();
                }

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

        struct ClawHammerSystem::FramePassContext
        {
            ClawHammerSystem *owner;
            SmartPtr<IRenderer> renderer;
            SmartPtr<IRenderTarget> target;
            SmartPtr<IViewport> viewport;
            bool window;
            bool failed = false;
            bool begun = false;
            bool pipelineStarted = false;
            bool nativeRtt = false;
            bool rttInside = false;
            bool rttCompleted = false;
            bool rttCaptured = false;
            SmartPtr<IRenderTarget> previousTarget;
            SmartPtr<IViewport> previousViewport;
            SmartPtr<IGraphicsCamera> previousCamera;
            Array<SmartPtr<ClawScene>> preparedScenes;
        };

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
                props->setProperty( "native_update_serial", m_nativeUpdateSerial );
                props->setProperty( "native_render_serial", m_nativeRenderSerial );
                props->setProperty( "native_completed_passes", m_nativeCompletedPasses );
                props->setProperty( "native_skipped_passes", m_nativeSkippedPasses );
                const auto split = m_renderStatistics.find( '\n' );
                props->setProperty( "render_statistics", m_renderStatistics.substr( 0, split ) );
                if( split != String::npos )
                {
                    props->setProperty( "render_counters", m_renderStatistics.substr( split + 1 ) );
                }
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
                {
                    m_statisticsSnapshotRequested = true;
                }
                bool enabled;
                if( properties->getPropertyValue( "vsync", enabled ) )
                {
                    m_vsync.store( enabled, std::memory_order_relaxed );
                }
            }
        }

        void ClawHammerSystem::publishRenderStatistics()
        {
            // Native stats belong to the render task. Publish a cached snapshot for
            // property readers on the editor task instead of racing the C system.
            wp_graphics_system_frame_stats frameStats{};
            wp_graphics_system_get_frame_stats( m_sys, &frameStats );
            {
                std::lock_guard<std::mutex> lock( m_statisticsMutex );
                m_nativeUpdateSerial = frameStats.update_serial;
                m_nativeRenderSerial = frameStats.render_serial;
                m_nativeCompletedPasses = frameStats.completed_pass_count;
                m_nativeSkippedPasses = frameStats.skipped_pass_count;
            }
            if( !m_statisticsSnapshotRequested.exchange( false ) )
            {
                return;
            }
            String result = "Render statistics unavailable";
            if( auto renderer = dynamic_pointer_cast<ClawRendererDX11>( getRenderer() ) )
            {
                if( auto dx11 = wp_renderer_get_dx11( renderer->getNativeRenderer() ) )
                {
                    wp_render_statistics_dx11 stats{};
                    wp_renderer_dx11_get_statistics( dx11, &stats );
                    char summary[1024];
                    std::snprintf(
                        summary, sizeof( summary ),
                        "Render frames=%llu interval mean=%.3f ms p95=%.3f ms render-pass CPU=%.3f ms "
                        "Present=%.3f ms GPU=%.3f ms GPU samples=%llu\nDraws=%llu triangles=%llu "
                        "material uploads=%llu transform uploads=%llu geometry creations=%llu state "
                        "bindings=%llu presentation=%s",
                        (unsigned long long)stats.frames, stats.interval_ms, stats.interval_p95_ms,
                        stats.cpu_frame_ms, stats.present_ms, stats.gpu_frame_ms,
                        (unsigned long long)stats.gpu_samples, (unsigned long long)stats.draws,
                        (unsigned long long)stats.triangles, (unsigned long long)stats.material_uploads,
                        (unsigned long long)stats.transform_uploads,
                        (unsigned long long)stats.geometry_creations,
                        (unsigned long long)stats.state_bindings, stats.flip_model ? "flip" : "legacy" );
                    result = summary;
                }
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
                WP_LOG_INFO( "WPGraphics: native graphics system and pass driver ready." );

                auto factoryManager = workphone::make_ptr<FactoryManager>();
                factoryManager->load( nullptr );
                setFactoryManager( factoryManager );

                FactoryUtil::addFactory<ClawScene>( factoryManager );
                FactoryUtil::addFactory<advanced::ClawVehicleVisualEffects>( factoryManager );
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

            m_preparationProfile = nullptr;
            m_sceneProfile = nullptr;
            m_uiBuildProfile = nullptr;
            m_uiSubmitProfile = nullptr;
            m_presentationProfile = nullptr;

            // Queued objects can be owned by the graphics managers below. Release
            // their references before those managers destroy the objects.
            clearObjectQueues();
            m_reloadQueue.clear();

            if( m_sys )
            {
                wp_graphics_system_set_render_passes( m_sys, nullptr, 0 );
                m_framePasses.clear();
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
                RenderPhaseProfile preparation( m_preparationProfile, "Graphics preparation" );
                if( m_statisticsResetRequested.exchange( false ) )
                {
                    if( auto renderer = dynamic_pointer_cast<ClawRendererDX11>( getRenderer() ) )
                    {
                        if( auto dx11 = wp_renderer_get_dx11( renderer->getNativeRenderer() ) )
                        {
                            wp_renderer_dx11_reset_statistics( dx11 );
                        }
                    }
                }
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
                auto renderUI = applicationManager ? applicationManager->getRenderUI() : nullptr;

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

                if( auto timer = applicationManager ? applicationManager->getTimer() : nullptr )
                {
                    deltaTime = static_cast<wp_f32>( timer->getDeltaTime() );
                }

                prepareFramePasses( deltaTime );
                if( !wp_graphics_system_set_delta_time( m_sys, deltaTime ) )
                {
                    throw std::runtime_error( "Native graphics frame is already executing." );
                }
                preparation.end();
                // The C89 system owns native update and the complete ordered frame.
                // Keep the engine's update-only scheduling contract at this boundary.
                wp_graphics_system_update( m_sys );
                wp_graphics_system_render( m_sys );
                publishRenderStatistics();

                for( auto &scene : m_scenes.snapshot() )
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

        void ClawHammerSystem::prepareFramePasses( wp_f32 deltaTime )
        {
            // Publish descriptors and their retained objects together. If allocation or
            // validation fails, the previous native descriptors still have live contexts.
            std::vector<FramePassContext> nextFramePasses;
            auto renderer = getRenderer();
            if( !renderer )
            {
                if( !wp_graphics_system_set_render_passes( m_sys, nullptr, 0 ) )
                    throw std::runtime_error( "Cannot clear an executing native graphics frame." );
                m_framePasses.clear();
                return;
            }
            if( auto pipeline = dynamic_pointer_cast<ClawGraphicsPipeline>( m_graphicsPipeline ) )
                pipeline->setFrameDeltaTime( deltaTime );
            if( auto textures = getTextureManager() )
            {
                for( auto &texture : textures->getTextures() )
                {
                    if( !texture || !( texture->getUsageFlags() &
                                       static_cast<u32>( TextureUsage::TU_RENDERTARGET ) ) ) continue;
                    auto target = texture->getRenderTarget();
                    if( !target || !target->isActive() || target->getSize().x <= 0 ||
                        target->getSize().y <= 0 ) continue;
                    for( auto &viewport : target->getViewports() )
                    {
                        if( viewport && viewport->isActive() && viewport->getEnableSceneRender() )
                            nextFramePasses.push_back( { this, renderer, target, viewport, false } );
                    }
                }
            }
            auto window = getDefaultWindow();
            nextFramePasses.push_back( { this, renderer, window,
                                      window ? window->getViewport( 0 ) : nullptr, true } );
            if( nextFramePasses.size() > WP_GRAPHICS_SYSTEM_MAX_RENDER_PASSES )
                throw std::runtime_error( "Graphics frame exceeds native render pass capacity." );
            std::vector<wp_graphics_system_render_pass> passes;
            passes.reserve( nextFramePasses.size() );
            for( auto &context : nextFramePasses )
            {
                wp_graphics_system_render_pass pass = {};
                pass.begin = &ClawHammerSystem::beginFramePass;
                pass.draw = &ClawHammerSystem::drawFramePass;
                pass.effects = &ClawHammerSystem::effectsFramePass;
                pass.ui = &ClawHammerSystem::uiFramePass;
                pass.overlay = &ClawHammerSystem::overlayFramePass;
                pass.end = &ClawHammerSystem::endFramePass;
                pass.user_data = &context;
                passes.push_back( pass );
            }
            if( !wp_graphics_system_set_render_passes( m_sys, passes.data(),
                                                       static_cast<wp_s32>( passes.size() ) ) )
                throw std::runtime_error( "Native graphics frame pass registration failed." );
            m_framePasses.swap( nextFramePasses );
        }

        wp_s32 ClawHammerSystem::beginFramePass( wp_graphics_system *system, wp_f32 dt, void *data )
        {
            auto &pass = *static_cast<FramePassContext *>( data );
            if( pass.owner->m_sys != system ) return 0;
            pass.failed = false;
            pass.begun = false;
            pass.pipelineStarted = false;
            pass.nativeRtt = pass.rttInside = pass.rttCompleted = pass.rttCaptured = false;
            executeFramePassStage( system, dt, data, 0 );
            // Restore offscreen adapter state even when binding/preparation failed
            // before the native frame driver entered this pass.
            if( pass.failed && !pass.begun )
            {
                executeFramePassStage( system, dt, data, 5 );
            }
            return pass.begun ? 1 : 0;
        }

        void ClawHammerSystem::drawFramePass( wp_graphics_system *s, wp_f32 dt, void *d )
        { executeFramePassStage( s, dt, d, 1 ); }
        void ClawHammerSystem::effectsFramePass( wp_graphics_system *s, wp_f32 dt, void *d )
        { executeFramePassStage( s, dt, d, 2 ); }
        void ClawHammerSystem::uiFramePass( wp_graphics_system *s, wp_f32 dt, void *d )
        { executeFramePassStage( s, dt, d, 3 ); }
        void ClawHammerSystem::overlayFramePass( wp_graphics_system *s, wp_f32 dt, void *d )
        { executeFramePassStage( s, dt, d, 4 ); }
        void ClawHammerSystem::endFramePass( wp_graphics_system *s, wp_f32 dt, void *d )
        { executeFramePassStage( s, dt, d, 5 ); }

        void ClawHammerSystem::executeFramePassStage( wp_graphics_system *system, wp_f32,
                                                      void *data, unsigned stage )
        {
            auto &pass = *static_cast<FramePassContext *>( data );
            auto owner = pass.owner;
            if( owner->m_sys != system || ( pass.failed && stage != 5 ) ) return;
            if( pass.nativeRtt && pass.rttCompleted && stage >= 2 && stage <= 4 ) return;
            auto viewport = pass.viewport;
            auto renderer = pass.renderer;
            try
            {
                const bool sceneEnabled = !viewport || viewport->getEnableSceneRender();
                switch( stage )
                {
                case 0:
                    // Skip detached, inactive and minimized targets before entering
                    // begin/end/present. Keep virtual calls behind this exception barrier.
                    if( !renderer || !pass.target ||
                        ( !pass.window && !pass.target->isActive() ) ||
                        pass.target->getSize().x <= 0 || pass.target->getSize().y <= 0 ||
                        ( viewport && !viewport->isActive() ) ) return;
                    pass.nativeRtt = !pass.window &&
                        dynamic_pointer_cast<ClawRendererDX11>( renderer ) &&
                        dynamic_pointer_cast<ClawRenderTarget>( pass.target );
                    if( pass.nativeRtt )
                    {
                        pass.previousTarget = renderer->getRenderTarget();
                        pass.previousViewport = renderer->getViewport();
                        pass.previousCamera = renderer->getCamera();
                        pass.rttCaptured = true;
                    }
                    if( sceneEnabled )
                    {
                        auto camera = viewport ? viewport->getCamera() : nullptr;
                        // Resolve the pass aspect before capture, without binding its target.
                        if(camera && viewport)
                        {
                            const auto size = viewport->getActualSize();
                            if(size.X() > 0 && size.Y() > 0) camera->setAspectRatio(size.X() / size.Y());
                        }
                        auto cameraScene = camera ? camera->getCreator() : nullptr;
                        for( auto &scene : owner->m_scenes.snapshot() )
                        {
                            if( cameraScene && scene != cameraScene )
                            {
                                continue;
                            }
                            if( auto clawScene = dynamic_pointer_cast<ClawScene>( scene ) )
                            {
                                pass.preparedScenes.push_back( clawScene );
                                if( !clawScene->prepareRenderJobs( camera ) )
                                {
                                    throw std::runtime_error(
                                        "Native transform/cull preparation failed." );
                                }
                            }
                        }
                    }
                    renderer->setRenderTarget( pass.target );
                    renderer->setViewport( viewport );
                    if( auto renderer3 = dynamic_pointer_cast<IRenderer3>( renderer ) )
                        renderer3->setCamera( viewport ? viewport->getCamera() : nullptr );
                    pass.begun = true;
                    renderer->beginRender();
                    if( !pass.nativeRtt )
                    {
                        if( pass.window || ( viewport && viewport->getClearEveryFrame() ) )
                            renderer->clear( viewport ? viewport->getBackgroundColour() : ColourF::Black );
                        if( sceneEnabled )
                            pass.pipelineStarted = owner->beginGraphicsPipelineFrame( renderer );
                    }
                    break;
                case 1:
                    if( pass.nativeRtt && !pass.rttInside )
                    {
                        auto dx11Renderer = dynamic_pointer_cast<ClawRendererDX11>( renderer );
                        auto target = dynamic_pointer_cast<ClawRenderTarget>( pass.target );
                        auto native = wp_renderer_get_dx11( dx11Renderer->getNativeRenderer() );
                        auto output = target->getNativeRenderTexture( dx11Renderer->getNativeRenderer() );
                        const auto size = target->getSize();
                        const auto colour = viewport ? viewport->getBackgroundColour() : ColourF::Black;
                        wp_graphics_rtt_desc desc{};
                        desc.renderer_generation = wp_renderer_dx11_get_rtt_generation( native );
                        desc.target_generation = wp_renderer_dx11_get_render_texture_generation( output );
                        desc.width = size.x; desc.height = size.y;
                        desc.viewport = wp_renderer_dx11_get_viewport( native );
                        desc.clear_colour = desc.clear_depth = viewport && viewport->getClearEveryFrame();
                        desc.colour[0] = colour.r; desc.colour[1] = colour.g;
                        desc.colour[2] = colour.b; desc.colour[3] = colour.a; desc.depth = 1.0f;
                        const auto validate = []( void *data, const wp_graphics_rtt_desc *expected ) -> wp_rtt_result {
                            try
                            {
                                auto &p = *static_cast<FramePassContext *>( data );
                                const auto actual = p.target->getSize();
                                for( auto &scene : p.preparedScenes )
                                {
                                    if( !scene->validateRenderJobs() )
                                    {
                                        return WP_RTT_STALE;
                                    }
                                }
                                return !p.failed && actual.x == expected->width && actual.y == expected->height
                                    ? WP_RTT_OK : WP_RTT_STALE;
                            }
                            catch( ... ) { return WP_RTT_BACKEND_FAILED; }
                        };
                        const auto draw = []( void *data, const wp_graphics_rtt_desc * ) -> wp_rtt_result {
                            auto &p = *static_cast<FramePassContext *>( data );
                            p.rttInside = true;
                            try
                            {
                                if( !p.viewport || p.viewport->getEnableSceneRender() )
                                    p.pipelineStarted = p.owner->beginGraphicsPipelineFrame( p.renderer );
                                executeFramePassStage( p.owner->m_sys, 0.0f, data, 1 );
                                // Close HDR even when scene submission failed; never publish it.
                                if( p.pipelineStarted )
                                {
                                    p.pipelineStarted = false;
                                    p.owner->endGraphicsPipelineFrame( p.renderer );
                                }
                                executeFramePassStage( p.owner->m_sys, 0.0f, data, 3 );
                            }
                            catch( const std::exception &e )
                            {
                                p.failed = true;
                                try { WP_LOG_EXCEPTION( e ); } catch( ... ) {}
                            }
                            catch( ... )
                            {
                                p.failed = true;
                                try { WP_LOG_ERROR( "RTT draw callback failed." ); } catch( ... ) {}
                            }
                            p.rttInside = false;
                            return p.failed ? WP_RTT_BACKEND_FAILED : WP_RTT_OK;
                        };
                        const auto result = wp_renderer_dx11_execute_rtt_job(
                            native, output, &desc,
                            []( void *data, const wp_graphics_rtt_desc * ) -> wp_rtt_result {
                                auto &p = *static_cast<FramePassContext *>( data );
                                for( auto &scene : p.preparedScenes )
                                {
                                    if( !scene->validateRenderJobs() )
                                    {
                                        return WP_RTT_STALE;
                                    }
                                }
                                return WP_RTT_OK;
                            },
                            validate, draw, &pass );
                        pass.rttCompleted = true;
                        if( result != WP_RTT_OK )
                        {
                            pass.failed = true;
                            WP_LOG_ERROR( "Native offscreen job failed: " + std::to_string( result ) );
                        }
                        break;
                    }
                    if( sceneEnabled )
                    {
                        RenderPhaseProfile timing( owner->m_sceneProfile, "Scene draw" );
                        owner->render();
                    }
                    break;
                case 2:
                    if( pass.pipelineStarted )
                    {
                        pass.pipelineStarted = false;
                        owner->endGraphicsPipelineFrame( renderer );
                    }
                    break;
                case 3:
                    if( sceneEnabled && ( !viewport || viewport->getEnableUI() ) )
                        if( auto app = core::IApplicationManager::instancePtr() )
                            if( auto ui = app->getRenderUI(); ui && ui->isLoaded() ) ui->render();
                    break;
                case 4:
                    if( pass.window && owner->m_imguiManager && owner->m_imguiManager->isInitialised() )
                    {
                        owner->m_imguiManager->newFrame();
                        if( auto app = core::IApplicationManager::instancePtr() )
                            if( auto ui = app->getUI() )
                                if( auto application = ui->getApplication() )
                                {
                                    RenderPhaseProfile timing( owner->m_uiBuildProfile, "Editor UI build" );
                                    application->update();
                                }
                        RenderPhaseProfile timing( owner->m_uiSubmitProfile, "UI submission" );
                        owner->m_imguiManager->render();
                    }
                    break;
                case 5:
                    for( auto &scene : pass.preparedScenes )
                    {
                        scene->discardRenderJobs();
                    }
                    pass.preparedScenes.clear();
                    if( pass.pipelineStarted )
                    {
                        pass.pipelineStarted = false;
                        try { owner->endGraphicsPipelineFrame( renderer ); }
                        catch( ... ) { WP_LOG_ERROR( "Failed to close graphics pipeline frame." ); }
                    }
                    if( pass.begun )
                    {
                        pass.begun = false;
                        RenderPhaseProfile timing( owner->m_presentationProfile, "Presentation" );
                        renderer->endRender();
                    }
                    if( pass.nativeRtt && pass.rttCaptured )
                    {
                        renderer->setRenderTarget( pass.previousTarget );
                        renderer->setViewport( pass.previousViewport );
                        if( auto renderer3 = dynamic_pointer_cast<IRenderer3>( renderer ) )
                            renderer3->setCamera( pass.previousCamera );
                        pass.previousTarget = nullptr;
                        pass.previousViewport = nullptr;
                        pass.previousCamera = nullptr;
                        pass.rttCaptured = false;
                    }
                    break;
                }
            }
            catch( const std::exception &e )
            {
                pass.failed = true;
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                pass.failed = true;
                WP_LOG_ERROR( "Unknown exception in graphics frame adapter." );
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

            // The native C backend owns linear scene storage and the GPU filmic resolve.
            // The C89 frame driver calls this adapter before scene draw and ends it
            // in the effects stage, leaving display UI outside tone mapping.
            if( auto dx11Renderer = dynamic_pointer_cast<ClawRendererDX11>( renderer ) )
            {
                if( renderer->getRenderTarget() != getDefaultWindow() || !renderer->getCamera() ||
                    !m_graphicsPipeline->getHdrSettings().m_enableAgxToneMapping ) return false;
                return wp_renderer_dx11_begin_hdr_scene(
                    wp_renderer_get_dx11( dx11Renderer->getNativeRenderer() ), 1.0f ) != 0;
            }

            // The CPU pipeline consumes and presents the software framebuffer.
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
            if( auto dx11Renderer = dynamic_pointer_cast<ClawRendererDX11>( renderer ) )
            {
                wp_renderer_dx11_end_hdr_scene(
                    wp_renderer_get_dx11( dx11Renderer->getNativeRenderer() ) );
                return;
            }
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
                    {
                        WP_LOG_WARNING(
                            "WPGraphics: selected renderer has no native bridge; native scene "
                            "submission and UI are unavailable." );
                    }
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
            {
                WP_LOG_WARNING( "WPGraphics: window '" + name +
                                "' has zero dimensions; clamping each dimension to at least 1 pixel." );
            }

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
