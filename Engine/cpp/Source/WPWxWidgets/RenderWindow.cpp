#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include "WPWxWidgets/RenderWindow.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace ui
    {

        //---------------------------------------------------------------------
        RenderWindow::RenderWindow( wxWindow *parent, wxWindowID id ) : wxScrolledWindow( parent, id )
        {
            try
            {
                auto baseSizer = new wxBoxSizer( wxHORIZONTAL );
                SetSizer( baseSizer );

                auto applicationManager = core::ApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();

                auto sceneManagerName = String( "ViewSM" );
                auto sceneManager = graphicsSystem->getGraphicsScene( sceneManagerName );
                WP_ASSERT( sceneManager );

                auto viewWindow = new ui::wxViewWindow( this, 0, 0, 400, 400 );

                m_viewWindow = viewWindow;
                baseSizer->Add( viewWindow, 1, wxEXPAND );

                m_window = viewWindow->getWindow();
                WP_ASSERT( m_window );

                if( m_window )
                {
                    m_camera = sceneManager->addCamera( "DefaultWindowCamera" );
                    WP_ASSERT( m_camera );

                    m_camera->setNearClipDistance( 0.0001f );
                    m_camera->setFarClipDistance( 10000.0f );

                    m_cameraNode = sceneManager->getRootSceneNode()->addChildSceneNode();
                    WP_ASSERT( m_cameraNode );

                    m_cameraNode->attachObject( m_camera );
                    //m_cameraNode->setPosition(Vector3F::UNIT_Z * 250.0f);

                    m_viewport = m_window->addViewport( 0, m_camera );
                    WP_ASSERT( m_viewport );

                    m_viewport->setBackgroundColour( ColourF( 0.02, 0.85, 0.75, 1.0 ) );

                    m_window->setAutoUpdated( false );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                wxMessageBox( e.what() );
            }
        }

        //---------------------------------------------------------------------
        RenderWindow::~RenderWindow()
        {
        }

        //---------------------------------------------------------------------
        void RenderWindow::initialiseRenderWindows()
        {
            try
            {
                auto applicationManager = core::ApplicationManager::instance();

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                auto sceneManager = graphicsSystem->getGraphicsScene( String( "ViewSM" ) );

                auto viewWindow = new ui::wxViewWindow( this, 0, 0, 400, 400 );
                m_viewWindow = viewWindow;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                wxMessageBox( e.what() );
            }
        }

        ui::wxViewWindow *RenderWindow::getWxWindow() const
        {
            return m_viewWindow;
        }

        SmartPtr<render::IWindow> RenderWindow::getWindow() const
        {
            return m_window;
        }

        void RenderWindow::setCameraNode( SmartPtr<render::ISceneNode> cameraNode )
        {
            m_cameraNode = cameraNode;
        }

        SmartPtr<render::ISceneNode> RenderWindow::getCameraNode() const
        {
            return m_cameraNode;
        }

        void RenderWindow::setCamera( SmartPtr<render::ICamera> camera )
        {
            m_camera = camera;
        }

        SmartPtr<render::ICamera> RenderWindow::getCamera() const
        {
            return m_camera;
        }

        void RenderWindow::setWindow( SmartPtr<render::IWindow> window )
        {
            m_window = window;
        }

    }  // namespace ui
}  // namespace workphone
