#ifndef List_h__
#define List_h__

#include <Workphone/WorkphoneEnums.hpp>
#include <list>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace workphone
{

    template <class T>
    class ListBase
    {
    private:
        using Storage = std::aligned_storage_t<sizeof( T ), alignof( T )>;

        struct Node
        {
            /*
             * A live node needs:
             *
             *      prev
             *      next
             *
             * A free node only needs:
             *
             *      freeNext
             *
             * Therefore prev and freeNext can share storage.
             */
            union
            {
                Node *prev;
                Node *freeNext;
            };

            Node *next = nullptr;

            Storage storage;

            Node() noexcept : freeNext( nullptr ), next( nullptr )
            {
            }

            T *getValue() noexcept
            {
                return std::launder( reinterpret_cast<T *>( &storage ) );
            }

            const T *getValue() const noexcept
            {
                return std::launder( reinterpret_cast<const T *>( &storage ) );
            }
        };

    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T &;
        using const_reference = const T &;

        template <bool IsConst>
        class BasicIterator
        {
        private:
            using NodePtr = std::conditional_t<IsConst, const Node *, Node *>;

            NodePtr m_node = nullptr;

            explicit BasicIterator( NodePtr node ) noexcept : m_node( node )
            {
            }

            friend class ListBase;

            template <bool>
            friend class BasicIterator;

        public:
            using iterator_category = std::bidirectional_iterator_tag;

            using value_type = T;
            using difference_type = std::ptrdiff_t;

            using pointer = std::conditional_t<IsConst, const T *, T *>;

            using reference = std::conditional_t<IsConst, const T &, T &>;

            BasicIterator() noexcept = default;

            /*
             * Allow:
             *
             * iterator -> const_iterator
             *
             * but not the reverse.
             */
            template <bool B = IsConst, typename = std::enable_if_t<B>>
            BasicIterator( const BasicIterator<false> &other ) noexcept : m_node( other.m_node )
            {
            }

            reference operator*() const
            {
                assert( m_node );
                return *m_node->getValue();
            }

            pointer operator->() const
            {
                assert( m_node );
                return m_node->getValue();
            }

            BasicIterator &operator++()
            {
                assert( m_node );

                m_node = m_node->next;

                return *this;
            }

            BasicIterator operator++( int )
            {
                BasicIterator tmp( *this );

                ++( *this );

                return tmp;
            }

            BasicIterator &operator--()
            {
                assert( m_node );

                m_node = m_node->prev;

                return *this;
            }

            BasicIterator operator--( int )
            {
                BasicIterator tmp( *this );

                --( *this );

                return tmp;
            }

            template <bool B>
            bool operator==( const BasicIterator<B> &other ) const noexcept
            {
                return m_node == other.m_node;
            }

            template <bool B>
            bool operator!=( const BasicIterator<B> &other ) const noexcept
            {
                return m_node != other.m_node;
            }
        };

        using iterator = BasicIterator<false>;
        using const_iterator = BasicIterator<true>;

        /**
         * Default:
         *
         * - no initial allocation
         * - doubles capacity when necessary
         * - starts with 64 nodes
         */
        ListBase() noexcept
        {
            initialiseSentinel();
        }

        /**
         * Create a pool with an initial capacity.
         */
        explicit ListBase( size_type initialCapacity, GrowthPolicy growthPolicy = GrowthPolicy::Double,
                           size_type growthSize = 64 ) :
            m_growthPolicy( growthPolicy ),
            m_growthSize( growthSize )
        {
            initialiseSentinel();

            if( initialCapacity > 0 )
            {
                reserve( initialCapacity );
            }
        }

        ListBase( const ListBase &other ) :
            m_growthPolicy( other.m_growthPolicy ),
            m_growthSize( other.m_growthSize )
        {
            initialiseSentinel();

            try
            {
                /*
                 * Preserve the capacity rather than only the size.
                 *
                 * This makes copying a pre-reserved list retain
                 * its allocation characteristics.
                 */
                reserve( other.capacity() );

                for( const auto &value : other )
                {
                    emplace_back( value );
                }
            }
            catch( ... )
            {
                clear();
                throw;
            }
        }

        ListBase( ListBase &&other ) noexcept :
            m_growthPolicy( other.m_growthPolicy ),
            m_growthSize( other.m_growthSize )
        {
            initialiseSentinel();

            moveFrom( std::move( other ) );
        }

        ~ListBase()
        {
            clear();
        }

        ListBase &operator=( const ListBase &other )
        {
            if( this == &other )
            {
                return *this;
            }

            /*
             * First destroy our current elements.
             *
             * Existing pool memory is retained and reused.
             */
            clear();

            m_growthPolicy = other.m_growthPolicy;
            m_growthSize = other.m_growthSize;

            reserve( other.capacity() );

            try
            {
                for( const auto &value : other )
                {
                    emplace_back( value );
                }
            }
            catch( ... )
            {
                clear();
                throw;
            }

            return *this;
        }

        ListBase &operator=( ListBase &&other ) noexcept
        {
            if( this == &other )
            {
                return *this;
            }

            clear();

            /*
             * Release our slabs before taking ownership
             * of the other list.
             */
            m_blocks.clear();

            m_freeHead = nullptr;
            m_size = 0;
            m_capacity = 0;

            m_growthPolicy = other.m_growthPolicy;
            m_growthSize = other.m_growthSize;

            moveFrom( std::move( other ) );

            return *this;
        }

        // ------------------------------------------------------------
        // Capacity
        // ------------------------------------------------------------

        [[nodiscard]]
        bool empty() const noexcept
        {
            return m_size == 0;
        }

        [[nodiscard]]
        size_type size() const noexcept
        {
            return m_size;
        }

        /**
         * Number of nodes currently allocated.
         *
         * This is analogous to vector::capacity().
         */
        [[nodiscard]]
        size_type capacity() const noexcept
        {
            return m_capacity;
        }

        /**
         * Number of currently unused allocated nodes.
         */
        [[nodiscard]]
        size_type available() const noexcept
        {
            return m_capacity - m_size;
        }

        /**
         * Explicitly reserve space for at least
         * requestedCapacity elements.
         *
         * reserve() is ALWAYS permitted, even when using
         * GrowthPolicy::Fixed.
         *
         * Fixed only disables automatic growth.
         */
        void reserve( size_type requestedCapacity )
        {
            if( requestedCapacity <= m_capacity )
            {
                return;
            }

            const size_type additionalNodes = requestedCapacity - m_capacity;

            addBlock( additionalNodes );
        }

        // ------------------------------------------------------------
        // Growth policy
        // ------------------------------------------------------------

        [[nodiscard]]
        GrowthPolicy getGrowthPolicy() const noexcept
        {
            return m_growthPolicy;
        }

        void setGrowthPolicy( GrowthPolicy growthPolicy ) noexcept
        {
            m_growthPolicy = growthPolicy;
        }

        /**
         * Number of nodes added by GrowthPolicy::Grow.
         *
         * It also controls the initial allocation made by
         * Double when the list currently has zero capacity.
         */
        [[nodiscard]]
        size_type getGrowthSize() const noexcept
        {
            return m_growthSize;
        }

        void setGrowthSize( size_type growthSize )
        {
            if( growthSize == 0 )
            {
                throw std::invalid_argument( "PoolList growth size cannot be zero." );
            }

            m_growthSize = growthSize;
        }

        // ------------------------------------------------------------
        // Iterators
        // ------------------------------------------------------------

        iterator begin() noexcept
        {
            return iterator( m_sentinel.next );
        }

        const_iterator begin() const noexcept
        {
            return const_iterator( m_sentinel.next );
        }

        const_iterator cbegin() const noexcept
        {
            return const_iterator( m_sentinel.next );
        }

        iterator end() noexcept
        {
            return iterator( &m_sentinel );
        }

        const_iterator end() const noexcept
        {
            return const_iterator( &m_sentinel );
        }

        const_iterator cend() const noexcept
        {
            return const_iterator( &m_sentinel );
        }

        // ------------------------------------------------------------
        // Element access
        // ------------------------------------------------------------

        reference front()
        {
            assert( !empty() );

            return *m_sentinel.next->getValue();
        }

        const_reference front() const
        {
            assert( !empty() );

            return *m_sentinel.next->getValue();
        }

        reference back()
        {
            assert( !empty() );

            return *m_sentinel.prev->getValue();
        }

        const_reference back() const
        {
            assert( !empty() );

            return *m_sentinel.prev->getValue();
        }

        // ------------------------------------------------------------
        // Insertion
        // ------------------------------------------------------------

        template <class... Args>
        iterator emplace( const_iterator position, Args &&...args )
        {
            Node *next = const_cast<Node *>( position.m_node );

            Node *node = acquireNode();

            try
            {
                ::new( static_cast<void *>( &node->storage ) ) T( std::forward<Args>( args )... );
            }
            catch( ... )
            {
                releaseNode( node );
                throw;
            }

            Node *prev = next->prev;

            node->prev = prev;
            node->next = next;

            prev->next = node;
            next->prev = node;

            ++m_size;

            return iterator( node );
        }

        iterator insert( const_iterator position, const T &value )
        {
            return emplace( position, value );
        }

        iterator insert( const_iterator position, T &&value )
        {
            return emplace( position, std::move( value ) );
        }

        template <class... Args>
        reference emplace_back( Args &&...args )
        {
            iterator it = emplace( cend(), std::forward<Args>( args )... );

            return *it;
        }

        template <class... Args>
        reference emplace_front( Args &&...args )
        {
            iterator it = emplace( cbegin(), std::forward<Args>( args )... );

            return *it;
        }

        void push_back( const T &value )
        {
            emplace_back( value );
        }

        void push_back( T &&value )
        {
            emplace_back( std::move( value ) );
        }

        void push_front( const T &value )
        {
            emplace_front( value );
        }

        void push_front( T &&value )
        {
            emplace_front( std::move( value ) );
        }

        // ------------------------------------------------------------
        // Removal
        // ------------------------------------------------------------

        iterator erase( const_iterator position )
        {
            Node *node = const_cast<Node *>( position.m_node );

            assert( node != &m_sentinel );

            Node *next = node->next;
            Node *prev = node->prev;

            prev->next = next;
            next->prev = prev;

            node->getValue()->~T();

            releaseNode( node );

            --m_size;

            return iterator( next );
        }

        iterator erase( const_iterator first, const_iterator last )
        {
            while( first != last )
            {
                first = erase( first );
            }

            return iterator( const_cast<Node *>( last.m_node ) );
        }

        void pop_front()
        {
            assert( !empty() );

            erase( const_iterator( m_sentinel.next ) );
        }

        void pop_back()
        {
            assert( !empty() );

            erase( const_iterator( m_sentinel.prev ) );
        }

        void clear() noexcept
        {
            Node *node = m_sentinel.next;

            while( node != &m_sentinel )
            {
                Node *next = node->next;

                node->getValue()->~T();

                releaseNode( node );

                node = next;
            }

            initialiseSentinel();

            m_size = 0;
        }

    private:
        // ------------------------------------------------------------
        // Pool management
        // ------------------------------------------------------------

        void initialiseSentinel() noexcept
        {
            m_sentinel.prev = &m_sentinel;
            m_sentinel.next = &m_sentinel;
        }

        /**
         * Allocate a contiguous slab of Nodes.
         *
         * Existing slabs never move, so existing elements,
         * references and iterators remain valid.
         */
        void addBlock( size_type count )
        {
            if( count == 0 )
            {
                return;
            }

            auto block = std::make_unique<Node[]>( count );

            Node *nodes = block.get();

            /*
             * Transfer ownership before linking the nodes
             * into the free list.
             *
             * If vector::push_back throws, block is still
             * owned locally and is destroyed correctly.
             */
            m_blocks.push_back( std::move( block ) );

            /*
             * Add nodes in reverse order.
             *
             * This causes acquireNode() to return:
             *
             * nodes[0]
             * nodes[1]
             * nodes[2]
             * ...
             *
             * which means sequential insertions tend to use
             * sequential memory locations.
             */
            for( size_type i = count; i > 0; --i )
            {
                Node *node = &nodes[i - 1];

                node->freeNext = m_freeHead;
                m_freeHead = node;
            }

            m_capacity += count;
        }

        Node *acquireNode()
        {
            if( !m_freeHead )
            {
                grow();
            }

            /*
             * grow() either provides storage or throws.
             */
            assert( m_freeHead );

            Node *node = m_freeHead;

            m_freeHead = node->freeNext;

            return node;
        }

        void releaseNode( Node *node ) noexcept
        {
            assert( node );

            node->freeNext = m_freeHead;
            m_freeHead = node;
        }

        /**
         * Automatic pool growth.
         */
        void grow()
        {
            switch( m_growthPolicy )
            {
            case GrowthPolicy::Fixed:
            {
                /*
                 * This is intentional.
                 *
                 * A Fixed pool guarantees that insertion
                 * never performs an allocation.
                 */
                throw std::length_error( "PoolList fixed capacity exhausted." );
            }

            case GrowthPolicy::Grow:
            {
                if( m_growthSize == 0 )
                {
                    throw std::length_error( "PoolList growth size is zero." );
                }

                addBlock( m_growthSize );
                break;
            }

            case GrowthPolicy::Double:
            {
                size_type growth = 0;

                if( m_capacity == 0 )
                {
                    growth = m_growthSize;
                }
                else
                {
                    growth = m_capacity;
                }

                if( growth == 0 )
                {
                    throw std::length_error( "PoolList growth size is zero." );
                }

                addBlock( growth );
                break;
            }

            default:
            {
                throw std::logic_error( "Invalid PoolList growth policy." );
            }
            }
        }

        void moveFrom( ListBase &&other ) noexcept
        {
            m_blocks = std::move( other.m_blocks );

            m_freeHead = other.m_freeHead;
            m_capacity = other.m_capacity;
            m_size = other.m_size;

            if( other.m_size != 0 )
            {
                m_sentinel.next = other.m_sentinel.next;

                m_sentinel.prev = other.m_sentinel.prev;

                /*
                 * The first and last node previously pointed
                 * to other's sentinel.
                 *
                 * Redirect them to ours.
                 */
                m_sentinel.next->prev = &m_sentinel;

                m_sentinel.prev->next = &m_sentinel;
            }
            else
            {
                initialiseSentinel();
            }

            other.m_freeHead = nullptr;
            other.m_capacity = 0;
            other.m_size = 0;

            other.initialiseSentinel();
        }

        Node m_sentinel;

        /*
         * Each entry owns one contiguous slab.
         *
         * Growing this vector does NOT move the actual nodes.
         */
        std::vector<std::unique_ptr<Node[]>> m_blocks;

        Node *m_freeHead = nullptr;

        size_type m_size = 0;
        size_type m_capacity = 0;

        GrowthPolicy m_growthPolicy = GrowthPolicy::Default;

        size_type m_growthSize = 64;
    };

    template <class _Ty>
    using List = ListBase<_Ty>;

}  // namespace workphone

#endif  // List_h__
