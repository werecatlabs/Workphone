#ifndef __WorkphoneUtil_H_
#define __WorkphoneUtil_H_

#include <Workphone/WorkphoneEnums.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Set.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Interface/UI/IUIMenuItem.hpp>
#include <deque>
#include <algorithm>

namespace workphone
{

    /**
     * Provides utility functions for dealing with core types.
     */
    class WPCore_API Util
    {
    public:
        static const String resourceStr;       ///< String constant for "resource".
        static const String resourceTypeStr;   ///< String constant for "resourceType".
        static const String materialStr;       ///< String constant for "material".
        static const String textureStr;        ///< String constant for "texture".
        static const String componentStr;      ///< String constant for "component".
        static const String noneStr;           ///< String constant for "none".
        static const String attributeNameStr;  ///< String constant for "attributeName".
        static const String defaultValue;      ///< String constant for the default value.
        static const String buttonTypeStr;     ///< String constant for "buttonType".
        static const String enumTypeStr;       ///< String constant for "enumType".
        static const String defaultType;       ///< String constant for the default type.
        static const String nullStr;           ///< String constant for "null".
        static const String colourStr;         ///< String constant for "colour".
        static const String colouriStr;        ///< String constant for "colouri".
        static const String boolStr;           ///< String constant for "bool".
        static const String intStr;            ///< String constant for "int".
        static const String floatStr;          ///< String constant for "float".
        static const String doubleStr;         ///< String constant for "double".
        static const String vector2Str;        ///< String constant for "vector2".
        static const String vector2fStr;       ///< String constant for "vector2f".
        static const String vector2dStr;       ///< String constant for "vector2d".
        static const String vector2iStr;       ///< String constant for "vector2i".
        static const String vector3Str;        ///< String constant for "vector3".
        static const String vector3fStr;       ///< String constant for "vector3f".
        static const String vector3dStr;       ///< String constant for "vector3d".
        static const String vector3iStr;       ///< String constant for "vector3i".
        static const String quatdStr;          ///< String constant for "quatd".
        static const String quatfStr;          ///< String constant for "quatf".

        /**
         * The range used for compacting and expanding floating-point values.
         * Change to 20000 to `SHRT_MAX` if you don't mind whole numbers being turned into fractional
         * ones.
         */
        static const int compact_range;

        /**
         * Creates a set from an array.
         *
         * @tparam T The type of elements in the array.
         * @param data The array from which to create the set.
         * @return The created set.
         */
        template <class T>
        static Set<T> createSet( const Array<T> &data );

        /**
         * Erases an element from an array.
         *
         * @tparam T The type of elements in the array.
         * @param array The array from which to erase the element.
         * @param element The element to erase.
         * @return `true` if the element was successfully erased, `false` otherwise.
         */
        template <class T>
        static bool erase( Array<T> &array, const T &element );

#if WP_USE_TBB
        /**
         * Erases an element from a concurrent vector.
         *
         * @tparam T The type of elements in the concurrent vector.
         * @param array The concurrent vector from which to erase the element.
         * @param element The element to erase.
         * @return `true` if the element was successfully erased, `false` otherwise.
         */
        template <class T>
        static bool erase( ConcurrentArray<T> &array, const T &element );

        /**
         * Erases an element of a specific type from a concurrent vector.
         *
         * @tparam T The type of elements in the concurrent vector.
         * @tparam ElemType The type of element to erase.
         * @param array The concurrent vector from which to erase the element.
         * @param element The element to erase.
         * @return `true` if the element was successfully erased, `false` otherwise.
         */
        template <class T, class ElemType>
        static bool eraseElementOfType( ConcurrentArray<T> &array, const ElemType &element );
#endif

        /**
         * Performs aligned memory copy using SSE2 instructions.
         *
         * @param dest The destination memory address.
         * @param src The source memory address.
         * @param size The number of bytes to copy.
         */
        static void X_aligned_memcpy_sse2( void *dest, const void *src, size_t size );

        /**
         * Sleeps for a specified number of seconds with higher accuracy.
         *
         * @param seconds The number of seconds to sleep.
         */
        static void accurateSleep( double seconds );

