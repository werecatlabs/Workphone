#ifndef WP_OBJECT_OBSERVER_HPP
#define WP_OBJECT_OBSERVER_HPP
#include <Workphone/Memory/SmartPtr.hpp>
#include <memory>

namespace workphone
{
    // Captured only while a strong owner exists. Invalidated before destruction,
    // including factory teardown. No reads from released or recycled storage.
    WPCore_API std::weak_ptr<const void> observeObjectLifetime( ISharedObject *object );
    WPCore_API void expireObjectLifetime( const ISharedObject *object );

    /** Non-owning observer for operations serialized with object destruction on
     * their owner thread. This is not a concurrent weak-to-strong promotion. */
    template <class T> class ObjectObserver
    {
    public:
        ObjectObserver() = default;
        ObjectObserver( T *object ) { assign( object ); }
        template <class U> ObjectObserver( const SmartPtr<U> &object ) { assign( object.get() ); }
        ObjectObserver &operator=( T *object ) { assign( object ); return *this; }
        template <class U> ObjectObserver &operator=( const SmartPtr<U> &object )
        { assign( object.get() ); return *this; }
        SmartPtr<T> lock() const { return m_lifetime.expired() ? nullptr : SmartPtr<T>( m_object ); }
        bool expired() const { return m_lifetime.expired(); }
    private:
        void assign( T *object )
        {
            m_object = object;
            m_lifetime = object ? observeObjectLifetime( object ) : std::weak_ptr<const void>();
        }
        T *m_object = nullptr;
        std::weak_ptr<const void> m_lifetime;
    };
}
#endif
