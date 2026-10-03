// ---------------------------------------------------------------------------
//  ModelBrowserWindow.cpp
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <ui\ModelBrowserWindow.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::adbeditor
{
    WP_CLASS_REGISTER_DERIVED( workphone::adbeditor, ModelBrowserWindow, ISharedObject );

    ModelBrowserWindow::ModelBrowserWindow() = default;
    ModelBrowserWindow::~ModelBrowserWindow() = default;

    void ModelBrowserWindow::setDatabase( SmartPtr<AssetDatabaseEditorDatabase> database )
    {
        m_database = database;
    }

    void ModelBrowserWindow::setModelObjectType( const String &type )
    {
        m_modelObjectType = type;
    }

    void ModelBrowserWindow::show()
    {
        if( m_visible )
        {
            return;
        }
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );
        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        m_window = ui->addElementByType<ui::IUIWindow>();
        m_window->setLabel( "Model Browser" );
        m_window->setSize( Vector2F( 480.f, 320.f ) );

        m_modelDropdown = ui->addElementByType<ui::IUIDropdown>();
        m_modelDropdown->setLabel( "Models" );
        m_window->addChild( m_modelDropdown );

        if( m_database )
        {
            const auto models = m_database->getModels();
            Array<String> options;
            for( const auto &row : models )
            {
                options.push_back( row.getFieldValue( "name" ) );
            }
            m_modelDropdown->setOptions( options );
        }

        m_visible = true;
    }
}  // namespace workphone::adbeditor
