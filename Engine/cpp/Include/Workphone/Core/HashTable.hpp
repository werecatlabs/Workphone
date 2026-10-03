#ifndef HashTable_h__
#define HashTable_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Math.hpp>

#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace workphone
{

    template <typename Key, typename Value, typename TAlloc>
    class HashTableBase
    {
    public:
        explicit HashTableBase( u32 size );
        ~HashTableBase();

        void insert( const Key &key, const Value &value );

        void remove( const Key &key );

        Value &get( const Key &key );

        const Value &get( const Key &key ) const;

        u32 size() const;
        Value *data() const;

    private:
        /** Default hash function. */
        u32 hash( const Key &key ) const;

        u32 m_size;
        Value *m_data;
    };

    template <typename Key, typename Value, typename TAlloc>
    HashTableBase<Key, Value, TAlloc>::HashTableBase( u32 size ) : m_size( size ), m_data( nullptr )
    {
        if( m_size > 0 )
        {
            m_data = new Value[size];
        }
    }

    template <typename Key, typename Value, typename TAlloc>
    HashTableBase<Key, Value, TAlloc>::~HashTableBase()
    {
        if( m_data )
        {
            delete[] m_data;
            m_data = nullptr;
        }
    }

    template <typename Key, typename Value, typename TAlloc>
    void HashTableBase<Key, Value, TAlloc>::insert( const Key &key, const Value &value )
    {
        u32 index = hash( key );
        m_data[index] = value;
    }

    template <typename Key, typename Value, typename TAlloc>
    void HashTableBase<Key, Value, TAlloc>::remove( const Key &key )
    {
        u32 index = hash( key );
        m_data[index] = Value();
    }

    template <typename Key, typename Value, typename TAlloc>
    Value &HashTableBase<Key, Value, TAlloc>::get( const Key &key )
    {
        u32 index = hash( key );
        return m_data[index];
    }

    template <typename Key, typename Value, typename TAlloc>
    const Value &HashTableBase<Key, Value, TAlloc>::get( const Key &key ) const
    {
        u32 index = hash( key );
        return m_data[index];
    }

    template <typename Key, typename Value, typename TAlloc>
    u32 HashTableBase<Key, Value, TAlloc>::size() const
    {
        return m_size;
    }

    template <typename Key, typename Value, typename TAlloc>
    Value *HashTableBase<Key, Value, TAlloc>::data() const
    {
        return m_data;
    }

    template <typename Key, typename Value, typename TAlloc>
    u32 HashTableBase<Key, Value, TAlloc>::hash( const Key &key ) const
    {
        if( m_size == 0 )
            throw std::out_of_range( "Cannot access a zero-sized HashTable" );

        static double s_dHashMultiplier = 0.5 * ( MathD::Sqrt( 5.0 ) - 1.0 );
        unsigned int uiKey = 0;
        std::memcpy( &uiKey, &key, std::min( sizeof( uiKey ), sizeof( key ) ) );
        uiKey %= m_size;
        double dFraction = MathD::Mod( s_dHashMultiplier * uiKey, 1.0 );
        return static_cast<u32>( MathD::Floor( m_size * dFraction ) );
    }

    template <typename Key, typename Value, typename TAlloc>
    using HashTable = HashTableBase<Key, Value, TAlloc>;

}  // namespace workphone

#endif  // HashTable_h__
