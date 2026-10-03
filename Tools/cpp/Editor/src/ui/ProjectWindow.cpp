#include <EditorPCH.hpp>
#include <ui/ProjectWindow.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <ui/FileWindow.hpp>
#include <ui/ProjectAssetsWindow.hpp>
#include <ui/ProjectTreeData.hpp>
#include <ui/UIManager.hpp>
#include <commands/DragDropActorCmd.hpp>
#include <commands/RemoveResourceCmd.hpp>
#include <commands/AddActorCmd.hpp>
#include <commands/RemoveSelectionCmd.hpp>
#include <commands/AddNewScriptCmd.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone, ProjectWindow, EditorWindow );

    ProjectWindow::ProjectWindow() = default;

    ProjectWindow::~ProjectWindow()
    {
        unload( nullptr );
    }

    void ProjectWindow::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto parentWindow = ui->addElementByType<ui::IUIWindow>();
            if( parentWindow )
            {
#if 0
                parentWindow->setLabel( "Project" );
                parentWindow->setHasBorder( true );
                setParentWindow( parentWindow );

                auto workspaceToolbar = ui->addElementByType<ui::IUIWindow>();
                workspaceToolbar->setLabel( "Project Toolbar | Search, Import, Build, Source Control" );
                workspaceToolbar->setHasBorder( true );
                parentWindow->addChild( workspaceToolbar );

                auto projectSearch = ui->addElementByType<ui::IUITextEntry>();
                projectSearch->setLabel( "Search Project" );
                projectSearch->setText( "" );
                workspaceToolbar->addChild( projectSearch );

                auto addProjectButton = [&]( const String &label, bool sameLine ) {
                    auto button = ui->addElementByType<ui::IUIButton>();
                    button->setLabel( label );
                    button->setSameLine( sameLine );
                    workspaceToolbar->addChild( button );
                    return button;
                };

                addProjectButton( "Refresh", false );
                addProjectButton( "Import", true );
                addProjectButton( "Reimport", true );
                addProjectButton( "Build", true );
                addProjectButton( "Save All", true );
                addProjectButton( "Settings", true );

                auto workspaceContent = ui->addElementByType<ui::IUIWindow>();
                workspaceContent->setLabel( "Project Workspace" );
                workspaceContent->setHasBorder( true );
                parentWindow->addChild( workspaceContent );

                m_projectAssetsWindow = workphone::make_ptr<ProjectAssetsWindow>();
                m_projectAssetsWindow->setParent( workspaceContent );
                m_projectAssetsWindow->load( nullptr );

                m_fileWindow = workphone::make_ptr<FileWindow>();
                m_fileWindow->setParent( workspaceContent );
                m_fileWindow->load( nullptr );
#else
                parentWindow->setLabel( "Project" );
                setParentWindow( parentWindow );

                m_projectAssetsWindow = workphone::make_ptr<ProjectAssetsWindow>();
                m_projectAssetsWindow->setParent( parentWindow );
                m_projectAssetsWindow->load( nullptr );

                m_fileWindow = workphone::make_ptr<FileWindow>();
                m_fileWindow->setParent( parentWindow );
                m_fileWindow->load( nullptr );
#endif
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ProjectWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            if( m_projectAssetsWindow )
            {
                m_projectAssetsWindow->unload( nullptr );
                m_projectAssetsWindow = nullptr;
            }

            if( m_fileWindow )
            {
                m_fileWindow->unload( nullptr );
                m_fileWindow = nullptr;
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

    void ProjectWindow::buildTree()
    {
        try
        {
            if( m_projectAssetsWindow )
            {
                m_projectAssetsWindow->build();
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    bool ProjectWindow::isValid() const
    {
        if( m_projectAssetsWindow )
        {
            return m_projectAssetsWindow->isValid();
        }

        return false;
    }
}  // namespace workphone::editor
