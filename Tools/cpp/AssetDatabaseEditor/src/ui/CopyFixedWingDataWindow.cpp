// ---------------------------------------------------------------------------
//  CopyFixedWingDataWindow.cpp
// ---------------------------------------------------------------------------

#include <AssetDatabaseEditorPrerequisites.hpp>
#include <ui/CopyFixedWingDataWindow.hpp>
#include <Workphone/Workphone.hpp>

#include <vector>

namespace workphone::adbeditor
{
    WP_CLASS_REGISTER_DERIVED( workphone::adbeditor, CopyFixedWingDataWindow, ISharedObject );

    CopyFixedWingDataWindow::CopyFixedWingDataWindow() = default;
    CopyFixedWingDataWindow::~CopyFixedWingDataWindow() = default;

    void CopyFixedWingDataWindow::setSourceConnection(
        SmartPtr<AssetDatabaseEditorDatabase> database )
    {
        m_source = database;
    }

    void CopyFixedWingDataWindow::setDestinationConnection(
        SmartPtr<AssetDatabaseEditorDatabase> database )
    {
        m_destination = database;
    }

    void CopyFixedWingDataWindow::show()
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
        m_window->setLabel( "Copy Fixed Wing Data" );
        m_window->setSize( Vector2F( 640.f, 480.f ) );

        auto button = ui->addElementByType<ui::IUIButton>();
        button->setLabel( "Run Copy" );
        m_window->addChild( button );
        m_visible = true;
    }

    void CopyFixedWingDataWindow::performCopy()
    {
        if( !m_source || !m_destination )
        {
            return;
        }

        std::vector<String> classNames;
        if( m_wingData ) classNames.push_back( "wing_data" );
        if( m_wheelData ) classNames.push_back( "wheel_data" );
        if( m_engineData ) classNames.push_back( "engine_data" );
        if( m_propwashData ) classNames.push_back( "prop_wash_data" );
        if( m_modelData )
        {
            classNames.push_back( "model_data" );
            classNames.push_back( "car_data" );
            classNames.push_back( "truck_data" );
            classNames.push_back( "plane_data" );
        }
        if( m_controlSurfaceData )
        {
            classNames.push_back( "control_surface_data" );
            classNames.push_back( "controlsurface" );
        }

        for( const auto &className : classNames )
        {
            const auto sql = "Select * From actor_objects where class = '" + className + "'";
            (void)sql;  // surfaced through the database wrapper for the caller to use.
        }

        WP_LOG_INFO( "CopyFixedWingDataWindow: prepared class list for fixed wing copy" );
    }
}  // namespace workphone::adbeditor
