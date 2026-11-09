#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/Resources/ClawMaterialResource.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <WPGraphics/Resources/ClawTextureStream.hpp>
#include <WPGraphics/Resources/FontResourceCompiler.hpp>
#include <WPGraphics/ClawFont.hpp>
#include <WPGraphics/MaterialProgramReloadDX11.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <Workphone/Graphics/ProjectIrradiance.hpp>
#include <Workphone/Graphics/ProjectGraphicsProviders.hpp>
#include <stdexcept>
#include "workphone_graphics_cull_job.h"
#include "workphone_graphics_transform_job.h"
#include <WPGraphics/ClawDebug.hpp>
#include <WPGraphics/ClawCamera.hpp>
#include <WPGraphics/ClawFontManager.hpp>
#include <WPGraphics/ClawGraphicsPipeline.hpp>
#include <WPGraphics/ClawHammerSystem.hpp>
#include <WPGraphics/ClawImguiManager.hpp>
#include "ClawImGuiFrameRecovery.hpp"
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
#include <WPGraphics/ProjectSceneLights.hpp>
#include <WPGraphics/ClawResourceGroupManager.hpp>
#include <WPGraphics/ClawScene.hpp>
#include <WPGraphics/ClawShader.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/ClawTextureManager.hpp>
#include <WPGraphics/ClawWindow.hpp>
#include <WPGraphics/UI/ClawUIManager.hpp>
#include <WPGraphics/Vehicle/ClawVehicleVisualEffects.hpp>
#include <WPImGui/WPImGui.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <exception>
#include <optional>

#include "workphone_graphics_renderer_dx11.h"
#include "workphone_graphics_renderer.h"
#include "workphone_graphics_system.h"

namespace workphone
{
    namespace render
    {
        std::shared_ptr<resource::ResourceLeafProviderFactory>
        ClawHammerSystem::createRuntimeLeafProviderFactory(
            std::shared_ptr<resource::ResourcePayloadMount> mount, String &error )
        {
#if WP_FOLIAGE_SOURCE_RESOURCES
            return createRuntimeTextureLeafProviderFactory( std::move( mount ), error );
#else
            error.clear();
            return {};
#endif
        }

        std::shared_ptr<const resource::RuntimeResource>
        ClawHammerSystem::captureRuntimeResourceSnapshot(
            std::shared_ptr<const resource::RuntimeResource> resource,
            std::shared_ptr<resource::ResourcePayloadMount> mount, String &error )
        {
#if WP_FOLIAGE_SOURCE_RESOURCES
            return snapshotRuntimeTexture( std::move( resource ), std::move( mount ), error );
#else
            error.clear();
            return resource;
#endif
        }

        SmartPtr<IMaterial> ClawHammerSystem::createRuntimeMaterial(
            std::shared_ptr<const resource::RuntimeResource> resource, String &error,
            const String &target, bool packagedBuild,
            std::shared_ptr<resource::ResourcePayloadMount> mount )
        {
#if WP_FOLIAGE_SOURCE_RESOURCES
            error.clear();
            try
            {
                auto renderer = workphone::dynamic_pointer_cast<ClawRendererDX11>( getRenderer() );
                if( !renderer || !renderer->isLoaded() )
                {
                    error = "Runtime material requires an active DX11 renderer";
                    return nullptr;
                }
                ClawMaterialResource installer( nullptr, nullptr, "", {}, renderer, target,
                                                packagedBuild );
                auto candidate =
                    installer.stageRuntime( std::move( resource ), error, std::move( mount ) );
                if( !candidate )
                {
                    return nullptr;
                }
                auto material = candidate->material();
                material->setLoadingState( LoadingState::Loaded );
                return material;
            }
            catch( const std::exception &failure )
            {
                error = failure.what();
                return nullptr;
            }
#else
            return IGraphicsSystem::createRuntimeMaterial( std::move( resource ), error, target,
                                                           packagedBuild, std::move( mount ) );
#endif
        }

        SmartPtr<ITexture> ClawHammerSystem::createRuntimeTexture(
            std::shared_ptr<const resource::RuntimeResource> resource, String &error,
            const String &target, bool packagedBuild,
            std::shared_ptr<resource::ResourcePayloadMount> mount )
        {
#if WP_FOLIAGE_SOURCE_RESOURCES
            error.clear();
            try
            {
                auto renderer = workphone::dynamic_pointer_cast<ClawRendererDX11>( getRenderer() );
                if( !resource || !renderer || !renderer->isLoaded() )
                {
                    error = "Runtime texture requires a pinned resource and active DX11 renderer";
                    return nullptr;
                }
                if( mount && isRuntimeTextureSnapshot( *resource ) )
                {
                    auto texture = make_ptr<ClawTexture>();
                    if( !texture->loadRuntimeStream(
                            resource, mount, target, packagedBuild,
                            wp_renderer_get_dx11( renderer->getNativeRenderer() ), error ) )
                    {
                        return nullptr;
                    }
                    texture->setLoadingState( LoadingState::Loaded );
                    return texture;
                }
                CookedTextureData data;
                if( !decodeCookedTexture( *resource, data, error ) )
                {
                    return nullptr;
                }
                if( data.target != target || data.packagedBuild != packagedBuild )
                {
                    error = "Runtime texture target/build mismatch";
                    return nullptr;
                }
                auto texture = make_ptr<ClawTexture>();
                auto native = wp_renderer_get_dx11( renderer->getNativeRenderer() );
                if( !( data.ddsBytes.empty()
                           ? texture->uploadCookedMips( data.levels, data.mipSettings, native, error )
                           : uploadCookedDds( texture.get(), data, native, error ) ) )
                {
                    return nullptr;
                }
                texture->setLoadingState( LoadingState::Loaded );
                return texture;
            }
            catch( const std::exception &failure )
            {
                error = failure.what();
                return nullptr;
            }
#else
            return IGraphicsSystem::createRuntimeTexture( std::move( resource ), error, target,
                                                          packagedBuild, std::move( mount ) );
#endif
        }

        SmartPtr<IFont> ClawHammerSystem::createRuntimeFont(
            std::shared_ptr<const resource::RuntimeResource> value, String &error, const String &target,
            bool packagedBuild, std::shared_ptr<resource::ResourcePayloadMount> mount )
        {
#if WP_FOLIAGE_SOURCE_RESOURCES
            error.clear();
            try
            {
                if( !value )
                {
                    error = "Runtime font requires its pinned resource";
                    return nullptr;
                }
                CookedFontData data;
                if( !decodeCookedFont( *value, data, error ) )
                {
                    return nullptr;
                }
                if( data.target != target || data.packagedBuild != packagedBuild )
                {
                    error = "Runtime font target/build mismatch";
                    return nullptr;
                }
                auto atlas =
                    createRuntimeTexture( value->dependencies[0], error, target, packagedBuild, mount );
                if( !atlas )
                {
                    return nullptr;
                }
                auto font = make_ptr<ClawFont>();
                if( !font->loadRuntimeFont( data, std::move( atlas ), error, std::move( mount ) ) )
                {
                    return nullptr;
                }
                font->setFontType( "Bitmap" );
                font->setFontSize( u32( std::lround( data.height ) ) );
                font->setFontSource( value->header.resourceId.str() );
                return font;
            }
            catch( const std::exception &failure )
            {
                try
                {
                    error = failure.what();
                }
                catch( ... )
                {
                    error.clear();
                }
                return nullptr;
            }
#else
            return IGraphicsSystem::createRuntimeFont( std::move( value ), error, target, packagedBuild,
                                                       std::move( mount ) );
#endif
        }

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

        class ClawHammerSystem::ImGuiFrameJob final : public Job
        {
        public:
            explicit ImGuiFrameJob( ClawHammerSystem *owner ) : m_owner( owner ) {}

            void execute() override
            {
                try
                {
                    if( auto owner = m_owner.lock() )
                    {
                        ScopedLock lock( owner.get() );
                        if( isInterrupted() || owner->m_imguiFrameJob.get() != this ||
                            !owner->isLoaded() || owner->m_frameUpdateActive )
                        {
                            return;
                        }
                        auto app = core::IApplicationManager::instancePtr();
                        auto window = owner->getDefaultWindow();
                        if( !app || !app->isRunning() || app->getQuit() || !window ||
                            window->isClosed() )
                        {
                            return;
                        }

                        // A widget callback must not enter a paused redraw while
                        // ImGui is still building this frame.
                        struct BuildScope
                        {
                            bool &active;
                            explicit BuildScope( bool &value ) : active( value ) { active = true; }
                            ~BuildScope() { active = false; }
                        } buildScope( owner->m_frameUpdateActive );
                        owner->m_imguiBuildAttempted = true;
                        owner->prepareImGuiFrame();
                    }
                }
                catch( const std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
                catch( ... )
                {
                    WP_LOG_ERROR( "Unknown exception while preparing an ImGui frame." );
                }
            }

        private:
            WeakPtr<ClawHammerSystem> m_owner;
        };

