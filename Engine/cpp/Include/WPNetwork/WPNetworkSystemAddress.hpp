#ifndef WPNetworkSystemAddress_h__
#define WPNetworkSystemAddress_h__

#include <WPNetwork/WPNetworkPrerequisites.hpp>
#include <Workphone/Interface/Net/ISystemAddress.hpp>

extern "C" {
#include <WorkphoneNetwork/workphone_network.h>
}

namespace workphone
{
    class WPNetwork_API WPNetworkSystemAddress : public ISystemAddress
    {
    public:
        WPNetworkSystemAddress();
        explicit WPNetworkSystemAddress( const NetAddress &address );
        ~WPNetworkSystemAddress() override;

        void setBinaryAddress( u32 binaryAddress ) override;
        u32 getBinaryAddress() const override;
        void setPort( u16 port ) override;
        u16 getPort() const override;
        String toString() const override;
        bool isValid() const override;

        const NetAddress &getNetAddress() const;
        void setNetAddress( const NetAddress &address );

        WP_CLASS_REGISTER_DECL;

    private:
        NetAddress m_address;
        bool m_valid = false;
    };
}  // namespace workphone

#endif  // WPNetworkSystemAddress_h__
