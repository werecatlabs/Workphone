#pragma once
#include <WPNetwork/WPNetworkPrerequisites.hpp>
#include <Workphone/Interface/Net/INetworkManager.hpp>
#include <memory>
#include <vector>

namespace workphone
{
    struct WPNetworkGNSConfig
    {
        String bindAddress = "[::1]:28080";
        u32 maxClients = 32;
        u32 eventsPerPoll = 128;
        bool allowUnverifiedLoopback = false;
        Array<String> allowedPeerIdentities;
    };

    /** GNS implementation of the engine facade. Construction opens no sockets.
     * Strict mode requires a provisioned certificate and explicit peer allowlist.
     * Unverified development mode is restricted to loopback endpoints.
     * One process-wide GNS identity is shared; certificates can only be installed
     * when this is the sole, stopped manager. No application secrets are logged.
     */
    class WPNetwork_API WPNetworkGNSManager : public INetworkManager
    {
    public:
        explicit WPNetworkGNSManager( const WPNetworkGNSConfig &config = {} );
        ~WPNetworkGNSManager() override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void poll() override;
        void disconnect();
        std::vector<u8> getCertificateRequest();
        /// Import an issuer response. Successful import is not proof of trust;
        /// the peer must still validate the chain during its strict handshake.
        void installCertificate( const std::vector<u8> &certificate );
        String getLastError() const;
        String getBoundAddress() const;
        void setServer( bool server ) override;
        bool isServer() const override;
        void setPeer() override;
        void setVerbose( bool verbose ) override;
        void setListener( SmartPtr<INetworkListener> listener ) override;
        void addListener( SmartPtr<INetworkListener> listener ) override;
        void removeListener( SmartPtr<INetworkListener> listener ) override;
        void sendPacket( SmartPtr<IPacket> &packet ) override;
        void sendPacket( SmartPtr<IPacket> &packet, u16 player ) override;
        void sendPacket( SmartPtr<IPacket> &packet, SmartPtr<ISystemAddress> address ) override;
        void sendPacketToAllExcept( SmartPtr<IPacket> &packet,
                                    SmartPtr<ISystemAddress> address ) override;
        void sendPacketUnreliable( SmartPtr<IPacket> &packet ) override;
        void sendPacketUnreliable( SmartPtr<IPacket> &packet, u16 player ) override;
        const u32 getPeerCount() override;
        u16 getPlayerNumber() const override;
        const u32 getClientAddress( u16 player ) override;
        String getLocalRakNetGUID() override;
        void kickClient( u16 player, bool hardKick = false ) override;
        ConnectionStatus getConnectionStatus() override;
        void connect( const String &address, unsigned short port, const String &password ) override;
        const u32 getPing() override;
        void setNetIterations( u16 iterations ) override;
        void setGlobalPacketRelay( bool relay ) override;
        SmartPtr<IPacket> createPacket() override;
        time_interval getServerTime() const override;
        u32 getCapabilities() const override;
        s32 getPacketSenderId( SmartPtr<IPacket> packet ) const override;
        WP_CLASS_REGISTER_DECL;

    private:
        struct Impl;
        std::shared_ptr<Impl> m_impl;
    };
}  // namespace workphone
