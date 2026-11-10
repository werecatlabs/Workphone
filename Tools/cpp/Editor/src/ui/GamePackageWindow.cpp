#include <EditorPCH.hpp>
#include <EditorApplication.hpp>
#include <ui/GamePackageWindow.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Workphone.hpp>
#include <filesystem>
#include <stdexcept>

#ifdef _WIN32
#    include <Windows.h>
#    include <shellapi.h>
#endif

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, GamePackageWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, GamePackageWindow::Listener, IEventListener );

    namespace
    {
        namespace fs = std::filesystem;

        SmartPtr<Project> currentProject()
        {
            auto editor = EditorManager::getSingletonPtr();
            return editor ? editor->getProject() : nullptr;
        }

        String availableDestination( const fs::path &root )
        {
            auto output = root / "WindowsGame";
            std::error_code error;
            for( unsigned index = 2; fs::exists( output, error ) && !error; ++index )
            {
                output = root / ( "WindowsGame-" + std::to_string( index ) );
            }
            return output.generic_u8string().c_str();
        }

        String defaultDestination( const String &projectFile )
        {
            const auto projectRoot = fs::u8path( projectFile.c_str() ).parent_path();
            const auto builds = fs::u8path( projectRoot.filename().u8string() + " Builds" );
            return availableDestination( projectRoot.parent_path() / builds );
        }
    }

    GamePackageWindow::~GamePackageWindow()
    {
        unload( nullptr );
    }

    void GamePackageWindow::load( SmartPtr<ISharedObject> data )
    {
        EditorWindow::load( data );
        auto ui = core::IApplicationManager::instance()->getUI();
        auto window = ui->addElementByType<ui::IUIWindow>();
        window->setLabel( "Build Game" );
        window->setSize( { 740.0f, 235.0f } );
        auto placement = window->getProperties();
        placement->setProperty( "initialPlacement", String( "floating" ) );
        window->setProperties( placement );
        setParentWindow( window );
        m_listener = make_ptr<Listener>();
        m_listener->owner = this;

        auto hint = ui->addElementByType<ui::IUIText>();
        hint->setText( "Windows standalone game. Save your scenes and project before building." );
        window->addChild( hint );

        m_destination = ui->addElementByType<ui::IUITextEntry>();
        m_destination->setLabel( "Output folder" );
        m_destination->setPlaceholder( "Enter a new folder path" );
        window->addChild( m_destination );

        const auto button = [&]( const char *label, Action action, bool sameLine = false ) {
            auto control = ui->addElementByType<ui::IUIButton>();
            control->setLabel( label );
            control->setElementId( action );
            control->setSameLine( sameLine );
            control->addObjectListener( m_listener );
            window->addChild( control );
            return control;
        };
        m_browse = button( "Choose Parent Folder...", Browse );
        m_build = button( "Build", Build, true );
        m_cancel = button( "Cancel Build", Cancel, true );
        m_open = button( "Open Folder", OpenFolder, true );
        m_run = button( "Run Game", Run, true );

        auto status = ui->addElementByType<ui::IUITextEntry>();
        status->setLabel( "Build status" );
        status->setReadOnly( true );
        status->setMultiline( true );
        m_status = status;
        window->addChild( m_status );
        setLoadingState( LoadingState::Loaded );
        setWindowVisible( false );
    }

    void GamePackageWindow::unload( SmartPtr<ISharedObject> data )
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        if( m_job )
        {
            m_job->cancel();
        }
        if( m_worker.joinable() )
        {
            m_worker.join();
        }
        if( m_job )
        {
            m_job = nullptr;
        }
        if( m_listener )
        {
            m_listener->owner = nullptr;
            m_listener = nullptr;
        }
        if( auto window = getParentWindow() )
        {
            auto app = core::IApplicationManager::instancePtr();
            if( auto ui = app ? app->getUIPtr() : nullptr )
            {
                for( auto child : window->getChildren() )
                {
                    ui->removeElement( child );
                }
                ui->removeElement( window );
            }
            setParentWindow( nullptr );
        }
        m_destination = nullptr;
        m_status = nullptr;
        m_browse = m_build = m_cancel = m_open = m_run = nullptr;
        EditorWindow::unload( data );
    }

    void GamePackageWindow::setWindowVisible( bool visible )
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        if( visible )
        {
            refresh();
        }
        EditorWindow::setWindowVisible( visible );
    }

    void GamePackageWindow::update()
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        if( m_destination && isWindowVisible() )
        {
            refresh();
        }
    }

    void GamePackageWindow::refresh()
    {
        const bool busy = m_job && !m_job->isFinished();
        auto project = currentProject();
        const auto projectFile = project ? project->getFilePath() : String();
        if( !busy && projectFile != m_projectFile )
        {
            m_projectFile = projectFile;
            m_destination->setText( projectFile.empty() ? String() : defaultDestination( projectFile ) );
            m_job = nullptr;
            m_message.clear();
        }

        const bool canBuild = !busy && !m_projectFile.empty();
        const bool built = m_job && m_job->getStatus() == "succeeded";
        if( built && m_usedDefaultDestination )
        {
            m_destination->setText( defaultDestination( m_projectFile ) );
            m_usedDefaultDestination = false;
        }
        m_destination->setReadOnly( !canBuild );
        m_build->setEnabled( canBuild );
        m_browse->setEnabled( canBuild );
        m_cancel->setEnabled( busy && !m_cancelling );
        m_open->setEnabled( built );
        m_run->setEnabled( built );

        String message = m_message;
        if( message.empty() )
        {
            if( busy && m_cancelling )
                message = "Cancelling build...";
            else if( built )
                message = "Build complete\n" + m_job->getOutputPath();
            else if( m_job )
            {
                message = m_job->getDiagnostic();
                if( message.empty() && busy )
                    message = "Waiting for a worker thread";
                if( busy )
                    message += " (" + StringUtil::toString( m_job->getProgress() ) + "%)";
            }
            else if( canBuild )
                message = "Choose a new output folder and click Build.";
            else
                message = "Open a saved project to build a game.";
        }
        if( m_status->getText() != message )
        {
            m_status->setText( message );
        }
    }

    void GamePackageWindow::startBuild()
    {
        auto app = core::IApplicationManager::instance();
        if( m_worker.joinable() )
        {
            m_worker.join();
        }
        const auto mediaPath = app->getMediaPath();
        auto editorApp = dynamic_pointer_cast<EditorApplication>( app->getApplication() );
        const auto catalog = editorApp ? editorApp->getMediaCatalogPath( mediaPath ) : String();
        const auto destination = m_destination->getText();
        const bool useDefaultDestination = destination == defaultDestination( m_projectFile );
        auto job = make_ptr<JobCreatePackage>( m_projectFile, destination, mediaPath,
                                               app->getCachePath(), app->getSettingsPath(), catalog );
        job->setState( IJob::State::Queue );
        // Engine workers also run frame tasks whose barriers can wait for the
        // whole job. Keep file packaging independent of the frame scheduler.
        std::thread worker( [job]() mutable {
            Thread::setCurrentTask( TaskId::None );
            job->setState( IJob::State::Executing );
            job->execute();
            job->setState( IJob::State::Finish );
        } );
        m_job = job;
        m_worker = std::move( worker );
        m_cancelling = false;
        m_usedDefaultDestination = useDefaultDestination;
    }

    void GamePackageWindow::browse()
    {
        auto fileSystem = core::IApplicationManager::instance()->getFileSystem();
        auto dialog = fileSystem->openFileDialog();
        if( !dialog )
        {
            throw std::runtime_error( "The folder picker is unavailable." );
        }
        const auto parent = fs::u8path( m_destination->getText().c_str() ).parent_path();
        const auto projectRoot = fs::u8path( m_projectFile.c_str() ).parent_path();
        dialog->setDialogMode( INativeFileDialog::DialogMode::Select );
        dialog->setFilterMode( INativeFileDialog::FilterMode::FilterMode_Dirs );
        dialog->setFilePath( ( fs::is_directory( parent ) ? parent : projectRoot ).u8string().c_str() );
        const auto result = dialog->openDialog();
        if( result == INativeFileDialog::Result::Dialog_Okay )
        {
            const auto selected = fs::u8path( dialog->getFilePath().c_str() );
            m_destination->setText( availableDestination( selected ) );
        }
        else if( result == INativeFileDialog::Result::Dialog_Error )
        {
            throw std::runtime_error( "Could not open the folder picker." );
        }
    }

    void GamePackageWindow::handleAction( Action action )
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        if( !m_destination || !isWindowVisible() )
            return;

        refresh();
        m_message.clear();
        try
        {
            if( m_job && !m_job->isFinished() )
            {
                if( action == Cancel )
                {
                    m_job->cancel();
                    m_cancelling = true;
                }
            }
            else if( action == Browse && !m_projectFile.empty() )
            {
                browse();
            }
            else if( action == Build && !m_projectFile.empty() )
            {
                startBuild();
            }
#ifdef _WIN32
            else if( ( action == OpenFolder || action == Run ) && m_job &&
                     m_job->getStatus() == "succeeded" )
            {
                const auto folder = fs::u8path( m_job->getOutputPath().c_str() );
                const auto target = action == Run ? folder / "WPRuntime.exe" : folder;
                if( reinterpret_cast<INT_PTR>( ShellExecuteW( nullptr, L"open", target.c_str(),
                                                              nullptr, folder.c_str(), SW_SHOWNORMAL ) ) <= 32 )
                {
                    throw std::runtime_error( action == Run ? "Could not start WPRuntime.exe."
                                                            : "Could not open the output folder." );
                }
            }
#endif
        }
        catch( const std::exception &error )
        {
            m_message = error.what();
        }
        refresh();
    }

    Parameter GamePackageWindow::Listener::handleEvent( EventType, hash_type eventValue,
                                                       const Array<Parameter> &,
                                                       SmartPtr<ISharedObject> sender,
                                                       SmartPtr<ISharedObject>, SmartPtr<IEvent> )
    {
        if( eventValue == IEvent::handleSelection )
        {
            auto window = owner.load().lock();
            auto button = dynamic_pointer_cast<ui::IUIButton>( sender );
            if( window && button )
            {
                window->handleAction( static_cast<Action>( button->getElementId() ) );
            }
        }
        return {};
    }
}
