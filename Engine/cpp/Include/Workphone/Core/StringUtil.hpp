#ifndef _FBStringUtil_H_
#define _FBStringUtil_H_

#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector4.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Core/ColourI.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/UUID.hpp>

namespace workphone
{

    /**
     * @brief A template-based string utility class for converting and manipulating data types.
     * @tparam T The data type to be used as the base string type in the utility class.
     */
    template <class T>
    class WPCore_API StringUtility
    {
    public:
        /** @brief An empty string constant. */
        static const BaseString<T> EmptyString;

        /**
         * @brief Returns the default delimiter.
         * @return T The default delimiter.
         */
        static BaseString<T> default_delim();

        /**
         * @brief Returns an empty string of type T.
         * @return T The empty string.
         */
        static BaseString<T> blank();

        static std::basic_string<T> str( const BaseString<T> &valueStr );

        /**
         * @brief Compares two strings.
         * @param a The first string to compare.
         * @param b The second string to compare.
         * @param ignoreCase If true, the comparison is case-insensitive.
         * @return True if the strings are equal, false otherwise.
         */
        static bool isEqual( const BaseString<T> &a, const BaseString<T> &b, bool ignoreCase = false );

        /**
         * @brief Checks if a string is empty.
         * @param str The string to check.
         * @return True if the string is empty, false otherwise.
         */
        static bool isNullOrEmpty( const BaseString<T> &str );

        /**
         * @brief Checks if a string contains a substring.
         * @param str The string to search in.
         * @param value The substring to search for.
         * @return True if the substring is found, false otherwise.
         */
        static bool contains( const BaseString<T> &str, const BaseString<T> &value );

        /**
         * @brief Converts a boolean to a string.
         * @param value The boolean value to convert.
         * @return T The string representation of the boolean.
         */
        static BaseString<T> toString( bool value );

        /**
         * @brief Converts a string to a boolean.
         * @param value The string to convert.
         * @param defaultValue The value to return if parsing fails.
         * @return True if the string's value equals "true", "yes" or "1" (case-insensitive), false
         * otherwise.
         */
        static bool parseBool( const BaseString<T> &value, bool defaultValue = true );

        /**
         * @brief Converts a signed 32-bit integer to a string.
         * @param value The value to convert.
         * @return T The converted string.
         */
        static BaseString<T> toString( s32 value );

        /**
         * @brief Converts a signed 64-bit integer to a string.
         * @param value The value to convert.
         * @return T The converted string.
         */
        static BaseString<T> toString( s64 value );

        /**
         * @brief Converts a string to a signed 32-bit integer.
         * @param value The value to convert.
         * @param defaultValue The default value to return if the conversion fails.
         * @return The converted integer.
         */
        static s32 parseInt( const BaseString<T> &value, s32 defaultValue = 0 );

        /**
         * @brief Converts an unsigned 32-bit integer to a string.
         * @param value The value to convert.
         * @return T The converted string.
         */
        static BaseString<T> toString( u32 value );

#if defined WP_PLATFORM_WIN32
        /**
         * @brief Converts an unsigned 64-bit integer to a string.
         * @param value The value to convert.
         * @return T The converted string.
         */
        static BaseString<T> toString( u64 value );

        /**
         * @brief Converts an unsigned long integer to a string.
         * @param value The value to convert.
         * @return String The converted string.
         */
        static BaseString<T> toString( unsigned long int value );
#elif defined WP_PLATFORM_APPLE
        /**
         * @brief Converts an unsigned 64-bit integer to a string.
         * @param value The value to convert.
         * @return T The converted string.
         */
        static BaseString<T> toString( u64 value );

        /**
         * @brief Converts an unsigned long integer to a string.
         * @param value The value to convert.
         * @return String The converted string.
         */
        static BaseString<T> toString( unsigned long int value );
#elif defined WP_PLATFORM_LINUX
        /**
         * @brief Converts an unsigned 64-bit integer to a string.
         * @param value The value to convert.
         * @return T The converted string.
         */
        static BaseString<T> toString( u64 value );
#elif defined WP_PLATFORM_ANDROID
        /**
         * @brief Converts an unsigned 64-bit integer to a string.
         * @param value The value to convert.
         * @return T The converted string.
         */
        static BaseString<T> toString( u64 value );