        struct ClawHammerSystem::FramePassContext
        {
            ClawHammerSystem *owner;
            SmartPtr<IRenderer> renderer;
            SmartPtr<IRenderTarget> target;
            SmartPtr<IViewport> viewport;
            wp_u32 flags;
            bool nativeRtt;
            IGraphicsCamera::PostProcessSettings postProcess;
            Array<IGraphicsCamera::CompositeLayer> compositeLayers;
            bool manualUpdate = false;
            bool rttCaptured = false;
            SmartPtr<IRenderTarget> previousTarget;
            SmartPtr<IViewport> previousViewport;
            SmartPtr<IGraphicsCamera> previousCamera;
            Array<SmartPtr<ClawScene>> preparedScenes;

            static wp_s32 execute( wp_graphics_system *, wp_f32, wp_graphics_system_frame_stage,
                                   void * );
            static wp_rtt_result transact( void *, wp_rtt_check_func, void * );
            static FramePassContext *&activeDrawPass()
            {
                static thread_local FramePassContext *pass = nullptr;
                return pass;
            }

            struct PreparationWork
            {
                u64 views = 0, candidates = 0, visible = 0, dispatches = 0, multiplies = 0;
                u64 hashProbes = 0, hashCollisions = 0, hashFallbacks = 0;
                u64 scratchAllocations = 0, scratchReuses = 0;
                u64 pathAllocations = 0, pathReuses = 0;
                u64 indexAllocations = 0, indexReuses = 0;
                u64 batchAllocations = 0, batchReuses = 0;
                u64 localAllocations = 0, localReuses = 0;
                u64 prefixAllocations = 0, prefixReuses = 0;
                u64 sortChecks = 0, sortPasses = 0, sortAllocations = 0, sortBytes = 0;
                u64 groupTests = 0, groupRejects = 0, groupSkipped = 0, groupFallbacks = 0;
                u64 groupInside = 0, groupInsideCandidates = 0;
                u64 timingSamples = 0, prepareNs = 0, mainPrepareNs = 0, shadowDeriveNs = 0;
                u64 phaseSamples = 0;
                wp_graphics_cull_timing_stats phases{};

                void addPhases( const wp_graphics_cull_timing_stats &value )
                {
                    phases.capture_ns += value.capture_ns;
                    phases.transform_ns += value.transform_ns;
                    phases.bounds_ns += value.bounds_ns;
                    phases.groups_ns += value.groups_ns;
                    phases.view_ns += value.view_ns;
                    phases.visibility_ns += value.visibility_ns;
                    phases.sort_ns += value.sort_ns;
                    phases.publish_ns += value.publish_ns;
                    phases.transform_hierarchy_ns += value.transform_hierarchy_ns;
                    phases.transform_unique_ns += value.transform_unique_ns;
                    phases.transform_locals_ns += value.transform_locals_ns;
                    phases.transform_prefixes_ns += value.transform_prefixes_ns;
                    phases.transform_submit_ns += value.transform_submit_ns;
                    phases.transform_wait_ns += value.transform_wait_ns;
                }

                u64 cullSharedBytes = 0, cullGroupBytes = 0, cullMapBytes = 0;
                u64 cullEntryBytes = 0, ancestorBytes = 0, ancestorRefs = 0, ancestorAllocs = 0;
                u64 cullViewBytes = 0, cullViewPeakSum = 0, cullClassScratch = 0;
                u64 spatialViews = 0, blockViews = 0, spatialMapped = 0, spatialIndividual = 0;
                u64 sphereTests = 0, sphereRejects = 0, boxTests = 0, satTests = 0, satRejects = 0,
                    satAxes = 0, satFallbacks = 0;
            } work;
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
                props->setProperty( "diagnostics.gpuProfiler",
                                    m_gpuProfilerEnabled.load( std::memory_order_relaxed ) );
                if( auto renderer = dynamic_pointer_cast<ClawRendererDX11>( getRenderer() ) )
                {
                    props->setProperty( "msaa_supported_mask",
                                        wp_renderer_dx11_supported_scene_samples(
                                            wp_renderer_get_dx11( renderer->getNativeRenderer() ) ) );
                }
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
                if( properties->getPropertyValue( "diagnostics.gpuProfiler", enabled ) )
                {
                    m_gpuProfilerEnabled.store( enabled, std::memory_order_relaxed );
                }
                if( properties->getPropertyValue( "vsync", enabled ) ||
                    properties->getPropertyValue( "graphics.vsync", enabled ) )
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
            FramePassContext::PreparationWork work;
            for( const auto &pass : m_framePasses )
            {
                work.views += pass.work.views;
                work.candidates += pass.work.candidates;
                work.visible += pass.work.visible;
                work.dispatches += pass.work.dispatches;
                work.multiplies += pass.work.multiplies;
                work.hashProbes += pass.work.hashProbes;
                work.hashCollisions += pass.work.hashCollisions;
                work.hashFallbacks += pass.work.hashFallbacks;
                work.scratchAllocations += pass.work.scratchAllocations;
                work.scratchReuses += pass.work.scratchReuses;
                work.indexAllocations += pass.work.indexAllocations;
                work.indexReuses += pass.work.indexReuses;
                work.batchAllocations += pass.work.batchAllocations;
                work.batchReuses += pass.work.batchReuses;
                work.localAllocations += pass.work.localAllocations;
                work.localReuses += pass.work.localReuses;
                work.prefixAllocations += pass.work.prefixAllocations;
                work.prefixReuses += pass.work.prefixReuses;
                work.sortChecks += pass.work.sortChecks;
                work.sortPasses += pass.work.sortPasses;
                work.sortAllocations += pass.work.sortAllocations;
                work.sortBytes += pass.work.sortBytes;
                work.groupTests += pass.work.groupTests;
                work.groupRejects += pass.work.groupRejects;
                work.groupSkipped += pass.work.groupSkipped;
                work.groupFallbacks += pass.work.groupFallbacks;
                work.groupInside += pass.work.groupInside;
                work.groupInsideCandidates += pass.work.groupInsideCandidates;
                work.timingSamples += pass.work.timingSamples;
                work.prepareNs += pass.work.prepareNs;
                work.mainPrepareNs += pass.work.mainPrepareNs;
                work.shadowDeriveNs += pass.work.shadowDeriveNs;
                work.phaseSamples += pass.work.phaseSamples;
                work.addPhases( pass.work.phases );
                work.cullSharedBytes += pass.work.cullSharedBytes;
                work.cullEntryBytes += pass.work.cullEntryBytes;
                work.ancestorBytes += pass.work.ancestorBytes;
                work.ancestorRefs += pass.work.ancestorRefs;
                work.ancestorAllocs += pass.work.ancestorAllocs;
                work.cullGroupBytes += pass.work.cullGroupBytes;
                work.cullMapBytes += pass.work.cullMapBytes;
                work.cullViewBytes += pass.work.cullViewBytes;
                work.cullViewPeakSum += pass.work.cullViewPeakSum;
                work.cullClassScratch += pass.work.cullClassScratch;
                work.spatialViews += pass.work.spatialViews;
                work.blockViews += pass.work.blockViews;
                work.spatialMapped += pass.work.spatialMapped;
                work.spatialIndividual += pass.work.spatialIndividual;
                work.pathAllocations += pass.work.pathAllocations;
                work.pathReuses += pass.work.pathReuses;
                work.sphereTests += pass.work.sphereTests;
                work.sphereRejects += pass.work.sphereRejects;
                work.boxTests += pass.work.boxTests;
                work.satTests += pass.work.satTests;
                work.satRejects += pass.work.satRejects;
                work.satAxes += pass.work.satAxes;
                work.satFallbacks += pass.work.satFallbacks;
            }
            char preparation[2560];
            std::snprintf(
                preparation, sizeof( preparation ),
                "\nPreparation last frame: views=%llu candidates=%llu main-visible=%llu "
                "transform dispatches=%llu matrix multiplies=%llu sphere tests=%llu rejects=%llu "
                "box tests=%llu SAT tests=%llu rejects=%llu axes=%llu fallbacks=%llu "
                "reference scratch allocs=%llu reuses=%llu path records allocs=%llu reuses=%llu "
                "index records allocs=%llu reuses=%llu batch records allocs=%llu reuses=%llu "
                "local matrices allocs=%llu reuses=%llu prefix cache allocs=%llu reuses=%llu "
                "main sort checks=%llu passes=%llu scratch allocs=%llu bytes=%llu "
                "coarse groups tests=%llu rejects=%llu skipped=%llu fallbacks=%llu inside=%llu "
                "candidates=%llu "
                "CPU prepare samples=%llu total-ns=%llu main-ns=%llu shadow-ns=%llu "
                "cull arrays shared=%llu groups=%llu map=%llu view=%llu view-peak-sum=%llu "
                "class-scratch=%llu spatial-views=%llu block-views=%llu mapped=%llu individual=%llu",
                (unsigned long long)work.views, (unsigned long long)work.candidates,
                (unsigned long long)work.visible, (unsigned long long)work.dispatches,
                (unsigned long long)work.multiplies, (unsigned long long)work.sphereTests,
                (unsigned long long)work.sphereRejects, (unsigned long long)work.boxTests,
                (unsigned long long)work.satTests, (unsigned long long)work.satRejects,
                (unsigned long long)work.satAxes, (unsigned long long)work.satFallbacks,
                (unsigned long long)work.scratchAllocations, (unsigned long long)work.scratchReuses,
                (unsigned long long)work.pathAllocations, (unsigned long long)work.pathReuses,
                (unsigned long long)work.indexAllocations, (unsigned long long)work.indexReuses,
                (unsigned long long)work.batchAllocations, (unsigned long long)work.batchReuses,
                (unsigned long long)work.localAllocations, (unsigned long long)work.localReuses,
                (unsigned long long)work.prefixAllocations, (unsigned long long)work.prefixReuses,
                (unsigned long long)work.sortChecks, (unsigned long long)work.sortPasses,
                (unsigned long long)work.sortAllocations, (unsigned long long)work.sortBytes,
                (unsigned long long)work.groupTests, (unsigned long long)work.groupRejects,
                (unsigned long long)work.groupSkipped, (unsigned long long)work.groupFallbacks,
                (unsigned long long)work.groupInside, (unsigned long long)work.groupInsideCandidates,
                (unsigned long long)work.timingSamples, (unsigned long long)work.prepareNs,
                (unsigned long long)work.mainPrepareNs, (unsigned long long)work.shadowDeriveNs,
                (unsigned long long)work.cullSharedBytes, (unsigned long long)work.cullGroupBytes,
                (unsigned long long)work.cullMapBytes, (unsigned long long)work.cullViewBytes,
                (unsigned long long)work.cullViewPeakSum, (unsigned long long)work.cullClassScratch,
                (unsigned long long)work.spatialViews, (unsigned long long)work.blockViews,
                (unsigned long long)work.spatialMapped, (unsigned long long)work.spatialIndividual );
            result += preparation;
            char phases[640];
            std::snprintf(
                phases, sizeof( phases ),
                "\nCPU main phases samples=%llu capture-ns=%llu transform-ns=%llu bounds-ns=%llu "
                "groups-ns=%llu view-ns=%llu visibility-ns=%llu sort-ns=%llu publish-ns=%llu",
                (unsigned long long)work.phaseSamples, (unsigned long long)work.phases.capture_ns,
                (unsigned long long)work.phases.transform_ns, (unsigned long long)work.phases.bounds_ns,
                (unsigned long long)work.phases.groups_ns, (unsigned long long)work.phases.view_ns,
                (unsigned long long)work.phases.visibility_ns, (unsigned long long)work.phases.sort_ns,
                (unsigned long long)work.phases.publish_ns );
            result += phases;
            char transforms[512];
            std::snprintf( transforms, sizeof( transforms ),
                           "\nCPU transform phases hierarchy-ns=%llu unique-ns=%llu locals-ns=%llu "
                           "prefixes-ns=%llu submit-ns=%llu wait-ns=%llu",
                           (unsigned long long)work.phases.transform_hierarchy_ns,
                           (unsigned long long)work.phases.transform_unique_ns,
                           (unsigned long long)work.phases.transform_locals_ns,
                           (unsigned long long)work.phases.transform_prefixes_ns,
                           (unsigned long long)work.phases.transform_submit_ns,
                           (unsigned long long)work.phases.transform_wait_ns );
            result += transforms;
            char hash[192];
            std::snprintf( hash, sizeof( hash ),
                           "\nTransform capture hash probes=%llu collisions=%llu sort-fallbacks=%llu",
                           (unsigned long long)work.hashProbes, (unsigned long long)work.hashCollisions,
                           (unsigned long long)work.hashFallbacks );
            result += hash;
            char ancestors[256];
            std::snprintf( ancestors, sizeof( ancestors ),
                           "\nCull publication entries-bytes=%llu ancestor-bytes=%llu "
                           "ancestor-refs=%llu ancestor-allocs=%llu",
                           (unsigned long long)work.cullEntryBytes,
                           (unsigned long long)work.ancestorBytes, (unsigned long long)work.ancestorRefs,
                           (unsigned long long)work.ancestorAllocs );
            result += ancestors;
            std::lock_guard<std::mutex> lock( m_statisticsMutex );
            m_renderStatistics = std::move( result );
        }

