#ifndef _WP_ConcurrentFixedArrayGrowable_h__
#define _WP_ConcurrentFixedArrayGrowable_h__

#include <Workphone/Core/FixedArrayGrowable.hpp>
#include <Workphone/Thread/SpinRWMutex.hpp>

namespace workphone
{
    /**
     * Inline, fixed-capacity array synchronized by SpinRWMutex.
     * Individual reads return copies; emplace_back returns no reference. Use
     * readLocked()/writeLocked() for references, iterators, data(), or compound
     * operations. A view also exposes the complete FixedArrayGrowable API via ->.
     * References and iterators from a view must not outlive its lock.
     *
     * The mutex is not recursive: while holding a view, use that view rather than
     * calling locking operations on its owner. Predicates, comparisons, and T's
     * operations execute under the lock and must not re-enter the container.
     * Copy/move assignment and swap lock both containers in address order.
     * The owner must outlive all views and concurrent operations.
     *
     * Container storage, locks, and snapshots do not allocate. Element operations
     * and exception handling may allocate. try_push_back/try_emplace_back wait for
     * the write lock and return false on capacity exhaustion, not lock contention.
     */
    template <class T, std::size_t N>
    class ConcurrentFixedArrayGrowable
    {
    public:
        using storage_type = FixedArrayGrowable<T, N>;
        using value_type = T;
        using size_type = typename storage_type::size_type;
        using difference_type = typename storage_type::difference_type;
        using mutex_type = SpinRWMutex;

        template <bool IsConst>
        class LockedView
        {
            friend class ConcurrentFixedArrayGrowable;
            using owner_type = typename std::conditional<IsConst,
                const ConcurrentFixedArrayGrowable, ConcurrentFixedArrayGrowable>::type;
            using array_type = typename std::conditional<IsConst, const storage_type,
                                                         storage_type>::type;
            owner_type *m_owner;

            explicit LockedView( owner_type &owner ) : m_owner( &owner )
            {
                if constexpr( IsConst )
                    m_owner->m_mutex.lock_shared();
                else
                    m_owner->m_mutex.lock();
            }
            void release() noexcept
            {
                if( m_owner )
                {
                    if constexpr( IsConst )
                        m_owner->m_mutex.unlock_shared();
                    else
                        m_owner->m_mutex.unlock();
                    m_owner = nullptr;
                }
            }

        public:
            using iterator = typename std::conditional<IsConst,
                typename storage_type::const_iterator, typename storage_type::iterator>::type;
            using const_iterator = typename storage_type::const_iterator;
            using reference = typename std::conditional<IsConst, const T &, T &>::type;
            using pointer = typename std::conditional<IsConst, const T *, T *>::type;

            LockedView( const LockedView & ) = delete;
            LockedView &operator=( const LockedView & ) = delete;
            LockedView( LockedView &&other ) noexcept : m_owner( other.m_owner )
            { other.m_owner = nullptr; }
            LockedView &operator=( LockedView &&other ) noexcept
            {
                if( this != &other )
                {
                    release();
                    m_owner = other.m_owner;
                    other.m_owner = nullptr;
                }
                return *this;
            }
            ~LockedView() { release(); }
            explicit operator bool() const noexcept { return m_owner != nullptr; }
            array_type &get() const noexcept { assert( m_owner ); return m_owner->m_values; }
            array_type *operator->() const noexcept { return std::addressof( get() ); }
            array_type &operator*() const noexcept { return get(); }
            iterator begin() const noexcept { return get().begin(); }
            iterator end() const noexcept { return get().end(); }
            const_iterator cbegin() const noexcept { return get().cbegin(); }
            const_iterator cend() const noexcept { return get().cend(); }
            auto rbegin() const noexcept { return get().rbegin(); }
            auto rend() const noexcept { return get().rend(); }
            auto crbegin() const noexcept { return get().crbegin(); }
            auto crend() const noexcept { return get().crend(); }
            size_type size() const noexcept { return get().size(); }
            static constexpr size_type capacity() noexcept { return N; }
            static constexpr size_type max_size() noexcept { return N; }
            bool empty() const noexcept { return get().empty(); }
            bool full() const noexcept { return get().full(); }
            reference at( size_type index ) const { return get().at( index ); }
            reference operator[]( size_type index ) const { return get()[index]; }
            reference front() const { return get().front(); }
            reference back() const { return get().back(); }
            pointer data() const noexcept { return get().data(); }

