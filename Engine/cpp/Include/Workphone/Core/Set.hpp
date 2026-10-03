#ifndef _WP_Set_h__
#define _WP_Set_h__

#include <Workphone/Core/Array.hpp>

#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <utility>

namespace workphone
{
    template <class T>
    struct SetDefaultCompare
    {
        bool operator()( const T &lhs, const T &rhs ) const
        {
            return lhs < rhs;
        }
    };

    /**
     * @brief Sorted unique collection backed by Workphone Array.
     *
     * This intentionally avoids std::set storage. Values are kept sorted by the
     * comparator so lookup is binary-search based, while insert/erase move array
     * elements. Iterators expose const values to protect the sorted invariant.
     */
    template <class T, class Cmp = SetDefaultCompare<T>>
    class Set
    {
    public:
        using container_type = Array<T>;
        using value_type = T;
        using key_type = T;
        using key_compare = Cmp;
        using value_compare = Cmp;
        using size_type = typename container_type::size_type;
        using difference_type = typename container_type::difference_type;
        using const_reference = const T &;
        using const_pointer = const T *;
        using const_iterator = typename container_type::const_iterator;
        using iterator = const_iterator;

        Set() = default;

        explicit Set( const Cmp &compare ) : m_compare( compare )
        {
        }

        template <class InputIt>
        Set( InputIt first, InputIt last, const Cmp &compare = Cmp() ) : m_compare( compare )
        {
            insert( first, last );
        }

        Set( std::initializer_list<T> values, const Cmp &compare = Cmp() ) : m_compare( compare )
        {
            insert( values.begin(), values.end() );
        }

        bool empty() const
        {
            return m_values.empty();
        }

        size_type size() const
        {
            return m_values.size();
        }

        void clear()
        {
            m_values.clear();
        }

        std::pair<iterator, bool> insert( const T &value )
        {
            const auto index = lower_bound_index( value );
            if( index < m_values.size() && equivalent( m_values[index], value ) )
            {
                return std::make_pair( iterator_at( index ), false );
            }

            m_values.insert( m_values.begin() + static_cast<difference_type>( index ), value );
            return std::make_pair( iterator_at( index ), true );
        }

        std::pair<iterator, bool> insert( T &&value )
        {
            const auto index = lower_bound_index( value );
            if( index < m_values.size() && equivalent( m_values[index], value ) )
            {
                return std::make_pair( iterator_at( index ), false );
            }

            m_values.insert( m_values.begin() + static_cast<difference_type>( index ),
                             std::move( value ) );
            return std::make_pair( iterator_at( index ), true );
        }

        iterator insert( const_iterator, const T &value )
        {
            return insert( value ).first;
        }

        iterator insert( const_iterator, T &&value )
        {
            return insert( std::move( value ) ).first;
        }

        template <class InputIt>
        void insert( InputIt first, InputIt last )
        {
            container_type pendingValues;
            for( ; first != last; ++first )
            {
                pendingValues.push_back( *first );
            }

            for( const auto &value : pendingValues )
            {
                insert( value );
            }
        }

        size_type erase( const T &value )
        {
            const auto index = lower_bound_index( value );
            if( index >= m_values.size() || !equivalent( m_values[index], value ) )
            {
                return 0;
            }

            erase_at_index( index );
            return 1;
        }

        iterator erase( const_iterator it )
        {
            const auto index = iterator_index( it );
            if( index >= m_values.size() )
            {
                return end();
            }

            erase_at_index( index );
            return iterator_at( index );
        }

        iterator erase( const_iterator first, const_iterator last )
        {
            auto firstIndex = iterator_index( first );
            const auto lastIndex = iterator_index( last );
            const auto eraseCount = lastIndex > firstIndex ? lastIndex - firstIndex : 0;

            for( size_type i = 0; i < eraseCount && firstIndex < m_values.size(); ++i )
            {
                erase_at_index( firstIndex );
            }

            return iterator_at( firstIndex );
        }

        iterator find( const T &value )
        {
            return const_cast<const Set *>( this )->find( value );
        }

        const_iterator find( const T &value ) const
        {
            const auto index = lower_bound_index( value );
            if( index < m_values.size() && equivalent( m_values[index], value ) )
            {
                return iterator_at( index );
            }

            return end();
        }

        bool contains( const T &value ) const
        {
            return find( value ) != end();
        }

        size_type count( const T &value ) const
        {
            return contains( value ) ? 1 : 0;
        }

        iterator lower_bound( const T &value )
        {
            return iterator_at( lower_bound_index( value ) );
        }

        const_iterator lower_bound( const T &value ) const
        {
            return iterator_at( lower_bound_index( value ) );
        }

        iterator begin()
        {
            return m_values.begin();
        }

        const_iterator begin() const
        {
            return m_values.begin();
        }

        const_iterator cbegin() const
        {
            return m_values.begin();
        }

        iterator end()
        {
            return m_values.end();
        }

        const_iterator end() const
        {
            return m_values.end();
        }

        const_iterator cend() const
        {
            return m_values.end();
        }

        const_reference front() const
        {
            assert( !empty() );
            if( empty() )
            {
                throw std::out_of_range( "Set::front on empty set" );
            }

            return *begin();
        }

        const_reference back() const
        {
            assert( !empty() );
            if( empty() )
            {
                throw std::out_of_range( "Set::back on empty set" );
            }

            auto it = end();
            --it;
            return *it;
        }

        const_pointer data() const
        {
            return m_values.data();
        }

        const container_type &values() const
        {
            return m_values;
        }

        Cmp key_comp() const
        {
            return m_compare;
        }

        Cmp value_comp() const
        {
            return m_compare;
        }

    private:
        bool equivalent( const T &lhs, const T &rhs ) const
        {
            return !m_compare( lhs, rhs ) && !m_compare( rhs, lhs );
        }

        size_type lower_bound_index( const T &value ) const
        {
            size_type first = 0;
            size_type count = m_values.size();

            while( count > 0 )
            {
                const auto step = count / 2;
                const auto middle = first + step;

                if( m_compare( m_values[middle], value ) )
                {
                    first = middle + 1;
                    count -= step + 1;
                }
                else
                {
                    count = step;
                }
            }

            return first;
        }

        size_type iterator_index( const_iterator it ) const
        {
            if( it == end() )
            {
                return m_values.size();
            }

            size_type index = 0;
            for( auto current = begin(); current != end(); ++current, ++index )
            {
                if( current == it )
                {
                    return index;
                }
            }

            return m_values.size();
        }

        const_iterator iterator_at( size_type index ) const
        {
            if( index >= m_values.size() )
            {
                return m_values.end();
            }

            return m_values.begin() + static_cast<difference_type>( index );
        }

        void erase_at_index( size_type index )
        {
            m_values.erase( m_values.begin() + static_cast<difference_type>( index ) );
        }

        container_type m_values;
        Cmp m_compare;
    };
}  // namespace workphone

#endif  // Set_h__
