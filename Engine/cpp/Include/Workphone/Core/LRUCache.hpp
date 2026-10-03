#ifndef LRUCache_h__
#define LRUCache_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <iostream>

namespace workphone
{

    class WPCore_API LRUCache
    {
    public:
        LRUCache();

        LRUCache( int capacity );

        void removeLRU();

        int get( int key );

        void put( int key, int value );

        void print();

        int m_capcity = 0;
        std::pair<int, int> m_lru;
        Array<std::pair<int, int>> m_values;
    };
}  // namespace workphone

#endif  // LRUCache_h__
