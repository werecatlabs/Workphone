#ifndef StringBuffer_h__
#define StringBuffer_h__

#include <Workphone/WorkphoneTypes.hpp>

#include <string>
#include <cstring>
#include <algorithm>

namespace workphone
{

    /**
     * @brief A string buffer for the workphone game engine.
     *
     * A dynamic string buffer that efficiently manages string concatenation and manipulation
     * operations. Uses small buffer optimization for short strings to avoid heap allocations.
     */
    class WPCore_API StringBuffer
    {
    public:
        /** Default constructor. Initializes with empty string. */
        StringBuffer();

        /** Constructor with initial capacity.
         * @param capacity Initial capacity to reserve
         */
        explicit StringBuffer( size_t capacity );

        /** Constructor from C-string.
         * @param str Null-terminated C-string
         */
        explicit StringBuffer( const char *str );

        /** Constructor from std::string.
         * @param str String to initialize with
         */
        explicit StringBuffer( const std::string &str );

        /** Copy constructor. */
        StringBuffer( const StringBuffer &other );

        /** Move constructor. */
        StringBuffer( StringBuffer &&other ) noexcept;

        /** Destructor. */
        ~StringBuffer();

        /** Copy assignment operator. */
        StringBuffer &operator=( const StringBuffer &other );

        /** Move assignment operator. */
        StringBuffer &operator=( StringBuffer &&other ) noexcept;

        /** Append a C-string.
         * @param str Null-terminated C-string to append
         * @return Reference to this buffer for chaining
         */
        StringBuffer &append( const char *str );

        /** Append a string.
         * @param str String to append
         * @return Reference to this buffer for chaining
         */
        StringBuffer &append( const std::string &str );

        /** Append another StringBuffer.
         * @param other Buffer to append
         * @return Reference to this buffer for chaining
         */
        StringBuffer &append( const StringBuffer &other );

        /** Append a character.
         * @param ch Character to append
         * @return Reference to this buffer for chaining
         */
        StringBuffer &append( char ch );

        /** Append a substring.
         * @param str String to append from
         * @param pos Starting position
         * @param len Length of substring (use npos for remainder)
         * @return Reference to this buffer for chaining
         */
        StringBuffer &append( const char *str, size_t pos, size_t len );

        /** Clear the buffer content. */
        void clear();

        /** Reserve capacity for future operations.
         * @param capacity Minimum capacity to reserve
         */
        void reserve( size_t capacity );

        /** Get the current length of the string.
         * @return String length in characters
         */
        size_t length() const;

        /** Get the current size of the string (same as length).
         * @return String size in characters
         */
        size_t size() const;

        /** Get the current capacity.
         * @return Current capacity in characters
         */
        size_t capacity() const;

        /** Check if buffer is empty.
         * @return true if empty, false otherwise
         */
        bool empty() const;

        /** Get C-string representation.
         * @return Null-terminated C-string
         */
        const char *c_str() const;

        /** Get raw data pointer.
         * @return Pointer to character data
         */
        const char *data() const;

        /** Convert to std::string.
         * @return String copy
         */
        std::string toString() const;

        /** Operator to append C-string.
         * @param str C-string to append
         * @return Reference to this buffer
         */
        StringBuffer &operator+=( const char *str );

        /** Operator to append std::string.
         * @param str String to append
         * @return Reference to this buffer
         */
        StringBuffer &operator+=( const std::string &str );

        /** Operator to append character.
         * @param ch Character to append
         * @return Reference to this buffer
         */
        StringBuffer &operator+=( char ch );

        /** Get character at index.
         * @param index Position to access
         * @return Character at position
         */
        char operator[]( size_t index ) const;

        /** Equality comparison.
         * @param other Buffer to compare with
         * @return true if equal, false otherwise
         */
        bool operator==( const StringBuffer &other ) const;

        /** Inequality comparison.
         * @param other Buffer to compare with
         * @return true if not equal, false otherwise
         */
        bool operator!=( const StringBuffer &other ) const;

        /** Special value for "until end of string" */
        static constexpr size_t npos = static_cast<size_t>( -1 );

    private:
        /** Small buffer size for optimization */
        static constexpr size_t SMALL_BUFFER_SIZE = 64;

        /** Ensure capacity is sufficient.
         * @param requiredCapacity Minimum required capacity
         */
        void ensureCapacity( size_t requiredCapacity );

        /** Destroy current buffer if using heap allocation. */
        void destroy();

        char *m_buffer;                         ///< Pointer to character buffer
        size_t m_length;                        ///< Current string length
        size_t m_capacity;                      ///< Current buffer capacity
        char m_smallBuffer[SMALL_BUFFER_SIZE];  ///< Small buffer optimization
        bool m_usingSmallBuffer;                ///< Flag indicating if using small buffer
    };

}  // namespace workphone

#endif  // StringBuffer_h__
