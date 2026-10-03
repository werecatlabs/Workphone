#ifndef ChatDlg_h__
#define ChatDlg_h__

#if FB_BUILD_WXWIDGETS
#include <wx/wx.hpp>
#include "FBRakNet/CNetworkManager.hpp"
#include "FBRakNet/CPacket.hpp"

using namespace fb;

class wxTimer;

class ChatDlg : public wxDialog
{
public:
    ChatDlg( const wxString &title );

    void onSendChatText( wxCommandEvent &event );
    void onIncrement( wxCommandEvent &event );
    void onKeyDown( wxKeyEvent &event );
    void OnProgressTimer();

protected:
    wxTextCtrl *m_editChat;
    wxListBox *m_listChat;
    wxButton *m_btnPush;

    String stationName;

    CNetworkManager m_netManager;
};
#endif

#endif  // ChatDlg_h__
