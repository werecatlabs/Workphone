#ifndef __CGraphicsSystemOgre_H
#define __CGraphicsSystemOgre_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/GraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <OgreWindowEventUtilities.h>
#include <OgreFrameListener.h>
#include <OgreRenderQueueListener.h>
#include <OgreRTShaderSystem.h>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/HashMap.hpp>
#include <Workphone/Core/Set.hpp>
#include <OgreBuildSettings.h>
#include <OgreComponents.h>
#include <OgreSGTechniqueResolverListener.h>

#define OGRE_USE_GLES2

#ifdef OGRE_STATIC_LIB
#    define OGRE_STATIC_GL
#    if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
//#    define OGRE_STATIC_Direct3D9
// dx10 will only work on vista, so be careful about statically linking
#        if OGRE_USE_D3D10
#            define OGRE_STATIC_Direct3D10
#        endif
#    endif
#    define OGRE_STATIC_CgProgramManager
#    ifdef OGRE_USE_PCZ
#        define OGRE_STATIC_PCZSceneManager
#        define OGRE_STATIC_OctreeZone
#    endif
#    if OGRE_VERSION >= 0x10800
#        if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
#            define OGRE_IS_IOS 1
#        endif
#    else
#        if OGRE_PLATFORM == OGRE_PLATFORM_IPHONE
#            define OGRE_IS_IOS 1
#        endif
#    endif
#    ifdef OGRE_IS_IOS
#        undef OGRE_STATIC_CgProgramManager
#        undef OGRE_STATIC_GL
//#    define OGRE_STATIC_GLES 1
#        ifdef OGRE_USE_GLES2
#            define OGRE_STATIC_GLES2 1
#            define INCLUDE_RTSHADER_SYSTEM
#            undef OGRE_STATIC_GLES
#        endif
#        ifdef __OBJC__
#            import <UIKit/UIKit.h>
#        endif
#    endif
#endif

namespace workphone
{
    namespace render
    {
        /**
         * @brief Ogre-based implementation of the graphics system.
         *
         * CGraphicsSystemOgre implements the engine GraphicsSystem using the Ogre rendering
         * backend. It manages the Ogre root, render systems, resource helpers, scene factories,
         * compositor manager and other Ogre-specific subsystems. The class is responsible for
         * constructing/destroying Ogre components and providing the IGraphicsSystem contract
         * to the rest of the engine.
         */
        class CGraphicsSystemOgre : public GraphicsSystem
        {
        public:
            /**
             * @brief Frame listener forwarding Ogre frame events to the graphics system.
             *
             * Handles per-frame callbacks from Ogre and drives update/tick logic inside
             * CGraphicsSystemOgre.
             */
            class AppFrameListener : public Ogre::FrameListener
            {
            public:
                /** @brief Construct with owning graphics system pointer. */
                AppFrameListener( CGraphicsSystemOgre *graphicsSystem );
                /** @brief Virtual destructor. */
                ~AppFrameListener() override;

                /** @brief Called after a frame has ended. */
                bool frameEnded( const Ogre::FrameEvent &evt ) override;
                /** @brief Called at the start of a frame. */
                bool frameStarted( const Ogre::FrameEvent &evt ) override;
                /** @brief Called just before rendering each frame. */
                bool frameRenderingQueued( const Ogre::FrameEvent &evt ) override;

            protected:
                CGraphicsSystemOgre *m_graphicsSystem =
                    nullptr;       /**< Owner graphics system (non-owning). */
                f32 m_time = 0.0f; /**< Accumulated time used for debug/updating. */
            };

            /**
             * @brief Render queue listener used to intercept render queue events.
             *
             * Primarily used to insert custom render behaviour or to manage overlays
             * and compositor passes before/after particular queue groups.
             */
            class RenderQueueListener : public Ogre::RenderQueueListener
            {
            public:
                RenderQueueListener( CGraphicsSystemOgre *graphicsSystem );
                ~RenderQueueListener() override;

                /**
                 * @brief Called when a render queue starts executing.
                 *
                 * @param queueGroupId Queue group identifier.
                 * @param invocation Invocation string.
                 * @param skipThisInvocation Out parameter to instruct Ogre to skip this invocation.
                 */
                void renderQueueStarted( Ogre::uint8 queueGroupId, const Ogre::String &invocation,
                                         bool &skipThisInvocation ) override;

                CGraphicsSystemOgre *m_graphicsSystem = nullptr; /**< Non-owning pointer to parent. */
            };

