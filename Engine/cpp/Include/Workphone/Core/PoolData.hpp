#ifndef PoolData_h__
#define PoolData_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Deque.hpp>

namespace workphone
{

    /**
     * @brief A memory pool implementation for efficient object allocation and reuse.
     *
     * This class provides a memory pool implementation that pre-allocates a fixed number of objects
     * and manages their allocation/deallocation. It's designed to reduce memory fragmentation and
     * improve performance by reusing objects instead of frequently allocating/deallocating them.
     *
     * @tparam T The type of objects to be stored in the pool
     * @tparam A The allocator type to use for memory management
     */
    template <class T, class A>
    class PoolData : public ISharedObject
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Creates an empty pool with no pre-allocated elements.
         */
        PoolData();

        /**
         * @brief Constructor that pre-allocates a specified number of elements.
         *
         * @param numElements The number of elements to pre-allocate in the pool
         */
        explicit PoolData( size_Num numElements );

        /**
         * @brief Destructor.
         *
         * Frees all allocated memory and cleans up the pool.
         */
        ~PoolData() override;

        /**
         * @brief Loads the pool with the specified number of elements.
         *
         * @param data Unused parameter, maintained for interface compatibility
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unloads the pool and frees all allocated memory.
         *
         * @param data Unused parameter, maintained for interface compatibility
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Sets the number of elements in the pool.
         *
         * This will reallocate the pool with the new size. All existing elements will be freed
         * and new elements will be added to the free list.
         *
         * @param numElements The new number of elements to allocate
         */
        void setNumElements( size_Num numElements );

        /**
         * @brief Frees all elements in the pool and adds them back to the free list.
         *
         * This effectively resets the pool to its initial state where all elements are available.
         */
        void freeAllElements();

        /**
         * @brief Gets a free element from the pool.
         *
         * @return A pointer to a free element, or nullptr if no free elements are available
         */
        T *getFreeElement();

        /**
         * @brief Returns an element back to the pool's free list.
         *
         * @param element Pointer to the element to be returned to the free list
         */
        void setElementFree( T *element );

        /**
         * @brief Gets the raw data array of the pool.
         *
         * @return Pointer to the underlying data array
         */
        T *getData();

        /**
         * @brief Gets the total number of allocated elements in the pool.
         *
         * @return The total number of elements allocated in the pool
         */
        size_Num getNumAllocated() const;

        /**
         * @brief Gets the number of free elements available in the pool.
         *
         * @return The number of elements currently available in the free list
         */
        size_Num getNumFree() const;

        /**
         * @brief Array access operator.
         *
         * @param index The index of the element to access
         * @return Reference to the element at the specified index
         * @throw std::out_of_range if index is out of bounds
         */
        T &operator[]( size_Num index );

        /**
         * @brief Const array access operator.
         *
         * @param index The index of the element to access
         * @return Const reference to the element at the specified index
         * @throw std::out_of_range if index is out of bounds
         */
        const T &operator[]( size_Num index ) const;

        /**
         * @brief Gets the allocator used by the pool.
         *
         * @return Reference to the allocator instance
         */
        A &getAlloc();

        /**
         * @brief Gets the allocator used by the pool (const version).
         *
         * @return Const reference to the allocator instance
         */
        const A &getAlloc() const;

    private:
        /// The array of pre-allocated data elements
        T *m_data = nullptr;

        /// The total number of elements allocated in the array
        size_Num m_numAllocated = 0;

        /// Queue of pointers to free elements available for allocation
        Deque<T *> m_freeElements;

        /// The allocator used for memory management
        A m_alloc;
    };

    template <class T, class A>
    PoolData<T, A>::PoolData() : m_data( nullptr ), m_numAllocated( 0 )
    {
        // uninitialized
    }

    template <class T, class A>
    PoolData<T, A>::PoolData( size_Num numElements ) : m_numAllocated( numElements )
    {
    }

    template <class T, class A>
    PoolData<T, A>::~PoolData()
    {
        unload( nullptr );
    }

    template <class T, class A>
    void PoolData<T, A>::load( SmartPtr<ISharedObject> data )
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
    void PoolData<T, A>::unload( SmartPtr<ISharedObject> data )
    {
        m_freeElements.clear();

        if( m_data )
        {
            m_alloc.deallocate( m_data, m_numAllocated );
            m_data = nullptr;
        }
    }

    template <class T, class A>
    void PoolData<T, A>::setNumElements( size_Num numElements )
    {
        T *old_data = m_data;
        m_freeElements.clear();

        if( numElements > 0 )
        {
            m_data = m_alloc.allocate( numElements );
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

        auto element = getData();
        for( size_t i = 0; element && i < m_numAllocated; ++i )
        {
            m_freeElements.push_back( element++ );
        }
    }

    template <class T, class A>
    void PoolData<T, A>::freeAllElements()
    {
        m_freeElements.clear();

        auto element = getData();
        for( size_t i = 0; i < m_numAllocated; ++i )
        {
            m_freeElements.push_back( element++ );
        }
    }

    template <class T, class A>
    T *PoolData<T, A>::getFreeElement()
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
    void PoolData<T, A>::setElementFree( T *element )
    {
        m_freeElements.push_back( element );
    }

    template <class T, class A>
    T *PoolData<T, A>::getData()
    {
        return m_data;
    }

    template <class T, class A>
    size_Num PoolData<T, A>::getNumAllocated() const
    {
        return m_numAllocated;
    }

    template <class T, class A>
    size_Num PoolData<T, A>::getNumFree() const
    {
        return m_freeElements.size();
    }

    template <class T, class A>
    T &PoolData<T, A>::operator[]( size_Num index )
    {
        WP_ASSERT( index < m_numAllocated );
        return m_data[index];
    }

    template <class T, class A>
    const T &PoolData<T, A>::operator[]( size_Num index ) const
    {
        WP_ASSERT( index < m_numAllocated );
        return m_data[index];
    }

    template <class T, class A>
    A &PoolData<T, A>::getAlloc()
    {
        return m_alloc;
    }

    template <class T, class A>
    const A &PoolData<T, A>::getAlloc() const
    {
        return m_alloc;
    }

}  // namespace workphone

#endif  // PoolData_h__
