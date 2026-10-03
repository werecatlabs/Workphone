#ifndef __EditorManager_h__
#define __EditorManager_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Interface/System/IEditorManager.hpp>
#include <Workphone/Core/HashMap.hpp>

namespace workphone
{
    namespace editor
    {
        class EditorManager : public IEditorManager
        {
        public:
            EditorManager();
            ~EditorManager() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            /** Import the assets. */
            void importAssets() override;

            void loadProject( const String &filePath );

            /** Get the project path.
            @return A string with the project path.
            */
            String getProjectPath() const override;

            /** Set the project path.
            @param path A string to the project path.
            */
            void setProjectPath( const String &path ) override;

            /** Get the cache path.
            @return A string with the cache path.
            */
            String getCachePath() const override;

            /** Set the cache path.
            @param path A string to the cache path.
            */
            void setCachePath( const String &path ) override;

            SmartPtr<ProjectManager> getProjectManager() const;
            void setProjectManager( SmartPtr<ProjectManager> projectManager );

            SmartPtr<Project> getProject() const;
            void setProject( SmartPtr<Project> project );

            SmartPtr<UIManager> getUI() const;
            void setUI( SmartPtr<UIManager> ui );

            SmartPtr<render::IDecalCursor> getDecalCursor() const;
            void setDecalCursor( SmartPtr<render::IDecalCursor> decalCursor );

            bool getEditTerrain() const;
            void setEditTerrain( bool editTerrain );

            bool getEditFoliage() const;
            void setEditFoliage( bool editFoliage );

            bool getFileSaved() const;
            void setFileSaved( bool fileSaved );

            /** Capture the editor scene before the first play request in a session. */
            bool capturePlayModeScene( SmartPtr<scene::IGameScene> scene );
            SmartPtr<scene::IGameScene> getPlayModeScene() const;
            SmartPtr<ISharedObject> getPlayModeSceneData() const;
            SmartPtr<scene::IGameScene> restorePlayModeScene();
            void clearPlayModeScene();

            void previewAsset( const String &path );

            SmartPtr<TranslateManipulator> getTranslateManipulator() const;
            void setTranslateManipulator( SmartPtr<TranslateManipulator> translateManipulator );

            SmartPtr<RotateManipulator> getRotateManipulator() const;
            void setRotateManipulator( SmartPtr<RotateManipulator> rotateManipulator );

            SmartPtr<ScaleManipulator> getScaleManipulator() const;
            void setScaleManipulator( SmartPtr<ScaleManipulator> scaleManipulator );

            bool getShowDebug() const override;

            void setShowDebug( bool showDebug ) override;

            bool isTransformLocal() const;

            void setTransformLocal( bool transformLocal );

            void refreshTransformUI() override;

            bool getDrawSceneDebug() const;
            void setDrawSceneDebug( bool drawSceneDebug );

            bool getDrawUiDebug() const;
            void setDrawUiDebug( bool drawUiDebug );

            static SmartPtr<EditorManager> getSingleton();
            static EditorManager *getSingletonPtr();
            static void setSingletonPtr( SmartPtr<EditorManager> editorManager );

            WP_CLASS_REGISTER_DECL;

        protected:
            /// the translate gizmo
            SmartPtr<TranslateManipulator> m_translateManipulator;

            /// the rotate gizmo
            SmartPtr<RotateManipulator> m_rotateManipulator;

            /// the scale gizmo
            SmartPtr<ScaleManipulator> m_scaleManipulator;

            ///
            SmartPtr<ProjectManager> m_projectManager;

            ///
            AtomicSmartPtr<Project> m_project;

            ///
            SmartPtr<UIManager> m_ui;

            ///
            SmartPtr<render::IDecalCursor> m_decalCursor;

            ///
            bool m_showDebug = false;

            ///
            bool m_editTerrain = false;

            ///
            bool m_editFoliage = false;

            ///
            bool m_fileSaved = false;

            atomic_bool m_isTransformLocal = false;

            bool m_drawSceneDebug = false;
            bool m_drawUiDebug = false;

            ///
            atomic_bool m_enablePhysics = false;

            ///
            EditorGrid *m_editorGrid = nullptr;

            /// The project path.
            String m_projectPath;

            /// The cache path.
            String m_cachePath;

            /// The primary-thread play/stop jobs own this snapshot until stop succeeds.
            SmartPtr<scene::IGameScene> m_playModeScene;
            SmartPtr<ISharedObject> m_playModeSceneData;
            String m_playModeSceneFilePath;
            String m_playModeSceneLabel;
            bool m_playModeFileSaved = false;

            /// A pointer to the singleton
            static SmartPtr<EditorManager> m_singleton;
        };

        inline bool EditorManager::getShowDebug() const
        {
            return m_showDebug;
        }

    }  // end namespace editor
}  // namespace workphone

#endif  // AppRoot_h__
