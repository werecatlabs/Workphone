#ifndef UUID_h__
#define UUID_h__

#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/FixedArray.hpp>

namespace workphone
{
    /**
     * @brief Universally Unique Identifier (UUID) class.
     *
     * Provides a 128-bit identifier and utility functions for generation,
     * string conversion, and basic UUID properties (version, variant).
     */
    class WPCore_API uuid
    {
    public:
        using value_type = u8;
        using storage_type = FixedArray<value_type, 16>;
        using iterator = storage_type::iterator;
        using const_iterator = storage_type::const_iterator;

        /** @brief Constructs a nil UUID (all zeros). */
        uuid();

        /**
         * @brief Constructs a UUID from a predefined byte array.
         * @param bytes The 16-byte array containing the UUID.
         */
        explicit uuid( const storage_type &bytes );

        /**
         * @brief Constructs a UUID from an initializer list of bytes.
         * @param bytes List of bytes (should be 16 bytes).
         */
        uuid( std::initializer_list<value_type> bytes );

        /**
         * @brief Generates a new random UUID.
         * @return A newly generated uuid.
         */
        static uuid generate();

        /**
         * @brief Creates a UUID from a string representation.
         * @param str The string representation of the UUID.
         * @return The parsed uuid.
         */
        static uuid from_string( const std::string &str );

        /**
         * @brief Creates a UUID from a wide string representation.
         * @param str The wide string representation of the UUID.
         * @return The parsed uuid.
         */
        static uuid from_string( const std::wstring &str );

        /** @brief Converts the UUID to a standard string. */
        String to_string() const;

        /** @brief Converts the UUID to a wide string. */
        StringW to_wstring() const;

        /** @brief Checks if the UUID is the nil UUID (all zeros). */
        bool is_nil() const;

        /** @brief Returns the UUID variant. */
        s32 variant() const;

        /** @brief Returns the UUID version. */
        s32 version() const;

        /** @brief Returns a reference to the underlying byte storage. */
        const storage_type &bytes() const;

        /** @brief Returns a pointer to the raw byte data. */
        value_type *data();

        /** @brief Returns a const pointer to the raw byte data. */
        const value_type *data() const;

        /** @brief Returns the size of the UUID in bytes (always 16). */
        size_t size() const;

        iterator begin();

        const_iterator begin() const;

        const_iterator cbegin() const;

        iterator end();

        const_iterator end() const;

        const_iterator cend() const;

        value_type &operator[]( size_t index );

        const value_type &operator[]( size_t index ) const;

        bool operator==( const uuid &other ) const;

        bool operator!=( const uuid &other ) const;

        bool operator<( const uuid &other ) const;

        bool operator<=( const uuid &other ) const;

        bool operator>( const uuid &other ) const;

        bool operator>=( const uuid &other ) const;

        /**
         * @brief Static helper to convert a UUID to a string.
         * @param value The UUID to convert.
         * @return The string representation.
         */
        static String to_string( const uuid &value );

        /**
         * @brief Static helper to convert a UUID to a wide string.
         * @param value The UUID to convert.
         * @return The wide string representation.
         */
        static StringW to_wstring( const uuid &value );

    private:
        /** @brief Helper to convert a hex character to its integer value. */
        static s32 hexValue( char ch );

        storage_type m_bytes = {};
    };

    using UUID = uuid;

}  // namespace workphone

namespace std
{
    template <>
    struct hash<workphone::uuid>
    {
        size_t operator()( const workphone::uuid &value ) const
        {
            size_t result = 1469598103934665603ull;
            for( const auto byte : value )
            {
                result ^= static_cast<size_t>( byte );
                result *= 1099511628211ull;
            }

            return result;
        }
    };
}  // namespace std

#endif  // UUID_h__
