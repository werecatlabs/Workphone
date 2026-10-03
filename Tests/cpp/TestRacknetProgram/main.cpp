#include "main.hpp"

#if FB_BUILD_WXWIDGETS
#define ID_TIMER 100

BEGIN_EVENT_TABLE( MyApp, wxApp )
EVT_TIMER( ID_TIMER, MyApp::OnProgressTimer )
END_EVENT_TABLE()

bool MyApp::OnInit()
{
    static const int INTERVAL = 300;  // milliseconds
    m_timer = new wxTimer( this, ID_TIMER );
    m_timer->Start( INTERVAL );

    m_chat = new ChatDlg( wxT( "ChatDlg" ) );
    m_chat->ShowModal();
    m_timer->Stop();
    m_chat->Destroy();

    return true;
}

void MyApp::OnProgressTimer( wxTimerEvent &event )
{
    if( m_chat )
        m_chat->OnProgressTimer();
}
#endif
