#ifndef AtomicFixedString_h__
#define AtomicFixedString_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Atomics/AtomicNumber.hpp>
#include <Workphone/Core/FixedString.hpp>

namespace workphone
{

    /**
     * @class AtomicFixedString
     * @brief A thread-safe wrapper for fixed-size string types.
     *
     * AtomicFixedString provides synchronization for strings with a compile-time
     * fixed size, preventing data races during concurrent access using a
     * lightweight spin-lock mechanism.
     *
     * @tparam size The maximum capacity of the string.
     * @tparam T The character type of the string (defaults to c8).
     */
    template <u32 size, class T = c8>
    class WPCore_API AtomicFixedString
    {
    public:
        /** @brief Default constructor. Initializes an empty fixed string. */
        AtomicFixedString();

        /**
         * @brief Constructor that initializes from a FixedStringBase.
         * @param other The fixed string to copy.
         */
        AtomicFixedString( const FixedStringBase<size, T> &other );

        /**
         * @brief Copy constructor from another AtomicFixedString.
         * @param other The source atomic fixed string.
         */
        AtomicFixedString( const AtomicFixedString<size, T> &other );

        /**
         * @brief Constructor that initializes from a raw character array.
         * @param s Pointer to the source character array.
         */
        AtomicFixedString( const T *s );

        /**
         * @brief Assignment operator from a FixedStringBase.
         * @param other The fixed string to assign.
         * @return Reference to this object.
         */
        AtomicFixedString<size, T> &operator=( const FixedStringBase<size, T> &other );

        /**
         * @brief Assignment operator from another AtomicFixedString.
         * @param other The source atomic fixed string.
         * @return Reference to this object.
         */
        AtomicFixedString<size, T> &operator=( const AtomicFixedString<size, T> &other );

        /**
         * @brief Atomically stores a new value.
         * @param value The fixed string to store.
         */
        void store( const FixedStringBase<size, T> &value );

        /**
         * @brief Atomically loads a copy of the current value.
         * @return The current fixed string.
         */
        FixedStringBase<size, T> load() const;

        /** @brief Conversion operator to a dynamic StringBase. */
        operator StringBase<T, std::char_traits<T>, std::allocator<T>>() const;

        /**
         * @brief Returns the content as a dynamic StringBase.
         * @return A dynamic string copy of the fixed string.
         */
        StringBase<T, std::char_traits<T>, std::allocator<T>> str() const;

        /** @brief Acquires the lock for exclusive access. */
        void lock() const;

        /**
         * @brief Attempts to acquire the lock without blocking.
         * @return True if the lock was acquired, false otherwise.
         */
        bool try_lock() const;

        /** @brief Releases the lock. */
        void unlock() const;

    protected:
        /** The underlying fixed-size string being managed. */
        FixedStringBase<size, T> m_string;

        /**
         * @brief Atomic lock state.
         * 0: Unlocked, 1: Locked.
         */
        mutable std::atomic_int m_lockState = 0;
    };

    template <u32 size, class T = c8>
    using atomic_fixed_string = AtomicFixedString<size, T>;

}  // namespace workphone

#endif  // AtomicFixedString_h__
