#include <EditorPCH.hpp>
#include <ui/ImageToMeshWindow.hpp>
#include <ui/ImageToMeshAssets.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/Workphone.hpp>
#include <filesystem>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, ImageToMeshWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, ImageToMeshWindow::Listener, IEventListener );

    ImageToMeshWindow::~ImageToMeshWindow()
    {
        unload( nullptr );
    }

    void ImageToMeshWindow::load( SmartPtr<ISharedObject> data )
    {
        if( getParentWindow() ) return;
        EditorWindow::load( data );
        auto app = core::IApplicationManager::instance();
        auto ui = app->getUI();
        m_displayProjectPath = app->getProjectPath();
        auto window = ui->addElementByType<ui::IUIWindow>();
        window->setLabel( "Image to Mesh" );
        window->setSize( { 720.0f, 570.0f } );
        auto placement = window->getProperties();
        placement->setProperty( "initialPlacement", String( "floating" ) );
        window->setProperties( placement );
        setParentWindow( window );
        m_listener = make_ptr<Listener>();
        m_listener->owner = this;

        auto hint = ui->addElementByType<ui::IUIText>();
        hint->setText( "Generate a 3D object from a PNG or JPEG with Meshy.\n"
                       "Set a separate Meshy API key in File > Settings > Editor > Meshy.\n"
                       "Generate Mesh uploads the selected image to Meshy and uses your Meshy credits.\n"
                       "GLB assets are saved in Assets/Generated without replacing existing files." );
        window->addChild( hint );

        m_imagePath = ui->addElementByType<ui::IUITextEntry>();
        m_imagePath->setLabel( "Source image" );
        m_imagePath->setPlaceholder( "Choose a PNG/JPEG, or enter an image path" );
        window->addChild( m_imagePath );

        const auto button = [&]( const char *label, Action action ) {
            auto control = ui->addElementByType<ui::IUIButton>();
            control->setLabel( label );
            control->setElementId( action );
            control->addObjectListener( m_listener );
            window->addChild( control );
            return control;
        };
        m_browse = button( "Browse Image...", Browse );
        m_name = ui->addElementByType<ui::IUITextEntry>();
        m_name->setLabel( "Mesh asset name" );
        m_name->setText( "generated-mesh" );
        window->addChild( m_name );
        m_polycount = ui->addElementByType<ui::IUIDropdown>();
        m_polycount->setLabel( "Target polygon count" );
        m_polycount->setOptions( { "10,000", "30,000", "100,000" } );
        m_polycount->setSelectedOption( 0 );
        window->addChild( m_polycount );
        m_textured = ui->addElementByType<ui::IUICheckbox>();
        m_textured->setLabel( "Generate textures" );
        m_textured->setValue( true );
        window->addChild( m_textured );
        m_generate = button( "Generate Mesh", Generate );
        m_cancel = button( "Discard Pending Mesh", Cancel );
        m_cancel->setSameLine( true );
        m_cancel->setEnabled( false );
        m_status = ui->addElementByType<ui::IUITextEntry>();
        m_status->setLabel( "Mesh generation status" );
        m_status->setReadOnly( true );
        m_status->setMultiline( true );
        m_status->setSize( { 650.0f, 150.0f } );
        window->addChild( m_status );
        setLoadingState( LoadingState::Loaded );
        setWindowVisible( false );
        refreshControls();
    }

    void ImageToMeshWindow::unload( SmartPtr<ISharedObject> data )
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        setLoadingState( LoadingState::Unloading );
        if( m_listener ) m_listener->owner = nullptr;
        // The worker owns no window, project, engine service or UI references.
        // Stop local polling/download and join before DLL/allocator teardown.
        if( m_result ) m_result->cancel.store( true, std::memory_order_release );
        if( m_worker.joinable() ) m_worker.join();
        m_result.reset();
        m_action = None;
        m_session.clear();
        if( auto window = getParentWindow() )
        {
            auto app = core::IApplicationManager::instancePtr();
            if( auto ui = app ? app->getUIPtr() : nullptr )
            {
                for( auto child : window->getChildren() )
                {
                    if( m_listener ) child->removeObjectListener( m_listener );
                    ui->removeElement( child );
                }
                ui->removeElement( window );
            }
            setParentWindow( nullptr );
        }
        m_listener = nullptr;
        m_imagePath = m_name = m_status = nullptr;
        m_polycount = nullptr;
        m_textured = nullptr;
        m_browse = m_generate = m_cancel = nullptr;
        m_taskId.clear();
        m_message.clear();
        m_displayProjectPath.clear();
        EditorWindow::unload( data );
    }

    bool ImageToMeshWindow::sessionCurrent() const
    {
        auto app = core::IApplicationManager::instancePtr();
        auto resources = app ? app->getResourceDatabase() : nullptr;
        auto catalog = resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() )
                                 : nullptr;
        return app && app->getProjectPath() == m_projectPath && catalog && !catalog->isReadOnly() &&
               catalog->getProjectRoot() == m_projectRoot && m_session.isCurrent( catalog );
    }

    void ImageToMeshWindow::discardPending( const String &message )
    {
        if( !m_result ) return;
        m_discard = true;
        m_result->cancel.store( true, std::memory_order_release );
        m_message = message;
    }

    void ImageToMeshWindow::update()
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        if( !m_imagePath ) return;
        auto app = core::IApplicationManager::instancePtr();
        if( !app ) return;
        const bool wasBusy = m_result != nullptr;
        const auto action = static_cast<Action>( m_action.exchange( None ) );
        try
        {
            if( app->getProjectPath() != m_displayProjectPath )
            {
                m_displayProjectPath = app->getProjectPath();
                m_imagePath->setText( "" );
                m_name->setText( "generated-mesh" );
                if( m_result )
                    discardPending( "Project changed. Discarding the mesh locally; the Meshy task may still run and use credits." );
                else
                {
                    m_message.clear();
                    m_taskId.clear();
                }
            }
            const auto now = std::chrono::steady_clock::now();
            if( m_result && !m_discard && now >= m_nextSessionCheck )
            {
                m_nextSessionCheck = now + std::chrono::seconds( 1 );
                if( !sessionCurrent() )
                    discardPending( "Project catalog changed. Discarding the mesh locally; the Meshy task may still run and use credits." );
            }
            if( action == Cancel && m_result )
                discardPending( "Stopping local processing. The Meshy task may still run and use credits; no mesh will be saved." );
            if( m_result && m_result->ready.load( std::memory_order_acquire ) ) finishGeneration();
            // Ignore a second click queued while controls were becoming disabled,
            // even if a very fast request completed before this tick.
            if( !wasBusy && !m_result )
            {
                if( action == Browse ) browseImage();
                else if( action == Generate ) startGeneration();
            }
        }
        catch( const std::exception & )
        {
            m_message = "Could not complete the mesh operation. Check the image and active project.";
        }
        refreshControls();
    }

    void ImageToMeshWindow::browseImage()
    {
        auto app = core::IApplicationManager::instance();
        auto fileSystem = app->getFileSystem();
        auto dialog = fileSystem ? fileSystem->openFileDialog() : nullptr;
        if( !dialog )
        {
            m_message = "Image browsing is unavailable. Enter a PNG or JPEG path in Source image.";
            return;
        }
        dialog->setDialogMode( INativeFileDialog::DialogMode::Open );
        dialog->setFileExtension( ".png;.jpg;.jpeg" );
        dialog->setFilePath( app->getProjectPath() );
        const auto selected = dialog->openDialog();
        if( selected == INativeFileDialog::Result::Dialog_Cancel ) return;
        if( selected != INativeFileDialog::Result::Dialog_Okay )
        {
            m_message = "Could not open the image picker. Enter a PNG or JPEG path in Source image.";
            return;
        }
        const auto path = dialog->getFilePath();
        m_imagePath->setText( path );
        if( m_name->getText().empty() || m_name->getText() == "generated-mesh" )
        {
            const auto stem = std::filesystem::u8path( path.c_str() ).stem().u8string();
            m_name->setText( mesh_assets::sanitizeBasename( stem ).c_str() );
        }
        m_taskId.clear();
        m_message = "Image selected. Generate Mesh uploads it to Meshy and starts one generation task.";
    }

    void ImageToMeshWindow::startGeneration()
    {
        auto app = core::IApplicationManager::instance();
        auto editor = EditorManager::getSingletonPtr();
        auto project = editor ? editor->getProject() : nullptr;
        auto resources = app->getResourceDatabase();
        auto catalog = resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() )
                                 : nullptr;
        String error;
        m_projectPath = app->getProjectPath();
        if( !project || project->getFilePath().empty() || m_projectPath.empty() || !catalog ||
            catalog->isReadOnly() || !canonicalAssetCatalogRoot( m_projectPath, m_projectRoot, error ) ||
            catalog->getProjectRoot() != m_projectRoot || !m_session.capture( catalog ) )
        {
            m_message = "Open a saved project with a writable asset catalog before generating a mesh.";
            return;
        }
        const auto input = m_imagePath->getText();
        if( input.find_first_not_of( " \t\r\n" ) == String::npos )
        {
            m_message = "Choose a PNG or JPEG source image.";
            return;
        }
        auto source = std::filesystem::u8path( input.c_str() );
        if( source.is_relative() ) source = std::filesystem::u8path( m_projectRoot.c_str() ) / source;
        mesh_generation::Request request;
        const std::uint32_t counts[] = { 10000, 30000, 100000 };
        request.targetPolycount = counts[std::min<u32>( m_polycount->getSelectedOption(), 2 )];
        request.textured = m_textured->getValue();
        m_basename = mesh_assets::sanitizeBasename( m_name->getText().c_str() ).c_str();
        m_name->setText( m_basename );
        if( m_worker.joinable() ) m_worker.join();
        auto result = std::make_shared<ResultState>();
        result->progress.stage = "Reading source image";
        std::thread worker( [source = std::move( source ), request = std::move( request ), result]() mutable {
            Thread::setCurrentTask( TaskId::None );
            try
            {
                if( result->cancel.load( std::memory_order_acquire ) ) result->result.cancelled = true;
                else if( mesh_assets::readImage( source, request.imageBytes, request.mimeType,
                                                result->result.error ) )
                {
                    mesh_generation::MeshyClient client;
                    result->result = client.generate( request, result->cancel,
                        [result]( const mesh_generation::Progress &progress ) {
                            std::lock_guard<std::mutex> lock( result->progressMutex );
                            result->progress = progress;
                        } );
                }
            }
            catch( ... )
            {
                result->result.error = "Mesh generation failed. Check the image, network and Meshy configuration.";
            }
            result->ready.store( true, std::memory_order_release );
        } );
        m_result = result;
        m_worker = std::move( worker );
        m_discard = false;
        m_taskId.clear();
        m_nextSessionCheck = std::chrono::steady_clock::now() + std::chrono::seconds( 1 );
        m_message = "Preparing image for Meshy... You can continue editing.";
    }

    void ImageToMeshWindow::finishGeneration()
    {
        if( m_worker.joinable() ) m_worker.join();
        auto pending = std::move( m_result );
        auto &result = pending->result;
        m_taskId = result.taskId.c_str();
        if( m_taskId.empty() )
        {
            std::lock_guard<std::mutex> lock( pending->progressMutex );
            m_taskId = pending->progress.taskId.c_str();
        }
        if( m_discard || result.cancelled || !sessionCurrent() )
        {
            m_message = "Mesh discarded locally. No asset was saved. The Meshy task may still run and use credits.";
            return;
        }
        if( !result.error.empty() )
        {
            m_message = result.error.c_str();
            return;
        }
        std::filesystem::path output;
        std::string error;
        if( !mesh_assets::saveGlb( std::filesystem::u8path( m_projectRoot.c_str() ), m_basename.c_str(),
                                  result.glbBytes.data(), result.glbBytes.size(), output, error ) )
        {
            m_message = error.c_str();
            return;
        }
        const String relativePath = "Assets/Generated/" + String( output.filename().generic_u8string().c_str() );
        m_message = "Saved " + relativePath;
        if( !sessionCurrent() )
        {
            m_message += "\nThe project changed during saving. The GLB remains in the previous project; import was skipped.";
            return;
        }
        auto app = core::IApplicationManager::instance();
        try
        {
            if( auto fileSystem = app->getFileSystem() )
            {
                fileSystem->refreshPath( m_projectRoot, false );
                fileSystem->refreshPath( m_projectRoot + "/Assets", false );
                fileSystem->refreshPath( m_projectRoot + "/Assets/Generated", false );
            }
        }
        catch( const std::exception & )
        {
            m_message += "\nThe GLB was saved, but the asset browser needs refreshing.";
        }
        try
        {
            if( auto resources = app->getResourceDatabase() )
            {
                // Source identity must match the project-relative path used by
                // Project Assets and the prefab loader when the mesh is dragged.
                resources->importFile( relativePath );
                m_message += "\nImport requested. When ready, drag the mesh from Project Assets into the scene.";
            }
            else
                m_message += "\nImport the saved GLB from Project Assets when the resource service is available.";
        }
        catch( const std::exception & )
        {
            m_message += "\nAutomatic import failed. Import the saved GLB from Project Assets.";
        }
    }

    void ImageToMeshWindow::refreshControls()
    {
        const bool busy = m_result != nullptr;
        auto editor = EditorManager::getSingletonPtr();
        auto project = editor ? editor->getProject() : nullptr;
        const bool hasProject = project && !project->getFilePath().empty();
        m_imagePath->setReadOnly( busy );
        m_name->setReadOnly( busy );
        m_polycount->setEnabled( !busy );
        m_textured->setEnabled( !busy );
        m_browse->setEnabled( !busy );
        m_generate->setEnabled( !busy && hasProject &&
            m_imagePath->getText().find_first_not_of( " \t\r\n" ) != String::npos );
        m_cancel->setEnabled( busy && !m_discard );
        auto message = m_message;
        if( busy )
        {
            std::lock_guard<std::mutex> lock( m_result->progressMutex );
            const auto &progress = m_result->progress;
            if( !progress.taskId.empty() ) m_taskId = progress.taskId.c_str();
            if( !m_discard && !progress.stage.empty() )
            {
                message = progress.stage.c_str();
                if( !progress.taskId.empty() )
                    message += " (" + StringUtil::toString( std::min<std::uint32_t>( progress.percent, 100 ) ) + "%)";
                message += "\nYou can continue editing while Meshy processes the image.";
            }
        }
        if( message.empty() ) message = hasProject ? "Choose an image and click Generate Mesh."
                                                  : "Open a saved project to generate meshes.";
        if( !m_taskId.empty() ) message += "\nMeshy task: " + m_taskId;
        if( m_status->getText() != message ) m_status->setText( message );
    }

    Parameter ImageToMeshWindow::Listener::handleEvent( EventType, hash_type eventValue,
                                                       const Array<Parameter> &,
                                                       SmartPtr<ISharedObject> sender,
                                                       SmartPtr<ISharedObject>, SmartPtr<IEvent> )
    {
        if( eventValue == IEvent::handleSelection )
        {
            auto window = owner.load().lock();
            auto button = dynamic_pointer_cast<ui::IUIButton>( sender );
            if( window && button && window->isLoaded() && button->isEnabled() )
            {
                const auto action = button->getElementId();
                if( action < Browse || action > Cancel ) return {};
                int expected = None;
                window->m_action.compare_exchange_strong( expected, action );
            }
        }
        return {};
    }
}
