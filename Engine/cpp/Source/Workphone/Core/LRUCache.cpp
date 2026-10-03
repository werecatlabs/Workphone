#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/LRUCache.hpp>

namespace workphone
{

    LRUCache::LRUCache( int capacity ) : m_capcity( std::max( 0, capacity ) )
    {
        if( m_capcity > 0 )
            m_values.reserve( static_cast<Array<std::pair<int, int>>::size_type>( m_capcity ) );
    }

    LRUCache::LRUCache() : m_capcity( 2 )
    {
        m_values.reserve( 2 );
    }

    void LRUCache::removeLRU()
    {
        if( m_values.empty() )
            return;

        m_lru = m_values.front();
        m_values.erase( m_values.begin() );
    }

    int LRUCache::get( int key )
    {
        auto it = std::find_if( m_values.begin(), m_values.end(),
                                [&]( const auto &candidate ) { return candidate.first == key; } );

        if( it == m_values.end() )
            return -1;

        const auto entry = *it;
        m_values.erase( it );
        m_values.push_back( entry );
        return entry.second;
    }

    void LRUCache::put( int key, int value )
    {
        if( m_capcity == 0 )
            return;

        auto it = std::find_if( m_values.begin(), m_values.end(),
                                [&]( const auto &candidate ) { return candidate.first == key; } );
        if( it != m_values.end() )
            m_values.erase( it );
        else if( m_values.size() >= static_cast<Array<std::pair<int, int>>::size_type>( m_capcity ) )
            removeLRU();

        m_values.push_back( std::make_pair( key, value ) );
    }

    void LRUCache::print()
    {
        for( auto &value : m_values )
        {
            std::cout << value.first << " " << value.second << std::endl;
        }
    }

}  // namespace workphone
