#include "ChatDlg.hpp"
#include "ChatListener.hpp"
#include "FBRaknet/FBRakNetMessageIdentifiers.hpp"

#if FB_BUILD_WXWIDGETS
//#define ID_LIST_EVENTS wxID_HIGHEST + 1
//#define ID_BTN_SEND_CHAT_TEXT ID_LIST_EVENTS + 1

enum
{
    ID_LIST_EVENTS = wxID_HIGHEST,
    ID_BTN_SEND_CHAT_TEXT,
    ID_BTN_INCREMENT,
    ID_EDIT_CHAT
};

ChatDlg::ChatDlg( const wxString &title ) :
    wxDialog( nullptr, -1, title, wxDefaultPosition, wxSize( 250, 230 ),
              wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    auto baseSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( baseSizer );

    auto chatDataBox = new wxBoxSizer( wxVERTICAL );
    baseSizer->Add( chatDataBox, 1, wxEXPAND );

    auto chatBox = new wxBoxSizer( wxHORIZONTAL );
    chatDataBox->Add( chatBox, 0, wxEXPAND );
    m_editChat = new wxTextCtrl( this, ID_EDIT_CHAT );
    chatBox->Add( m_editChat, 1, wxEXPAND );
    chatBox->Add( new wxButton( this, ID_BTN_SEND_CHAT_TEXT, "Send" ) );

    auto listBoxSiz = new wxBoxSizer( wxVERTICAL );
    baseSizer->Add( listBoxSiz, 1, wxEXPAND );
    m_listChat = new wxListBox( this, ID_LIST_EVENTS );
    listBoxSiz->Add( m_listChat, 1, wxEXPAND );

    auto incrementBoxSiz = new wxBoxSizer( wxVERTICAL );
    baseSizer->Add( incrementBoxSiz, 1, wxEXPAND );
    m_btnPush = new wxButton( this, ID_BTN_INCREMENT, "Push(0)" );
    incrementBoxSiz->Add( m_btnPush, 1, wxEXPAND );

    Connect( ID_BTN_SEND_CHAT_TEXT, wxEVT_COMMAND_BUTTON_CLICKED,
             wxCommandEventHandler( ChatDlg::onSendChatText ) );
    Connect( ID_BTN_INCREMENT, wxEVT_COMMAND_BUTTON_CLICKED,
             wxCommandEventHandler( ChatDlg::onIncrement ) );

    Connect( wxEVT_KEY_UP, wxKeyEventHandler( ChatDlg::onKeyDown ) );

    bool isPeer1 = false;

    auto dial = new wxMessageDialog( nullptr, wxT( "Start as Peer1?" ), wxT( "Network mode" ),
                                     wxYES_NO | wxICON_QUESTION );
    if( dial->ShowModal() == wxID_YES )
    {
        isPeer1 = true;

        m_netManager.setPort( 60000 );
        this->SetTitle( "Chat (Raknet) - Peer1" );

        stationName = "Peer1";
    }
    else
    {
        m_netManager.setPort( 60001 );
        this->SetTitle( "Chat (Raknet) - Peer2" );

        stationName = "Peer2";
    }

    SmartPtr<ChatListener> listener( new ChatListener() );
    m_netManager.setListener( listener );
    listener->m_listChat = m_listChat;
    listener->m_parent = &m_netManager;

    m_netManager.setPeer();

    // make a connection between peer1 and peer2
    if( !isPeer1 )
        m_netManager.connect( "127.0.0.1", 60000, "" );

    Centre();
    //ShowModal();

    //Destroy();
}

void ChatDlg::onSendChatText( wxCommandEvent &event )
{
    m_editChat->SetFocus();

    auto text = String( m_editChat->GetLineText( 0 ) );
    if( text == "" )
        return;
    // add text in list
    text = stationName + ": " + text;
    m_listChat->Append( text.c_str() );
    m_editChat->Clear();

    // send packet
    SmartPtr<CPacket> packet = m_netManager.createPacket();
    packet->write( static_cast<unsigned char>( ID_GAME_MESSAGE_1 ) );
    packet->write( text );
    //m_netManager.sendPacket((SmartPtr<IPacket>)packet);
}

void ChatDlg::OnProgressTimer()
{
    m_netManager.update();
}

void ChatDlg::onIncrement( wxCommandEvent &event )
{
    static int value = 0;
    value++;
    if( value > 5 )
        value = 1;

    wxString text;
    text = wxString::Format( "Push(%d)", value );
    m_btnPush->SetLabelText( text );

    // send packet
    SmartPtr<CPacket> packet = m_netManager.createPacket();
    packet->write( static_cast<unsigned char>( ID_GAME_MESSAGE_2 ) );
    packet->write( value );
    //m_netManager.sendPacket((SmartPtr<IPacket>)packet);
}

void ChatDlg::onKeyDown( wxKeyEvent &event )
{
    int keycode = event.GetKeyCode();

    if( keycode == WXK_RETURN )
    {
        wxCommandEvent event;
        onSendChatText( event );
    }
}

#endif