            /**
             * @brief Window event listener to detect window close and related events.
             *
             * This listener is registered with Ogre::WindowEventUtilities so the engine can
             * respond to platform window events such as close and resize.
             */
            class WindowEventListener : public Ogre::WindowEventListener
            {
            public:
                WindowEventListener();
                ~WindowEventListener() override;

                /**
                 * @brief Called when a window is closing.
                 * @param rw Pointer to the render window that is closing.
                 * @return True to allow close, false to cancel (engine-specific).
                 */
                bool windowClosing( Ogre::RenderWindow *rw ) override;
            };

            /**
             * @brief Technique resolver for the RTShader system.
             *
             * When the RTShader system is enabled this listener is responsible for creating
             * shader-based techniques when Ogre requests a scheme that is not present on a material.
             */
            class TechniqueResolverListener : public OgreBites::SGTechniqueResolverListener
            {
            public:
                TechniqueResolverListener( Ogre::RTShader::ShaderGenerator *pShaderGenerator );

                ~TechniqueResolverListener() override;

                /**
                 * @brief Handle missing technique scheme by creating an RTShader technique.
                 *
                 * @param schemeIndex Index of the scheme requested.
                 * @param schemeName Scheme name requested.
                 * @param originalMaterial Material lacking the requested scheme.
                 * @param lodIndex Level-of-detail index.
                 * @param rend Renderable requesting the technique.
                 * @return A newly created Ogre::Technique or nullptr if none created.
                 */
                Ogre::Technique *handleSchemeNotFound( unsigned short schemeIndex,
                                                       const Ogre::String &schemeName,
                                                       Ogre::Material *originalMaterial,
                                                       unsigned short lodIndex,
                                                       const Ogre::Renderable *rend ) override;
            };

            /// Names for supported render system plugins / backends.
            static const String rsDX12Name;
            static const String rsDX11Name;
            static const String rsGL3Name;
            static const String rsDX9Name;
            static const String rsGLName;
            static const String rsMetalName;
            static const String rsVulkanName;
            static const String rsNoneName;

            /** @brief Default constructor. Initializes members to safe defaults. */
            CGraphicsSystemOgre();

            /** @brief Destructor. Cleans up Ogre and owned subsystems. */
            ~CGraphicsSystemOgre() override;

