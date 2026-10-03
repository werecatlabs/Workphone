// ---------------------------------------------------------------------------
//  CopyDataWindow.cpp
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <ui/CopyDataWindow.hpp>
#include <Workphone/Workphone.hpp>

#include <sstream>

namespace workphone::adbeditor
{
    WP_CLASS_REGISTER_DERIVED( workphone::adbeditor, CopyDataWindow, ISharedObject );

    CopyDataWindow::CopyDataWindow() = default;
    CopyDataWindow::~CopyDataWindow() = default;

    void CopyDataWindow::setSourceConnection( SmartPtr<AssetDatabaseEditorDatabase> database )
    {
        m_source = database;
    }

    void CopyDataWindow::setDestinationConnection( SmartPtr<AssetDatabaseEditorDatabase> database )
    {
        m_destination = database;
    }

    void CopyDataWindow::setModelObjectType( const String &type )
    {
        m_modelObjectType = type;
    }

    void CopyDataWindow::show()
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
        m_window->setLabel( "Copy Data" );
        m_window->setSize( Vector2F( 600.f, 400.f ) );

        auto button = ui->addElementByType<ui::IUIButton>();
        button->setLabel( "Copy" );
        m_window->addChild( button );
        m_visible = true;
    }

    void CopyDataWindow::performCopy()
    {
        if( !m_source || !m_destination )
        {
            return;
        }

        // The C# code expects the user to pick the source/destination model
        // and model object through a separate dialog.  For the C++ port we
        // simply log the intent and provide the equivalent API call shape.
        WP_LOG_INFO( "CopyDataWindow: copying model object attributes" );
    }
}  // namespace workphone::adbeditor
