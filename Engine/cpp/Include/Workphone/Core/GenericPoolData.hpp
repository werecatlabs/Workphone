#ifndef GenericPoolData_h__
#define GenericPoolData_h__

#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Deque.hpp>

namespace workphone
{
    /**
     * @brief A generic memory pool for allocating and managing objects of type T using allocator A.
     *
     * This class provides efficient allocation and deallocation of objects by maintaining a pool of
     * pre-allocated memory blocks. It is useful for scenarios where frequent allocation and deallocation
     * of objects of the same type are required.
     *
     * @tparam T The type of objects to allocate.
     * @tparam A The allocator type to use for memory management.
     */
    template <class T, class A>
    class GenericPoolData
    {
    public:
        /**
         * @brief Default constructor. Does not allocate memory until load() or setNumElements() is
         * called.
         */
        GenericPoolData();

        GenericPoolData( GenericPoolData &&other ) noexcept;

        /**
         * @brief Constructor that pre-allocates a specified number of elements.
         * @param numElements Number of elements to allocate in the pool.
         */
        explicit GenericPoolData( size_t numElements );

        /**
         * @brief Destructor. Releases all allocated memory.
         */
        ~GenericPoolData();

        /**
         * @brief Allocates memory for the pool using the allocator.
         * @param data Optional shared object data (unused in this implementation).
         */
        void load( SmartPtr<ISharedObject> data );

        /**
         * @brief Deallocates all memory and clears the pool.
         * @param data Optional shared object data (unused in this implementation).
         */
        void unload( SmartPtr<ISharedObject> data );

        /**
         * @brief Sets the number of elements in the pool, reallocating memory as needed.
         * @param numElements The new number of elements to allocate.
         *
         * Existing data is deallocated and new memory is allocated. All elements are marked as free.
         */
        void setNumElements( size_t numElements );

        /**
         * @brief Marks all elements in the pool as free.
         *
         * Does not deallocate memory, but resets the free list.
         */
        void freeAllElements();

        /**
         * @brief Retrieves a pointer to a free element from the pool.
         * @return Pointer to a free element, or nullptr if none are available.
         */
        T *getFreeElement();

        /**
         * @brief Marks a specific element as free, returning it to the pool.
         * @param element Pointer to the element to mark as free.
         */
        void setElementFree( T *element );

        /**
         * @brief Gets a pointer to the underlying data array.
         * @return Pointer to the data array, or nullptr if not allocated.
         */
        T *getData();

        /**
         * @brief Gets the number of elements currently allocated in the pool.
         * @return Number of allocated elements.
         */
        size_t getNumAllocated() const;

        /**
         * @brief Gets the number of free elements currently available in the pool.
         * @return Number of free elements.
         */
        size_t getNumFree() const;

        /**
         * @brief Accesses an element by index.
         * @param index Index of the element to access.
         * @return Reference to the element at the specified index.
         * @note Asserts if index is out of bounds.
         */
        T &operator[]( size_t index );

        /**
         * @brief Accesses an element by index (const version).
         * @param index Index of the element to access.
         * @return Const reference to the element at the specified index.
         * @note Asserts if index is out of bounds.
         */
        const T &operator[]( size_t index ) const;

        GenericPoolData<T, A> &operator=( GenericPoolData<T, A> &&other ) noexcept;

        /**
         * @brief Gets a reference to the allocator used by the pool.
         * @return Reference to the allocator.
         */
        A &getAlloc();

        /**
         * @brief Gets a const reference to the allocator used by the pool.
         * @return Const reference to the allocator.
         */
        const A &getAlloc() const;

    private:
        /// Pointer to the array of allocated data.
        T *m_data = nullptr;

        /// Number of elements allocated in the array.
        size_t m_numAllocated = 0;

        /// List of pointers to free elements in the pool.
        Deque<T *> m_freeElements;

        /// Allocator instance used for memory management.
        A m_alloc;
    };

    template <class T, class A>
    GenericPoolData<T, A>::GenericPoolData() : m_data( nullptr ), m_numAllocated( 0 )
    {
        // Pool is uninitialized until load() or setNumElements() is called.
    }

    template <class T, class A>
    GenericPoolData<T, A>::GenericPoolData( GenericPoolData &&other ) noexcept :
        m_data( other.m_data ),
        m_numAllocated( other.m_numAllocated ),
        m_freeElements( std::move( other.m_freeElements ) ),
        m_alloc( std::move( other.m_alloc ) )
    {
        other.m_data = nullptr;
        other.m_numAllocated = 0;
        other.m_freeElements.clear();
    }

