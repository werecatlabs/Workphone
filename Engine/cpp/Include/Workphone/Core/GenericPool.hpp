#ifndef __GenericPool_H
#define __GenericPool_H

#include <Workphone/Core/Allocator.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/GenericPoolData.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/System/RttiClass.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace workphone
{
    /**
     * @brief Thread-safe generic memory pool.
     *
     * GenericPool allocates objects in blocks and keeps a concurrent
     * queue containing pointers to currently free elements.
     *
     * Copies share the same synchronized pool state. This is important because
     * the free queue contains pointers into the allocated blocks; copying the
     * blocks and queue separately would make those pointers invalid or allow
     * the same slot to be allocated more than once.
     *
     * @tparam T Type stored by the pool.
     * @tparam A Allocator used to allocate T.
     */
    template <class T, class A = Allocator<T>>
    class GenericPool
    {
    public:
        using value_type = T;
        using allocator_type = A;
        using size_type = size_Num;
        using pointer = T *;
        using const_pointer = const T *;

        /**
         * @brief Rebind this pool to another element type.
         *
         * Importantly, the underlying allocator is also rebound.
         *
         * GenericPool<T, Allocator<T>>
         *
         * becomes:
         *
         * GenericPool<U, Allocator<U>>
         */
        template <class U>
        struct rebind
        {
            using AllocatorType = typename std::allocator_traits<A>::template rebind_alloc<U>;

            using other = GenericPool<U, AllocatorType>;
        };

        /**
         * @brief Default constructor.
         *
         * Uses a default growth size of 32 elements.
         */
        GenericPool();

        /**
         * @brief Construct with a particular growth size.
         *
         * @param nextSize Number of objects allocated each time
         *                 the pool needs to grow.
         */
        explicit GenericPool( u32 nextSize );

        /**
         * @brief Converting constructor used by allocator rebinding.
         *
         * Pools with different value types cannot share storage, so this
         * constructor copies only the growth configuration.
         *
         * Only configuration such as the growth size is copied.
         *
         * For example:
         *
         * GenericPool<Foo, Allocator<Foo>>
         *
         * can be used to construct:
         *
         * GenericPool<Bar, Allocator<Bar>>
         */
        template <class U, class B>
        GenericPool( const GenericPool<U, B> &other );

        GenericPool( const GenericPool &other ) = default;
        GenericPool &operator=( const GenericPool &other ) = default;
        GenericPool( GenericPool &&other ) noexcept = default;
        GenericPool &operator=( GenericPool &&other ) noexcept = default;

        /**
         * @brief Destructor.
         *
         * Releases all allocated pool blocks.
         */
        ~GenericPool() = default;

        /**
         * @brief Allocate another block of pool storage.
         *
         * The number of objects allocated is controlled by getNextSize().
         */
        void allocateData();

        /**
         * @brief Obtain a free object from the pool.
         *
         * The pool automatically grows if there are currently no
         * free objects.
         *
         * @return Pointer to a free T object, or nullptr if allocation fails.
         */
        RawPtr<T> allocate_object();

        /**
         * @brief Return an object to the pool.
         *
         * @param ptr Object previously obtained from allocate_object().
         */
        void free_object( T *ptr );

        /**
         * @brief Release all currently allocated pool blocks.
         */
        void clear();

        /**
         * @brief Return the number of elements allocated when the pool grows.
         */
        u32 getNextSize() const;

        /**
         * @brief Change the number of elements allocated when the pool grows.
         */
        void setNextSize( u32 nextSize );

        /**
         * @brief Number of currently allocated pool blocks.
         */
        u32 getSize() const;

    private:
        struct PoolState
        {
            explicit PoolState( u32 nextSize = 32 ) : nextSize( nextSize )
            {
            }

            // The queue is declared after the blocks so it is destroyed first.
            Array<SharedPtr<GenericPoolData<T, A>>> elements;
            ConcurrentQueue<RawPtr<T>> freeElements;
            atomic_u32 nextSize;
            mutable RecursiveSpinMutex mutex;
        };

        std::shared_ptr<PoolState> m_state;
    };

    // -------------------------------------------------------------------------
    // GenericPool
    // -------------------------------------------------------------------------

    template <class T, class A>
    GenericPool<T, A>::GenericPool() : m_state( std::make_shared<PoolState>() )
    {
    }

    template <class T, class A>
    GenericPool<T, A>::GenericPool( u32 nextSize ) : m_state( std::make_shared<PoolState>( nextSize ) )
    {
    }

    /**
     * Converting constructor.
     *
     * This is the important constructor for the compiler error you were
     * seeing.
     *
     * A GenericPool<U> has completely separate storage from GenericPool<T>
     * because their element and allocator types differ.
     */
    template <class T, class A>
    template <class U, class B>
    GenericPool<T, A>::GenericPool( const GenericPool<U, B> &other ) :
        m_state( std::make_shared<PoolState>( other.getNextSize() ) )
    {
    }

    template <class T, class A>
    void GenericPool<T, A>::allocateData()
    {
        auto state = m_state;
        ScopedLock lock( &state->mutex );

        const auto nextSize = getNextSize();

        if( nextSize == 0 )
        {
            return;
        }

        auto element = workphone::make_shared<GenericPoolData<T, A>>();

        element->setNumElements( nextSize );

        /*
         * setNumElements() currently performs the allocation itself.
         *
         * Calling load() afterwards is harmless because GenericPoolData::load()
         * checks whether m_data already exists before allocating.
         */
        element->load( nullptr );

        state->elements.push_back( element );

        const auto numAllocated = element->getNumAllocated();

        for( size_t i = 0; i < numAllocated; ++i )
        {
            auto ptr = &( ( *element )[i] );

            state->freeElements.push( ptr );
        }
    }

    template <class T, class A>
    RawPtr<T> GenericPool<T, A>::allocate_object()
    {
        /*
         * First try the common/fast case without taking the pool mutex.
         */
        RawPtr<T> ptr = nullptr;

        auto state = m_state;

        if( state->freeElements.try_pop( ptr ) )
        {
            return ptr;
        }

        /*
         * Nothing free, so grow the pool.
         */
        allocateData();

        /*
         * allocateData() has populated the state's free-element queue.
         *
         * Because this is a concurrent pool, another thread may obtain an
         * element before this thread does. Keep trying while the queue
         * reports that elements exist.
         */
        while( !state->freeElements.empty() )
        {
            if( state->freeElements.try_pop( ptr ) )
            {
                return ptr;
            }

            Thread::yield();
        }

        return nullptr;
    }

    template <class T, class A>
    void GenericPool<T, A>::free_object( T *ptr )
    {
        if( ptr )
        {
            m_state->freeElements.push( ptr );
        }
    }

    template <class T, class A>
    void GenericPool<T, A>::clear()
    {
        auto state = m_state;
        ScopedLock lock( &state->mutex );

        /*
         * Remove every pointer from the free-element queue first.
         *
         * Those pointers become invalid as soon as the pool blocks are destroyed.
         */
        RawPtr<T> ptr = nullptr;

        while( state->freeElements.try_pop( ptr ) )
        {
            // Intentionally empty.
        }

        state->freeElements.clear();

        /*
         * GenericPoolData owns its allocation and its destructor calls
         * unload(), so dropping the SharedPtr is enough.
         *
         * There is no need to call:
         *
         *     element->unload(nullptr);
         *
         * manually here.
         */
        for( auto &element : state->elements )
        {
            element = nullptr;
        }

        state->elements.clear();
    }

    template <class T, class A>
    u32 GenericPool<T, A>::getNextSize() const
    {
        return m_state->nextSize;
    }

    template <class T, class A>
    void GenericPool<T, A>::setNextSize( u32 nextSize )
    {
        m_state->nextSize = nextSize;
    }

    template <class T, class A>
    u32 GenericPool<T, A>::getSize() const
    {
        auto state = m_state;
        ScopedLock lock( &state->mutex );

        return static_cast<u32>( state->elements.size() );
    }

}  // namespace workphone

#endif  // __GenericPool_H
