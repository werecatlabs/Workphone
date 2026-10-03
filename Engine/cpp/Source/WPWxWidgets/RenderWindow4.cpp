#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include "WPWxWidgets/RenderWindow4.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace ui
    {

        //---------------------------------------------------------------------
        RenderWindow4::RenderWindow4( wxWindow *parent, wxWindowID id ) : wxScrolledWindow( parent, id )
        {
            try
            {
                auto baseSizer = new wxBoxSizer( wxHORIZONTAL );
                SetSizer( baseSizer );

                //// Create child splitter
                //mSplitter = new wxFourWaySplitter(this,
                //	-1, wxDefaultPosition, wxDefaultSize, wxSP_3D);
                //mSplitter->setBackgroundColour(wxColour(128, 128, 128));
                //mSplitter->Show();

                //baseSizer->Add(mSplitter, 1, wxEXPAND);

                //initialiseRenderWindows();

                auto applicationManager = core::ApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();

                auto sceneManagerName = String( "ViewSM" );
                auto sceneManager = graphicsSystem->getGraphicsScene( sceneManagerName );
                WP_ASSERT( sceneManager );

                auto viewWindow = new ui::wxViewWindow( this, 0, 0, 400, 400 );

                mViewWindows[0] = viewWindow;
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
        RenderWindow4::~RenderWindow4()
        {
        }

        //---------------------------------------------------------------------
        void RenderWindow4::initialiseRenderWindows()
        {
            try
            {
                auto applicationManager = core::ApplicationManager::instance();

                /*OgreMVC::ViewOgreWindow* mOgrePerspectiveWindow = new OgreMVC::ViewOgreWindow(m_viewPortWindow);
                mOgrePerspectiveWindow->initialise();
                mOgrePerspectiveWindow->setViewType(OgreMVC::ViewOgreWindow::PERSPECTIVE);
                mOgrePerspectiveWindow->setViewDetail(OgreMVC::ViewOgreWindow::SHADED);
                m_mgr.Update();
                return;*/

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                auto sceneManager = graphicsSystem->getGraphicsScene( String( "ViewSM" ) );

                for( int i = 0; i < 4; ++i )
                {
                    auto viewWindow = new ui::wxViewWindow( mSplitter, 0, 0, 400, 400 );

                    mSplitter->SetWindow( i, viewWindow );
                    mViewWindows[i] = viewWindow;

                    /*switch(i)
                    {
                    case 0:
                    mViewWindows[i]->setViewType(OgreMVC::ViewOgreWindow::FRONT);
                    mViewWindows[i]->setViewDetail(OgreMVC::ViewOgreWindow::WIREFRAME);
                    break;
                    case 1:
                    mViewWindows[i]->setViewType(OgreMVC::ViewOgreWindow::RIGHT);
                    mViewWindows[i]->setViewDetail(OgreMVC::ViewOgreWindow::WIREFRAME);
                    break;
                    case 2:
                    mViewWindows[i]->setViewType(OgreMVC::ViewOgreWindow::TOP);
                    mViewWindows[i]->setViewDetail(OgreMVC::ViewOgreWindow::WIREFRAME);
                    break;
                    case 3:
                    //mSplitter->setExpanded(i);
                    mViewWindows[i]->setViewType(OgreMVC::ViewOgreWindow::PERSPECTIVE);
                    mViewWindows[i]->setViewDetail(OgreMVC::ViewOgreWindow::SHADED);
                    break;
                    default:
                    break;
                    };*/
                }

                //m_mgr.Update();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
                wxMessageBox( e.what() );
            }
        }

        ui::wxViewWindow *RenderWindow4::getWindow( u32 index ) const
        {
            return mViewWindows[index];
        }

        SmartPtr<render::IWindow> RenderWindow4::getWindow() const
        {
            return m_window;
        }

        void RenderWindow4::setCameraNode( SmartPtr<render::ISceneNode> cameraNode )
        {
            m_cameraNode = cameraNode;
        }

        SmartPtr<render::ISceneNode> RenderWindow4::getCameraNode() const
        {
            return m_cameraNode;
        }

        void RenderWindow4::setCamera( SmartPtr<render::ICamera> camera )
        {
            m_camera = camera;
        }

        SmartPtr<render::ICamera> RenderWindow4::getCamera() const
        {
            return m_camera;
        }

        void RenderWindow4::setWindow( SmartPtr<render::IWindow> window )
        {
            m_window = window;
        }

    }  // namespace ui
}  // namespace workphone
