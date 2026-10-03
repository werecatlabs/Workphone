#ifndef Pair_h__
#define Pair_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <utility>

namespace workphone
{

    /**
     * @brief A simple templated pair container.
     *
     * This class stores two related values (first and second) and provides a
     * pointer to the next Pair of the same type which can be useful for
     * implementing lightweight linked structures without allocating extra
     * container objects.
     *
     * The design intentionally mirrors std::pair but adds the `next` pointer
     * for simple chaining. It provides forwarding constructors and a
     * converting copy constructor to allow implicit conversion between
     * compatible Pair types.
     *
     * @tparam T Type of the first element.
     * @tparam B Type of the second element.
     */
    template <class T, class B>
    class Pair
    {
    public:
        /**
         * @brief Default constructs both elements using their default ctors.
         */
        Pair() = default;

        /**
         * @brief Constructs a Pair by copying the provided values.
         *
         * @param first  Const reference used to initialize the first element.
         * @param second Const reference used to initialize the second element.
         */
        Pair( const T &first, const B &second ) : first( first ), second( second )
        {
        }

        /**
         * @brief Perfect-forwarding constructor.
         *
         * Allows constructing Pair from temporaries or lvalues without
         * unnecessary copies. Mirrors the behaviour of std::pair's
         * forwarding constructor.
         *
         * @tparam U1 Type used to deduce and forward the first argument.
         * @tparam U2 Type used to deduce and forward the second argument.
         * @param first  Value (or forwardable) used to initialize first.
         * @param second Value (or forwardable) used to initialize second.
         */
        template <typename U1, typename U2>
        Pair( U1 &&first, U2 &&second ) :
            first( std::forward<U1>( first ) ),
            second( std::forward<U2>( second ) )
        {
        }

        /**
         * @brief Converting copy constructor from another Pair with
         *        convertible element types.
         *
         * This allows implicit conversion when the contained element types
         * of the other Pair are convertible to T and B respectively.
         *
         * @tparam U1 Source type for the first element.
         * @tparam U2 Source type for the second element.
         * @param other The Pair whose elements will be copied/converted.
         */
        template <typename U1, typename U2>
        Pair( const Pair<U1, U2> &other ) : first( other.first ), second( other.second )
        {
        }

        /** @brief The first element stored in the pair. */
        T first;

        /** @brief The second element stored in the pair. */
        B second;

        /**
         * @brief Optional pointer to the next Pair in a singly-linked chain.
         *
         * This is initialized to nullptr and can be used to link Pair
         * instances without introducing a separate node type.
         */
        Pair *next = nullptr;
    };

    /** @brief Lexicographically compares two pairs by value. */
    template <class T, class B>
    bool operator<( const Pair<T, B> &lhs, const Pair<T, B> &rhs )
    {
        return lhs.first < rhs.first || ( !( rhs.first < lhs.first ) && lhs.second < rhs.second );
    }

    /**
     * @brief Helper to create a Pair with template argument deduction.
     *
     * Similar to std::make_pair but returns workphone::Pair and performs
     * decay of the argument types (removes references, cv-qualifiers,
     * and turns array/function types into pointers) to preserve the
     * expected semantics for value containers.
     *
     * @tparam T1 Type of first parameter (deduced).
     * @tparam T2 Type of second parameter (deduced).
     * @param first  Value used to initialize the first element (forwarded).
     * @param second Value used to initialize the second element (forwarded).
     * @return A Pair holding decayed types of the provided arguments.
     */
    template <typename T1, typename T2>
    Pair<typename std::decay<T1>::type, typename std::decay<T2>::type> make_pair( T1 &&first,
                                                                                  T2 &&second )
    {
        using DecayT1 = typename std::decay<T1>::type;
        using DecayT2 = typename std::decay<T2>::type;
        return Pair<DecayT1, DecayT2>( std::forward<T1>( first ), std::forward<T2>( second ) );
    }

}  // namespace workphone

#endif  // Pair_h__