        bool ClawHammerSystem::getVSync() const
        {
            return m_vsync.load( std::memory_order_relaxed );
        }

        void ClawHammerSystem::setSceneProfilingEnabled( bool enabled )
        {
            std::lock_guard<std::mutex> lock( m_sceneProfileMutex );
            if( m_sceneProfilingEnabled.load( std::memory_order_relaxed ) != enabled )
            {
                ++m_sceneProfileGeneration;
                m_publishedSceneProfile = {};
                m_sceneProfilingEnabled.store( enabled, std::memory_order_release );
            }
        }

        bool ClawHammerSystem::getSceneProfilingEnabled() const
        {
            return m_sceneProfilingEnabled.load( std::memory_order_acquire );
        }

        void ClawHammerSystem::resetSceneProfile()
        {
            std::lock_guard<std::mutex> lock( m_sceneProfileMutex );
            ++m_sceneProfileGeneration;
            m_publishedSceneProfile = {};
        }

        bool ClawHammerSystem::readSceneProfile( SceneProfileSnapshot &snapshot ) const
        {
            std::lock_guard<std::mutex> lock( m_sceneProfileMutex );
            snapshot = {};
            if( !m_sceneProfilingEnabled.load( std::memory_order_relaxed ) ||
                !m_publishedSceneProfile.serial )
            {
                return false;
            }
            snapshot = m_publishedSceneProfile;
            return true;
        }

        void ClawHammerSystem::beginSceneProfileFrame()
        {
            m_sceneProfileFrameActive = false;
            if( !m_sceneProfilingEnabled.load( std::memory_order_acquire ) )
            {
                return;
            }
            std::lock_guard<std::mutex> lock( m_sceneProfileMutex );
            if( m_sceneProfilingEnabled.load( std::memory_order_relaxed ) )
            {
                m_currentSceneProfile = {};
                m_currentSceneProfile.generation = m_sceneProfileGeneration;
                m_currentSceneProfile.serial = ++m_sceneProfileSequence;
                m_currentSceneProfile.registeredScenes = m_scenes.size();
                m_sceneProfileFrameActive = true;
            }
        }

        void ClawHammerSystem::publishSceneProfileFrame()
        {
            if( !m_sceneProfileFrameActive )
            {
                return;
            }

            // Native and renderer tables are inspected only by the graphics owner.
            // Public readers receive this fixed-size copy after jobs have retired.
            auto &snapshot = m_currentSceneProfile;
            wp_graphics_system_frame_stats frame{};
            if( m_sys && wp_graphics_system_get_frame_stats( m_sys, &frame ) )
            {
                snapshot.nativeUpdateSerial = frame.update_serial;
                snapshot.nativeRenderSerial = frame.render_serial;
                snapshot.nativeDeltaTime = frame.delta_time;
                snapshot.frameSimulationEnabled = m_frameSimulationEnabled;
                snapshot.completedPasses =
                    frame.completed_pass_count > 0 ? frame.completed_pass_count : 0;
                snapshot.skippedPasses = frame.skipped_pass_count > 0 ? frame.skipped_pass_count : 0;
                snapshot.failedPasses = wp_graphics_system_get_failed_pass_count( m_sys );
            }
            if( auto renderer = dynamic_pointer_cast<ClawRendererDX11>( getRenderer() ) )
            {
                const auto cache = renderer->getMaterialRecordStatistics();
                snapshot.cachePreparationsLifetime = cache.preparations;
                snapshot.cacheStorageAllocationsLifetime = cache.storageAllocations;
                snapshot.directStorageAllocationsLifetime = cache.directStorageAllocations;
                snapshot.cacheStorageBytes = cache.storageBytes;
                snapshot.cacheSlots = cache.slots;
                snapshot.cacheResident = cache.resident;
                if( auto dx11 = wp_renderer_get_dx11( renderer->getNativeRenderer() ) )
                {
                    wp_render_statistics_dx11 stats{};
                    // Profiling must not allocate or sort frame history every frame.
                    wp_renderer_dx11_get_statistics_counters( dx11, &stats );
                    snapshot.dx11Available = true;
                    snapshot.dx11FramesLifetime = stats.frames;
                    snapshot.dx11DrawsLifetime = stats.draws;
                    snapshot.dx11TrianglesLifetime = stats.triangles;
                    snapshot.dx11MaterialUploadsLifetime = stats.material_uploads;
                    snapshot.dx11TransformUploadsLifetime = stats.transform_uploads;
                    snapshot.dx11StateBindingsLifetime = stats.state_bindings;
                    snapshot.dx11InstancesLifetime = stats.instances;
                    snapshot.dx11GpuSamplesLifetime = stats.gpu_samples;
                    snapshot.dx11PresentAttemptsLifetime = stats.present_attempts;
                    snapshot.dx11PresentOccludedLifetime = stats.present_occluded;
                    snapshot.dx11PresentFailuresLifetime = stats.present_failures;
                    snapshot.dx11IntervalMs = stats.interval_ms;
                    snapshot.dx11P95IntervalMs = stats.interval_p95_ms;
                    snapshot.dx11CpuFrameMs = stats.cpu_frame_ms;
                    snapshot.dx11PresentMs = stats.present_ms;
                    snapshot.dx11GpuFrameMs = stats.gpu_frame_ms;
                }
            }

            std::lock_guard<std::mutex> lock( m_sceneProfileMutex );
            if( m_sceneProfilingEnabled.load( std::memory_order_relaxed ) &&
                snapshot.generation == m_sceneProfileGeneration )
            {
                m_publishedSceneProfile = snapshot;
            }
            m_sceneProfileFrameActive = false;
        }

