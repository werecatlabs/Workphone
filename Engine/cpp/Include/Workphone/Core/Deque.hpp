#ifndef __Deque_h__
#define __Deque_h__

#include <Workphone/Core/Allocator.hpp>
#include <vector>
#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <utility>

namespace workphone
{
    /**
     * @brief Double-ended queue facade backed by Workphone Array.
     *
     * This removes the Deque storage dependency while preserving the small
     * std-deque-like API used by Workphone. Front insertion/removal shifts the
     * underlying array, so prefer push_back/pop_back for hot paths.
     */
    template <class T, class A = Allocator<T>>
    class Deque
    {
    public:
        using allocator_type = A;
        using container_type = std::vector<T, A>;
        using value_type = T;
        using reference = T &;
        using const_reference = const T &;
        using pointer = T *;
        using const_pointer = const T *;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;
        using reverse_iterator = typename container_type::reverse_iterator;
        using const_reverse_iterator = typename container_type::const_reverse_iterator;
        using size_type = typename container_type::size_type;
        using difference_type = typename container_type::difference_type;

        Deque() = default;

        explicit Deque( const allocator_type &alloc ) : m_data( alloc )
        {
        }

        explicit Deque( size_type count ) : m_data( count )
        {
        }

        Deque( size_type count, const T &value, const allocator_type &alloc = allocator_type() ) :
            m_data( count, value, alloc )
        {
        }

        template <class InputIt>
        Deque( InputIt first, InputIt last, const allocator_type &alloc = allocator_type() ) :
            m_data( first, last, alloc )
        {
        }

        Deque( std::initializer_list<T> values, const allocator_type &alloc = allocator_type() ) :
            m_data( values.begin(), values.end(), alloc )
        {
        }

        Deque &operator=( std::initializer_list<T> values )
        {
            m_data.clear();
            m_data.insert( m_data.end(), values.begin(), values.end() );
            return *this;
        }

        allocator_type get_allocator() const
        {
            return m_data.get_allocator();
        }

        bool empty() const
        {
            return m_data.empty();
        }

        size_type size() const
        {
            return m_data.size();
        }

        size_type max_size() const
        {
            return m_data.max_size();
        }

        void reserve( size_type capacity )
        {
            m_data.reserve( capacity );
        }

        void resize( size_type newSize )
        {
            m_data.resize( newSize );
        }

        void resize( size_type newSize, const T &value )
        {
            m_data.resize( newSize, value );
        }

        void clear()
        {
            m_data.clear();
        }

        void push_back( const T &value )
        {
            m_data.push_back( value );
        }

        void push_back( T &&value )
        {
            m_data.push_back( std::move( value ) );
        }

        void push_front( const T &value )
        {
            m_data.insert( m_data.begin(), value );
        }

        void push_front( T &&value )
        {
            m_data.insert( m_data.begin(), std::move( value ) );
        }

        template <typename... Args>
        reference emplace_back( Args &&...args )
        {
            m_data.emplace_back( std::forward<Args>( args )... );
            return back();
        }

        template <typename... Args>
        reference emplace_front( Args &&...args )
        {
            m_data.emplace( m_data.begin(), std::forward<Args>( args )... );
            return front();
        }

        void pop_back()
        {
            check_not_empty( "Deque::pop_back on empty deque" );
            m_data.pop_back();
        }

        void pop_front()
        {
            check_not_empty( "Deque::pop_front on empty deque" );
            m_data.erase( m_data.begin() );
        }

        iterator erase( const_iterator it )
        {
            return m_data.erase( it );
        }

        iterator erase( const_iterator first, const_iterator last )
        {
            return m_data.erase( first, last );
        }

        void erase( size_type index )
        {
            if( index < m_data.size() )
            {
                m_data.erase( m_data.begin() + static_cast<difference_type>( index ) );
            }
        }

        reference at( size_type index )
        {
            return m_data.at( index );
        }

        const_reference at( size_type index ) const
        {
            return m_data.at( index );
        }

        reference operator[]( difference_type index )
        {
            return m_data[index];
        }

        const_reference operator[]( difference_type index ) const
        {
            return m_data[index];
        }

        reference front()
        {
            check_not_empty( "Deque::front on empty deque" );
            return m_data.front();
        }

        const_reference front() const
        {
            check_not_empty( "Deque::front on empty deque" );
            return m_data.front();
        }

        reference back()
        {
            check_not_empty( "Deque::back on empty deque" );
            return m_data.back();
        }

        const_reference back() const
        {
            check_not_empty( "Deque::back on empty deque" );
            return m_data.back();
        }

        pointer data()
        {
            return m_data.data();
        }

        const_pointer data() const
        {
            return m_data.data();
        }

        iterator begin()
        {
            return m_data.begin();
        }

        const_iterator begin() const
        {
            return m_data.begin();
        }

        const_iterator cbegin() const
        {
            return m_data.cbegin();
        }

        iterator end()
        {
            return m_data.end();
        }

        const_iterator end() const
        {
            return m_data.end();
        }

        const_iterator cend() const
        {
            return m_data.cend();
        }

        reverse_iterator rbegin()
        {
            return m_data.rbegin();
        }

        const_reverse_iterator rbegin() const
        {
            return m_data.rbegin();
        }

        reverse_iterator rend()
        {
            return m_data.rend();
        }

        const_reverse_iterator rend() const
        {
            return m_data.rend();
        }

    private:
        void check_not_empty( const char *message ) const
        {
            assert( !m_data.empty() );
            if( m_data.empty() )
            {
                throw std::out_of_range( message );
            }
        }

        container_type m_data;
    };

    template <class T, class A>
    bool operator==( const Deque<T, A> &lhs, const Deque<T, A> &rhs )
    {
        if( lhs.size() != rhs.size() )
        {
            return false;
        }

        for( typename Deque<T, A>::size_type i = 0; i < lhs.size(); ++i )
        {
            if( !( lhs[static_cast<typename Deque<T, A>::difference_type>( i )] ==
                   rhs[static_cast<typename Deque<T, A>::difference_type>( i )] ) )
            {
                return false;
            }
        }

        return true;
    }

    template <class T, class A>
    bool operator!=( const Deque<T, A> &lhs, const Deque<T, A> &rhs )
    {
        return !( lhs == rhs );
    }
}  // namespace workphone

#endif  // __Deque_h__