        /**
         * @brief Converts an unsigned long integer to a string.
         * @param value The value to convert.
         * @return String The converted string.
         */
        static BaseString<T> toString( unsigned long int value );
#endif

        /**
         * @brief Converts a string to an unsigned 32-bit integer.
         * @param value The value to convert.
         * @param defaultValue The default value to return if the conversion fails.
         * @return The converted unsigned integer.
         */
        static u32 parseUInt( const BaseString<T> &value, u32 defaultValue = 0 );

        /**
         * @brief Converts a 32-bit float to a string.
         * @param value The value to convert.
         * @return T The converted string.
         */
        static BaseString<T> toString( f32 value );

        /**
         * @brief Converts a string to a 32-bit float.
         * @param value The value to convert.
         * @param defaultValue The default value to return if the conversion fails.
         * @return The converted float.
         */
        static f32 parseFloat( const BaseString<T> &value, f32 defaultValue = 0.0f );

        /**
         * @brief Converts a 64-bit float (double) to a string.
         * @param value The value to convert.
         * @return T The converted string.
         */
        static BaseString<T> toString( f64 value );

        /**
         * @brief Converts a string to a 64-bit float (double).
         * @param value The value to convert.
         * @param defaultValue The default value to return if the conversion fails.
         * @return The converted double.
         */
        static f64 parseDouble( const BaseString<T> &value, f64 defaultValue = 0.0 );

        /**
         * @brief Converts a 2D vector to a string.
         * @tparam B The underlying type of the vector components.
         * @param value The vector to convert.
         * @return T The converted string.
         */
        template <class B>
        static BaseString<T> toString( const Vector2<B> &value );

        /**
         * @brief Converts a string to a 2D vector.
         * @tparam B The underlying type of the vector components.
         * @param value The string to convert.
         * @param defaultValue The default value to return if conversion fails.
         * @return The parsed 2D vector.
         */
        template <class B>
        static Vector2<B> parseVector2( const BaseString<T> &value,
                                        const Vector2<B> &defaultValue = Vector2<B>::zero() );

        /**
         * @brief Converts a 3D vector to a string.
         * @tparam B The underlying type of the vector components.
         * @param value The vector to convert.
         * @return T The converted string.
         */
        template <class B>
        static BaseString<T> toString( const Vector3<B> &value );

        /**
         * @brief Converts a string to a 3D vector.
         * @tparam B The underlying type of the vector components.
         * @param value The string to convert.
         * @param defaultValue The default value to return if conversion fails.
         * @return The parsed 3D vector.
         */
        template <class B>
        static Vector3<B> parseVector3( const BaseString<T> &value,
                                        const Vector3<B> &defaultValue = Vector3<B>::zero() );

        /**
         * @brief Converts a string to a 3D vector using a specified delimiter.
         * @tparam B The underlying type of the vector components.
         * @param value The string to convert.
         * @param split The delimiter to split the string on.
         * @param defaultValue The default value to return if conversion fails.
         * @return The parsed 3D vector.
         */
        template <class B>
        static Vector3<B> parseVector3( const BaseString<T> &value, const BaseString<T> &split,
                                        const Vector3<B> &defaultValue = Vector3<B>::zero() );

        /**
         * @brief Converts a 4D vector to a string.
         * @tparam B The underlying type of the vector components.
         * @param value The vector to convert.
         * @return T The converted string.
         */
        template <class B>
        static BaseString<T> toString( const Vector4<B> &value );

        /**
         * @brief Converts a string to a 4D vector.
         * @tparam B The underlying type of the vector components.
         * @param value The string to convert.
         * @param defaultValue The default value to return if conversion fails.
         * @return The parsed 4D vector.
         */
        template <class B>
        static Vector4<B> parseVector4( const BaseString<T> &value,
                                        const Vector4<B> &defaultValue = Vector4<B>::zero() );

        /**
         * @brief Converts a string to a 4D vector using a specified delimiter.
         * @tparam B The underlying type of the vector components.
         * @param value The string to convert.
         * @param split The delimiter to split the string on.
         * @param defaultValue The default value to return if conversion fails.
         * @return The parsed 4D vector.
         */
        template <class B>
        static Vector4<B> parseVector4( const BaseString<T> &value, const BaseString<T> &split,
                                        const Vector4<B> &defaultValue = Vector4<B>::zero() );