        /**
         * Gets the resolution of the desktop screen.
         *
         * @param horizontal Reference to store the horizontal resolution.
         * @param vertical Reference to store the vertical resolution.
         */
        static void getDesktopResolution( int &horizontal, int &vertical );

        /**
         * Aligns an offset to the next multiple of an alignment value.
         *
         * @tparam T The type of the offset and alignment.
         * @param offset The offset value to align.
         * @param alignment The alignment value.
         * @return The aligned offset value.
         */
        template <typename T>
        static T alignToNextMultiple( T offset, T alignment );

        /**
         * Converts a floating-point value to a compact representation.
         *
         * @param input The input value to convert.
         * @param range The range value to use for conversion (optional, default is 1000).
         * @return The compact representation of the input value.
         */
        static short compactFloat( double input, int range = 1000 );

        /**
         * Converts a compact representation of a floating-point value to its original form.
         *
         * @param input The compact representation of the value.
         * @param range The range value used for compacting the value (optional, default is 1000).
         * @return The expanded floating-point value.
         */
        static double expandToFloat( short input, int range = 1000 );

        /**
         * Converts the pixel format between BGRA and RGBA.
         *
         * @param input The input pixel data.
         * @param pixel_width The width of the pixel data.
         * @param pixel_height The height of the pixel data.
         * @param output The output pixel data.
         */
        static void convertBetweenBGRAandRGBA( unsigned char *input, int pixel_width, int pixel_height,
                                               unsigned char *output );

        /**
         * Finds the lowest value in a range defined by iterators.
         *
         * @tparam IT_TYPE The type of the iterators.
         * @tparam T The type of the value to find.
         * @param begin The beginning iterator of the range.
         * @param end The ending iterator of the range.
         * @param value Reference to store the lowest value found.
         */
        template <class IT_TYPE, class T>
        static void lowest( IT_TYPE begin, IT_TYPE end, T &value );

        /**
         * Finds the highest value in a range defined by iterators.
         *
         * @tparam IT_TYPE The type of the iterators.
         * @tparam T The type of the value to find.
         * @param begin The beginning iterator of the range.
         * @param end The ending iterator of the range.
         * @param value Reference to store the highest value found.
         */
        template <class IT_TYPE, class T>
        static void highest( IT_TYPE begin, IT_TYPE end, T &value );

        /**
         * Computes the average value of an array.
         *
         * @tparam T The type of elements in the array.
         * @param v The array to compute the average from.
         * @return The average value of the array.
         */
        template <class T>
        static T average( const Array<T> &v );

        /**
         * Computes the average value of a deque.
         *
         * @tparam T The type of elements in the deque.
         * @param v The deque to compute the average from.
         * @return The average value of the deque.
         */
        template <class T>
        static T average( const Deque<T> &v );

        /**
         * Computes the cross product of two __m128 vectors.
         *
         * @param a The first vector.
         * @param b The second vector.
         * @return The cross product of the two vectors.
         */
#if defined WP_PLATFORM_WIN32
        static __m128 crossProduct( __m128 a, __m128 b );
#endif

        /**
         * Removes an element from an Array container.
         *
         * @tparam T The type of elements in the Array container.
         * @param array The Array container from which to remove the element.
         * @param element The element to remove.
         * @return `true` if the element was successfully removed, `false` otherwise.
         */
        template <class T>
        static bool eraseElement( Array<T> &array, const T &element );

        /**
         * Compares two values of type s32.
         *
         * @param a Pointer to the first value to compare.
         * @param b Pointer to the second value to compare.
         * @return A negative value if `a` is less than `b`, zero if they are equal, or a positive value
         * if `a` is greater than `b`.
         */
        static s32 compare( const void *a, const void *b );

        /**
         * Checks if there are any duplicate values in the specified range defined by iterators.
         *
         * @tparam Iterator The type of the iterators.
         * @param first The beginning iterator of the range.
         * @param end The ending iterator of the range.
         * @return `true` if duplicates are found, `false` otherwise.
         */
        template <typename Iterator>
        static bool hasDuplicates( Iterator first, Iterator end );

        /**
         * Checks if there are any duplicate values in the specified container.
         *
         * @tparam Container The type of the container.
         * @param v The container to check for duplicates.
         * @return `true` if duplicates are found, `false` otherwise.
         */
        template <typename Container>
        static bool hasDuplicates( const Container &v );

