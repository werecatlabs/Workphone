// ---------------------------------------------------------------------------
//  AddComponentGroupWindow.cpp
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <ui/AddComponentGroupWindow.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::adbeditor
{
    WP_CLASS_REGISTER_DERIVED( workphone::adbeditor, AddComponentGroupWindow, ISharedObject );

    AddComponentGroupWindow::AddComponentGroupWindow() = default;
    AddComponentGroupWindow::~AddComponentGroupWindow() = default;

    void AddComponentGroupWindow::setDatabase( SmartPtr<AssetDatabaseEditorDatabase> database )
    {
        m_database = database;
    }

    void AddComponentGroupWindow::show()
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
        m_window->setLabel( "Add Component Group" );
        m_window->setSize( Vector2F( 360.f, 200.f ) );

        auto textEntry = ui->addElementByType<ui::IUITextEntry>();
        textEntry->setText( "Group Name" );
        m_window->addChild( textEntry );

        auto button = ui->addElementByType<ui::IUIButton>();
        button->setLabel( "Add" );
        m_window->addChild( button );
        m_visible = true;
    }
}  // namespace workphone::adbeditor