        /**
         * @brief Converts a quaternion to a string.
         * @tparam B The underlying type of the quaternion components.
         * @param value The quaternion to convert.
         * @return T The converted string.
         */
        template <class B>
        static BaseString<T> toString( const Quaternion<B> &value );

        /**
         * @brief Converts a string to a quaternion.
         * @tparam B The underlying type of the quaternion components.
         * @param value The string to convert.
         * @param defaultValue The default value to return if conversion fails.
         * @return The parsed quaternion.
         */
        template <class B>
        static Quaternion<B> parseQuaternion(
            const BaseString<T> &value, const Quaternion<B> &defaultValue = Quaternion<B>::identity() );

        /**
         * @brief Converts a ColourI to a string.
         * @param colour The colour to convert.
         * @return T The string representation of the colour.
         */
        static BaseString<T> toString( const ColourI &colour );

        /**
         * @brief Converts a ColourF to a string.
         * @param colour The colour to convert.
         * @return T The string representation of the colour.
         */
        static BaseString<T> toString( const ColourF &colour );

        /**
         * @brief Parses a ColourI from a string.
         * @param value A string in an R,G,B,A format e.g. "255,255,255,255".
         * @return The parsed ColourI.
         */
        static ColourI parseColour( const BaseString<T> &value );

        /**
         * @brief Parses a ColourF from a string.
         * @param value A string in an R,G,B,A format.
         * @return The parsed ColourF.
         */
        static ColourF parseColourf( const BaseString<T> &value );

        /**
         * @brief Parses a formatted string into an Array of strings.
         * @param formatedString The string to parse.
         * @param[out] stringArray The array to populate with parsed strings.
         */
        static void parseArray( const BaseString<T> &formatedString, Array<BaseString<T>> &stringArray );

        /**
         * @brief Parses a formatted string into an Array of floats.
         * @param formatedString The string to parse.
         * @param[out] floatArray The array to populate with parsed floats.
         */
        static void parseArray( const BaseString<T> &formatedString, Array<f32> &floatArray );

        /**
         * @brief Converts an Array of strings to a formatted string.
         * @param stringArray The array of strings to convert.
         * @return A formatted string.
         */
        static BaseString<T> toString( const Array<BaseString<T>> &stringArray );

        /**
         * @brief Converts an Array of floats to a formatted string.
         * @param floatArray The array of floats to convert.
         * @return A formatted string.
         */
        static BaseString<T> toString( const Array<f32> &floatArray );

        /**
         * @brief Converts two Arrays of strings to a formatted string.
         * @param stringArray1 The first array of strings.
         * @param stringArray2 The second array of strings.
         * @return A formatted string.
         */
        static BaseString<T> toString( const Array<BaseString<T>> &stringArray1,
                                       const Array<BaseString<T>> &stringArray2 );

        /**
         * @brief Parses a formatted string into two Arrays of strings.
         * @param formatedString The string to parse.
         * @param[out] stringArray1 The first array to populate.
         * @param[out] stringArray2 The second array to populate.
         */
        static void parseMultiChoice( const BaseString<T> &formatedString,
                                      Array<BaseString<T>> &stringArray1,
                                      Array<BaseString<T>> &stringArray2 );

        /**
         * @brief Creates a hash value from the string.
         * @param str The input string.
         * @return A hash value.
         */
        static hash_type getHash( const BaseString<T> &str );

        /**
         * @brief Creates a hash value from the lower-case version of the string.
         * @param str The input string.
         * @return A hash value.
         */
        static hash_type getHashMakeLower( const BaseString<T> &str );

        /**
         * @brief Creates a 32-bit signed hash value from the string.
         * @param str The input string.
         * @return A 32-bit signed hash value.
         */
        static s32 getHashS32( const BaseString<T> &str );

        /**
         * @brief Creates a 32-bit signed hash value from the lower-case version of the string.
         * @param str The input string.
         * @return A 32-bit signed hash value.
         */
        static s32 getHashMakeLowerS32( const BaseString<T> &str );

        /**
         * @brief Creates a 32-bit hash value from the string.
         * @param str The input string.
         * @return A 32-bit hash value.
         */
        static hash32 getHash32( const BaseString<T> &str );