            template <class... Args>
            decltype(auto) emplace_back( Args &&...args )
            { return get().emplace_back( std::forward<Args>( args )... ); }
            template <class... Args>
            bool try_emplace_back( Args &&...args )
            { return get().try_emplace_back( std::forward<Args>( args )... ); }
            template <class U>
            void push_back( U &&value ) { get().push_back( std::forward<U>( value ) ); }
            template <class U>
            bool try_push_back( U &&value ) { return get().try_push_back( std::forward<U>( value ) ); }
            template <class... Args>
            decltype(auto) emplace( Args &&...args )
            { return get().emplace( std::forward<Args>( args )... ); }
            template <class... Args>
            decltype(auto) insert( Args &&...args )
            { return get().insert( std::forward<Args>( args )... ); }
            template <class... Args>
            decltype(auto) erase( Args &&...args )
            { return get().erase( std::forward<Args>( args )... ); }
            template <class... Args>
            void assign( Args &&...args ) { get().assign( std::forward<Args>( args )... ); }
            template <class... Args>
            void resize( Args &&...args ) { get().resize( std::forward<Args>( args )... ); }
            template <class Compare = std::less<T>>
            void sort( Compare comp = Compare() ) { get().sort( comp ); }
            template <bool C = IsConst, typename std::enable_if<!C, int>::type = 0>
            void clear() noexcept { get().clear(); }
            template <bool C = IsConst, typename std::enable_if<!C, int>::type = 0>
            void pop_back() { get().pop_back(); }
        };

        using ReadLockedView = LockedView<true>;
        using WriteLockedView = LockedView<false>;

        ConcurrentFixedArrayGrowable() = default;
        explicit ConcurrentFixedArrayGrowable( size_type count ) : m_values( count ) {}
        ConcurrentFixedArrayGrowable( size_type count, const T &value ) : m_values( count, value ) {}
        ConcurrentFixedArrayGrowable( std::initializer_list<T> values ) : m_values( values ) {}
        template <class InputIt,
                  typename std::enable_if<!std::is_integral<InputIt>::value, int>::type = 0>
        ConcurrentFixedArrayGrowable( InputIt first, InputIt last ) : m_values( first, last ) {}
        ConcurrentFixedArrayGrowable( const ConcurrentFixedArrayGrowable &other ) :
            m_values( other.snapshot() )
        {
        }
        ConcurrentFixedArrayGrowable( ConcurrentFixedArrayGrowable &&other ) :
            m_values( takeStorage( other ) )
        {
        }
        ConcurrentFixedArrayGrowable &operator=( const ConcurrentFixedArrayGrowable &other )
        {
            if( this != &other )
                withBothLocked( other, true, false, [&] { m_values = other.m_values; } );
            return *this;
        }
        ConcurrentFixedArrayGrowable &operator=( ConcurrentFixedArrayGrowable &&other )
        {
            if( this != &other )
                withBothLocked( other, true, true, [&] { m_values = std::move( other.m_values ); } );
            return *this;
        }
        ConcurrentFixedArrayGrowable &operator=( std::initializer_list<T> values )
        { assign( values ); return *this; }

