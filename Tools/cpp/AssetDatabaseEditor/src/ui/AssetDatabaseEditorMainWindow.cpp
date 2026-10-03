// ---------------------------------------------------------------------------
//  AssetDatabaseEditorMainWindow.cpp
//
//  Standalone top-level window for the C++ asset database editor.
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <ui/AssetDatabaseEditorMainWindow.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::adbeditor
{
    WP_CLASS_REGISTER_DERIVED( workphone::adbeditor, AssetDatabaseEditorMainWindow,
                               ISharedObject );

    AssetDatabaseEditorMainWindow::AssetDatabaseEditorMainWindow() = default;

    AssetDatabaseEditorMainWindow::~AssetDatabaseEditorMainWindow() = default;

    void AssetDatabaseEditorMainWindow::load( SmartPtr<ISharedObject> data )
    {
        m_database = workphone::make_ptr<AssetDatabaseEditorDatabase>();
        m_settings = workphone::make_ptr<AssetDatabaseEditorSettings>();

        createRoot();
        createTabs();
        populateModels();
    }

    void AssetDatabaseEditorMainWindow::unload( SmartPtr<ISharedObject> data )
    {
        if( m_rootWindow )
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( applicationManager )
            {
                if( auto ui = applicationManager->getUI() )
                {
                    ui->removeElement( m_rootWindow );
                }
            }
            m_rootWindow = nullptr;
        }
        if( m_database )
        {
            m_database->close();
            m_database = nullptr;
        }
        m_settings = nullptr;
    }

    void AssetDatabaseEditorMainWindow::createRoot()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        m_rootWindow = ui->addElementByType<ui::IUIWindow>();
        WP_ASSERT( m_rootWindow );
        m_rootWindow->setLabel( "Asset Database Editor" );
        m_rootWindow->setSize( Vector2F( 1024.f, 720.f ) );

    }

    void AssetDatabaseEditorMainWindow::createTabs()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        m_tabBar = ui->addElementByType<ui::IUITabBar>();
        m_rootWindow->addChild( m_tabBar );

        auto modelsTab = m_tabBar->addTabItem();
        modelsTab->setLabel( "Models" );
        m_modelsDropdown = ui->addElementByType<ui::IUIDropdown>();
        m_modelsDropdown->setLabel( "Configured models" );
        modelsTab->addChild( m_modelsDropdown );
        m_modelAttribsGrid = ui->addElementByType<ui::IUIDataGrid>();
        modelsTab->addChild( m_modelAttribsGrid );

        auto componentsTab = m_tabBar->addTabItem();
        componentsTab->setLabel( "Components" );
        m_componentsGrid = ui->addElementByType<ui::IUIDataGrid>();
        componentsTab->addChild( m_componentsGrid );

        auto groupsTab = m_tabBar->addTabItem();
        groupsTab->setLabel( "Component Groups" );
        m_componentGroupsGrid = ui->addElementByType<ui::IUIDataGrid>();
        groupsTab->addChild( m_componentGroupsGrid );
    }

    void AssetDatabaseEditorMainWindow::populateModels()
    {
        if( !m_database || !m_database->isOpen() || !m_modelsDropdown )
        {
            return;
        }

        Array<String> modelNames;
        for( const auto &row : m_database->getModels() )
        {
            modelNames.push_back( row.getFieldValue( "name" ) );
        }
        m_modelsDropdown->setOptions( modelNames );
    }
}  // namespace workphone::adbeditor
