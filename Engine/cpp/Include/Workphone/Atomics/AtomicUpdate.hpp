#ifndef AtomicUpdate_h__
#define AtomicUpdate_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Thread/SpinRWMutex.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>
#include <atomic>

namespace workphone
{
    /** An atomic class to update a generic object. */
    template <class T>
    class AtomicUpdate
    {
    public:
        /** Default constructor. */
        AtomicUpdate();

        /** Constructor that copies a value. */
        AtomicUpdate( const T &value );

        /** Copy constructor. */
        AtomicUpdate( const AtomicUpdate &other );

        /** Sets the object value. */
        void setValue( const T &value );

        /** Gets the object value. */
        T getValue() const;

        /** To know if the object needs an update. */
        bool isUpdateNeeded() const;

        /** Triggers the update event. */
        void updateEvent();

        /** Assignment operator.
        @remarks
        Assumes type has assignment operator to perform atomic operations.
        */
        void operator=( const AtomicUpdate &other );

    protected:
        AtomicValue<u32> m_lastUpdate;
        AtomicValue<u32> m_lastRequest;
        std::atomic<T> m_value;
    };

    template <class T>
    AtomicUpdate<T>::AtomicUpdate() : m_lastUpdate( 0 ), m_lastRequest( 0 )
    {
    }

    template <class T>
    AtomicUpdate<T>::AtomicUpdate( const T &value ) : m_lastUpdate( 0 ), m_lastRequest( 0 )
    {
        m_value = value;
    }

    template <class T>
    AtomicUpdate<T>::AtomicUpdate( const AtomicUpdate<T> &other ) : m_lastUpdate( 0 ), m_lastRequest( 0 )
    {
    }

    template <class T>
    void AtomicUpdate<T>::setValue( const T &value )
    {
        m_value.setValue( value );
        ++m_lastUpdate;
    }

    template <class T>
    T AtomicUpdate<T>::getValue() const
    {
        return m_value.getValue();
    }

    template <class T>
    bool AtomicUpdate<T>::isUpdateNeeded() const
    {
        return m_lastUpdate != m_lastRequest;
    }

    template <class T>
    void AtomicUpdate<T>::updateEvent()
    {
        m_lastUpdate = m_lastRequest;
    }

    template <class T>
    void AtomicUpdate<T>::operator=( const AtomicUpdate &other )
    {
        m_lastUpdate = other.m_lastUpdate;
        m_lastRequest = other.m_lastRequest;
        m_value = other.m_value;
    }

}  // namespace workphone

#endif  // AtomicUpdate_h__
