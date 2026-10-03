#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Atomics/AtomicString.hpp>

namespace workphone
{

    // ---------------------------------------------------------------------------
    // AtomicString implementation
    // ---------------------------------------------------------------------------

    template <class T>
    AtomicString<T>::AtomicString()
    {
        // Default constructed, member m_string default‑constructed as well.
    }

    template <class T>
    AtomicString<T>::AtomicString( const StringBase<T, std::char_traits<T>, std::allocator<T>> &other ) :
        m_string( other )
    {
    }

    template <class T>
    StringBase<T, std::char_traits<T>, std::allocator<T>> AtomicString<T>::load() const
    {
        // Spin until lock acquired
        while( !try_lock() )
        {
            // busy‑wait
        }

        auto value = m_string;
        unlock();
        return value;
    }

    template <class T>
    AtomicString<T>::operator BaseString<T>() const
    {
        return load();
    }

    template <class T>
    StringBase<T, std::char_traits<T>, std::allocator<T>> AtomicString<T>::str() const
    {
        return load();
    }

    template <class T>
    void AtomicString<T>::lock() const
    {
        while( m_lockState.exchange( 1, std::memory_order_acquire ) == 1 )
        {
            // busy‑wait
        }
    }

    template <class T>
    bool AtomicString<T>::try_lock() const
    {
        return m_lockState.exchange( 1, std::memory_order_acquire ) == 0;
    }

    template <class T>
    void AtomicString<T>::unlock() const
    {
        m_lockState.store( 0, std::memory_order_release );
    }

    // Explicit instantiation for the engine's default character type.
    template class AtomicString<c8>;

}  // namespace workphone