        ReadLockedView readLocked() const & { return ReadLockedView( *this ); }
        ReadLockedView readLocked() const && = delete;
        WriteLockedView writeLocked() & { return WriteLockedView( *this ); }
        storage_type snapshot() const
        {
            SpinRWMutex::ScopedLock lock( m_mutex, false );
            return m_values;
        }
        static constexpr size_type capacity() noexcept { return N; }
        static constexpr size_type max_size() noexcept { return N; }
        size_type size() const
        { SpinRWMutex::ScopedLock lock( m_mutex, false ); return m_values.size(); }
        bool empty() const
        { SpinRWMutex::ScopedLock lock( m_mutex, false ); return m_values.empty(); }
        bool full() const
        { SpinRWMutex::ScopedLock lock( m_mutex, false ); return m_values.full(); }
        void reserve( size_type count ) const { m_values.reserve( count ); }
        void shrink_to_fit() noexcept {}
        T at( size_type index ) const
        { SpinRWMutex::ScopedLock lock( m_mutex, false ); return m_values.at( index ); }
        T operator[]( size_type index ) const { return at( index ); }
        T front() const
        { SpinRWMutex::ScopedLock lock( m_mutex, false ); return m_values.front(); }
        T back() const
        { SpinRWMutex::ScopedLock lock( m_mutex, false ); return m_values.back(); }
        bool try_at( size_type index, T &value ) const
        {
            SpinRWMutex::ScopedLock lock( m_mutex, false );
            if( index >= m_values.size() )
                return false;
            value = m_values[index];
            return true;
        }
        bool try_front( T &value ) const { return try_at( 0, value ); }
        bool try_back( T &value ) const
        {
            SpinRWMutex::ScopedLock lock( m_mutex, false );
            if( m_values.empty() )
                return false;
            value = m_values.back();
            return true;
        }
        void set( size_type index, const T &value )
        { SpinRWMutex::ScopedLock lock( m_mutex ); m_values.at( index ) = value; }
        void set( size_type index, T &&value )
        { SpinRWMutex::ScopedLock lock( m_mutex ); m_values.at( index ) = std::move( value ); }
        template <class... Args>
        void emplace_back( Args &&...args )
        { SpinRWMutex::ScopedLock lock( m_mutex ); m_values.emplace_back( std::forward<Args>( args )... ); }
        void push_back( const T &value ) { emplace_back( value ); }
        void push_back( T &&value ) { emplace_back( std::move( value ) ); }
        template <class... Args>
        bool try_emplace_back( Args &&...args )
        {
            SpinRWMutex::ScopedLock lock( m_mutex );
            return m_values.try_emplace_back( std::forward<Args>( args )... );
        }
        bool try_push_back( const T &value ) { return try_emplace_back( value ); }
        bool try_push_back( T &&value ) { return try_emplace_back( std::move( value ) ); }
        void pop_back() { SpinRWMutex::ScopedLock lock( m_mutex ); m_values.pop_back(); }
        bool try_pop_back( T &value )
        {
            SpinRWMutex::ScopedLock lock( m_mutex );
            if( m_values.empty() )
                return false;
            value = std::move_if_noexcept( m_values.back() );
            m_values.pop_back();
            return true;
        }
        void clear() { SpinRWMutex::ScopedLock lock( m_mutex ); m_values.clear(); }
        void resize( size_type count )
        { SpinRWMutex::ScopedLock lock( m_mutex ); m_values.resize( count ); }
        void resize( size_type count, const T &value )
        { SpinRWMutex::ScopedLock lock( m_mutex ); m_values.resize( count, value ); }
        void assign( size_type count, const T &value )
        { SpinRWMutex::ScopedLock lock( m_mutex ); m_values.assign( count, value ); }
        template <class InputIt,
                  typename std::enable_if<!std::is_integral<InputIt>::value, int>::type = 0>
        void assign( InputIt first, InputIt last )
        { SpinRWMutex::ScopedLock lock( m_mutex ); m_values.assign( first, last ); }
        void assign( std::initializer_list<T> values )
        { SpinRWMutex::ScopedLock lock( m_mutex ); m_values.assign( values ); }
        template <class... Args>
        void emplace( size_type index, Args &&...args )
        { SpinRWMutex::ScopedLock lock( m_mutex ); m_values.emplace( index, std::forward<Args>( args )... ); }
        void insert( size_type index, const T &value ) { emplace( index, value ); }
        void insert( size_type index, T &&value ) { emplace( index, std::move( value ) ); }
        void insert( size_type index, size_type count, const T &value )
        {
            SpinRWMutex::ScopedLock lock( m_mutex );
            m_values.insert( positionAt( index ), count, value );
        }
        template <class InputIt,
                  typename std::enable_if<!std::is_integral<InputIt>::value, int>::type = 0>
        void insert( size_type index, InputIt first, InputIt last )
        {
            SpinRWMutex::ScopedLock lock( m_mutex );
            m_values.insert( positionAt( index ), first, last );
        }
        void insert( size_type index, std::initializer_list<T> values )
        { insert( index, values.begin(), values.end() ); }
        void erase( size_type index )
        { SpinRWMutex::ScopedLock lock( m_mutex ); m_values.erase( index ); }
        void erase( size_type first, size_type last )
        {
            SpinRWMutex::ScopedLock lock( m_mutex );
            m_values.erase( positionAt( first ), positionAt( last ) );
        }
        template <class Pred>
        size_type find_if( Pred pred ) const
        { SpinRWMutex::ScopedLock lock( m_mutex, false ); return m_values.find_if( pred ); }
        template <class Compare = std::less<T>>
        void sort( Compare comp = Compare() )
        { SpinRWMutex::ScopedLock lock( m_mutex ); m_values.sort( comp ); }
        void swap( ConcurrentFixedArrayGrowable &other )
        {
            if( this != &other )
                withBothLocked( other, true, true, [&] { m_values.swap( other.m_values ); } );
        }
        friend void swap( ConcurrentFixedArrayGrowable &lhs, ConcurrentFixedArrayGrowable &rhs )
        { lhs.swap( rhs ); }
        friend bool operator==( const ConcurrentFixedArrayGrowable &lhs,
                                const ConcurrentFixedArrayGrowable &rhs )
        {
            if( &lhs == &rhs )
            {
                SpinRWMutex::ScopedLock lock( lhs.m_mutex, false );
                return lhs.m_values == rhs.m_values;
            }
            return lhs.withBothLocked( rhs, false, false,
                                       [&] { return lhs.m_values == rhs.m_values; } );
        }
        friend bool operator!=( const ConcurrentFixedArrayGrowable &lhs,
                                const ConcurrentFixedArrayGrowable &rhs )
        { return !(lhs == rhs); }

