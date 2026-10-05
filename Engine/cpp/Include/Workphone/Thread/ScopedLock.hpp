#ifndef ScopedLock_h__
#define ScopedLock_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone
{

    /**
     * A nested class that provides a scoped locking mechanism for the shared object.
     *
     * The `ScopeLock` class locks the specified shared object during its construction and unlocks it
     * during its destruction. This class is used to ensure that a shared object is thread-safe by
     * preventing concurrent access to the object's data. The shared object is unlocked when the
     * `ScopeLock` object is destructed.
     *
     * @note If the specified shared object is `nullptr`, the `ScopeLock` object is constructed without
     * locking any object.
     *
     * @see ISharedObject, SmartPtr, IObject
     */
    template <class T>
    class ScopedLock
    {
    public:
        /**
         * Constructs a `ScopeLock` object and locks the specified shared object.
         * @param sharedObject A `SmartPtr` object that represents the shared object to lock.
         * @see ISharedObject, SmartPtr, IObject
         */
        explicit ScopedLock( const SmartPtr<T> &object, bool write = true );
        explicit ScopedLock( const T *object, bool write = true );

        ScopedLock( const ScopedLock & ) = delete;
        ScopedLock &operator=( const ScopedLock & ) = delete;

        /**
         * Destructs a `ScopeLock` object and unlocks the specified shared object.
         *
         * @see ISharedObject, SmartPtr, IObject
         */
        ~ScopedLock();

    protected:
        /** A pointer to the shared object to lock. */
        mutable T *m_object;

        /** True if the lock is a write lock. */
        bool m_write = true;
    };

    template <class T>
    ScopedLock<T>::ScopedLock( const SmartPtr<T> &object, bool write ) :
        m_object( object.get() ),
        m_write( write )
    {
        if( m_write )
        {
            if( m_object )
            {
                m_object->lock();
            }
        }
        else
        {
            if( m_object )
            {
                m_object->lock_shared();
            }
        }
    }

    template <class T>
    ScopedLock<T>::ScopedLock( const T *object, bool write ) : m_object( (T *)object ), m_write( write )
    {
        if( m_write )
        {
            if( m_object )
            {
                m_object->lock();
            }
        }
        else
        {
            if( m_object )
            {
                m_object->lock_shared();
            }
        }
    }

    template <class T>
    ScopedLock<T>::~ScopedLock()
    {
        if( m_write )
        {
            if( m_object )
            {
                m_object->unlock();
            }
        }
        else
        {
            if( m_object )
            {
                m_object->unlock_shared();
            }
        }
    }

}  // namespace workphone

#endif  // ScopedLock_h__
