#ifndef WP_SOURCE_BUDGET_CONTAINERS_HPP
#define WP_SOURCE_BUDGET_CONTAINERS_HPP
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentMap.hpp>
#include <string>
#include <utility>

namespace workphone
{
    template<class K, class V, class Compare = std::less<K>>
    using SourceBudgetConcurrentMap = ConcurrentMap<K, V, Compare, SourceBudgetAllocator<std::pair<const K,V>>>;

    template<class T> using SourceBudgetArray = Array<T, SourceBudgetAllocator<T>>;

    // Persistent storage uses the shared budget. Public snapshots keep the
    // existing caller-owned Array API and are copied under one inventory lock.
    template<class T> class SourceBudgetConcurrentArray : public ConcurrentArray<T, SourceBudgetAllocator<T>>
    {
        using Base = ConcurrentArray<T, SourceBudgetAllocator<T>>;
    public:
        using Base::Base;
        void releaseStorage() { Base::operator=( Base{} ); }
        Array<T> snapshot() const
        {
            ScopedLock lock( this, false );
            return Array<T>( this->cbegin(), this->cend() );
        }
    };

    template<class T> struct SourceBudgetDeleter
    {
        void operator()( T *value ) const noexcept
        {
            if( value ) { value->~T(); freeSourceBudgetStorage( value, alignof(T) ); }
        }
    };
    template<class T> using SourceBudgetUnique = std::unique_ptr<T, SourceBudgetDeleter<T>>;
    template<class T, class... Args> SourceBudgetUnique<T> makeSourceBudgetUnique( Args&&... args )
    {
        auto memory = allocateSourceBudgetStorage( sizeof(T), alignof(T) );
        try { return SourceBudgetUnique<T>( new(memory) T( std::forward<Args>(args)... ) ); }
        catch(...) { freeSourceBudgetStorage( memory, alignof(T) ); throw; }
    }

    template<class T> struct SourceBudgetArrayDeleter
    {
        void operator()( T *p ) const noexcept
        {
            static_assert(std::is_trivially_destructible_v<T>);
            freeSourceBudgetStorage(p, alignof(T));
        }
    };
    template<class T> using SourceBudgetUniqueArray = std::unique_ptr<T[], SourceBudgetArrayDeleter<T>>;
    template<class T> SourceBudgetUniqueArray<T> makeSourceBudgetArray( size_t count )
    {
        auto p = SourceBudgetAllocator<T>{}.allocate(count);
        std::uninitialized_value_construct_n(p, count);
        return SourceBudgetUniqueArray<T>(p);
    }

    inline StreamBudget reserveSourceBudgetBytes( size_t bytes )
    {
        String error;
        auto budget = WorldStreamingScheduler::shared()->reserve( StreamMemoryClass::SourceCpu, bytes, error );
        if( !budget ) throw std::bad_alloc();
        return budget;
    }

    // Preserve Array reference APIs for immutable shared payloads. The buffer
    // lease precedes the value so physical storage dies before its reservation.
    template<class T> std::shared_ptr<Array<T>> copySourceBudgetArray( const Array<T> &value )
    {
        struct Owner
        {
            StreamBudget budget;
            Array<T> value;
            explicit Owner( const Array<T> &input ) :
                budget( reserveSourceBudgetBytes( input.capacity() * sizeof(T) ) ), value(input) {}
        };
        auto owner = std::allocate_shared<Owner>( SourceBudgetAllocator<Owner>{}, value );
        auto payload = &owner->value;
        return std::shared_ptr<Array<T>>( std::move(owner), payload );
    }

    // Private authoring/cache strings admit every heap buffer, including old/new
    // overlap. No lease is needed for the basic_string inline buffer.
    class SourceBudgetString
    {
        using Storage = std::basic_string<char, std::char_traits<char>, SourceBudgetAllocator<char>>;
        Storage m_value;
    public:
        SourceBudgetString() = default;
        SourceBudgetString( const String &value ) : m_value(value.data(), value.size()) {}
        SourceBudgetString &operator=( const String &value )
        { Storage next(value.data(), value.size()); m_value.swap(next); return *this; }
        operator String() const { return String(m_value.data(), m_value.size()); }
        int compare( const SourceBudgetString &other ) const noexcept { return m_value.compare(other.m_value); }
        int compare( const String &other ) const noexcept { return m_value.compare(0, m_value.size(), other.data(), other.size()); }
        bool empty() const noexcept { return m_value.empty(); }
        size_t size() const noexcept { return m_value.size(); }
        void clear() noexcept { Storage{}.swap(m_value); }
        void swap( SourceBudgetString &other ) noexcept { m_value.swap(other.m_value); }
        friend bool operator==( const SourceBudgetString &a, const String &b )
        { return a.m_value.size() == b.size() && std::char_traits<char>::compare(a.m_value.data(), b.data(), b.size()) == 0; }
        friend bool operator!=( const SourceBudgetString &a, const String &b ) { return !(a == b); }
    };
    struct SourceBudgetStringLess
    {
        bool operator()(const SourceBudgetString &a, const SourceBudgetString &b) const noexcept { return a.compare(b) < 0; }
        bool operator()(const SourceBudgetString &a, const String &b) const noexcept { return a.compare(b) < 0; }
        bool operator()(const String &a, const SourceBudgetString &b) const noexcept { return b.compare(a) > 0; }
    };
}
#endif