        void ClawHammerSystem::invalidateSceneProfileFrame()
        {
            if( m_sceneProfileFrameActive )
            {
                std::lock_guard<std::mutex> lock( m_sceneProfileMutex );
                if( m_currentSceneProfile.generation == m_sceneProfileGeneration )
                {
                    m_publishedSceneProfile = {};
                }
                m_sceneProfileFrameActive = false;
            }
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
            setSceneProfilingEnabled( false );
            if( getLoadingState() == LoadingState::Unloaded && !m_sys )
            {
                return;
            }

            ScopedLock lock( this );
            m_sceneProfileFrameActive = false;
            setLoadingState( LoadingState::Unloading );
            cancelImGuiFrame();

            m_preparationProfile = nullptr;
            m_sceneProfile = nullptr;
            m_uiBuildProfile = nullptr;
            m_uiSubmitProfile = nullptr;
            m_presentationProfile = nullptr;

            // Queued objects can be owned by the graphics managers below. Release
            // their references before those managers destroy the objects.
            clearObjectQueues();
            m_reloadQueue.clear();

            m_framePasses.clear();

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
            updateFrame( true );
        }

        bool ClawHammerSystem::updatePausedFrame()
        {
            return updateFrame( false );
        }

        void ClawHammerSystem::queueImGuiFrame()
        {
            const auto renderTask = getRenderTask();
            if( !m_frameSimulationEnabled || Thread::getCurrentTask() != renderTask ||
                ( m_imguiFrameJob && !m_imguiFrameJob->isFinished() ) )
            {
                return;
            }

            auto app = core::IApplicationManager::instancePtr();
            auto queue = app ? app->getJobQueue() : nullptr;
            auto taskManager = app ? app->getTaskManager() : nullptr;
            auto task = taskManager ? taskManager->getTask( renderTask ) : nullptr;
            if( !queue || !queue->isRunning() || !task || !task->isExecuting() )
            {
                return;
            }

            // Editor callbacks use render-task affinity to defer Lua work. The
            // task runs this job at its next tick, before it enters a native pass.
            if( !m_imguiFrameJob )
            {
                m_imguiFrameJob = workphone::make_ptr<ImGuiFrameJob>( this );
            }
            queue->addJob( m_imguiFrameJob, renderTask );
        }

        void ClawHammerSystem::prepareImGuiFrame()
        {
            auto manager = m_imguiManager;
            if( !manager || !manager->isInitialised() )
            {
                return;
            }

            RenderPhaseProfile timing( m_uiBuildProfile, "Editor UI build" );
            try
            {
                manager->newFrame();
                if( auto app = core::IApplicationManager::instancePtr() )
                {
                    if( auto ui = app->getUI() )
                    {
                        if( auto application = ui->getApplication() )
                        {
                            application->update();
                        }
                    }
                }
                manager->finishFrame();
            }
            catch( ... )
            {
                detail::discardInterruptedImGuiFrame( *manager );
                throw;
            }
        }

        void ClawHammerSystem::drawImGuiOverlay()
        {
            if( !m_imguiManager || !m_imguiManager->isInitialised() )
            {
                return;
            }

            // First frame, paused redraws, and callers without a running render
            // task prepare immediately. Never retry a partially executed build.
            if( !m_imguiManager->hasPreparedFrame() && !m_imguiBuildAttempted )
            {
                m_imguiBuildAttempted = true;
                prepareImGuiFrame();
            }
            if( m_imguiManager->hasPreparedFrame() )
            {
                RenderPhaseProfile timing( m_uiSubmitProfile, "UI submission" );
                m_imguiManager->render();
                m_imguiBuildAttempted = false;
            }
            queueImGuiFrame();
        }

        void ClawHammerSystem::cancelImGuiFrame()
        {
            if( m_imguiFrameJob )
            {
                m_imguiFrameJob->stop();
                m_imguiFrameJob = nullptr;
            }
            m_imguiBuildAttempted = false;
            if( m_imguiManager )
            {
                m_imguiManager->discardPreparedFrame();
            }
        }

        bool ClawHammerSystem::updateFrame( bool simulationEnabled )
        {
            if( !isLoaded() || !m_sys )
            {
                if( getSceneProfilingEnabled() )
                {
                    resetSceneProfile();
                }
                return false;
            }

            try
            {
                ScopedLock lock( this );
                // The native renderer and ImGui both own in-flight frame state.
                // Reentrant submission cannot redraw a suspended frame safely.
                if( m_frameUpdateActive )
                {
                    return false;
                }

                if( !simulationEnabled )
                {
                    // The debugger's direct redraw does not pump render-task jobs.
                    cancelImGuiFrame();
                }

                struct FrameMode
                {
                    ClawHammerSystem &owner;
                    bool previousSimulation;

                    FrameMode( ClawHammerSystem &value, bool enabled ) :
                        owner( value ),
                        previousSimulation( value.m_frameSimulationEnabled )
                    {
                        owner.m_frameUpdateActive = true;
                        owner.m_frameSimulationEnabled = enabled;
                    }

                    ~FrameMode()
                    {
                        // A skipped window pass must not leave draw data queued.
                        if( owner.m_imguiManager && owner.m_imguiManager->hasPreparedFrame() )
                        {
                            owner.m_imguiManager->discardPreparedFrame();
                        }
                        owner.m_imguiBuildAttempted = false;
                        owner.m_frameSimulationEnabled = previousSimulation;
                        owner.m_frameUpdateActive = false;
                    }
                } frameMode( *this, simulationEnabled );

                beginSceneProfileFrame();
                using ProfileClock = std::chrono::steady_clock;
                const auto profileStart =
                    m_sceneProfileFrameActive ? ProfileClock::now() : ProfileClock::time_point{};
                std::optional<MaterialPublicationScope> materialUpdates;
                if( simulationEnabled )
                {
                    materialUpdates.emplace();
                }
                RenderPhaseProfile preparation( m_preparationProfile, "Graphics preparation" );
                if( simulationEnabled && m_statisticsResetRequested.exchange( false ) )
                {
                    if( auto renderer = dynamic_pointer_cast<ClawRendererDX11>( getRenderer() ) )
                    {
                        if( auto dx11 = wp_renderer_get_dx11( renderer->getNativeRenderer() ) )
                        {
                            wp_renderer_dx11_reset_statistics( dx11 );
                        }
                    }
                }
                if( simulationEnabled )
                {
                    GraphicsSystem::update();
                    updateUnloadQueue();
                }

                for( auto &window : m_windows )
                {
                    if( window )
                    {
                        window->update();
                    }
                }

                auto applicationManager = core::IApplicationManager::instancePtr();
                auto renderUI = applicationManager ? applicationManager->getRenderUI() : nullptr;

                if( renderUI && simulationEnabled )
                {
                    renderUI->update();
                }
                else if( auto nativeUI = dynamic_pointer_cast<ui::ClawUIManager>( renderUI ) )
                {
                    // Widget drawing/input remains available, but native UI
                    // animation must not reuse the interrupted positive delta.
                    if( auto context = nativeUI->getContext() )
                    {
                        context->delta_time_seconds = 0.0f;
                    }
                }

                if( auto debug = simulationEnabled ? getDebug() : nullptr )
                {
                    debug->preUpdate();
                    debug->update();
                    debug->postUpdate();
                }

                wp_f32 deltaTime = simulationEnabled ? 1.0f / 60.0f : 0.0f;

                if( auto timer = simulationEnabled && applicationManager ? applicationManager->getTimer()
                                                                         : nullptr )
                {
                    deltaTime = static_cast<wp_f32>( timer->getDeltaTime() );
                }

                if( materialUpdates )
                {
                    materialUpdates->commit();  // Normal update publication boundary.
                }
                if( auto renderer = simulationEnabled
                                        ? dynamic_pointer_cast<ClawRendererDX11>( getRenderer() )
                                        : nullptr )
                {
                    ClawMaterial::processRuntimeShaderReloads( *renderer );
                    auto native = renderer->getNativeRenderer();
                    auto dx = native ? wp_renderer_get_dx11( native ) : nullptr;
                    MaterialProgramReloadDX11::UpdateAll(
                        dx ? wp_renderer_dx11_get_device( dx ) : nullptr, renderer.get() );
                    ClawMaterial::processRuntimeShaderReloads( *renderer );
                }
                prepareFramePasses( deltaTime );
                preparation.end();
                const auto nativeStart =
                    m_sceneProfileFrameActive ? ProfileClock::now() : ProfileClock::time_point{};
                // The C89 system owns native update and the complete ordered frame.
                // Keep the engine's update-only scheduling contract at this boundary.
                if( !renderFramePasses( deltaTime, true ) )
                {
                    throw std::runtime_error( "Native graphics frame was rejected." );
                }
                const auto nativeEnd =
                    m_sceneProfileFrameActive ? ProfileClock::now() : ProfileClock::time_point{};
                publishRenderStatistics();

                if( simulationEnabled )
                {
                    for( auto &scene : m_scenes.snapshot() )
                    {
                        if( scene )
                        {
                            scene->postUpdate();
                        }
                    }
                }
                if( m_sceneProfileFrameActive )
                {
                    const auto finished = ProfileClock::now();
                    auto &snapshot = m_currentSceneProfile;
                    snapshot.graphicsPrepareNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                     nativeStart - profileStart )
                                                     .count();
                    snapshot.nativeFrameNs =
                        std::chrono::duration_cast<std::chrono::nanoseconds>( nativeEnd - nativeStart )
                            .count();
                    snapshot.graphicsUpdateNs =
                        std::chrono::duration_cast<std::chrono::nanoseconds>( finished - profileStart )
                            .count();
                    publishSceneProfileFrame();
                }
                return true;
            }
            catch( std::exception &e )
            {
                invalidateSceneProfileFrame();
                WP_LOG_EXCEPTION( e );
            }
            return false;
        }

