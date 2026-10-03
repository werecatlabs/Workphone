#ifndef ProjectAssetsWindow_h__
#define ProjectAssetsWindow_h__

#include <EditorPrerequisites.hpp>
#include "ui/EditorWindow.hpp"
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/UI/IUIDragSource.hpp>
#include <Workphone/Interface/UI/IUIDropTarget.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class ProjectAssetsWindow : public EditorWindow
        {
        public:
            enum class MenuId
            {
                AddDirector,
                AddLightingPreset,
                AddMaterial,
                AddScript,
                AddTerrainDirector,
                AddFolder,
                Cut,
                Copy,
                Paste,
                Duplicate,
                Remove,
                Import,
                Reimport,
                Refresh,
                NavigateBack,
                NavigateForward,
                NavigateUp,
                NavigateRoot,

                Count
            };

            class BuildTreeJob : public Job
            {
            public:
                BuildTreeJob();
                ~BuildTreeJob() override;

                void execute() override;

                SmartPtr<ProjectAssetsWindow> getOwner() const;

                void setOwner( SmartPtr<ProjectAssetsWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                WeakPtr<ProjectAssetsWindow> m_owner;
            };

            class TreeCtrlListener : public IEventListener
            {
            public:
                TreeCtrlListener();
                ~TreeCtrlListener() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                ProjectAssetsWindow *getOwnerPtr() const;
                SmartPtr<ProjectAssetsWindow> getOwner() const;
                void setOwner( SmartPtr<ProjectAssetsWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<ProjectAssetsWindow> m_owner;
            };

            class WindowListener : public IEventListener
            {
            public:
                WindowListener();
                ~WindowListener() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                SmartPtr<ProjectAssetsWindow> getOwner() const;
                void setOwner( SmartPtr<ProjectAssetsWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<ProjectAssetsWindow> m_owner;
            };

            class DragSource : public ui::IUIDragSource
            {
            public:
                DragSource();
                ~DragSource() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                String handleDrag( const Vector2I &position, SmartPtr<ui::IUIElement> element );

                SmartPtr<ProjectAssetsWindow> getOwner() const;
                void setOwner( SmartPtr<ProjectAssetsWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<ProjectAssetsWindow> m_owner;
            };

            class DropTarget : public ui::IUIDropTarget
            {
            public:
                DropTarget();
                ~DropTarget() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                bool handleDrop( const Vector2I &position, SmartPtr<ui::IUIElement> src,
                                 SmartPtr<ui::IUIElement> dst, const String &data );

                SmartPtr<ProjectAssetsWindow> getOwner() const;

                void setOwner( SmartPtr<ProjectAssetsWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<ProjectAssetsWindow> m_owner;
            };

            static const String fileExt;

            ProjectAssetsWindow();
            ~ProjectAssetsWindow() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            SmartPtr<ui::IUITreeNode> addFolderToTree( SmartPtr<ui::IUITreeNode> parent,
                                                       SmartPtr<IFolderExplorer> listing,
                                                       SmartPtr<IJob> buildTreeJob = nullptr );

            SmartPtr<ui::IUITreeNode> addMediaFolderToTree( SmartPtr<ui::IUITreeNode> parent,
                                                            SmartPtr<IFolderExplorer> listing,
                                                            SmartPtr<IJob> buildTreeJob = nullptr );

            SmartPtr<ui::IUITreeNode> addFileToTree( SmartPtr<ui::IUITreeNode> parent,
                                                     const String &filePath );

            void build();

            SmartPtr<ui::IUIWindow> getParentWindow() const override;
            void setParentWindow( SmartPtr<ui::IUIWindow> parentWindow ) override;

            bool isValid() const override;

            SmartPtr<ui::IUITreeCtrl> getTree() const;

            void setTree( SmartPtr<ui::IUITreeCtrl> tree );

            Parameter handleApplicationEvent( EventType eventType, hash_type eventValue,
                                              const Array<Parameter> &arguments,
                                              SmartPtr<ISharedObject> sender,
                                              SmartPtr<ISharedObject> object,
                                              SmartPtr<IEvent> event ) override;

            SmartPtr<IJob> getBuildTreeJob() const;

            void setBuildTreeJob( SmartPtr<IJob> buildTreeJob );

            WP_CLASS_REGISTER_DECL;

        protected:
            void buildTree( SmartPtr<IJob> buildTreeJob = nullptr );

            void handleTreeSelectionActivated( SmartPtr<ui::IUITreeNode> node );
            void handleTreeNodeDoubleClicked( SmartPtr<ui::IUITreeNode> node );

            String getSelectedPath() const;
            String getSelectedFolderPath() const;

            bool copyAsset( const String &sourcePath, const String &targetFolder,
                            bool generateUniqueName );
            bool moveAsset( const String &sourcePath, const String &targetFolder );
            bool pasteClipboard();
            bool duplicateSelectedAsset();
            bool refreshFolder( const String &folderPath );
            void refreshFolders( const Array<String> &folderPaths );

            String getAssetsRootPath() const;
            bool navigateToFolder( const String &folderPath, bool addToHistory = true );
            void navigateBack();
            void navigateForward();
            void navigateUp();
            void updateNavigationControls();

            void applySearchFilter( const String &filter );
            bool updateNodeFilter( SmartPtr<ui::IUITreeNode> node, const String &filter );
            void updateThumbnailPreview( const String &filePath );

            static String getAssetIcon( const String &path, bool isFolder );
            static String getAssetDisplayName( const String &path, bool isFolder );

            SmartPtr<ui::IUITreeNode> findNodeByPath( const String &path ) const;
            void removeTreeNode( SmartPtr<ui::IUITreeNode> node );

            SmartPtr<IJob> m_buildTreeJob;

            SmartPtr<ui::IUIWindow> m_window;
            SmartPtr<ui::IUIWindow> m_sceneWindow;

            SmartPtr<ui::IUITreeCtrl> m_tree;
            SmartPtr<IEventListener> m_treeListener;

            SmartPtr<ui::IUIButton> m_backButton;
            SmartPtr<ui::IUIButton> m_forwardButton;
            SmartPtr<ui::IUIButton> m_upButton;
            SmartPtr<ui::IUIButton> m_rootButton;
            SmartPtr<ui::IUIButton> m_refreshButton;
            SmartPtr<ui::IUITextEntry> m_searchEntry;
            SmartPtr<ui::IUIText> m_breadcrumbText;
            SmartPtr<ui::IUIImage> m_thumbnailPreview;
            SmartPtr<ui::IUIText> m_thumbnailCaption;

            SmartPtr<IEventListener> m_menuListener;
            SmartPtr<ui::IUIMenu> m_applicationMenu;
            SmartPtr<ui::IUIMenu> m_applicationAddMenu;

            SmartPtr<ISharedObject> m_selectedObject;

            SmartPtr<ISharedObject> m_selectedEntity;

            String m_clipboardPath;
            bool m_clipboardCut = false;

            Array<String> m_navigationHistory;
            s32 m_navigationHistoryIndex = -1;
            String m_currentFolder;
            String m_searchFilter;

            //SmartPtr<TemplateFilter> m_parentFilter;

            SmartPtr<ui::IUIWindow> m_parentWindow;

            std::map<String, bool> treeState;
        };

        inline ProjectAssetsWindow *ProjectAssetsWindow::TreeCtrlListener::getOwnerPtr() const
        {
            return m_owner.get();
        }

    }  // end namespace editor
}  // namespace workphone

#endif  // EntityWindow_h__
