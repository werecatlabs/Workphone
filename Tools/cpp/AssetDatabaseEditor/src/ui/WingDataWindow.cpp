// ---------------------------------------------------------------------------
//  WingDataWindow.cpp
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <ui/WingDataWindow.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::adbeditor
{
    WP_CLASS_REGISTER_DERIVED( workphone::adbeditor, WingDataWindow, ISharedObject );

    WingDataWindow::WingDataWindow() = default;
    WingDataWindow::~WingDataWindow() = default;

    void WingDataWindow::setDatabase( SmartPtr<AssetDatabaseEditorDatabase> database )
    {
        m_database = database;
    }

    void WingDataWindow::show()
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
        m_window->setLabel( "Wing Data" );
        m_window->setSize( Vector2F( 480.f, 320.f ) );

        m_statusText = ui->addElementByType<ui::IUIText>();
        m_statusText->setText( "Wing data editor stub - select a wing to edit" );
        m_window->addChild( m_statusText );

        m_visible = true;
    }
}  // namespace workphone::adbeditor
