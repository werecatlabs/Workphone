#include <EditorPCH.hpp>
#include <ui/ImageToMeshWindow.hpp>
#include <ui/ImageToMeshAssets.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/AI/Trellis2Client.hpp>
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
        window->setSize( { 720.0f, 640.0f } );
        auto placement = window->getProperties();
        placement->setProperty( "initialPlacement", String( "floating" ) );
        window->setProperties( placement );
        setParentWindow( window );
        m_listener = make_ptr<Listener>();
        m_listener->owner = this;

        m_provider = ui->addElementByType<ui::IUIDropdown>();
        m_provider->setLabel( "Generation provider" );
        m_provider->setOptions( { "Meshy", "TRELLIS 2 (fal.ai)" } );
        m_provider->setSelectedOption( static_cast<u32>( Provider::Meshy ) );
        window->addChild( m_provider );
        m_hint = ui->addElementByType<ui::IUIText>();
        window->addChild( m_hint );

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
        m_resolution = ui->addElementByType<ui::IUIDropdown>();
        m_resolution->setLabel( "TRELLIS 2 resolution" );
        m_resolution->setOptions( { "512", "1024", "1536" } );
        m_resolution->setSelectedOption( 1 );
        window->addChild( m_resolution );
        // The Editor backend implements labelled toggles; IUICheckbox has no
        // registered ImGui factory and would return a null element here.
        m_textured = ui->addElementByType<ui::IUILabelTogglePair>();
        m_textured->setLabel( "Generate textures" );
        m_textured->setValue( true );
        window->addChild( m_textured );
        m_selectedProvider = m_jobProvider = Provider::Meshy;
        m_meshyTextured = true;
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
        m_hint = nullptr;
        m_imagePath = m_name = m_status = nullptr;
        m_provider = m_polycount = m_resolution = nullptr;
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
            refreshProvider();
            if( app->getProjectPath() != m_displayProjectPath )
            {
                m_displayProjectPath = app->getProjectPath();
                m_imagePath->setText( "" );
                m_name->setText( "generated-mesh" );
                if( m_result )
                    discardPending( String( "Project changed. Discarding the mesh locally; the " ) +
                        providerName( m_jobProvider ) + " task may still run and use credits." );
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
                    discardPending( String( "Project catalog changed. Discarding the mesh locally; the " ) +
                        providerName( m_jobProvider ) + " task may still run and use credits." );
            }
            if( action == Cancel && m_result )
                discardPending( String( "Stopping local processing. The " ) + providerName( m_jobProvider ) +
                    " task may still run and use credits; no mesh will be saved." );
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
        dialog->setFileExtension( ".png,.jpg,.jpeg" );
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
        m_message = String( "Image selected. Generate Mesh uploads it to " ) +
                    providerName( m_selectedProvider ) + " and starts one generation task.";
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
        const auto provider = m_selectedProvider;
        const std::uint32_t counts[] = { 10000, 30000, 100000 };
        request.targetPolycount = counts[std::min<u32>( m_polycount->getSelectedOption(), 2 )];
        request.textured = provider == Provider::Trellis2 || m_textured->getValue();
        const std::uint32_t resolutions[] = { 512, 1024, 1536 };
        const auto resolution = resolutions[std::min<u32>( m_resolution->getSelectedOption(), 2 )];
        m_basename = mesh_assets::sanitizeBasename( m_name->getText().c_str() ).c_str();
        m_name->setText( m_basename );
        if( m_worker.joinable() ) m_worker.join();
        auto result = std::make_shared<ResultState>();
        result->progress.stage = "Reading source image";
        std::thread worker( [source = std::move( source ), request = std::move( request ),
                             provider, resolution, result]() mutable {
            Thread::setCurrentTask( TaskId::None );
            try
            {
                if( result->cancel.load( std::memory_order_acquire ) ) result->result.cancelled = true;
                else if( mesh_assets::readImage( source, request.imageBytes, request.mimeType,
                                                result->result.error ) )
                {
                    const auto progress = [result]( const mesh_generation::Progress &value ) {
                        std::lock_guard<std::mutex> lock( result->progressMutex );
                        result->progress = value;
                    };
                    if( provider == Provider::Trellis2 )
                    {
                        mesh_generation::Trellis2Client client;
                        result->result = client.generate( request, resolution, result->cancel, progress );
                    }
                    else
                    {
                        mesh_generation::MeshyClient client;
                        result->result = client.generate( request, result->cancel, progress );
                    }
                }
            }
            catch( ... )
            {
                result->result.error = std::string( "Mesh generation failed. Check the image, network and " ) +
                                       providerName( provider ) + " configuration.";
            }
            result->ready.store( true, std::memory_order_release );
        } );
        m_jobProvider = provider;
        m_result = result;
        m_worker = std::move( worker );
        m_discard = false;
        m_taskId.clear();
        m_nextSessionCheck = std::chrono::steady_clock::now() + std::chrono::seconds( 1 );
        m_message = String( "Preparing image for " ) + providerName( provider ) +
                    "... You can continue editing.";
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
            m_message = String( "Mesh discarded locally. No asset was saved. The " ) +
                        providerName( m_jobProvider ) + " task may still run and use credits.";
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

    const char *ImageToMeshWindow::providerName( Provider provider )
    {
        return provider == Provider::Trellis2 ? "TRELLIS 2 (fal.ai)" : "Meshy";
    }

    void ImageToMeshWindow::refreshProvider()
    {
        const auto selected = m_provider->getSelectedOption() == static_cast<u32>( Provider::Trellis2 )
                                  ? Provider::Trellis2 : Provider::Meshy;
        const auto provider = m_result ? m_jobProvider : selected;
        // Keep an active job's options and status tied to the captured provider,
        // including a selection queued just before the controls were disabled.
        if( m_result ) m_provider->setSelectedOption( static_cast<u32>( provider ) );
        if( provider != m_selectedProvider )
        {
            if( m_selectedProvider == Provider::Meshy ) m_meshyTextured = m_textured->getValue();
            m_selectedProvider = provider;
            m_textured->setValue( provider == Provider::Trellis2 || m_meshyTextured );
            if( !m_result )
            {
                m_message.clear();
                m_taskId.clear();
            }
        }
        const String countLabel = provider == Provider::Trellis2 ? "Target vertex count" : "Target polygon count";
        if( m_polycount->getLabel() != countLabel ) m_polycount->setLabel( countLabel );
        const String hint = provider == Provider::Trellis2
            ? "Generate a textured 3D object with TRELLIS 2 hosted by fal.ai.\n"
              "Set a separate fal.ai API key in File > Settings > Editor > TRELLIS 2 (fal.ai).\n"
              "Generate Mesh uploads the image to fal.ai and uses your fal.ai credits.\n"
              "GLB assets are saved in Assets/Generated without replacing existing files."
            : "Generate a 3D object from a PNG or JPEG with Meshy.\n"
              "Set a separate Meshy API key in File > Settings > Editor > Meshy.\n"
              "Generate Mesh uploads the image to Meshy and uses your Meshy credits.\n"
              "GLB assets are saved in Assets/Generated without replacing existing files.";
        if( m_hint->getText() != hint ) m_hint->setText( hint );
    }

    void ImageToMeshWindow::refreshControls()
    {
        refreshProvider();
        const bool busy = m_result != nullptr;
        auto editor = EditorManager::getSingletonPtr();
        auto project = editor ? editor->getProject() : nullptr;
        const bool hasProject = project && !project->getFilePath().empty();
        m_imagePath->setReadOnly( busy );
        m_name->setReadOnly( busy );
        m_provider->setEnabled( !busy );
        m_polycount->setEnabled( !busy );
        m_resolution->setEnabled( !busy && m_selectedProvider == Provider::Trellis2 );
        m_textured->setEnabled( !busy && m_selectedProvider == Provider::Meshy );
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
                if( !progress.taskId.empty() && m_jobProvider == Provider::Meshy )
                    message += " (" + StringUtil::toString( std::min<std::uint32_t>( progress.percent, 100 ) ) + "%)";
                message += String( "\nYou can continue editing while " ) +
                           providerName( m_jobProvider ) + " processes the image.";
            }
        }
        if( message.empty() ) message = hasProject ? "Choose an image and click Generate Mesh."
                                                  : "Open a saved project to generate meshes.";
        if( !m_taskId.empty() ) message += String( "\n" ) + providerName( m_jobProvider ) + " task: " + m_taskId;
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
                // Discard must supersede a queued click from before Generate was
                // disabled; otherwise the user's cancellation could be lost.
                if( action == Cancel )
                {
                    window->m_action.store( Cancel );
                    return {};
                }
                int expected = None;
                window->m_action.compare_exchange_strong( expected, action );
            }
        }
        return {};
    }
}
