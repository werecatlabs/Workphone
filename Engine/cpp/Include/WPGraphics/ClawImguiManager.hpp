#pragma once

#include <WPGraphics/WPClawHammerPCH.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <chrono>

// Forward declarations keep this header light-weight. The full ImGui and
// renderer types are only needed by the .cpp; consumers of the manager do not
// pay for them.
struct wp_renderer;
struct ImGuiContext;  // documented for readers; the .cpp uses ImGui::GetCurrentContext()

namespace workphone
{
    namespace ui
    {
        class WPImGui;
    }

    namespace render
    {
        /// Manages a Dear ImGui context bound to a Workphone C89 renderer
        /// (wp_renderer). Lifetime is driven by init()/shutdown(); the manager
        /// is also an ISharedObject plugin that can be load()/unload()ed by
        /// the engine's plugin system.
        class ClawImguiManager : public ISharedObject
        {
        public:
            static ClawImguiManager *getSingletonPtr();
            static ClawImguiManager &getSingleton();

            ClawImguiManager();
            ~ClawImguiManager();

            /// @name ISharedObject lifecycle (plugin system).
            /// load() constructs the WPImGui UI module; unload() releases it
            /// and tears down the ImGui context.
            /// @{
            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            /// Tears down the ImGui context and releases the renderer binding.
            /// Idempotent and re-entrancy-safe.
            void shutdown();

            /// Begins a new ImGui frame. Must be called between the host
            /// beginRender/endRender and before render().
            void newFrame();

            /// Renders the queued ImGui draw data into the bound renderer.
            /// No-op if no frame is in progress or the context is gone.
            void render();

            /// Bakes the font atlas and uploads it to the renderer. Called
            /// once during init(); call again after adding fonts to rebuild.
            void createFontTexture();

            /// True when a renderer is bound and an ImGui context exists.
            bool isInitialised() const;

            /// Updates the cached display/projection dimensions.
            void setupViewport( s32 width, s32 height );

            wp_renderer *getRenderer() const;

            void setRenderer( wp_renderer *renderer );
            /// @}

            WP_CLASS_REGISTER_DECL;

        private:
            /// Binds the manager to a renderer and creates the ImGui context.
            /// Idempotent for the same renderer + live context; tears down and
            /// rebuilds when binding to a different renderer or recovering
            /// from a lost context. Safe to call with a null renderer (logs
            /// and returns).
            void init( wp_renderer *renderer );

            void updateProjectionMatrix( float width, float height );

            static ClawImguiManager *ms_singleton;

            wp_renderer *m_renderer = nullptr;

            // ImGui .ini/.log filenames. Stored as members so the C strings
            // handed to ImGui remain valid for the manager's lifetime.
            String m_iniPath;
            String m_logPath;

            // Cached display size used for both the projection and the
            // DisplaySize reported to ImGui.
            s32 m_vpWidth = 0;
            s32 m_vpHeight = 0;

            s32 m_lastRenderedFrame = -1;

            // Frame accounting: prevents double Render() within a single frame
            // and skips a re-render of the same ImGui frame.
            bool m_frameEnded = true;

            // Guards shutdown() against re-entrancy (destructor + unload +
            // init-teardown may overlap).
            bool m_shuttingDown = false;

            // True while this manager owns the live ImGui context. Cleared by
            // shutdown().
            bool m_contextOwner = false;

            // Clock used to drive io.DeltaTime when no platform backend is
            // bound (headless / render-to-texture). Per-instance so multiple
            // managers do not share a single static clock.
            std::chrono::high_resolution_clock::time_point m_lastHeadlessTime;
        };
    }  // namespace render
}  // namespace workphone
