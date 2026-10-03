#ifndef WPNetworkPacket_h__
#define WPNetworkPacket_h__

#include <WPNetwork/WPNetworkPrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Interface/Net/IPacket.hpp>

extern "C" {
#include <WorkphoneNetwork/workphone_network.h>
}

namespace workphone
{
    /**
     * @class WPNetworkPacket
     * @brief Implementation of a network packet used for serializing and deserializing data for transmission.
     *
     * This class provides a buffer-backed implementation of IPacket, allowing various data types to be
     * written to and read from the packet.
     */
    class WPNetwork_API WPNetworkPacket : public IPacket
    {
    public:
        /** @brief Default constructor. */
        WPNetworkPacket();

        /** @brief Constructor that initializes the packet from a network event. @param event The network event containing the packet data. */
        explicit WPNetworkPacket( const NetEvent &event );

        ~WPNetworkPacket() override;

        /** @brief Initializes an empty packet. */
        void initialise();
        /** @brief Initializes the packet using data from a network event. @param event The network event. */
        void initialise( const NetEvent &event );

        /** @brief Reads a signed 8-bit integer from the packet. @param value Reference to the variable to store the read value. */
        void read( s8 &value ) override;
        /** @brief Reads an unsigned 8-bit integer from the packet. @param value Reference to the variable to store the read value. */
        void read( u8 &value ) override;
        /** @brief Reads an unsigned 16-bit integer from the packet. @param value Reference to the variable to store the read value. */
        void read( u16 &value ) override;
        /** @brief Reads a signed 16-bit integer from the packet. @param value Reference to the variable to store the read value. */
        void read( s16 &value ) override;
        /** @brief Reads an unsigned 32-bit integer from the packet. @param value Reference to the variable to store the read value. */
        void read( u32 &value ) override;
        /** @brief Reads a signed 32-bit integer from the packet. @param value Reference to the variable to store the read value. */
        void read( s32 &value ) override;
        /** @brief Reads a 32-bit float from the packet. @param value Reference to the variable to store the read value. */
        void read( f32 &value ) override;
        /** @brief Reads a 2D integer vector from the packet. @param value Reference to the variable to store the read value. */
        void read( Vector2I &value ) override;
        /** @brief Reads a 2D floating-point vector from the packet. @param value Reference to the variable to store the read value. */
        void read( Vector2<real_Num> &value ) override;
        /** @brief Reads a 3D integer vector from the packet. @param value Reference to the variable to store the read value. */
        void read( Vector3I &value ) override;
        /** @brief Reads a 3D floating-point vector from the packet. @param value Reference to the variable to store the read value. */
        void read( Vector3<real_Num> &value ) override;
        /** @brief Reads a string from the packet. @param value Reference to the variable to store the read value. */
        void read( String &value ) override;
        /** @brief Reads a boolean value from the packet. @param value Reference to the variable to store the read value. */
        void read( bool &value ) override;

        /** @brief Writes a signed 8-bit integer to the packet. @param value The value to write. */
        void write( s8 value ) override;
        /** @brief Writes an unsigned 8-bit integer to the packet. @param value The value to write. */
        void write( u8 value ) override;
        /** @brief Writes an unsigned 16-bit integer to the packet. @param value The value to write. */
        void write( u16 value ) override;
        /** @brief Writes a signed 16-bit integer to the packet. @param value The value to write. */
        void write( s16 value ) override;
        /** @brief Writes an unsigned 32-bit integer to the packet. @param value The value to write. */
        void write( u32 value ) override;
        /** @brief Writes a signed 32-bit integer to the packet. @param value The value to write. */
        void write( s32 value ) override;
        /** @brief Writes a 32-bit float to the packet. @param value The value to write. */
        void write( f32 value ) override;
        /** @brief Writes a 2D integer vector to the packet. @param value The vector to write. */
        void write( const Vector2I &value ) override;
        /** @brief Writes a 2D floating-point vector to the packet. @param value The vector to write. */
        void write( const Vector2<real_Num> &value ) override;
        /** @brief Writes a 3D integer vector to the packet. @param value The vector to write. */
        void write( const Vector3I &value ) override;
        /** @brief Writes a 3D floating-point vector to the packet. @param value The vector to write. */
        void write( const Vector3<real_Num> &value ) override;
        /** @brief Writes a string to the packet. @param value The string to write. */
        void write( const String &value ) override;
        /** @brief Writes a boolean value to the packet. @param value The value to write. */
        void write( const bool &value ) override;

        /** @brief Marks the message ID as ignored for this packet. */
        void ignoreMessageId() override;

        /** @brief Returns the current length of the packet data. @return Size of the data buffer in bytes. */
        u32 getDataLength() const override;

        /** @brief Resets the read pointer to the beginning of the packet. */
        void resetReadPointer() override;

        /** @brief Returns the system address associated with this packet. @return Smart pointer to the system address. */
        SmartPtr<ISystemAddress> getSystemAddress() const override;

        /** @brief Returns a pointer to the raw packet data. @return Pointer to the start of the data buffer. */
        const u8 *getData() const;

        /** @brief Directly sets the packet data. @param data Pointer to the source data. @param size Size of the data in bytes. */
        void setData( const void *data, size_t size );

        /** @brief Associates a system address with this packet. @param systemAddress Smart pointer to the system address. */
        void setSystemAddress( SmartPtr<ISystemAddress> systemAddress );

        WP_CLASS_REGISTER_DECL;

    private:
        /** @brief Appends raw bytes to the packet buffer. @param data Pointer to the data to append. @param size Number of bytes to append. */
        void appendBytes( const void *data, size_t size );

        /** @brief Extracts raw bytes from the packet buffer. @param data Pointer to the destination buffer. @param size Number of bytes to extract. */
        void extractBytes( void *data, size_t size );

        Array<u8> m_buffer;                        ///< Internal buffer storing the packet data
        size_t m_readPosition = 0;                 ///< Current read offset within the buffer
        SmartPtr<ISystemAddress> m_systemAddress;  ///< The system address associated with this packet
    };
}  // namespace workphone

#endif  // WPNetworkPacket_h__
