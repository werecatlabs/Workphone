/**
 * @file StringTypes.hpp
 * @brief Core string type definitions and utilities for the WorkPhone engine
 */

#ifndef StringTypes_h__
#define StringTypes_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Allocator.hpp>
#include <Workphone/Core/StringBase.hpp>

#if !WP_USE_STANDARD_STRING_ALLOCATOR
#    include <Workphone/Core/StringPoolAllocator.hpp>
#endif

#include <atomic>
#include <map>
#include <sstream>
#include <string>

/**
 * @namespace workphone
 * @brief Main namespace for the WorkPhone engine
 */
namespace workphone
{

#if WP_USE_WP_STRING
#    if WP_USE_STANDARD_STRING_ALLOCATOR
    typedef std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> StringStream;
    typedef StringStream stringstream;

    // Use standard string types with default allocator
    template <class T>
    using BaseString = StringBase<T, std::char_traits<T>, std::allocator<T>>;

    using StringPtr = c8 *;
    using AtomicStringPtr = std::atomic<c8 *>;
    using StringWPtr = wchar_t *;
    using AtomicStringWPtr = std::atomic<wchar_t *>;

    /// @brief Standard string type
    using String = StringBase<char, std::char_traits<char>, std::allocator<char>>;

    /// @brief Wide string type
    using StringW = StringBase<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>;

    /// @brief Map type for storing name-value pairs using String type
    using NameValuePairList = std::map<String, String>;

    /// @brief Pair of standard strings
    using StringPair = std::pair<String, String>;

    /// @brief Array of string pairs
    using StringPairs = Array<StringPair>;

    /// @brief Array of standard strings
    using Strings = Array<String>;
#    else
    typedef std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> StringStream;
    typedef StringStream stringstream;

    /// @brief Base string template
    template <class T>
    using BaseString = std::basic_string<T, std::char_traits<T>, std::allocator<T>>;

    using StringPtr = c8 *;
    using AtomicStringPtr = std::atomic<c8 *>;
    using StringWPtr = wchar_t *;
    using AtomicStringWPtr = std::atomic<wchar_t *>;

    /// @brief Standard string type
    using String = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;

    /// @brief Wide string type
    using StringW = std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>;

    /// @brief Map type for storing name-value pairs using String type
    using NameValuePairList = std::map<String, String>;

    /// @brief Pair of standard strings
    using StringPair = std::pair<String, String>;

    /// @brief Array of string pairs
    using StringPairs = Array<StringPair>;

    /// @brief Array of standard strings
    using Strings = Array<String>;
#    endif
#else
#    if WP_USE_STANDARD_STRING_ALLOCATOR
    typedef std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> StringStream;
    typedef StringStream stringstream;

    // Use standard string types with default allocator
    template <class T>
    using BaseString = std::basic_string<T, std::char_traits<T>, std::allocator<T>>;

    using StringPtr = c8 *;
    using AtomicStringPtr = std::atomic<c8 *>;
    using StringWPtr = wchar_t *;
    using AtomicStringWPtr = std::atomic<wchar_t *>;

    /// @brief Standard string type
    using String = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;

    /// @brief Wide string type
    using StringW = std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>;

    /// @brief Map type for storing name-value pairs using String type
    using NameValuePairList = std::map<String, String>;

    /// @brief Pair of standard strings
    using StringPair = std::pair<String, String>;

    /// @brief Array of string pairs
    using StringPairs = Array<StringPair>;

    /// @brief Array of standard strings
    using Strings = Array<String>;
#    else
    typedef std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> StringStream;
    typedef StringStream stringstream;

    /// @brief Base string template
    template <class T>
    using BaseString = std::basic_string<T, std::char_traits<T>, std::allocator<T>>;

    using StringPtr = c8 *;
    using AtomicStringPtr = std::atomic<c8 *>;
    using StringWPtr = wchar_t *;
    using AtomicStringWPtr = std::atomic<wchar_t *>;

    /// @brief Standard string type
    using String = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;

    /// @brief Wide string type
    using StringW = std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>;

    /// @brief Map type for storing name-value pairs using String type
    using NameValuePairList = std::map<String, String>;

    /// @brief Pair of standard strings
    using StringPair = std::pair<String, String>;

    /// @brief Array of string pairs
    using StringPairs = Array<StringPair>;

    /// @brief Array of standard strings
    using Strings = Array<String>;
#    endif
#endif

}  // namespace workphone

#endif  // StringTypes_h__
