#ifndef AtomicString_h__
#define AtomicString_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Atomics/AtomicNumber.hpp>
#include <Workphone/Core/StringBase.hpp>

namespace workphone
{

    /**
     * @class AtomicString
     * @brief A thread-safe wrapper for string types using a custom lightweight lock.
     *
     * AtomicString provides synchronization for string operations, ensuring that access
     * to the underlying string is thread-safe. It uses a spin-lock mechanism based on
     * an atomic state variable.
     *
     * @tparam T The character type of the string (e.g., c8 for UTF-8).
     */
    template <class T>
    class WPCore_API AtomicString
    {
    public:
        /** @brief Default constructor. Initializes an empty atomic string. */
        AtomicString();

        /**
         * @brief Constructor that initializes the atomic string with a given base string.
         * @param other The base string to copy.
         */
        AtomicString( const StringBase<T, std::char_traits<T>, std::allocator<T>> &other );

        /**
         * @brief Loads the current value of the string.
         * @return A copy of the underlying string.
         */
        StringBase<T, std::char_traits<T>, std::allocator<T>> load() const;

        /** @brief Conversion operator to BaseString. */
        operator BaseString<T>() const;

        /**
         * @brief Returns the underlying string.
         * @note This may require external synchronization depending on implementation.
         * @return The current string value.
         */
        StringBase<T, std::char_traits<T>, std::allocator<T>> str() const;

        /** @brief Acquires the lock to provide exclusive access to the string. */
        void lock() const;

        /**
         * @brief Attempts to acquire the lock without blocking.
         * @return True if the lock was successfully acquired, false otherwise.
         */
        bool try_lock() const;

        /** @brief Releases the lock. */
        void unlock() const;

    protected:
        /** The underlying string being managed. */
        StringBase<T, std::char_traits<T>, std::allocator<T>> m_string;

        /**
         * @brief Atomic lock state.
         * 0: Unlocked, 1: Locked.
         */
        mutable std::atomic_int m_lockState = 0;
    };

    using atomic_string = AtomicString<c8>;

}  // namespace workphone

#endif  // AtomicString_h__
