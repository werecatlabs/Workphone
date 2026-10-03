#ifndef MeshViewer_h__
#define MeshViewer_h__

#include "MeshViewerPrerequisites.hpp"
#include <Workphone/Application.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <chrono>

namespace workphone
{
    namespace viewer
    {
        /**
         * @brief Standalone production mesh viewer and inspection tool.
         *
         * The viewer is intentionally lightweight: the core path is the same
         * as the original sample viewer, but the UI is now organised like a small
         * production validation tool for artists, technical artists and engine
         * programmers. Expensive or engine-specific operations are exposed as
         * action hooks so they can be wired to import/build/validation systems later.
         */
        class MeshViewer : public core::Application
        {
        public:
            /**
             * @brief Menu and widget identifiers used by the viewer UI.
             */
            enum class ElementId
            {
                Open,
                OpenRecent,
                ReloadMesh,
                CloseMesh,
                SaveReport,
                ExportScreenshot,
                ExportThumbnail,
                Exit,

                FrameMesh,
                ResetCamera,
                ViewFront,
                ViewSide,
                ViewTop,
                ViewIso,
                AutoRotate,
                Turntable,

                ToggleGrid,
                ToggleAxes,
                ToggleBounds,
                ToggleWireframe,
                ToggleNormals,
                ToggleTangents,
                ToggleBinormals,
                ToggleSkeleton,
                ToggleSkinWeights,
                ToggleColliders,
                ToggleOverdraw,
                ToggleUVChecker,
                ToggleStats,
                ToggleInspector,
                ToggleAssetPanel,
                ToggleMaterialPanel,

                RenderLit,
                RenderUnlit,
                RenderWireframe,
                RenderNormals,
                RenderTangents,
                RenderUV0,
                RenderUV1,
                RenderVertexColour,
                RenderLightmapUV,
                RenderSkinWeights,

                LightingStudio,
                LightingOutdoor,
                LightingIndoor,
                LightingNeutral,
                LightingCustom,

                ValidateMesh,
                ValidateMaterials,
                ValidateSkeleton,
                ValidateLODs,
                RecalculateBounds,
                RebuildTangents,
                GenerateLODs,
                OptimiseMesh,
                BuildCollision,
                GeneratePreview,
                BakeThumbnail,
                CopyMeshReport,

                SearchText,
                PathText,
                RenderModeDropdown,
                LodDropdown,
                MaterialVariantDropdown,
                AnimationDropdown,
                SkeletonDropdown,
                Exposure,
                CameraSpeed,
                NearClip,
                FarClip,
                LodBias,
                WireThickness,
                NormalLength,
                TangentLength,

                Count
            };

            MeshViewer();
            ~MeshViewer() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            SmartPtr<scene::IGameActor> getMeshActor() const;
            void setMeshActor( SmartPtr<scene::IGameActor> meshActor );

            /**
             * @brief Loads a mesh from a file path and refreshes the production UI.
             */
            void loadMeshFromFile( const String &filePath );

            /**
             * @brief Sets the mesh path to open on startup (e.g. from the command line).
             */
            static void setStartupMeshPath( const String &filePath );

            WP_CLASS_REGISTER_DECL;

        private:
            class CUIMenuBarListener : public IEventListener
            {
            public:
                CUIMenuBarListener();
                ~CUIMenuBarListener() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                SmartPtr<MeshViewer> getOwner() const;
                void setOwner( SmartPtr<MeshViewer> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                void handleOpenFile( SmartPtr<MeshViewer> owner, SmartPtr<IFileSystem> fileSystem );
                void handleExit();

                WeakPtr<MeshViewer> m_owner;
            };

            void createCamera() override;
            void createPlugins() override;
            void createUI() override;
            void createRenderWindow() override;
            void iterate() override;

            void setupUI();
            void setupMenuBar();
            void setupWorkspaceUI();
            void setupToolbar();
            void setupViewportPanel();
            void setupAssetPanel();
            void setupInspectorPanel();
            void setupRenderPanel();
            void setupValidationPanel();
            void setupStatusBar();

            void setupCamera();
            void setupViewport();
            void centerCameraOnMesh();
            void applyCameraView( ElementId view );

            void registerControl( SmartPtr<ui::IUIElement> element );
            void handleAction( ElementId action );
            void handleDropdown( ElementId action, s32 selectedIndex );
            void handleSlider( ElementId action, real_Num value );
            void handleToggle( ElementId action, bool value );
            bool getOverlayFlag( ElementId action ) const;
            void setOverlayFlag( ElementId action, bool value );
            void restoreDefaultMaterials();
            void openRecentFile( size_t index );
            void setStatus( const String &status );
            void refreshMeshInfo();
            void refreshRenderStateText();
            void refreshValidationText();
            void updateViewportOverlays();
            void applyRenderMode( s32 mode );
            void applyLightingPreset( ElementId preset );
            void updateMeshMaterialVariant( s32 variant );

