#ifndef IGraphicsSystem_h__
#define IGraphicsSystem_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringPool.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Thread/Thread.hpp>

namespace workphone
{
    namespace render
    {
        class IGraphicsPipeline;

        /**
         * @class IGraphicsSystem
         * @brief The base interface for the graphics system.
         *
         * This interface defines the core functionality for managing the graphics pipeline,
         * including render API selection, scene management, object loading/unloading,
         * and integration with the system's task and state management.
         */
        class WPCore_API IGraphicsSystem : public ISharedObject
        {
        public:
            /** Optional live pipeline for editor inspection. */
            virtual SmartPtr<IGraphicsPipeline> getGraphicsPipeline() const;

            /**
             * @enum RenderApi
             * @brief Supported graphics rendering APIs.
             */
            enum class RenderApi
            {
                None,      ///< No API selected
                DX9,       ///< DirectX 9
                DX11,      ///< DirectX 11
                DX12,      ///< DirectX 12
                GL,        ///< OpenGL
                GL3Plus,   ///< OpenGL 3.0+
                Vulkan,    ///< Vulkan
                Metal,     ///< Apple Metal
                Software,  ///< Software rendering

                Count  ///< Total number of render APIs
            };
            /** Configure the API before initializing the graphics system. */
            virtual void setRendererType( RenderApi api );

            /** Event triggered immediately before the rendering process starts. */
            static const hash_type FRAME_EVENT_PRE_RENDER;
            /** Event triggered when a render command has been queued. */
            static const hash_type FRAME_EVENT_RENDER_QUEUED;
            /** Event triggered after the rendering process has completed. */
            static const hash_type FRAME_EVENT_POST_RENDER;

            /** @brief Virtual destructor for the graphics system. */
            ~IGraphicsSystem() override;

            /**
             * @brief Creates a new graphics configuration object.
             * @return A SmartPtr to a newly created IBuildDirector used for configuration.
             */
            virtual SmartPtr<IBuildDirector> createConfiguration() = 0;

            /**
             * @brief Configures the graphics system using the provided settings.
             * @param config The configuration director containing the desired settings.
             * @return True if the configuration was applied successfully, false otherwise.
             */
            virtual bool configure( SmartPtr<IBuildDirector> config ) = 0;

            /**
             * @brief Processes messages from the system message queue.
             *
             * Should be called regularly to handle asynchronous graphics events or
             * windowing messages.
             */
            virtual void messagePump() = 0;

            /**
             * @brief Triggers the actual rendering process for the current frame.
             */
            virtual void render() = 0;

            /**
             * @brief Retrieves the raw pointer to the debug interface.
             * @return Raw pointer to the IDebug instance.
             */
            virtual IDebug *getDebugPtr() const = 0;

            /**
             * @brief Retrieves the debug interface as a SmartPtr.
             * @return SmartPtr to the IDebug instance.
             */
            virtual SmartPtr<IDebug> getDebug() const = 0;

            /**
             * @brief Sets the debug interface for the graphics system.
             * @param debug SmartPtr to the debug interface to be used.
             */
            virtual void setDebug( SmartPtr<IDebug> debug ) = 0;

            /**
             * @brief Creates and adds a scene manager to the graphics system.
             * @param type The type identifier of the scene manager to instantiate.
             * @param name A unique name to assign to the scene manager.
             * @return SmartPtr to the newly created IGraphicsScene.
             */
            virtual SmartPtr<IGraphicsScene> addGraphicsScene( const String &type,
                                                               const String &name ) = 0;

            /**
             * @brief Removes a specific scene manager from the graphics system.
             * @param scene SmartPtr to the scene manager to be removed.
             */
            virtual void removeGraphicsScene( SmartPtr<IGraphicsScene> scene ) = 0;

            /**
             * @brief Removes all registered scene managers from the graphics system.
             */
            virtual void removeAllGraphicsScenes() = 0;

            /**
             * @brief Clears all scene managers from the graphics system without unloading them.
             */
            virtual void clearGraphicScenes() = 0;

            /**
             * @brief Gets the raw pointer to the default scene manager.
             * @return Raw pointer to the default IGraphicsScene.
             */
            virtual IGraphicsScene *getGraphicsScenePtr() const = 0;

            /**
             * @brief Gets the default scene manager as a SmartPtr.
             * @return SmartPtr to the default IGraphicsScene.
             */
            virtual SmartPtr<IGraphicsScene> getGraphicsScene() const = 0;

