#include <WPWxWidgets/WPWxWidgetsPCH.hpp>

#include "WPWxWidgets/WPWxViewWindow.hpp"
#include <Workphone/Workphone.hpp>

#include "WPWxWidgets/WPWxUtil.hpp"

#include "WPWxWidgets/WPWxInputEvent.hpp"
#include "WPWxWidgets/WPWxKeyboardState.hpp"
#include "WPWxWidgets/WPWxMouseState.hpp"
#include <wx/window.hpp>

BEGIN_EVENT_TABLE( fb::ui::wxViewWindow, wxWindow )
EVT_MOVE_END( wxViewWindow::OnMove )
EVT_SIZE( wxViewWindow::OnSize )
EVT_PAINT( wxViewWindow::OnPaint )
EVT_MOUSE_EVENTS( wxViewWindow::OnMouse )
EVT_KEY_DOWN( wxViewWindow::OnKey )
EVT_KEY_UP( wxViewWindow::OnKey )
END_EVENT_TABLE()

namespace workphone
{
    namespace ui
    {

        //--------------------------------------------
        wxViewWindow::wxViewWindow( wxWindow *parent, int xPosition, int yPosition, int width,
                                    int height ) :
            wxWindow( parent, -1 )
        {
            //createToolbar();
            //createPopupMenu();

            setupRenderWindow();
        }

        //--------------------------------------------
        void wxViewWindow::setupRenderWindow()
        {
            static u32 nameExt = 0;
            String name = String( "Window" ) + StringUtil::toString( nameExt++ );

            auto pHandle = GetHandle();
            auto uiHandle = (size_t)( pHandle );
            auto handle = std::to_string( uiHandle );

            SmartPtr<Properties> properties( new Properties );
            properties->setProperty( "WindowHandle", handle );

            auto window = WxUtil::createRenderWindow( this, name, 400, 400, properties );
            window->setAutoUpdated( false );
            m_window = window;
        }

        //--------------------------------------------
        wxViewWindow::~wxViewWindow()
        {
        }

        //--------------------------------------------
        void wxViewWindow::OnMove( wxMoveEvent &event )
        {
        }

        //--------------------------------------------
        void wxViewWindow::OnSize( wxSizeEvent &event )
        {
            if( m_window )
            {
                auto size = event.GetSize();
                auto width = size.GetWidth();
                auto height = size.GetHeight();

                if( width > 100 && height > 100 )
                {
                    m_window->resize( width, height );
                }
            }
        }

        //--------------------------------------------
        void wxViewWindow::OnPaint( wxPaintEvent &event )
        {
            //wxPaintDC dc(this);

            //if ( m_window )
            //	m_window->update();
        }

