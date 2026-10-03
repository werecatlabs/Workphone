#include <WPNetwork/WPNetworkListener.hpp>
#include <Workphone/Interface/Net/IPacket.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WPNetworkListener, INetworkListener );

    WPNetworkListener::WPNetworkListener() = default;
    WPNetworkListener::~WPNetworkListener() = default;

    void WPNetworkListener::handlePacket( SmartPtr<IPacket> packet )
    {
        (void)packet;
    }

    void WPNetworkListener::connect( u32 playerId )
    {
        (void)playerId;
    }

    void WPNetworkListener::disconnect( u32 playerId )
    {
        (void)playerId;
    }
}  // namespace workphone
