#ifndef WPNetworkListener_h__
#define WPNetworkListener_h__

#include <WPNetwork/WPNetworkPrerequisites.hpp>
#include <Workphone/Interface/Net/INetworkListener.hpp>

namespace workphone
{
    class WPNetwork_API WPNetworkListener : public INetworkListener
    {
    public:
        WPNetworkListener();
        ~WPNetworkListener() override;

        void handlePacket( SmartPtr<IPacket> packet ) override;
        void connect( u32 playerId ) override;
        void disconnect( u32 playerId ) override;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // WPNetworkListener_h__