            void gatherMeshStats();
            String getCurrentMeshReport() const;
            String getValidationReport() const;
            void runValidation( ElementId validationType );
            void saveReportToFile( const String &filePath );
            void exportScreenshot( const String &filePath, const Vector2I &size );
            void exportThumbnail( const String &filePath );

            void addRecentFile( const String &filePath );
            void loadRecentFiles();
            void saveRecentFiles();
            void rebuildRecentMenu();
            bool isSupportedMeshFormat( const String &filePath ) const;
            void openMeshFileDialog();

            void queueScreenshot( const String &filePath, const Vector2I &size );
            void processPendingScreenshot();

            SmartPtr<ui::IUIApplication> m_application;
            SmartPtr<ui::IUIRenderWindow> m_renderWindow;
            SmartPtr<IEventListener> m_menubarListener;

            SmartPtr<render::IGraphicsWindow> m_window;
            SmartPtr<render::IGraphicsScene> m_sceneManager;
            SmartPtr<render::IGraphicsCamera> m_mainCamera;
            SmartPtr<render::IGraphicsSceneNode> m_mainCameraSceneNode;
            SmartPtr<render::IGraphicsSceneNode> m_cameraSceneNode;
            SmartPtr<render::IViewport> m_mainViewport;
            SmartPtr<render::ITexture> m_renderTarget;
            SmartPtr<scene::IGameActor> m_cameraActor;
            SmartPtr<scene::IGameActor> m_meshActor;

            // Production viewer chrome.
            SmartPtr<ui::IUIWindow> m_workspaceWindow;
            SmartPtr<ui::IUIWindow> m_toolbarWindow;
            SmartPtr<ui::IUIWindow> m_viewportPanel;
            SmartPtr<ui::IUIWindow> m_assetPanel;
            SmartPtr<ui::IUIWindow> m_inspectorPanel;
            SmartPtr<ui::IUIWindow> m_renderPanel;
            SmartPtr<ui::IUIWindow> m_validationPanel;
            SmartPtr<ui::IUIWindow> m_statusPanel;
            SmartPtr<ui::IUIMenu> m_recentFilesMenu;

            SmartPtr<ui::IUITextEntry> m_pathText;
            SmartPtr<ui::IUITextEntry> m_searchText;
            SmartPtr<ui::IUIText> m_meshInfoText;
            SmartPtr<ui::IUIText> m_renderStateText;
            SmartPtr<ui::IUIText> m_validationText;
            SmartPtr<ui::IUIText> m_statusText;

            String m_currentMeshPath;
            s32 m_currentRenderMode = 0;
            s32 m_currentMaterialVariant = 0;
            s32 m_currentLod = 0;

            bool m_showGrid = true;
            bool m_showAxes = true;
            bool m_showBounds = true;
            bool m_showWireframe = false;
            bool m_showNormals = false;
            bool m_showTangents = false;
            bool m_showBinormals = false;
            bool m_showSkeleton = false;
            bool m_showSkinWeights = false;
            bool m_showColliders = false;
            bool m_showOverdraw = false;
            bool m_showUVChecker = false;
            bool m_showStats = true;
            bool m_autoRotate = false;

            f32 m_cameraSpeed = 1.0f;
            f32 m_nearClip = 0.01f;
            f32 m_farClip = 1000.0f;
            f32 m_exposure = 0.0f;
            f32 m_lodBias = 0.0f;
            f32 m_wireThickness = 1.0f;
            f32 m_normalLength = 0.25f;
            f32 m_tangentLength = 0.25f;

            struct MeshStats
            {
                u32 subMeshCount = 0;
                u32 vertexCount = 0;
                u32 indexCount = 0;
                u32 triangleCount = 0;
                u32 materialCount = 0;
                u32 animationCount = 0;
                u32 boneCount = 0;
                bool hasSkeleton = false;
                bool hasSharedVertices = false;
                bool hasUvChannel0 = false;
                bool hasUvChannel1 = false;
                bool hasVertexColours = false;
                bool hasLightmapUvs = false;
                bool hasNormals = false;
                bool hasTangents = false;
                AABB3<real_Num> bounds;
                Array<String> materialNames;
                Array<String> warnings;
            };

            MeshStats m_meshStats;
            Array<String> m_recentFiles;
            String m_recentFilesPath;

            bool m_pendingScreenshot = false;
            Vector2I m_pendingScreenshotSize;
            String m_pendingScreenshotPath;
            bool m_pendingThumbnail = false;
            String m_pendingThumbnailPath;

            Array<SmartPtr<ui::IUIElement>> m_recentFileItems;
            Array<String> m_originalMaterialNames;
            ElementId m_lastValidationType = ElementId::ValidateMesh;
            std::chrono::steady_clock::time_point m_lastUpdateTime;
            f32 m_turntableSpeed = 0.5f;

            static String s_startupMeshPath;
        };
    }  // namespace viewer
}  // namespace workphone

#endif  // MeshViewer_h__
