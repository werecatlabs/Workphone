#ifndef auto_ptr_h__
#define auto_ptr_h__

namespace luabind
{

    template <typename T>
    class auto_ptr
    {
    public:
        // Constructor
        explicit auto_ptr( T *ptr = nullptr ) : m_ptr( ptr )
        {
        }

        // Copy constructor
        auto_ptr( auto_ptr &other ) : m_ptr( other.release() )
        {
        }

        // Destructor
        ~auto_ptr()
        {
            delete m_ptr;
        }

        // Assignment operator
        auto_ptr &operator=( auto_ptr &other )
        {
            if( this != &other )
            {
                delete m_ptr;
                m_ptr = other.release();
            }
            return *this;
        }

        // Dereference operator
        T &operator*() const
        {
            return *m_ptr;
        }

        // Arrow operator
        T *operator->() const
        {
            return m_ptr;
        }

        // Get the underlying pointer
        T *get() const
        {
            return m_ptr;
        }

        // Release the ownership of the pointer
        T *release()
        {
            T *temp = m_ptr;
            m_ptr = nullptr;
            return temp;
        }

        // Reset the managed object
        void reset( T *ptr = nullptr )
        {
            if( m_ptr != ptr )
            {
                delete m_ptr;
                m_ptr = ptr;
            }
        }

    private:
        T *m_ptr;
    };

    template <class T>
    T *get_pointer( const auto_ptr<T> &pointer )
    {
        return pointer.get();
    }

}  // namespace luabind

#endif  // auto_ptr_h__
