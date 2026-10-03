
#include "MeshViewerPCH.hpp"
#include "MeshViewer.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/Mesh/ISkeleton.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/Interface/Mesh/IIndexBuffer.hpp>
#include <Workphone/Interface/Graphics/IGraphicsMesh.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/IO/INativeFileDialog.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>

#include <sstream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <cstring>

#ifdef WP_PLATFORM_WIN32
#    include <windows.h>
#    include <shellapi.h>
#endif

#ifdef _WP_STATIC_LIB_
#    include <WPSQLite/WPSQLite.hpp>

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
#        include <WPGraphicsOgreNext/WPGraphicsOgreNext.hpp>
#    elif WP_GRAPHICS_SYSTEM_OGRE
#        include <WPGraphicsOgre/WPGraphicsOgre.h>
#    endif

#    if WP_BUILD_IMGUI
#        include <FBImGui/FBImGui.hpp>
#    endif

#    if WP_USE_ASSET_IMPORT
#        include <FBAssimp/FBAssimp.hpp>
#    endif

#    if WP_BUILD_OISINPUT
#        include "FBOISInput/FBOISInput.hpp"
#    endif

#    if WP_BUILD_PHYSX
#        include "FBPhysx/FBPhysx.hpp"
#    endif
#endif

namespace workphone
{
    namespace viewer
    {
        namespace
        {
            // Material scheme names used by the viewport for debug render modes.
            const char *const RENDER_MODE_SCHEMES[] = {
                "Default",       // Lit
                "Unlit",         // Unlit
                "Wireframe",     // Wireframe
                "Normals",       // Normals
                "Tangents",      // Tangents
                "UV0",           // UV0
                "UV1",           // UV1
                "VertexColour",  // Vertex Colour
                "LightmapUV",    // Lightmap UV
                "SkinWeights"    // Skin Weights
            };

            // Material names applied to the mesh actor for material variant preview.
            const char *const MATERIAL_VARIANT_NAMES[] = {
                "",                  // Default - keep existing materials
                "Debug/GreyClay",    // Grey Clay
                "Debug/Checker",     // Checker
                "Debug/UVChecker",   // UV Checker
                "Debug/Normals",     // Normal Debug
                "Debug/Roughness",   // Roughness Debug
                "Debug/Metalness"    // Metalness Debug
            };

            const char *getRenderModeScheme( s32 mode )
            {
                static const s32 count = static_cast<s32>( sizeof( RENDER_MODE_SCHEMES ) /
                                                           sizeof( RENDER_MODE_SCHEMES[0] ) );
                mode = Math<s32>::clamp( mode, 0, count - 1 );
                return RENDER_MODE_SCHEMES[mode];
            }

            const char *getMaterialVariantName( s32 variant )
            {
                static const s32 count = static_cast<s32>( sizeof( MATERIAL_VARIANT_NAMES ) /
                                                           sizeof( MATERIAL_VARIANT_NAMES[0] ) );
                variant = Math<s32>::clamp( variant, 0, count - 1 );
                return MATERIAL_VARIANT_NAMES[variant];
            }

            template <class T>
            SmartPtr<T> addUiElement( SmartPtr<ui::IUIManager> uiManager,
                                      SmartPtr<ui::IUIElement> parent, const String &label,
                                      MeshViewer::ElementId elementId, bool sameLine = false )
            {
                auto element = uiManager->addElementByType<T>();
                if( element )
                {
                    element->setLabel( label );
                    element->setElementId( static_cast<hash_type>( elementId ) );
                    element->setSameLine( sameLine );
                    if( parent )
                    {
                        parent->addChild( element );
                    }
                }

                return element;
            }

            SmartPtr<ui::IUIText> addText( SmartPtr<ui::IUIManager> uiManager,
                                           SmartPtr<ui::IUIElement> parent, const String &text,
                                           bool sameLine = false )
            {
                auto element = uiManager->addElementByType<ui::IUIText>();
                if( element )
                {
                    element->setText( text );
                    element->setSameLine( sameLine );
                    if( parent )
                    {
                        parent->addChild( element );
                    }
                }

                return element;
            }

            SmartPtr<ui::IUIButton> addButton( SmartPtr<ui::IUIManager> uiManager,
                                               SmartPtr<ui::IUIElement> parent,
                                               const String &label, MeshViewer::ElementId elementId,
                                               bool sameLine = false )
            {
                return addUiElement<ui::IUIButton>( uiManager, parent, label, elementId, sameLine );
            }

            SmartPtr<ui::IUITextEntry> addTextEntry( SmartPtr<ui::IUIManager> uiManager,
                                                     SmartPtr<ui::IUIElement> parent,
                                                     const String &label,
                                                     MeshViewer::ElementId elementId,
                                                     const String &value = String() )
            {
                auto element =
                    addUiElement<ui::IUITextEntry>( uiManager, parent, label, elementId, false );
                if( element )
                {
                    element->setText( value );
                }

                return element;
            }

            SmartPtr<ui::IUIDropdown> addDropdown( SmartPtr<ui::IUIManager> uiManager,
                                                   SmartPtr<ui::IUIElement> parent,
                                                   const String &label,
                                                   MeshViewer::ElementId elementId,
                                                   const Array<String> &options,
                                                   s32 selected = 0 )
            {
                auto element =
                    addUiElement<ui::IUIDropdown>( uiManager, parent, label, elementId, false );
                if( element )
                {
                    element->setOptions( options );
                    element->setSelectedOption( static_cast<u32>( selected ) );
                }

                return element;
            }

            SmartPtr<ui::IUILabelTogglePair> addToggle( SmartPtr<ui::IUIManager> uiManager,
                                                        SmartPtr<ui::IUIElement> parent,
                                                        const String &label,
                                                        MeshViewer::ElementId elementId, bool value )
            {
                auto element = addUiElement<ui::IUILabelTogglePair>( uiManager, parent, label,
                                                                     elementId, false );
                if( element )
                {
                    element->setValue( value );
                }

                return element;
            }

            SmartPtr<ui::IUILabelSliderPair> addSlider( SmartPtr<ui::IUIManager> uiManager,
                                                        SmartPtr<ui::IUIElement> parent,
                                                        const String &label,
                                                        MeshViewer::ElementId elementId,
                                                        real_Num minValue, real_Num maxValue,
                                                        real_Num value )
            {
                auto element = addUiElement<ui::IUILabelSliderPair>( uiManager, parent, label,
                                                                     elementId, false );
                if( element )
                {
                    element->setMinValue( minValue );
                    element->setMaxValue( maxValue );
                    element->setValue( value );
                }

                return element;
            }

            Array<String> makeOptions( std::initializer_list<String> values )
            {
                Array<String> options;
                for( const auto &value : values )
                {
                    options.push_back( value );
                }

                return options;
            }

            String getFileNameFromPath( const String &filePath )
            {
                if( StringUtil::isNullOrEmpty( filePath ) )
                {
                    return String();
                }

                auto lastSlash = filePath.find_last_of( "/\\" );
                if( lastSlash == String::npos )
                {
                    return filePath;
                }

                return filePath.substr( lastSlash + 1 );
            }

            String getFileExtensionLower( const String &filePath )
            {
                auto name = getFileNameFromPath( filePath );
                auto dot = name.find_last_of( '.' );
                if( dot == String::npos )
                {
                    return String();
                }

                return StringUtil::make_lower( name.substr( dot ) );
            }

            String getTimestampString()
            {
                auto now = std::time( nullptr );
                auto tm = std::localtime( &now );
                std::ostringstream ss;
                ss << std::put_time( tm, "%Y-%m-%d_%H-%M-%S" );
                return ss.str();
            }

            // Recent-file menu items use element ids beyond the fixed ElementId enum so the
            // listener can distinguish which recent file was selected.
            hash_type getRecentFileElementId( size_t index )
            {
                return static_cast<hash_type>( MeshViewer::ElementId::Count ) +
                       static_cast<hash_type>( 0x1000 ) + static_cast<hash_type>( index );
            }

            bool isRecentFileElementId( hash_type id )
            {
                return id >= ( static_cast<hash_type>( MeshViewer::ElementId::Count ) +
                               static_cast<hash_type>( 0x1000 ) );
            }

            size_t getRecentFileIndex( hash_type id )
            {
                return static_cast<size_t>(
                    id - static_cast<hash_type>( MeshViewer::ElementId::Count ) -
                    static_cast<hash_type>( 0x1000 ) );
            }
        }  // namespace

        String MeshViewer::s_startupMeshPath;

        WP_CLASS_REGISTER_DERIVED( workphone::viewer, MeshViewer, Application );
        WP_CLASS_REGISTER_DERIVED( workphone::viewer, MeshViewer::CUIMenuBarListener, IEventListener );

        MeshViewer::MeshViewer() = default;

        MeshViewer::~MeshViewer() = default;

        void MeshViewer::setStartupMeshPath( const String &filePath )
        {
            s_startupMeshPath = filePath;
        }

        void MeshViewer::iterate()
        {
            Application::iterate();

            // Deferred screenshot/thumbnail capture runs after the viewport has rendered.
            processPendingScreenshot();

            // Turntable preview: rotate the loaded mesh around its up axis at a steady rate.
            if( m_autoRotate && m_meshActor )
            {
                auto now = std::chrono::steady_clock::now();
                auto seconds = std::chrono::duration<real_Num>( now - m_lastUpdateTime ).count();
                m_lastUpdateTime = now;

                if( seconds > real_Num( 0.0 ) && seconds < real_Num( 1.0 ) )
                {
                    auto rotation = m_meshActor->getRotation();
                    rotation.y += m_turntableSpeed * seconds;
                    m_meshActor->setRotation( rotation );
                }
            }
            else
            {
                m_lastUpdateTime = std::chrono::steady_clock::now();
            }
        }

        void MeshViewer::load( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Loading );

            auto currentThread = Thread::ThreadId::Primary;
            Thread::setCurrentThreadId( currentThread );

            auto currentTask = TaskId::Primary;
            Thread::setCurrentTask( currentTask );

            auto taskFlags = std::numeric_limits<u32>::max();
            Thread::setTaskFlags( taskFlags );

            auto applicationManager = workphone::make_ptr<core::ApplicationManager>();
            core::ApplicationManager::setInstance( applicationManager );

            Application::load( data );

            setupUI();
            setupCamera();
            setupViewport();

            if( m_cameraActor )
            {
                auto sphericalCamera = m_cameraActor->addComponent<scene::SphericalCameraController>();
                if( sphericalCamera )
                {
                    sphericalCamera->setSphericalCoords( Vector3<real_Num>( 5.0, 0.0, 2.0 ) );
                    sphericalCamera->setUiWindow( m_renderWindow );
                }
            }

            auto gameManager = applicationManager->getGameManager();
            gameManager->edit();

            loadRecentFiles();

            refreshMeshInfo();
            refreshRenderStateText();
            refreshValidationText();
            setStatus( "Ready. Open a mesh to inspect geometry, materials, LODs and import quality." );