        /**
         * Sorts an Array container of SmartPtr objects by their names.
         *
         * @tparam T The type of the objects stored in the Array container.
         * @param vector The Array container to sort by name.
         */
        template <class T>
        static void sortByName( Array<SmartPtr<T>> &vector );

        /**
         * Calculates the nearest power of two that is greater than or equal to the input.
         *
         * @param input The input value.
         * @return The nearest power of two.
         */
        static int calculateNearest2Pow( int input );

        /**
         * Adds a menu item to a UI menu.
         *
         * @param menu The menu to add the item to.
         * @param itemid The hash identifier for the menu item.
         * @param text The display text for the menu item.
         * @param help The help/tooltip text for the menu item.
         * @param type The type of menu item (optional, default is Normal).
         * @return A smart pointer to the created menu item.
         */
        static SmartPtr<ui::IUIMenuItem> addMenuItem(
            SmartPtr<ui::IUIMenu> menu, hash_type itemid, const String &text, const String &help,
            ui::IUIMenuItem::Type type = ui::IUIMenuItem::Type::Normal );

        /**
         * Adds a separator to a UI menu.
         *
         * @param menu The menu to add the separator to.
         * @return A smart pointer to the created separator menu item.
         */
        static SmartPtr<ui::IUIMenuItem> addMenuSeparator( SmartPtr<ui::IUIMenu> menu );

        /**
         * Sets the text of a UI tree node.
         *
         * @param node The tree node to set the text on.
         * @param text The text to set.
         * @return A smart pointer to the UI element that was modified.
         */
        static SmartPtr<ui::IUIElement> setText( SmartPtr<ui::IUITreeNode> node, const String &text );

        /**
         * Sets the image of a UI tree node.
         *
         * @param node The tree node to set the image on.
         * @param imagePath The file path to the image.
         * @return A smart pointer to the UI element that was modified.
         */
        static SmartPtr<ui::IUIElement> setImage( SmartPtr<ui::IUITreeNode> node,
                                                  const String &imagePath );

        /**
         * Gets the first child element of a UI element.
         *
         * @param element The parent UI element.
         * @return A smart pointer to the first child element, or null if none exists.
         */
        static SmartPtr<ui::IUIElement> getFirstChild( SmartPtr<ui::IUIElement> element );

        /**
         * Gets the text of a UI tree node.
         *
         * @param node The tree node to get the text from.
         * @return The text of the tree node.
         */
        static String getText( SmartPtr<ui::IUITreeNode> node );

        /**
         * Converts a string representation to a ParameterType enum value.
         *
         * @param typeStr The string representation of the parameter type.
         * @return The corresponding ParameterType value.
         */
        static ParameterType getParameterTypeFromString( const String &typeStr );

        /**
         * Converts a string pointer representation to a ParameterType enum value.
         *
         * @param typeStr The string pointer representation of the parameter type.
         * @return The corresponding ParameterType value.
         */
        static ParameterType getParameterTypeFromStringPtr( const StringPtr typeStr );

        /**
         * Converts a ParameterType enum value to its string representation.
         *
         * @param type The parameter type to convert.
         * @return The string representation of the parameter type.
         */
        static String getStringFromParameterType( ParameterType type );

        /**
         * Converts a ParameterType enum value to its string pointer representation.
         *
         * @param type The parameter type to convert.
         * @return The string pointer representation of the parameter type.
         */
        static const StringPtr getStringFromParameterTypePtr( ParameterType type );

        /**
         * Checks if all values in a vector are unique.
         *
         * @tparam T The type of elements in the vector.
         * @param vec The vector to check.
         * @return `true` if all values are unique, `false` otherwise.
         */
        template <class T>
        static bool areAllValuesUnique( const std::vector<T> &vec );

        /**
         * Computes the average velocity from a sequence of transforms and timestamps.
         *
         * @tparam T The numeric type used by the transform and vector (e.g. float, double).
         * @param positions The array of transforms representing positions over time.
         * @param times The array of timestamps corresponding to each position.
         * @return The average velocity as a 3D vector.
         */
        template <class T>
        static Vector3<T> getAverageVelocity( const Array<Transform3<T>> &positions,
                                              const Array<f64> &times );