            /**
             * @brief Retrieves a scene manager by its assigned name.
             * @param name The name of the scene manager to find.
             * @return SmartPtr to the scene manager if found, otherwise nullptr.
             */
            virtual SmartPtr<IGraphicsScene> getGraphicsScene( const String &name ) const = 0;

            /**
             * @brief Retrieves a scene manager by its unique ID.
             * @param id The hash ID of the scene manager to find.
             * @return SmartPtr to the scene manager if found, otherwise nullptr.
             */
            virtual SmartPtr<IGraphicsScene> getGraphicsSceneById( hash_type id ) const = 0;

            /** Gets the scene managers.
             * @return An array of smart pointers to the scene managers.
             */
            virtual Array<SmartPtr<IGraphicsScene>> getSceneManagers() const = 0;

            /**
             * Gets the overlay manager.
             * @return A pointer to the overlay manager.
             */
            virtual IOverlayManager *getOverlayManagerPtr() const = 0;

            /**
             * Gets the overlay manager.
             * @return A pointer to the overlay manager.
             */
            virtual SmartPtr<IOverlayManager> getOverlayManager() const = 0;

            /**
             * Gets the resource group manager.
             * @return A pointer to the resource group manager.
             */
            virtual SmartPtr<IResourceGroupManager> getResourceGroupManager() const = 0;

            /**
             * Gets the material manager.
             * @return A pointer to the material manager.
             */
            virtual IMaterialManager *getMaterialManagerPtr() const = 0;

            /**
             * Gets the material manager.
             * @return A pointer to the material manager.
             */
            virtual SmartPtr<IMaterialManager> getMaterialManager() const = 0;

            /**
             * Gets the texture manager.
             * @return A pointer to the texture manager.
             */
            virtual ITextureManager *getTextureManagerPtr() const = 0;

            /**
             * Gets the texture manager.
             * @return A pointer to the texture manager.
             */
            virtual SmartPtr<ITextureManager> getTextureManager() const = 0;

            /** Gets the instance manager. */
            virtual SmartPtr<IInstanceManager> getInstanceManager() const = 0;

            /** Gets the renderer. */
            virtual IRenderer *getRendererPtr() const = 0;

            /** Gets the renderer. */
            virtual SmartPtr<IRenderer> getRenderer() const = 0;

            /** Gets the font manager. */
            virtual SmartPtr<IFontManager> getFontManager() const = 0;

            /** Creates a render window. */
            virtual SmartPtr<IGraphicsWindow> createRenderWindow(
                const String &name, u32 width, u32 height, bool fullScreen,
                const SmartPtr<Properties> &properties ) = 0;

            /** Destroys a render window. */
            virtual void destroyRenderWindow( SmartPtr<IGraphicsWindow> window ) = 0;

            /** Gets a render window.
             * @return A pointer to the render window with the specified name.
             */
            virtual SmartPtr<IGraphicsWindow> getRenderWindow(
                const String &name = StringUtil::EmptyString ) const = 0;

            /** Gets the default render window.
             * @return A pointer to the default render window.
             */
            virtual SmartPtr<IGraphicsWindow> getDefaultWindow() const = 0;

            /** Gets the default render window.
             * @param defaultWindow A smart pointer to the default render window.
             */
            virtual void setDefaultWindow( SmartPtr<IGraphicsWindow> defaultWindow ) = 0;

            /** Gets the windows.
             * @return An array of smart pointers to the render windows.
             */
            virtual Array<SmartPtr<IGraphicsWindow>> getWindows() const = 0;

            /** Adds a deferred shading system to a viewport. */
            virtual SmartPtr<IGraphicsDeferredShading> addDeferredShadingSystem(
                SmartPtr<IViewport> vp ) = 0;

            /** Removes a deferred shading system from a viewport. */
            virtual void removeDeferredShadingSystem( SmartPtr<IViewport> vp ) = 0;

            /** Gets the deferred shading systems. */
            virtual Array<SmartPtr<IGraphicsDeferredShading>> getDeferredShadingSystems() const = 0;

            /** Loads an object via the graphics system.
             * @param graphicsObject The object to be loaded.
             * @param forceQueue Forces the object to be queued for deferred loading.
             */
            virtual void loadObject( SmartPtr<ISharedObject> graphicsObject,
                                     bool forceQueue = false ) = 0;

            /**
             * @brief Reloads an existing graphics object.
             * @param graphicsObject The object to be reloaded.
             * @param forceQueue If true, the operation is queued for deferred execution.
             */
            virtual void reloadObject( SmartPtr<ISharedObject> graphicsObject,
                                       bool forceQueue = false ) = 0;