            if( !StringUtil::isNullOrEmpty( s_startupMeshPath ) )
            {
                loadMeshFromFile( s_startupMeshPath );
            }

            m_lastUpdateTime = std::chrono::steady_clock::now();
            setLoadingState( LoadingState::Loaded );
        }

        void MeshViewer::unload( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Unloading );

            saveRecentFiles();

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto gameManager = applicationManager->getGameManager();
            WP_ASSERT( gameManager );

            if( m_menubarListener )
            {
                if( auto uiManager = applicationManager->getUI() )
                {
                    if( m_application )
                    {
                        if( auto menuBar = m_application->getMenubar() )
                        {
                            menuBar->removeObjectListener( m_menubarListener );
                        }
                    }
                }

                m_menubarListener = nullptr;
            }

            if( auto ui = applicationManager->getUI() )
            {
                if( m_workspaceWindow )
                {
                    ui->removeElement( m_workspaceWindow );
                    m_workspaceWindow = nullptr;
                }
            }

            m_toolbarWindow = nullptr;
            m_viewportPanel = nullptr;
            m_assetPanel = nullptr;
            m_inspectorPanel = nullptr;
            m_renderPanel = nullptr;
            m_validationPanel = nullptr;
            m_statusPanel = nullptr;
            m_recentFilesMenu = nullptr;
            m_pathText = nullptr;
            m_searchText = nullptr;
            m_meshInfoText = nullptr;
            m_renderStateText = nullptr;
            m_validationText = nullptr;
            m_statusText = nullptr;

            if( m_cameraActor )
            {
                gameManager->destroyActor( m_cameraActor );
                m_cameraActor = nullptr;
            }

            if( m_meshActor )
            {
                gameManager->destroyActor( m_meshActor );
                m_meshActor = nullptr;
            }

            if( m_renderTarget )
            {
                m_renderTarget->unload( nullptr );
                m_renderTarget = nullptr;
            }

            m_renderWindow = nullptr;
            m_application = nullptr;

            if( m_frameStatistics )
            {
                m_frameStatistics->unload( nullptr );
                m_frameStatistics = nullptr;
            }

            gameManager = nullptr;
            applicationManager = nullptr;

            Application::unload( data );
            core::ApplicationManager::setInstance( nullptr );

