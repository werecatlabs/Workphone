#ifndef GraphicsSystemClaw_h__
#define GraphicsSystemClaw_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WPGraphics/ClawCapabilities.hpp>
#include <Workphone/Graphics/GraphicsSystem.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/Graphics/IGraphicsPipeline.hpp>
#include <atomic>

#include <mutex>
#include <vector>

namespace workphone
{
    namespace ui
    {
        class WPImGui;
    }

    namespace render
    {
        class ClawImguiManager;

        /**
         * @class ClawHammerSystem
         * @brief Implementation of the graphics system using the ClawHammer backend.
         *
         * This class manages the lifecycle and execution of the native ClawHammer graphics system,
         * providing an interface between the engine's graphics abstraction and the native API.
         * Supports runtime selection between Software, DX11, and DX12 renderers.
         */
        class WPGraphics_API ClawHammerSystem : public GraphicsSystem
        {
        public:
            /**
             * @class StateListener
             * @brief Listener for handling state changes within the ClawHammerSystem.
             */
            class StateListener : public IStateListener
            {
            public:
                StateListener();

                ~StateListener() override;

                /**
                 * @brief Handles state changes.
                 * @param state The new state.
                 * @return True if the state change was handled.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Handles state messages.
                 * @param message The state message.
                 * @return True if the message was handled.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Gets the owner of this listener.
                 * @return Smart pointer to the ClawHammerSystem owner.
                 */
                SmartPtr<ClawHammerSystem> getOwner() const;

                /**
                 * @brief Sets the owner of this listener.
                 * @param owner Smart pointer to the ClawHammerSystem owner.
                 */
                void setOwner( SmartPtr<ClawHammerSystem> owner );

            protected:
                /** Weak pointer to the owning system to avoid circular references. */
                AtomicWeakPtr<ClawHammerSystem> m_owner;
            };

            ClawHammerSystem();

            ~ClawHammerSystem() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void update() override;
            void render() override;
            void messagePump() override;

            /**
             * @brief Configures the graphics system.
             *
             * The native ClawHammer system is created during load(). This override applies
             * backend-agnostic defaults when no settings are supplied (the engine calls
             * configure(nullptr)) so the graphics lifecycle can complete: it provisions a
             * default render window, a resource group manager and a debug helper.
             *
             * @param config Optional graphics settings. When null, defaults are applied.
             * @return True if configuration succeeded.
             */
            bool configure( SmartPtr<IBuildDirector> config ) override;

            SmartPtr<IGraphicsWindow> createRenderWindow(
                const String &name, u32 width, u32 height, bool fullScreen,
                const SmartPtr<Properties> &properties = nullptr ) override;

            SmartPtr<IGraphicsScene> addGraphicsScene( const String &type, const String &name ) override;
            void removeGraphicsScene( SmartPtr<IGraphicsScene> scene ) override;
            void removeAllGraphicsScenes() override;

            void setupRenderer( SmartPtr<IGraphicsScene> sceneManager, SmartPtr<IGraphicsWindow> window,
                                SmartPtr<IGraphicsCamera> camera, String workspaceName,
                                bool enabled ) override;

            /**
             * @brief Get the native ClawHammer graphics system pointer.
             * @return Raw pointer to the native system.
             */
            wp_graphics_system *getNativeSystem() const;

            /** Get the C89-backed post-processing pipeline owned by this system. */
            SmartPtr<IGraphicsPipeline> getGraphicsPipeline() const override;

            bool handleStateChanged( SmartPtr<IState> &state );

            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Gets properties for the game editor.
             * @return Properties object containing system settings.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Sets properties from the game editor.
             * @param properties Properties to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** Current presentation setting, readable from the render thread. */
            bool getVSync() const;

