#ifndef _CSystemAddress_H
#define _CSystemAddress_H

#define _WINSOCKAPI_  // stops windows.h including winsock.h

#include <Workphone/Interface/Net/ISystemAddress.hpp>
#include <RakNetTypes.h>

namespace workphone
{

    /**
     * @brief RakNet-backed implementation of ISystemAddress.
     *
     * CSystemAddress wraps a heap-allocated @c RakNet::SystemAddress and maps
     * the engine's address API onto RakNet's IPv4 address model.  It is the
     * only concrete address type used by @c CPacket and @c CNetworkManager.
     *
     * The binary address is stored in network byte order inside
     * @c addr4.sin_addr.s_addr, matching the layout that RakNet populates when
     * a packet arrives.  @c getBinaryAddress / @c setBinaryAddress expose it as
     * a plain @c u32 without byte-swapping so callers get the same value
     * that @c getClientAddress() returns from @c CNetworkManager.
     *
     * ### Typical usage
     * @code
     * // Reading the sender of an incoming packet:
     * auto addr = packet->getSystemAddress();
     * if ( addr->isValid() )
     *     FB_LOG_MESSAGE( "Net", "From " + addr->toString() );
     *
     * // Building an address to target a specific peer:
     * auto addr = make_ptr<CSystemAddress>();
     * addr->setBinaryAddress( knownIp );
     * addr->setPort( 15822 );
     * networkManager->sendPacket( pkt, addr );
     * @endcode
     *
     * @see ISystemAddress, CPacket, CNetworkManager
     */
    class CSystemAddress : public ISystemAddress
    {
    public:
        /// @brief Allocates a default-initialised @c RakNet::SystemAddress.
        CSystemAddress();

        /// @brief Frees the owned @c RakNet::SystemAddress.
        ~CSystemAddress() override;

        // ------------------------------------------------------------------
        // ISystemAddress overrides
        // ------------------------------------------------------------------

        /// @copydoc ISystemAddress::setBinaryAddress
        void setBinaryAddress( u32 binaryAddress ) override;

        /// @copydoc ISystemAddress::getBinaryAddress
        [[nodiscard]] u32 getBinaryAddress() const override;

        /// @copydoc ISystemAddress::setPort
        void setPort( u16 port ) override;

        /// @copydoc ISystemAddress::getPort
        [[nodiscard]] u16 getPort() const override;

        /// @copydoc ISystemAddress::toString
        [[nodiscard]] String toString() const override;

        /// @copydoc ISystemAddress::isValid
        [[nodiscard]] bool isValid() const override;

        // ------------------------------------------------------------------
        // RakNet-specific accessor
        // ------------------------------------------------------------------

        /**
         * @brief Returns a pointer to the underlying @c RakNet::SystemAddress.
         *
         * Used by @c CPacket to copy the sender address into this wrapper
         * and by @c CNetworkManager to pass the address directly to
         * @c RakPeerInterface::Send.
         *
         * @return Non-owning pointer to the internal address; never @c nullptr
         *         after construction.
         */
        [[nodiscard]] RakNet::SystemAddress *getSystemAddress() const;
        void setSystemAddress( const RakNet::SystemAddress &systemAddress );

    protected:
        /// Owned heap-allocated RakNet address; freed in the destructor.
        RakNet::SystemAddress *m_systemAddress = nullptr;
    };

    /// @brief Convenience alias for a smart-pointer to CSystemAddress.
    using CSystemAddressPtr = SmartPtr<CSystemAddress>;

}  // namespace workphone

#endif