        /**
         * @brief Creates a 32-bit hash value from the lower-case version of the string.
         * @param str The input string.
         * @return A 32-bit hash value.
         */
        static hash32 getHashMakeLower32( const BaseString<T> &str );

        /**
         * @brief Creates a 64-bit hash value from the string.
         * @param str The input string.
         * @return A 64-bit hash value.
         */
        static hash64 getHash64( const BaseString<T> &str );

        /**
         * @brief Creates a 64-bit hash value from the lower-case version of the string.
         * @param str The input string.
         * @return A 64-bit hash value.
         */
        static hash64 getHashMakeLower64( const BaseString<T> &str );

        /**
         * @brief Converts a narrow string to a wide string.
         * @param str The narrow string to convert.
         * @return The converted wide string.
         */
        static StringW getWString( const String &str );

        /**
         * @brief Converts a wide string to a narrow string.
         * @param str The wide string to convert.
         * @return The converted narrow string.
         */
        static String toString( const StringW &str );

        /**
         * @brief Trims whitespace from the start of a string.
         * @param s The string to trim.
         * @return The trimmed string.
         */
        static BaseString<T> ltrim( const BaseString<T> &s );

        /**
         * @brief Trims whitespace from the end of a string.
         * @param s The string to trim.
         * @return The trimmed string.
         */
        static BaseString<T> rtrim( const BaseString<T> &s );

        /**
         * @brief Trims whitespace from both ends of a string.
         * @param s The string to trim.
         * @return The trimmed string.
         */
        static BaseString<T> trim( const BaseString<T> &s );

        /**
         * @brief Lower-cases all the characters in the string.
         * @param str The string to convert to lower case.
         * @return The lower-cased string.
         */
        static BaseString<T> make_lower( const BaseString<T> &str );

        /**
         * @brief Upper-cases all the characters in the string.
         * @param str The string to convert to upper case.
         * @return The upper-cased string.
         */
        static BaseString<T> make_upper( const BaseString<T> &str );

        /**
         * @brief Replaces all occurrences of a character in a string with another character.
         * @param str The string to perform the replacement on.
         * @param toReplace The character to replace.
         * @param replaceWith The character to replace with.
         * @return The modified string with characters replaced.
         */
        static BaseString<T> replace( const BaseString<T> &str, T toReplace, T replaceWith );

        /**
         * @brief Gets the current date and time as a string.
         * @return T The current date and time as a string.
         */
        static BaseString<T> getCurrentDateTime();

        /**
         * @brief Gets the current date and time as a string, with an optional display format.
         * @param bDisplayStr True to display the date and time in a human-readable format, false for
         * a machine-readable format.
         * @return The current date and time as a string.
         */
        static BaseString<T> getCurrentDateTime( bool bDisplayStr );

        /**
         * @brief Gets the current date and time as a string, with optional display and SQLite formats.
         * @param bDisplayStr True to display the date and time in a human-readable format, false for
         * a machine-readable format.
         * @param sqlLiteFormat True to return the date and time in SQLite format, false for a
         * standard format.
         * @return The current date and time as a string.
         */
        static BaseString<T> getCurrentDateTime( bool bDisplayStr, bool sqlLiteFormat );

        /**
         * @brief Gets the current time as a string.
         * @return The current time as a string.
         */
        static BaseString<T> getCurrentTime();

        /**
         * @brief Gets the current time as a string, with an optional display format.
         * @param bDisplayStr True to display the time in a human-readable format, false for a
         * machine-readable format.
         * @return The current time as a string.
         */
        static BaseString<T> getCurrentTime( bool bDisplayStr );

        /**
         * @brief Converts a UTF-16 wide string to a UTF-8 narrow string.
         * @param str The UTF-16 wide string to convert.
         * @return The converted UTF-8 narrow string.
         */
        static String toUTF16to8( const StringW &str );

        /**
         * @brief Converts a UTF-8 narrow string to a UTF-16 wide string.
         * @param str The UTF-8 narrow string to convert.
         * @return The converted UTF-16 wide string.
         */
        static StringW toUTF8to16( const String &str );