        void ClawHammerSystem::prepareFramePasses( wp_f32 deltaTime )
        {
            MaterialProgramReloadDX11::CaptureScope shaderCapture;
            m_framePasses.clear();
            auto renderer = getRenderer();
            if( !renderer )
            {
                return;
            }
            auto dx11 = dynamic_pointer_cast<ClawRendererDX11>( renderer );
            if( dx11 )
            {
                wp_renderer_dx11_set_gpu_profiling(
                    wp_renderer_get_dx11( dx11->getNativeRenderer() ),
                    m_gpuProfilerEnabled.load( std::memory_order_relaxed ) );
                const auto probes = getProjectIrradianceProbes();
                if( !wp_renderer_dx11_set_irradiance_probes(
                        wp_renderer_get_dx11( dx11->getNativeRenderer() ),
                        probes.empty() ? nullptr : probes.data(), static_cast<wp_u32>( probes.size() ),
                        getProjectIrradianceMode() ) )
                {
                    WP_LOG_WARNING(
                        "Project irradiance upload failed; retaining the previous GPU probe set." );
                }
            }
            if( auto pipeline = dynamic_pointer_cast<ClawGraphicsPipeline>( m_graphicsPipeline ) )
            {
                pipeline->setFrameDeltaTime( deltaTime );
            }
            m_framePipelineSettings = dx11 && m_graphicsPipeline ? m_graphicsPipeline->getRenderSettings()
                                                                : nullptr;
            if( !m_framePipelineSettings ||
                !m_framePipelineSettings->dynamicResolution )
            {
                m_adaptiveSceneScale = 1.0f;
            }
            else if( m_frameSimulationEnabled && std::isfinite( deltaTime ) && deltaTime > 0.0f )
            {
                const auto fps = m_framePipelineSettings->frameRateLimit;
                if( deltaTime > 1.1f / fps )
                    m_adaptiveSceneScale = std::max( 0.5f, m_adaptiveSceneScale - 0.0625f );
                // A paced renderer can recover at its frame-rate limit.
                else if( deltaTime <= 1.03f / fps )
                    m_adaptiveSceneScale = std::min( 1.0f, m_adaptiveSceneScale + 0.03125f );
            }
            const auto activeLayer = []( const IGraphicsCamera::CompositeLayer &layer ) {
                return layer.enabled && layer.texture &&
                       std::isfinite( layer.opacity ) && layer.opacity > 0.0f;
            };
            const auto capture = [&]( const SmartPtr<IRenderTarget> &target,
                                      const SmartPtr<IViewport> &viewport, bool window,
                                      bool manualUpdate = false ) {
                if( m_framePasses.size() >= WP_GRAPHICS_SYSTEM_MAX_RENDER_PASSES )
                {
                    throw std::runtime_error( "Graphics frame exceeds native render pass capacity." );
                }
                wp_u32 flags = window ? WP_GRAPHICS_FRAME_TARGET_WINDOW : 0u;
                if( !viewport || viewport->getEnableSceneRender() ) flags |= WP_GRAPHICS_FRAME_TARGET_SCENE;
                if( !viewport || viewport->getEnableUI() ) flags |= WP_GRAPHICS_FRAME_TARGET_UI;
                if( !viewport || viewport->getOverlaysEnabled() ) flags |= WP_GRAPHICS_FRAME_TARGET_OVERLAY;
                if( viewport )
                {
                    flags |= viewport->getClearEveryFrame() ? WP_GRAPHICS_FRAME_TARGET_CLEAR
                                                           : WP_GRAPHICS_FRAME_TARGET_PRESERVE_WINDOW;
                }
                const bool nativeRtt = !window && dx11 && dynamic_pointer_cast<ClawRenderTarget>( target );
                FramePassContext pass{ this, renderer, target, viewport, flags, nativeRtt };
                pass.manualUpdate = manualUpdate;
                if( auto camera = dx11 && viewport ? viewport->getCamera() : nullptr )
                {
                    const auto clawCamera = dynamic_pointer_cast<ClawCamera>( camera );
                    pass.postProcess = clawCamera ? clawCamera->getRenderPostProcessSettings()
                                                 : camera->getPostProcessSettings();
                    pass.compositeLayers = camera->getCompositeLayers();
                    if( pass.postProcess.enabled ||
                        std::any_of( pass.compositeLayers.begin(), pass.compositeLayers.end(), activeLayer ) )
                    {
                        pass.flags |= WP_GRAPHICS_FRAME_TARGET_CAMERA_EFFECTS;
                    }
                }
                m_framePasses.push_back( std::move( pass ) );
            };
            struct TargetGroup
            {
                IRenderTarget *target;
                size_t begin;
                size_t end;
                bool visited = false;
            };
            std::vector<TargetGroup> groups;
            if( auto textures = getTextureManager() )
            {
                for( auto &texture : textures->getTextures() )
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
                    auto clawTarget = dynamic_pointer_cast<ClawRenderTarget>( target );
                    const bool manualUpdate = clawTarget && clawTarget->takeUpdateRequest();
                    if( !manualUpdate && !target->isAutoUpdated() )
                    {
                        continue;
                    }
                    std::vector<std::pair<s32, SmartPtr<IViewport>>> ordered;
                    for( const auto &viewport : target->getViewports() )
                    {
                        if( viewport && viewport->isActive() &&
                            ( manualUpdate || viewport->isAutoUpdated() ) )
                        {
                            ordered.emplace_back( viewport->getZOrder(), viewport );
                        }
                    }
                    if( ordered.empty() )
                    {
                        if( manualUpdate ) clawTarget->update();
                        continue;
                    }
                    std::stable_sort( ordered.begin(), ordered.end(),
                                      []( const auto &a, const auto &b ) { return a.first < b.first; } );
                    const auto begin = m_framePasses.size();
                    for( const auto &[order, viewport] : ordered )
                    {
                        capture( target, viewport, false, manualUpdate );
                    }
                    groups.push_back( { target.get(), begin, m_framePasses.size() } );
                }
            }
            if( groups.size() > 1 )
            {
                // A dependency that closes a cycle reads its last published frame.
                // Visit in capture order to keep that choice stable across frames.
                std::vector<FramePassContext> ordered;
                ordered.reserve( m_framePasses.size() + 1 );
                const auto visit = [&]( auto &&visit, TargetGroup &group ) -> void {
                    if( group.visited ) return;
                    group.visited = true;
                    for( size_t i = group.begin; i < group.end; ++i )
                    {
                        for( const auto &layer : m_framePasses[i].compositeLayers )
                        {
                            if( !activeLayer( layer ) ) continue;
                            const auto source = layer.texture->getRenderTarget();
                            const auto producer = std::find_if( groups.begin(), groups.end(),
                                [&]( const auto &candidate ) { return candidate.target == source.get(); } );
                            if( producer != groups.end() ) visit( visit, *producer );
                        }
                    }
                    for( size_t i = group.begin; i < group.end; ++i )
                    {
                        ordered.push_back( std::move( m_framePasses[i] ) );
                    }
                };
                for( auto &group : groups ) visit( visit, group );
                m_framePasses.swap( ordered );
            }
            auto window = getDefaultWindow();
            if( window )
            {
                SmartPtr<IViewport> foreground;
                const auto viewports = window->getViewports();
                for( const auto &viewport : viewports )
                {
                    if( viewport && viewport->isActive() && viewport->isAutoUpdated() &&
                        ( !foreground || viewport->getZOrder() > foreground->getZOrder() ) )
                    {
                        foreground = viewport;
                    }
                }
                const auto app = core::IApplicationManager::instancePtr();
                if( foreground || viewports.empty() || ( app && app->isEditor() ) )
                {
                    capture( window, foreground, true );
                }
            }
        }

        bool ClawHammerSystem::renderFramePasses( wp_f32 deltaTime, bool advance )
        {
            std::array<wp_graphics_system_frame_target, WP_GRAPHICS_SYSTEM_MAX_RENDER_PASSES> targets;
            wp_s32 count = 0;
            for( auto &pass : m_framePasses )
            {
                targets[count++] = { pass.flags, &pass, pass.nativeRtt ? &FramePassContext::transact : nullptr };
            }
            return wp_graphics_system_render_target_frame( m_sys, deltaTime, advance, targets.data(),
                                                          count, &FramePassContext::execute ) != 0;
        }

