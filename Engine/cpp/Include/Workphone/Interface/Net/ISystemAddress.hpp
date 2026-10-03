#ifndef _ISystemAddress_H
#define _ISystemAddress_H

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /**
     * @brief Engine-agnostic network endpoint address.
     *
     * ISystemAddress represents the IP address and port of a remote (or local)
     * network peer, mirroring the role of @c RakNet::SystemAddress in the
     * transport layer.  The interface intentionally hides RakNet types so that
     * higher-level engine code stays portable.
     *
     * The binary address is stored as a 32-bit host-order IPv4 value
     * (same layout as @c in_addr::s_addr after @c ntohl).  Convert to and
     * from a human-readable string via @c toString.
     *
     * ### Typical usage
     * @code
     * SmartPtr<ISystemAddress> addr = packet->getSystemAddress();
     * if ( addr && addr->isValid() )
     *     FB_LOG_MESSAGE( "Net", "Packet from " + addr->toString() );
     * @endcode
     *
     * @see IPacket, INetworkManager, INetworkListener
     */
    class WPCore_API ISystemAddress : public ISharedObject
    {
    public:
        ~ISystemAddress() override;

        // ------------------------------------------------------------------
        // Address
        // ------------------------------------------------------------------

        /**
         * @brief Sets the IPv4 address as a 32-bit host-order integer.
         *
         * @param binaryAddress  Host-order IPv4 address (e.g. the value of
         *                       @c in_addr::s_addr after @c ntohl).
         */
        virtual void setBinaryAddress( u32 binaryAddress ) = 0;

        /**
         * @brief Returns the IPv4 address as a 32-bit host-order integer.
         *
         * @return Host-order IPv4 address, or @c 0 if not set.
         */
        virtual u32 getBinaryAddress() const = 0;

        // ------------------------------------------------------------------
        // Port
        // ------------------------------------------------------------------

        /**
         * @brief Sets the port number in host byte order.
         *
         * @param port  Port number (1–65535).
         */
        virtual void setPort( u16 port ) = 0;

        /**
         * @brief Returns the port number in host byte order.
         *
         * @return Port number, or @c 0 if not set.
         */
        virtual u16 getPort() const = 0;

        // ------------------------------------------------------------------
        // Utilities
        // ------------------------------------------------------------------

        /**
         * @brief Returns a human-readable @c "IP:port" string for this address.
         *
         * Equivalent to @c RakNet::SystemAddress::ToString(true).  Useful for
         * logging and diagnostics.
         *
         * @return String of the form @c "192.168.1.1:7777", or an empty string
         *         if the address is unassigned.
         */
        String toString() const override = 0;

        /**
         * @brief Returns @c true if this address has been assigned a valid
         *        endpoint (i.e. it is not the unassigned/null sentinel value).
         */
        bool isValid() const override = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif
