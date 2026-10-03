// ---------------------------------------------------------------------------
//  ModelObjectWindow.cpp
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <ui\ModelObjectWindow.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::adbeditor
{
    WP_CLASS_REGISTER_DERIVED( workphone::adbeditor, ModelObjectWindow, ISharedObject );

    ModelObjectWindow::ModelObjectWindow() = default;
    ModelObjectWindow::~ModelObjectWindow() = default;

    void ModelObjectWindow::setDatabase( SmartPtr<AssetDatabaseEditorDatabase> database )
    {
        m_database = database;
    }

    void ModelObjectWindow::setModelObjectType( const String &type )
    {
        m_modelObjectType = type;
    }

    void ModelObjectWindow::show()
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
        m_window->setLabel( "Model Object" );
        m_window->setSize( Vector2F( 520.f, 400.f ) );

        m_modelDropdown = ui->addElementByType<ui::IUIDropdown>();
        m_modelDropdown->setLabel( "Models" );
        m_window->addChild( m_modelDropdown );

        m_modelObjectDropdown = ui->addElementByType<ui::IUIDropdown>();
        m_modelObjectDropdown->setLabel( "Model Objects" );
        m_window->addChild( m_modelObjectDropdown );

        auto button = ui->addElementByType<ui::IUIButton>();
        button->setLabel( "Apply" );
        m_window->addChild( button );

        populateModels();
        m_visible = true;
    }

    void ModelObjectWindow::populateModels()
    {
        if( !m_database || !m_modelDropdown )
        {
            return;
        }
        const auto models = m_database->getModels();
        Array<String> options;
        for( const auto &row : models )
        {
            options.push_back( row.getFieldValue( "name" ) );
        }
        m_modelDropdown->setOptions( options );
    }
}  // namespace workphone::adbeditor
