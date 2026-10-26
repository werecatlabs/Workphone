#pragma once
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <vector>
#include <utility>
#include <algorithm>

namespace workphone
{
    /** STL-compatible storage with allocation-local SourceCpu ownership.
     * Copies/imports retain physical spare capacity and stage before replacement.
     * clear() retains its charge; releaseStorage() retires the allocation. */
    template <class T>
    class SourceBudgetVector : public std::vector<T, SourceBudgetAllocator<T>>, public SourceBudgetObject
    {
        using Base = std::vector<T, SourceBudgetAllocator<T>>;

        template <class A>
        void import( const std::vector<T, A> &other )
        {
            Base::reserve( other.capacity() );
            Base::insert( Base::end(), other.begin(), other.end() );
        }

    public:
        using Base::Base;
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;
        SourceBudgetVector() = default;

        SourceBudgetVector( const SourceBudgetVector &other )
        {
            import( other );
        }

        SourceBudgetVector( SourceBudgetVector && ) noexcept = default;
        SourceBudgetVector &operator=( SourceBudgetVector && ) noexcept = default;

        SourceBudgetVector &operator=( const SourceBudgetVector &other )
        {
            if( this != &other )
            {
                SourceBudgetVector candidate( other );
                Base::swap( candidate );
            }
            return *this;
        }

        template <class A>
        SourceBudgetVector( const std::vector<T, A> &other )
        {
            import( other );
        }

        template <class A>
        SourceBudgetVector &operator=( const std::vector<T, A> &other )
        {
            SourceBudgetVector candidate( other );
            Base::swap( candidate );
            return *this;
        }

        SourceBudgetVector &operator=( std::initializer_list<T> values )
        {
            SourceBudgetVector candidate( values );
            Base::swap( candidate );
            return *this;
        }

        void releaseStorage() noexcept
        {
            Base{}.swap( *this );
        }

        friend bool operator==( const SourceBudgetVector &a, const SourceBudgetVector &b )
        {
            return static_cast<const Base &>( a ) == static_cast<const Base &>( b );
        }

        friend bool operator!=( const SourceBudgetVector &a, const SourceBudgetVector &b )
        {
            return !( a == b );
        }
    };
}  // namespace workphone