    private:
        mutable SpinRWMutex m_mutex;
        storage_type m_values;

        static storage_type takeStorage( ConcurrentFixedArrayGrowable &other )
        {
            SpinRWMutex::ScopedLock lock( other.m_mutex );
            return std::move( other.m_values );
        }
        typename storage_type::const_iterator positionAt( size_type index ) const
        {
            if( index > m_values.size() )
                throw std::out_of_range( "ConcurrentFixedArrayGrowable position out of range" );
            return m_values.cbegin() + static_cast<difference_type>( index );
        }
        template <class Function>
        decltype(auto) withBothLocked( const ConcurrentFixedArrayGrowable &other,
                                      bool writeThis, bool writeOther, Function function ) const
        {
            const bool thisFirst = std::less<const ConcurrentFixedArrayGrowable *>()( this, &other );
            const auto *first = thisFirst ? this : &other;
            const auto *second = thisFirst ? &other : this;
            SpinRWMutex::ScopedLock firstLock( first->m_mutex, thisFirst ? writeThis : writeOther );
            SpinRWMutex::ScopedLock secondLock( second->m_mutex, thisFirst ? writeOther : writeThis );
            return function();
        }
    };
}  // namespace workphone

#endif  // _WP_ConcurrentFixedArrayGrowable_h__
