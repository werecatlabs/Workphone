#ifndef _ChatListener_H
#define _ChatListener_H

#if FB_BUILD_WXWIDGETS
#include <wx/wx.hpp>
#include <FBCore/Interface/Memory/ISharedObject.hpp>
#include "FBCore/Interface/Net/INetworkManager.hpp"
#include "FBCore/Interface/Net/INetworkListener.hpp"

enum
{
    ID_GAME_MESSAGE_1 = 100,
    ID_GAME_MESSAGE_2
};

namespace fb
{
    class CNetworkManager;

    class ChatListener : public INetworkListener
    {
    public:
        ~ChatListener() override{};

        void handlePacket( SmartPtr<IPacket> packet ) override;
        virtual void onConnect( u16 playerId );

        virtual void onDisconnect( const u16 playerId ){};

        wxListBox *m_listChat;
        CNetworkManager *m_parent;

        void connect( u32 playerId ) override;

        void disconnect( u32 playerId ) override;
    };
}  // end namespace fb
#endif

#endif  // _ChatListener_H
