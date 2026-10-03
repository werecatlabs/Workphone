// ---------------------------------------------------------------------------
//  HelpSupportWindow.cpp
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <ui/HelpSupportWindow.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::adbeditor
{
    WP_CLASS_REGISTER_DERIVED( workphone::adbeditor, HelpSupportWindow, ISharedObject );

    HelpSupportWindow::HelpSupportWindow() = default;
    HelpSupportWindow::~HelpSupportWindow() = default;

    void HelpSupportWindow::show()
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
        m_window->setLabel( "Help / Support" );
        m_window->setSize( Vector2F( 480.f, 320.f ) );

        m_text = ui->addElementByType<ui::IUIText>();
        m_text->setText(
            "Lioncat Asset Database Editor\n"
            "Bug reports:  support@lioncat.example\n"
            "Documentation: https://lioncat.example/docs" );
        m_window->addChild( m_text );
        m_visible = true;
    }
}  // namespace workphone::adbeditor
