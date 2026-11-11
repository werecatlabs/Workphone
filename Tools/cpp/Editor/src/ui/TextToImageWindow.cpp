#include <EditorPCH.hpp>
#include <ui/TextToImageWindow.hpp>
#include <ui/TextToImageAssets.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, TextToImageWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, TextToImageWindow::Listener, IEventListener );

    TextToImageWindow::~TextToImageWindow()
    {
        unload( nullptr );
    }

    void TextToImageWindow::load( SmartPtr<ISharedObject> data )
    {
        if( getParentWindow() ) return;
        EditorWindow::load( data );
        auto ui = core::IApplicationManager::instance()->getUI();
        auto window = ui->addElementByType<ui::IUIWindow>();
        window->setLabel( "Text to Image" );
        window->setSize( { 700.0f, 720.0f } );
        auto placement = window->getProperties();
        placement->setProperty( "initialPlacement", String( "floating" ) );
        window->setProperties( placement );
        setParentWindow( window );
        m_listener = make_ptr<Listener>();
        m_listener->owner = this;

        auto hint = ui->addElementByType<ui::IUIText>();
        hint->setText( "Generate with OpenAI. Set your API key in File > Settings > Editor > OpenAI.\n"
                       "Prompts are sent to OpenAI and API usage is billed to your account.\n"
                       "PNG images are saved in Assets/Generated without replacing existing files." );
        window->addChild( hint );

        m_prompt = ui->addElementByType<ui::IUITextEntry>();
        m_prompt->setLabel( "Image prompt" );
        m_prompt->setMultiline( true );
        m_prompt->setSize( { 620.0f, 110.0f } );
        m_prompt->setPlaceholder( "Describe the image, style, colours and composition..." );
        window->addChild( m_prompt );
        m_name = ui->addElementByType<ui::IUITextEntry>();
        m_name->setLabel( "Asset name" );
        m_name->setText( "generated-image" );
        window->addChild( m_name );

        m_size = ui->addElementByType<ui::IUIDropdown>();
        m_size->setLabel( "Image size" );
        m_size->setOptions( { "Square (1024 x 1024)", "Landscape (1536 x 1024)",
                              "Portrait (1024 x 1536)" } );
        m_size->setSelectedOption( 0 );
        window->addChild( m_size );
        m_quality = ui->addElementByType<ui::IUIDropdown>();
        m_quality->setLabel( "Quality" );
        m_quality->setOptions( { "Low", "Medium", "High" } );
        m_quality->setSelectedOption( 1 );
        window->addChild( m_quality );

        const auto button = [&]( const char *label, Action action ) {
            auto control = ui->addElementByType<ui::IUIButton>();
            control->setLabel( label );
            control->setElementId( action );
            control->addObjectListener( m_listener );
            window->addChild( control );
            return control;
        };
        m_generate = button( "Generate Image", Generate );
        m_cancel = button( "Discard Pending Image", Cancel );
        m_cancel->setSameLine( true );
        m_cancel->setEnabled( false );
        m_status = ui->addElementByType<ui::IUITextEntry>();
        m_status->setLabel( "Image status" );
        m_status->setReadOnly( true );
        m_status->setMultiline( true );
        m_status->setSize( { 620.0f, 95.0f } );
        window->addChild( m_status );
        m_preview = ui->addElementByType<ui::IUIImage>();
        m_preview->setSize( { 300.0f, 300.0f } );
        m_preview->setVisible( false );
        window->addChild( m_preview );
        setLoadingState( LoadingState::Loaded );
        setWindowVisible( false );
    }

    void TextToImageWindow::unload( SmartPtr<ISharedObject> data )
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        // Join before the engine's DLLs and allocators can be torn down. The
        // worker never accesses this window, the project, or any UI element.
        if( m_worker.joinable() ) m_worker.join();
        m_result.reset();
        m_action = None;
        m_session.clear();
        m_previewSession.clear();
        if( m_listener ) m_listener->owner = nullptr;
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
        m_prompt = m_name = m_status = nullptr;
        m_size = m_quality = nullptr;
        m_generate = m_cancel = nullptr;
        m_preview = nullptr;
        EditorWindow::unload( data );
    }

    bool TextToImageWindow::sessionCurrent() const
    {
        auto app = core::IApplicationManager::instancePtr();
        auto resources = app ? app->getResourceDatabase() : nullptr;
        auto catalog = resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() )
                                 : nullptr;
        return app && app->getProjectPath() == m_projectPath && catalog && !catalog->isReadOnly() &&
               catalog->getProjectRoot() == m_projectRoot && m_session.isCurrent( catalog );
    }

    void TextToImageWindow::update()
    {
        // Button listeners only enqueue an action. Network work, saving and
        // texture publication are initiated here on the Application task.
        std::lock_guard<std::mutex> lock( m_mutex );
        if( !m_prompt ) return;
        auto app = core::IApplicationManager::instancePtr();
        if( !app ) return;
        const auto now = std::chrono::steady_clock::now();
        if( !m_previewProject.empty() &&
            ( app->getProjectPath() != m_previewProject || now >= m_nextPreviewCheck ) )
        {
            m_nextPreviewCheck = now + std::chrono::seconds( 1 );
            auto resources = app->getResourceDatabase();
            auto catalog = resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() )
                                     : nullptr;
            if( app->getProjectPath() != m_previewProject || !m_previewSession.isCurrent( catalog ) )
            {
                m_preview->setTexture( nullptr );
                m_preview->setVisible( false );
                m_previewProject.clear();
                m_previewSession.clear();
                if( !m_result ) m_message.clear();
            }
        }
        if( m_result && app->getProjectPath() != m_projectPath && !m_discard )
        {
            m_discard = true;
            m_message = "Project changed. The pending image will be discarded; API usage may still be billed.";
        }
        const auto action = static_cast<Action>( m_action.exchange( None ) );
        try
        {
            if( action == Cancel && m_result )
            {
                m_discard = true;
                m_message = "Pending image discarded. Waiting for OpenAI to finish; API usage may still be billed.";
            }
            if( m_result && m_result->ready.load( std::memory_order_acquire ) ) finishGeneration();
            if( action == Generate && !m_result ) startGeneration();
        }
        catch( const std::exception & )
        {
            m_message = "Could not complete image generation. Check the project and try again.";
        }
        refreshControls();
    }

    void TextToImageWindow::startGeneration()
    {
        auto app = core::IApplicationManager::instance();
        auto editor = EditorManager::getSingletonPtr();
        auto project = editor ? editor->getProject() : nullptr;
        auto resources = app->getResourceDatabase();
        auto catalog = resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() )
                                 : nullptr;
        auto ai = app->getAiManager();
        String error;
        m_projectPath = app->getProjectPath();
        if( !project || project->getFilePath().empty() || m_projectPath.empty() || !catalog ||
            catalog->isReadOnly() || !canonicalAssetCatalogRoot( m_projectPath, m_projectRoot, error ) ||
            catalog->getProjectRoot() != m_projectRoot || !m_session.capture( catalog ) )
        {
            m_message = "Open a saved project with a writable asset catalog before generating an image.";
            return;
        }
        if( !ai )
        {
            m_message = "Image generation is unavailable: the Editor AI service is not loaded.";
            return;
        }
        IAiManager::ImageGenerationRequest request;
        request.prompt = m_prompt->getText();
        if( request.prompt.find_first_not_of( " \t\r\n" ) == String::npos )
        {
            m_message = "Enter a description of the image to generate.";
            return;
        }
        const char *sizes[] = { "1024x1024", "1536x1024", "1024x1536" };
        const char *qualities[] = { "low", "medium", "high" };
        request.size = sizes[std::min<u32>( m_size->getSelectedOption(), 2 )];
        request.quality = qualities[std::min<u32>( m_quality->getSelectedOption(), 2 )];
        m_basename = image_assets::sanitizeBasename( m_name->getText().c_str() ).c_str();
        m_name->setText( m_basename );
        if( m_worker.joinable() ) m_worker.join();
        auto result = std::make_shared<ResultState>();
        std::thread worker( [ai, request, result]() {
            Thread::setCurrentTask( TaskId::None );
            try
            {
                result->result = ai->generateImage( request );
            }
            catch( ... )
            {
                result->result.error = "Image generation failed. Check your network and API configuration.";
            }
            result->ready.store( true, std::memory_order_release );
        } );
        m_result = result;
        m_worker = std::move( worker );
        m_discard = false;
        m_message = "Generating image with OpenAI... This can take a few minutes. You can keep editing.";
    }

    void TextToImageWindow::finishGeneration()
    {
        if( m_worker.joinable() ) m_worker.join();
        auto pending = std::move( m_result );
        if( m_discard || !sessionCurrent() )
        {
            m_message = "Image discarded. No asset was saved. API usage may still be billed.";
            return;
        }
        auto &result = pending->result;
        if( !result.error.empty() )
        {
            m_message = result.error;
            return;
        }
        std::filesystem::path output;
        std::string error;
        if( !image_assets::savePng( std::filesystem::u8path( m_projectRoot.c_str() ), m_basename.c_str(),
                                    result.pngBytes.data(), result.pngBytes.size(), output, error ) )
        {
            m_message = error.c_str();
            return;
        }
        const String outputPath = output.generic_u8string().c_str();
        m_message = "Saved " + outputPath;
        auto app = core::IApplicationManager::instance();
        try
        {
            if( auto fileSystem = app->getFileSystem() )
            {
                // refreshPath publishes Application-task events consumed by
                // ProjectAssetsWindow::handleApplicationEvent. Refresh parents
                // first so a newly created Generated folder is visible.
                fileSystem->refreshPath( m_projectRoot, false );
                fileSystem->refreshPath( m_projectRoot + "/Assets", false );
                fileSystem->refreshPath( m_projectRoot + "/Assets/Generated", false );
            }
            if( auto resources = app->getResourceDatabase() ) resources->importFile( outputPath );
        }
        catch( const std::exception & )
        {
            m_message += "\nThe PNG was saved, but asset refresh failed. Refresh or import it from Project Assets.";
        }
        try
        {
            m_preview->setTexture( nullptr );
            m_preview->setVisible( false );
            m_previewProject = m_projectPath;
            if( auto resources = app->getResourceDatabase() )
                m_previewSession.capture( dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() ) );
            m_nextPreviewCheck = std::chrono::steady_clock::now() + std::chrono::seconds( 1 );
            auto graphics = app->getGraphicsSystemPtr();
            auto textures = graphics ? graphics->getTextureManager() : nullptr;
            if( textures )
            {
                if( auto texture = dynamic_pointer_cast<render::ITexture>( textures->loadFromFile( outputPath ) ) )
                {
                    m_preview->setTexture( texture );
                    const auto dimensions = texture->getSize();
                    if( dimensions.X() > 0 && dimensions.Y() > 0 )
                    {
                        const auto scale = 300.0f / std::max<f32>( dimensions.X(), dimensions.Y() );
                        m_preview->setSize( { dimensions.X() * scale, dimensions.Y() * scale } );
                    }
                    m_preview->setVisible( true );
                }
            }
        }
        catch( const std::exception & )
        {
            m_message += "\nThe PNG was saved, but the preview could not be loaded.";
        }
    }

    void TextToImageWindow::refreshControls()
    {
        const bool busy = m_result != nullptr;
        auto editor = EditorManager::getSingletonPtr();
        auto project = editor ? editor->getProject() : nullptr;
        const bool hasProject = project && !project->getFilePath().empty();
        m_prompt->setReadOnly( busy );
        m_name->setReadOnly( busy );
        m_size->setEnabled( !busy );
        m_quality->setEnabled( !busy );
        m_generate->setEnabled( !busy && hasProject );
        m_cancel->setEnabled( busy && !m_discard );
        auto message = m_message;
        if( message.empty() ) message = hasProject ? "Describe an image and click Generate Image."
                                                  : "Open a saved project to generate images.";
        if( m_status->getText() != message ) m_status->setText( message );
    }

    Parameter TextToImageWindow::Listener::handleEvent( EventType, hash_type eventValue,
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
                int expected = None;
                window->m_action.compare_exchange_strong( expected, button->getElementId() );
            }
        }
        return {};
    }
}
