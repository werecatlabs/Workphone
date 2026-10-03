#include <EditorPCH.hpp>
#include <ui/FileViewWindow.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    FileViewWindow::FileViewWindow( SmartPtr<ui::IUIWindow> parent )
    {
        setParent( parent );
    }

    FileViewWindow::~FileViewWindow() = default;

    void FileViewWindow::load( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        auto parent = getParent();

        auto parentWindow = ui->addElementByType<ui::IUIWindow>();
        setParentWindow( parentWindow );

        if( parent )
        {
            parent->addChild( parentWindow );
        }

        m_text = ui->addElementByType<ui::IUIText>();
        WP_ASSERT( m_text );

        parentWindow->addChild( m_text );
    }

    void FileViewWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            if( m_text )
            {
                ui->removeElement( m_text );
                m_text = nullptr;
            }

            if( auto parentWindow = getParentWindow() )
            {
                ui->removeElement( parentWindow );
                setParentWindow( nullptr );
            }

            EditorWindow::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FileViewWindow::updateSelection()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto selectionManager = applicationManager->getSelectionManager();
        WP_ASSERT( selectionManager );

        auto selection = selectionManager->getSelection();
        if( !selection.empty() )
        {
            auto selected = selection.front();
            if( selected->isDerived<FileSelection>() )
            {
                auto fileSelection = workphone::static_pointer_cast<FileSelection>( selected );
                auto filePath = fileSelection->getFilePath();

                auto data = fileSystem->readAllText( filePath );

                m_text->setText( data );
            }
        }
    }
}  // namespace workphone::editor