        /**
         * @brief Splits a string into an array of substrings based on start and end delimiters.
         * @param str The string to split.
         * @param startDelims The starting delimiters.
         * @param endDelims The ending delimiters.
         * @param maxSplits The maximum number of splits to perform (0 for no limit).
         * @param preserveDelims True to include delimiters in the result.
         * @return An array of substrings.
         */
        static Array<BaseString<T>> split( const BaseString<T> &str, const BaseString<T> &startDelims,
                                           const BaseString<T> &endDelims, u32 maxSplits = 0,
                                           bool preserveDelims = false );

        /**
         * @brief Splits a string into an array of substrings based on delimiters.
         * @param str The string to split.
         * @param delims The delimiters to split the string on.
         * @param maxSplits The maximum number of splits to perform (0 for no limit).
         * @param preserveDelims True to include delimiters in the result.
         * @return An array of substrings.
         */
        static Array<BaseString<T>> split( const BaseString<T> &str,
                                           const BaseString<T> &delims = default_delim(),
                                           u32 maxSplits = 0, bool preserveDelims = false );

        /**
         * @brief Replaces all occurrences of a substring in a string with another substring.
         * @param source The original string.
         * @param replaceWhat The substring to replace.
         * @param replaceWithWhat The substring to replace with.
         * @return The modified string.
         */
        static const BaseString<T> replaceAll( const BaseString<T> &source,
                                               const BaseString<T> &replaceWhat,
                                               const BaseString<T> &replaceWithWhat );

        /**
         * @brief Encodes a byte array into a Base64 string.
         * @param bytes_to_encode The byte array to encode.
         * @param in_len The length of the input byte array.
         * @return The encoded Base64 string.
         */
        static BaseString<T> encodeBase64( const u8 *bytes_to_encode, size_t in_len );

        /**
         * @brief Decodes a Base64 string into a byte array.
         * @param encoded_string The Base64 string to decode.
         * @return The decoded byte array as a string.
         */
        static BaseString<T> decodeBase64( const BaseString<T> &encoded_string );

        /**
         * @brief Parses a universally unique identifier (UUID) from a string.
         * @param str The string to parse the UUID from.
         * @return The parsed UUID.
         */
        static UUID parseUUID( const BaseString<T> &str );

        /**
         * @brief Converts a universally unique identifier (UUID) to a string.
         * @param uuid The UUID to convert.
         * @return The UUID as a string.
         */
        static BaseString<T> toString( const UUID &uuid );

        /**
         * @brief Generates a universally unique identifier (UUID) as a string.
         * @return The generated UUID as a string.
         */
        static BaseString<T> getUUID();

        /**
         * @brief Generates a universally unique identifier (UUID).
         * @return The generated UUID.
         */
        static UUID generateUUID();

        /**
         * @brief Generates a universally unique identifier (UUID) from a string.
         * @param str The string to generate the UUID from.
         * @return The generated UUID.
         */
        static UUID getUUID( const BaseString<T> &str );

        /**
         * @brief Converts a UTF-8 encoded string to a wide string.
         * @param str The UTF-8 encoded string to be converted.
         * @return The resulting wide string.
         */
        static StringW toStringW( String str );

        /**
         * @brief Converts a wide string to a UTF-8 encoded string.
         * @param str The wide string to be converted.
         * @return The resulting UTF-8 encoded string.
         */
        static String toStringC( StringW str );

        /**
         * @brief Parses a StringVector from a string.
         * @remarks Strings must not contain spaces since space is used as a delimiter.
         * @param value The string to parse.
         * @return An array of strings.
         */
        static Array<BaseString<T>> parseStringVector( const BaseString<T> &value );

        /**
         * @brief Checks if a string represents a valid number.
         * @param value The string to check.
         * @return True if the string is a number, false otherwise.
         */
        static bool isBoolean( const BaseString<T> &value );

        /**
         * @brief Checks if a string represents a valid number.
         * @param value The string to check.
         * @return True if the string is a number, false otherwise.
         */
        static bool isInteger( const BaseString<T> &value );

        /**
         * @brief Checks if a string represents a valid number.
         * @param value The string to check.
         * @return True if the string is a number, false otherwise.
         */
        static bool isNumber( const BaseString<T> &value );

        /**
         * @brief Checks if a character is a whitespace character.
         * @param c The character to check.
         * @return True if the character is a whitespace character, false otherwise.
         */
        static bool isSpace( T c );