            /** Opt-in numeric telemetry for one completed graphics-owner update.
             * Registered scenes are counted at frame start. Scene/view counts describe
             * preparation pairs, not distinct scene owners or submitted shadow draws.
             * Cull view bytes describe main-view indices. Publication byte sums may
             * count a reused workspace in several views; they are not deduplicated
             * heap residency. Material peak bytes are the largest workspace high-water
             * mark observed in this frame. Failed passes remain visible in diagnostics.
             * Cache and DX11 lifetime counters restart with their existing owners or
             * statistics resets. No hardware/front-cache hit counter is available.
             */
            struct SceneProfileSnapshot
            {
                u64 serial = 0, generation = 0;
                u32 nativeUpdateSerial = 0, nativeRenderSerial = 0;
                u64 completedPasses = 0, skippedPasses = 0, failedPasses = 0;
                u64 registeredScenes = 0, scenePreparations = 0, mainViews = 0, shadowViews = 0;
                // Graphics update includes native rendering/Present, not the whole
                // application's physics/game update. Preparation timings are nested.
                u64 graphicsUpdateNs = 0, graphicsPrepareNs = 0, nativeFrameNs = 0;
                u64 preparationTotalNs = 0, mainPrepareNs = 0, shadowDeriveNs = 0, materialPrepareNs = 0;
                u64 candidates = 0, mainVisible = 0, transformDispatches = 0;
                u64 cullPublicationBytesSum = 0, cullViewBytesSum = 0;
                u64 cullCaptureNs = 0, cullTransformNs = 0, cullBoundsNs = 0;
                u64 cullGroupsNs = 0, cullViewNs = 0, cullVisibilityNs = 0, cullSortNs = 0, cullPublishNs = 0;
                u64 materialRecords = 0, materialBindings = 0, materialMeshes = 0, materialPreparations = 0;
                u64 materialStorageAllocations = 0, materialStorageReusedViews = 0;
                u64 materialDenseCapacityBytesSum = 0, materialRetainedBytesSum = 0, materialPeakBytesMax = 0;
                // One renderer-owned persistent table, charged once after the frame.
                u64 cachePreparationsLifetime = 0, cacheStorageAllocationsLifetime = 0;
                u64 directStorageAllocationsLifetime = 0, cacheStorageBytes = 0;
                u64 cacheSlots = 0, cacheResident = 0;
                bool dx11Available = false;
                u64 dx11FramesLifetime = 0, dx11DrawsLifetime = 0, dx11TrianglesLifetime = 0;
                u64 dx11MaterialUploadsLifetime = 0, dx11TransformUploadsLifetime = 0;
                u64 dx11StateBindingsLifetime = 0, dx11InstancesLifetime = 0, dx11GpuSamplesLifetime = 0;
                u64 dx11PresentAttemptsLifetime = 0, dx11PresentOccludedLifetime = 0, dx11PresentFailuresLifetime = 0;
                f64 dx11IntervalMs = 0, dx11P95IntervalMs = 0, dx11CpuFrameMs = 0;
                f64 dx11PresentMs = 0, dx11GpuFrameMs = 0;
            };

            /** Thread-safe controls/readback. Disabled by default; disabled frames
             * perform no additional timing or collection. Changes/reset immediately
             * clear readback and reject publications already in flight. Reset does
             * not alter renderer statistics, material caches or normal rendering.
             * Read returns false and a zeroed value until a new update completes.
             */
            void setSceneProfilingEnabled( bool enabled );
            bool getSceneProfilingEnabled() const;
            void resetSceneProfile();
            bool readSceneProfile( SceneProfileSnapshot &snapshot ) const;

            /**
             * @brief Set the renderer type to use.
             *
             * Call this before configure() to select which renderer backend to use.
             * Valid options: RenderApi::DX11, RenderApi::DX12, RenderApi::None (software)
             *
             * @param api The renderer API to use
             */
            void setRendererType( RenderApi api ) override;

            /**
             * @brief Get the currently configured renderer type.
             *
             * @return The renderer API type
             */
            RenderApi getRendererType() const;
            ClawCapabilities getCapabilities() const;

