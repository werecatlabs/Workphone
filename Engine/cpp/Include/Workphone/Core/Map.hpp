#ifndef WORKPHONE_MAP_HPP
#define WORKPHONE_MAP_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace workphone
{
    template <class Key, class Value>
    struct MapNode
    {
        using value_type = std::pair<const Key, Value>;
        using storage_type =
            typename std::aligned_storage<sizeof( value_type ), alignof( value_type )>::type;

        MapNode *parent = nullptr;
        MapNode *left = nullptr;
        MapNode *right = nullptr;

        // Used only while this node is not part of the tree.
        MapNode *freeNext = nullptr;

        int height = 1;
        bool hasValue = false;

        storage_type storage;

        value_type *value() noexcept
        {
            return std::launder( reinterpret_cast<value_type *>( &storage ) );
        }

        const value_type *value() const noexcept
        {
            return std::launder( reinterpret_cast<const value_type *>( &storage ) );
        }
    };

    template <class Key, class Value, class Compare, class A>
    class MapBase;

    template <class Key, class Value, class Compare, class A>
    class MapBaseConstIterator;

    /**
     * @brief Bidirectional iterator for MapBase.
     *
     * The iterator stores a direct node pointer. reserve() and automatic pool
     * growth allocate new slabs and therefore do not move existing nodes.
     */
    template <class Key, class Value, class Compare, class A>
    class MapBaseIterator
    {
    public:
        using map_type = MapBase<Key, Value, Compare, A>;
        using node_type = MapNode<Key, Value>;

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = std::pair<const Key, Value>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

        MapBaseIterator() noexcept = default;

        reference operator*() const
        {
            assert( m_node );
            return *m_node->value();
        }

        pointer operator->() const
        {
            assert( m_node );
            return m_node->value();
        }

        MapBaseIterator &operator++()
        {
            assert( m_owner );
            assert( m_node );
            m_node = map_type::successor( m_node );
            return *this;
        }

        MapBaseIterator operator++( int )
        {
            MapBaseIterator tmp( *this );
            ++( *this );
            return tmp;
        }

        MapBaseIterator &operator--()
        {
            assert( m_owner );

            if( m_node )
            {
                m_node = map_type::predecessor( m_node );
            }
            else
            {
                // --end()
                m_node = map_type::maximum( m_owner->m_root );
            }

            return *this;
        }

        MapBaseIterator operator--( int )
        {
            MapBaseIterator tmp( *this );
            --( *this );
            return tmp;
        }

        bool operator==( const MapBaseIterator &other ) const noexcept
        {
            return m_node == other.m_node && m_owner == other.m_owner;
        }

        bool operator!=( const MapBaseIterator &other ) const noexcept
        {
            return !( *this == other );
        }

    private:
        explicit MapBaseIterator( map_type *owner, node_type *node ) noexcept :
            m_owner( owner ),
            m_node( node )
        {
        }

        map_type *m_owner = nullptr;
        node_type *m_node = nullptr;

        friend class MapBase<Key, Value, Compare, A>;
        friend class MapBaseConstIterator<Key, Value, Compare, A>;
    };

    /**
     * @brief Const bidirectional iterator for MapBase.
     */
    template <class Key, class Value, class Compare, class A>
    class MapBaseConstIterator
    {
    public:
        using map_type = MapBase<Key, Value, Compare, A>;
        using node_type = MapNode<Key, Value>;

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = std::pair<const Key, Value>;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type *;
        using reference = const value_type &;

        MapBaseConstIterator() noexcept = default;

        MapBaseConstIterator( const MapBaseIterator<Key, Value, Compare, A> &other ) noexcept :
            m_owner( other.m_owner ),
            m_node( other.m_node )
        {
        }

        reference operator*() const
        {
            assert( m_node );
            return *m_node->value();
        }

        pointer operator->() const
        {
            assert( m_node );
            return m_node->value();
        }

        MapBaseConstIterator &operator++()
        {
            assert( m_owner );
            assert( m_node );
            m_node = map_type::successor( m_node );
            return *this;
        }

        MapBaseConstIterator operator++( int )
        {
            MapBaseConstIterator tmp( *this );
            ++( *this );
            return tmp;
        }

        MapBaseConstIterator &operator--()
        {
            assert( m_owner );

            if( m_node )
            {
                m_node = map_type::predecessor( m_node );
            }
            else
            {
                m_node = map_type::maximum( m_owner->m_root );
            }

            return *this;
        }

        MapBaseConstIterator operator--( int )
        {
            MapBaseConstIterator tmp( *this );
            --( *this );
            return tmp;
        }

        bool operator==( const MapBaseConstIterator &other ) const noexcept
        {
            return m_node == other.m_node && m_owner == other.m_owner;
        }

        bool operator!=( const MapBaseConstIterator &other ) const noexcept
        {
            return !( *this == other );
        }

    private:
        explicit MapBaseConstIterator( const map_type *owner, const node_type *node ) noexcept :
            m_owner( owner ),
            m_node( node )
        {
        }

        const map_type *m_owner = nullptr;
        const node_type *m_node = nullptr;

        friend class MapBase<Key, Value, Compare, A>;
    };

    /**
     * @brief Ordered associative map backed by an AVL tree and a slab node pool.
     *
     * This implementation owns its tree node layout, which means reserve() can
     * genuinely preallocate map nodes. std::map cannot provide a portable
     * reserve() because its internal tree node type is implementation-defined.
     *
     * Nodes are allocated in contiguous slabs. Existing nodes are never moved
     * when the pool grows, preserving references and iterators to non-erased
     * elements.
     */
    template <class Key, class Value, class Compare = std::less<Key>,
              class A = std::allocator<std::pair<const Key, Value>>>
    class MapBase
    {
    public:
        using allocator_type = A;
        using key_type = Key;
        using mapped_type = Value;
        using value_type = std::pair<const Key, Value>;
        using reference = value_type &;
        using const_reference = const value_type &;

        using allocator_traits = std::allocator_traits<allocator_type>;
        using difference_type = typename allocator_traits::difference_type;
        using size_type = typename allocator_traits::size_type;

        using iterator = MapBaseIterator<Key, Value, Compare, A>;
        using const_iterator = MapBaseConstIterator<Key, Value, Compare, A>;

    private:
        using node_type = MapNode<Key, Value>;
        using node_allocator_type = typename allocator_traits::template rebind_alloc<node_type>;
        using node_allocator_traits = std::allocator_traits<node_allocator_type>;

        struct Block
        {
            node_type *data = nullptr;
            size_type count = 0;
        };

    public:
        MapBase() : m_allocator(), m_nodeAllocator( m_allocator )
        {
        }

        explicit MapBase( const Compare &compare, const allocator_type &allocator = allocator_type() ) :
            m_compare( compare ),
            m_allocator( allocator ),
            m_nodeAllocator( m_allocator )
        {
        }

        /**
         * @brief Construct a map and immediately preallocate node storage.
         */
        explicit MapBase( size_type initialCapacity, GrowthPolicy growthPolicy = GrowthPolicy::Double,
                          size_type growthSize = 64, const Compare &compare = Compare(),
                          const allocator_type &allocator = allocator_type() ) :
            m_compare( compare ),
            m_allocator( allocator ),
            m_nodeAllocator( m_allocator ),
            m_growthPolicy( growthPolicy ),
            m_growthSize( growthSize )
        {
            validateGrowthSize();
            reserve( initialCapacity );
        }

        MapBase( std::initializer_list<value_type> il, const Compare &compare = Compare(),
                 const allocator_type &allocator = allocator_type() ) :
            m_compare( compare ),
            m_allocator( allocator ),
            m_nodeAllocator( m_allocator )
        {
            reserve( static_cast<size_type>( il.size() ) );

            try
            {
                insert( il.begin(), il.end() );
            }
            catch( ... )
            {
                clear();
                releaseBlocks();
                throw;
            }
        }

        MapBase( const MapBase &other ) :
            m_compare( other.m_compare ),
            m_allocator( allocator_traits::select_on_container_copy_construction( other.m_allocator ) ),
            m_nodeAllocator( m_allocator ),
            m_growthPolicy( other.m_growthPolicy ),
            m_growthSize( other.m_growthSize )
        {
            reserve( other.m_capacity );

            try
            {
                for( const auto &entry : other )
                {
                    insert( entry );
                }
            }
            catch( ... )
            {
                clear();
                releaseBlocks();
                throw;
            }
        }

        MapBase( MapBase &&other ) noexcept(
            std::is_nothrow_move_constructible<Compare>::value &&
            std::is_nothrow_move_constructible<allocator_type>::value &&
            std::is_nothrow_move_constructible<node_allocator_type>::value ) :
            m_compare( std::move( other.m_compare ) ),
            m_allocator( std::move( other.m_allocator ) ),
            m_nodeAllocator( std::move( other.m_nodeAllocator ) ),
            m_blocks( std::move( other.m_blocks ) ),
            m_root( other.m_root ),
            m_freeHead( other.m_freeHead ),
            m_size( other.m_size ),
            m_capacity( other.m_capacity ),
            m_growthPolicy( other.m_growthPolicy ),
            m_growthSize( other.m_growthSize )
        {
            other.m_root = nullptr;
            other.m_freeHead = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }

        ~MapBase()
        {
            clear();
            releaseBlocks();
        }

        MapBase &operator=( const MapBase &other )
        {
            if( this != &other )
            {
                MapBase tmp( other );
                swap( tmp );
            }

            return *this;
        }

        MapBase &operator=( MapBase &&other ) noexcept(
            noexcept( MapBase( std::move( other ) ) ) &&
            noexcept( std::declval<MapBase &>().swap( std::declval<MapBase &>() ) ) )
        {
            if( this != &other )
            {
                MapBase tmp( std::move( other ) );
                swap( tmp );
            }

            return *this;
        }

        void swap( MapBase &other ) noexcept(
            noexcept( std::swap( std::declval<Compare &>(), std::declval<Compare &>() ) ) &&
            noexcept( std::swap( std::declval<allocator_type &>(),
                                 std::declval<allocator_type &>() ) ) &&
            noexcept( std::swap( std::declval<node_allocator_type &>(),
                                 std::declval<node_allocator_type &>() ) ) )
        {
            using std::swap;

            swap( m_compare, other.m_compare );
            swap( m_allocator, other.m_allocator );
            swap( m_nodeAllocator, other.m_nodeAllocator );
            swap( m_blocks, other.m_blocks );
            swap( m_root, other.m_root );
            swap( m_freeHead, other.m_freeHead );
            swap( m_size, other.m_size );
            swap( m_capacity, other.m_capacity );
            swap( m_growthPolicy, other.m_growthPolicy );
            swap( m_growthSize, other.m_growthSize );
        }

        // --------------------------------------------------------------------
        // Capacity / pool control
        // --------------------------------------------------------------------

        [[nodiscard]]
        size_type size() const noexcept
        {
            return m_size;
        }

        [[nodiscard]]
        bool empty() const noexcept
        {
            return m_size == 0;
        }

        /**
         * @brief Number of tree nodes currently allocated in the pool.
         */
        [[nodiscard]]
        size_type capacity() const noexcept
        {
            return m_capacity;
        }

        /**
         * @brief Number of preallocated nodes currently available for insertion.
         */
        [[nodiscard]]
        size_type available() const noexcept
        {
            return m_capacity - m_size;
        }

        /**
         * @brief Preallocate enough node storage for at least requestedCapacity elements.
         *
         * Explicit reserve() is permitted for every growth policy, including Fixed.
         * Existing nodes and iterators are not invalidated.
         */
        void reserve( size_type requestedCapacity )
        {
            if( requestedCapacity <= m_capacity )
            {
                return;
            }

            addBlock( requestedCapacity - m_capacity );
        }

        [[nodiscard]]
        size_type max_size() const noexcept
        {
            return node_allocator_traits::max_size( m_nodeAllocator );
        }

        [[nodiscard]]
        GrowthPolicy getGrowthPolicy() const noexcept
        {
            return m_growthPolicy;
        }

        void setGrowthPolicy( GrowthPolicy policy ) noexcept
        {
            m_growthPolicy = policy;
        }

        [[nodiscard]]
        size_type getGrowthSize() const noexcept
        {
            return m_growthSize;
        }

        void setGrowthSize( size_type growthSize )
        {
            if( growthSize == 0 )
            {
                throw std::invalid_argument( "MapBase growth size cannot be zero." );
            }

            m_growthSize = growthSize;
        }

        // --------------------------------------------------------------------
        // Element access
        // --------------------------------------------------------------------

        mapped_type &operator[]( const key_type &key )
        {
            auto node = findNode( key );

            if( node )
            {
                return node->value()->second;
            }

            auto result = emplace( std::piecewise_construct, std::forward_as_tuple( key ),
                                   std::forward_as_tuple() );

            return result.first->second;
        }

        mapped_type &operator[]( key_type &&key )
        {
            auto node = findNode( key );

            if( node )
            {
                return node->value()->second;
            }

            auto result = emplace( std::piecewise_construct, std::forward_as_tuple( std::move( key ) ),
                                   std::forward_as_tuple() );

            return result.first->second;
        }

        mapped_type &at( const key_type &key )
        {
            auto node = findNode( key );

            if( !node )
            {
                throw std::out_of_range( "MapBase::at key not found." );
            }

            return node->value()->second;
        }

        const mapped_type &at( const key_type &key ) const
        {
            auto node = findNode( key );

            if( !node )
            {
                throw std::out_of_range( "MapBase::at key not found." );
            }

            return node->value()->second;
        }

        // --------------------------------------------------------------------
        // Insertion
        // --------------------------------------------------------------------

        std::pair<iterator, bool> insert( const value_type &value )
        {
            if( auto existing = findNode( value.first ) )
            {
                return { iterator( this, existing ), false };
            }

            return emplace( value );
        }

        std::pair<iterator, bool> insert( value_type &&value )
        {
            if( auto existing = findNode( value.first ) )
            {
                return { iterator( this, existing ), false };
            }

            return emplace( std::move( value ) );
        }

        template <class InputIt>
        void insert( InputIt first, InputIt last )
        {
            for( ; first != last; ++first )
            {
                insert( *first );
            }
        }

        template <typename... Args>
        std::pair<iterator, bool> emplace( Args &&...args )
        {
            // Check the key before consuming a pooled node.  In a fixed-capacity
            // map a duplicate insertion must be rejected even when every node is
            // occupied; acquiring first incorrectly reported capacity exhaustion.
            value_type candidate( std::forward<Args>( args )... );
            if( auto existing = findNode( candidate.first ) )
            {
                return { iterator( this, existing ), false };
            }

            node_type *node = acquireNode();

            try
            {
                allocator_traits::construct( m_allocator, node->value(), std::move( candidate ) );

                node->hasValue = true;
            }
            catch( ... )
            {
                releaseNode( node );
                throw;
            }

            try
            {
                const key_type &key = node->value()->first;

                node_type *parent = nullptr;
                node_type *current = m_root;
                bool insertLeft = false;

                while( current )
                {
                    parent = current;

                    if( m_compare( key, current->value()->first ) )
                    {
                        current = current->left;
                        insertLeft = true;
                    }
                    else if( m_compare( current->value()->first, key ) )
                    {
                        current = current->right;
                        insertLeft = false;
                    }
                    else
                    {
                        // The key was checked before acquiring the node. This is
                        // retained for comparators whose ordering can change at
                        // runtime, and releases the node before returning.
                        destroyNodeValue( node );
                        releaseNode( node );
                        return { iterator( this, current ), false };
                    }
                }

                node->parent = parent;
                node->left = nullptr;
                node->right = nullptr;
                node->height = 1;

                if( !parent )
                {
                    m_root = node;
                }
                else if( insertLeft )
                {
                    parent->left = node;
                }
                else
                {
                    parent->right = node;
                }

                ++m_size;
                rebalanceFrom( parent );

                return { iterator( this, node ), true };
            }
            catch( ... )
            {
                destroyNodeValue( node );
                releaseNode( node );
                throw;
            }
        }

        // --------------------------------------------------------------------
        // Lookup
        // --------------------------------------------------------------------

        iterator find( const key_type &key )
        {
            return iterator( this, findNode( key ) );
        }

        const_iterator find( const key_type &key ) const
        {
            return const_iterator( this, findNode( key ) );
        }

        size_type count( const key_type &key ) const
        {
            return findNode( key ) ? 1 : 0;
        }

        iterator lower_bound( const key_type &key )
        {
            return iterator( this, lowerBoundNode( key ) );
        }

        const_iterator lower_bound( const key_type &key ) const
        {
            return const_iterator( this, lowerBoundNode( key ) );
        }

        iterator upper_bound( const key_type &key )
        {
            return iterator( this, upperBoundNode( key ) );
        }

        const_iterator upper_bound( const key_type &key ) const
        {
            return const_iterator( this, upperBoundNode( key ) );
        }

        // --------------------------------------------------------------------
        // Modifiers
        // --------------------------------------------------------------------

        void clear() noexcept
        {
            destroySubtree( m_root );
            m_root = nullptr;
            m_size = 0;
        }

        iterator erase( const_iterator pos )
        {
            if( pos.m_owner != this || !pos.m_node )
            {
                return end();
            }

            node_type *node = const_cast<node_type *>( pos.m_node );
            node_type *next = successor( node );

            eraseNode( node );

            return iterator( this, next );
        }

        iterator erase( const_iterator first, const_iterator last )
        {
            while( first != last )
            {
                first = erase( first );
            }

            return iterator( this, const_cast<node_type *>( last.m_node ) );
        }

        size_type erase( const key_type &key )
        {
            node_type *node = findNode( key );

            if( !node )
            {
                return 0;
            }

            eraseNode( node );
            return 1;
        }

        // --------------------------------------------------------------------
        // Iterators
        // --------------------------------------------------------------------

        iterator begin() noexcept
        {
            return iterator( this, minimum( m_root ) );
        }

        const_iterator begin() const noexcept
        {
            return const_iterator( this, minimum( m_root ) );
        }

        const_iterator cbegin() const noexcept
        {
            return const_iterator( this, minimum( m_root ) );
        }

        iterator end() noexcept
        {
            return iterator( this, nullptr );
        }

        const_iterator end() const noexcept
        {
            return const_iterator( this, nullptr );
        }

        const_iterator cend() const noexcept
        {
            return const_iterator( this, nullptr );
        }

    private:
        // Iterator classes require access to the root for --end().
        friend class MapBaseIterator<Key, Value, Compare, A>;
        friend class MapBaseConstIterator<Key, Value, Compare, A>;

        void validateGrowthSize() const
        {
            if( m_growthSize == 0 )
            {
                throw std::invalid_argument( "MapBase growth size cannot be zero." );
            }
        }

        static int nodeHeight( const node_type *node ) noexcept
        {
            return node ? node->height : 0;
        }

        static void updateHeight( node_type *node ) noexcept
        {
            node->height = 1 + std::max( nodeHeight( node->left ), nodeHeight( node->right ) );
        }

        static int balanceFactor( const node_type *node ) noexcept
        {
            return nodeHeight( node->left ) - nodeHeight( node->right );
        }

        static node_type *minimum( node_type *node ) noexcept
        {
            if( !node )
            {
                return nullptr;
            }

            while( node->left )
            {
                node = node->left;
            }

            return node;
        }

        static const node_type *minimum( const node_type *node ) noexcept
        {
            if( !node )
            {
                return nullptr;
            }

            while( node->left )
            {
                node = node->left;
            }

            return node;
        }

        static node_type *maximum( node_type *node ) noexcept
        {
            if( !node )
            {
                return nullptr;
            }

            while( node->right )
            {
                node = node->right;
            }

            return node;
        }

        static const node_type *maximum( const node_type *node ) noexcept
        {
            if( !node )
            {
                return nullptr;
            }

            while( node->right )
            {
                node = node->right;
            }

            return node;
        }

        static node_type *successor( node_type *node ) noexcept
        {
            if( !node )
            {
                return nullptr;
            }

            if( node->right )
            {
                return minimum( node->right );
            }

            node_type *parent = node->parent;

            while( parent && node == parent->right )
            {
                node = parent;
                parent = parent->parent;
            }

            return parent;
        }

        static const node_type *successor( const node_type *node ) noexcept
        {
            return successor( const_cast<node_type *>( node ) );
        }

        static node_type *predecessor( node_type *node ) noexcept
        {
            if( !node )
            {
                return nullptr;
            }

            if( node->left )
            {
                return maximum( node->left );
            }

            node_type *parent = node->parent;

            while( parent && node == parent->left )
            {
                node = parent;
                parent = parent->parent;
            }

            return parent;
        }

        static const node_type *predecessor( const node_type *node ) noexcept
        {
            return predecessor( const_cast<node_type *>( node ) );
        }

        node_type *findNode( const key_type &key )
        {
            node_type *current = m_root;

            while( current )
            {
                if( m_compare( key, current->value()->first ) )
                {
                    current = current->left;
                }
                else if( m_compare( current->value()->first, key ) )
                {
                    current = current->right;
                }
                else
                {
                    return current;
                }
            }

            return nullptr;
        }

        const node_type *findNode( const key_type &key ) const
        {
            const node_type *current = m_root;

            while( current )
            {
                if( m_compare( key, current->value()->first ) )
                {
                    current = current->left;
                }
                else if( m_compare( current->value()->first, key ) )
                {
                    current = current->right;
                }
                else
                {
                    return current;
                }
            }

            return nullptr;
        }

        node_type *lowerBoundNode( const key_type &key )
        {
            node_type *current = m_root;
            node_type *result = nullptr;

            while( current )
            {
                if( !m_compare( current->value()->first, key ) )
                {
                    result = current;
                    current = current->left;
                }
                else
                {
                    current = current->right;
                }
            }

            return result;
        }

        const node_type *lowerBoundNode( const key_type &key ) const
        {
            const node_type *current = m_root;
            const node_type *result = nullptr;

            while( current )
            {
                if( !m_compare( current->value()->first, key ) )
                {
                    result = current;
                    current = current->left;
                }
                else
                {
                    current = current->right;
                }
            }

            return result;
        }

        node_type *upperBoundNode( const key_type &key )
        {
            node_type *current = m_root;
            node_type *result = nullptr;

            while( current )
            {
                if( m_compare( key, current->value()->first ) )
                {
                    result = current;
                    current = current->left;
                }
                else
                {
                    current = current->right;
                }
            }

            return result;
        }

        const node_type *upperBoundNode( const key_type &key ) const
        {
            const node_type *current = m_root;
            const node_type *result = nullptr;

            while( current )
            {
                if( m_compare( key, current->value()->first ) )
                {
                    result = current;
                    current = current->left;
                }
                else
                {
                    current = current->right;
                }
            }

            return result;
        }

        node_type *rotateLeft( node_type *x ) noexcept
        {
            node_type *y = x->right;
            assert( y );

            node_type *transfer = y->left;
            node_type *oldParent = x->parent;

            y->parent = oldParent;

            if( !oldParent )
            {
                m_root = y;
            }
            else if( oldParent->left == x )
            {
                oldParent->left = y;
            }
            else
            {
                oldParent->right = y;
            }

            y->left = x;
            x->parent = y;

            x->right = transfer;

            if( transfer )
            {
                transfer->parent = x;
            }

            updateHeight( x );
            updateHeight( y );

            return y;
        }

        node_type *rotateRight( node_type *y ) noexcept
        {
            node_type *x = y->left;
            assert( x );

            node_type *transfer = x->right;
            node_type *oldParent = y->parent;

            x->parent = oldParent;

            if( !oldParent )
            {
                m_root = x;
            }
            else if( oldParent->left == y )
            {
                oldParent->left = x;
            }
            else
            {
                oldParent->right = x;
            }

            x->right = y;
            y->parent = x;

            y->left = transfer;

            if( transfer )
            {
                transfer->parent = y;
            }

            updateHeight( y );
            updateHeight( x );

            return x;
        }

        void rebalanceFrom( node_type *node ) noexcept
        {
            while( node )
            {
                updateHeight( node );

                node_type *subtreeRoot = node;
                const int balance = balanceFactor( node );

                if( balance > 1 )
                {
                    if( balanceFactor( node->left ) < 0 )
                    {
                        rotateLeft( node->left );
                    }

                    subtreeRoot = rotateRight( node );
                }
                else if( balance < -1 )
                {
                    if( balanceFactor( node->right ) > 0 )
                    {
                        rotateRight( node->right );
                    }

                    subtreeRoot = rotateLeft( node );
                }

                node = subtreeRoot->parent;
            }
        }

        void transplant( node_type *oldNode, node_type *newNode ) noexcept
        {
            if( !oldNode->parent )
            {
                m_root = newNode;
            }
            else if( oldNode == oldNode->parent->left )
            {
                oldNode->parent->left = newNode;
            }
            else
            {
                oldNode->parent->right = newNode;
            }

            if( newNode )
            {
                newNode->parent = oldNode->parent;
            }
        }

        void eraseNode( node_type *node )
        {
            assert( node );
            assert( node->hasValue );

            node_type *rebalanceStart = nullptr;

            if( !node->left )
            {
                rebalanceStart = node->parent;
                transplant( node, node->right );
            }
            else if( !node->right )
            {
                rebalanceStart = node->parent;
                transplant( node, node->left );
            }
            else
            {
                node_type *replacement = minimum( node->right );

                if( replacement->parent != node )
                {
                    node_type *replacementOldParent = replacement->parent;

                    transplant( replacement, replacement->right );

                    replacement->right = node->right;
                    replacement->right->parent = replacement;

                    transplant( node, replacement );

                    replacement->left = node->left;
                    replacement->left->parent = replacement;

                    updateHeight( replacement );
                    rebalanceStart = replacementOldParent;
                }
                else
                {
                    transplant( node, replacement );

                    replacement->left = node->left;
                    replacement->left->parent = replacement;

                    updateHeight( replacement );
                    rebalanceStart = replacement;
                }
            }

            destroyNodeValue( node );
            releaseNode( node );
            --m_size;

            if( rebalanceStart )
            {
                rebalanceFrom( rebalanceStart );
            }
            else if( m_root )
            {
                // Root replacement with no parent. Usually no rotation is needed,
                // but refreshing its cached height keeps the invariant explicit.
                updateHeight( m_root );
            }
        }

        void destroySubtree( node_type *node ) noexcept
        {
            if( !node )
            {
                return;
            }

            destroySubtree( node->left );
            destroySubtree( node->right );

            destroyNodeValue( node );
            releaseNode( node );
        }

        template <class... Args>
        void constructNodeValue( node_type *node, Args &&...args )
        {
            allocator_traits::construct( m_allocator, node->value(), std::forward<Args>( args )... );

            node->hasValue = true;
        }

        void destroyNodeValue( node_type *node ) noexcept
        {
            if( node->hasValue )
            {
                allocator_traits::destroy( m_allocator, node->value() );
                node->hasValue = false;
            }
        }

        node_type *acquireNode()
        {
            if( !m_freeHead )
            {
                grow();
            }

            assert( m_freeHead );

            node_type *node = m_freeHead;
            m_freeHead = node->freeNext;

            node->parent = nullptr;
            node->left = nullptr;
            node->right = nullptr;
            node->freeNext = nullptr;
            node->height = 1;
            node->hasValue = false;

            return node;
        }

        void releaseNode( node_type *node ) noexcept
        {
            node->parent = nullptr;
            node->left = nullptr;
            node->right = nullptr;
            node->height = 1;
            node->hasValue = false;

            node->freeNext = m_freeHead;
            m_freeHead = node;
        }

        void grow()
        {
            validateGrowthSize();

            switch( m_growthPolicy )
            {
            case GrowthPolicy::Fixed:
                throw std::length_error( "MapBase fixed capacity exhausted." );

            case GrowthPolicy::Grow:
                addBlock( m_growthSize );
                break;

            case GrowthPolicy::Double:
                addBlock( m_capacity == 0 ? m_growthSize : m_capacity );
                break;

            default:
                throw std::logic_error( "MapBase has an invalid pool growth policy." );
            }
        }

        void addBlock( size_type count )
        {
            if( count == 0 )
            {
                return;
            }

            const size_type maximum = max_size();

            if( count > maximum - m_capacity )
            {
                throw std::length_error( "MapBase capacity exceeds max_size()." );
            }

            node_type *data = node_allocator_traits::allocate( m_nodeAllocator, count );
            size_type constructed = 0;

            try
            {
                for( ; constructed < count; ++constructed )
                {
                    node_allocator_traits::construct( m_nodeAllocator, data + constructed );
                }

                m_blocks.push_back( Block{ data, count } );
            }
            catch( ... )
            {
                while( constructed > 0 )
                {
                    --constructed;
                    node_allocator_traits::destroy( m_nodeAllocator, data + constructed );
                }

                node_allocator_traits::deallocate( m_nodeAllocator, data, count );
                throw;
            }

            // Push in reverse order so sequential acquisitions walk forward
            // through the slab: data[0], data[1], data[2], ...
            for( size_type i = count; i > 0; --i )
            {
                node_type *node = data + ( i - 1 );
                node->freeNext = m_freeHead;
                m_freeHead = node;
            }

            m_capacity += count;
        }

        void releaseBlocks() noexcept
        {
            for( auto &block : m_blocks )
            {
                for( size_type i = 0; i < block.count; ++i )
                {
                    node_allocator_traits::destroy( m_nodeAllocator, block.data + i );
                }

                node_allocator_traits::deallocate( m_nodeAllocator, block.data, block.count );
            }

            m_blocks.clear();
            m_freeHead = nullptr;
            m_capacity = 0;
        }

        Compare m_compare{};

        allocator_type m_allocator{};
        node_allocator_type m_nodeAllocator;

        std::vector<Block> m_blocks;

        node_type *m_root = nullptr;
        node_type *m_freeHead = nullptr;

        size_type m_size = 0;
        size_type m_capacity = 0;

        GrowthPolicy m_growthPolicy = GrowthPolicy::Double;
        size_type m_growthSize = 64;
    };

    template <class Key, class Value, class Compare, class A>
    void swap( MapBase<Key, Value, Compare, A> &a,
               MapBase<Key, Value, Compare, A> &b ) noexcept( noexcept( a.swap( b ) ) )
    {
        a.swap( b );
    }

    /**
     * @brief Engine map alias.
     *
     * Unlike the old alias, this now points at MapBase so callers receive the
     * pooled/preallocation behaviour.
     */
    template <class Key, class Value, class Compare = std::less<Key>,
              class A = std::allocator<std::pair<const Key, Value>>>
    using Map = MapBase<Key, Value, Compare, A>;

}  // namespace workphone

#endif  // WORKPHONE_MAP_HPP