        /**
         * @brief Checks if a character is a digit.
         * @param c The character to check.
         * @return True if the character is a digit, false otherwise.
         */
        static bool isDigit( T c );

        /**
         * @brief Splits a fully qualified file name into its base name and path.
         * @param qualifiedName The fully qualified file name.
         * @param[out] outBasename The base name of the file.
         * @param[out] outPath The path to the file.
         */
        static void splitFilename( const BaseString<T> &qualifiedName, BaseString<T> &outBasename,
                                   BaseString<T> &outPath );

        /**
         * @brief Splits a file name with extension into its base name and extension.
         * @param fullName The full file name including the extension.
         * @param[out] outBasename The base name of the file without extension.
         * @param[out] outExtention The file extension.
         */
        static void splitBaseFilename( const BaseString<T> &fullName, BaseString<T> &outBasename,
                                       BaseString<T> &outExtention );

        /**
         * @brief Splits a fully qualified file name into its base name, extension, and path.
         * @param qualifiedName The fully qualified file name.
         * @param[out] outBasename The base name of the file without extension.
         * @param[out] outExtention The file extension.
         * @param[out] outPath The path to the file.
         */
        static void splitFullFilename( const BaseString<T> &qualifiedName, BaseString<T> &outBasename,
                                       BaseString<T> &outExtention, BaseString<T> &outPath );

        /**
         * @brief Checks if a string matches a given pattern.
         * @param str The string to check.
         * @param pattern The pattern to match against.
         * @param caseSensitive True for case-sensitive matching (default).
         * @return True if the string matches the pattern, false otherwise.
         */
        static bool match( const BaseString<T> &str, const BaseString<T> &pattern,
                           bool caseSensitive = true );

        /**
         * @brief Cleans up a file path, resolving redundant separators and relative references.
         * @param path The file path to clean up.
         * @return The cleaned-up file path.
         */
        static BaseString<T> cleanupPath( const BaseString<T> &path );

        /**
         * @brief Copies the contents of a string into a specified buffer.
         * @param src The source string to copy.
         * @param[out] dst The destination buffer to copy the string into.
         * @param bufferSize The size of the destination buffer.
         */
        static void toBuffer( const BaseString<T> &src, void *dst, size_t bufferSize );

        /**
         * @brief Finds the longest common subsequence of two strings.
         * @param str1 The first string.
         * @param str2 The second string.
         * @return The longest common subsequence.
         */
        static BaseString<T> longestCommonSubsequence( const BaseString<T> &str1,
                                                       const BaseString<T> &str2 );

        /**
         * @brief Calculates the number of common characters in two strings.
         * @param str1 The first string.
         * @param str2 The second string.
         * @return The number of common characters.
         */
        static size_t numCommonSubsequence( const BaseString<T> &str1, const BaseString<T> &str2 );

        /**
         * @brief Counts the occurrences of a character in a string.
         * @param str The string to search in.
         * @param target The character to count.
         * @return The number of occurrences.
         */
        static s32 countMatchingCharacters( const BaseString<T> &str, T target );

        /**
         * @brief Counts the number of matching characters between two strings.
         * @param a The first string.
         * @param b The second string.
         * @return The number of matching characters.
         */
        static s32 countMatchingCharacters( const BaseString<T> &a, const BaseString<T> &b );

        /**
         * @brief Tokenizes an input string.
         * @param input The string to tokenize.
         * @return An array of tokens.
         */
        static Array<BaseString<T>> tokenize( const BaseString<T> &input );

        /**
         * @brief Extracts named entities from a list of tokens using a simple keyword approach.
         * @param tokens The list of tokens to process.
         * @param entities The list of entities to search for.
         * @return An array of extracted named entities.
         */
        static Array<BaseString<T>> extractNamedEntities( const Array<BaseString<T>> &tokens,
                                                          const Array<BaseString<T>> &entities );

        /**
         * @brief Calculates the Levenshtein distance between two strings.
         * @param s1 The first string.
         * @param s2 The second string.
         * @return The Levenshtein distance between the two strings.
         */
        static s32 levenshteinDistance( const BaseString<T> &s1, const BaseString<T> &s2 );
    };

    using StringUtil = StringUtility<c8>;
    using StringUtilW = StringUtility<wchar_t>;

}  // namespace workphone

#endif