        template <class T>
        static bool compareByPriority( const T &a, const T &b )
        {
            if( a && b )
            {
                return ( a->getPriority() < b->getPriority() );
            }

            return false;
        }

        template <class IT_TYPE>
        static void sortByPriority( IT_TYPE begin, IT_TYPE end )
        {
            std::sort( begin, end, compareByPriority<typename IT_TYPE::value_type> );
        }

        /**
         * Sorts a range of elements in ascending order using an introsort algorithm
         * (quicksort with median-of-three pivot selection, falling back to heap sort
         * when recursion depth is exceeded, and insertion sort for small sub-ranges).
         *
         * @tparam _RanIt A random-access iterator type.
         * @param _First Iterator to the beginning of the range to sort.
         * @param _Last Iterator past the end of the range to sort.
         */
        template <class _RanIt>
        static void sort( const _RanIt _First, const _RanIt _Last )
        {
            if( _Last - _First < 2 )
                return;
            int depth_limit = 0;
            auto n = _Last - _First;
            while( n > 1 )
            {
                n >>= 1;
                ++depth_limit;
            }
            depth_limit *= 2;
            _wp_introsort( _First, _Last, depth_limit );
        }

    private:
        /**
         * Insertion sort for small ranges (<= 16 elements).
         * Used as the final pass of introsort for sub-ranges that are too small
         * for quicksort to be efficient.
         *
         * @tparam _RanIt A random-access iterator type.
         * @param first Iterator to the beginning of the range.
         * @param last Iterator past the end of the range.
         */
        template <class _RanIt>
        static void _wp_insertion_sort( _RanIt first, _RanIt last )
        {
            if( last - first < 2 )
                return;
            for( _RanIt i = first + 1; i != last; ++i )
            {
                auto value = *i;
                _RanIt j = i;
                while( j != first && value < *( j - 1 ) )
                {
                    *j = *( j - 1 );
                    --j;
                }
                *j = value;
            }
        }

        /**
         * Sifts a node down in a max-heap to restore the heap property.
         *
         * @tparam _RanIt A random-access iterator type.
         * @param first Iterator to the beginning of the heap.
         * @param last Iterator past the end of the heap.
         * @param root Iterator to the node to sift down.
         */
        template <class _RanIt>
        static void _wp_sift_down( _RanIt first, _RanIt last, _RanIt root )
        {
            for( ;; )
            {
                _RanIt child = first + ( root - first ) * 2 + 1;
                if( child >= last )
                    break;
                if( child + 1 < last && *child < *( child + 1 ) )
                    ++child;
                if( !( *root < *child ) )
                    break;
                auto tmp = *root;
                *root = *child;
                *child = tmp;
                root = child;
            }
        }

        /**
         * Sorts a range using heap sort. Used as an O(n log n) fallback when
         * introsort recursion depth is exceeded.
         *
         * @tparam _RanIt A random-access iterator type.
         * @param first Iterator to the beginning of the range.
         * @param last Iterator past the end of the range.
         */
        template <class _RanIt>
        static void _wp_heap_sort( _RanIt first, _RanIt last )
        {
            auto n = last - first;
            for( auto i = n / 2 - 1; i >= 0; --i )
                _wp_sift_down( first, last, first + i );
            while( last - first > 1 )
            {
                --last;
                auto tmp = *first;
                *first = *last;
                *last = tmp;
                _wp_sift_down( first, last, first );
            }
        }