        //--------------------------------------------
        void wxViewWindow::OnMouse( wxMouseEvent &event )
        {
            try
            {
                SmartPtr<IInputEvent> sevt( new WxInputEvent );
                sevt->setEventType( IInputEvent::EventType::Mouse );

                SmartPtr<IMouseState> mouseState;  //( new WxMouseState );
                sevt->setMouseState( mouseState );

                if( event.IsButton() )
                {
                    if( event.LeftDown() )
                    {
                        mouseState->setEventType( IMouseState::Event::LeftPressed );
                        mouseState->setButtonPressed( IMouseState::MOUSE_LEFT_BUTTON, true );
                    }
                    else if( event.LeftUp() )
                    {
                        mouseState->setEventType( IMouseState::Event::LeftReleased );
                        mouseState->setButtonPressed( IMouseState::MOUSE_LEFT_BUTTON, false );
                    }
                    else if( event.MiddleDown() )
                    {
                        mouseState->setEventType( IMouseState::Event::MiddlePressed );
                        mouseState->setButtonPressed( IMouseState::MOUSE_MIDDLE_BUTTON, true );
                    }
                    else if( event.MiddleUp() )
                    {
                        mouseState->setEventType( IMouseState::Event::MiddleReleased );
                        mouseState->setButtonPressed( IMouseState::MOUSE_MIDDLE_BUTTON, false );
                    }
                    else if( event.RightDown() )
                    {
                        mouseState->setEventType( IMouseState::Event::RightPressed );
                        mouseState->setButtonPressed( IMouseState::MOUSE_RIGHT_BUTTON, true );
                    }
                    else if( event.RightUp() )
                    {
                        mouseState->setEventType( IMouseState::Event::RightReleased );
                        mouseState->setButtonPressed( IMouseState::MOUSE_RIGHT_BUTTON, false );
                    }
                }
                else if( event.GetWheelRotation() != 0 )
                {
                    auto wheelDelta = Vector2F( 0, (float)event.GetWheelRotation() );
                    mouseState->setWheelDelta( wheelDelta );

                    /*if(  sevt.MouseInput.Wheel > 0 )
                    sevt.MouseInput.Wheel = min_( 1.f, sevt.MouseInput.Wheel );
                    else
                    sevt.MouseInput.Wheel = max_( -1.f, sevt.MouseInput.Wheel );*/

                    mouseState->setEventType( IMouseState::Event::Wheel );
                }
                // motion events
                else if( event.Moving() || event.Dragging() )
                {
                    mouseState->setEventType( IMouseState::Event::Moved );
                }

                //if (event.GetWheelRotation() != 0)
                //   {
                //       // TODO (mandrav#1#): Verify that this block works as expected
                //       // things of interest:
                //       //      event.GetWheelDelta()
                //       sevt.MouseInput.Wheel = (float)event.GetWheelRotation();

                //	if(  sevt.MouseInput.Wheel > 0 )
                //		sevt.MouseInput.Wheel = min_( 1.f, sevt.MouseInput.Wheel );
                //	else
                //		sevt.MouseInput.Wheel = max_( -1.f, sevt.MouseInput.Wheel );

                //       sevt.MouseInput.Event = fb::EMIE_MOUSE_WHEEL;
                //   }

                // setup the XY values
                Vector2F currentMousePosition( event.GetX(), event.GetY() );
                Vector2F relativePosition = m_prevMousePosition - currentMousePosition;
                mouseState->setRelativePosition( relativePosition );
                mouseState->setAbsolutePosition( Vector2F( event.GetX(), event.GetY() ) );
                m_prevMousePosition = Vector2F( event.GetX(), event.GetY() );

                //if the mouse position is outside the view port change focus
                if( event.GetX() < 0 || event.GetY() < 0 || event.GetX() > GetSize().GetWidth() ||
                    event.GetY() > GetSize().GetHeight() )
                {
                    wxWindow *parent = GetParent();
                    if( parent )
                        parent->SetFocus();
                }

                if( event.IsButton() )
                {
                    SetFocus();
                }

                sevt->setUserData( m_camera.get() );
                sevt->setWindow( m_window.get() );
                sevt->setUserData( this );

                auto applicationManager = core::ApplicationManager::instance();
                auto inputManager = applicationManager->getInputDeviceManager();
                if( inputManager )
                {
                    if( inputManager->postEvent( sevt ) )
                    {
                        event.Skip();
                    }
                }

                // if the rendering timer isn't running, update the render window now
                //if (!m_Timer.IsRunning())
                //    OnRender();
                event.Skip();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        //--------------------------------------------
        void wxViewWindow::OnKey( wxKeyEvent &event )
        {
            try
            {
                SmartPtr<IInputEvent> sevt( new WxInputEvent );
                sevt->setEventType( IInputEvent::EventType::Key );

                SmartPtr<IKeyboardState> keyboardState( new WxKeyboardState );
                sevt->setKeyboardState( keyboardState );

                keyboardState->setKeyCode( event.GetRawKeyCode() );
                keyboardState->setRawKeyCode( event.GetRawKeyCode() );
                keyboardState->setPressedDown( event.GetEventType() == wxEVT_KEY_DOWN );
                keyboardState->setShiftPressed( event.ShiftDown() );
                keyboardState->setControlPressed( event.CmdDown() );

#if wxUSE_UNICODE
                keyboardState->setChar( event.GetUnicodeKey() );
#else
                keyboardState->setChar( event.GetKeyCode() );
#endif

                sevt->setUserData( m_camera.get() );
                sevt->setWindow( m_window.get() );
                sevt->setUserData( this );

                auto applicationManager = core::ApplicationManager::instance();
                auto inputMgr = applicationManager->getInputDeviceManager();
                if( inputMgr->postEvent( sevt ) )
                {
                    event.Skip();
                }

                // if the rendering timer isn't running, update the render window now
                //if (!m_Timer.IsRunning())
                //    OnRender();
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        //--------------------------------------------
        SmartPtr<render::ICamera> wxViewWindow::getCamera() const
        {
            return m_camera;
        }

        //--------------------------------------------
        void wxViewWindow::setCamera( SmartPtr<render::ICamera> camera )
        {
            m_camera = camera;

            if( m_camera )
            {
                if( !m_viewport )
                {
                    m_viewport = m_window->addViewport( 0, m_camera );
                }
                else
                {
                    m_viewport->setCamera( m_camera );
                }
            }
            else
            {
                m_viewport->setCamera( nullptr );
            }
        }

        //--------------------------------------------
        SmartPtr<render::ISceneNode> wxViewWindow::getCameraNode() const
        {
            return m_cameraNode;
        }

        //--------------------------------------------
        void wxViewWindow::setCameraNode( SmartPtr<render::ISceneNode> cameraNode )
        {
            m_cameraNode = cameraNode;
        }

        //--------------------------------------------
        SmartPtr<render::IViewport> wxViewWindow::getViewport() const
        {
            return m_viewport;
        }

        //--------------------------------------------
        void wxViewWindow::setViewport( SmartPtr<render::IViewport> viewport )
        {
            m_viewport = viewport;
        }

        //--------------------------------------------
        SmartPtr<render::IWindow> wxViewWindow::getWindow() const
        {
            return m_window;
        }

        //--------------------------------------------
        void wxViewWindow::setWindow( SmartPtr<render::IWindow> window )
        {
            m_window = window;
        }

    }  // namespace ui
}  // namespace workphone
