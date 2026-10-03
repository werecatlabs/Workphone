#ifndef __UIManager_h__
#define __UIManager_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {

        /**
         * @file UIManager.hpp
         * @brief Central UI manager for the editor.
         *
         * This header declares the UIManager class which owns and coordinates the editor's
         * windows, dialogs and UI-related listeners and jobs. The UIManager provides accessors
         * for main editor windows, mediates UI events and exposes hooks for scripting.
         *
         * Usage notes:
         * - UIManager is responsible for creating / holding references to many UI windows.
         * - Use accessors to obtain pointers/shared pointers to windows rather than storing
         *   your own copies.
         */

        /**
         * @class UIManager
         * @brief Central manager class for handling all UI-related functionality in the editor.
         *
         * The UIManager coordinates UI windows, dialogs, and event listeners. It is the
         * primary place to query and modify top-level editor UI components and to trigger
         * UI updates such as rebuilding trees or synchronizing selection state between
         * windows. It also provides script integration points via script class/invoker.
         */
        class UIManager : public ISharedObject
        {
        public:
            /**
             * @class UpdateSelectionJob
             * @brief Background job that applies selection updates to UI components.
             *
             * This Job is scheduled when the editor selection changes. When executed it
             * invokes the UIManager selection update routines on the appropriate thread
             * or context so that UI controls stay synchronized with the engine selection.
             */
            class UpdateSelectionJob : public Job
            {
            public:
                /** @brief Constructs an UpdateSelectionJob. */
                UpdateSelectionJob();
                /** @brief Virtual destructor. */
                ~UpdateSelectionJob() override;

                /**
                 * @brief Performs the selection update work.
                 *
                 * Implementations should call back into the owning UIManager to update
                 * selection state across windows. This runs as part of the job system.
                 */
                void execute() override;

                WP_CLASS_REGISTER_DECL;
            };

            /**
             * @class MenuBarListener
             * @brief Listener for menu bar events forwarded to the UIManager.
             *
             * Handles menu-related UI events and delegates handling to the owning UIManager.
             * The listener stores a raw pointer to the owner; ownership is not transferred.
             */
            class MenuBarListener : public IEventListener
            {
            public:
                MenuBarListener();
                ~MenuBarListener() override;

                /**
                 * @brief Handle a menu bar event.
                 * @param eventType The type/category of the event.
                 * @param eventValue A hashed identifier for the specific event.
                 * @param arguments Additional event parameters.
                 * @param sender The originator of the event (may be null).
                 * @param object Optional target object for the event.
                 * @param event Raw event object (may contain extra data).
                 * @return Parameter containing any result produced by the handler.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /** @brief Returns the owning UIManager pointer (non-owning). */
                UIManager *getOwner() const;
                /** @brief Sets the non-owning owner pointer. */
                void setOwner( UIManager *owner );

                WP_CLASS_REGISTER_DECL;

            private:
                UIManager *m_owner =
                    nullptr;  ///< Non-owning pointer to the UIManager that created this listener.
            };

            /**
             * @class ToolbarListener
             * @brief Listener for toolbar events forwarded to the UIManager.
             *
             * Processes toolbar button/toggle events and forwards them to the UIManager
             * for higher-level action handling.
             */
            class ToolbarListener : public IEventListener
            {
            public:
                ToolbarListener();
                ~ToolbarListener() override;

                /**
                 * @brief Handle a toolbar event.
                 * @param eventType The type/category of the event.
                 * @param eventValue A hashed identifier for the specific event.
                 * @param arguments Additional event parameters.
                 * @param sender The originator of the event (may be null).
                 * @param object Optional target object for the event.
                 * @param event Raw event object (may contain extra data).
                 * @return Parameter containing any result produced by the handler.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /** @brief Returns the owning UIManager pointer (non-owning). */
                UIManager *getOwner() const;

                /** @brief Sets the non-owning owner pointer. */
                void setOwner( UIManager *owner );

                WP_CLASS_REGISTER_DECL;

            private:
                UIManager *m_owner =
                    nullptr;  ///< Non-owning pointer to the UIManager that created this listener.
            };

            /**
             * @class EventListener
             * @brief General purpose event listener for UI events that holds a weak reference to the UIManager.
             *
             * Used by various UI components to forward events into the manager without creating
             * strong ownership cycles. The owner is stored as an AtomicWeakPtr to allow safe
             * cross-thread access and avoid lifetime issues.
             */
            class EventListener : public IEventListener
            {
            public:
                EventListener();
                ~EventListener() override;

                /**
                 * @brief Handle a general UI event.
                 * @param eventType The type/category of the event.
                 * @param eventValue A hashed identifier for the specific event.
                 * @param arguments Additional event parameters.
                 * @param sender The originator of the event (may be null).
                 * @param object Optional target object for the event.
                 * @param event Raw event object (may contain extra data).
                 * @return Parameter containing any result produced by the handler.
                 */
                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                /** @brief Returns a smart pointer to the owner UIManager (may be null). */
                SmartPtr<UIManager> getOwner() const;
                /** @brief Sets the owner as a weak smart pointer to avoid ownership cycles. */
                void setOwner( SmartPtr<UIManager> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<UIManager> m_owner;  ///< Weak reference to owning UIManager.
            };

            /**
             * @enum WidgetId
             * @brief Unique identifiers for editor UI widgets, dialogs and actions.
             *
             * Use these identifiers when raising or handling UI events so handlers can
             * determine the source widget or requested action. Values are grouped roughly
             * by functionality (file, project, windows, tools, etc.).
             */
            enum class WidgetId
            {
                None,

                Exit,
                About,

                NewProjectDialog,
                OpenProjectDialog,
                SaveProjectDialog,
                NewSceneDialog,
                OpenSceneDialog,
                OpenSaveSceneDialog,
                OpenSaveAsSceneDialog,
                ImportJsonSceneDialog,
                LoadProceduralSceneDialog,
                SaveProceduralSceneDialog,

                BatchAllBtnId,
                AppPropertiesId,

                NewProjectId,
                OpenProjectId,
                SaveProjectId,

                NewSceneId,
                OpenSceneId,
                SaveId,
                SaveSceneAsId,
                SaveAllId,
                GenerateCMakeProjectId,
                CompileId,
                CreatePackageId,
                CreateUnityBindings,
                ProjectSettingsId,

                LoadProceduralSceneId,
                SaveProceduralSceneId,

                LuaEditConfigDialogId,

                AnimationEditorId,
                AnimationGraphEditorId,
                ComponentsId,
                ResourcesId,
                InputWindowId,
                LayersWindowId,
                CollisionMasksWindowId,
                TagsWindowId,
                ProfilerWindowId,
                GraphicsPipelineId,
                GameGraphicsId,
                ObjectWindowId,
                ProjectWindowId,
                SceneWindowId,
                SoundWindowId,
                ParticleWindowId,
                ShaderWindowId,
                AssetDatabaseWindowId,
                ProceduralModelEditorId,

                CutsceneWindowId,

                UndoId,
                RedoId,
                CutId,
                CopyId,
                PasteId,
                DeleteId,
                SelectAllId,
                DuplicateId,
                GotoId,

                CreatePluginCodeId,
                LoadPluginId,
                UnloadPluginId,
                CopyEngineFilesId,

                ImportJsonSceneId,
                ImportUnityYamlId,

                ShowAllOverlaysId,
                HideAllOverlaysId,
                CreateOverlayTestId,
                CreateOverlayTextTestId,
                CreateOverlayButtonTestId,

                CreateRigidBodies,
                CreateRigidStaticMeshId,
                CreateRigidDynamicMeshId,
                CreateConstraintId,

                CreateProceduralTestId,
                CreateProceduralTestSceneMenuId,

                GenerateSkyboxMaterialsId,
                ConvertFbx,
                ConvertAllFbx,

                OptimiseDatabasesId,
                CleanDatabasesId,
                CreateAssetFromDatabasesId,

                SetupMaterialsId,

                CreateDefaultCarId,
                CreateDefaultTruckId,

                ConvertCSharpId,

                PhysicsEnableId,

                ID_CustomizeToolbar,

                RunId,
                StopId,
                StatsId,
                ShowDebugId,
                ShowSceneDebugId,
                ShowUiDebugId,
                LocalTransformId,
                ReloadScriptsId,
                ToggleEditorCameraId,

                TranslateEditorCameraId,
                RotateEditorCameraId,
                ScaleEditorCameraId,

                AssetImportId,
                AssetReimportId,
                AssetDatabaseBuildId,
                AssetDatabaseImportCacheId,
                AssetDatabaseDeleteCacheId,

                AddResourceNodeId,
                DeleteResourceNodeId,
                ResourcesAddComponentId,
                ResourcesRemoveComponentId,
                AddGroupId,
                RemoveGroupId,
                GroupAddComponentId,
                GroupRemoveComponentId,

                FileBrowserId,
                CreateBoxTestId,

                CleanProjectId,

                MakeAllStateContextsDirtyId,

                AboutId,

                ID_SampleItem,
            };

            /** @brief Default constructor. Sets up internal listeners and state. */
            UIManager();

            /** @brief Destructor. Cleans up UI references. */
            ~UIManager() override;

            /**
             * @brief Initialize and load UI resources.
             * @param data Optional initialization data (implementation-specific).
             *
             * This method should create or bind windows, register listeners and perform
             * any setup required for the UI to function.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload UI resources and detach listeners.
             * @param data Optional data passed for cleanup (implementation-specific).
             *
             * Implementations must release owned references and ensure no dangling
             * callbacks remain after unload completes.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Returns the SceneWindow instance.
             * @return SmartPtr<SceneWindow> Smart pointer to the scene window (may be null).
             */
            SmartPtr<SceneWindow> getSceneWindow() const;

            /**
             * @brief Set the SceneWindow instance used by the manager.
             * @param sceneWindow Smart pointer to the scene window to use.
             */
            void setSceneWindow( SmartPtr<SceneWindow> sceneWindow );

            /**
             * @brief Returns the ActorWindow instance.
             * @return SmartPtr<ActorWindow> Smart pointer to the actor window (may be null).
             */
            SmartPtr<ActorWindow> getActorWindow() const;

            /**
             * @brief Set the ActorWindow instance used by the manager.
             * @param actorWindow Smart pointer to the actor window to use.
             */
            void setActorWindow( SmartPtr<ActorWindow> actorWindow );

            /**
             * @brief Returns the TerrainWindow instance.
             * @return TerrainWindow* Raw pointer to the terrain window (may be null).
             *
             * Note: caller must not assume ownership; lifetime is managed by UIManager.
             */
            TerrainWindow *getTerrainWindow() const;

            /**
             * @brief Set the TerrainWindow instance used by the manager.
             * @param val Pointer to terrain window (UIManager does not take ownership).
             */
            void setTerrainWindow( TerrainWindow *val );

            /**
             * @brief Returns the PropertiesWindow instance.
             * @return PropertiesWindow* Raw pointer to the properties window (may be null).
             *
             * The returned pointer is non-owning.
             */
            PropertiesWindow *getPropertiesWindow() const;

            /**
             * @brief Set the PropertiesWindow instance used by the manager.
             * @param val Pointer to properties window (UIManager does not take ownership).
             */
            void setPropertiesWindow( PropertiesWindow *val );

            /**
             * @brief Returns the ApplicationFrame instance.
             * @return ApplicationFrame* Raw pointer to the main application frame (may be null).
             */
            ApplicationFrame *getApplicationFrame() const;

            /**
             * @brief Set the ApplicationFrame instance used by the manager.
             * @param val Pointer to application frame (UIManager does not take ownership).
             */
            void setApplicationFrame( ApplicationFrame *val );

            /**
             * @brief Rebuilds the scene tree view to reflect the latest scene contents.
             *
             * This triggers a refresh of the scene hierarchy UI; call after scene edits.
             */
            void rebuildSceneTree();

            /**
             * @brief Rebuilds the actor tree view to reflect the latest actor hierarchy.
             *
             * Use this when actor additions/removals or component structural changes occur.
             */
            void rebuildActorTree();

            /**
             * @brief Rebuilds the resource tree view to reflect the latest resources.
             *
             * Call after resource import, deletion or database changes.
             */
            void rebuildResourceTree();

            /**
             * @brief Update global selection across windows.
             *
             * Synchronizes the editor selection so all relevant windows show consistent state.
             * This method may queue an UpdateSelectionJob for asynchronous processing.
             */
            void updateSelection();

            /**
             * @brief Update actor-specific selection state.
             *
             * Propagates actor selection changes to windows that display actor data.
             */
            void updateActorSelection();

            /**
             * @brief Update component-specific selection state.
             *
             * Propagates component selection changes to windows that display component data.
             */
            void updateComponentSelection();

            /**
             * @brief Returns the FileWindow instance.
             * @return FileWindow* Non-owning pointer to the file window (may be null).
             */
            FileWindow *getFileWindow() const;

            /**
             * @brief Set the FileWindow instance used by the manager.
             * @param val Pointer to file window (non-owning).
             */
            void setFileWindow( FileWindow *val );

            /**
             * @brief Returns the ProjectWindow instance.
             * @return SmartPtr<ProjectWindow> Smart pointer to the project window (may be null).
             */
            SmartPtr<ProjectWindow> getProjectWindow() const;

            /**
             * @brief Set the ProjectWindow instance used by the manager.
             * @param val Smart pointer to project window.
             */
            void setProjectWindow( SmartPtr<ProjectWindow> val );

            /**
             * @brief Returns the MaterialWindow instance.
             * @return MaterialWindow* Non-owning pointer to the material window (may be null).
             */
            MaterialWindow *getMaterialWindow() const;

            /**
             * @brief Set the MaterialWindow instance used by the manager.
             * @param materialWindow Pointer to material window (non-owning).
             */
            void setMaterialWindow( MaterialWindow *materialWindow );

            /**
             * @brief Returns the ObjectWindow instance.
             * @return SmartPtr<ObjectWindow> Smart pointer to the object window (may be null).
             */
            SmartPtr<ObjectWindow> getObjectWindow() const;

            /**
             * @brief Set the ObjectWindow instance used by the manager.
             * @param objectWindow Smart pointer to object window.
             */
            void setObjectWindow( SmartPtr<ObjectWindow> objectWindow );

            /**
             * @brief Returns the ObjectBrowserDialog instance.
             * @return SmartPtr<ObjectBrowserDialog> Smart pointer to the dialog (may be null).
             */
            SmartPtr<ObjectBrowserDialog> getObjectBrowserDialog() const;

            /**
             * @brief Set the ObjectBrowserDialog instance used by the manager.
             * @param objectBrowserDialog Smart pointer to the dialog.
             */
            void setObjectBrowserDialog( SmartPtr<ObjectBrowserDialog> objectBrowserDialog );

            /**
             * @brief Returns the ResourceDatabaseDialog instance.
             * @return SmartPtr<ResourceDatabaseDialog> Smart pointer to the dialog (may be null).
             */
            SmartPtr<ResourceDatabaseDialog> getResourceDatabaseDialog() const;

            /**
             * @brief Set the ResourceDatabaseDialog instance used by the manager.
             * @param resourceDatabaseDialog Smart pointer to the dialog.
             */
            void setResourceDatabaseDialog( SmartPtr<ResourceDatabaseDialog> resourceDatabaseDialog );

            /**
             * @brief Returns the About dialog instance.
             * @return SmartPtr<EditorWindow> Smart pointer to the about dialog (may be null).
             */
            SmartPtr<EditorWindow> getAboutDialog() const;

            /**
             * @brief Set the About dialog instance used by the manager.
             * @param aboutDialog Smart pointer to the about dialog.
             */
            void setAboutDialog( SmartPtr<EditorWindow> aboutDialog );

            /**
             * @brief Returns the InputManagerWindow instance.
             * @return SmartPtr<InputManagerWindow> Smart pointer to input manager window (may be null).
             */
            SmartPtr<EditorWindow> getInputManagerWindow() const;

            /**
             * @brief Set the InputManagerWindow instance used by the manager.
             * @param inputManagerWindow Smart pointer to input manager window.
             */
            void setInputManagerWindow( SmartPtr<EditorWindow> inputManagerWindow );

            SmartPtr<LayerManager> getLayerManager() const;

            void setLayerManager( SmartPtr<LayerManager> layerManager );

            SmartPtr<LayerDialog> getLayerDialog() const;

            void setLayerDialog( SmartPtr<LayerDialog> layerDialog );

            SmartPtr<CollisionMaskManager> getCollisionMaskManager() const;

            void setCollisionMaskManager( SmartPtr<CollisionMaskManager> collisionMaskManager );

            SmartPtr<CollisionMaskDialog> getCollisionMaskDialog() const;

            void setCollisionMaskDialog( SmartPtr<CollisionMaskDialog> collisionMaskDialog );

            SmartPtr<TagManager> getTagManager() const;

            void setTagManager( SmartPtr<TagManager> tagManager );

            SmartPtr<TagDialog> getTagDialog() const;

            void setTagDialog( SmartPtr<TagDialog> tagDialog );

            /**
             * @brief Returns the ProfilerWindow instance.
             * @return SmartPtr<ProfilerWindow> Smart pointer to profiler window (may be null).
             */
            SmartPtr<ProfilerWindow> getProfilerWindow() const;

            /**
             * @brief Set the ProfilerWindow instance used by the manager.
             * @param profilerWindow Smart pointer to profiler window.
             */
            void setProfilerWindow( SmartPtr<ProfilerWindow> profilerWindow );

            /**
             * @brief Returns the Sound window instance.
             * @return SmartPtr<EditorWindow> Smart pointer to the sound window (may be null).
             */
            SmartPtr<EditorWindow> getSoundWindow() const;

            /**
             * @brief Set the Sound window instance used by the manager.
             * @param soundWindow Smart pointer to sound window.
             */
            void setSoundWindow( SmartPtr<EditorWindow> soundWindow );

            /**
             * @brief Returns the Particle System window instance.
             * @return SmartPtr<EditorWindow> Smart pointer to the particle system window (may be null).
             */
            SmartPtr<EditorWindow> getParticleSystemWindow() const;

            /**
             * @brief Set the Particle System window instance used by the manager.
             * @param particleSystemWindow Smart pointer to particle system window.
             */
            void setParticleSystemWindow( SmartPtr<EditorWindow> particleSystemWindow );

            /**
             * @brief Returns the Shader editor window instance.
             * @return SmartPtr<EditorWindow> Smart pointer to the shader editor window (may be null).
             */
            SmartPtr<EditorWindow> getShaderWindow() const;

            /**
             * @brief Set the Shader editor window instance used by the manager.
             * @param shaderWindow Smart pointer to shader editor window.
             */
            void setShaderWindow( SmartPtr<EditorWindow> shaderWindow );

            /**
             * @brief Returns the Package window instance.
             * @return SmartPtr<EditorWindow> Smart pointer to the package window (may be null).
             */
            SmartPtr<EditorWindow> getPackageWindow() const;

            /**
             * @brief Set the Package window instance used by the manager.
             * @param packageWindow Smart pointer to package window.
             */
            void setPackageWindow( SmartPtr<EditorWindow> packageWindow );

            /**
             * @brief Returns the Project Settings window instance.
             * @return SmartPtr<EditorWindow> Smart pointer to the project settings window (may be null).
             */
            SmartPtr<EditorWindow> getProjectSettingsWindow() const;

            /**
             * @brief Set the Project Settings window instance used by the manager.
             * @param projectSettingsWindow Smart pointer to project settings window.
             */
            void setProjectSettingsWindow( SmartPtr<EditorWindow> projectSettingsWindow );

            SmartPtr<EditorWindow> getAnimationWindow() const;

            void setAnimationWindow( SmartPtr<EditorWindow> animationWindow );

            SmartPtr<EditorWindow> getAnimationGraphWindow() const;

            void setAnimationGraphWindow( SmartPtr<EditorWindow> animationGraphWindow );

            SmartPtr<EditorWindow> getCutsceneWindow() const;

            void setCutsceneWindow( SmartPtr<EditorWindow> cutsceneWindow );

            /**
             * @brief Returns the main toolbar UI interface.
             * @return SmartPtr<ui::IUIToolbar> Smart pointer to toolbar (may be null).
             */
            SmartPtr<ui::IUIToolbar> getToolbar() const;

            /**
             * @brief Set the main toolbar UI interface.
             * @param toolbar Smart pointer to toolbar.
             */
            void setToolbar( SmartPtr<ui::IUIToolbar> toolbar );

            /**
             * @brief Returns the playmode toggle UI control.
             * @return SmartPtr<ui::IUILabelTogglePair> Smart pointer to playmode toggle (may be null).
             */
            SmartPtr<ui::IUILabelTogglePair> getPlaymodeToggle() const;

            /**
             * @brief Set the playmode toggle UI control.
             * @param playmodeToggle Smart pointer to playmode toggle.
             */
            void setPlaymodeToggle( SmartPtr<ui::IUILabelTogglePair> playmodeToggle );

            /**
             * @brief Returns the editor camera toggle UI control.
             * @return SmartPtr<ui::IUILabelTogglePair> Smart pointer to editor camera toggle (may be null).
             */
            SmartPtr<ui::IUILabelTogglePair> getEditorCameraToggle() const;

            /**
             * @brief Set the editor camera toggle UI control.
             * @param editorCameraToggle Smart pointer to editor camera toggle.
             */
            void setEditorCameraToggle( SmartPtr<ui::IUILabelTogglePair> editorCameraToggle );

            /**
             * @brief Returns the toolbar debug visibility toggle.
             * @return SmartPtr<ui::IUILabelTogglePair> Smart pointer to debug toggle (may be null).
             */
            SmartPtr<ui::IUILabelTogglePair> getToolbarShowDebugButton() const;

            /**
             * @brief Set the toolbar debug visibility toggle.
             * @param toolbarShowDebugButton Smart pointer to debug toggle.
             */
            void setToolbarShowDebugButton( SmartPtr<ui::IUILabelTogglePair> toolbarShowDebugButton );

            /**
             * @brief Returns the Asset Database window instance.
             * @return SmartPtr<EditorWindow> Smart pointer to the asset database window (may be null).
             */
            SmartPtr<EditorWindow> getAssetDatabaseWindow() const;

            /**
             * @brief Set the Asset Database window instance used by the manager.
             * @param assetDatabaseWindow Smart pointer to asset database window.
             */
            void setAssetDatabaseWindow( SmartPtr<EditorWindow> assetDatabaseWindow );

            SmartPtr<EditorWindow> getProceduralModelEditorWindow() const;

            void setProceduralModelEditorWindow( SmartPtr<EditorWindow> proceduralModelEditorWindow );

            /**
             * @brief Get the script class name associated with this UIManager.
             * @return String The class name used by the scripting system.
             */
            String getClassName() const;

            /**
             * @brief Set the script class name associated with this UIManager.
             * @param className The class name to expose to the scripting system.
             */
            void setClassName( const String &className );

            /**
             * @brief Get the script class object used by UI windows.
             * @return SmartPtr<IScriptClass> Smart pointer to script class (may be null).
             */
            SmartPtr<IScriptClass> getScriptClass() const;

            /**
             * @brief Set the script class object used by UI windows.
             * @param scriptClass Smart pointer to script class.
             */
            void setScriptClass( SmartPtr<IScriptClass> scriptClass );

            /**
             * @brief Get the script invoker used to call script functions.
             * @return SmartPtr<IScriptInvoker> Smart pointer to script invoker (may be null).
             */
            SmartPtr<IScriptInvoker> getScriptInvoker() const;

            /**
             * @brief Set the script invoker used to call script functions.
             * @param invoker Smart pointer to script invoker.
             */
            void setScriptInvoker( SmartPtr<IScriptInvoker> invoker );

            /**
             * @brief Clears the actor data clipboard used by copy/cut/paste.
             */
            static void clearClipboardActorData();

            /**
             * @brief Adds serialized actor data to the clipboard.
             * @param data Smart pointer to the actor data to store.
             */
            static void addClipboardActorData( SmartPtr<Properties> data );

            /**
             * @brief Returns the actor data currently stored in the clipboard.
             * @return Const reference to the array of serialized actor data.
             */
            static const Array<SmartPtr<Properties>> &getClipboardActorData();

            /**
             * @brief Returns true when the UIManager has been initialized and is in a usable state.
             * @return bool True if valid; false otherwise.
             */
            bool isValid() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<IEventListener> m_eventListener;      ///< General UI event listener.
            SmartPtr<IJob> m_updateSelectionJob;           ///< Job for updating selection state.
            SmartPtr<IFrameStatistics> m_frameStatistics;  ///< Optional frame statistics for profiling.
            SmartPtr<EditorWindow> m_aboutDialog;          ///< About dialog instance.

            RenderWindow *m_renderWindow = nullptr;  ///< Main render window (non-owning).
            RenderWindow *m_gameWindow = nullptr;    ///< Game preview window (non-owning).
            FileWindow *m_fileWindow = nullptr;      ///< File management window (non-owning).
            AtomicSmartPtr<ActorWindow>
                m_actorWindow;  ///< Actor management window (atomic smart pointer).
            TerrainWindow *m_terrainWindow = nullptr;        ///< Terrain editing window (non-owning).
            SmartPtr<ProjectWindow> m_projectWindow;         ///< Project management window.
            PropertiesWindow *m_propertiesWindow = nullptr;  ///< Properties editing window (non-owning).
            ApplicationFrame *m_appFrame = nullptr;          ///< Main application frame (non-owning).
            FoliageWindow *m_foliageWindow = nullptr;        ///< Foliage editing window (non-owning).
            RoadFrame *m_roadWindow = nullptr;               ///< Road editing window (non-owning).
            HoudiniWindow *m_houdiniWindow = nullptr;    ///< Houdini integration window (non-owning).
            TextureWindow *m_textureWindow = nullptr;    ///< Texture management window (non-owning).
            MaterialWindow *m_materialWindow = nullptr;  ///< Material editing window (non-owning).

            SmartPtr<ResourceDatabaseDialog> m_resourceDatabaseDialog;  ///< Resource database dialog.
            SmartPtr<ObjectBrowserDialog> m_objectBrowserDialog;        ///< Object browser dialog.
            SmartPtr<ObjectWindow> m_objectWindow;                      ///< Object management window.
            SmartPtr<SceneWindow> m_sceneWindow;                        ///< Scene editing window.
            SmartPtr<EditorWindow> m_inputManagerWindow;                ///< Input management window.
            SmartPtr<LayerManager> m_layerManager;                      ///< Central actor layer list.
            SmartPtr<LayerDialog> m_layerDialog;                    ///< Actor layer management dialog.
            SmartPtr<CollisionMaskManager> m_collisionMaskManager;  ///< Central collision mask options.
            SmartPtr<CollisionMaskDialog> m_collisionMaskDialog;    ///< Collision mask dialog.
            SmartPtr<TagManager> m_tagManager;                      ///< Central actor tag options.
            SmartPtr<TagDialog> m_tagDialog;                        ///< Actor tag management dialog.
            SmartPtr<ProfilerWindow> m_profilerWindow;              ///< Profiler window.

            SmartPtr<EditorWindow> m_animationWindow;
            SmartPtr<EditorWindow> m_animationGraphWindow;
            SmartPtr<EditorWindow> m_cutsceneWindow;            ///< Cutscene editor window.
            SmartPtr<EditorWindow> m_soundWindow;                  ///< Sound editing window.
            SmartPtr<EditorWindow> m_particleSystemWindow;         ///< Particle system window.
            SmartPtr<EditorWindow> m_shaderWindow;                 ///< Shader editor window.
            SmartPtr<EditorWindow> m_assetDatabaseWindow;          ///< Asset database window.
            SmartPtr<EditorWindow> m_packageWindow;                ///< Package management window.
            SmartPtr<EditorWindow> m_projectSettingsWindow;        ///< Project settings window.
            SmartPtr<EditorWindow> m_proceduralModelEditorWindow;  ///< Procedural model editor window.

            SmartPtr<IEventListener> m_menubarListener;             ///< Menu bar event listener.
            SmartPtr<ui::IUIToolbar> m_toolbar;                     ///< Main toolbar interface.
            SmartPtr<ui::IUILabelTogglePair> m_playmodeToggle;      ///< Play mode toggle control.
            SmartPtr<ui::IUILabelTogglePair> m_editorCameraToggle;  ///< Editor camera toggle control.
            SmartPtr<ui::IUILabelTogglePair>
                m_toolbarShowDebugButton;  ///< Debug visibility toggle control.
            SmartPtr<ui::IUILabelTogglePair>
                m_toolbarShowSceneDebugButton;  ///< Scene debug toggle control.
            SmartPtr<ui::IUILabelTogglePair> m_toolbarShowUiDebugButton;  ///< UI debug toggle control.
            SmartPtr<ui::IUILabelTogglePair>
                m_toolbarLocalTransformButton;  ///< Local transform toggle control.
            SmartPtr<ui::IUILabelTogglePair>
                m_toolbarTranslateManipulatorButton;  ///< Translate manipulator toggle.
            SmartPtr<ui::IUILabelTogglePair>
                m_toolbarRotateManipulatorButton;  ///< Rotate manipulator toggle.
            SmartPtr<ui::IUILabelTogglePair>
                m_toolbarScaleManipulatorButton;  ///< Scale manipulator toggle.

            /** @brief The class name of the script class exposed for UI scripting. */
            AtomicObject<String> m_className;

            /** @brief The script class instance used by UI windows (if any). */
            AtomicSmartPtr<IScriptClass> m_scriptClass;

            /** @brief Invoker used to call script functions from the UI. */
            AtomicSmartPtr<IScriptInvoker> m_invoker;

            /** @brief Clipboard holding serialized actor data for copy/cut/paste operations. */
            static Array<SmartPtr<Properties>> s_clipboardActorData;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // __UIManager_h__