    template <class T, class A>
    GenericPoolData<T, A>::GenericPoolData( size_t numElements ) : m_numAllocated( numElements )
    {
        // Memory is not allocated until load() or setNumElements() is called.
    }

    template <class T, class A>
    GenericPoolData<T, A>::~GenericPoolData()
    {
        unload( nullptr );
    }

    template <class T, class A>
    void GenericPoolData<T, A>::load( SmartPtr<ISharedObject> data )
    {
        if( m_data )
        {
            return;
        }

        auto numElements = getNumAllocated();
        if( numElements == 0 )
        {
            return;
        }

        m_data = m_alloc.allocate( numElements );
        freeAllElements();
    }

    template <class T, class A>
    void GenericPoolData<T, A>::unload( SmartPtr<ISharedObject> data )
    {
        m_freeElements.clear();

        if( m_data )
        {
            m_alloc.deallocate( m_data, m_numAllocated );
            m_data = nullptr;
        }
    }

    template <class T, class A>
    void GenericPoolData<T, A>::setNumElements( size_t numElements )
    {
        T *old_data = m_data;
        m_freeElements.clear();

        if( numElements > 0 )
        {
            m_data = m_alloc.allocate( numElements );

            // Optionally copy old data if needed.
            // auto numElementCopy = m_numAllocated < numElements ? m_numAllocated : numElements;
            // for(size_t i = 0; i < numElementCopy; ++i)
            // {
            //     m_data[i] = old_data[i];
            // }
        }
        else
        {
            m_data = nullptr;
        }

        if( old_data )
        {
            m_alloc.deallocate( old_data, m_numAllocated );
        }

        m_numAllocated = numElements;

        // Mark all elements as free.
        auto element = getData();
        for( size_t i = 0; element && i < m_numAllocated; ++i )
        {
            m_freeElements.push_back( element++ );
        }
    }

    template <class T, class A>
    void GenericPoolData<T, A>::freeAllElements()
    {
        m_freeElements.clear();

        // Mark all elements as free.
        auto element = getData();
        for( size_t i = 0; i < m_numAllocated; ++i )
        {
            m_freeElements.push_back( element++ );
        }
    }

    template <class T, class A>
    T *GenericPoolData<T, A>::getFreeElement()
    {
        if( !m_freeElements.empty() )
        {
            auto element = m_freeElements.front();
            m_freeElements.pop_front();
            return element;
        }

        return nullptr;
    }

    template <class T, class A>
    void GenericPoolData<T, A>::setElementFree( T *element )
    {
        m_freeElements.push_back( element );
    }

    template <class T, class A>
    T *GenericPoolData<T, A>::getData()
    {
        return m_data;
    }

    template <class T, class A>
    size_t GenericPoolData<T, A>::getNumAllocated() const
    {
        return m_numAllocated;
    }

    template <class T, class A>
    size_t GenericPoolData<T, A>::getNumFree() const
    {
        return m_freeElements.size();
    }

    template <class T, class A>
    T &GenericPoolData<T, A>::operator[]( size_t index )
    {
        WP_ASSERT( index < m_numAllocated );
        return m_data[index];
    }

    template <class T, class A>
    const T &GenericPoolData<T, A>::operator[]( size_t index ) const
    {
        WP_ASSERT( index < m_numAllocated );
        return m_data[index];
    }

    template <class T, class A>
    GenericPoolData<T, A> &GenericPoolData<T, A>::operator=( GenericPoolData &&other ) noexcept
    {
        if( this != &other )
        {
            unload( nullptr );

            m_data = other.m_data;
            m_numAllocated = other.m_numAllocated;
            m_freeElements = std::move( other.m_freeElements );
            m_alloc = std::move( other.m_alloc );

            other.m_data = nullptr;
            other.m_numAllocated = 0;
            other.m_freeElements.clear();
        }

        return *this;
    }

    template <class T, class A>
    A &GenericPoolData<T, A>::getAlloc()
    {
        return m_alloc;
    }

    template <class T, class A>
    const A &GenericPoolData<T, A>::getAlloc() const
    {
        return m_alloc;
    }

}  // namespace workphone

#endif  // PoolData_h__