        /**
         * Partitions a range using the Lomuto scheme with a median-of-three pivot.
         * After partitioning, elements less than or equal to the pivot are to the left
         * of the returned iterator, and elements greater than the pivot are to the right.
         *
         * @tparam _RanIt A random-access iterator type.
         * @param first Iterator to the beginning of the range.
         * @param last Iterator past the end of the range.
         * @return Iterator to the final position of the pivot element.
         */
        template <class _RanIt>
        static _RanIt _wp_partition( _RanIt first, _RanIt last )
        {
            _RanIt mid = first + ( last - first ) / 2;
            _RanIt tail = last - 1;

            /* 3-element sort so that *first <= *mid <= *tail (median at mid). */
            if( *mid < *first )
            {
                auto t = *mid;
                *mid = *first;
                *first = t;
            }
            if( *tail < *mid )
            {
                auto t = *tail;
                *tail = *mid;
                *mid = t;
            }
            if( *mid < *first )
            {
                auto t = *mid;
                *mid = *first;
                *first = t;
            }

            /* Move pivot to tail, then partition [first, tail-1]. */
            auto pivot = *mid;
            {
                auto t = *mid;
                *mid = *tail;
                *tail = t;
            }

            _RanIt store = first;
            for( _RanIt j = first; j != tail; ++j )
            {
                if( !( pivot < *j ) )
                {
                    auto t = *store;
                    *store = *j;
                    *j = t;
                    ++store;
                }
            }
            {
                auto t = *store;
                *store = *tail;
                *tail = t;
            }
            return store;
        }

        /**
         * Introsort implementation: quicksort with heap-sort fallback.
         * Recursively partitions the range, switching to heap sort when the
         * recursion depth limit is reached, and to insertion sort for small
         * sub-ranges (<= 16 elements).
         *
         * @tparam _RanIt A random-access iterator type.
         * @param first Iterator to the beginning of the range.
         * @param last Iterator past the end of the range.
         * @param depth_limit Maximum remaining recursion depth before falling back to heap sort.
         */
        template <class _RanIt>
        static void _wp_introsort( _RanIt first, _RanIt last, int depth_limit )
        {
            while( last - first > 16 )
            {
                if( depth_limit == 0 )
                {
                    _wp_heap_sort( first, last );
                    return;
                }
                --depth_limit;
                _RanIt p = _wp_partition( first, last );
                /* Recurse on the smaller partition to bound stack depth. */
                if( p - first < last - ( p + 1 ) )
                {
                    _wp_introsort( first, p, depth_limit );
                    first = p + 1;
                }
                else
                {
                    _wp_introsort( p + 1, last, depth_limit );
                    last = p;
                }
            }
            _wp_insertion_sort( first, last );
        }
    };

    /**
     * Fills an array with a specified value.
     *
     * @tparam T The type of the array elements.
     * @param d Pointer to the beginning of the array to fill.
     * @param size The number of elements to fill.
     * @param value The value to assign to each element.
     */
    template <class T>
    void fill_n( T *d, size_t size, T value )
    {
        for( size_t i = 0; i < size; ++i )
        {
            d[i] = value;
        }
    }

    template <class T>
    Set<T> Util::createSet( const Array<T> &data )
    {
        Set<T> set;
        for( u32 i = 0; i < data.size(); ++i )
        {
            set.insert( data[i] );
        }

        return set;
    }

    template <class T>
    bool Util::erase( Array<T> &array, const T &element )
    {
        if( array.empty() )
        {
            return false;
        }

        auto hasElement = false;

        typename Array<T>::iterator it = std::find( array.begin(), array.end(), element );
        if( it != array.end() )
        {
            hasElement = true;
        }

        if( hasElement )
        {
            Array<T> newArray;
            newArray.reserve( array.size() );

            for( size_t i = 0; i < array.size(); ++i )
            {
                if( element != array[i] )
                {
                    newArray.push_back( array[i] );
                }
            }

            array = newArray;
        }

        return hasElement;
    }

#if WP_USE_TBB

    template <class T>
    bool Util::erase( ConcurrentArray<T> &array, const T &element )
    {
        if( array.empty() )
        {
            return false;
        }

        bool hasElement = false;
        typename ConcurrentArray<T>::iterator it = std::find( array.begin(), array.end(), element );
        if( it != array.end() )
        {
            hasElement = true;
        }

        if( hasElement )
        {
            ConcurrentArray<T> newArray;
            for( unsigned int i = 0; i < array.size(); ++i )
            {
                if( element != array[i] )
                    newArray.push_back( array[i] );
            }

            array = newArray;
        }

        return hasElement;
    }

