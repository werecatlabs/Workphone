#ifndef __EditorManager_h__
#define __EditorManager_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Interface/System/IEditorManager.hpp>
#include <Workphone/Core/HashMap.hpp>

namespace workphone
{
    namespace editor
    {
        /**
         * @brief Central coordinator for editor state, project management, and editing tools.
         *
         * The editor manager owns the active project, shared UI state, tool manipulators,
         * and runtime scene snapshot data used by play mode. It acts as the primary
         * entry point for project loading, file import, editor preferences, and tool
         * state updates.
         */
        class EditorManager : public IEditorManager
        {
        public:
            /** @brief Constructs the editor manager with default state. */
            EditorManager();
            /** @brief Destroys the editor manager and releases owned state. */
            ~EditorManager() override;

            /** @brief Loads editor state from serialized data. */
            void load( SmartPtr<ISharedObject> data ) override;
            /** @brief Unloads any serialized state associated with the editor. */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @brief Imports project assets into the current workspace. */
            void importAssets() override;

            /** @brief Loads a project from the specified file path. */
            void loadProject( const String &filePath );

            /** @return Absolute path to the current project file or directory. */
            String getProjectPath() const override;

            /** @brief Sets the active project path. */
            /** @param path Absolute or relative path to the project file or directory. */
            void setProjectPath( const String &path ) override;

            /** @return Path used for editor cache and generated output files. */
            String getCachePath() const override;

            /** @brief Sets the cache directory used by the editor for generated data. */
            /** @param path Directory path for editor cache files. */
            void setCachePath( const String &path ) override;

            /** @return Project manager instance associated with this editor. */
            SmartPtr<ProjectManager> getProjectManager() const;
            /** @brief Assigns the project manager for the editor. */
            void setProjectManager( SmartPtr<ProjectManager> projectManager );

            /** @return Currently active project instance. */
            SmartPtr<Project> getProject() const;
            /** @brief Sets the current project for the editor. */
            void setProject( SmartPtr<Project> project );

            /** @return UI manager used by the editor. */
            SmartPtr<UIManager> getUI() const;
            /** @brief Assigns the UI manager used by the editor. */
            void setUI( SmartPtr<UIManager> ui );

            /** @return Decal cursor used for placement and painting tools. */
            SmartPtr<render::IDecalCursor> getDecalCursor() const;
            /** @brief Sets the decal cursor used by editor painting tools. */
            void setDecalCursor( SmartPtr<render::IDecalCursor> decalCursor );

            /** @return True if terrain editing mode is currently active. */
            bool getEditTerrain() const;
            /** @brief Enables or disables terrain editing mode. */
            void setEditTerrain( bool editTerrain );

            /** @return True if foliage editing mode is currently active. */
            bool getEditFoliage() const;
            /** @brief Enables or disables foliage editing mode. */
            void setEditFoliage( bool editFoliage );

            /** @return True if the current file has been saved. */
            bool getFileSaved() const;
            /** @brief Marks whether the current file has been saved. */
            void setFileSaved( bool fileSaved );

            /**
             * @brief Captures the active scene before entering play mode.
             *
             * This preserves the editor scene for later restoration after play mode ends.
             * @param scene Scene to capture for the play mode snapshot.
             * @return True if the scene was captured successfully.
             */
            bool capturePlayModeScene( SmartPtr<scene::IGameScene> scene );
            /** @return Scene snapshot captured for play mode, if any. */
            SmartPtr<scene::IGameScene> getPlayModeScene() const;
            /** @return Serialized snapshot of the play mode scene. */
            SmartPtr<ISharedObject> getPlayModeSceneData() const;
            /** @brief Restores the scene captured before entering play mode. */
            SmartPtr<scene::IGameScene> restorePlayModeScene();
            /** @brief Clears any recorded play mode scene state. */
            void clearPlayModeScene();

            /** @brief Previews an asset in the editor from the given path. */
            void previewAsset( const String &path );

            /** @return Current translate manipulator gizmo. */
            SmartPtr<TranslateManipulator> getTranslateManipulator() const;
            /** @brief Sets the translate manipulator used for object movement. */
            void setTranslateManipulator( SmartPtr<TranslateManipulator> translateManipulator );

            /** @return Current rotate manipulator gizmo. */
            SmartPtr<RotateManipulator> getRotateManipulator() const;
            /** @brief Sets the rotate manipulator used for object rotation. */
            void setRotateManipulator( SmartPtr<RotateManipulator> rotateManipulator );