        wp_s32 ClawHammerSystem::FramePassContext::execute( wp_graphics_system *system, wp_f32,
            wp_graphics_system_frame_stage stage, void *data )
        {
            auto &pass = *static_cast<FramePassContext *>( data );
            auto owner = pass.owner;
            if( owner->m_sys != system )
            {
                return WP_GRAPHICS_FRAME_SKIP;
            }
            auto &viewport = pass.viewport;
            auto &renderer = pass.renderer;
            try
            {
                switch( stage )
                {
                case WP_GRAPHICS_FRAME_PREPARE_TARGET:
                    pass.work = {};
                    // Skip detached, inactive and minimized targets before entering
                    // begin/end/present. Keep virtual calls behind this exception barrier.
                    if( !renderer || !pass.target ||
                        ( !(pass.flags & WP_GRAPHICS_FRAME_TARGET_WINDOW) && !pass.target->isActive() ) ||
                        pass.target->getSize().x <= 0 || pass.target->getSize().y <= 0 ||
                        ( viewport && !viewport->isActive() ) )
                    {
                        return WP_GRAPHICS_FRAME_SKIP;
                    }
                    if( pass.nativeRtt )
                    {
                        pass.previousTarget = renderer->getRenderTarget();
                        pass.previousViewport = renderer->getViewport();
                        pass.previousCamera = renderer->getCamera();
                        pass.rttCaptured = true;
                    }
                    if( pass.flags & WP_GRAPHICS_FRAME_TARGET_SCENE )
                    {
                        auto camera = viewport ? viewport->getCamera() : nullptr;
                        // Resolve the pass aspect before capture, without binding its target.
                        if( camera && viewport )
                        {
                            const auto size = viewport->getActualSize();
                            if( size.X() > 0 && size.Y() > 0 )
                            {
                                camera->setAspectRatio( size.X() / size.Y() );
                            }
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
                                if( !clawScene->prepareRenderJobs(
                                        camera,
                                        owner->m_statisticsSnapshotRequested.load() ||
                                            owner->m_sceneProfileFrameActive,
                                        dynamic_cast<ClawRendererDX11 *>( renderer.get() ) ) )
                                {
                                    throw std::runtime_error(
                                        "Native transform/cull preparation failed." );
                                }
                                wp_transform_job_stats transforms{};
                                wp_graphics_cull_bounds_stats bounds{};
                                wp_graphics_cull_sort_stats sort{};
                                wp_u32 candidates = 0, visible = 0;
                                clawScene->getPreparedJobStatistics( &transforms, &bounds, &candidates,
                                                                     &visible, &sort );
                                pass.work.sortChecks += sort.order_checks;
                                pass.work.sortPasses += sort.merge_passes;
                                pass.work.sortAllocations += sort.scratch_allocations;
                                pass.work.sortBytes += sort.scratch_bytes;
                                ClawScene::PreparationTiming timing;
                                if( clawScene->getPreparedTiming( timing ) )
                                {
                                    ++pass.work.timingSamples;
                                    pass.work.prepareNs += timing.total_ns;
                                    pass.work.mainPrepareNs += timing.main_prepare_ns;
                                    pass.work.shadowDeriveNs += timing.shadow_derive_ns;
                                }
                                wp_graphics_cull_storage_stats storage{};
                                wp_graphics_cull_timing_stats phases{};
                                if( clawScene->getPreparedPhaseTiming( &phases ) )
                                {
                                    ++pass.work.phaseSamples;
                                    pass.work.addPhases( phases );
                                }
                                clawScene->getPreparedStorageStatistics( &storage );
                                if( owner->m_sceneProfileFrameActive )
                                {
                                    auto &snapshot = owner->m_currentSceneProfile;
                                    ++snapshot.scenePreparations;
                                    ++snapshot.mainViews;
                                    ++snapshot.shadowViews;
                                    snapshot.candidates += candidates;
                                    snapshot.mainVisible += visible;
                                    snapshot.transformDispatches +=
                                        clawScene->getPreparedTransformDispatchCount();
                                    snapshot.preparationTotalNs += timing.total_ns;
                                    snapshot.mainPrepareNs += timing.main_prepare_ns;
                                    snapshot.shadowDeriveNs += timing.shadow_derive_ns;
                                    snapshot.materialPrepareNs += timing.material_prepare_ns;
                                    snapshot.cullPublicationBytesSum += storage.publication_bytes;
                                    snapshot.cullViewBytesSum += storage.view_index_bytes;
                                    snapshot.cullCaptureNs += phases.capture_ns;
                                    snapshot.cullTransformNs += phases.transform_ns;
                                    snapshot.cullBoundsNs += phases.bounds_ns;
                                    snapshot.cullGroupsNs += phases.groups_ns;
                                    snapshot.cullViewNs += phases.view_ns;
                                    snapshot.cullVisibilityNs += phases.visibility_ns;
                                    snapshot.cullSortNs += phases.sort_ns;
                                    snapshot.cullPublishNs += phases.publish_ns;
                                    ClawScene::MaterialPreparationStatistics materials;
                                    if( clawScene->getPreparedMaterialStatistics( materials ) )
                                    {
                                        snapshot.materialRecords += materials.records;
                                        snapshot.materialBindings += materials.bindings;
                                        snapshot.materialMeshes += materials.meshes;
                                        snapshot.materialPreparations += materials.preparations;
                                        snapshot.materialStorageAllocations +=
                                            materials.storage_allocations;
                                        snapshot.materialStorageReusedViews += materials.reused_storage;
                                        snapshot.materialDenseCapacityBytesSum +=
                                            materials.dense_capacity_bytes;
                                        snapshot.materialRetainedBytesSum +=
                                            materials.retained_storage_bytes;
                                        if( materials.peak_storage_bytes >
                                            snapshot.materialPeakBytesMax )
                                        {
                                            snapshot.materialPeakBytesMax = materials.peak_storage_bytes;
                                        }
                                    }
                                }
                                pass.work.cullSharedBytes += storage.publication_bytes;
                                pass.work.cullEntryBytes += storage.entry_bytes;
                                pass.work.ancestorBytes += storage.ancestor_bytes;
                                pass.work.ancestorRefs += storage.ancestor_reference_count;
                                pass.work.ancestorAllocs += storage.ancestor_allocations;
                                pass.work.cullGroupBytes += storage.group_bytes;
                                pass.work.cullMapBytes += storage.map_bytes;
                                pass.work.cullViewBytes += storage.view_index_bytes;
                                pass.work.cullViewPeakSum += storage.view_array_peak_bytes;
                                pass.work.cullClassScratch += storage.classification_scratch_bytes;
                                pass.work.spatialViews += storage.group_mode == 2;
                                pass.work.blockViews += storage.group_mode == 1;
                                pass.work.spatialMapped += storage.mapped_candidates;
                                pass.work.spatialIndividual += storage.individual_candidates;
                                ++pass.work.views;
                                pass.work.candidates += candidates;
                                pass.work.visible += visible;
                                pass.work.dispatches += clawScene->getPreparedTransformDispatchCount();
                                pass.work.multiplies += transforms.matrix_multiply_count;
                                pass.work.hashProbes += transforms.capture_hash_probes;
                                pass.work.hashCollisions += transforms.capture_hash_collisions;
                                pass.work.hashFallbacks += transforms.capture_hash_fallbacks;
                                pass.work.scratchAllocations += transforms.capture_scratch_allocations;
                                pass.work.scratchReuses += transforms.capture_scratch_reuses;
                                pass.work.pathAllocations += transforms.capture_path_allocations;
                                pass.work.pathReuses += transforms.capture_path_reuses;
                                pass.work.indexAllocations += transforms.capture_index_allocations;
                                pass.work.indexReuses += transforms.capture_index_reuses;
                                pass.work.batchAllocations += transforms.capture_batch_allocations;
                                pass.work.batchReuses += transforms.capture_batch_reuses;
                                pass.work.localAllocations += transforms.capture_local_allocations;
                                pass.work.localReuses += transforms.capture_local_reuses;
                                pass.work.prefixAllocations += transforms.capture_prefix_allocations;
                                pass.work.prefixReuses += transforms.capture_prefix_reuses;
                                pass.work.groupTests += bounds.group_tests;
                                pass.work.groupRejects += bounds.group_rejections;
                                pass.work.groupSkipped += bounds.group_skipped_candidates;
                                pass.work.groupFallbacks += bounds.group_fallbacks;
                                pass.work.groupInside += bounds.group_inside;
                                pass.work.groupInsideCandidates += bounds.group_inside_candidates;
                                pass.work.sphereTests += bounds.sphere_tests;
                                pass.work.sphereRejects += bounds.sphere_rejections;
                                pass.work.boxTests += bounds.box_tests;
                                pass.work.satTests += bounds.sat_tests;
                                pass.work.satRejects += bounds.sat_rejections;
                                pass.work.satAxes += bounds.sat_axis_tests;
                                pass.work.satFallbacks += bounds.sat_fallbacks;
                            }
                        }
                    }
                    renderer->setRenderTarget( pass.target );
                    renderer->setViewport( viewport );
                    if( auto renderer3 = dynamic_pointer_cast<IRenderer3>( renderer ) )
                    {
                        renderer3->setCamera( viewport ? viewport->getCamera() : nullptr );
                    }
                    break;
                case WP_GRAPHICS_FRAME_BEGIN_TARGET:
                    renderer->beginRender();
                    if( (pass.flags & WP_GRAPHICS_FRAME_TARGET_WINDOW) != 0 )
                    {
                        if( auto dx11 = dynamic_pointer_cast<ClawRendererDX11>( renderer ) )
                        {
                            auto native = wp_renderer_get_dx11( dx11->getNativeRenderer() );
                            if( pass.flags & WP_GRAPHICS_FRAME_TARGET_PRESERVE_WINDOW )
                            {
                                const auto colour = viewport->getBackgroundColour();
                                const wp_vec4f background{ colour.r, colour.g, colour.b, colour.a };
                                const auto camera = viewport->getCamera();
                                const auto key = static_cast<wp_u64>( reinterpret_cast<uintptr_t>( camera.get() ) );
                                if( !wp_renderer_dx11_restore_window_history( native, key, &background ) )
                                    return WP_GRAPHICS_FRAME_FAILED;
                            }
                            else
                            {
                                wp_renderer_dx11_discard_window_history( native );
                            }
                        }
                    }
                    break;
                case WP_GRAPHICS_FRAME_CLEAR_TARGET:
                    renderer->clear( viewport ? viewport->getBackgroundColour() : ColourF::Black );
                    break;
                case WP_GRAPHICS_FRAME_BEGIN_EFFECTS:
                    return owner->beginGraphicsPipelineFrame(
                               renderer, (pass.flags & WP_GRAPHICS_FRAME_TARGET_PRESERVE_WINDOW) != 0 ) ?
                               WP_GRAPHICS_FRAME_OK : WP_GRAPHICS_FRAME_SKIP;
                case WP_GRAPHICS_FRAME_DRAW_SCENE:
                {
                    auto &active = activeDrawPass();
                    struct DrawPassScope
                    {
                        FramePassContext *&current;
                        FramePassContext *previous;
                        ~DrawPassScope() { current = previous; }
                    } scope{ active, active };
                    active = &pass;
                    RenderPhaseProfile timing( owner->m_sceneProfile, "Scene draw" );
                    owner->render();
                    if( viewport && viewport->getCamera() )
                    {
                        auto debug = dynamic_pointer_cast<ClawDebug>( owner->getDebug() );
                        auto renderer3 = dynamic_pointer_cast<IRenderer3>( renderer );
                        if( debug && renderer3 )
                        {
                            debug->renderLines( *renderer3 );
                        }
                    }
                    break;
                }
                case WP_GRAPHICS_FRAME_END_EFFECTS:
                    owner->endGraphicsPipelineFrame( renderer );
                    break;
                case WP_GRAPHICS_FRAME_DRAW_CAMERA_EFFECTS:
                    if( auto dx11 = dynamic_pointer_cast<ClawRendererDX11>( renderer );
                        dx11 && !dx11->applyCameraEffects( pass.postProcess, pass.compositeLayers ) )
                    {
                        return WP_GRAPHICS_FRAME_FAILED;
                    }
                    break;
                case WP_GRAPHICS_FRAME_DRAW_UI:
                    if( auto app = core::IApplicationManager::instancePtr() )
                    {
                        if( auto renderUI = app->getRenderUI(); renderUI && renderUI->isLoaded() )
                        {
                            if( auto nativeUI = dynamic_pointer_cast<ui::ClawUIManager>( renderUI ) )
                            {
                                nativeUI->renderViewport(
                                    !owner->m_frameSimulationEnabled,
                                    ( pass.flags & WP_GRAPHICS_FRAME_TARGET_UI ) != 0,
                                    ( pass.flags & WP_GRAPHICS_FRAME_TARGET_OVERLAY ) != 0 );
                            }
                            else if( owner->m_frameSimulationEnabled &&
                                     ( pass.flags & WP_GRAPHICS_FRAME_TARGET_UI ) )
                            {
                                renderUI->render();
                            }
                            // Unknown game UI backends have no read-only input contract.
                        }
                    }
                    break;
                case WP_GRAPHICS_FRAME_DRAW_OVERLAY:
                    if( pass.flags & WP_GRAPHICS_FRAME_TARGET_PRESERVE_WINDOW )
                    {
                        if( auto dx11 = dynamic_pointer_cast<ClawRendererDX11>( renderer ) )
                            wp_renderer_dx11_save_window_history( wp_renderer_get_dx11( dx11->getNativeRenderer() ) );
                    }
                    owner->drawImGuiOverlay();
                    break;
                case WP_GRAPHICS_FRAME_RETIRE_SCENES:
                    for( auto &scene : pass.preparedScenes )
                    {
                        scene->discardRenderJobs();
                    }
                    pass.preparedScenes.clear();
                    break;
                case WP_GRAPHICS_FRAME_END_TARGET:
                {
                    RenderPhaseProfile timing( owner->m_presentationProfile, "Presentation" );
                    renderer->endRender();
                    break;
                }
                case WP_GRAPHICS_FRAME_RESTORE_TARGET:
                    if( pass.rttCaptured )
                    {
                        pass.rttCaptured = false;
                        auto previousTarget = std::move( pass.previousTarget );
                        auto previousViewport = std::move( pass.previousViewport );
                        auto previousCamera = std::move( pass.previousCamera );
                        renderer->setRenderTarget( previousTarget );
                        renderer->setViewport( previousViewport );
                        if( auto renderer3 = dynamic_pointer_cast<IRenderer3>( renderer ) )
                        {
                            renderer3->setCamera( previousCamera );
                        }
                    }
                    break;
                }
                return WP_GRAPHICS_FRAME_OK;
            }
            catch( const std::exception &e )
            {
                try
                {
                    WP_LOG_EXCEPTION( e );
                }
                catch( ... )
                {
                }
            }
            catch( ... )
            {
                try
                {
                    WP_LOG_ERROR( "Unknown exception in graphics frame adapter." );
                }
                catch( ... )
                {
                }
            }
            return WP_GRAPHICS_FRAME_FAILED;
        }

        wp_rtt_result ClawHammerSystem::FramePassContext::transact( void *data,
            wp_rtt_check_func draw, void *drawData )
        {
            auto &pass = *static_cast<FramePassContext *>( data );
            try
            {
                auto renderer = dynamic_pointer_cast<ClawRendererDX11>( pass.renderer );
                auto target = dynamic_pointer_cast<ClawRenderTarget>( pass.target );
                auto nativeRenderer = renderer->getNativeRenderer();
                auto dx11 = wp_renderer_get_dx11( nativeRenderer );
                auto output = target->getNativeRenderTexture( nativeRenderer );
                const auto size = target->getSize();
                const auto colour = pass.viewport ? pass.viewport->getBackgroundColour()
                                                  : ColourF::Black;
                wp_graphics_rtt_desc desc{};
                desc.renderer_generation = wp_renderer_dx11_get_rtt_generation( dx11 );
                desc.target_generation = wp_renderer_dx11_get_render_texture_generation( output );
                desc.width = size.x;
                desc.height = size.y;
                desc.viewport = wp_renderer_dx11_get_viewport( dx11 );
                desc.clear_colour = desc.clear_depth = (pass.flags & WP_GRAPHICS_FRAME_TARGET_CLEAR) != 0;
                desc.colour[0] = colour.r;
                desc.colour[1] = colour.g;
                desc.colour[2] = colour.b;
                desc.colour[3] = colour.a;
                desc.depth = 1.0f;

                struct RttContext
                {
                    FramePassContext &pass;
                    wp_rtt_check_func draw;
                    void *drawData;
                } context{ pass, draw, drawData };
                const auto validate = []( void *opaque, const wp_graphics_rtt_desc *expected ) {
                    auto &pass = static_cast<RttContext *>( opaque )->pass;
                    try
                    {
                        const auto actual = pass.target->getSize();
                        if( actual.x != expected->width || actual.y != expected->height )
                        {
                            return WP_RTT_STALE;
                        }
                        for( const auto &scene : pass.preparedScenes )
                        {
                            if( !scene->validateRenderJobs() )
                            {
                                return WP_RTT_STALE;
                            }
                        }
                        return WP_RTT_OK;
                    }
                    catch( ... )
                    {
                        return WP_RTT_BACKEND_FAILED;
                    }
                };
                const auto result = wp_renderer_dx11_execute_rtt_job(
                    dx11, output, &desc, nullptr, validate,
                    []( void *opaque, const wp_graphics_rtt_desc *descriptor ) {
                        auto &context = *static_cast<RttContext *>( opaque );
                        return context.draw( context.drawData, descriptor );
                    }, &context );
                if( result != WP_RTT_OK )
                {
                    if( pass.manualUpdate ) target->update();
                    WP_LOG_ERROR( "Native offscreen job failed: " + std::to_string( result ) );
                }
                return result;
            }
            catch( ... )
            {
                try
                {
                    WP_LOG_ERROR( "Failed to prepare the native offscreen transaction." );
                }
                catch( ... )
                {
                }
                return WP_RTT_BACKEND_FAILED;
            }
        }

        void ClawHammerSystem::render()
        {
            GraphicsSystem::render();

            if( auto *pass = FramePassContext::activeDrawPass(); pass && pass->owner == this )
            {
                struct Submission
                {
                    FramePassContext &pass;
                    std::exception_ptr error;
                } submission{ *pass, {} };
                const auto submit = []( size_t index, void *opaque ) -> wp_s32 {
                    auto &submission = *static_cast<Submission *>( opaque );
                    try
                    {
                        auto &pass = submission.pass;
                        pass.preparedScenes[index]->render( pass.renderer.get() );
                        return 1;
                    }
                    catch( ... )
                    {
                        submission.error = std::current_exception();
                        return 0;
                    }
                };
                if( !wp_graphics_system_submit_frame_scenes(
                        m_sys, pass->preparedScenes.size(), submit, &submission ) )
                {
                    if( submission.error ) std::rethrow_exception( submission.error );
                    throw std::runtime_error( "Native prepared scene submission was rejected." );
                }
                return;
            }

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

        bool ClawHammerSystem::beginGraphicsPipelineFrame( SmartPtr<IRenderer> renderer, bool preserveContents )
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
                if( !renderer->getCamera() )
                {
                    return false;
                }
                m_graphicsPipeline->setCamera( renderer->getCamera().get() );
                m_graphicsPipeline->setScene( renderer->getCamera()->getCreator().get() );
                const auto settings = m_frameUpdateActive && m_framePipelineSettings
                                            ? m_framePipelineSettings
                                            : m_graphicsPipeline->getRenderSettings();
                const auto &options = *settings;
                const bool legacyHdr = renderer->getRenderTarget() == getDefaultWindow() &&
                                       options.legacyToneMapping;
                const bool bloom = options.bloom;
                const bool motionBlur = options.motionBlur;
                const bool ao = options.ambientOcclusion;
                const s32 aa = options.antiAliasing;
                const s32 pipeline = options.renderPipeline;
                const bool localLights = options.localLights;
                const f32 scale = options.renderScale * m_adaptiveSceneScale;

                // Scene textures used display-space output before project colour controls.
                // Scaling, anti-aliasing and effects can use a resolve without changing that policy.
                const auto &output = options.output[legacyHdr ? 1 : 0];
                const bool hdr = output.hdr;
                const s32 linear = output.colorSpace;
                const s32 tone = output.tonemapping;
                const bool resolve = hdr || linear != 0 || scale != 1.0f || aa != 0 || bloom ||
                                     motionBlur || ao || localLights;
                auto native = wp_renderer_get_dx11( dx11Renderer->getNativeRenderer() );
                // HDR staging clears its own colour and depth. Accumulation needs
                // the viewport's retained colour/depth, so draw directly in that case.
                if( preserveContents || !resolve )
                {
                    // A legacy project may follow one with an explicit GPU profile.
                    // Its direct draw path must not inherit that project's light grid
                    // or pipeline state merely because it bypasses the HDR resolve.
                    wp_renderer_dx11_set_scene_sampling( native, 1, 1.f );
                    wp_renderer_dx11_set_pipeline_mode( native, 0, 0 );
                    wp_renderer_dx11_set_post_options( native, 0, 0, 0, 0 );
                    wp_renderer_dx11_set_local_lights(
                        native, nullptr, 0, nullptr, nullptr, 0, 1, 1, 1, .1f, 1000.f, nullptr );
                    return false;
                }
                const s32 samples = aa == 3 ? 2 : aa == 4 ? 4 : aa == 5 ? 8 : 1;
                wp_renderer_dx11_set_scene_sampling( native, samples, scale );
                wp_renderer_dx11_set_pipeline_mode( native, pipeline, ao );
                if( localLights || legacyHdr )
                    uploadProjectSceneLights( native, renderer->getCamera()->getCreator(),
                                              renderer->getCamera(), options );
                else
                    wp_renderer_dx11_set_local_lights(
                        native, nullptr, 0, nullptr, nullptr, 0, 1, 1, 1, .1f, 1000.f, nullptr );
                wp_renderer_dx11_set_post_options(
                    native, aa, bloom, motionBlur,
                    static_cast<wp_u64>( reinterpret_cast<uintptr_t>( renderer->getCamera().get() ) ) );
                wp_renderer_dx11_set_output_options( native, hdr, linear, tone );
                return wp_renderer_dx11_begin_hdr_scene( native, 1.0f ) != 0;
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
            const auto softwareRenderer = dynamic_pointer_cast<ClawRendererSoftware>( renderer );
            const auto nativeRenderer =
                softwareRenderer ? softwareRenderer->getNativeRenderer() : nullptr;
            const auto width =
                nativeRenderer ? wp_renderer_get_width( nativeRenderer ) : static_cast<s32>( size.x );
            const auto height =
                nativeRenderer ? wp_renderer_get_height( nativeRenderer ) : static_cast<s32>( size.y );
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
                if( auto scene = dynamic_cast<ClawScene *>( m_graphicsPipeline->getScene() ) )
                {
                    scene->captureOcclusionDepth( dx11Renderer->getNativeRenderer(),
                                                  renderer->getCamera() );
                }
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
            if( auto scene = dynamic_cast<ClawScene *>( m_graphicsPipeline->getScene() ) )
            {
                scene->captureOcclusionDepth( softwareNativeRenderer, renderer->getCamera() );
            }
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
                    SmartPtr<Properties> projectProfile;
                    if( auto app = core::IApplicationManager::instancePtr() )
                    {
                        projectProfile = app->getPlayerSettings();
                    }
                    if( projectProfile )
                    {
                        String error, notes;
                        if( !validateProjectGraphicsProviders( projectProfile, error, notes ) )
                        {
                            WP_LOG_ERROR( "WPGraphics: " + error );
                            return false;
                        }
                        if( !notes.empty() )
                        {
                            WP_LOG_INFO( "WPGraphics: " + notes );
                        }
                        const auto api = projectRendererApi( projectProfile );
                        if( api == "Vulkan" )
                        {
                            selectedApi = RenderApi::Vulkan;
                        }
                        else if( api == "OpenGL" )
                        {
                            selectedApi = RenderApi::GL;
                        }
                        else if( api == "OpenGLES" )
                        {
                            selectedApi = RenderApi::GL;
                        }
                        else if( api == "Metal" )
                        {
                            selectedApi = RenderApi::Metal;
                        }
                        else if( projectProfile->hasProperty( "graphics.renderer.pc" ) &&
                                 projectProfile->getPropertyAsInt( "graphics.renderer.pc" ) != 0 )
                        {
                            selectedApi = api == "DX12"       ? RenderApi::DX12
                                          : api == "Software" ? RenderApi::Software
                                                              : RenderApi::DX11;
                        }
                    }

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

                    const bool waitingForWindow =
                        !createWindow && !getDefaultWindow() && selectedApi != RenderApi::Software;
                    if( selectedApi != RenderApi::None )
                    {
                        m_configuredRendererType = selectedApi;
                    }
                    if( waitingForWindow )
                    {
                        selectedApi = RenderApi::Software;
                    }

                    // A registered API provider owns the complete selected profile.
                    const auto selectedName = projectProfile ? projectRendererApi( projectProfile )
                                                             : String( renderApiName( selectedApi ) );
                    if( auto provider =
                            waitingForWindow ? nullptr : getProjectRendererProvider( selectedName ) )
                    {
                        String error;
                        renderer = provider->create( projectProfile, error );
                        if( !renderer )
                        {
                            WP_LOG_ERROR( "WPGraphics provider: " + error );
                            return false;
                        }
                    }
                    if( !renderer )
                    {
                        switch( selectedApi )
                        {
#if defined WP_PLATFORM_WIN32
                        case RenderApi::DX12:
                        {
                            String error;
                            if( auto provider = getProjectRendererProvider( "DX12" ) )
                            {
                                renderer = provider->create( projectProfile, error );
                            }
                            else
                            {
                                renderer = workphone::make_ptr<ClawRendererDX11>( true );
                            }
                            WP_LOG_INFO(
                                "WPGraphics: Direct3D 12 executes the shared scene pipeline through "
                                "D3D11On12." );
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
                            String error;
                            const auto api = projectProfile ? projectRendererApi( projectProfile )
                                                            : String( renderApiName( selectedApi ) );
                            if( auto provider = getProjectRendererProvider( api ) )
                            {
                                renderer = provider->create( projectProfile, error );
                            }
                            if( !renderer )
                            {
                                WP_LOG_ERROR( "WPGraphics: renderer provider failed for " + api + ": " +
                                              error );
                                return false;
                            }
                            break;
                        }
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
                if( auto app = core::IApplicationManager::instancePtr() )
                {
                    if( auto settings = app->getPlayerSettings() )
                    {
                        setProperties( settings );
                        m_graphicsPipeline->setProperties( settings );
                    }
                }
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
                SmartPtr<Properties> projectProfile;
                if( auto app = core::IApplicationManager::instancePtr() )
                {
                    projectProfile = app->getPlayerSettings();
                }
                const auto providerName = projectProfile ? projectRendererApi( projectProfile )
                                                         : String( renderApiName( targetApi ) );
                if( auto provider = getProjectRendererProvider( providerName ) )
                {
                    String error;
                    newRenderer = provider->create( projectProfile, error );
                    if( !newRenderer )
                    {
                        WP_LOG_ERROR( "WPGraphics provider: " + error );
                        return nullptr;
                    }
                }

                // Prefer DX12 if configured and available, otherwise DX11
                if( !newRenderer && targetApi == RenderApi::DX12 )
                {
                    newRenderer = workphone::make_ptr<ClawRendererDX11>( true );
                }
                else if( !newRenderer && targetApi == RenderApi::DX11 )
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
                            m_renderApi =
                                targetApi == RenderApi::DX12 ? RenderApi::DX12 : RenderApi::DX11;
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

            ScopedLock lock( this );
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
            ScopedLock lock( this );
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