    template <class T, class ElemType>
    bool Util::eraseElementOfType( ConcurrentArray<T> &array, const ElemType &element )
    {
        if( array.empty() )
        {
            return false;
        }

        auto hasElement = false;

        typename ConcurrentArray<T>::iterator it = std::find( array.begin(), array.end(), element );
        if( it != array.end() )
        {
            hasElement = true;
        }

        if( hasElement )
        {
            tbb::concurrent_vector<T> newArray;
            newArray.reserve( array.size() );

            for( size_t i = 0; i < array.size(); ++i )
            {
                if( element != array[i] )
                {
                    newArray.push_back( array[i] );
                }
            }

            array = newArray;
        }

        return hasElement;
    }
#endif

    template <typename T>
    T Util::alignToNextMultiple( T offset, T alignment )
    {
        return ( ( offset + alignment - 1u ) / alignment ) * alignment;
    }

    template <class IT_TYPE, class T>
    void Util::lowest( IT_TYPE begin, IT_TYPE end, T &value )
    {
        value = T( 1e10 );
        IT_TYPE i = begin;
        for( ; i != end; ++i )
        {
            value = std::min( value, *i );
        }
    }

    template <class IT_TYPE, class T>
    void Util::highest( IT_TYPE begin, IT_TYPE end, T &value )
    {
        value = T( -1e10 );
        IT_TYPE i = begin;
        for( ; i != end; ++i )
        {
            value = std::max( value, *i );
        }
    }

    template <class T>
    T Util::average( const Array<T> &v )
    {
        T sum = T( 0.0 );
        typename Array<T>::const_iterator it = v.begin();
        typename Array<T>::const_iterator endIt = v.end();
        for( ; it != endIt; ++it )
        {
            sum += *it;
        }

        return sum / v.size();
    }

    template <class T>
    T Util::average( const Deque<T> &v )
    {
        T sum = T( 0.0 );
        typename Deque<T>::const_iterator it = v.begin();
        typename Deque<T>::const_iterator endIt = v.end();
        for( ; it != endIt; ++it )
        {
            sum += *it;
        }

        return sum / static_cast<T>( v.size() );
    }

    template <class T>
    bool Util::eraseElement( Array<T> &array, const T &element )
    {
        auto iter = array.begin();
        for( ; iter != array.end(); ++iter )
        {
            if( element == ( *iter ) )
            {
                array.erase( iter );
                return true;
            }
        }

        return false;
    }

    inline s32 Util::compare( const void *a, const void *b )
    {
        auto int_a = *( (size_t *)a );
        auto int_b = *( (size_t *)b );

        if( int_a == int_b )
        {
            return 0;
        }
        if( int_a < int_b )
        {
            return -1;
        }

        return 1;
    }

    template <typename Iterator>
    bool Util::hasDuplicates( Iterator first, Iterator end )
    {
        for( auto i = first; i != end; ++i )
        {
            for( auto j = first; i != j; ++j )
            {
                if( *i == *j )
                {
                    return true;
                }
            }
        }
        return false;
    }

    template <typename Container>
    bool Util::hasDuplicates( const Container &v )
    {
        for( const auto &i : v )
        {
            for( const auto &j : v )
            {
                if( &i == &j )
                {
                    break;
                }

                if( i == j )
                {
                    return true;
                }
            }
        }
        return false;
    }

    template <class T>
    void Util::sortByName( Array<SmartPtr<T>> &vector )
    {
        std::sort( vector.begin(), vector.end(),
                   []( SmartPtr<T> a, SmartPtr<T> b ) -> bool { return a->getName() < b->getName(); } );
    }

    template <class T>
    bool Util::areAllValuesUnique( const std::vector<T> &vec )
    {
        const auto uniqueElements = Set<T>( vec.begin(), vec.end() );

        // If the size of the set is equal to the size of the vector, all values are unique
        return uniqueElements.size() == vec.size();
    }

    template <class T>
    Vector3<T> Util::getAverageVelocity( const Array<Transform3<T>> &positions, const Array<f64> &times )
    {
        auto averageVelocity = Vector3<T>( 0.0, 0.0, 0.0 );
        for( size_t i = 0; i < positions.size() - 1; i++ )
        {
            auto p0 = positions[i].getPosition();
            auto p1 = positions[i + 1].getPosition();
            auto velocity = ( p1 - p0 ) / ( times[i + 1] - times[i] );
            averageVelocity += velocity;
        }

        averageVelocity /= positions.size() - 1;
        return averageVelocity;
    }

}  // namespace workphone

#endif
