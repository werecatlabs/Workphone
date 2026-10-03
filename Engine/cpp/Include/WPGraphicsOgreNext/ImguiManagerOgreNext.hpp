#pragma once

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPGraphicsOgreNext/Wrapper/CRenderer.hpp>
#include "imgui.h"
#include "ImguiRenderableOgreNext.hpp"
#include <OgrePrerequisites.h>

namespace workphone::render
{

    /**
     * @brief ImGui integration manager for Ogre Next.
     *
     * This class provides an adapter between Dear ImGui and the Ogre Next
     * rendering backend. It manages font atlas creation, material/shader
     * setup, multi-viewport callbacks and per-frame rendering of ImGui draw
     * lists. The class derives from CRenderer so it can be used like other
     * engine renderers.
     */
    class ImguiManagerOgreNext : public CRenderer
    {
    public:
        /**
         * @brief Construct an ImGui manager instance.
         *
         * Constructor performs minimal initialization. Call load() / init()
         * to prepare the manager for rendering.
         */
        ImguiManagerOgreNext();

        /**
         * @brief Destructor.
         *
         * Ensures resources are released by calling shutdown()/unload().
         */
        ~ImguiManagerOgreNext();

        /**
         * @brief Load internal plugin and resources.
         *
         * @param data Optional initialization data (unused).
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unload and free resources.
         *
         * @param data Optional parameter (unused).
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Initialize ImGui for use with a given scene manager.
         *
         * Must be called once before using ImGui functions. This sets up
         * fonts, materials, shaders and platform callbacks.
         *
         * @param mgr Pointer to the Ogre scene manager to use.
         */
        void init( Ogre::SceneManager *mgr );

        /**
         * @brief Returns true when init() has been called successfully.
         */
        bool isInitialised() const;

        /**
         * @brief Set the viewport used for display size and scissor calculations.
         *
         * @param viewport Ogre viewport to use. Passing nullptr clears the
         *                 stored viewport.
         */
        void setupViewport( Ogre::Viewport *viewport );

        /**
         * @brief Start a new ImGui frame.
         *
         * Call this before any ImGui calls each frame.
         */
        void newFrame();

        /**
         * @brief Render the ImGui draw lists for the current frame.
         *
         * Call this after ImGui::Render() has been prepared. The function
         * will convert ImGui draw lists into Ogre draw calls.
         */
        void render();

        /**
         * @brief Update the projection matrix used by ImGui shaders.
         *
         * @param width  Current render target width in pixels.
         * @param height Current render target height in pixels.
         */
        void updateProjectionMatrix( float width, float height );

        /**
         * @brief Shutdown immediate-mode renderer state and release helpers.
         *
         * Frees internal helper objects but does not unload the external
         * plugin pointer (m_plugin is unloaded in unload()).
         */
        void shutdown();

        /**
         * @brief Get singleton instance reference.
         *
         * The singleton accessor is provided to avoid multiple definitions
         * when the header is included across translation units. Implementation
         * instantiates the singleton on first use.
         */
        static ImguiManagerOgreNext &getSingleton( void );

        /**
         * @brief Get singleton instance pointer.
         *
         * Use this when a nullable pointer is required.
         */
        static ImguiManagerOgreNext *getSingletonPtr( void );

        WP_CLASS_REGISTER_DECL;

    private:
        /**
         * @brief Bake ImGui font atlas into an Ogre texture and upload it.
         */
        void createFontTexture();

        /**
         * @brief Create and configure the material and shader passes used by ImGui.
         */
        void createMaterial();

        SmartPtr<ui::WPImGui> m_plugin; /**< External ImGui plugin wrapper. */

        Ogre::FastArray<ImguiRenderableOgreNext *> m_renderables; /**< Per-draw-call renderables. */

        Ogre::PsoCacheHelper *m_psoCache; /**< PSO cache / helper used to build pipeline state. */

        Ogre::SceneManager *m_sceneMgr; /**< Scene manager used to query render system and resources. */

        Ogre::Pass *m_pass;                        /**< Material pass used for ImGui rendering. */
        Ogre::TextureGpu *m_fontTexture = nullptr; /**< Font atlas texture. */

        int m_lastRenderedFrame; /**< Last ImGui frame index rendered. */
        bool m_frameEnded;       /**< True when newFrame() has not been called for the current frame. */

        String m_iniPath; /**< Path to ImGui .ini file for layouts. */
        String m_logPath; /**< Path to ImGui log file. */

        Ogre::Viewport *m_vp; /**< Optional viewport used for scissor and size queries. */

        static ImguiManagerOgreNext *ms_singleton; /**< Static singleton instance. */
    };

}  // namespace workphone::render
