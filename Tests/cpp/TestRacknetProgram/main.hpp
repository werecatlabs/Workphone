#ifndef main_h__
#define main_h__

#if FB_BUILD_WXWIDGETS
#include <wx/wx.hpp>
#include <wx/timer.hpp>

#include "ChatDlg.hpp"

class MyApp : public wxApp
{
public:
    bool OnInit() override;
    void OnProgressTimer( wxTimerEvent &event );

    wxTimer *m_timer;
    ChatDlg *m_chat;

    DECLARE_EVENT_TABLE()
};

IMPLEMENT_APP( MyApp )
#endif

#endif  // main_h__