            /** @return Current scale manipulator gizmo. */
            SmartPtr<ScaleManipulator> getScaleManipulator() const;
            /** @brief Sets the scale manipulator used for object scaling. */
            void setScaleManipulator( SmartPtr<ScaleManipulator> scaleManipulator );

            /** @return True if debug overlays are currently shown. */
            bool getShowDebug() const override;

            /** @brief Enables or disables editor debug display. */
            void setShowDebug( bool showDebug ) override;

            /** @return True if transforms are applied in local space. */
            bool isTransformLocal() const;

            /** @brief Sets whether transform operations occur in local or world space. */
            void setTransformLocal( bool transformLocal );

            /** @brief Refreshes the transform UI to match the current transform mode. */
            void refreshTransformUI() override;

            /** @return True if scene debug drawing is enabled. */
            bool getDrawSceneDebug() const;
            /** @brief Enables or disables debug drawing for the scene view. */
            void setDrawSceneDebug( bool drawSceneDebug );

            /** @return True if UI debug drawing is enabled. */
            bool getDrawUiDebug() const;
            /** @brief Enables or disables debug drawing for the UI overlay. */
            void setDrawUiDebug( bool drawUiDebug );

            /** @return Singleton instance of the editor manager. */
            static SmartPtr<EditorManager> getSingleton();
            /** @return Raw singleton pointer for the editor manager. */
            static EditorManager *getSingletonPtr();
            /** @brief Sets the singleton instance for the editor manager. */
            static void setSingletonPtr( SmartPtr<EditorManager> editorManager );

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Translate gizmo used to move objects in the viewport. */
            SmartPtr<TranslateManipulator> m_translateManipulator;

            /** @brief Rotate gizmo used to rotate objects in the viewport. */
            SmartPtr<RotateManipulator> m_rotateManipulator;

            /** @brief Scale gizmo used to resize objects in the viewport. */
            SmartPtr<ScaleManipulator> m_scaleManipulator;

            /** @brief Manager responsible for project creation, loading, and metadata. */
            SmartPtr<ProjectManager> m_projectManager;

            /** @brief Active project instance for the editor session. */
            AtomicSmartPtr<Project> m_project;

            /** @brief User interface manager for the editor. */
            SmartPtr<UIManager> m_ui;

            /** @brief Cursor used for decal placement and painting workflows. */
            SmartPtr<render::IDecalCursor> m_decalCursor;

            /** @brief True if debug information is visible in the editor. */
            bool m_showDebug = false;

            /** @brief True while terrain editing mode is active. */
            bool m_editTerrain = false;

            /** @brief True while foliage editing mode is active. */
            bool m_editFoliage = false;

            /** @brief True if the current file has been saved successfully. */
            bool m_fileSaved = false;

            /** @brief True if transform operations use local space rather than world space. */
            atomic_bool m_isTransformLocal = false;

            /** @brief True if scene debug rendering is enabled. */
            bool m_drawSceneDebug = false;
            /** @brief True if UI debug rendering is enabled. */
            bool m_drawUiDebug = false;

            /** @brief True if physics simulation is enabled for the editor. */
            atomic_bool m_enablePhysics = false;

            /** @brief Grid used by the editor to aid placement and alignment. */
            EditorGrid *m_editorGrid = nullptr;

            /** @brief Path to the current project file or directory. */
            String m_projectPath;

            /** @brief Path used for editor cache and intermediate generated data. */
            String m_cachePath;

            /** @brief Snapshot of the scene preserved before entering play mode. */
            SmartPtr<scene::IGameScene> m_playModeScene;
            /** @brief Serialized form of the play mode scene snapshot. */
            SmartPtr<ISharedObject> m_playModeSceneData;
            /** @brief File path for the captured play mode scene snapshot. */
            String m_playModeSceneFilePath;
            /** @brief Label associated with the captured play mode scene snapshot. */
            String m_playModeSceneLabel;
            /** @brief True if the captured play mode scene has been saved. */
            bool m_playModeFileSaved = false;

            /** @brief Singleton instance of the editor manager. */
            static SmartPtr<EditorManager> m_singleton;
        };

        inline bool EditorManager::getShowDebug() const
        {
            return m_showDebug;
        }

    }  // end namespace editor
}  // namespace workphone

#endif  // __EditorManager_h__
