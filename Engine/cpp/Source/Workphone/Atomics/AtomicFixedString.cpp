#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Atomics/AtomicFixedString.hpp>

namespace workphone
{

    template <u32 size, class T>
    AtomicFixedString<size, T>::AtomicFixedString() = default;

    template <u32 size, class T>
    AtomicFixedString<size, T>::AtomicFixedString( const FixedStringBase<size, T> &other ) :
        m_string( other )
    {
    }

    template <u32 size, class T>
    AtomicFixedString<size, T>::AtomicFixedString( const AtomicFixedString<size, T> &other ) :
        m_string( other.m_string )
    {
    }

    template <u32 size, class T>
    AtomicFixedString<size, T>::AtomicFixedString( const T *s ) : m_string( s )
    {
    }

    template <u32 size, class T>
    AtomicFixedString<size, T> &AtomicFixedString<size, T>::operator=(
        const FixedStringBase<size, T> &other )
    {
        store( other );
        return *this;
    }

    template <u32 size, class T>
    AtomicFixedString<size, T> &AtomicFixedString<size, T>::operator=(
        const AtomicFixedString<size, T> &other )
    {
        if( this != &other )
        {
            store( other.load() );
        }
        return *this;
    }

    template <u32 size, class T>
    void AtomicFixedString<size, T>::store( const FixedStringBase<size, T> &value )
    {
        while( !try_lock() )
        {
            // Spin until the lock is acquired
        }
        m_string = value;
        unlock();
    }

    template <u32 size, class T>
    FixedStringBase<size, T> AtomicFixedString<size, T>::load() const
    {
        while( !try_lock() )
        {
            // Spin until the lock is acquired
        }
        FixedStringBase<size, T> value = m_string;
        unlock();
        return value;
    }

    template <u32 size, class T>
    AtomicFixedString<size, T>::operator StringBase<T, std::char_traits<T>, std::allocator<T>>() const
    {
        return load().str();
    }

    template <u32 size, class T>
    StringBase<T, std::char_traits<T>, std::allocator<T>> AtomicFixedString<size, T>::str() const
    {
        return load().str();
    }

    template <u32 size, class T>
    void AtomicFixedString<size, T>::lock() const
    {
        while( m_lockState.exchange( 1, std::memory_order_acquire ) == 1 )
        {
            // Spin until the lock is acquired
        }
    }

    template <u32 size, class T>
    bool AtomicFixedString<size, T>::try_lock() const
    {
        return m_lockState.exchange( 1, std::memory_order_acquire ) == 0;
    }

    template <u32 size, class T>
    void AtomicFixedString<size, T>::unlock() const
    {
        m_lockState.store( 0, std::memory_order_release );
    }

    // Explicit instantiation for common sizes with default character type.
    template class AtomicFixedString<32, c8>;
    template class AtomicFixedString<64, c8>;
    template class AtomicFixedString<128, c8>;
    template class AtomicFixedString<256, c8>;
    template class AtomicFixedString<512, c8>;
    template class AtomicFixedString<1024, c8>;

}  // namespace workphone