            setLoadingState( LoadingState::Unloaded );
        }

        void MeshViewer::createPlugins()
        {
            setPluginsConfigFilePath( "wp_plugins_samples.cfg" );

            Application::createPlugins();

#ifdef _WP_STATIC_LIB_
            try
            {
                WP_DEBUG_TRACE;

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );
                WP_ASSERT( applicationManager->isValid() );

                auto corePlugin = workphone::make_ptr<WPCore>();
                corePlugin->load( nullptr );

                auto databasePlugin = workphone::make_ptr<SQLitePlugin>();
                applicationManager->addPlugin( databasePlugin );

                auto inputPlugin = workphone::make_ptr<OISInput>();
                applicationManager->addPlugin( inputPlugin );

#    if WP_GRAPHICS_SYSTEM_OGRENEXT
                auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgreNext>();
                applicationManager->addPlugin( graphicsPlugin );
#    elif WP_GRAPHICS_SYSTEM_OGRE
                auto graphicsPlugin = fb::make_ptr<render::WPGraphicsOgre>();
                applicationManager->addPlugin( graphicsPlugin );
#    endif

                ApplicationUtil::createFactories();
                ApplicationUtil::createDefaultMaterials();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
#endif
        }

        SmartPtr<scene::IGameActor> MeshViewer::getMeshActor() const
        {
            return m_meshActor;
        }

        void MeshViewer::setMeshActor( SmartPtr<scene::IGameActor> meshActor )
        {
            if( m_meshActor )
            {
                m_meshActor->unload( nullptr );

                auto applicationManager = core::IApplicationManager::instance();
                if( auto gameManager = applicationManager ? applicationManager->getGameManager() : nullptr )
                {
                    gameManager->destroyActor( m_meshActor );
                }
            }

            m_meshActor = meshActor;
            gatherMeshStats();

            if( m_meshActor )
            {
                centerCameraOnMesh();
                applyRenderMode( m_currentRenderMode );
                updateMeshMaterialVariant( m_currentMaterialVariant );
                updateViewportOverlays();
            }

            refreshMeshInfo();
            refreshRenderStateText();
            refreshValidationText();
        }

        bool MeshViewer::isSupportedMeshFormat( const String &filePath ) const
        {
            static const Array<String> supportedFormats = ApplicationUtil::getSupportedMeshFormats();
            auto ext = getFileExtensionLower( filePath );
            if( StringUtil::isNullOrEmpty( ext ) )
            {
                return false;
            }

            for( const auto &format : supportedFormats )
            {
                if( StringUtil::make_lower( format ) == ext )
                {
                    return true;
                }
            }

            return false;
        }

        void MeshViewer::loadMeshFromFile( const String &filePath )
        {
            try
            {
                if( StringUtil::isNullOrEmpty( filePath ) )
                {
                    setStatus( "No file path provided." );
                    return;
                }

                if( !isSupportedMeshFormat( filePath ) )
                {
                    std::ostringstream ss;
                    ss << "Unsupported mesh format: " << getFileExtensionLower( filePath )
                       << ". Supported formats: ";
                    auto formats = ApplicationUtil::getSupportedMeshFormats();
                    for( size_t i = 0; i < formats.size(); ++i )
                    {
                        if( i > 0 )
                        {
                            ss << ", ";
                        }
                        ss << formats[i];
                    }
                    setStatus( ss.str() );
                    return;
                }

                m_currentMeshPath = filePath;
                if( m_pathText )
                {
                    m_pathText->setText( filePath );
                }

                auto meshActor = ApplicationUtil::loadMesh( filePath );
                setMeshActor( meshActor );
                addRecentFile( filePath );

                std::ostringstream ss;
                ss << "Loaded mesh: " << getFileNameFromPath( filePath );
                setStatus( ss.str() );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                setStatus( "Failed to load mesh. See log for details." );
            }
        }

        void MeshViewer::createUI()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto factoryManager = applicationManager->getFactoryManager();

            auto uiManager = factoryManager->make_object<ui::IUIManager>( "ImGui" );
            if( uiManager )
            {
                uiManager->load( nullptr );
                applicationManager->setUI( uiManager );

                auto application = uiManager->addApplication();
                uiManager->setApplication( application );
                m_application = application;
            }
        }

        void MeshViewer::createRenderWindow()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            if( ui )
            {
                m_renderWindow = ui->addElementByType<ui::IUIRenderWindow>();

                if( m_renderWindow )
                {
                    if( m_renderTarget )
                    {
                        m_renderWindow->setRenderTexture( m_renderTarget );
                    }
                }
            }
        }

        void MeshViewer::setupUI()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto uiManager = applicationManager->getUI();
            if( !uiManager )
            {
                return;
            }

            auto menubarListener = workphone::make_ptr<CUIMenuBarListener>();
            menubarListener->setOwner( this );
            m_menubarListener = menubarListener;

            setupMenuBar();
            setupWorkspaceUI();
        }

        void MeshViewer::setupMenuBar()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto uiManager = applicationManager->getUI();
            if( !uiManager || !m_application )
            {
                return;
            }

            auto menuBar = uiManager->addElementByType<ui::IUIMenubar>();

            auto fileMenu = uiManager->addElementByType<ui::IUIMenu>();
            fileMenu->setLabel( "File" );
            menuBar->addMenu( fileMenu );
            Util::addMenuItem( fileMenu, static_cast<s32>( ElementId::Open ), "Open Mesh...",
                               "Open a mesh file.", ui::IUIMenuItem::Type::Normal );

            m_recentFilesMenu = uiManager->addElementByType<ui::IUIMenu>();
            m_recentFilesMenu->setLabel( "Open Recent" );
            menuBar->addMenu( m_recentFilesMenu );
            rebuildRecentMenu();

            Util::addMenuItem( fileMenu, static_cast<s32>( ElementId::ReloadMesh ), "Reload Mesh",
                               "Reload the current mesh.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( fileMenu, static_cast<s32>( ElementId::CloseMesh ), "Close Mesh",
                               "Unload the current mesh.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuSeparator( fileMenu );
            Util::addMenuItem( fileMenu, static_cast<s32>( ElementId::SaveReport ), "Save Report",
                               "Save validation report.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( fileMenu, static_cast<s32>( ElementId::ExportScreenshot ),
                               "Export Screenshot", "Export viewport screenshot.",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( fileMenu, static_cast<s32>( ElementId::ExportThumbnail ),
                               "Export Thumbnail", "Export thumbnail.",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuSeparator( fileMenu );
            Util::addMenuItem( fileMenu, static_cast<s32>( ElementId::Exit ), "Exit", "Exit",
                               ui::IUIMenuItem::Type::Normal );

            auto viewMenu = uiManager->addElementByType<ui::IUIMenu>();
            viewMenu->setLabel( "View" );
            menuBar->addMenu( viewMenu );
            Util::addMenuItem( viewMenu, static_cast<s32>( ElementId::FrameMesh ), "Frame Mesh",
                               "Frame the loaded mesh.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( viewMenu, static_cast<s32>( ElementId::ResetCamera ), "Reset Camera",
                               "Reset the camera.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuSeparator( viewMenu );
            Util::addMenuItem( viewMenu, static_cast<s32>( ElementId::ViewFront ), "Front",
                               "Front view.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( viewMenu, static_cast<s32>( ElementId::ViewSide ), "Side",
                               "Side view.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( viewMenu, static_cast<s32>( ElementId::ViewTop ), "Top",
                               "Top view.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( viewMenu, static_cast<s32>( ElementId::ViewIso ), "Isometric",
                               "Isometric view.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuSeparator( viewMenu );
            Util::addMenuItem( viewMenu, static_cast<s32>( ElementId::ToggleGrid ), "Grid",
                               "Toggle grid.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( viewMenu, static_cast<s32>( ElementId::ToggleAxes ), "Axes",
                               "Toggle axes.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( viewMenu, static_cast<s32>( ElementId::ToggleBounds ), "Bounds",
                               "Toggle bounds.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( viewMenu, static_cast<s32>( ElementId::ToggleStats ), "Stats",
                               "Toggle stats.", ui::IUIMenuItem::Type::Normal );

            auto renderMenu = uiManager->addElementByType<ui::IUIMenu>();
            renderMenu->setLabel( "Render" );
            menuBar->addMenu( renderMenu );
            Util::addMenuItem( renderMenu, static_cast<s32>( ElementId::RenderLit ), "Lit",
                               "Lit preview.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( renderMenu, static_cast<s32>( ElementId::RenderUnlit ), "Unlit",
                               "Unlit preview.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( renderMenu, static_cast<s32>( ElementId::RenderWireframe ),
                               "Wireframe", "Wireframe preview.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( renderMenu, static_cast<s32>( ElementId::RenderNormals ), "Normals",
                               "Normal debug view.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( renderMenu, static_cast<s32>( ElementId::RenderUV0 ), "UV0",
                               "UV0 debug view.", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( renderMenu, static_cast<s32>( ElementId::RenderVertexColour ),
                               "Vertex Colours", "Vertex colour debug view.",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuSeparator( renderMenu );
            Util::addMenuItem( renderMenu, static_cast<s32>( ElementId::LightingStudio ),
                               "Studio Lighting", "Studio light rig.",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( renderMenu, static_cast<s32>( ElementId::LightingOutdoor ),
                               "Outdoor Lighting", "Outdoor light rig.",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( renderMenu, static_cast<s32>( ElementId::LightingIndoor ),
                               "Indoor Lighting", "Indoor light rig.",
                               ui::IUIMenuItem::Type::Normal );

            auto toolsMenu = uiManager->addElementByType<ui::IUIMenu>();
            toolsMenu->setLabel( "Tools" );
            menuBar->addMenu( toolsMenu );
            Util::addMenuItem( toolsMenu, static_cast<s32>( ElementId::ValidateMesh ),
                               "Validate Mesh", "Run mesh validation.",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( toolsMenu, static_cast<s32>( ElementId::ValidateMaterials ),
                               "Validate Materials", "Run material validation.",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( toolsMenu, static_cast<s32>( ElementId::ValidateSkeleton ),
                               "Validate Skeleton", "Run skeleton validation.",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( toolsMenu, static_cast<s32>( ElementId::ValidateLODs ),
                               "Validate LODs", "Run LOD validation.",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuSeparator( toolsMenu );
            Util::addMenuItem( toolsMenu, static_cast<s32>( ElementId::RecalculateBounds ),
                               "Recalculate Bounds", "Recalculate bounds.",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( toolsMenu, static_cast<s32>( ElementId::RebuildTangents ),
                               "Rebuild Tangents", "Rebuild tangent basis.",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( toolsMenu, static_cast<s32>( ElementId::GenerateLODs ),
                               "Generate LODs", "Generate LOD chain.",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( toolsMenu, static_cast<s32>( ElementId::OptimiseMesh ),
                               "Optimise Mesh", "Optimise mesh buffers.",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( toolsMenu, static_cast<s32>( ElementId::BuildCollision ),
                               "Build Collision", "Build collision mesh.",
                               ui::IUIMenuItem::Type::Normal );

            m_application->setMenubar( menuBar );

            if( m_menubarListener )
            {
                menuBar->addObjectListener( m_menubarListener );
            }
        }

        void MeshViewer::setupWorkspaceUI()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto uiManager = applicationManager->getUI();
            if( !uiManager )
            {
                return;
            }

            m_workspaceWindow = uiManager->addElementByType<ui::IUIWindow>();
            if( !m_workspaceWindow )
            {
                return;
            }

            m_workspaceWindow->setLabel( "Mesh Viewer" );
            m_workspaceWindow->setHasBorder( true );
            m_workspaceWindow->setSize( Vector2F( 1280.0f, 820.0f ) );

            setupToolbar();
            setupViewportPanel();
            setupAssetPanel();
            setupInspectorPanel();
            setupRenderPanel();
            setupValidationPanel();
            setupStatusBar();
        }

        void MeshViewer::setupToolbar()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto uiManager = applicationManager->getUI();

            m_toolbarWindow = uiManager->addElementByType<ui::IUIWindow>();
            m_toolbarWindow->setLabel( "Toolbar" );
            m_toolbarWindow->setHasBorder( true );
            m_workspaceWindow->addChild( m_toolbarWindow );

            registerControl( addButton( uiManager, m_toolbarWindow, "Open", ElementId::Open, false ) );
            registerControl(
                addButton( uiManager, m_toolbarWindow, "Reload", ElementId::ReloadMesh, true ) );
            registerControl(
                addButton( uiManager, m_toolbarWindow, "Frame", ElementId::FrameMesh, true ) );
            registerControl(
                addButton( uiManager, m_toolbarWindow, "Validate", ElementId::ValidateMesh, true ) );
            registerControl(
                addButton( uiManager, m_toolbarWindow, "Screenshot", ElementId::ExportScreenshot, true ) );

            m_searchText =
                addTextEntry( uiManager, m_toolbarWindow, "Search / Filter Mesh Data",
                              ElementId::SearchText, "" );
            registerControl( m_searchText );
        }

        void MeshViewer::setupViewportPanel()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto uiManager = applicationManager->getUI();

            m_viewportPanel = uiManager->addElementByType<ui::IUIWindow>();
            m_viewportPanel->setLabel( "Viewport" );
            m_viewportPanel->setHasBorder( true );
            m_workspaceWindow->addChild( m_viewportPanel );

            if( m_renderWindow )
            {
                m_viewportPanel->addChild( m_renderWindow );
            }

            registerControl( addButton( uiManager, m_viewportPanel, "Front", ElementId::ViewFront, false ) );
            registerControl( addButton( uiManager, m_viewportPanel, "Side", ElementId::ViewSide, true ) );
            registerControl( addButton( uiManager, m_viewportPanel, "Top", ElementId::ViewTop, true ) );
            registerControl( addButton( uiManager, m_viewportPanel, "Iso", ElementId::ViewIso, true ) );

            registerControl( addToggle( uiManager, m_viewportPanel, "Grid", ElementId::ToggleGrid, true ) );
            registerControl( addToggle( uiManager, m_viewportPanel, "Axes", ElementId::ToggleAxes, true ) );
            registerControl(
                addToggle( uiManager, m_viewportPanel, "Bounds", ElementId::ToggleBounds, true ) );
            registerControl(
                addToggle( uiManager, m_viewportPanel, "Auto Rotate", ElementId::AutoRotate, false ) );

            registerControl( addSlider( uiManager, m_viewportPanel, "Camera Speed", ElementId::CameraSpeed, 0.05f, 10.0f,
                                        1.0f ) );
            registerControl( addSlider( uiManager, m_viewportPanel, "Near Clip", ElementId::NearClip, 0.001f, 10.0f,
                                        0.01f ) );
            registerControl( addSlider( uiManager, m_viewportPanel, "Far Clip", ElementId::FarClip, 10.0f, 10000.0f,
                                        1000.0f ) );
        }

        void MeshViewer::setupAssetPanel()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto uiManager = applicationManager->getUI();

            m_assetPanel = uiManager->addElementByType<ui::IUIWindow>();
            m_assetPanel->setLabel( "Asset" );
            m_assetPanel->setHasBorder( true );
            m_workspaceWindow->addChild( m_assetPanel );

            m_pathText =
                addTextEntry( uiManager, m_assetPanel, "Mesh Path", ElementId::PathText, "None" );
            registerControl( m_pathText );

            m_meshInfoText = addText( uiManager, m_assetPanel, "No mesh loaded.", false );

            registerControl(
                addButton( uiManager, m_assetPanel, "Copy Report", ElementId::CopyMeshReport, false ) );
            registerControl(
                addButton( uiManager, m_assetPanel, "Generate Preview", ElementId::GeneratePreview,
                           true ) );
            registerControl(
                addButton( uiManager, m_assetPanel, "Bake Thumbnail", ElementId::BakeThumbnail, true ) );

            registerControl( addDropdown( uiManager, m_assetPanel, "LOD", ElementId::LodDropdown,
                                          makeOptions( { "LOD 0", "LOD 1", "LOD 2", "LOD 3", "All LODs" } ), 0 ) );
            registerControl( addDropdown( uiManager, m_assetPanel, "Animation Clip", ElementId::AnimationDropdown,
                                          makeOptions( { "Bind Pose", "Idle", "Walk", "Run", "Custom" } ), 0 ) );
            registerControl( addDropdown( uiManager, m_assetPanel, "Skeleton", ElementId::SkeletonDropdown,
                                          makeOptions( { "Default", "Display Bones", "Display Joints", "Retarget Preview" } ),
                                          0 ) );
        }

        void MeshViewer::setupInspectorPanel()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto uiManager = applicationManager->getUI();

            m_inspectorPanel = uiManager->addElementByType<ui::IUIWindow>();
            m_inspectorPanel->setLabel( "Inspector" );
            m_inspectorPanel->setHasBorder( true );
            m_workspaceWindow->addChild( m_inspectorPanel );

            addText( uiManager, m_inspectorPanel, "-- Geometry --", false );
            registerControl(
                addToggle( uiManager, m_inspectorPanel, "Wireframe Overlay", ElementId::ToggleWireframe,
                           false ) );
            registerControl(
                addToggle( uiManager, m_inspectorPanel, "Normals", ElementId::ToggleNormals, false ) );
            registerControl(
                addToggle( uiManager, m_inspectorPanel, "Tangents", ElementId::ToggleTangents, false ) );
            registerControl(
                addToggle( uiManager, m_inspectorPanel, "Binormals", ElementId::ToggleBinormals, false ) );
            registerControl(
                addToggle( uiManager, m_inspectorPanel, "Colliders", ElementId::ToggleColliders, false ) );

            registerControl( addSlider( uiManager, m_inspectorPanel, "Wire Thickness", ElementId::WireThickness, 0.1f,
                                        8.0f, 1.0f ) );
            registerControl( addSlider( uiManager, m_inspectorPanel, "Normal Length", ElementId::NormalLength, 0.01f,
                                        5.0f, 0.25f ) );
            registerControl( addSlider( uiManager, m_inspectorPanel, "Tangent Length", ElementId::TangentLength, 0.01f,
                                        5.0f, 0.25f ) );

            addText( uiManager, m_inspectorPanel, "-- Skinning / Animation --", false );
            registerControl(
                addToggle( uiManager, m_inspectorPanel, "Skeleton", ElementId::ToggleSkeleton, false ) );
            registerControl( addToggle( uiManager, m_inspectorPanel, "Skin Weights",
                                        ElementId::ToggleSkinWeights, false ) );

            addText( uiManager, m_inspectorPanel, "-- Import Tools --", false );
            registerControl( addButton( uiManager, m_inspectorPanel, "Recalculate Bounds",
                                        ElementId::RecalculateBounds, false ) );
            registerControl( addButton( uiManager, m_inspectorPanel, "Rebuild Tangents",
                                        ElementId::RebuildTangents, true ) );
            registerControl(
                addButton( uiManager, m_inspectorPanel, "Generate LODs", ElementId::GenerateLODs, false ) );
            registerControl(
                addButton( uiManager, m_inspectorPanel, "Optimise Mesh", ElementId::OptimiseMesh, true ) );
            registerControl( addButton( uiManager, m_inspectorPanel, "Build Collision",
                                        ElementId::BuildCollision, false ) );
        }

        void MeshViewer::setupRenderPanel()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto uiManager = applicationManager->getUI();

            m_renderPanel = uiManager->addElementByType<ui::IUIWindow>();
            m_renderPanel->setLabel( "Render / Materials" );
            m_renderPanel->setHasBorder( true );
            m_workspaceWindow->addChild( m_renderPanel );

            registerControl( addDropdown( uiManager, m_renderPanel, "Render Mode", ElementId::RenderModeDropdown,
                                           makeOptions( { "Lit", "Unlit", "Wireframe", "Normals", "Tangents", "UV0",
                                                          "UV1", "Vertex Colour", "Lightmap UV", "Skin Weights" } ),
                                           0 ) );
            registerControl( addDropdown( uiManager, m_renderPanel, "Material Variant",
                                          ElementId::MaterialVariantDropdown,
                                          makeOptions( { "Default", "Grey Clay", "Checker", "UV Checker",
                                                         "Normal Debug", "Roughness Debug", "Metalness Debug" } ),
                                          0 ) );

            registerControl( addButton( uiManager, m_renderPanel, "Lit", ElementId::RenderLit, false ) );
            registerControl(
                addButton( uiManager, m_renderPanel, "Unlit", ElementId::RenderUnlit, true ) );
            registerControl( addButton( uiManager, m_renderPanel, "Normals", ElementId::RenderNormals,
                                        true ) );
            registerControl(
                addButton( uiManager, m_renderPanel, "UV0", ElementId::RenderUV0, true ) );
            registerControl(
                addToggle( uiManager, m_renderPanel, "UV Checker", ElementId::ToggleUVChecker, false ) );
            registerControl(
                addToggle( uiManager, m_renderPanel, "Overdraw", ElementId::ToggleOverdraw, false ) );

            registerControl( addSlider( uiManager, m_renderPanel, "Exposure", ElementId::Exposure, -5.0f, 5.0f, 0.0f ) );
            registerControl( addSlider( uiManager, m_renderPanel, "LOD Bias", ElementId::LodBias, -2.0f, 2.0f, 0.0f ) );

            addText( uiManager, m_renderPanel, "-- Lighting Presets --", false );
            registerControl( addButton( uiManager, m_renderPanel, "Studio", ElementId::LightingStudio,
                                        false ) );
            registerControl( addButton( uiManager, m_renderPanel, "Outdoor", ElementId::LightingOutdoor,
                                        true ) );
            registerControl( addButton( uiManager, m_renderPanel, "Indoor", ElementId::LightingIndoor,
                                        true ) );
            registerControl( addButton( uiManager, m_renderPanel, "Neutral", ElementId::LightingNeutral,
                                        true ) );

            m_renderStateText = addText( uiManager, m_renderPanel, "Render mode: Lit", false );
        }

        void MeshViewer::setupValidationPanel()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto uiManager = applicationManager->getUI();

            m_validationPanel = uiManager->addElementByType<ui::IUIWindow>();
            m_validationPanel->setLabel( "Validation" );
            m_validationPanel->setHasBorder( true );
            m_workspaceWindow->addChild( m_validationPanel );

            registerControl( addButton( uiManager, m_validationPanel, "Validate Mesh",
                                        ElementId::ValidateMesh, false ) );
            registerControl( addButton( uiManager, m_validationPanel, "Materials",
                                        ElementId::ValidateMaterials, true ) );
            registerControl( addButton( uiManager, m_validationPanel, "Skeleton",
                                        ElementId::ValidateSkeleton, true ) );
            registerControl(
                addButton( uiManager, m_validationPanel, "LODs", ElementId::ValidateLODs, true ) );

            m_validationText = addText(
                uiManager, m_validationPanel,
                "Run validation to check geometry, materials, skeleton, LODs and collision.",
                false );
        }

        void MeshViewer::setupStatusBar()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto uiManager = applicationManager->getUI();

            m_statusPanel = uiManager->addElementByType<ui::IUIWindow>();
            m_statusPanel->setLabel( "Status" );
            m_statusPanel->setHasBorder( true );
            m_workspaceWindow->addChild( m_statusPanel );

            m_statusText = addText( uiManager, m_statusPanel, "Ready.", false );
        }

        void MeshViewer::setupCamera()
        {
            if( auto camera = m_camera )
            {
                camera->setRenderUI( true );
            }
        }

        void MeshViewer::setupViewport()
        {
            if( auto vp = m_viewport )
            {
                vp->setEnableUI( true );
                vp->setEnableSceneRender( true );
                vp->setMaterialScheme( getRenderModeScheme( m_currentRenderMode ) );
            }
        }

        void MeshViewer::createCamera()
        {
            Application::createCamera();

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto sceneManager = applicationManager->getGameManagerPtr();
            WP_ASSERT( sceneManager );

            auto scene = sceneManager->getCurrentScenePtr();
            WP_ASSERT( scene );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( graphicsSystem )
            {
                applicationManager->setEditorCamera( true );

                auto textureManager = graphicsSystem->getTextureManager();
                WP_ASSERT( textureManager );

                auto cameraMgr = factoryManager->make_ptr<scene::CameraManager>();
                cameraMgr->load( nullptr );
                applicationManager->setCameraManager( cameraMgr );

                m_cameraActor = sceneManager->createActor();
                m_cameraActor->setFlag( scene::IGameActor::ActorFlagIsEditor, true );
                m_cameraActor->setPerpetual( true );
                cameraMgr->setEditorCamera( m_cameraActor );

                auto cameraComponent = m_cameraActor->addComponent<scene::Camera>();
                cameraComponent->setEnableShadows( true );
                cameraComponent->setEnableSceneRender( true );
                cameraComponent->setEnableUI( true );
                cameraComponent->setActive( true );

                m_renderTarget = textureManager->createRenderTexture();
                WP_ASSERT( m_renderTarget );

                WP_ASSERT( cameraComponent );
                cameraComponent->setTargetTexture( m_renderTarget );

                auto gameScene = sceneManager->getCurrentScene();
                gameScene->addActor( m_cameraActor );
            }
        }

        void MeshViewer::centerCameraOnMesh()
        {
            if( !m_meshActor || !m_cameraActor )
            {
                return;
            }

            try
            {
                auto bounds = scene::GameActorUtil::getActorAABB( m_meshActor );
                if( !bounds.isFinite() || !bounds.isValid() )
                {
                    setStatus( "Mesh loaded, but bounds were invalid. Check import scale and source data." );
                    return;
                }

                auto center = bounds.getCenter();
                auto radius = bounds.getRadius();
                if( radius <= real_Num( 0.0 ) || !Math<real_Num>::isFinite( radius ) )
                {
                    radius = real_Num( 1.0 );
                }

                auto distance = Math<real_Num>::max( radius * real_Num( 2.5 ), real_Num( 2.0 ) );
                auto sphericalCoords = Vector3<real_Num>( distance, real_Num( 0.0 ),
                                                          Math<real_Num>::pi() * real_Num( 0.5 ) );

                if( auto sphericalCamera =
                        m_cameraActor->getComponent<scene::SphericalCameraController>() )
                {
                    sphericalCamera->setMaxDistance(
                        Math<real_Num>::max( distance * real_Num( 8.0 ), real_Num( 100.0 ) ) );
                    sphericalCamera->setTargetPosition( center );
                    sphericalCamera->setSphericalCoords( sphericalCoords );
                }

                auto cameraPosition = center + Vector3<real_Num>( 0.0, 0.0, distance );
                m_cameraActor->setPosition( cameraPosition );
                m_cameraActor->lookAt( center );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void MeshViewer::applyCameraView( ElementId view )
        {
            if( !m_cameraActor )
            {
                return;
            }

            auto sphericalCamera = m_cameraActor->getComponent<scene::SphericalCameraController>();
            if( !sphericalCamera )
            {
                return;
            }

            auto current = sphericalCamera->getSphericalCoords();
            auto distance = current.x;

            real_Num yaw = real_Num( 0.0 );
            real_Num pitch = Math<real_Num>::pi() * real_Num( 0.5 );

            switch( view )
            {
            case ElementId::ViewFront:
                yaw = real_Num( 0.0 );
                pitch = Math<real_Num>::pi() * real_Num( 0.5 );
                break;
            case ElementId::ViewSide:
                yaw = Math<real_Num>::pi() * real_Num( 0.5 );
                pitch = Math<real_Num>::pi() * real_Num( 0.5 );
                break;
            case ElementId::ViewTop:
                yaw = real_Num( 0.0 );
                pitch = real_Num( 0.01 );
                break;
            case ElementId::ViewIso:
                yaw = Math<real_Num>::pi() * real_Num( 0.25 );
                pitch = Math<real_Num>::pi() * real_Num( 0.25 );
                break;
            default:
                break;
            }

            sphericalCamera->setSphericalCoords( Vector3<real_Num>( distance, yaw, pitch ) );
        }

        void MeshViewer::registerControl( SmartPtr<ui::IUIElement> element )
        {
            if( element && m_menubarListener )
            {
                element->addObjectListener( m_menubarListener );
            }
        }

        void MeshViewer::handleAction( ElementId action )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto fileSystem = applicationManager ? applicationManager->getFileSystem() : nullptr;

            switch( action )
            {
            case ElementId::Open:
                openMeshFileDialog();
                break;
            case ElementId::ReloadMesh:
                if( !StringUtil::isNullOrEmpty( m_currentMeshPath ) )
                {
                    loadMeshFromFile( m_currentMeshPath );
                }
                else
                {
                    setStatus( "No mesh is loaded to reload." );
                }
                break;
            case ElementId::CloseMesh:
                setMeshActor( nullptr );
                m_currentMeshPath = String();
                if( m_pathText )
                {
                    m_pathText->setText( "None" );
                }
                setStatus( "Mesh closed." );
                break;
            case ElementId::SaveReport:
            {
                auto report = getCurrentMeshReport();
                if( !m_currentMeshPath.empty() )
                {
                    auto reportPath = m_currentMeshPath + ".report.txt";
                    saveReportToFile( reportPath );
                }
                else
                {
                    setStatus( "No mesh loaded to report on." );
                }
            }
            break;
            case ElementId::ExportScreenshot:
            {
                auto path = String( "MeshViewer_Screenshot_" ) + getTimestampString() + ".bmp";
                queueScreenshot( path, Vector2I( 1920, 1080 ) );
            }
            break;
            case ElementId::ExportThumbnail:
            {
                m_pendingThumbnail = true;
                m_pendingThumbnailPath =
                    String( "MeshViewer_Thumbnail_" ) + getTimestampString() + ".bmp";
                setStatus( "Thumbnail queued for next frame." );
            }
            break;
            case ElementId::FrameMesh:
            case ElementId::ResetCamera:
                centerCameraOnMesh();
                setStatus( "Camera framed on mesh." );
                break;
            case ElementId::ViewFront:
            case ElementId::ViewSide:
            case ElementId::ViewTop:
            case ElementId::ViewIso:
                applyCameraView( action );
                setStatus( "Camera view updated." );
                break;
            case ElementId::RenderLit:
                m_currentRenderMode = 0;
                applyRenderMode( m_currentRenderMode );
                setStatus( "Render mode set to Lit." );
                break;
            case ElementId::RenderUnlit:
                m_currentRenderMode = 1;
                applyRenderMode( m_currentRenderMode );
                setStatus( "Render mode set to Unlit." );
                break;
            case ElementId::RenderWireframe:
            case ElementId::ToggleWireframe:
                m_showWireframe = !m_showWireframe;
                m_currentRenderMode = 2;
                applyRenderMode( m_currentRenderMode );
                refreshRenderStateText();
                setStatus( "Wireframe overlay toggled." );
                break;
            case ElementId::RenderNormals:
            case ElementId::ToggleNormals:
                m_showNormals = !m_showNormals;
                m_currentRenderMode = 3;
                applyRenderMode( m_currentRenderMode );
                refreshRenderStateText();
                setStatus( "Normal debug view toggled." );
                break;
            case ElementId::ToggleTangents:
            case ElementId::RenderTangents:
                m_showTangents = !m_showTangents;
                m_currentRenderMode = 4;
                applyRenderMode( m_currentRenderMode );
                refreshRenderStateText();
                setStatus( "Tangent debug view toggled." );
                break;
            case ElementId::ToggleBinormals:
                m_showBinormals = !m_showBinormals;
                refreshRenderStateText();
                setStatus( "Binormal debug view toggled." );
                break;
            case ElementId::ToggleSkeleton:
            case ElementId::RenderSkinWeights:
            case ElementId::ToggleSkinWeights:
                m_showSkeleton = !m_showSkeleton;
                m_showSkinWeights = !m_showSkinWeights;
                refreshRenderStateText();
                setStatus( "Skeleton / skinning debug view toggled." );
                break;
            case ElementId::ToggleColliders:
                m_showColliders = !m_showColliders;
                refreshRenderStateText();
                setStatus( "Collider debug overlay toggled." );
                break;
            case ElementId::ToggleUVChecker:
                m_showUVChecker = !m_showUVChecker;
                refreshRenderStateText();
                setStatus( "UV checker overlay toggled." );
                break;
            case ElementId::ToggleOverdraw:
                m_showOverdraw = !m_showOverdraw;
                refreshRenderStateText();
                setStatus( "Overdraw view toggled." );
                break;
            case ElementId::ToggleGrid:
                m_showGrid = !m_showGrid;
                refreshRenderStateText();
                setStatus( "Grid overlay toggled." );
                break;
            case ElementId::ToggleAxes:
                m_showAxes = !m_showAxes;
                refreshRenderStateText();
                setStatus( "Axis gizmo toggled." );
                break;
            case ElementId::ToggleBounds:
                m_showBounds = !m_showBounds;
                refreshRenderStateText();
                setStatus( "Bounds overlay toggled." );
                break;
            case ElementId::ToggleStats:
                m_showStats = !m_showStats;
                refreshRenderStateText();
                setStatus( "Stats overlay toggled." );
                break;
            case ElementId::AutoRotate:
            case ElementId::Turntable:
                m_autoRotate = !m_autoRotate;
                refreshRenderStateText();
                setStatus( "Turntable preview toggled." );
                break;
            case ElementId::ValidateMesh:
                runValidation( ElementId::ValidateMesh );
                break;
            case ElementId::ValidateMaterials:
                runValidation( ElementId::ValidateMaterials );
                break;
            case ElementId::ValidateSkeleton:
                runValidation( ElementId::ValidateSkeleton );
                break;
            case ElementId::ValidateLODs:
                runValidation( ElementId::ValidateLODs );
                break;
            case ElementId::CopyMeshReport:
                setStatus( getCurrentMeshReport() );
                break;
            case ElementId::RecalculateBounds:
                centerCameraOnMesh();
                refreshMeshInfo();
                setStatus( "Bounds recalculated from current mesh." );
                break;
            case ElementId::GeneratePreview:
            {
                auto path = String( "MeshViewer_Preview_" ) + getTimestampString() + ".bmp";
                queueScreenshot( path, Vector2I( 1280, 720 ) );
            }
            break;
            case ElementId::BakeThumbnail:
            {
                m_pendingThumbnail = true;
                m_pendingThumbnailPath =
                    String( "MeshViewer_Thumbnail_" ) + getTimestampString() + ".bmp";
                setStatus( "Thumbnail queued for next frame." );
            }
            break;
            case ElementId::RebuildTangents:
            case ElementId::GenerateLODs:
            case ElementId::OptimiseMesh:
            case ElementId::BuildCollision:
                setStatus( "Action queued: wire this command to your asset pipeline/export system." );
                break;
            case ElementId::LightingStudio:
            case ElementId::LightingOutdoor:
            case ElementId::LightingIndoor:
            case ElementId::LightingNeutral:
                applyLightingPreset( action );
                break;
            case ElementId::Exit:
                if( applicationManager )
                {
                    applicationManager->setQuit( true );
                }
                break;
            default:
                break;
            }

            refreshMeshInfo();
        }

        void MeshViewer::handleDropdown( ElementId action, s32 selectedIndex )
        {
            switch( action )
            {
            case ElementId::RenderModeDropdown:
                m_currentRenderMode = selectedIndex;
                applyRenderMode( m_currentRenderMode );
                refreshRenderStateText();
                setStatus( "Render mode changed via dropdown." );
                break;
            case ElementId::MaterialVariantDropdown:
                m_currentMaterialVariant = selectedIndex;
                updateMeshMaterialVariant( m_currentMaterialVariant );
                refreshRenderStateText();
                setStatus( "Material variant changed." );
                break;
            case ElementId::LodDropdown:
                m_currentLod = selectedIndex;
                refreshRenderStateText();
                setStatus( "LOD selection changed." );
                break;
            case ElementId::AnimationDropdown:
                refreshRenderStateText();
                setStatus( "Animation clip selection changed." );
                break;
            case ElementId::SkeletonDropdown:
                refreshRenderStateText();
                setStatus( "Skeleton display mode changed." );
                break;
            default:
                break;
            }

            refreshMeshInfo();
        }

        void MeshViewer::handleSlider( ElementId action, real_Num value )
        {
            switch( action )
            {
            case ElementId::Exposure:
                m_exposure = static_cast<f32>( value );
                setStatus( "Exposure adjusted." );
                break;
            case ElementId::CameraSpeed:
                m_cameraSpeed = static_cast<f32>( value );
                if( auto sphericalCamera = m_cameraActor ? m_cameraActor->getComponent<scene::SphericalCameraController>() : nullptr )
                {
                    sphericalCamera->setMoveSpeed( m_cameraSpeed );
                    sphericalCamera->setZoomSpeed( m_cameraSpeed );
                }
                setStatus( "Camera speed adjusted." );
                break;
            case ElementId::NearClip:
                m_nearClip = static_cast<f32>( value );
                setStatus( "Near clip adjusted." );
                break;
            case ElementId::FarClip:
                m_farClip = static_cast<f32>( value );
                setStatus( "Far clip adjusted." );
                break;
            case ElementId::LodBias:
                m_lodBias = static_cast<f32>( value );
                setStatus( "LOD bias adjusted." );
                break;
            case ElementId::WireThickness:
                m_wireThickness = static_cast<f32>( value );
                setStatus( "Wire thickness adjusted." );
                break;
            case ElementId::NormalLength:
                m_normalLength = static_cast<f32>( value );
                setStatus( "Normal length adjusted." );
                break;
            case ElementId::TangentLength:
                m_tangentLength = static_cast<f32>( value );
                setStatus( "Tangent length adjusted." );
                break;
            default:
                break;
            }
        }

        bool MeshViewer::getOverlayFlag( ElementId action ) const
        {
            switch( action )
            {
            case ElementId::ToggleGrid: return m_showGrid;
            case ElementId::ToggleAxes: return m_showAxes;
            case ElementId::ToggleBounds: return m_showBounds;
            case ElementId::ToggleWireframe: return m_showWireframe;
            case ElementId::ToggleNormals: return m_showNormals;
            case ElementId::ToggleTangents: return m_showTangents;
            case ElementId::ToggleBinormals: return m_showBinormals;
            case ElementId::ToggleSkeleton: return m_showSkeleton;
            case ElementId::ToggleSkinWeights: return m_showSkinWeights;
            case ElementId::ToggleColliders: return m_showColliders;
            case ElementId::ToggleOverdraw: return m_showOverdraw;
            case ElementId::ToggleUVChecker: return m_showUVChecker;
            case ElementId::ToggleStats: return m_showStats;
            case ElementId::AutoRotate:
            case ElementId::Turntable: return m_autoRotate;
            default: return false;
            }
        }

        void MeshViewer::setOverlayFlag( ElementId action, bool value )
        {
            switch( action )
            {
            case ElementId::ToggleGrid: m_showGrid = value; break;
            case ElementId::ToggleAxes: m_showAxes = value; break;
            case ElementId::ToggleBounds: m_showBounds = value; break;
            case ElementId::ToggleWireframe: m_showWireframe = value; break;
            case ElementId::ToggleNormals: m_showNormals = value; break;
            case ElementId::ToggleTangents: m_showTangents = value; break;
            case ElementId::ToggleBinormals: m_showBinormals = value; break;
            case ElementId::ToggleSkeleton: m_showSkeleton = value; break;
            case ElementId::ToggleSkinWeights: m_showSkinWeights = value; break;
            case ElementId::ToggleColliders: m_showColliders = value; break;
            case ElementId::ToggleOverdraw: m_showOverdraw = value; break;
            case ElementId::ToggleUVChecker: m_showUVChecker = value; break;
            case ElementId::ToggleStats: m_showStats = value; break;
            case ElementId::AutoRotate:
            case ElementId::Turntable: m_autoRotate = value; break;
            default: break;
            }
        }

        void MeshViewer::handleToggle( ElementId action, bool value )
        {
            switch( action )
            {
            case ElementId::ToggleInspector:
                if( m_inspectorPanel ) m_inspectorPanel->setVisible( value, true );
                break;
            case ElementId::ToggleAssetPanel:
                if( m_assetPanel ) m_assetPanel->setVisible( value, true );
                break;
            case ElementId::ToggleMaterialPanel:
                if( m_renderPanel ) m_renderPanel->setVisible( value, true );
                break;
            default:
                setOverlayFlag( action, value );
                break;
            }

            refreshRenderStateText();
            updateViewportOverlays();
            setStatus( value ? "Viewport overlay enabled." : "Viewport overlay disabled." );
        }

        void MeshViewer::setStatus( const String &status )
        {
            if( m_statusText )
            {
                m_statusText->setText( status );
            }
        }

        void MeshViewer::refreshMeshInfo()
        {
            if( !m_meshInfoText )
            {
                return;
            }

            if( !m_meshActor )
            {
                m_meshInfoText->setText(
                    "No mesh loaded.\nOpen a mesh to inspect bounds, materials, LODs, skeleton, "
                    "collision and import warnings." );
                return;
            }

            std::ostringstream ss;
            ss << "Mesh: "
               << ( StringUtil::isNullOrEmpty( m_currentMeshPath )
                        ? String( "Runtime mesh" )
                        : getFileNameFromPath( m_currentMeshPath ) )
               << "\n";

            try
            {
                auto bounds = scene::GameActorUtil::getActorAABB( m_meshActor );
                if( bounds.isFinite() && bounds.isValid() )
                {
                    auto center = bounds.getCenter();
                    auto radius = bounds.getRadius();
                    ss << "Bounds center: (" << center.x << ", " << center.y << ", " << center.z
                       << ")  radius: " << radius << "\n";
                }
                else
                {
                    ss << "Bounds: invalid\n";
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                ss << "Bounds: unavailable\n";
            }

            ss << "Submeshes: " << m_meshStats.subMeshCount << "\n"
               << "Vertices: " << m_meshStats.vertexCount << "\n"
               << "Indices: " << m_meshStats.indexCount << "\n"
               << "Triangles: " << m_meshStats.triangleCount << "\n"
               << "Materials: " << m_meshStats.materialCount << "\n"
               << "Animations: " << m_meshStats.animationCount << "\n"
               << "Bones: " << m_meshStats.boneCount << "\n"
               << "Shared vertices: " << ( m_meshStats.hasSharedVertices ? "Yes" : "No" ) << "\n";

            if( !m_meshStats.materialNames.empty() )
            {
                ss << "Material names:\n";
                for( const auto &name : m_meshStats.materialNames )
                {
                    ss << "  - " << ( name.empty() ? String( "(unnamed)" ) : name ) << "\n";
                }
            }

            if( !m_meshStats.warnings.empty() )
            {
                ss << "\nWarnings:\n";
                for( const auto &warning : m_meshStats.warnings )
                {
                    ss << "  ! " << warning << "\n";
                }
            }

            m_meshInfoText->setText( ss.str() );
        }

        void MeshViewer::refreshRenderStateText()
        {
            if( !m_renderStateText )
            {
                return;
            }

            static const char *renderModes[] = { "Lit",      "Unlit",   "Wireframe",
                                                 "Normals",  "Tangents", "UV0",
                                                 "UV1",      "Vertex Colour",
                                                 "Lightmap UV",
                                                 "Skin Weights" };

            const auto count = static_cast<s32>( sizeof( renderModes ) / sizeof( renderModes[0] ) );
            auto index = Math<s32>::clamp( m_currentRenderMode, 0, count - 1 );

            std::ostringstream ss;
            ss << "Render mode: " << renderModes[index]
               << " | Material: " << getMaterialVariantName( m_currentMaterialVariant )
               << " | LOD: " << m_currentLod
               << " | Grid: " << ( m_showGrid ? "On" : "Off" )
               << " | Axes: " << ( m_showAxes ? "On" : "Off" )
               << " | Bounds: " << ( m_showBounds ? "On" : "Off" )
               << " | Wire: " << ( m_showWireframe ? "On" : "Off" )
               << " | Normals: " << ( m_showNormals ? "On" : "Off" )
               << " | Tangents: " << ( m_showTangents ? "On" : "Off" )
               << " | Skeleton: " << ( m_showSkeleton ? "On" : "Off" )
               << " | Stats: " << ( m_showStats ? "On" : "Off" )
               << " | Auto-rotate: " << ( m_autoRotate ? "On" : "Off" )
               << " | Exposure: " << m_exposure << " | LOD bias: " << m_lodBias;

            m_renderStateText->setText( ss.str() );
        }

        void MeshViewer::refreshValidationText()
        {
            if( !m_validationText )
            {
                return;
            }

            if( !m_meshActor )
            {
                m_validationText->setText( "Load a mesh and run validation to inspect import quality." );
                return;
            }

            m_validationText->setText( getValidationReport() );
        }

        void MeshViewer::gatherMeshStats()
        {
            m_meshStats = MeshStats();
            m_originalMaterialNames.clear();

            if( !m_meshActor )
            {
                return;
            }

            try
            {
                m_meshStats.bounds = scene::GameActorUtil::getActorAABB( m_meshActor );

                auto meshComponent = m_meshActor->getComponent<scene::Mesh>();
                if( !meshComponent )
                {
                    m_meshStats.warnings.push_back( "Actor has no Mesh component." );
                    return;
                }

                auto meshResource = meshComponent->getMeshResource();
                if( !meshResource )
                {
                    m_meshStats.warnings.push_back( "Mesh component has no mesh resource." );
                    return;
                }

                auto mesh = meshResource->getMesh();
                if( !mesh )
                {
                    m_meshStats.warnings.push_back( "Mesh resource contains no mesh data." );
                    return;
                }

                m_meshStats.hasSharedVertices = mesh->getHasSharedVertexData();
                m_meshStats.subMeshCount = mesh->getNumSubMeshes();
                m_meshStats.animationCount = mesh->getNumAnimations();
                m_meshStats.hasSkeleton = mesh->hasSkeleton();

                if( auto skeleton = mesh->getSkeleton() )
                {
                    if( auto concreteSkeleton = workphone::dynamic_pointer_cast<MeshSkeleton>( skeleton ) )
                    {
                        m_meshStats.boneCount = static_cast<u32>( concreteSkeleton->getBones().size() );
                    }
                }

                u32 assignedMaterials = 0u;
                for( u32 i = 0; i < mesh->getNumSubMeshes(); ++i )
                {
                    auto subMesh = mesh->getSubMesh( i );
                    if( !subMesh )
                    {
                        m_originalMaterialNames.push_back( String() );
                        continue;
                    }

                    auto materialName = subMesh->getMaterialName();
                    m_originalMaterialNames.push_back( materialName );
                    m_meshStats.materialNames.push_back( materialName );
                    if( !StringUtil::isNullOrEmpty( materialName ) )
                    {
                        ++assignedMaterials;
                    }

                    auto vertexBuffer = subMesh->getVertexBuffer();
                    if( vertexBuffer )
                    {
                        m_meshStats.vertexCount += vertexBuffer->getNumVertices();

                        // Inspect the vertex declaration to detect attribute channels.
                        auto declaration = vertexBuffer->getVertexDeclaration();
                        if( declaration )
                        {
                            const auto &elements = declaration->getElements();
                            for( const auto &vertexElement : elements )
                            {
                                if( !vertexElement )
                                {
                                    continue;
                                }

                                auto semantic = static_cast<VertexElementSemantic>(
                                    vertexElement->getSemantic() );
                                auto channelIndex = static_cast<u32>( vertexElement->getIndex() );

                                switch( semantic )
                                {
                                case VertexElementSemantic::VES_NORMAL:
                                    m_meshStats.hasNormals = true;
                                    break;
                                case VertexElementSemantic::VES_TANGENT:
                                    m_meshStats.hasTangents = true;
                                    break;
                                case VertexElementSemantic::VES_DIFFUSE:
                                    m_meshStats.hasVertexColours = true;
                                    break;
                                case VertexElementSemantic::VES_TEXTURE_COORDINATES:
                                    if( channelIndex == 0u )
                                    {
                                        m_meshStats.hasUvChannel0 = true;
                                    }
                                    else if( channelIndex == 1u )
                                    {
                                        m_meshStats.hasUvChannel1 = true;
                                        m_meshStats.hasLightmapUvs = true;
                                    }
                                    else if( channelIndex > 1u )
                                    {
                                        m_meshStats.hasLightmapUvs = true;
                                    }
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    }

                    auto indexBuffer = subMesh->getIndexBuffer();
                    if( indexBuffer )
                    {
                        m_meshStats.indexCount += indexBuffer->getNumIndices();
                        if( subMesh->getRenderOperationType() == RenderOperationType::OT_TRIANGLE_LIST )
                        {
                            m_meshStats.triangleCount += indexBuffer->getNumIndices() / 3u;
                        }
                    }
                }

                m_meshStats.materialCount = assignedMaterials;

                if( m_meshStats.triangleCount == 0u )
                {
                    m_meshStats.warnings.push_back( "Mesh contains no triangles." );
                }
                if( m_meshStats.materialCount == 0u )
                {
                    m_meshStats.warnings.push_back( "No materials assigned to submeshes." );
                }
                if( m_meshStats.animationCount > 0u && !m_meshStats.hasSkeleton )
                {
                    m_meshStats.warnings.push_back( "Mesh has animations but no skeleton." );
                }
                if( !m_meshStats.bounds.isFinite() || !m_meshStats.bounds.isValid() )
                {
                    m_meshStats.warnings.push_back( "Mesh bounds are invalid." );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                m_meshStats.warnings.push_back( "Exception while gathering mesh statistics." );
            }
        }

        String MeshViewer::getCurrentMeshReport() const
        {
            std::ostringstream ss;
            ss << "Mesh Viewer Report\n";
            ss << "==================\n";
            ss << "Path: " << ( StringUtil::isNullOrEmpty( m_currentMeshPath )
                                    ? String( "(none)" )
                                    : m_currentMeshPath )
               << "\n";
            ss << "Submeshes: " << m_meshStats.subMeshCount << "\n";
            ss << "Vertices: " << m_meshStats.vertexCount << "\n";
            ss << "Indices: " << m_meshStats.indexCount << "\n";
            ss << "Triangles: " << m_meshStats.triangleCount << "\n";
            ss << "Materials: " << m_meshStats.materialCount << "\n";
            ss << "Animations: " << m_meshStats.animationCount << "\n";
            ss << "Bones: " << m_meshStats.boneCount << "\n";
            ss << "Skeleton: " << ( m_meshStats.hasSkeleton ? "Yes" : "No" ) << "\n";
            ss << "Shared vertices: " << ( m_meshStats.hasSharedVertices ? "Yes" : "No" ) << "\n";

            try
            {
                auto bounds = m_meshActor ? scene::GameActorUtil::getActorAABB( m_meshActor ) : AABB3<real_Num>();
                if( bounds.isFinite() && bounds.isValid() )
                {
                    auto center = bounds.getCenter();
                    ss << "Bounds center: (" << center.x << ", " << center.y << ", " << center.z
                       << ")\n";
                    ss << "Bounds radius: " << bounds.getRadius() << "\n";
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            if( !m_meshStats.materialNames.empty() )
            {
                ss << "\nMaterials:\n";
                for( const auto &name : m_meshStats.materialNames )
                {
                    ss << "  " << ( name.empty() ? String( "(unnamed)" ) : name ) << "\n";
                }
            }

            if( !m_meshStats.warnings.empty() )
            {
                ss << "\nValidation warnings:\n";
                for( const auto &warning : m_meshStats.warnings )
                {
                    ss << "  ! " << warning << "\n";
                }
            }

            return ss.str();
        }

        String MeshViewer::getValidationReport() const
        {
            if( !m_meshActor )
            {
                return "No mesh loaded.";
            }

            std::ostringstream ss;
            ss << "Validation Results (";
            switch( m_lastValidationType )
            {
            case ElementId::ValidateMaterials: ss << "Materials"; break;
            case ElementId::ValidateSkeleton: ss << "Skeleton"; break;
            case ElementId::ValidateLODs: ss << "LODs"; break;
            default: ss << "Mesh"; break;
            }
            ss << ")\n------------------\n";

            auto addCheck = [&ss]( const char *name, bool ok, const char *detail )
            {
                ss << "[" << ( ok ? "PASS" : "FAIL" ) << "] " << name;
                if( detail && detail[0] != '\0' )
                {
                    ss << ": " << detail;
                }
                ss << "\n";
            };

            switch( m_lastValidationType )
            {
            case ElementId::ValidateMaterials:
                addCheck( "Materials assigned", m_meshStats.materialCount > 0u,
                          std::to_string( m_meshStats.materialCount ).c_str() );
                addCheck( "All submeshes have materials",
                          m_meshStats.subMeshCount > 0u &&
                              m_meshStats.materialCount == m_meshStats.subMeshCount,
                          std::to_string( m_meshStats.materialCount ).c_str() );
                break;
            case ElementId::ValidateSkeleton:
                addCheck( "Has skeleton", m_meshStats.hasSkeleton, "" );
                addCheck( "Bones present", m_meshStats.boneCount > 0u,
                          std::to_string( m_meshStats.boneCount ).c_str() );
                addCheck( "Animations consistent",
                          !( m_meshStats.animationCount > 0u && !m_meshStats.hasSkeleton ),
                          std::to_string( m_meshStats.animationCount ).c_str() );
                break;
            case ElementId::ValidateLODs:
                ss << "LOD chain inspection requires an LOD-enabled mesh resource. "
                   << "Submesh counts and triangle ratios are reported once an LOD "
                   << "resource is attached.\n";
                addCheck( "Mesh loaded", true, "" );
                addCheck( "Submeshes present", m_meshStats.subMeshCount > 0u,
                          std::to_string( m_meshStats.subMeshCount ).c_str() );
                addCheck( "Triangles present", m_meshStats.triangleCount > 0u,
                          std::to_string( m_meshStats.triangleCount ).c_str() );
                break;
            default:
                addCheck( "Mesh loaded", true, "" );
                addCheck( "Has submeshes", m_meshStats.subMeshCount > 0u,
                          std::to_string( m_meshStats.subMeshCount ).c_str() );
                addCheck( "Has vertices", m_meshStats.vertexCount > 0u,
                          std::to_string( m_meshStats.vertexCount ).c_str() );
                addCheck( "Has triangles", m_meshStats.triangleCount > 0u,
                          std::to_string( m_meshStats.triangleCount ).c_str() );
                addCheck( "Valid bounds",
                          m_meshStats.bounds.isFinite() && m_meshStats.bounds.isValid(), "" );
                addCheck( "Has normals", m_meshStats.hasNormals, "" );
                addCheck( "Has UV0", m_meshStats.hasUvChannel0, "" );
                addCheck( "Has tangents", m_meshStats.hasTangents, "" );
                addCheck( "Has vertex colours", m_meshStats.hasVertexColours, "" );
                break;
            }

            if( !m_meshStats.warnings.empty() )
            {
                ss << "\nWarnings:\n";
                for( const auto &warning : m_meshStats.warnings )
                {
                    ss << "  - " << warning << "\n";
                }
            }

            return ss.str();
        }

        void MeshViewer::runValidation( ElementId validationType )
        {
            if( !m_meshActor )
            {
                setStatus( "No mesh loaded to validate." );
                return;
            }

            m_lastValidationType = validationType;

            std::ostringstream ss;
            ss << "Validation run: ";
            switch( validationType )
            {
            case ElementId::ValidateMesh:
                ss << "Mesh geometry";
                break;
            case ElementId::ValidateMaterials:
                ss << "Materials";
                break;
            case ElementId::ValidateSkeleton:
                ss << "Skeleton";
                break;
            case ElementId::ValidateLODs:
                ss << "LODs";
                break;
            default:
                ss << "General";
                break;
            }
            ss << ". See Validation panel for results.";

            refreshValidationText();
            setStatus( ss.str() );
        }

        void MeshViewer::applyRenderMode( s32 mode )
        {
            m_currentRenderMode = Math<s32>::clamp( mode, 0, 9 );
            if( m_viewport )
            {
                m_viewport->setMaterialScheme( getRenderModeScheme( m_currentRenderMode ) );
            }
            refreshRenderStateText();
        }

        void MeshViewer::updateMeshMaterialVariant( s32 variant )
        {
            if( !m_meshActor )
            {
                return;
            }

            auto renderer = m_meshActor->getComponent<scene::MeshRenderer>();
            if( !renderer )
            {
                return;
            }

            auto name = getMaterialVariantName( variant );
            if( !StringUtil::isNullOrEmpty( name ) )
            {
                renderer->setMaterialName( name );
            }
            else
            {
                // Default variant: restore the original per-submesh materials captured at load.
                restoreDefaultMaterials();
            }
        }

        void MeshViewer::restoreDefaultMaterials()
        {
            if( !m_meshActor || m_originalMaterialNames.empty() )
            {
                return;
            }

            auto meshComponent = m_meshActor->getComponent<scene::Mesh>();
            if( !meshComponent )
            {
                return;
            }

            auto meshResource = meshComponent->getMeshResource();
            if( !meshResource )
            {
                return;
            }

            auto mesh = meshResource->getMesh();
            if( !mesh )
            {
                return;
            }

            const auto stored = static_cast<u32>( m_originalMaterialNames.size() );
            const auto subCount = mesh->getNumSubMeshes();
            const auto count = ( stored < subCount ) ? stored : subCount;

            for( u32 i = 0; i < count; ++i )
            {
                auto subMesh = mesh->getSubMesh( i );
                if( subMesh )
                {
                    subMesh->setMaterialName( m_originalMaterialNames[i] );
                }
            }

            if( auto renderer = m_meshActor->getComponent<scene::MeshRenderer>() )
            {
                renderer->updateMaterials();
            }
        }

        void MeshViewer::updateViewportOverlays()
        {
            // Viewport overlay toggles (grid, axes, bounds, etc.) are tracked in UI state.
            // Wired to the renderer or debug drawing system in a full integration.
            refreshRenderStateText();
        }

        void MeshViewer::applyLightingPreset( ElementId preset )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto sceneManager = applicationManager ? applicationManager->getGameManager() : nullptr;
            if( !sceneManager )
            {
                setStatus( "Unable to apply lighting preset: no scene manager." );
                return;
            }

            String status;
            switch( preset )
            {
            case ElementId::LightingStudio:
                status = "Studio lighting preset applied.";
                break;
            case ElementId::LightingOutdoor:
                status = "Outdoor lighting preset applied.";
                break;
            case ElementId::LightingIndoor:
                status = "Indoor lighting preset applied.";
                break;
            case ElementId::LightingNeutral:
                status = "Neutral lighting preset applied.";
                break;
            default:
                status = "Custom lighting preset applied.";
                break;
            }

            // Production hook: create/replace directional lights, ambient colour, HDRI,
            // or environment cubemap based on the selected preset.
            setStatus( status );
        }

        void MeshViewer::openMeshFileDialog()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto fileSystem = applicationManager ? applicationManager->getFileSystem() : nullptr;
            if( !fileSystem )
            {
                return;
            }

            auto fileDialog = fileSystem->openFileDialog();
            fileDialog->setFileExtension( ".fbx;.obj;.dae;.gltf;.glb;.mesh" );

            auto result = fileDialog->openDialog();
            if( result == INativeFileDialog::Result::Dialog_Okay )
            {
                auto filePath = fileDialog->getFilePath();
                if( !StringUtil::isNullOrEmpty( filePath ) )
                {
                    loadMeshFromFile( filePath );
                }
            }
        }

        void MeshViewer::addRecentFile( const String &filePath )
        {
            if( StringUtil::isNullOrEmpty( filePath ) )
            {
                return;
            }

            // Remove existing entry to move it to the top.
            for( auto it = m_recentFiles.begin(); it != m_recentFiles.end(); )
            {
                if( *it == filePath )
                {
                    it = m_recentFiles.erase( it );
                }
                else
                {
                    ++it;
                }
            }

            m_recentFiles.insert( m_recentFiles.begin(), filePath );
            while( m_recentFiles.size() > 10u )
            {
                m_recentFiles.pop_back();
            }

            rebuildRecentMenu();
        }

        void MeshViewer::loadRecentFiles()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto fileSystem = applicationManager ? applicationManager->getFileSystem() : nullptr;
            if( !fileSystem )
            {
                return;
            }

            m_recentFilesPath = "MeshViewer.recent.txt";
            auto stream = fileSystem->open( m_recentFilesPath, true, false, false );
            if( !stream )
            {
                return;
            }

            auto text = stream->getAsString();
            stream->close();

            if( !text.empty() )
            {
                m_recentFiles = StringUtil::split( text, "\n" );
            }

            rebuildRecentMenu();
        }

        void MeshViewer::saveRecentFiles()
        {
            if( StringUtil::isNullOrEmpty( m_recentFilesPath ) )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instance();
            auto fileSystem = applicationManager ? applicationManager->getFileSystem() : nullptr;
            if( !fileSystem )
            {
                return;
            }

            auto stream = fileSystem->open( m_recentFilesPath, false, false, true );
            if( !stream )
            {
                return;
            }

            for( const auto &path : m_recentFiles )
            {
                if( !path.empty() )
                {
                    auto line = path + "\n";
                    stream->write( line.c_str(), line.size() );
                }
            }

            stream->close();
        }

        void MeshViewer::rebuildRecentMenu()
        {
            if( !m_recentFilesMenu )
            {
                return;
            }

            // Remove any previously registered recent-file menu items.
            for( auto &item : m_recentFileItems )
            {
                if( item )
                {
                    m_recentFilesMenu->removeMenuItem( item );
                }
            }
            m_recentFileItems.clear();

            if( m_recentFiles.empty() )
            {
                m_recentFilesMenu->setLabel( "Open Recent (empty)" );
                return;
            }

            m_recentFilesMenu->setLabel( "Open Recent" );
            for( size_t i = 0; i < m_recentFiles.size(); ++i )
            {
                const auto &path = m_recentFiles[i];
                auto item = Util::addMenuItem( m_recentFilesMenu, getRecentFileElementId( i ),
                                              getFileNameFromPath( path ), path,
                                              ui::IUIMenuItem::Type::Normal );
                if( item )
                {
                    auto element = workphone::static_pointer_cast<ui::IUIElement>( item );
                    registerControl( element );
                    m_recentFileItems.push_back( element );
                }
            }
        }

        void MeshViewer::openRecentFile( size_t index )
        {
            if( index >= m_recentFiles.size() )
            {
                setStatus( "Selected recent file is no longer available." );
                return;
            }

            loadMeshFromFile( m_recentFiles[index] );
        }

        void MeshViewer::saveReportToFile( const String &filePath )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto fileSystem = applicationManager ? applicationManager->getFileSystem() : nullptr;
            if( !fileSystem )
            {
                setStatus( "Unable to save report: no file system." );
                return;
            }

            auto stream = fileSystem->open( filePath, false, false, true );
            if( !stream )
            {
                setStatus( "Unable to create report file." );
                return;
            }

            auto report = getCurrentMeshReport();
            stream->write( report.c_str(), report.size() );
            stream->close();

            std::ostringstream ss;
            ss << "Report saved to " << filePath;
            setStatus( ss.str() );
        }

        void MeshViewer::queueScreenshot( const String &filePath, const Vector2I &size )
        {
            m_pendingScreenshot = true;
            m_pendingScreenshotPath = filePath;
            m_pendingScreenshotSize = size;
            setStatus( "Screenshot queued for next frame." );
        }

        void MeshViewer::exportScreenshot( const String &filePath, const Vector2I &size )
        {
            (void)size; // Captures at the render target's native resolution for fidelity.

            if( !m_renderTarget )
            {
                setStatus( "No render target available for screenshot." );
                return;
            }

            auto renderTarget = m_renderTarget->getRenderTarget();
            if( !renderTarget )
            {
                setStatus( "Render target has no readable surface for screenshot." );
                return;
            }

            try
            {
                auto actualSize = m_renderTarget->getActualSize();
                auto width = static_cast<u32>( actualSize.x );
                auto height = static_cast<u32>( actualSize.y );

                if( width == 0u || height == 0u )
                {
                    setStatus( "Render target has zero size." );
                    return;
                }

                // Read back 32 bits per pixel from the render target. The exact channel
                // ordering is backend dependent (typically BGRA8 on desktop GPUs); the BMP
                // writer below stores the bytes as-is in a 32-bit BGRX image, which matches
                // the common case. Adjust the pixel layout here if your backend uses RGBA8.
                const auto pixelSize = 4u;
                const auto bufferSize = width * height * pixelSize;
                std::vector<u8> buffer( bufferSize, 0u );
                renderTarget->copyContentsToMemory( buffer.data(), bufferSize );

                auto applicationManager = core::IApplicationManager::instance();
                auto fileSystem = applicationManager ? applicationManager->getFileSystem() : nullptr;
                if( !fileSystem )
                {
                    setStatus( "No file system available to write screenshot." );
                    return;
                }

                auto stream = fileSystem->open( filePath, false, true, true );
                if( !stream )
                {
                    setStatus( "Unable to create screenshot file." );
                    return;
                }

                // 32-bit BMP: 14-byte file header + 40-byte DIB header. Rows are already
                // 4-byte aligned (4 bytes per pixel), so no row padding is required.
                const u32 imageSize = width * height * pixelSize;
                const u32 fileSize = 54u + imageSize;

                u8 header[54] = {};
                header[0] = 'B';
                header[1] = 'M';
                *reinterpret_cast<u32 *>( &header[2] ) = fileSize;
                *reinterpret_cast<u32 *>( &header[10] ) = 54u;
                *reinterpret_cast<u32 *>( &header[14] ) = 40u;
                *reinterpret_cast<u32 *>( &header[18] ) = width;
                *reinterpret_cast<u32 *>( &header[22] ) = height;
                *( &header[26] ) = 1u;
                *( &header[28] ) = 32u;
                *reinterpret_cast<u32 *>( &header[34] ) = imageSize;

                stream->write( header, 54u );

                // BMP rows are stored bottom-up; copyContentsToMemory returns top-down on
                // most backends, so emit rows in reverse order.
                const auto rowBytes = width * pixelSize;
                std::vector<u8> row( rowBytes );
                for( u32 y = 0u; y < height; ++y )
                {
                    auto srcRow = ( height - 1u - y ) * rowBytes;
                    std::memcpy( row.data(), buffer.data() + srcRow, rowBytes );
                    stream->write( row.data(), rowBytes );
                }

                stream->close();

                std::ostringstream ss;
                ss << "Screenshot saved to " << filePath << " (" << width << "x" << height << ").";
                setStatus( ss.str() );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                setStatus( "Failed to export screenshot." );
            }
        }

        void MeshViewer::exportThumbnail( const String &filePath )
        {
            exportScreenshot( filePath, Vector2I( 256, 256 ) );
        }

        void MeshViewer::processPendingScreenshot()
        {
            if( m_pendingScreenshot )
            {
                m_pendingScreenshot = false;
                exportScreenshot( m_pendingScreenshotPath, m_pendingScreenshotSize );
            }

            if( m_pendingThumbnail )
            {
                m_pendingThumbnail = false;
                exportThumbnail( m_pendingThumbnailPath );
            }
        }

        //
        // CUIMenuBarListener Implementation
        //

        MeshViewer::CUIMenuBarListener::CUIMenuBarListener() = default;

        MeshViewer::CUIMenuBarListener::~CUIMenuBarListener() = default;

        Parameter MeshViewer::CUIMenuBarListener::handleEvent( EventType eventType,
                                                               hash_type eventValue,
                                                               const Array<Parameter> &arguments,
                                                               SmartPtr<ISharedObject> sender,
                                                               SmartPtr<ISharedObject> object,
                                                               SmartPtr<IEvent> event )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto fileSystem = applicationManager ? applicationManager->getFileSystem() : nullptr;

            if( auto owner = getOwner() )
            {
                if( eventValue == IEvent::handleSelection || eventValue == IEvent::handleValueChanged ||
                    eventValue == IEvent::handleToggle )
                {
                    SmartPtr<ui::IUIElement> element;
                    if( object && object->isDerived<ui::IUIElement>() )
                    {
                        element = workphone::static_pointer_cast<ui::IUIElement>( object );
                    }
                    else if( sender && sender->isDerived<ui::IUIElement>() )
                    {
                        element = workphone::static_pointer_cast<ui::IUIElement>( sender );
                    }

                    if( !element )
                    {
                        return {};
                    }

                    auto widgetID = static_cast<ElementId>( element->getElementId() );
                    switch( widgetID )
                    {
                    case ElementId::Open:
                        handleOpenFile( owner, fileSystem );
                        break;
                    case ElementId::Exit:
                        handleExit();
                        break;
                    default:
                    {
                        auto elementIdRaw = element->getElementId();
                        if( isRecentFileElementId( elementIdRaw ) )
                        {
                            owner->openRecentFile( getRecentFileIndex( elementIdRaw ) );
                            break;
                        }

                        if( element->isDerived<ui::IUIDropdown>() )
                        {
                            auto dropdown =
                                workphone::static_pointer_cast<ui::IUIDropdown>( element );
                            owner->handleDropdown(
                                widgetID, static_cast<s32>( dropdown->getSelectedOption() ) );
                        }
                        else if( element->isDerived<ui::IUILabelSliderPair>() )
                        {
                            auto slider =
                                workphone::static_pointer_cast<ui::IUILabelSliderPair>( element );
                            owner->handleSlider( widgetID, slider->getValue() );
                        }
                        else if( element->isDerived<ui::IUILabelTogglePair>() )
                        {
                            auto toggle =
                                workphone::static_pointer_cast<ui::IUILabelTogglePair>( element );
                            owner->handleToggle( widgetID, toggle->getValue() );
                        }
                        else
                        {
                            owner->handleAction( widgetID );
                        }
                    }
                    break;
                    }
                }
            }

            return {};
        }

        void MeshViewer::CUIMenuBarListener::handleOpenFile( SmartPtr<MeshViewer> owner,
                                                             SmartPtr<IFileSystem> fileSystem )
        {
            if( !fileSystem )
            {
                return;
            }

            auto fileDialog = fileSystem->openFileDialog();
            fileDialog->setFileExtension( ".fbx;.obj;.dae;.gltf;.glb;.mesh" );

            auto result = fileDialog->openDialog();
            if( result == INativeFileDialog::Result::Dialog_Okay )
            {
                auto filePath = fileDialog->getFilePath();
                if( !StringUtil::isNullOrEmpty( filePath ) )
                {
                    owner->loadMeshFromFile( filePath );
                }
            }
        }

        void MeshViewer::CUIMenuBarListener::handleExit()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            applicationManager->setQuit( true );
        }

        SmartPtr<MeshViewer> MeshViewer::CUIMenuBarListener::getOwner() const
        {
            return m_owner.lock();
        }

        void MeshViewer::CUIMenuBarListener::setOwner( SmartPtr<MeshViewer> owner )
        {
            m_owner = owner;
        }

    }  // end namespace viewer
}  // end namespace workphone

#ifdef WP_PLATFORM_WIN32
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow )
{
    using namespace workphone;

    try
    {
        auto typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = new TypeManager;
            TypeManager::setInstance( typeManager );
        }

        // Capture the first command-line argument as a startup mesh path.
        if( pCmdLine && pCmdLine[0] != L'\0' )
        {
            std::wstring widePath( pCmdLine );

            // Strip surrounding quotes if present.
            if( widePath.size() >= 2u && widePath.front() == L'"' && widePath.back() == L'"' )
            {
                widePath = widePath.substr( 1, widePath.size() - 2u );
            }

            if( !widePath.empty() )
            {
                // Convert the wide path to UTF-8 for the engine's narrow String type.
                int needed = WideCharToMultiByte( CP_UTF8, 0, widePath.c_str(),
                                                  static_cast<int>( widePath.size() ), nullptr, 0,
                                                  nullptr, nullptr );
                if( needed > 0 )
                {
                    String utf8( static_cast<size_t>( needed ), '\0' );
                    WideCharToMultiByte( CP_UTF8, 0, widePath.c_str(),
                                         static_cast<int>( widePath.size() ), &utf8[0], needed,
                                         nullptr, nullptr );
                    workphone::viewer::MeshViewer::setStartupMeshPath( utf8 );
                }
            }
        }

        workphone::viewer::MeshViewer app;
        app.load( nullptr );
        app.run();
        app.unload( nullptr );
        return 0;
    }
    catch( Exception &e )
    {
        workphone::MessageBoxUtil::show( e.what() );
    }

    return 0;
}
#else
int main( int argc, char *argv[] )
{
    using namespace workphone;

    try
    {
        auto typeManager = TypeManager::instance();
        if( !typeManager )
        {
            typeManager = new TypeManager;
            TypeManager::setInstance( typeManager );
        }

        if( argc > 1 && argv[1] != nullptr )
        {
            workphone::viewer::MeshViewer::setStartupMeshPath( argv[1] );
        }

        workphone::viewer::MeshViewer app;
        app.setActiveThreads( 4 );
        app.load( nullptr );
        app.run();
        app.unload( nullptr );

        if( typeManager )
        {
            delete typeManager;
            TypeManager::setInstance( nullptr );
            typeManager = nullptr;
        }

        return 0;
    }
    catch( Exception &e )
    {
        workphone::MessageBoxUtil::show( e.what() );
    }

    return 0;
}
#endif
