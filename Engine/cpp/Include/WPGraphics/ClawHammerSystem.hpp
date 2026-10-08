#ifndef GraphicsSystemClaw_h__
#define GraphicsSystemClaw_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WPGraphics/ClawCapabilities.hpp>
#include <Workphone/Graphics/GraphicsSystem.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/Graphics/IGraphicsPipeline.hpp>
#include <atomic>

#include <mutex>

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
            /** Native C89 frame-driver callback. */
            static void renderFrameCallback( wp_graphics_system *system, f32 deltaTime,
                                             void *userData );

            /** Render all C++ targets for one native frame-driver invocation. */
            void renderFrame( f32 deltaTime );

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
            void publishRenderStatistics();
            mutable std::mutex m_statisticsMutex;
            String m_renderStatistics;
            std::atomic<bool> m_statisticsResetRequested{ false };
            std::atomic<bool> m_statisticsSnapshotRequested{ false };
        };
    }  // namespace render
}  // namespace workphone

#endif  // GraphicsSystemClaw_h__
