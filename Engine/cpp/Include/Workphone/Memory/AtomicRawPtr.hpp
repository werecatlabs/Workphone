#ifndef __WP_AtomicRawPtr_h__
#define __WP_AtomicRawPtr_h__

#include <Workphone/Atomics/AtomicTypes.hpp>

namespace workphone
{

    /**
     * @brief A thin, thread-safe wrapper for an atomic raw pointer.
     *
     * This class encapsulates a `std::atomic<T*>` and exposes a small,
     * convenient API for storing, loading and exchanging the raw pointer.
     *
     * Important:
     * - This class does NOT take ownership of the pointee; it will not delete
     *   the object when it is destroyed or when the pointer value is changed.
     * - Copy and copy-assignment are implicitly deleted because `std::atomic<T*>`
     *   is not copyable. Move operations are also implicitly deleted.
     *
     * @tparam T Type of the pointed-to object.
     */
    template <class T>
    class AtomicRawPtr
    {
    public:
        using this_type = AtomicRawPtr<T>;  ///< The type of the atomic raw pointer.
        using element_type = T;             ///< The type of the element.
        using value_type = T;               ///< The type of the value.
        using pointer = T *;                ///< The type of the pointer.

        /**
         * @brief Constructs an atomic raw pointer initialized to null.
         */
        AtomicRawPtr();

        /**
         * @brief Destructor.
         *
         * Sets the stored pointer to null for clarity. Does not free the pointee.
         */
        ~AtomicRawPtr();

        /**
         * @brief Query whether the underlying atomic pointer is lock-free on this platform.
         *
         * This forwards to `std::atomic<T*>::is_lock_free()`; the result may
         * depend on the platform and compiler.
         *
         * @return true if the underlying atomic pointer is lock-free, false otherwise.
         */
        bool is_lock_free() const;

        /**
         * @brief Atomically store a new pointer value.
         *
         * This uses the default sequentially-consistent ordering (same as
         * calling `std::atomic::store` with `std::memory_order_seq_cst`).
         *
         * @param ptr Raw pointer to store. Ownership is not transferred.
         */
        void store( T *ptr );

        /**
         * @brief Atomically load the current pointer value.
         *
         * Uses the default sequentially-consistent ordering.
         *
         * @return The current stored raw pointer. May be null.
         */
        T *load() const;

        /**
         * @brief Atomically assign a new pointer value.
         *
         * Equivalent to `store(ptr)`. This operation is noexcept.
         *
         * @param ptr Raw pointer to assign. Ownership is not transferred.
         */
        void operator=( T *ptr ) noexcept;

        /**
         * @brief Convenience access to the pointee.
         *
         * This performs an atomic load and returns the pointer so callers can
         * use `ptr->member`. The caller must ensure the pointee remains valid
         * for the duration of access.
         *
         * @return The loaded pointer (may be null).
         */
        T *operator->();

        /**
         * @brief Implicit conversion to a raw pointer.
         *
         * Performs an atomic load (sequential consistency) and returns the value.
         * Use with care — implicit conversions can hide atomic operations.
         *
         * @return The stored raw pointer.
         */
        operator T *() const noexcept;

        /**
         * @brief Atomically exchange the stored pointer with a new one.
         *
         * Performs the operation with default sequentially-consistent memory ordering.
         *
         * @param ptr New raw pointer to store. Ownership is not transferred.
         * @return The previous raw pointer value (may be null).
         */
        T *exchange( T *ptr );

        T *get() const;

    protected:
        ///< Underlying atomic pointer. Thread-safe atomic operations operate on this.
        std::atomic<T *> m_pointer;
    };

    template <class T>
    AtomicRawPtr<T>::AtomicRawPtr()
    {
        m_pointer = nullptr;
    }

    template <class T>
    AtomicRawPtr<T>::~AtomicRawPtr()
    {
        m_pointer = nullptr;
    }

    template <class T>
    bool AtomicRawPtr<T>::is_lock_free() const
    {
        return m_pointer.is_lock_free();
    }

    template <class T>
    void AtomicRawPtr<T>::store( T *ptr )
    {
        m_pointer = ptr;
    }

    template <class T>
    T *AtomicRawPtr<T>::load() const
    {
        return m_pointer;
    }

    template <class T>
    void AtomicRawPtr<T>::operator=( T *ptr ) noexcept
    {
        m_pointer = ptr;
    }

    template <class T>
    T *AtomicRawPtr<T>::operator->()
    {
        return m_pointer.load();
    }

    template <class T>
    AtomicRawPtr<T>::operator T *() const noexcept
    {
        return m_pointer;
    }

    template <class T>
    T *AtomicRawPtr<T>::exchange( T *ptr )
    {
        auto old = m_pointer.load();
        m_pointer = ptr;
        return old;
    }

    template <class T>
    T *AtomicRawPtr<T>::get() const
    {
        return m_pointer;
    }

}  // namespace workphone

#endif  // __WP_AtomicRawPtr_h__