            /**
             * @brief Unloads a graphics object from the system.
             * @param graphicsObject The object to unload.
             * @param forceQueue If true, the object is queued for deferred unloading.
             */
            virtual void unloadObject( SmartPtr<ISharedObject> graphicsObject,
                                       bool forceQueue = false ) = 0;

            /**
             * @brief Processes and clears all pending object load/unload queues.
             *
             * This is typically called at the end of a frame to ensure all resource
             * updates are committed before the next frame.
             */
            virtual void clearObjectQueues() = 0;

            /**
             * @brief Initializes the renderer with the specified components.
             * @param sceneManager The scene manager to use for rendering.
             * @param window The target window for the output.
             * @param camera The camera used for the view.
             * @param workspaceName The name of the workspace associated with this setup.
             * @param enabled Whether the renderer should be active.
             */
            virtual void setupRenderer( SmartPtr<IGraphicsScene> sceneManager,
                                        SmartPtr<IGraphicsWindow> window,
                                        SmartPtr<IGraphicsCamera> camera, String workspaceName,
                                        bool enabled ) = 0;

            /**
             * @brief Gets the Task ID associated with the current system state.
             * @return The current state task ID.
             */
            virtual TaskId getStateTask() const = 0;

            /**
             * @brief Gets the Task ID associated with the rendering process.
             * @return The current render task ID.
             */
            virtual TaskId getRenderTask() const = 0;

            /**
             * @brief Gets the mesh converter used by the system.
             * @return SmartPtr to the current IMeshConverter.
             */
            virtual SmartPtr<IMeshConverter> getMeshConverter() const = 0;

            /**
             * @brief Sets the mesh converter to be used by the system.
             * @param meshConverter SmartPtr to the new IMeshConverter.
             */
            virtual void setMeshConverter( SmartPtr<IMeshConverter> meshConverter ) = 0;

            /**
             * @brief Gets the raw pointer to the factory manager.
             * @return Raw pointer to the IFactoryManager.
             */
            virtual IFactoryManager *getFactoryManagerPtr() const = 0;

            /**
             * @brief Gets the factory manager as a SmartPtr.
             * @return SmartPtr to the IFactoryManager.
             */
            virtual SmartPtr<IFactoryManager> getFactoryManager() const = 0;

            /**
             * @brief Sets the factory manager for the system.
             * @param factoryManager SmartPtr to the new IFactoryManager.
             */
            virtual void setFactoryManager( SmartPtr<IFactoryManager> factoryManager ) = 0;

            /**
             * @brief Gets the global string pool used for graphics resources.
             * @return Raw pointer to the StringPool.
             */
            virtual StringPool<c8> *getStringPool() const = 0;

            /**
             * @brief Sets the global string pool for the graphics system.
             * @param pool Raw pointer to the StringPool to be used.
             */
            virtual void setStringPool( StringPool<c8> *pool ) = 0;

            /**
             * @brief Determines the loading priority for a specific object.
             * @param obj The object to check.
             * @return The priority value (lower typically means higher priority).
             */
            virtual s32 getLoadPriority( SmartPtr<ISharedObject> obj ) = 0;

            /**
             * @brief Gets the raw pointer to the attached state context.
             *
             * Note: The returned pointer does not provide ownership guarantees.
             * @return Raw IStateContext pointer, or nullptr if not set.
             */
            virtual IStateContext *getStateContextPtr() const = 0;

            /**
             * @brief Gets the SmartPtr to the attached state context.
             * @return SmartPtr to the IStateContext, or nullptr if not set.
             */
            virtual SmartPtr<IStateContext> getStateContext() const = 0;

            /**
             * @brief Attaches a state context to the graphics system.
             *
             * The context will be managed and cleaned up during the system's teardown.
             * @param stateContext SmartPtr to the state context to attach.
             */
            virtual void setStateContext( SmartPtr<IStateContext> stateContext ) = 0;

            /**
             * @brief Gets the state listener attached to this system.
             * @return SmartPtr to the IStateListener, or nullptr if not set.
             */
            virtual SmartPtr<IStateListener> getStateListener() const = 0;

            /**
             * @brief Attaches a state listener to the graphics system.
             *
             * The listener will be removed and cleaned up when the system is torn down.
             * @param stateListener SmartPtr to the listener to attach.
             */
            virtual void setStateListener( SmartPtr<IStateListener> stateListener ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace render
}  // namespace workphone

#endif  // IGraphicsSystem_h__