            /**
             * @brief Switch to a different renderer at runtime.
             *
             * This will destroy the current renderer and create a new one of the specified type.
             * Use with caution as this may invalidate renderer-specific resources.
             *
             * @param api The new renderer API to switch to
             * @return True if the renderer was successfully switched
             */
            bool switchRenderer( RenderApi api );

            WP_CLASS_REGISTER_DECL;

        protected:
            struct FramePassContext;
            void prepareFramePasses( f32 deltaTime );
            static s32 beginFramePass( wp_graphics_system *, f32, void * );
            static void drawFramePass( wp_graphics_system *, f32, void * );
            static void effectsFramePass( wp_graphics_system *, f32, void * );
            static void uiFramePass( wp_graphics_system *, f32, void * );
            static void overlayFramePass( wp_graphics_system *, f32, void * );
            static void endFramePass( wp_graphics_system *, f32, void * );
            static void executeFramePassStage( wp_graphics_system *, f32, void *, unsigned stage );
            std::vector<FramePassContext> m_framePasses;

            /**
             * @brief Prepare the graphics pipeline for the renderer's current viewport.
             * @return True when a pipeline frame was started and must be completed.
             */
            bool beginGraphicsPipelineFrame( SmartPtr<IRenderer> renderer );

            /**
             * @brief Process and present the current renderer frame through the pipeline.
             *
             * This must only be called after beginGraphicsPipelineFrame() succeeds.
             */
            void endGraphicsPipelineFrame( SmartPtr<IRenderer> renderer );

            /** Raw pointer to the native ClawHammer system. */
            wp_graphics_system *m_sys;

            // Plugin handle for the WPImGui UI module. Held to keep the module
            // alive for the lifetime of this manager.
            SmartPtr<ui::WPImGui> m_plugin;

            ///< ImGui manager for rendering Dear ImGui UI
            ///< within the graphics system.
            SmartPtr<ClawImguiManager> m_imguiManager;

            ///< High-level C++17 adapter around the native WorkphoneGraphics pipeline.
            SmartPtr<IGraphicsPipeline> m_graphicsPipeline;

            ///< Configured renderer type (selected before configure())
            RenderApi m_configuredRendererType = RenderApi::None;
            std::atomic<bool> m_vsync{ false };
            std::atomic<bool> m_gpuProfilerEnabled{ true };
            // Cached timings must be released with this graphics system, before
            // the application destroys its profiler and allocation pools.
            SmartPtr<IProfile> m_preparationProfile;
            SmartPtr<IProfile> m_sceneProfile;
            SmartPtr<IProfile> m_uiBuildProfile;
            SmartPtr<IProfile> m_uiSubmitProfile;
            SmartPtr<IProfile> m_presentationProfile;
            void publishRenderStatistics();
            mutable std::mutex m_statisticsMutex;
            u32 m_nativeUpdateSerial = 0;
            u32 m_nativeRenderSerial = 0;
            u32 m_nativeCompletedPasses = 0;
            u32 m_nativeSkippedPasses = 0;
            String m_renderStatistics;
            std::atomic<bool> m_statisticsResetRequested{ false };
            std::atomic<bool> m_statisticsSnapshotRequested{ false };

            void beginSceneProfileFrame();
            void publishSceneProfileFrame();
            void invalidateSceneProfileFrame();
            std::atomic<bool> m_sceneProfilingEnabled{ false };
            mutable std::mutex m_sceneProfileMutex;
            SceneProfileSnapshot m_publishedSceneProfile;
            // The current frame is render-owner-only. Readers never inspect it.
            SceneProfileSnapshot m_currentSceneProfile;
            u64 m_sceneProfileGeneration = 0, m_sceneProfileSequence = 0;
            bool m_sceneProfileFrameActive = false;
        };
    }  // namespace render
}  // namespace workphone

#endif  // GraphicsSystemClaw_h__
