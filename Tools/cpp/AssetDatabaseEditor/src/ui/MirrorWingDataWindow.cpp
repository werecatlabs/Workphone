// ---------------------------------------------------------------------------
//  MirrorWingDataWindow.cpp
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <ui/MirrorWingDataWindow.hpp>
#include <Workphone/Workphone.hpp>

#include <vector>

namespace workphone::adbeditor
{
    WP_CLASS_REGISTER_DERIVED( workphone::adbeditor, MirrorWingDataWindow, ISharedObject );

    MirrorWingDataWindow::MirrorWingDataWindow() = default;
    MirrorWingDataWindow::~MirrorWingDataWindow() = default;

    void MirrorWingDataWindow::setDatabase( SmartPtr<AssetDatabaseEditorDatabase> database )
    {
        m_database = database;
    }

    void MirrorWingDataWindow::show()
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
        m_window->setLabel( "Mirror Wing Data" );
        m_window->setSize( Vector2F( 480.f, 320.f ) );

        auto button = ui->addElementByType<ui::IUIButton>();
        button->setLabel( "Mirror X" );
        m_window->addChild( button );
        m_visible = true;
    }
}  // namespace workphone::adbeditor