            /**
             * @brief Handle a state change message.
             *
             * This method receives state change messages from the engine state system and
             * applies any graphics-related responses (e.g. toggling debug, reloading resources).
             *
             * @param message Smart pointer to the state message.
             */
            virtual void handleStateChanged( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Handle a state object change.
             *
             * Called when a state object is modified. Implementations typically react to
             * changes that affect rendering configuration or resources.
             *
             * @param state Smart pointer to the changed state.
             */
            virtual void handleStateChanged( SmartPtr<IState> &state );

            /**
             * @copydoc IGraphicsSystem::load
             *
             * @param data Optional load data passed from the shared object system.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IGraphicsSystem::unload
             *
             * @param data Optional data used during unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IGraphicsSystem::configure
             *
             * Configure Ogre render system, resource locations and other renderer options
             * according to the provided settings.
             *
             * @param config Graphics settings to apply.
             * @return true on successful configuration, false otherwise.
             */
            bool configure( SmartPtr<IBuildDirector> config ) override;

            /**
             * @copydoc IGraphicsSystem::getOverlayManager
             *
             * @return Overlay manager instance used to render UI and debug overlays.
             */
            SmartPtr<IOverlayManager> getOverlayManager() const override;

            /**
             * @brief Get the compositor manager.
             *
             * The compositor manager handles compositor workspaces, effects and post-processing.
             *
             * @return Smart pointer to the CompositorManager.
             */
            SmartPtr<CompositorManager> getCompositorManager() const;

            /**
             * @copydoc IGraphicsSystem::getResourceGroupManager
             *
             * @return Resource group manager used to manage Ogre resource groups.
             */
            SmartPtr<IResourceGroupManager> getResourceGroupManager() const override;

            /** @brief Returns a raw pointer to the internal material manager (Ogre wrapper). */
            IMaterialManager *getMaterialManagerPtr() const override;

            /**
             * @copydoc IGraphicsSystem::getMaterialManager
             *
             * @return Smart pointer to the material manager.
             */
            SmartPtr<IMaterialManager> getMaterialManager() const override;

            /**
             * @copydoc IGraphicsSystem::getTextureManager
             *
             * @return Smart pointer to the texture manager used by this system.
             */
            SmartPtr<ITextureManager> getTextureManager() const override;

            /**
             * @copydoc IGraphicsSystem::getInstanceManager
             *
             * @return Smart pointer to the instance manager (handles instanced meshes/objects).
             */
            SmartPtr<IInstanceManager> getInstanceManager() const override;

            /**
             * @copydoc IGraphicsSystem::createRenderWindow
             *
             * Create an Ogre-based render window. The returned window is wrapped by the engine's
             * IGraphicsWindow interface.
             *
             * @param name Window name.
             * @param width Width in pixels.
             * @param height Height in pixels.
             * @param fullScreen Whether to create the window fullscreen.
             * @param properties Optional platform/renderer-specific properties.
             * @return Smart pointer to the created IGraphicsWindow.
             */
            SmartPtr<IGraphicsWindow> createRenderWindow( const String &name, u32 width, u32 height,
                                                  bool fullScreen,
                                                  const SmartPtr<Properties> &properties ) override;

            /**
             * @copydoc IGraphicsSystem::getRenderWindow
             *
             * @param name Optional window name. If empty returns default or first window.
             * @return Smart pointer to the window matching the name (or default).
             */
            SmartPtr<IGraphicsWindow> getRenderWindow(
                const String &name = StringUtil::EmptyString ) const override;

            /**
             * @copydoc IGraphicsSystem::getLoadPriority
             *
             * Return a load priority for scheduling load/unload of the given object.
             */
            s32 getLoadPriority( SmartPtr<ISharedObject> obj ) override;

            /**
             * @copydoc IGraphicsSystem::update
             *
             * Called regularly to perform per-frame updates for the graphics system.
             */
            void update() override;

            /**
             * @brief Access deferred shading system for advanced rendering.
             *
             * Deferred shading systems are associated with viewports and handle
             * multiple-pass G-buffer rendering, lighting and post-processing.
             */
            SmartPtr<IGraphicsDeferredShading> &getDeferredShadingSystem();

            /** @brief Const overload of getDeferredShadingSystem(). */
            const SmartPtr<IGraphicsDeferredShading> &getDeferredShadingSystem() const;

            /**
             * @brief Assign the deferred shading system instance.
             * @param deferredShadingSystem Deferred shading subsystem to use.
             */
            void setDeferredShadingSystem( SmartPtr<IGraphicsDeferredShading> deferredShadingSystem );

            /**
             * @brief Check whether deferred shading is enabled in any viewport.
             * @return True if deferred shading is enabled.
             */
            bool isDeferredShadingSystemEnabled() const;

            /**
             * @brief Enable or disable deferred shading on a viewport.
             *
             * @param enabled Whether the deferred shading pipeline should be active.
             * @param vp Viewport to apply the deferred shading system to.
             */
            void addDeferredShadingSystem( bool enabled, SmartPtr<IViewport> vp );

            /**
             * @copydoc IGraphicsSystem::addDeferredShadingSystem
             *
             * Create and attach a deferred shading system to the supplied viewport.
             *
             * @param vp Viewport to attach to.
             * @return Smart pointer to the created deferred shading system.
             */
            SmartPtr<IGraphicsDeferredShading> addDeferredShadingSystem( SmartPtr<IViewport> vp ) override;

            /**
             * @copydoc IGraphicsSystem::removeDeferredShadingSystem
             *
             * Remove the deferred shading system associated with the specified viewport.
             *
             * @param vp Viewport to remove the deferred system from.
             */
            void removeDeferredShadingSystem( SmartPtr<IViewport> vp ) override;

            /**
             * @copydoc IGraphicsSystem::getDeferredShadingSystems
             *
             * @return Array of active deferred shading systems.
             */
            Array<SmartPtr<IGraphicsDeferredShading>> getDeferredShadingSystems() const override;

            /**
             * @copydoc IGraphicsSystem::restoreConfig
             *
             * Restore renderer configuration from persisted settings or defaults.
             */
            void restoreConfig();

            /**
             * @copydoc IGraphicsSystem::saveConfig
             *
             * Persist current renderer configuration to disk or settings store.
             */
            void saveConfig();

            /** @brief Access the mesh resource manager used by Ogre. */
            SmartPtr<IResourceManager> getMeshManager() const;

            /** @brief Set the mesh manager instance. */
            void setMeshManager( SmartPtr<IResourceManager> meshManager );

            /**
             * @copydoc IGraphicsSystem::getDefaultWindow
             *
             * @return The default render window used by the system.
             */
            SmartPtr<IGraphicsWindow> getDefaultWindow() const override;

            /**
             * @copydoc IGraphicsSystem::setDefaultWindow
             *
             * @param defaultWindow The window to use as the default.
             */
            void setDefaultWindow( SmartPtr<IGraphicsWindow> defaultWindow ) override;

            /**
             * @copydoc IGraphicsSystem::createDebugTextOverlay
             *
             * Create an on-screen overlay used to show debug text (FPS, stats, warnings).
             */
            void createDebugTextOverlay();

            /**
             * @copydoc IGraphicsSystem::generateDebugText
             *
             * Append formatted debug text to the supplied string based on timing and current stats.
             *
             * @param timeSinceLast Time since last update in seconds.
             * @param outText Output string to append debug text to.
             */
            void generateDebugText( f32 timeSinceLast, Ogre::String &outText );

            /**
             * @copydoc IGraphicsSystem::messagePump
             *
             * Process platform/window events and forward them to Ogre and UI systems.
             */
            void messagePump() override;

            /**
             * @copydoc IGraphicsSystem::getSpriteRenderer
             *
             * @return Sprite renderer for 2D drawing.
             */
            SmartPtr<IRenderer> getRenderer() const override;

            /**
             * @copydoc IGraphicsSystem::getFontManager
             *
             * @return Font manager instance used by UI and debug rendering.
             */
            SmartPtr<IFontManager> getFontManager() const override;

            /**
             * @copydoc IGraphicsSystem::setupRenderer
             *
             * Set up the renderer for the given scene manager, window and camera.
             *
             * @param sceneManager Scene manager to render.
             * @param window Target window.
             * @param camera Camera to use for rendering.
             * @param workspaceName Compositor workspace name to use.
             * @param enabled Whether renderer setup is enabled.
             */
            void setupRenderer( SmartPtr<IGraphicsScene> sceneManager, SmartPtr<IGraphicsWindow> window,
                                SmartPtr<IGraphicsCamera> camera, String workspaceName, bool enabled ) override;

            /**
             * @copydoc IGraphicsSystem::getStateTask
             *
             * @return TaskId used for scheduling state updates.
             */
            TaskId getStateTask() const override;

            /**
             * @copydoc IGraphicsSystem::getRenderTask
             *
             * @return TaskId used for scheduling render updates.
             */
            TaskId getRenderTask() const override;

            /**
             * @copydoc IGraphicsSystem::loadObject
             *
             * Load a graphics object. When forceQueue is true the object is queued for loading
             * on the render thread.
             */
            void loadObject( SmartPtr<ISharedObject> graphicsObject, bool forceQueue = false ) override;

            /**
             * @copydoc IGraphicsSystem::unloadObject
             *
             * Unload a previously loaded graphics object. When forceQueue is true the object
             * is queued for unload on the render thread.
             */
            void unloadObject( SmartPtr<ISharedObject> graphicsObject,
                               bool forceQueue = false ) override;

            /**
             * @copydoc IGraphicsSystem::createConfiguration
             *
             * Create and return a default graphics configuration object.
             */
            SmartPtr<IBuildDirector> createConfiguration() override;

            /**
             * @copydoc IGraphicsSystem::getDebug
             *
             * @return Debugging interface used by the graphics system.
             */
            SmartPtr<IDebug> getDebug() const override;

            /**
             * @copydoc IGraphicsSystem::setDebug
             *
             * @param debug Debugging interface to use.
             */
            void setDebug( SmartPtr<IDebug> debug ) override;

            /**
             * @brief Get the currently selected RenderApi.
             * @return RenderApi enum value.
             */
            RenderApi getRenderApi() const;

            /**
             * @brief Set the current RenderApi.
             * @param renderApi Render API to use (OpenGL, DirectX, Vulkan, etc).
             */
            void setRenderApi( RenderApi renderApi );

            /** @brief Get the mesh converter helper used when importing/converting meshes. */
            SmartPtr<IMeshConverter> getMeshConverter() const override;

            /** @brief Set the mesh converter helper. */
            void setMeshConverter( SmartPtr<IMeshConverter> meshConverter ) override;

            /**
             * @copydoc IGraphicsSystem::isValid
             *
             * @return True if the graphics system and its subsystems are correctly initialised.
             */
            bool isValid() const override;

            /** @brief Return whether the run-time shader system (RTSS) is used. */
            bool getUseRTSS() const;

            /** @brief Enable/disable the run-time shader system (RTSS). */
            void setUseRTSS( bool useRTSS );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Internal query whether the system is currently performing an update. */
            bool isUpdating() const;

            /** @brief Set the internal updating state. */
            void setUpdating( bool updating );

            /**
             * @brief Resolve RenderApi enum from a render plugin name.
             * @param renderPluginName Name of the renderer plugin.
             * @return RenderApi enum corresponding to the plugin or RenderApi::None.
             */
            RenderApi getRenderApi( String renderPluginName );
            /**
             * @brief Get the plugin name for a given RenderApi.
             * @param renderApi Render API enum.
             * @return Plugin name string used by Ogre.
             */
            String getRenderPluginName( RenderApi renderApi );

            /** @brief Choose an appropriate render system from available Ogre plugins. */
            bool chooseRenderSystem();

            /**
             * @brief Apply sensible defaults to an Ogre::RenderSystem instance.
             *
             * This configures defaults such as VSync, multisampling and other renderer-specific
             * options to ensure consistent rendering behaviour across platforms.
             *
             * @param renderSystem Pointer to Ogre render system to configure.
             */
            void setRenderSystemDefaults( Ogre::RenderSystem *renderSystem );

            /** @brief Initialise the Ogre Run-Time Shader System (RTSS), if available. */
            bool initialiseRTShaderSystem();

            /**< Compositor manager instance. */
            AtomicSmartPtr<CompositorManager> m_compositorManager;

            /**< Immediate mode UI integration manager. */
            SmartPtr<ImGuiManagerOgre> m_imGuiManager;

            /**< The Ogre::Root instance (owns plugins, render systems, etc.). */
            Ogre::Root *m_root = nullptr;

            /**< Registered render queue listener. */
            Ogre::RenderQueueListener *m_rqListener = nullptr;

            /**< Frame listener instance. */
            AppFrameListener *m_frameListener;

            /**< Helper to manage resource groups. */
            ResourceGroupHelper *m_resourceGroupHelper = nullptr;

            /**< Listener for resource loading events. */
            ResourceLoadingListener *m_resourceLoadingListener = nullptr;

            /**< Listener to react to material events. */
            MaterialListener *m_materialListener = nullptr;

            /**< Optional particle plugin pointer. */
            Ogre::Plugin *m_particlePlugin = nullptr;

            /**< Loader for static Ogre plugins. */
            Ogre::StaticPluginLoader *m_staticPluginLoader = nullptr;

            /**< Ogre overlay system for UI rendering. */
            Ogre::OverlaySystem *m_overlaySystem = nullptr;

#ifdef OGRE_STATIC_LIB
#    if WP_BUILD_RENDERER_OPENGL
            Ogre::GLPlugin *m_glPlugin = nullptr;
#    endif
#    if WP_BUILD_RENDERER_GL3PLUS
            Ogre::GL3PlusPlugin *m_gl3Plugin = nullptr;
#    endif
#    if defined OGRE_BUILD_RENDERSYSTEM_GLES2
            Ogre::GLES2Plugin *mGLES2Plugin = nullptr;
#    endif
#    if WP_BUILD_RENDERER_DX11
            Ogre::D3D11Plugin *m_d3d11Plugin = nullptr;
#    endif
#    if WP_BUILD_RENDERER_DX9
            Ogre::D3D9Plugin *m_d3d9Plugin = nullptr;
#    endif
#    if WP_BUILD_RENDERER_METAL
            Ogre::MetalPlugin *m_metalPlugin = nullptr;
#    endif
#endif

            /**< Cell scene manager factory. */
            CellSceneManagerFactory *m_cellSceneManagerFactory = nullptr;

            /**< Basic scene manager factory. */
#if !defined( ANDROID )
            BasicSceneManagerFactory *m_basicSceneManagerFactory = nullptr;
#endif

            /**< Terrain global options singleton. */
            Ogre::TerrainGlobalOptions *mTerrainGlobals = nullptr;

            /**< Ogitors editor root (if integrated). */
            Ogitors::OgitorsRoot *m_ogitorsRoot = nullptr;

#ifdef OGRE_BUILD_COMPONENT_RTSHADERSYSTEM
            // The Shader generator instance.
            Ogre::RTShader::ShaderGenerator *m_shaderGenerator = nullptr;

            // Shader generator material manager listener.
            OgreBites::SGTechniqueResolverListener *m_materialMgrListener = nullptr;
#endif

            /**< Whether the run-time shader system is active. */
            bool m_useRTSS = false;

            /**< Atomic flag indicating update in progress. */
            atomic_bool m_isUpdating = false;

            /**< Deferred shading systems per-viewport. */
            Array<SmartPtr<IGraphicsDeferredShading>> m_deferredShadingSystems;
        };
    }  // end namespace render
}  // namespace workphone

#endif
